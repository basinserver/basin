//
// Created by p on 4/14/19.
//

#include "login_stage_handler.h"
#include <basin/connection.h>
#include <basin/network.h>
#include <basin/packet.h>
#include <basin/globals.h>
#include <basin/version.h>
#include <avuna/string.h>
#include <openssl/ssl.h>
#include <netdb.h>

int handle_packet_handshake(struct connection* conn, struct packet* packet) {
    conn->host_ip = str_dup(packet->data.handshake_server.handshake.server_address, 0, conn->pool);
    conn->host_port = packet->data.handshake_server.handshake.server_port;
    conn->protocolVersion = (uint32_t) packet->data.handshake_server.handshake.protocol_version;
    if ((packet->data.handshake_server.handshake.protocol_version < MC_PROTOCOL_VERSION_MIN || packet->data.handshake_server.handshake.protocol_version > MC_PROTOCOL_VERSION_MAX) && packet->data.handshake_server.handshake.next_state != STATE_STATUS) return -2;
    if (packet->data.handshake_server.handshake.next_state == STATE_STATUS) {
        conn->protocol_state = STATE_STATUS;
    } else if (packet->data.handshake_server.handshake.next_state == STATE_LOGIN) {
        conn->protocol_state = STATE_LOGIN;
    } else return 1;
    return 0;
}

int handle_packet_status(struct connection* conn, struct packet* packet) {
    if (packet->id == PKT_STATUS_SERVER_REQUEST) {
        struct packet* resp = packet_new(packet->pool, PKT_STATUS_CLIENT_RESPONSE);
        resp->data.status_client.response.json_response = pmalloc(packet->pool, 1000);
        resp->data.status_client.response.json_response[999] = 0;
        snprintf(resp->data.status_client.response.json_response, 999, "{\"version\":{\"name\":\"1.11.2\",\"protocol\":%i},\"players\":{\"max\":%lu,\"online\":%lu},\"description\":{\"text\":\"%s\"}}", MC_PROTOCOL_VERSION_MIN, conn->server->max_players, conn->server->players->entry_count, conn->server->motd);
        if (packet_write(conn, resp) < 0) return 1;
    } else if (packet->id == PKT_STATUS_SERVER_PING) {
        struct packet* resp = packet_new(packet->pool, PKT_STATUS_CLIENT_PONG);
        if (packet_write(conn, resp) < 0) return 1;
        conn->disconnect = 1;
    } else return 1;
    return 0;
}

void encrypt_free(EVP_CIPHER_CTX* ctx) {
    unsigned char final[256];
    int throw_away = 0;
    EVP_EncryptFinal_ex(ctx, final, &throw_away);
    EVP_CIPHER_CTX_free(ctx);
}

void decrypt_free(EVP_CIPHER_CTX* ctx) {
    unsigned char final[256];
    int throw_away = 0;
    EVP_DecryptFinal_ex(ctx, final, &throw_away);
    EVP_CIPHER_CTX_free(ctx);
}

