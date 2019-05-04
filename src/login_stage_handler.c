#include "login_stage_handler.h"
#include <basin/connection.h>
#include <basin/network.h>
#include <basin/player.h>
#include <basin/packet.h>
#include <basin/globals.h>
#include <basin/version.h>
#include <basin/game.h>
#include <avuna/string.h>
#include <avuna/json.h>
#include <avuna/util.h>
#include <avuna/pmem_hooks.h>
#include <openssl/ssl.h>
#include <openssl/md5.h>
#include <netdb.h>
#include <arpa/inet.h>


int work_joinServer(struct connection* conn, struct mempool* pool, char* username, char* uuid_string) {
    struct uuid uuid;
    unsigned char* uuidx = (unsigned char*) &uuid;
    if (uuid_string == NULL) {
        if (conn->server->online_mode) return 1;
        MD5_CTX context;
        MD5_Init(&context);
        MD5_Update(&context, username, strlen(username));
        MD5_Final(uuidx, &context);
    } else {
        if (strlen(uuid_string) != 32) return 1;
        char ups[9];
        memcpy(ups, uuid_string, 8);
        ups[8] = 0;
        uuid.uuid1 = ((uint64_t) strtoll(ups, NULL, 16)) << 32;
        memcpy(ups, uuid_string + 8, 8);
        uuid.uuid1 |= ((uint64_t) strtoll(ups, NULL, 16)) & 0xFFFFFFFF;
        memcpy(ups, uuid_string + 16, 8);
        uuid.uuid2 = ((uint64_t) strtoll(ups, NULL, 16)) << 32;
        memcpy(ups, uuid_string + 24, 8);
        uuid.uuid2 |= ((uint64_t) strtoll(ups, NULL, 16)) & 0xFFFFFFFF;
    }
    struct packet* resp = packet_new(pool, PKT_LOGIN_CLIENT_LOGINSUCCESS);
    resp->data.login_client.loginsuccess.username = username;
    resp->data.login_client.loginsuccess.uuid = pmalloc(pool, 38);
    snprintf(resp->data.login_client.loginsuccess.uuid, 10, "%08X-", ((uint32_t*) uuidx)[0]);
    snprintf(resp->data.login_client.loginsuccess.uuid + 9, 6, "%04X-", ((uint16_t*) uuidx)[2]);
    snprintf(resp->data.login_client.loginsuccess.uuid + 14, 6, "%04X-", ((uint16_t*) uuidx)[3]);
    snprintf(resp->data.login_client.loginsuccess.uuid + 19, 6, "%04X-", ((uint16_t*) uuidx)[4]);
    snprintf(resp->data.login_client.loginsuccess.uuid + 24, 9, "%08X", ((uint32_t*) (uuidx + 4))[2]);
    snprintf(resp->data.login_client.loginsuccess.uuid + 32, 5, "%04X", ((uint16_t*) uuidx)[7]);
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    conn->protocol_state = STATE_PLAY;
    struct entity* ep = newEntity(nextEntityID++, (double) conn->server->overworld->spawnpos.x + .5, (double) conn->server->overworld->spawnpos.y, (double) conn->server->overworld->spawnpos.z + .5, ENT_PLAYER, 0., 0.);
    struct player* player = player_new(ep, str_dup(resp->data.login_client.loginsuccess.username, 1, pool), uuid, conn,
                                       0); // TODO default gamemode
    player->protocol_version = conn->protocol_version;
    conn->player = player;
    hashmap_putint(conn->server->players_by_entity_id, (uint64_t) player->entity->id, player);
    resp->id = PKT_PLAY_CLIENT_JOINGAME;
    resp->data.play_client.joingame.entity_id = ep->id;
    resp->data.play_client.joingame.gamemode = player->gamemode;
    resp->data.play_client.joingame.dimension = conn->server->overworld->dimension;
    resp->data.play_client.joingame.difficulty = (uint8_t) conn->server->difficulty;
    resp->data.play_client.joingame.max_players = (uint8_t) conn->server->max_players;
    resp->data.play_client.joingame.level_type = conn->server->overworld->level_type;
    resp->data.play_client.joingame.reduced_debug_info = 0; // TODO
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    resp->id = PKT_PLAY_CLIENT_PLUGINMESSAGE;
    resp->data.play_client.pluginmessage.channel = "MC|Brand";
    resp->data.play_client.pluginmessage.data = pmalloc(pool, 16);
    size_t str_length_length = (size_t) writeVarInt(5, (unsigned char*) resp->data.play_client.pluginmessage.data);
    memcpy(resp->data.play_client.pluginmessage.data + str_length_length, "Basin", 5);
    resp->data.play_client.pluginmessage.data_size = str_length_length + 5;
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    resp->id = PKT_PLAY_CLIENT_SERVERDIFFICULTY;
    resp->data.play_client.serverdifficulty.difficulty = (uint8_t) conn->server->difficulty;
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    resp->id = PKT_PLAY_CLIENT_SPAWNPOSITION;
    memcpy(&resp->data.play_client.spawnposition.location, &conn->server->overworld->spawnpos, sizeof(struct encpos));
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    resp->id = PKT_PLAY_CLIENT_PLAYERABILITIES;
    resp->data.play_client.playerabilities.flags = 0; // TODO: allows flying, remove
    resp->data.play_client.playerabilities.flying_speed = 0.05;
    resp->data.play_client.playerabilities.field_of_view_modifier = .1;
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    resp->id = PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK;
    resp->data.play_client.playerpositionandlook.x = ep->x;
    resp->data.play_client.playerpositionandlook.y = ep->y;
    resp->data.play_client.playerpositionandlook.z = ep->z;
    resp->data.play_client.playerpositionandlook.yaw = ep->yaw;
    resp->data.play_client.playerpositionandlook.pitch = ep->pitch;
    resp->data.play_client.playerpositionandlook.flags = 0x0;
    resp->data.play_client.playerpositionandlook.teleport_id = 0;
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    resp->id = PKT_PLAY_CLIENT_PLAYERLISTITEM;
    pthread_rwlock_rdlock(&conn->server->players_by_entity_id->rwlock);
    resp->data.play_client.playerlistitem.action_id = 0;
    resp->data.play_client.playerlistitem.number_of_players = (int32_t) (conn->server->players_by_entity_id->entry_count + 1);
    resp->data.play_client.playerlistitem.players = pmalloc(pool, resp->data.play_client.playerlistitem.number_of_players * sizeof(struct listitem_player));
    size_t players_written = 0;
    ITER_MAP(conn->server->players_by_entity_id) {
        struct player* iter_player = (struct player*) value;
        if (players_written < resp->data.play_client.playerlistitem.number_of_players) {
            memcpy(&resp->data.play_client.playerlistitem.players[players_written].uuid, &iter_player->uuid, sizeof(struct uuid));
            resp->data.play_client.playerlistitem.players[players_written].action.addplayer.name = iter_player->name;
            resp->data.play_client.playerlistitem.players[players_written].action.addplayer.number_of_properties = 0;
            resp->data.play_client.playerlistitem.players[players_written].action.addplayer.properties = NULL;
            resp->data.play_client.playerlistitem.players[players_written].action.addplayer.gamemode = iter_player->gamemode;
            resp->data.play_client.playerlistitem.players[players_written].action.addplayer.ping = 0; // TODO
            resp->data.play_client.playerlistitem.players[players_written].action.addplayer.has_display_name = 0;
            resp->data.play_client.playerlistitem.players[players_written].action.addplayer.display_name = NULL;
            players_written++;
        }
        if (player == iter_player) continue;
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_PLAYERLISTITEM);
        pkt->data.play_client.playerlistitem.action_id = 0;
        pkt->data.play_client.playerlistitem.number_of_players = 1;
        pkt->data.play_client.playerlistitem.players = pmalloc(pkt->pool, sizeof(struct listitem_player));
        memcpy(&pkt->data.play_client.playerlistitem.players->uuid, &player->uuid, sizeof(struct uuid));
        pkt->data.play_client.playerlistitem.players->action.addplayer.name = str_dup(player->name, 0, pkt->pool);
        pkt->data.play_client.playerlistitem.players->action.addplayer.number_of_properties = 0;
        pkt->data.play_client.playerlistitem.players->action.addplayer.properties = NULL;
        pkt->data.play_client.playerlistitem.players->action.addplayer.gamemode = player->gamemode;
        pkt->data.play_client.playerlistitem.players->action.addplayer.ping = 0; // TODO
        pkt->data.play_client.playerlistitem.players->action.addplayer.has_display_name = 0;
        pkt->data.play_client.playerlistitem.players->action.addplayer.display_name = NULL;
        queue_push(iter_player->outgoing_packets, pkt);
        flush_outgoing(iter_player);
        ITER_MAP_END();
    }
    pthread_rwlock_unlock(&conn->server->players_by_entity_id->rwlock);
    memcpy(&resp->data.play_client.playerlistitem.players[players_written].uuid, &player->uuid, sizeof(struct uuid));
    resp->data.play_client.playerlistitem.players[players_written].action.addplayer.name = player->name;
    resp->data.play_client.playerlistitem.players[players_written].action.addplayer.number_of_properties = 0;
    resp->data.play_client.playerlistitem.players[players_written].action.addplayer.properties = NULL;
    resp->data.play_client.playerlistitem.players[players_written].action.addplayer.gamemode = player->gamemode;
    resp->data.play_client.playerlistitem.players[players_written].action.addplayer.ping = 0; // TODO
    resp->data.play_client.playerlistitem.players[players_written].action.addplayer.has_display_name = 0;
    resp->data.play_client.playerlistitem.players[players_written].action.addplayer.display_name = NULL;
    players_written++;
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    resp->id = PKT_PLAY_CLIENT_TIMEUPDATE;
    resp->data.play_client.timeupdate.time_of_day = conn->server->overworld->time;
    resp->data.play_client.timeupdate.world_age = conn->server->overworld->age;
    if (packet_write(conn, resp) < 0) {
        return 1;
    }
    add_collection(playersToLoad, player);
    broadcastf("yellow", "%s has joined the server!", player->name);
    const char* ip_string = NULL;
    char tip[48];
    if (conn->addr.in6.sin6_family == AF_INET) {
        ip_string = inet_ntop(AF_INET, &conn->addr.in.sin_addr, tip, 48);
    } else if (conn->addr.in6.sin6_family == AF_INET6) {
        if (memseq((unsigned char*) &conn->addr.in6.sin6_addr, 10, 0) && memseq((unsigned char*) &conn->addr.in6.sin6_addr + 10, 2, 0xff)) {
            ip_string = inet_ntop(AF_INET, ((unsigned char*) &conn->addr.in6.sin6_addr) + 12, tip, 48);
        } else ip_string = inet_ntop(AF_INET6, &conn->addr.in6.sin6_addr, tip, 48);
    } else {
        ip_string = "UNKNOWN";
    }
    printf("Player '%s' has joined with IP '%s'\n", player->name, ip_string);
    return 0;
}

