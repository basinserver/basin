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

void closeConn(struct work_param* param, struct conn* conn) {
	close(conn->fd);
	if (rem_collection(param->conns, conn)) {
		errlog(param->logsess, "Failed to delete connection properly! This is bad!");
	}
	if (conn->player != NULL) {
		rem_collection(players, conn->player);
		char bct[256];
		snprintf(bct, 256, "%s has left the server!", conn->player->name);
		broadcast(bct);
		for (size_t i = 0; i < players->size; i++) {
			struct player* plx = (struct player*) players->data[i];
			if (plx != NULL) {
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_PLAYERLISTITEM;
				pkt->data.play_client.playerlistitem.action_id = 4;
				pkt->data.play_client.playerlistitem.number_of_players = 1;
				pkt->data.play_client.playerlistitem.players = xmalloc(sizeof(struct listitem_player));
				memcpy(&pkt->data.play_client.playerlistitem.players->uuid, &conn->player->uuid, sizeof(struct uuid));
				add_queue(plx->outgoingPacket, pkt);
				flush_outgoing(plx);
			}
		}
		conn->player->defunct = 1;
		conn->player->conn = NULL;
	}
	if (conn->host_ip != NULL) xfree(conn->host_ip);
	if (conn->readBuffer != NULL) xfree(conn->readBuffer);
	if (conn->writeBuffer != NULL) xfree(conn->writeBuffer);
	xfree(conn);
}