int handle_encryption_response(struct connection* conn, struct packet* packet) {
    if (conn->verifyToken == 0 || packet->data.login_server.encryptionresponse.shared_secret_length > 162 || packet->data.login_server.encryptionresponse.verify_token_length > 162) goto rete;
    unsigned char decSecret[162];
    int secLen = RSA_private_decrypt(packet->data.login_server.encryptionresponse.shared_secret_length, packet->data.login_server.encryptionresponse.shared_secret, decSecret, public_rsa, RSA_PKCS1_PADDING);
    if (secLen != 16) goto rete;
    unsigned char decVerifyToken[162];
    int vtLen = RSA_private_decrypt(packet->data.login_server.encryptionresponse.verify_token_length, packet->data.login_server.encryptionresponse.verify_token, decVerifyToken, public_rsa, RSA_PKCS1_PADDING);
    if (vtLen != 4) goto rete;
    uint32_t vt = *((uint32_t*) decVerifyToken);
    if (vt != conn->verifyToken) goto rete;
    memcpy(conn->shared_secret, decSecret, 16);
    uint8_t pubkey[162];
    memcpy(pubkey, public_rsa_publickey, 162);
    uint8_t hash[20];
    SHA_CTX context;
    SHA1_Init(&context);
    SHA1_Update(&context, decSecret, 16);
    SHA1_Update(&context, pubkey, 162);
    SHA1_Final(hash, &context);
    int m = 0;
    if (hash[0] & 0x80) {
        m = 1;
        for (int i = 0; i < 20; i++) {
            hash[i] = ~hash[i];
        }
        (hash[19])++;
    }
    char fhash[32];
    char* fhash2 = fhash + 1;
    for (int i = 0; i < 20; i++) {
        snprintf(fhash + 1 + (i * 2), 3, "%02X", hash[i]);
    }
    for (int i = 1; i < 41; i++) {
        if (fhash[i] == '0') fhash2++;
        else break;
    }
    fhash2--;
    if (m) fhash2[0] = '-';
    else fhash2++;
    struct sockaddr_in sin;
    struct hostent *host = gethostbyname("sessionserver.mojang.com");
    if (host != NULL) {
        struct in_addr **adl = (struct in_addr **) host->h_addr_list;
        if (adl[0] != NULL) {
            sin.sin_addr.s_addr = adl[0]->s_addr;
        } else goto merr;
    } else goto merr;
    sin.sin_port = htons(443);
    sin.sin_family = AF_INET;
    int mfd = socket(AF_INET, SOCK_STREAM, 0);
    SSL* sct = NULL;
    int initssl = 0;
    int val = 0;
    if (mfd < 0) goto merr;
    if (connect(mfd, (struct sockaddr*) &sin, sizeof(struct sockaddr_in))) goto merr;
    sct = SSL_new(mojang_ctx);
    SSL_set_connect_state(sct);
    SSL_set_fd(sct, mfd);
    if (SSL_connect(sct) != 1) goto merr;
    else initssl = 1;
    char cbuf[4096];
    int tl = snprintf(cbuf, 1024, "GET /session/minecraft/hasJoined?username=%s&serverId=%s HTTP/1.1\r\nHost: sessionserver.mojang.com\r\nUser-Agent: Basin " VERSION "\r\nConnection: close\r\n\r\n", conn->online_username, fhash2);
    int tw = 0;
    while (tw < tl) {
        int r = SSL_write(sct, cbuf, tl);
        if (r <= 0) goto merr;
        else tw += r;
    }
    tw = 0;
    int r = 0;
    while ((r = SSL_read(sct, cbuf + tw, 4095 - tw)) > 0) {
        tw += r;
    }
    cbuf[tw] = 0;
    char* data = strstr(cbuf, "\r\n\r\n");
    if (data == NULL) goto merr;
    data += 4;
    struct json_object json;
    json_parse(&json, data);
    struct json_object* tmp = json_get(&json, "id");
    if (tmp == NULL || tmp->type != JSON_STRING) {
        freeJSON(&json);
        goto merr;
    }
    char* id = trim(tmp->data.string);
    tmp = json_get(&json, "name");
    if (tmp == NULL || tmp->type != JSON_STRING) {
        freeJSON(&json);
        goto merr;
    }
    char* name = trim(tmp->data.string);
    size_t sl = strlen(name);
    if (sl < 2 || sl > 16) {
        freeJSON(&json);
        goto merr;
    }
    BEGIN_HASHMAP_ITERATION (players)
    struct player* player = (struct player*) value;
    if (streq_nocase(name, player->name)) {
        kickPlayer(player, "You have logged in from another location!");
        goto pbn2;
    }
    END_HASHMAP_ITERATION (players)
    pbn2: ;
    val = 1;
    conn->aes_ctx_enc = EVP_CIPHER_CTX_new();
    if (conn->aes_ctx_enc == NULL) goto rete;
    //EVP_CIPHER_CTX_set_padding(conn->aes_ctx_enc, 0);
    if (EVP_EncryptInit_ex(conn->aes_ctx_enc, EVP_aes_128_cfb8(), NULL, conn->sharedSecret, conn->sharedSecret) != 1) goto rete;
    if (conn->aes_ctx_enc != NULL) {
        phook(conn->pool, encrypt_free, conn->aes_ctx_enc);
    }

    conn->aes_ctx_dec = EVP_CIPHER_CTX_new();
    if (conn->aes_ctx_dec == NULL) goto rete;
    //EVP_CIPHER_CTX_set_padding(conn->aes_ctx_dec, 0);
    if (EVP_DecryptInit_ex(conn->aes_ctx_dec, EVP_aes_128_cfb8(), NULL, conn->sharedSecret, conn->sharedSecret) != 1) goto rete;
    if (conn->aes_ctx_dec != NULL) {
        phook(conn->pool, decrypt_free, conn->aes_ctx_dec);
    }

    if (work_joinServer(conn, name, id)) {
        freeJSON(&json);
        if (initssl) SSL_shutdown(sct);
        if (mfd >= 0) close(mfd);
        if (sct != NULL) SSL_free(sct);
        goto rete;
    }
    freeJSON(&json);
    merr: ;
    if (initssl) SSL_shutdown(sct);
    if (mfd >= 0) close(mfd);
    if (sct != NULL) SSL_free(sct);
    if (!val) {
        rep.id = PKT_LOGIN_CLIENT_DISCONNECT;
        rep.data.login_client.disconnect.reason = xstrdup("{\"text\": \"There was an unresolvable issue with the Mojang sessionserver! Please try again.\"}", 0);
        if (packet_write(conn, &rep) < 0) goto rete;
        conn->disconnect = 1;
        goto ret;
    }
}

