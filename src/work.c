/*
 * work.c
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */

#include "work.h"
#include "accept.h"
#include "basin/packet.h"
#include "login_stage_handler.h"
#include <basin/network.h>
#include <basin/globals.h>
#include <basin/connection.h>
#include <basin/entity.h>
#include <basin/server.h>
#include <basin/worldmanager.h>
#include <basin/game.h>
#include <basin/block.h>
#include <basin/item.h>
#include <basin/version.h>
#include <basin/profile.h>
#include <avuna/json.h>
#include <avuna/streams.h>
#include <avuna/queue.h>
#include <avuna/string.h>
#include <avuna/netmgr.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/md5.h>
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <avuna/util.h>


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
	if (packet_write(conn, &rep) < 0) return 1;
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
	if (packet_write(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_PLUGINMESSAGE;
	rep.data.play_client.pluginmessage.channel = "MC|Brand";
	rep.data.play_client.pluginmessage.data = xmalloc(16);
	int rx2 = writeVarInt(5, rep.data.play_client.pluginmessage.data);
	memcpy(rep.data.play_client.pluginmessage.data + rx2, "Basin", 5);
	rep.data.play_client.pluginmessage.data_size = rx2 + 5;
	if (packet_write(conn, &rep) < 0) return 1;
	xfree(rep.data.play_client.pluginmessage.data);
	rep.id = PKT_PLAY_CLIENT_SERVERDIFFICULTY;
	rep.data.play_client.serverdifficulty.difficulty = difficulty;
	if (packet_write(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_SPAWNPOSITION;
	memcpy(&rep.data.play_client.spawnposition.location, &overworld->spawnpos, sizeof(struct encpos));
	if (packet_write(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_PLAYERABILITIES;
	rep.data.play_client.playerabilities.flags = 0; // TODO: allows flying, remove
	rep.data.play_client.playerabilities.flying_speed = 0.05;
	rep.data.play_client.playerabilities.field_of_view_modifier = .1;
	if (packet_write(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK;
	rep.data.play_client.playerpositionandlook.x = ep->x;
	rep.data.play_client.playerpositionandlook.y = ep->y;
	rep.data.play_client.playerpositionandlook.z = ep->z;
	rep.data.play_client.playerpositionandlook.yaw = ep->yaw;
	rep.data.play_client.playerpositionandlook.pitch = ep->pitch;
	rep.data.play_client.playerpositionandlook.flags = 0x0;
	rep.data.play_client.playerpositionandlook.teleport_id = 0;
	if (packet_write(conn, &rep) < 0) return 1;
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
	if (packet_write(conn, &rep) < 0) return 1;
	rep.id = PKT_PLAY_CLIENT_TIMEUPDATE;
	rep.data.play_client.timeupdate.time_of_day = overworld->time;
	rep.data.play_client.timeupdate.world_age = overworld->age;
	if (packet_write(conn, &rep) < 0) return 1;
	add_collection(playersToLoad, player);
	broadcastf("yellow", "%s has joined the server!", player->name);
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


int connection_read(struct netmgr_connection* netmgr_conn, uint8_t* read_buf, size_t read_buf_len) {
	struct connection* conn = netmgr_conn->extra;
	if (conn->disconnect) {
		return 1;
	}
	if (conn->aes_ctx_dec != NULL) {
		int decrypted_length = (int) (read_buf_len + 32); // 16 extra just in case
		void* decrypted = pmalloc(netmgr_conn->read_buffer.pool, (size_t) decrypted_length);
		if (EVP_DecryptUpdate(conn->aes_ctx_dec, decrypted, &decrypted_length, read_buf, (int) read_buf_len) != 1) {
			pprefree(netmgr_conn->read_buffer.pool, decrypted);
			return 1;
		}
		if (decrypted_length == 0) return 0;
		buffer_push(&netmgr_conn->read_buffer, decrypted, (size_t) decrypted_length);
		pprefree(netmgr_conn->read_buffer.pool, read_buf);
	} else {
		buffer_push(&netmgr_conn->read_buffer, read_buf, read_buf_len);
	}
	while (netmgr_conn->read_buffer.size > 4) {
		uint8_t peek_buf[8];
		buffer_peek(&netmgr_conn->read_buffer, 5, peek_buf);
		int32_t length = 0;
		if (!readVarInt(&length, peek_buf, 5)) {
			return 0;
		}
		int ls = getVarIntSize(length);
		if (netmgr_conn->read_buffer.size - ls < length) {
			return 0;
		}
		buffer_skip(&netmgr_conn->read_buffer, (size_t) ls);
		struct mempool* packet_pool = mempool_new();
		pchild(conn->pool, packet_pool);
		uint8_t* packet_buf = pmalloc(packet_pool, (size_t) length);
		buffer_pop(&netmgr_conn->read_buffer, (size_t) length, packet_buf);

		struct packet* packet = pmalloc(packet_pool, sizeof(struct packet));
		packet->pool = packet_pool;
		ssize_t read_packet_length = packet_read(conn, packet_buf, (size_t) length, packet);

		if (read_packet_length == -1) goto ret_error;
		if (conn->protocol_state == STATE_HANDSHAKE && packet->id == PKT_HANDSHAKE_SERVER_HANDSHAKE) {
			if (handle_packet_handshake(conn, packet)) {
				goto ret_error;
			}
		} else if (conn->protocol_state == STATE_STATUS) {
			if (handle_packet_status(conn, packet)) {
				goto ret_error;
			}
		} else if (conn->protocol_state == STATE_LOGIN) {
			if (handle_packet_login(conn, packet)) {
				goto ret_error;
			}
		} else if (conn->protocol_state == STATE_PLAY) {
			queue_push(conn->player->incomingPacket, packet);
		} else {
			goto ret_error;
		}
		goto ret;
		ret_error: ;
		pfree(packet->pool);
		return 1;
		ret: ;
		pfree(packet->pool);
	}

	return 0;
}

void connection_on_closed(struct netmgr_connection* netmgr_conn) {
	struct connection* conn = netmgr_conn->extra;
	if (conn->player != NULL) {
		broadcastf("yellow", "%s has left the server!", conn->player->name);
		conn->player->defunct = 1;
		conn->player->conn = NULL;
	}
	pfree(conn->pool);

}