int handleRead(struct conn* conn, struct work_param* param, int fd) {
	while (conn->readBuffer != NULL && conn->readBuffer_size > 0) {
		int32_t length = 0;
		if (!readVarInt(&length, conn->readBuffer, conn->readBuffer_size)) {
			return 0;
		}
		int ls = getVarIntSize(length);
		if (conn->readBuffer_size - ls < length) {
			return 0;
		}
		struct packet* inp = xmalloc(sizeof(struct packet));
		ssize_t rx = readPacket(conn, conn->readBuffer + ls, length, inp);
		if (rx == -1) goto rete;
		int os = conn->state;
		struct packet rep;
		int df = 0;
		if (conn->state == STATE_HANDSHAKE && inp->id == PKT_HANDSHAKE_SERVER_HANDSHAKE) {
			conn->host_ip = xstrdup(inp->data.handshake_server.handshake.server_address, 0);
			conn->host_port = inp->data.handshake_server.handshake.server_port;
			if (inp->data.handshake_server.handshake.protocol_version != MC_PROTOCOL_VERSION && inp->data.handshake_server.handshake.next_state != STATE_STATUS) return -2;
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
				snprintf(rep.data.status_client.response.json_response, 999, "{\"version\":{\"name\":\"1.11.2\",\"protocol\":%i},\"players\":{\"max\":%i,\"online\":%i},\"description\":{\"text\":\"%s\"}}", MC_PROTOCOL_VERSION, max_players, players->count, motd);
				if (writePacket(conn, &rep) < 0) goto rete;
				xfree(rep.data.status_client.response.json_response);
			} else if (inp->id == PKT_STATUS_SERVER_PING) {
				rep.id = PKT_STATUS_CLIENT_PONG;
				if (writePacket(conn, &rep) < 0) goto rete;
				conn->state = -1;
			} else goto rete;
		} else if (conn->state == STATE_LOGIN) {
			if (inp->id == PKT_LOGIN_SERVER_LOGINSTART) {
				if (online_mode) {
					//TODO
					return -1;
				} else {
					rep.id = PKT_LOGIN_CLIENT_LOGINSUCCESS;
					rep.data.login_client.loginsuccess.username = inp->data.login_server.loginstart.name;
					struct uuid uuid;
					unsigned char* uuidx = (unsigned char*) &uuid;
					MD5_CTX context;
					MD5_Init(&context);
					MD5_Update(&context, rep.data.login_client.loginsuccess.username, strlen(rep.data.login_client.loginsuccess.username));
					MD5_Final(uuidx, &context);
					rep.data.login_client.loginsuccess.uuid = xmalloc(38);
					snprintf(rep.data.login_client.loginsuccess.uuid, 10, "%08X-", ((uint32_t*) uuidx)[0]);
					snprintf(rep.data.login_client.loginsuccess.uuid + 9, 6, "%04X-", ((uint16_t*) uuidx)[2]);
					snprintf(rep.data.login_client.loginsuccess.uuid + 14, 6, "%04X-", ((uint16_t*) uuidx)[3]);
					snprintf(rep.data.login_client.loginsuccess.uuid + 19, 6, "%04X-", ((uint16_t*) uuidx)[4]);
					snprintf(rep.data.login_client.loginsuccess.uuid + 24, 9, "%08X", ((uint32_t*) (uuidx + 4))[2]);
					snprintf(rep.data.login_client.loginsuccess.uuid + 32, 5, "%04X", ((uint16_t*) uuidx)[7]);
					if (writePacket(conn, &rep) < 0) goto rete;
					xfree(rep.data.login_client.loginsuccess.uuid);
					conn->state = STATE_PLAY;
					struct entity* ep = newEntity(nextEntityID++, (double) overworld->spawnpos.x + .5, (double) overworld->spawnpos.y, (double) overworld->spawnpos.z + .5, ENT_PLAYER, 0., 0.);
					struct player* player = newPlayer(ep, xstrdup(rep.data.login_client.loginsuccess.username, 1), uuid, conn, 0); // TODO default gamemode
					conn->player = player;
					add_collection(players, player);
					rep.id = PKT_PLAY_CLIENT_JOINGAME;
					rep.data.play_client.joingame.entity_id = ep->id;
					rep.data.play_client.joingame.gamemode = player->gamemode;
					rep.data.play_client.joingame.dimension = overworld->dimension;
					rep.data.play_client.joingame.difficulty = difficulty;
					rep.data.play_client.joingame.max_players = max_players;
					rep.data.play_client.joingame.level_type = overworld->levelType;
					rep.data.play_client.joingame.reduced_debug_info = 0; // TODO
					if (writePacket(conn, &rep) < 0) goto rete;
					rep.id = PKT_PLAY_CLIENT_PLUGINMESSAGE;
					rep.data.play_client.pluginmessage.channel = "MC|Brand";
					rep.data.play_client.pluginmessage.data = xmalloc(16);
					int rx = writeVarInt(5, rep.data.play_client.pluginmessage.data);
					memcpy(rep.data.play_client.pluginmessage.data + rx, "Basin", 5);
					rep.data.play_client.pluginmessage.data_size = rx + 5;
					if (writePacket(conn, &rep) < 0) goto rete;
					xfree(rep.data.play_client.pluginmessage.data);
					rep.id = PKT_PLAY_CLIENT_SERVERDIFFICULTY;
					rep.data.play_client.serverdifficulty.difficulty = difficulty;
					if (writePacket(conn, &rep) < 0) goto rete;
					rep.id = PKT_PLAY_CLIENT_SPAWNPOSITION;
					memcpy(&rep.data.play_client.spawnposition.location, &overworld->spawnpos, sizeof(struct encpos));
					if (writePacket(conn, &rep) < 0) goto rete;
					rep.id = PKT_PLAY_CLIENT_PLAYERABILITIES;
					rep.data.play_client.playerabilities.flags = 0x4; // TODO: allows flying, remove
					rep.data.play_client.playerabilities.flying_speed = 0.05;
					rep.data.play_client.playerabilities.field_of_view_modifier = .1;
					if (writePacket(conn, &rep) < 0) goto rete;
					rep.id = PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK;
					rep.data.play_client.playerpositionandlook.x = ep->x;
					rep.data.play_client.playerpositionandlook.y = ep->y;
					rep.data.play_client.playerpositionandlook.z = ep->z;
					rep.data.play_client.playerpositionandlook.yaw = ep->yaw;
					rep.data.play_client.playerpositionandlook.pitch = ep->pitch;
					rep.data.play_client.playerpositionandlook.flags = 0x0;
					rep.data.play_client.playerpositionandlook.teleport_id = 0;
					if (writePacket(conn, &rep) < 0) goto rete;
					rep.id = PKT_PLAY_CLIENT_PLAYERLISTITEM;
					rep.data.play_client.playerlistitem.action_id = 0;
					rep.data.play_client.playerlistitem.number_of_players = players->count;
					rep.data.play_client.playerlistitem.players = xmalloc(rep.data.play_client.playerlistitem.number_of_players * sizeof(struct listitem_player));
					size_t px = 0;
					for (size_t i = 0; i < players->size; i++) {
						struct player* plx = (struct player*) players->data[i];
						if (plx != NULL) {
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
						}
					}
					if (writePacket(conn, &rep) < 0) goto rete;
					spawnPlayer(overworld, player);
					for (size_t i = 0; i < player->world->entities->size; i++) {
						struct entity* ent = (struct entity*) player->world->entities->data[i];
						if (ent == NULL || entity_distsq(ent, player->entity) > 16384.) continue;
						if (ent->type == ENT_PLAYER) {
							if (ent->data.player.player != player) {
								loadPlayer(player, ent->data.player.player);
								loadPlayer(ent->data.player.player, player);
							}
						}
					}
					char bct[256];
					snprintf(bct, 256, "%s has joined the server!", player->name);
					broadcast(bct);
					/*for (int x = -9; x < 10; x++) {
					 for (int z = -9; z < 10; z++) {
					 struct chunk* ch = getChunk(overworld, x + (int32_t) ep->x / 16, z + (int32_t) ep->z / 16);
					 if (ch != NULL) {
					 struct packet* pkt = xmalloc(sizeof(struct packet));
					 pkt->id = PKT_PLAY_CLIENT_CHUNKDATA;
					 pkt->data.play_client.chunkdata.data = ch;
					 pkt->data.play_client.chunkdata.ground_up_continuous = 1;
					 pkt->data.play_client.chunkdata.number_of_block_entities = 0;
					 pkt->data.play_client.chunkdata.block_entities = NULL;
					 add_queue(player->conn->outgoingPacket, pkt);
					 flush_outgoing(player);
					 }
					 }
					 }*/

				}
			}
		} else if (conn->state == STATE_PLAY) {
			add_queue(conn->player->incomingPacket, inp);
			df = 1;
		}
		memmove(conn->readBuffer, conn->readBuffer + length + ls, conn->readBuffer_size - length - ls);
		conn->readBuffer_size -= length + ls;
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
			if (conn->disconnect) {
				closeConn(param, conn);
				conn = NULL;
				goto cont;
			}
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
			if (--cp == 0) break;
		}
	}
	xfree(mbuf);
	pthread_cancel (pthread_self());}