int handle_packet_handshake(struct connection* conn, struct packet* packet) {
    conn->host_ip = str_dup(packet->data.handshake_server.handshake.server_address, 0, conn->pool);
    conn->host_port = packet->data.handshake_server.handshake.server_port;
    conn->protocol_version = (uint32_t) packet->data.handshake_server.handshake.protocol_version;
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
        snprintf(resp->data.status_client.response.json_response, 999, "{\"version\":{\"name\":\"1.11.2\",\"protocol\":%i},\"players_by_entity_id\":{\"max\":%lu,\"online\":%lu},\"description\":{\"text\":\"%s\"}}", MC_PROTOCOL_VERSION_MIN, conn->server->max_players, conn->server->players_by_entity_id->entry_count, conn->server->motd);
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
    if (conn->verifyToken == 0 || packet->data.login_server.encryptionresponse.shared_secret_length > 162 || packet->data.login_server.encryptionresponse.verify_token_length > 162) {
        return 1;
    }
    unsigned char shared_secret[162];
    int secret_length = RSA_private_decrypt(packet->data.login_server.encryptionresponse.shared_secret_length, packet->data.login_server.encryptionresponse.shared_secret, shared_secret, public_rsa, RSA_PKCS1_PADDING);
    if (secret_length != 16) {
        return 1;
    }
    unsigned char verify_token[162];
    int verify_token_length = RSA_private_decrypt(packet->data.login_server.encryptionresponse.verify_token_length, packet->data.login_server.encryptionresponse.verify_token, verify_token, public_rsa, RSA_PKCS1_PADDING);
    if (verify_token_length != 4) {
        return 1;
    }
    uint32_t verify_token_int = *((uint32_t*) verify_token);
    if (verify_token_int != conn->verifyToken) goto rete;
    memcpy(conn->shared_secret, shared_secret, 16);
    uint8_t public_key[162];
    memcpy(public_key, public_rsa_publickey, 162);
    uint8_t hash[20];
    SHA_CTX context;
    SHA1_Init(&context);
    SHA1_Update(&context, shared_secret, 16);
    SHA1_Update(&context, public_key, 162);
    SHA1_Final(hash, &context);
    int is_signed = 0;
    if (hash[0] & 0x80) {
        is_signed = 1;
        for (int i = 0; i < 20; i++) {
            hash[i] = ~hash[i];
        }
        (hash[19])++;
    }
    char hex_hash[32];
    char* hex_hash_signed = hex_hash + 1;
    for (int i = 0; i < 20; i++) {
        snprintf(hex_hash + 1 + (i * 2), 3, "%02X", hash[i]);
    }
    for (int i = 1; i < 41; i++) {
        if (hex_hash[i] == '0') hex_hash_signed++;
        else break;
    }
    hex_hash_signed--;
    struct mempool* ssl_pool = mempool_new();
    if (is_signed) hex_hash_signed[0] = '-';
    else hex_hash_signed++;
    struct sockaddr_in sin;
    //TODO: use getaddrinfo
    struct hostent *host = gethostbyname("sessionserver.mojang.com");
    if (host != NULL) {
        struct in_addr **adl = (struct in_addr **) host->h_addr_list;
        if (adl[0] != NULL) {
            sin.sin_addr.s_addr = adl[0]->s_addr;
        } else goto ssl_error;
    } else goto ssl_error;
    sin.sin_port = htons(443);
    sin.sin_family = AF_INET;
    int session_tls_fd = socket(AF_INET, SOCK_STREAM, 0);
    SSL* ssl = NULL;
    if (session_tls_fd < 0) goto ssl_error;
    phook(ssl_pool, close_hook, (void*) session_tls_fd);
    if (connect(session_tls_fd, (struct sockaddr*) &sin, sizeof(struct sockaddr_in))) goto ssl_error;
    ssl = SSL_new(mojang_ctx);
    phook(ssl_pool, (void (*)(void*)) SSL_shutdown, ssl);
    phook(ssl_pool, (void (*)(void*)) SSL_free, ssl);
    SSL_set_connect_state(ssl);
    SSL_set_fd(ssl, session_tls_fd);
    if (SSL_connect(ssl) != 1) goto ssl_error;
    char write_buf[4096];
    int write_length = snprintf(write_buf, 1024, "GET /session/minecraft/hasJoined?username=%s&serverId=%s HTTP/1.1\r\nHost: sessionserver.mojang.com\r\nUser-Agent: Basin " VERSION "\r\nConnection: close\r\n\r\n", conn->online_username, hex_hash_signed);
    int written = 0;
    while (written < write_length) {
        int r = SSL_write(ssl, write_buf, write_length);
        if (r <= 0) goto ssl_error;
        else written += r;
    }
    int read = 0;
    int r = 0;
    while ((r = SSL_read(ssl, write_buf + read, 4095 - read)) > 0) { // we reuse write_buf as read_buf here
        read += r;
    }
    write_buf[read] = 0;
    char* data = strstr(write_buf, "\r\n\r\n");
    if (data == NULL) goto ssl_error;
    data += 4;
    struct json_object* json = json_make_object(ssl_pool, NULL, 0);
    json_parse(ssl_pool, &json, data);
    struct json_object* tmp = json_get(&json, "id");
    if (tmp == NULL || tmp->type != JSON_STRING) {
        goto ssl_error;
    }
    char* id = str_trim(tmp->data.string);
    tmp = json_get(json, "name");
    if (tmp == NULL || tmp->type != JSON_STRING) {
        goto ssl_error;
    }
    char* name = str_trim(tmp->data.string);
    size_t sl = strlen(name);
    if (sl < 2 || sl > 16) {
        goto ssl_error;
    }
    pthread_rwlock_rdlock(&conn->server->players_by_entity_id->rwlock);
    ITER_MAP(conn->server->players_by_entity_id) {
        struct player* player = (struct player*) value;
        if (str_eq(name, player->name)) {
            player_kick(player, "You have logged in from another location!");
            goto post_map_iteration;
        }
        ITER_MAP_END();
    }
    post_map_iteration:;
    pthread_rwlock_unlock(&conn->server->players_by_entity_id->rwlock);
    conn->aes_ctx_enc = EVP_CIPHER_CTX_new();
    if (conn->aes_ctx_enc == NULL) goto rete;
    if (EVP_EncryptInit_ex(conn->aes_ctx_enc, EVP_aes_128_cfb8(), NULL, conn->shared_secret, conn->shared_secret) != 1) {
        goto ssl_error;
    }
    if (conn->aes_ctx_enc != NULL) {
        phook(conn->pool, (void (*)(void*)) encrypt_free, conn->aes_ctx_enc);
    }

    conn->aes_ctx_dec = EVP_CIPHER_CTX_new();
    if (conn->aes_ctx_dec == NULL) goto rete;
    if (EVP_DecryptInit_ex(conn->aes_ctx_dec, EVP_aes_128_cfb8(), NULL, conn->shared_secret, conn->shared_secret) != 1) {
        goto ssl_error;
    }
    if (conn->aes_ctx_dec != NULL) {
        phook(conn->pool, (void (*)(void*)) decrypt_free, conn->aes_ctx_dec);
    }

    if (work_joinServer(conn, packet->pool, name, id)) {
        pfree(ssl_pool);
        return 1;
    }
    pfree(ssl_pool);
    return 0;
    ssl_error: ;
    pfree(ssl_pool);
    struct packet* resp = packet_new(packet->pool, PKT_LOGIN_CLIENT_DISCONNECT);
    resp->data.login_client.disconnect.reason = "{\"text\": \"There was an unresolvable issue with the Mojang sessionserver! Please try again.\"}";
    conn->disconnect = 1;
    packet_write(conn, resp); // failure is ignored, we disconnect regardless
    return 1;
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
            BEGIN_HASHMAP_ITERATION (players);
            struct player* player = (struct player*) value;
            if (streq_nocase(name_trimmed, player->name)) {
                bad_name = 1;
                goto pbn;
            }
            END_HASHMAP_ITERATION (players);
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
