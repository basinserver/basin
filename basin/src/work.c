/*
 * work.c
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */

#include "work.h"
#include "accept.h"
#include "xstring.h"
#include <errno.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "collection.h"
#include "util.h"
#include "streams.h"
#include <sys/ioctl.h>
#include "network.h"
#include "packet.h"
#include "globals.h"
#include <openssl/md5.h>
#include "entity.h"
#include "server.h"
#include "worldmanager.h"
#include "queue.h"
#include "game.h"
#include "block.h"
#include <math.h>
#include "item.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include "version.h"
#include "json.h"

void closeConn(struct work_param* param, struct conn* conn) {
	close(conn->fd);
	if (rem_collection(param->conns, conn)) {
		errlog(param->logsess, "Failed to delete connection properly! This is bad!");
	}
	if (conn->player != NULL) {
		//broadcastf("yellow", "%s has left the server!", conn->player->name);
		conn->player->defunct = 1;
		conn->player->conn = NULL;
	}
	if (conn->aes_ctx_enc != NULL) {
		char final[256];
		int csl2 = 0;
		EVP_EncryptFinal_ex(conn->aes_ctx_enc, final, &csl2);
		EVP_CIPHER_CTX_free(conn->aes_ctx_enc);
	}
	if (conn->aes_ctx_dec != NULL) {
		char final[256];
		int csl2 = 0;
		EVP_DecryptFinal_ex(conn->aes_ctx_dec, final, &csl2);
		EVP_CIPHER_CTX_free(conn->aes_ctx_dec);
	}
	if (conn->host_ip != NULL) xfree(conn->host_ip);
	if (conn->readBuffer != NULL) xfree(conn->readBuffer);
	if (conn->readDecBuffer != NULL) xfree(conn->readDecBuffer);
	if (conn->writeBuffer != NULL) xfree(conn->writeBuffer);
	if (conn->onll_username != NULL) xfree(conn->onll_username);
	xfree(conn);
}