int handle_login_start(struct connection* conn, struct packet* packet) {
    if (conn->server->online_mode) {
        if (conn->verifyToken) {
            return 1;
        }
        conn->online_username = pxfer(packet->pool, conn->pool, packet->data.login_server.loginstart.name);
        struct packet* resp = packet_new(packet->pool, PKT_LOGIN_CLIENT_ENCRYPTIONREQUEST);
        resp->data.login_client.encryptionrequest.server_id = "";
        resp->data.login_client.encryptionrequest.public_key = public_rsa_publickey;
        resp->data.login_client.encryptionrequest.public_key_length = 162;
        conn->verifyToken = rand(); // TODO: better RNG
        if (conn->verifyToken == 0) conn->verifyToken = 1;
        resp->data.login_client.encryptionrequest.verify_token = (uint8_t*) &conn->verifyToken;
        resp->data.login_client.encryptionrequest.verify_token_length = 4;
        if (packet_write(conn, resp) < 0) {
            return 1;
        }
    } else {
        int bad_name = 0;
        char* name_trimmed = str_trim(packet->data.login_server.loginstart.name);
        size_t trimmed_length = strlen(name_trimmed);
        if (trimmed_length > 16 || trimmed_length < 2) bad_name = 2;
        if (!bad_name) {
            BEGIN_HASHMAP_ITERATION (players)
            struct player* player = (struct player*) value;
            if (streq_nocase(name_trimmed, player->name)) {
                bad_name = 1;
                goto pbn;
            }
            END_HASHMAP_ITERATION (players)
            pbn: ;
        }
        if (bad_name) {
            struct packet* resp = packet_new(packet->pool, PKT_LOGIN_CLIENT_DISCONNECT);
            resp->id = PKT_LOGIN_CLIENT_DISCONNECT;
            resp->data.login_client.disconnect.reason = bad_name == 2 ? "{\"text\": \"Invalid name!\"}" : "{\"text\", \"You are already in the server!\"}";
            if (packet_write(conn, resp) < 0) {
                return 1;
            }
            conn->disconnect = 1;
            return 0;
        }
        if (work_joinServer(conn, name_trimmed, NULL)) goto rete;
    }
}


int handle_packet_login(struct connection* conn, struct packet* packet) {
    if (packet->id == PKT_LOGIN_SERVER_ENCRYPTIONRESPONSE) {
        return handle_encryption_response(conn, packet);
    } else if (packet->id == PKT_LOGIN_SERVER_LOGINSTART) {
        return handle_login_start(conn, packet);
    } else return 1;
    return 0;
}