int work_joinServer(struct conn* conn, char* username, char* uuids) {
	struct packet rep;
	rep.id = PKT_LOGIN_CLIENT_LOGINSUCCESS;
	rep.data.login_client.loginsuccess.username = username;
	struct uuid uuid;
	unsigned char* uuidx = (unsigned char*) &uuid;
	if (uuids == NULL) {
		if (online_mode) return 1;
		MD5_CTX context;
		MD5_Init(&context);
		MD5_Update(&context, username, strlen(username));
		MD5_Final(uuidx, &context);
	} else {
		if (strlen(uuids) != 32) return 1;
		char ups[9];
		memcpy(ups, uuids, 8);
		ups[8] = 0;
		uuid.uuid1 = ((uint64_t) strtoll(ups, NULL, 16)) << 32;
		memcpy(ups, uuids + 8, 8);
		uuid.uuid1 |= ((uint64_t) strtoll(ups, NULL, 16)) & 0xFFFFFFFF;
		memcpy(ups, uuids + 16, 8);
		uuid.uuid2 = ((uint64_t) strtoll(ups, NULL, 16)) << 32;
		memcpy(ups, uuids + 24, 8);
		uuid.uuid2 |= ((uint64_t) strtoll(ups, NULL, 16)) & 0xFFFFFFFF;
	}
	rep.data.login_client.loginsuccess.uuid = xmalloc(38);
	snprintf(rep.data.login_client.loginsuccess.uuid, 10, "%08X-", ((uint32_t*) uuidx)[0]);
	snprintf(rep.data.login_client.loginsuccess.uuid + 9, 6, "%04X-", ((uint16_t*) uuidx)[2]);
	snprintf(rep.data.login_client.loginsuccess.uuid + 14, 6, "%04X-", ((uint16_t*) uuidx)[3]);
	snprintf(rep.data.login_client.loginsuccess.uuid + 19, 6, "%04X-", ((uint16_t*) uuidx)[4]);
	snprintf(rep.data.login_client.loginsuccess.uuid + 24, 9, "%08X", ((uint32_t*) (uuidx + 4))[2]);
	snprintf(rep.data.login_client.loginsuccess.uuid + 32, 5, "%04X", ((uint16_t*) uuidx)[7]);
	if (writePacket(conn, &rep) < 0) return 1;
	xfree(rep.data.login_client.loginsuccess.uuid);
	conn->state = STATE_PLAY;
	struct entity* ep = newEntity(nextEntityID++, (double) overworld->spawnpos.x + .5, (double) overworld->spawnpos.y, (double) overworld->spawnpos.z + .5, ENT_PLAYER, 0., 0.);
	struct player* player = newPlayer(ep, xstrdup(rep.data.login_client.loginsuccess.username, 1), uuid, conn, 0); // TODO default gamemode
	player->protocolVersion = conn->protocolVersion;
	conn->player = player;
	put_hashmap(players, player->entity->id, player);
	rep.id = PKT_PLAY_CLIENT_JOINGAME;
	rep.data.play_client.joingame.entity_id = ep->id;
	rep.data.play_client.joingame.gamemode = player->gamemode;
	rep.data.play_client.joingame.dimension = overworld->dimension;
	rep.data.play_client.joingame.difficulty = difficulty;
	rep.data.play_client.joingame.max_players = max_players;
	rep.data.play_client.joingame.level_type = overworld->levelType;
	rep.data.play_client.joingame.reduced_debug_info = 0; // TODO
	if (writePacket(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_PLUGINMESSAGE;
	rep.data.play_client.pluginmessage.channel = "MC|Brand";
	rep.data.play_client.pluginmessage.data = xmalloc(16);
	int rx2 = writeVarInt(5, rep.data.play_client.pluginmessage.data);
	memcpy(rep.data.play_client.pluginmessage.data + rx2, "Basin", 5);
	rep.data.play_client.pluginmessage.data_size = rx2 + 5;
	if (writePacket(conn, &rep) < 0) return 1;
	xfree(rep.data.play_client.pluginmessage.data);
	rep.id = PKT_PLAY_CLIENT_SERVERDIFFICULTY;
	rep.data.play_client.serverdifficulty.difficulty = difficulty;
	if (writePacket(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_SPAWNPOSITION;
	memcpy(&rep.data.play_client.spawnposition.location, &overworld->spawnpos, sizeof(struct encpos));
	if (writePacket(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_PLAYERABILITIES;
	rep.data.play_client.playerabilities.flags = 0; // TODO: allows flying, remove
	rep.data.play_client.playerabilities.flying_speed = 0.05;
	rep.data.play_client.playerabilities.field_of_view_modifier = .1;
	if (writePacket(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK;
	rep.data.play_client.playerpositionandlook.x = ep->x;
	rep.data.play_client.playerpositionandlook.y = ep->y;
	rep.data.play_client.playerpositionandlook.z = ep->z;
	rep.data.play_client.playerpositionandlook.yaw = ep->yaw;
	rep.data.play_client.playerpositionandlook.pitch = ep->pitch;
	rep.data.play_client.playerpositionandlook.flags = 0x0;
	rep.data.play_client.playerpositionandlook.teleport_id = 0;
	if (writePacket(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_PLAYERLISTITEM;
	pthread_rwlock_rdlock(&players->data_mutex);
	rep.data.play_client.playerlistitem.action_id = 0;
	rep.data.play_client.playerlistitem.number_of_players = players->entry_count + 1;
	rep.data.play_client.playerlistitem.players = xmalloc(rep.data.play_client.playerlistitem.number_of_players * sizeof(struct listitem_player));
	size_t px = 0;
	BEGIN_HASHMAP_ITERATION (players)
	struct player* plx = (struct player*) value;
	if (px < rep.data.play_client.playerlistitem.number_of_players) {
		memcpy(&rep.data.play_client.playerlistitem.players[px].uuid, &plx->uuid, sizeof(struct uuid));
		rep.data.play_client.playerlistitem.players[px].action.addplayer.name = xstrdup(plx->name, 0);
		rep.data.play_client.playerlistitem.players[px].action.addplayer.number_of_properties = 0;
		rep.data.play_client.playerlistitem.players[px].action.addplayer.properties = NULL;
		rep.data.play_client.playerlistitem.players[px].action.addplayer.gamemode = plx->gamemode;
		rep.data.play_client.playerlistitem.players[px].action.addplayer.ping = 0; // TODO
		rep.data.play_client.playerlistitem.players[px].action.addplayer.has_display_name = 0;
		rep.data.play_client.playerlistitem.players[px].action.addplayer.display_name = NULL;
		px++;
	}
	if (player == plx) continue;
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_PLAYERLISTITEM;
	pkt->data.play_client.playerlistitem.action_id = 0;
	pkt->data.play_client.playerlistitem.number_of_players = 1;
	pkt->data.play_client.playerlistitem.players = xmalloc(sizeof(struct listitem_player));
	memcpy(&pkt->data.play_client.playerlistitem.players->uuid, &player->uuid, sizeof(struct uuid));
	pkt->data.play_client.playerlistitem.players->action.addplayer.name = xstrdup(player->name, 0);
	pkt->data.play_client.playerlistitem.players->action.addplayer.number_of_properties = 0;
	pkt->data.play_client.playerlistitem.players->action.addplayer.properties = NULL;
	pkt->data.play_client.playerlistitem.players->action.addplayer.gamemode = player->gamemode;
	pkt->data.play_client.playerlistitem.players->action.addplayer.ping = 0; // TODO
	pkt->data.play_client.playerlistitem.players->action.addplayer.has_display_name = 0;
	pkt->data.play_client.playerlistitem.players->action.addplayer.display_name = NULL;
	add_queue(plx->outgoingPacket, pkt);
	flush_outgoing(plx);
	END_HASHMAP_ITERATION (players)
	pthread_rwlock_unlock(&players->data_mutex);
	memcpy(&rep.data.play_client.playerlistitem.players[px].uuid, &player->uuid, sizeof(struct uuid));
	rep.data.play_client.playerlistitem.players[px].action.addplayer.name = xstrdup(player->name, 0);
	rep.data.play_client.playerlistitem.players[px].action.addplayer.number_of_properties = 0;
	rep.data.play_client.playerlistitem.players[px].action.addplayer.properties = NULL;
	rep.data.play_client.playerlistitem.players[px].action.addplayer.gamemode = player->gamemode;
	rep.data.play_client.playerlistitem.players[px].action.addplayer.ping = 0; // TODO
	rep.data.play_client.playerlistitem.players[px].action.addplayer.has_display_name = 0;
	rep.data.play_client.playerlistitem.players[px].action.addplayer.display_name = NULL;
	px++;
	if (writePacket(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_TIMEUPDATE;
	rep.data.play_client.timeupdate.time_of_day = overworld->time;
	rep.data.play_client.timeupdate.world_age = overworld->age;
	if (writePacket(conn, &rep) < 0) return 1;
	add_collection(playersToLoad, player);
	//broadcastf("yellow", "%s has joined the server!", player->name);
	const char* mip = NULL;
	char tip[48];
	if (conn->addr.sin6_family == AF_INET) {
		struct sockaddr_in *sip4 = (struct sockaddr_in*) &conn->addr;
		mip = inet_ntop(AF_INET, &sip4->sin_addr, tip, 48);
	} else if (conn->addr.sin6_family == AF_INET6) {
		struct sockaddr_in6 *sip6 = (struct sockaddr_in6*) &conn->addr;
		if (memseq((unsigned char*) &sip6->sin6_addr, 10, 0) && memseq((unsigned char*) &sip6->sin6_addr + 10, 2, 0xff)) {
			mip = inet_ntop(AF_INET, ((unsigned char*) &sip6->sin6_addr) + 12, tip, 48);
		} else mip = inet_ntop(AF_INET6, &sip6->sin6_addr, tip, 48);
	} else if (conn->addr.sin6_family == AF_LOCAL) {
		mip = "UNIX";
	} else {
		mip = "UNKNOWN";
	}
	printf("Player '%s' has joined with IP '%s'\n", player->name, mip);
	return 0;
}

int handleRead(struct conn* conn, struct work_param* param, int fd) {
	if (conn->disconnect) return 0;
	void* abuf;
	size_t asze;
	if (conn->aes_ctx_dec != NULL) {
		int csl = conn->readBuffer_size + 32; // 16 extra just in case
		void* edata = xmalloc(csl);
		if (EVP_DecryptUpdate(conn->aes_ctx_dec, edata, &csl, conn->readBuffer, conn->readBuffer_size) != 1) {
			xfree(edata);
			return -1;
		}
		if (csl == 0) return 0;
		if (conn->readDecBuffer == NULL) {
			conn->readDecBuffer = xmalloc(csl);
			conn->readDecBuffer_size = 0;
		} else {
			conn->readDecBuffer = xrealloc(conn->readDecBuffer, csl + conn->readDecBuffer_size);
		}
		memcpy(conn->readDecBuffer + conn->readDecBuffer_size, edata, csl);
		conn->readDecBuffer_size += csl;
		abuf = conn->readDecBuffer;
		asze = conn->readDecBuffer_size;
		xfree(conn->readBuffer);
		conn->readBuffer = NULL;
		conn->readBuffer_size = 0;
	} else {
		abuf = conn->readBuffer;
		asze = conn->readBuffer_size;
	}
	while (abuf != NULL && asze > 0) {
		int32_t length = 0;
		if (!readVarInt(&length, abuf, asze)) {
			return 0;
		}
		int ls = getVarIntSize(length);
		if (asze - ls < length) {
			return 0;
		}
		struct packet* inp = xmalloc(sizeof(struct packet));
		ssize_t rx = readPacket(conn, abuf + ls, length, inp);
		if (rx == -1) goto rete;
		//printf("State = %i, ID = %i, Data = ", conn->state, inp->id);
		//for (size_t i = 0; i < length + ls; i++) {
		//	uint8_t ch = ((uint8_t*) conn->readBuffer)[i];
		//	printf("%02X", ch);
		//}
		//printf("\n");
		int os = conn->state;
		struct packet rep;
		int df = 0;
		if (conn->state == STATE_HANDSHAKE && inp->id == PKT_HANDSHAKE_SERVER_HANDSHAKE) {
			conn->host_ip = xstrdup(inp->data.handshake_server.handshake.server_address, 0);
			conn->host_port = inp->data.handshake_server.handshake.server_port;
			conn->protocolVersion = inp->data.handshake_server.handshake.protocol_version;
			if ((inp->data.handshake_server.handshake.protocol_version < MC_PROTOCOL_VERSION_MIN || inp->data.handshake_server.handshake.protocol_version > MC_PROTOCOL_VERSION_MAX) && inp->data.handshake_server.handshake.next_state != STATE_STATUS) return -2;
			if (inp->data.handshake_server.handshake.next_state == STATE_STATUS) {
				conn->state = STATE_STATUS;
			} else if (inp->data.handshake_server.handshake.next_state == STATE_LOGIN) {
				conn->state = STATE_LOGIN;
			} else goto rete;
		} else if (conn->state == STATE_STATUS) {
			if (inp->id == PKT_STATUS_SERVER_REQUEST) {
				rep.id = PKT_STATUS_CLIENT_RESPONSE;
				rep.data.status_client.response.json_response = xmalloc(1000);
				rep.data.status_client.response.json_response[999] = 0;
				snprintf(rep.data.status_client.response.json_response, 999, "{\"version\":{\"name\":\"1.11.2\",\"protocol\":%i},\"players\":{\"max\":%i,\"online\":%i},\"description\":{\"text\":\"%s\"}}", MC_PROTOCOL_VERSION_MIN, max_players, players->entry_count, motd);
				if (writePacket(conn, &rep) < 0) goto rete;
				xfree(rep.data.status_client.response.json_response);
			} else if (inp->id == PKT_STATUS_SERVER_PING) {
				rep.id = PKT_STATUS_CLIENT_PONG;
				if (writePacket(conn, &rep) < 0) goto rete;
				conn->state = -1;
			} else goto rete;
		} else if (conn->state == STATE_LOGIN) {
			if (inp->id == PKT_LOGIN_SERVER_ENCRYPTIONRESPONSE) {
				if (conn->verifyToken == 0 || inp->data.login_server.encryptionresponse.shared_secret_length > 162 || inp->data.login_server.encryptionresponse.verify_token_length > 162) goto rete;
				unsigned char decSecret[162];
				int secLen = RSA_private_decrypt(inp->data.login_server.encryptionresponse.shared_secret_length, inp->data.login_server.encryptionresponse.shared_secret, decSecret, public_rsa, RSA_PKCS1_PADDING);
				if (secLen != 16) goto rete;
				unsigned char decVerifyToken[162];
				int vtLen = RSA_private_decrypt(inp->data.login_server.encryptionresponse.verify_token_length, inp->data.login_server.encryptionresponse.verify_token, decVerifyToken, public_rsa, RSA_PKCS1_PADDING);
				if (vtLen != 4) goto rete;
				uint32_t vt = *((uint32_t*) decVerifyToken);
				if (vt != conn->verifyToken) goto rete;
				memcpy(conn->sharedSecret, decSecret, 16);
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
				int tl = snprintf(cbuf, 1024, "GET /session/minecraft/hasJoined?username=%s&serverId=%s HTTP/1.1\r\nHost: sessionserver.mojang.com\r\nUser-Agent: Basin " VERSION "\r\nConnection: close\r\n\r\n", conn->onll_username, fhash2);
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
				parseJSON(&json, data);
				struct json_object* tmp = getJSONValue(&json, "id");
				if (tmp == NULL || tmp->type != JSON_STRING) {
					freeJSON(&json);
					goto merr;
				}
				char* id = trim(tmp->data.string);
				tmp = getJSONValue(&json, "name");
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
				conn->aes_ctx_dec = EVP_CIPHER_CTX_new();
				if (conn->aes_ctx_dec == NULL) goto rete;
				//EVP_CIPHER_CTX_set_padding(conn->aes_ctx_dec, 0);
				if (EVP_DecryptInit_ex(conn->aes_ctx_dec, EVP_aes_128_cfb8(), NULL, conn->sharedSecret, conn->sharedSecret) != 1) goto rete;
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
					if (writePacket(conn, &rep) < 0) goto rete;
					conn->disconnect = 1;
					goto ret;
				}
			} else if (inp->id == PKT_LOGIN_SERVER_LOGINSTART) {
				if (online_mode) {
					if (conn->verifyToken) goto rete;
					conn->onll_username = xstrdup(inp->data.login_server.loginstart.name, 0);
					rep.id = PKT_LOGIN_CLIENT_ENCRYPTIONREQUEST;
					rep.data.login_client.encryptionrequest.server_id = "";
					rep.data.login_client.encryptionrequest.public_key = public_rsa_publickey;
					rep.data.login_client.encryptionrequest.public_key_length = 162;
					conn->verifyToken = rand();
					if (conn->verifyToken == 0) conn->verifyToken = 1;
					rep.data.login_client.encryptionrequest.verify_token = (uint8_t*) &conn->verifyToken;
					rep.data.login_client.encryptionrequest.verify_token_length = 4;
					if (writePacket(conn, &rep) < 0) goto rete;
				} else {
					int bn = 0;
					char* rna = trim(inp->data.login_server.loginstart.name);
					size_t rnal = strlen(rna);
					if (rnal > 16 || rnal < 2) bn = 2;
					if (!bn) {
						BEGIN_HASHMAP_ITERATION (players)
						struct player* player = (struct player*) value;
						if (streq_nocase(rna, player->name)) {
							bn = 1;
							goto pbn;
						}
						END_HASHMAP_ITERATION (players)
						pbn: ;
					}
					if (bn) {
						rep.id = PKT_LOGIN_CLIENT_DISCONNECT;
						rep.data.login_client.disconnect.reason = xstrdup(bn == 2 ? "{\"text\": \"Invalid name!\"}" : "{\"text\", \"You are already in the server!\"}", 0);
						if (writePacket(conn, &rep) < 0) goto rete;
						conn->disconnect = 1;
						goto ret;
					}
					if (work_joinServer(conn, rna, NULL)) goto rete;
				}
			}
		} else if (conn->state == STATE_PLAY) {
			add_queue(conn->player->incomingPacket, inp);
			df = 1;
		}
		beginProfilerSection("movebuf");
		memmove(abuf, abuf + length + ls, asze - length - ls);
		asze -= length + ls;
		if (abuf == conn->readBuffer) conn->readBuffer_size = asze;
		else if (abuf == conn->readDecBuffer) conn->readDecBuffer_size = asze;
		endProfilerSection("movebuf");
		goto ret;
		rete: ;
		if (!df) {
			freePacket(os, 0, inp);
			xfree(inp);
		}
		return -1;
		ret: ;
		if (!df) {
			freePacket(os, 0, inp);
			xfree(inp);
		}
	}

	return 0;
}

void run_work(struct work_param* param) {
	if (pipe(param->pipes) != 0) {
		printf("Failed to create pipe! %s\n", strerror(errno));
		return;
	}
	unsigned char wb;
	unsigned char* mbuf = xmalloc(1024);
	while (1) {
		pthread_rwlock_rdlock(&param->conns->data_mutex);
		size_t cc = param->conns->count;
		struct pollfd fds[cc + 1];
		struct conn* conns[cc];
		int fdi = 0;
		for (int i = 0; i < param->conns->size; i++) {
			if (param->conns->data[i] != NULL) {
				conns[fdi] = (param->conns->data[i]);
				struct conn* conn = conns[fdi];
				fds[fdi].fd = conns[fdi]->fd;
				fds[fdi].events = POLLIN | ((conn->writeBuffer_size > 0 || (conn->player != NULL && conn->player->outgoingPacket->size > 0)) ? POLLOUT : 0);
				fds[fdi++].revents = 0;
				if (fdi == cc) break;
			}
		}
		pthread_rwlock_unlock(&param->conns->data_mutex);
		fds[cc].fd = param->pipes[0];
		fds[cc].events = POLLIN;
		fds[cc].revents = 0;
		int cp = poll(fds, cc + 1, -1);
		if (cp < 0) {
			printf("Poll error in worker thread! %s\n", strerror(errno));
		} else if (cp == 0) continue;
		else if ((fds[cc].revents & POLLIN) == POLLIN) {
			if (read(param->pipes[0], &wb, 1) < 1) printf("Error reading from pipe, infinite loop COULD happen here.\n");
			if (cp-- == 1) continue;
		}
		for (int i = 0; i < cc; i++) {
			int re = fds[i].revents;
			struct conn* conn = conns[i];
			if ((re & POLLERR) == POLLERR) {
				//printf("POLLERR in worker poll! This is bad!\n");
				closeConn(param, conn);
				conn = NULL;
				goto cont;
			}
			if ((re & POLLHUP) == POLLHUP) {
				closeConn(param, conn);
				conn = NULL;
				goto cont;
			}
			if ((re & POLLNVAL) == POLLNVAL) {
				printf("Invalid FD in worker poll! This is bad!\n");
				closeConn(param, conn);
				conn = NULL;
				goto cont;
			}
			if ((re & POLLIN) == POLLIN) {
				size_t tr = 0;
				ioctl(fds[i].fd, FIONREAD, &tr);
				unsigned char* loc;
				if (conn->readBuffer == NULL) {
					conn->readBuffer = xmalloc(tr); // TODO: max upload?
					conn->readBuffer_size = tr;
					loc = conn->readBuffer;
				} else {
					conn->readBuffer_size += tr;
					conn->readBuffer = xrealloc(conn->readBuffer, conn->readBuffer_size);
					loc = conn->readBuffer + conn->readBuffer_size - tr;
				}
				ssize_t r = 0;
				if (r == 0 && tr == 0) { // nothing to read, but wont block.
					ssize_t x = read(fds[i].fd, loc + r, tr - r);
					if (x <= 0) {
						closeConn(param, conn);
						conn = NULL;
						goto cont;
					}
					r += x;
				}
				while (r < tr) {
					ssize_t x = read(fds[i].fd, loc + r, tr - r);
					if (x <= 0) {
						closeConn(param, conn);
						conn = NULL;
						goto cont;
					}
					r += x;
				}
				int p = 0;
				p = handleRead(conn, param, fds[i].fd);
				if (p == 1) {
					goto cont;
				}
			}
			if ((re & POLLOUT) == POLLOUT && conn != NULL) {
				if (conn->player != NULL) {
					struct packet* wp = pop_nowait_queue(conn->player->outgoingPacket);
					while (wp != NULL) {
						int wr = writePacket(conn, wp);
						if (wr == -1) {
							closeConn(param, conn);
							conn = NULL;
							goto cont;
						}
						freePacket(conn->state, 1, wp);
						xfree(wp);
						wp = pop_nowait_queue(conn->player->outgoingPacket);
					}
					//printf("%s WBS: %lu\n", conn->player->name, conn->writeBuffer_size);
				}
				ssize_t mtr = write(fds[i].fd, conn->writeBuffer, conn->writeBuffer_size);
				if (mtr < 0 && errno != EAGAIN) {
					closeConn(param, conn);
					conn = NULL;
					goto cont;
				} else if (mtr < 0) {
					goto cont;
				} else if (mtr < conn->writeBuffer_size) {
					memmove(conn->writeBuffer, conn->writeBuffer + mtr, conn->writeBuffer_size - mtr);
					conn->writeBuffer_size -= mtr;
					if (conn->writeBuffer_capacity > conn->writeBuffer_size + 1024 * 1024) {
						conn->writeBuffer = xrealloc(conn->writeBuffer, conn->writeBuffer_size);
						conn->writeBuffer_capacity = conn->writeBuffer_size;
					}
				} else {
					conn->writeBuffer_size = 0;
					conn->writeBuffer_capacity = 0;
					xfree(conn->writeBuffer);
					conn->writeBuffer = NULL;
				}
			}
			if (conn->state == -1 && conn->writeBuffer_size == 0) {
				closeConn(param, conn);
				conn = NULL;
			}
			cont: ;
			if (conn != NULL && conn->disconnect) {
				closeConn(param, conn);
				conn = NULL;
			}
			if (--cp == 0) break;
		}
	}
	xfree(mbuf);
	pthread_cancel (pthread_self());}
