/*
 * packet1.9.c
 *
 *  Created on: Jun 23, 2016
 *      Author: root
 */

#include "globals.h"

#ifdef MC_VERSION_1_9_4

#include "network.h"
#include "packet1.9.h"
#include <zlib.h>
#include <errno.h>
#include "xstring.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nbt.h"
#include <zlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "accept.h"

int readPacket(struct conn* conn, struct packet* packet) {
	void* pktbuf = NULL;
	int32_t pktlen = 0;
	if (conn->comp >= 0) {
		int32_t pl = 0;
		int32_t dl = 0;
		//printf("reading pl\n");
		readVarInt_stream(&pl, conn->fd);
		//printf("pl = %i\n", pl);
		pl -= readVarInt_stream(&dl, conn->fd);
		//printf("dl = %i\n", dl);
		if (dl == 0) {
			pktlen = pl;
			if (pktlen > 0) {
				pktbuf = malloc(pktlen);
				size_t r = 0;
				while (r < pktlen) {
#ifdef __MINGW32__
					ssize_t x = recv(conn->fd, pktbuf + r, pktlen - r, 0);
#else
					ssize_t x = read(conn->fd, pktbuf + r, pktlen - r);
#endif
					if (x <= 0) {
						printf("read error: %s\n", strerror(errno));
						free(pktbuf);
						return -1;
					}
					r += x;
				}
			}
		} else {
			if (pl > 0) {
				void* cmppkt = malloc(pl);
				size_t r = 0;
				while (r < pl) {
#ifdef __MINGW32__
					ssize_t x = recv(conn->fd, cmppkt + r, pl - r, 0);
#else
					ssize_t x = read(conn->fd, cmppkt + r, pl - r);
#endif
					if (x <= 0) {
						printf("read error: %s\n", strerror(errno));
						free(cmppkt);
						return -1;
					}
					r += x;
				}
				pktlen = dl;
				pktbuf = malloc(dl);
				//
				z_stream strm;
				strm.zalloc = Z_NULL;
				strm.zfree = Z_NULL;
				strm.opaque = Z_NULL;
				int dr = 0;
				if ((dr = inflateInit(&strm)) != Z_OK) {
					free(pktbuf);
					free(cmppkt);
					printf("Compression initialization error!\n");
					return -1;
				}
				strm.avail_in = pl;
				strm.next_in = cmppkt;
				strm.avail_out = pktlen;
				strm.next_out = pktbuf;
				do {
					dr = inflate(&strm, Z_FINISH);
					if (dr == Z_STREAM_ERROR) {
						free(pktbuf);
						free(cmppkt);
						printf("Compression Read Error\n");
						return -1;
					}
					strm.avail_out = pktlen - strm.total_out;
					strm.next_out = pktbuf + strm.total_out;
				}while (strm.avail_in > 0 || strm.total_out < pktlen);
				inflateEnd(&strm);
				free(cmppkt);
			} else {
				printf("Incoherent Compression Data!\n");
				return -1;
			}
		}
	} else {
		//printf("reading pktlen\n");
		readVarInt_stream(&pktlen, conn->fd);
		//printf("pktlen = %i\n", pktlen);
		if (pktlen > 0) {
			pktbuf = malloc(pktlen);
			size_t r = 0;
			while (r < pktlen) {
#ifdef __MINGW32__
				ssize_t x = recv(conn->fd, pktbuf + r, pktlen - r, 0);
#else
				ssize_t x = read(conn->fd, pktbuf + r, pktlen - r);
#endif
				//printf("read = %i\n", x);
				if (x <= 0) {
					printf("read error: %s\n", strerror(errno));
					free(pktbuf);
					return -1;
				}
				r += x;
			}
		}
	}
	if (pktbuf == NULL) return 0;
	unsigned char* pbuf = (unsigned char*) pktbuf;
	size_t ps = pktlen;
	int32_t id = 0;
	size_t t = readVarInt(&id, pbuf, ps);
	//printf("pktid = %02X\n", id);
	pbuf += t;
	ps -= t;
	packet->id = id;
	if (conn->state == STATE_PLAY) {
		if (id == PKT_PLAY_SERVER_SPAWNOBJECT) {
			int rx = readVarInt(&packet->data.play_server.spawnobject.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 53) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.spawnobject.uuid, pbuf, sizeof(struct uuid));
			ps -= sizeof(struct uuid);
			pbuf += sizeof(struct uuid);
			packet->data.play_server.spawnobject.type = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.spawnobject.x, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnobject.x, 8);
			memcpy(&packet->data.play_server.spawnobject.y, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnobject.y, 8);
			memcpy(&packet->data.play_server.spawnobject.z, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnobject.z, 8);
			char ang;
			memcpy(&ang, pbuf, 1);
			pbuf++;
			ps--;
			packet->data.play_server.spawnobject.pitch = ((float) ang / 256. * 360.);
			memcpy(&ang, pbuf, 1);
			pbuf++;
			ps--;
			packet->data.play_server.spawnobject.yaw = ((float) ang / 256. * 360.);
			memcpy(&packet->data.play_server.spawnobject.data, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			int16_t vv;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.spawnobject.velX = (float) vv / 8000.;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.spawnobject.velY = (float) vv / 8000.;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.spawnobject.velZ = (float) vv / 8000.;
		} else if (id == PKT_PLAY_SERVER_SPAWNEXPERIENCEORB) {
			int rx = readVarInt(&packet->data.play_server.spawnexperienceorb.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 26) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.spawnexperienceorb.x, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnexperienceorb.x, 8);
			memcpy(&packet->data.play_server.spawnexperienceorb.y, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnexperienceorb.y, 8);
			memcpy(&packet->data.play_server.spawnexperienceorb.z, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnexperienceorb.z, 8);
			memcpy(&packet->data.play_server.spawnexperienceorb.count, pbuf, 2);
			swapEndian(&packet->data.play_server.spawnexperienceorb.count, 2);
			pbuf += 2;
			ps -= 2;
		} else if (id == PKT_PLAY_SERVER_SPAWNGLOBALENTITY) {
			int rx = readVarInt(&packet->data.play_server.spawnglobalentity.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 25) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.spawnglobalentity.type, pbuf, 1);
			pbuf += 1;
			ps -= 1;
			memcpy(&packet->data.play_server.spawnglobalentity.x, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnglobalentity.x, 8);
			memcpy(&packet->data.play_server.spawnglobalentity.y, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnglobalentity.y, 8);
			memcpy(&packet->data.play_server.spawnglobalentity.z, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnglobalentity.z, 8);
		} else if (id == PKT_PLAY_SERVER_SPAWNMOB) {
			int rx = readVarInt(&packet->data.play_server.spawnmob.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 50) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.spawnmob.uuid, pbuf, sizeof(struct uuid));
			ps -= sizeof(struct uuid);
			pbuf += sizeof(struct uuid);
			packet->data.play_server.spawnmob.type = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.spawnmob.x, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnmob.x, 8);
			memcpy(&packet->data.play_server.spawnmob.y, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnmob.y, 8);
			memcpy(&packet->data.play_server.spawnmob.z, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnmob.z, 8);
			char ang;
			memcpy(&ang, pbuf, 1);
			pbuf++;
			ps--;
			packet->data.play_server.spawnmob.yaw = ((float) ang / 256. * 360.);
			memcpy(&ang, pbuf, 1);
			pbuf++;
			ps--;
			packet->data.play_server.spawnmob.pitch = ((float) ang / 256. * 360.);
			memcpy(&ang, pbuf, 1);
			pbuf++;
			ps--;
			packet->data.play_server.spawnmob.headPitch = ((float) ang / 256. * 360.);
			int16_t vv;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.spawnmob.velX = (float) vv / 8000.;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.spawnmob.velY = (float) vv / 8000.;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.spawnmob.velZ = (float) vv / 8000.;
			packet->data.play_server.spawnmob.metadata_size = ps;
			packet->data.play_server.spawnmob.metadata = malloc(ps);
			memcpy(packet->data.play_server.spawnmob.metadata, pbuf, ps);
			pbuf += ps;
			ps = 0;
		} else if (id == PKT_PLAY_SERVER_SPAWNPAINTING) {
			int rx = readVarInt(&packet->data.play_server.spawnpainting.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 2 + sizeof(struct encpos) + 1) {
				free(pktbuf);
				return -1;
			}
			int x = readString(&packet->data.play_server.spawnpainting.title, pbuf, ps);
			ps -= x;
			pbuf += x;
			if (ps < sizeof(struct encpos) + 1) {
				free(pktbuf);
				free(packet->data.play_server.spawnpainting.title);
				return -1;
			}
			memcpy(&packet->data.play_server.spawnpainting.loc, pbuf, sizeof(struct encpos));
			pbuf += sizeof(struct encpos);
			ps -= sizeof(struct encpos);
			swapEndian(&packet->data.play_server.spawnpainting.loc, sizeof(struct encpos));
			packet->data.play_server.spawnpainting.direction = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_SPAWNPLAYER) {
			int rx = readVarInt(&packet->data.play_server.spawnplayer.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 42) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.spawnplayer.uuid, pbuf, sizeof(struct uuid));
			ps -= sizeof(struct uuid);
			pbuf += sizeof(struct uuid);
			memcpy(&packet->data.play_server.spawnplayer.x, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnplayer.x, 8);
			memcpy(&packet->data.play_server.spawnplayer.y, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnplayer.y, 8);
			memcpy(&packet->data.play_server.spawnplayer.z, pbuf, sizeof(double));
			ps -= sizeof(double);
			pbuf += sizeof(double);
			swapEndian(&packet->data.play_server.spawnplayer.z, 8);
			char ang;
			memcpy(&ang, pbuf, 1);
			pbuf++;
			ps--;
			packet->data.play_server.spawnplayer.yaw = ((float) ang / 256. * 360.);
			memcpy(&ang, pbuf, 1);
			pbuf++;
			ps--;
			packet->data.play_server.spawnplayer.pitch = ((float) ang / 256. * 360.);
			packet->data.play_server.spawnplayer.metadata_size = ps;
			packet->data.play_server.spawnplayer.metadata = malloc(ps);
			memcpy(packet->data.play_server.spawnplayer.metadata, pbuf, ps);
			pbuf += ps;
			ps = 0;
		} else if (id == PKT_PLAY_SERVER_ANIMATION) {
			int rx = readVarInt(&packet->data.play_server.animation.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.animation.anim = pbuf[0];
			ps--;
			pbuf++;
		} else if (id == PKT_PLAY_SERVER_STATISTICS) {
			int rx = readVarInt(&packet->data.play_server.statistics.stat_count, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			packet->data.play_server.statistics.stats = malloc(sizeof(struct statistic) * packet->data.play_server.statistics.stat_count);
			for (size_t i = 0; i < packet->data.play_server.statistics.stat_count; i++) {
				struct statistic* cs = &packet->data.play_server.statistics.stats[i];
				rx = readString(&cs->name, pbuf, ps);
				ps -= rx;
				pbuf += rx;
				rx = readVarInt(&cs->value, pbuf, ps);
				ps -= rx;
				pbuf += rx;
			}
		} else if (id == PKT_PLAY_SERVER_BLOCKBREAKANIMATION) {
			int rx = readVarInt(&packet->data.play_server.blockbreakanimation.entityID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 1 + sizeof(struct encpos)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.blockbreakanimation.pos, pbuf, sizeof(struct encpos));
			ps -= sizeof(struct encpos);
			pbuf += sizeof(struct encpos);
			swapEndian(&packet->data.play_server.blockbreakanimation.pos, sizeof(struct encpos));
			packet->data.play_server.blockbreakanimation.stage = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_UPDATEBLOCKENTITY) {
			if (ps < sizeof(struct encpos)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.updateblockentity.pos, pbuf, sizeof(struct encpos));
			ps -= sizeof(struct encpos);
			pbuf += sizeof(struct encpos);
			swapEndian(&packet->data.play_server.updateblockentity.pos, sizeof(struct encpos));
			if (ps < 2) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.updateblockentity.action = pbuf[0];
			pbuf++;
			ps--;
			readNBT(&packet->data.play_server.updateblockentity.nbt, pbuf, ps);
		} else if (id == PKT_PLAY_SERVER_BLOCKACTION) {
			if (ps < sizeof(struct encpos)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.blockaction.pos, pbuf, sizeof(struct encpos));
			ps -= sizeof(struct encpos);
			pbuf += sizeof(struct encpos);
			swapEndian(&packet->data.play_server.blockaction.pos, sizeof(struct encpos));
			if (ps < 2) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.blockaction.byte1 = pbuf[0];
			pbuf++;
			ps--;
			packet->data.play_server.blockaction.byte2 = pbuf[0];
			pbuf++;
			ps--;
			int rx = readVarInt(&packet->data.play_server.blockaction.type, pbuf, ps);
			ps -= rx;
			pbuf += rx;
		} else if (id == PKT_PLAY_SERVER_BLOCKCHANGE) {
			if (ps < sizeof(struct encpos)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.blockchange.pos, pbuf, sizeof(struct encpos));
			ps -= sizeof(struct encpos);
			pbuf += sizeof(struct encpos);
			swapEndian(&packet->data.play_server.blockchange.pos, sizeof(struct encpos));
			int rx = readVarInt(&packet->data.play_server.blockchange.blockID, pbuf, ps);
			ps -= rx;
			pbuf += rx;
		} else if (id == PKT_PLAY_SERVER_BOSSBAR) {
			if (ps < sizeof(struct uuid)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.bossbar.uuid, pbuf, sizeof(struct uuid));
			ps -= sizeof(struct uuid);
			pbuf += sizeof(struct uuid);
			int rx = readVarInt(&packet->data.play_server.bossbar.action, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (packet->data.play_server.bossbar.action == 0) {
				struct bossbar_add* ba = &packet->data.play_server.bossbar.data.add;
				rx = readString(&ba->title, pbuf, ps);
				ps -= rx;
				pbuf += rx;
				if (ps < 7) {
					free(ba->title);
					free(pktbuf);
					return -1;
				}
				memcpy(&ba->health, pbuf, 4);
				ps -= 4;
				pbuf += 4;
				swapEndian(&ba->health, 4);
				rx = readVarInt(&ba->color, pbuf, ps);
				ps -= rx;
				pbuf += rx;
				rx = readVarInt(&ba->division, pbuf, ps);
				ps -= rx;
				pbuf += rx;
				if (ps < 1) {
					free(ba->title);
					free(pktbuf);
					return -1;
				}
			} else if (packet->data.play_server.bossbar.action == 1) {

			} else if (packet->data.play_server.bossbar.action == 2) {
				if (ps < 4) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.bossbar.data.health, pbuf, 4);
				ps -= 4;
				pbuf += 4;
				swapEndian(&packet->data.play_server.bossbar.data.health, 4);
			} else if (packet->data.play_server.bossbar.action == 3) {
				rx = readString(&packet->data.play_server.bossbar.data.title, pbuf, ps);
				ps -= rx;
				pbuf += rx;
			} else if (packet->data.play_server.bossbar.action == 4) {
				struct bossbar_updatestyle* ba = &packet->data.play_server.bossbar.data.updatestyle;
				rx = readVarInt(&ba->color, pbuf, ps);
				ps -= rx;
				pbuf += rx;
				rx = readVarInt(&ba->dividers, pbuf, ps);
				ps -= rx;
				pbuf += rx;
			} else if (packet->data.play_server.bossbar.action == 5) {
				if (ps < 1) {
					free(pktbuf);
					return -1;
				}
				packet->data.play_server.bossbar.data.flags = pbuf[0];
				pbuf++;
				ps--;
			}
		} else if (id == PKT_PLAY_SERVER_SERVERDIFFICULTY) {
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.serverdifficulty.difficulty = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_TABCOMPLETE) {
			int rx = readVarInt(&packet->data.play_server.tabcomplete.count, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			packet->data.play_server.tabcomplete.matches = malloc(sizeof(char*) * packet->data.play_server.tabcomplete.count);
			for (int32_t i = 0; i < packet->data.play_server.tabcomplete.count; i++) {
				rx = readString(&(packet->data.play_server.tabcomplete.matches[i]), pbuf, ps);
				ps -= rx;
				pbuf += rx;
			}
		} else if (id == PKT_PLAY_SERVER_CHATMESSAGE) {
			int rx = readString(&packet->data.play_server.chatmessage.json, pbuf, ps);
			ps -= rx;
			pbuf += rx;
			if (ps < 1) {
				free(packet->data.play_server.chatmessage.json);
				free(pktbuf);
				return -1;
			}
		} else if (id == PKT_PLAY_SERVER_MULTIBLOCKCHANGE) {
			if (ps < 9) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.multiblockchange.x, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.multiblockchange.x, 4);
			memcpy(&packet->data.play_server.multiblockchange.z, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.multiblockchange.z, 4);
			int rx = readVarInt(&packet->data.play_server.multiblockchange.record_count, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			packet->data.play_server.multiblockchange.records = malloc(sizeof(struct mbc_record) * packet->data.play_server.multiblockchange.record_count);
			for (int i = 0; i < packet->data.play_server.multiblockchange.record_count; i++) {
				if (ps < 3) {
					free(packet->data.play_server.multiblockchange.records);
					free(pktbuf);
					return -1;
				}
				unsigned char hpos = pbuf[0];
				pbuf++;
				ps--;
				packet->data.play_server.multiblockchange.records[i].x = (hpos & 0xF0) >> 4;
				packet->data.play_server.multiblockchange.records[i].z = (hpos & 0x0F);
				packet->data.play_server.multiblockchange.records[i].y = pbuf[0];
				pbuf++;
				ps--;
				rx = readVarInt(&packet->data.play_server.multiblockchange.records[i].blockID, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			}
		} else if (id == PKT_PLAY_SERVER_CONFIRMTRANSACTION) {
			if (ps < 4) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.confirmtransaction.windowID = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.confirmtransaction.actionID, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			packet->data.play_server.confirmtransaction.accepted = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_CLOSEWINDOW) {
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.closewindow.windowID = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_OPENWINDOW) {
			if (ps < 6) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.openwindow.windowID = pbuf[0];
			pbuf++;
			ps--;
			int rx = readString(&packet->data.play_server.openwindow.type, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readString(&packet->data.play_server.openwindow.title, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.openwindow.slot_count = pbuf[0];
			pbuf++;
			ps--;
			if (ps >= 4) {
				memcpy(&packet->data.play_server.openwindow.entityID, pbuf, 4);
				pbuf += 4;
				ps -= 4;
				swapEndian(&packet->data.play_server.openwindow.entityID, 4);
			}
		} else if (id == PKT_PLAY_SERVER_WINDOWITEMS) {
			if (ps < 3) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.windowitems.windowID = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.windowitems.count, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&packet->data.play_server.windowitems.count, 2);
			size_t ss = packet->data.play_server.windowitems.count * sizeof(struct slot);
			packet->data.play_server.windowitems.slots = malloc(ss);
			for (int16_t i = 0; i < packet->data.play_server.windowitems.count; i++) {
				int rx = readSlot(&packet->data.play_server.windowitems.slots[i], pbuf, ps);
				pbuf += rx;
				ps -= rx;
			}
		} else if (id == PKT_PLAY_SERVER_WINDOWPROPERTY) {
			if (ps < 5) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.windowproperty.windowID = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.windowproperty.property, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&packet->data.play_server.windowproperty.property, 2);
			memcpy(&packet->data.play_server.windowproperty.value, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&packet->data.play_server.windowproperty.value, 2);
		} else if (id == PKT_PLAY_SERVER_SETSLOT) {
			if (ps < 3) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.setslot.windowID = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.setslot.slot, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&packet->data.play_server.setslot.slot, 2);
			int rx = readSlot(&packet->data.play_server.setslot.data, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_SETCOOLDOWN) {
			int rx = readVarInt(&packet->data.play_server.setcooldown.itemID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.setcooldown.cooldown, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_PLUGINMESSAGE) {
			int rx = readString(&packet->data.play_server.pluginmessage.channel, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps == 0) {
				packet->data.play_server.pluginmessage.data = NULL;
				packet->data.play_server.pluginmessage.data_size = 0;
			} else {
				packet->data.play_server.pluginmessage.data = malloc(ps);
				packet->data.play_server.pluginmessage.data_size = ps;
				memcpy(packet->data.play_server.pluginmessage.data, pbuf, ps);
				pbuf += ps;
				ps = 0;
			}
		} else if (id == PKT_PLAY_SERVER_NAMEDSOUNDEFFECT) {
			int rx = readString(&packet->data.play_server.namedsoundeffect.name, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.namedsoundeffect.category, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 17) {
				free(packet->data.play_server.namedsoundeffect.name);
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.namedsoundeffect.x, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.namedsoundeffect.x, 4);
			memcpy(&packet->data.play_server.namedsoundeffect.y, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.namedsoundeffect.y, 4);
			memcpy(&packet->data.play_server.namedsoundeffect.z, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.namedsoundeffect.z, 4);
			memcpy(&packet->data.play_server.namedsoundeffect.volume, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.namedsoundeffect.volume, 4);
			unsigned char pitch = pbuf[0];
			pbuf++;
			ps--;
			packet->data.play_server.namedsoundeffect.pitch = ((float) pitch) / 63.;
		} else if (id == PKT_PLAY_SERVER_DISCONNECT) {
			int rx = readString(&packet->data.play_server.disconnect.reason, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_ENTITYSTATUS) {
			if (ps < 5) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.entitystatus.entityID, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.entitystatus.entityID, 4);
			packet->data.play_server.entitystatus.status = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_EXPLOSION) {
			if (ps < 20) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.explosion.x, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.x, 4);
			memcpy(&packet->data.play_server.explosion.y, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.y, 4);
			memcpy(&packet->data.play_server.explosion.z, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.z, 4);
			memcpy(&packet->data.play_server.explosion.radius, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.radius, 4);
			memcpy(&packet->data.play_server.explosion.record_count, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.record_count, 4);
			size_t px = sizeof(struct exp_record) * packet->data.play_server.explosion.record_count;
			if (ps < px + 12) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.explosion.records = malloc(px);
			for (int32_t i = 0; i < packet->data.play_server.explosion.record_count; i++) {
				memcpy(&packet->data.play_server.explosion.records[i], pbuf, sizeof(struct exp_record));
				pbuf += sizeof(struct exp_record);
				ps -= sizeof(struct exp_record);
			}
			memcpy(&packet->data.play_server.explosion.velX, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.velX, 4);
			memcpy(&packet->data.play_server.explosion.velY, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.velY, 4);
			memcpy(&packet->data.play_server.explosion.velZ, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.explosion.velZ, 4);
		} else if (id == PKT_PLAY_SERVER_UNLOADCHUNK) {
			if (ps < 8) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.unloadchunk.chunkX, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.unloadchunk.chunkX, 4);
			memcpy(&packet->data.play_server.unloadchunk.chunkZ, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.unloadchunk.chunkZ, 4);
		} else if (id == PKT_PLAY_SERVER_CHANGEGAMESTATE) {
			if (ps < 5) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.changegamestate.reason = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.changegamestate.value, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.changegamestate.value, 4);
		} else if (id == PKT_PLAY_SERVER_KEEPALIVE) {
			int rx = readVarInt(&packet->data.play_server.keepalive.key, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_CHUNKDATA) {
			if (ps < 9) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.chunkdata.x, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.chunkdata.x, 4);
			memcpy(&packet->data.play_server.chunkdata.z, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.chunkdata.z, 4);
			packet->data.play_server.chunkdata.continuous = pbuf[0];
			pbuf++;
			ps--;
			int rx = readVarInt(&packet->data.play_server.chunkdata.bitMask, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.chunkdata.size, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (packet->data.play_server.chunkdata.continuous) packet->data.play_server.chunkdata.size -= 256;
			if (packet->data.play_server.chunkdata.size <= 0 || packet->data.play_server.chunkdata.size > ps) {
				free(pktbuf);
				return -1;
			}
			unsigned char* chunk = malloc(packet->data.play_server.chunkdata.size);
			packet->data.play_server.chunkdata.data = chunk;
			memcpy(chunk, pbuf, packet->data.play_server.chunkdata.size);
			if (packet->data.play_server.chunkdata.continuous) {
				if (ps < 256) {
					free(chunk);
					free(pktbuf);
					return -1;
				}
				memcpy(packet->data.play_server.chunkdata.biomes, pbuf, 256);
				pbuf += 256;
				ps -= 256;
			} // else biomes is undefined
			rx = readVarInt(&packet->data.play_server.chunkdata.nbtc, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (packet->data.play_server.chunkdata.nbtc > 0) {
				packet->data.play_server.chunkdata.nbts = malloc(sizeof(struct nbt_tag*) * packet->data.play_server.chunkdata.nbtc);
				for (int32_t i = 0; i < packet->data.play_server.chunkdata.nbtc; i++) {
					packet->data.play_server.chunkdata.nbts[i] = malloc(sizeof(struct nbt_tag));
					memset(packet->data.play_server.chunkdata.nbts[i], 0, sizeof(struct nbt_tag));
					rx = readNBT(&packet->data.play_server.chunkdata.nbts[i], pbuf, ps);
					pbuf += rx;
					ps -= rx;
				}
			} else packet->data.play_server.chunkdata.nbts = NULL;
		} else if (id == PKT_PLAY_SERVER_EFFECT) {
			if (ps < (9 + sizeof(struct encpos))) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.effect.effectID, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.effect.effectID, 4);
			memcpy(&packet->data.play_server.effect.pos, pbuf, sizeof(struct encpos));
			pbuf += sizeof(struct encpos);
			ps -= sizeof(struct encpos);
			swapEndian(&packet->data.play_server.effect.pos, sizeof(struct encpos));
			memcpy(&packet->data.play_server.effect.data, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.effect.data, 4);
			packet->data.play_server.effect.disableRelVolume = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_PARTICLE) {
			if (ps < 37) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.particles.particleID, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.particleID, 4);
			packet->data.play_server.particles.longDistance = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.particles.x, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.x, 4);
			memcpy(&packet->data.play_server.particles.y, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.y, 4);
			memcpy(&packet->data.play_server.particles.z, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.z, 4);
			memcpy(&packet->data.play_server.particles.offX, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.offX, 4);
			memcpy(&packet->data.play_server.particles.offY, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.offY, 4);
			memcpy(&packet->data.play_server.particles.offZ, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.offZ, 4);
			memcpy(&packet->data.play_server.particles.data, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.data, 4);
			memcpy(&packet->data.play_server.particles.count, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.particles.count, 4);
			if (ps > 0) {
				int rx = readVarInt(&packet->data.play_server.particles.data1, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else {
				packet->data.play_server.particles.data1 = 0;
			}
			if (ps > 0) {
				int rx = readVarInt(&packet->data.play_server.particles.data2, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else {
				packet->data.play_server.particles.data2 = 0;
			}
		} else if (id == PKT_PLAY_SERVER_JOINGAME) {
			if (ps < 11) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.joingame.eid, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.joingame.eid, 4);
			packet->data.play_server.joingame.gamemode = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.joingame.dimension, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.joingame.dimension, 4);
			packet->data.play_server.joingame.difficulty = pbuf[0];
			pbuf++;
			ps--;
			packet->data.play_server.joingame.maxPlayers = pbuf[0];
			pbuf++;
			ps--;
			int rx = readString(&packet->data.play_server.joingame.levelType, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(packet->data.play_server.joingame.levelType);
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.joingame.reducedDebugInfo = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_MAP) {
			int rx = readVarInt(&packet->data.play_server.map.damage, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 2) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.map.scale = pbuf[0];
			pbuf++;
			ps--;
			packet->data.play_server.map.showIcons = pbuf[0];
			pbuf++;
			ps--;
			rx = readVarInt(&packet->data.play_server.map.icon_count, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			size_t ics = packet->data.play_server.map.icon_count * sizeof(struct map_icon);
			if (ps < ics) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.map.icons = malloc(ics);
			memcpy(packet->data.play_server.map.icons, pbuf, ics);
			pbuf += ics;
			ps -= ics;
			if (ps < 1) {
				free(packet->data.play_server.map.icons);
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.map.columns = pbuf[0];
			pbuf++;
			ps--;
			if (packet->data.play_server.map.columns > 0) {
				if (ps < 3) {
					free(packet->data.play_server.map.icons);
					free(pktbuf);
					return -1;
				}
				packet->data.play_server.map.rows = pbuf[0];
				pbuf++;
				ps--;
				packet->data.play_server.map.x = pbuf[0];
				pbuf++;
				ps--;
				packet->data.play_server.map.z = pbuf[0];
				pbuf++;
				ps--;
				rx = readVarInt(&packet->data.play_server.map.data_size, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				if (ps < packet->data.play_server.map.data_size) {
					free(packet->data.play_server.map.icons);
					free(pktbuf);
					return -1;
				}
				packet->data.play_server.map.data = malloc(packet->data.play_server.map.data_size);
				memcpy(packet->data.play_server.map.data, pbuf, packet->data.play_server.map.data_size);
				pbuf += packet->data.play_server.map.data_size;
				ps -= packet->data.play_server.map.data_size;
			} else {
				packet->data.play_server.map.data = NULL;
			}
		} else if (id == PKT_PLAY_SERVER_ENTITYRELMOVE) {
			int rx = readVarInt(&packet->data.play_server.entityrelmove.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 7) {
				free(pktbuf);
				return -1;
			}
			int16_t dx;
			int16_t dy;
			int16_t dz;
			memcpy(&dx, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&dx, 2);
			memcpy(&dy, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&dy, 2);
			memcpy(&dz, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&dz, 2);
			packet->data.play_server.entityrelmove.dx = (float) dx / (128. * 32.);
			packet->data.play_server.entityrelmove.dy = (float) dy / (128. * 32.);
			packet->data.play_server.entityrelmove.dz = (float) dz / (128. * 32.);
			packet->data.play_server.entityrelmove.onGround = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_ENTITYLOOKRELMOVE) {
			int rx = readVarInt(&packet->data.play_server.entitylookrelmove.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 9) {
				free(pktbuf);
				return -1;
			}
			int16_t dx;
			int16_t dy;
			int16_t dz;
			memcpy(&dx, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&dx, 2);
			memcpy(&dy, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&dy, 2);
			memcpy(&dz, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&dz, 2);
			packet->data.play_server.entitylookrelmove.dx = (float) dx / (128. * 32.);
			packet->data.play_server.entitylookrelmove.dy = (float) dy / (128. * 32.);
			packet->data.play_server.entitylookrelmove.dz = (float) dz / (128. * 32.);
			packet->data.play_server.entitylookrelmove.yaw = (float) pbuf[0] / 256. * 360.;
			pbuf++;
			ps--;
			packet->data.play_server.entitylookrelmove.pitch = (float) pbuf[0] / 256. * 360.;
			pbuf++;
			ps--;
			packet->data.play_server.entitylookrelmove.onGround = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_ENTITYLOOK) {
			int rx = readVarInt(&packet->data.play_server.entitylook.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 3) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.entitylook.yaw = (float) pbuf[0] / 256. * 360.;
			pbuf++;
			ps--;
			packet->data.play_server.entitylook.pitch = (float) pbuf[0] / 256. * 360.;
			pbuf++;
			ps--;
			packet->data.play_server.entitylook.onGround = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_ENTITY) {
			int rx = readVarInt(&packet->data.play_server.entitylook.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_VEHICLEMOVE) {
			if (ps < 32) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.vehiclemove.x, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.vehiclemove.x, 8);
			memcpy(&packet->data.play_server.vehiclemove.y, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.vehiclemove.y, 8);
			memcpy(&packet->data.play_server.vehiclemove.z, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.vehiclemove.z, 8);
			memcpy(&packet->data.play_server.vehiclemove.yaw, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.vehiclemove.yaw, 4);
			memcpy(&packet->data.play_server.vehiclemove.pitch, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.vehiclemove.pitch, 4);
		} else if (id == PKT_PLAY_SERVER_OPENSIGNEDITOR) {
			if (ps < sizeof(struct encpos)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.opensigneditor.pos, pbuf, sizeof(struct encpos));
			pbuf += sizeof(struct encpos);
			ps -= sizeof(struct encpos);
			swapEndian(&packet->data.play_server.opensigneditor.pos, sizeof(struct encpos));
		} else if (id == PKT_PLAY_SERVER_PLAYERABILITIES) {
			if (ps < 9) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.playerabilities.flags = pbuf[0];
			pbuf++;
			ps--;
			memcpy(&packet->data.play_server.playerabilities.flyingSpeed, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.playerabilities.flyingSpeed, 4);
			memcpy(&packet->data.play_server.playerabilities.fov, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.playerabilities.fov, 4);
		} else if (id == PKT_PLAY_SERVER_COMBATEVENT) {
			int rx = readVarInt(&packet->data.play_server.combatevent.event, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (packet->data.play_server.combatevent.event == 1) {
				rx = readVarInt(&packet->data.play_server.combatevent.duration, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				if (ps < 4) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.combatevent.entityID, pbuf, 4);
				pbuf += 4;
				ps -= 4;
			} else if (packet->data.play_server.combatevent.event == 2) {
				rx = readVarInt(&packet->data.play_server.combatevent.playerId, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				if (ps < 4) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.combatevent.entityID, pbuf, 4);
				pbuf += 4;
				ps -= 4;
				swapEndian(&packet->data.play_server.combatevent.entityID, 4);
				rx = readString(&packet->data.play_server.combatevent.message, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			}
		} else if (id == PKT_PLAY_SERVER_PLAYERLISTITEM) {
			int rx = readVarInt(&packet->data.play_server.playerlistitem.action, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.playerlistitem.player_count, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (packet->data.play_server.playerlistitem.player_count < 1) {
				free(pktbuf);
				return -1;
			}
			if (packet->data.play_server.playerlistitem.action < 0 || packet->data.play_server.playerlistitem.action > 3) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.playerlistitem.players = malloc(sizeof(struct li_player) * packet->data.play_server.playerlistitem.player_count);
			for (int32_t i = 0; i < packet->data.play_server.playerlistitem.player_count; i++) {
				struct li_player* lp = &packet->data.play_server.playerlistitem.players[i];
				if (ps < sizeof(struct uuid)) {
					packet->data.play_server.playerlistitem.player_count = i; // this is a hack to prevent memory leaks, the packet is probably horribly malformed
					if (i == 0) {
						free(packet->data.play_server.playerlistitem.players);
						packet->data.play_server.playerlistitem.players = NULL;
					}
					break;
				}
				memcpy(&lp->uuid, pbuf, sizeof(struct uuid));
				pbuf += sizeof(struct uuid);
				ps -= sizeof(struct uuid);
				if (packet->data.play_server.playerlistitem.action == 0) {
					rx = readString(&lp->type.add.name, pbuf, ps);
					pbuf += rx;
					ps -= rx;
					rx = readVarInt(&lp->type.add.prop_count, pbuf, ps);
					pbuf += rx;
					ps -= rx;
					lp->type.add.props = malloc(lp->type.add.prop_count * sizeof(struct li_property));
					for (int32_t x = 0; x < lp->type.add.prop_count; x++) {
						struct li_property* lip = &lp->type.add.props[x];
						rx = readString(&lip->name, pbuf, ps);
						pbuf += rx;
						ps -= rx;
						rx = readString(&lip->value, pbuf, ps);
						pbuf += rx;
						ps -= rx;
						if (ps < 1) {
							lp->type.add.prop_count = x;
							if (x == 0) {
								free(lip->name);
								free(lip->value);
								free(lp->type.add.props);
								lp->type.add.props = NULL;
							}
							break;
						}
						unsigned char iss = pbuf[0];
						pbuf++;
						ps--;
						if (iss) {
							rx = readString(&lip->signature, pbuf, ps);
							pbuf += rx;
							ps -= rx;
						} else {
							lip->signature = NULL;
						}
					}
					rx = readVarInt(&lp->type.add.gamemode, pbuf, ps);
					pbuf += rx;
					ps -= rx;
					rx = readVarInt(&lp->type.add.ping, pbuf, ps);
					pbuf += rx;
					ps -= rx;
					if (ps < 1) {
						lp->type.add.displayName = NULL;
					} else {
						unsigned char lx = pbuf[0];
						pbuf++;
						ps--;
						if (lx) {
							rx = readString(&lp->type.add.displayName, pbuf, ps);
							pbuf += rx;
							ps -= rx;
						} else {
							lp->type.add.displayName = NULL;
						}
					}
				} else if (packet->data.play_server.playerlistitem.action == 1) {
					rx = readVarInt(&lp->type.gamemode, pbuf, ps);
					pbuf += rx;
					ps -= rx;
				} else if (packet->data.play_server.playerlistitem.action == 2) {
					rx = readVarInt(&lp->type.latency, pbuf, ps);
					pbuf += rx;
					ps -= rx;
				} else if (packet->data.play_server.playerlistitem.action == 3) {
					if (ps < 1) {
						packet->data.play_server.playerlistitem.player_count = i; // this is a hack to prevent memory leaks, the packet is probably horribly malformed
						if (i == 0) {
							free(packet->data.play_server.playerlistitem.players);
							packet->data.play_server.playerlistitem.players = NULL;
						}
						break;
					}
					unsigned char ix = pbuf[0];
					pbuf++;
					ps--;
					if (ix) {
						rx = readString(&lp->type.displayName, pbuf, ps);
						pbuf += rx;
						ps -= rx;
					} else {
						lp->type.displayName = NULL;
					}
				}
			}
		} else if (id == PKT_PLAY_SERVER_PLAYERPOSLOOK) {
			if (ps < 33) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.playerposlook.x, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.playerposlook.x, 8);
			memcpy(&packet->data.play_server.playerposlook.y, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.playerposlook.y, 8);
			memcpy(&packet->data.play_server.playerposlook.z, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.playerposlook.z, 8);
			memcpy(&packet->data.play_server.playerposlook.yaw, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.playerposlook.yaw, 4);
			memcpy(&packet->data.play_server.playerposlook.pitch, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.playerposlook.pitch, 4);
			packet->data.play_server.playerposlook.flags = pbuf[0];
			pbuf++;
			ps--;
			int rx = readVarInt(&packet->data.play_server.playerposlook.teleportID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_USEBED) {
			int rx = readVarInt(&packet->data.play_server.usebed.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < sizeof(struct encpos)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.usebed.pos, pbuf, sizeof(struct encpos));
			pbuf += sizeof(struct encpos);
			ps -= sizeof(struct encpos);
			swapEndian(&packet->data.play_server.usebed.pos, sizeof(struct encpos));
		} else if (id == PKT_PLAY_SERVER_DESTROYENTITIES) {
			int rx = readVarInt(&packet->data.play_server.destroyentities.count, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			packet->data.play_server.destroyentities.entityIDs = malloc(packet->data.play_server.destroyentities.count * 4);
			for (int32_t i = 0; i < packet->data.play_server.destroyentities.count; i++) {
				rx = readVarInt(&packet->data.play_server.destroyentities.entityIDs[i], pbuf, ps);
				pbuf += rx;
				ps -= rx;
			}
		} else if (id == PKT_PLAY_SERVER_REMOVEENTITYEFFECT) {
			int rx = readVarInt(&packet->data.play_server.removeentityeffect.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.removeentityeffect.effectID = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_RESOURCEPACKSEND) {
			int rx = readString(&packet->data.play_server.resourcepacksend.url, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readString(&packet->data.play_server.resourcepacksend.hash, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_RESPAWN) {
			if (ps < 6) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.respawn.dimension, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.respawn.dimension, 4);
			packet->data.play_server.respawn.difficulty = pbuf[0];
			pbuf++;
			ps--;
			packet->data.play_server.respawn.gamemode = pbuf[0];
			pbuf++;
			ps--;
			int rx = readString(&packet->data.play_server.respawn.levelType, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_ENTITYHEADLOOK) {
			int rx = readVarInt(&packet->data.play_server.entityheadlook.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.spawnobject.pitch = ((float) pbuf[0] / 256. * 360.);
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_WORLDBORDER) {
			int rx = readVarInt(&packet->data.play_server.worldborder.action, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (packet->data.play_server.worldborder.action == 0) {
				if (ps < sizeof(double)) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.worldborder.wb_action.setsize_radius, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.setsize_radius, 8);
			} else if (packet->data.play_server.worldborder.action == 1) {
				if (ps < sizeof(double) * 2) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.worldborder.wb_action.lerpsize.oldradius, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.lerpsize.oldradius, 8);
				memcpy(&packet->data.play_server.worldborder.wb_action.lerpsize.newradius, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.lerpsize.newradius, 8);
				rx = readVarLong(&packet->data.play_server.worldborder.wb_action.lerpsize.speed, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else if (packet->data.play_server.worldborder.action == 2) {
				if (ps < sizeof(double) * 2) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.worldborder.wb_action.setcenter.x, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.setcenter.x, 8);
				memcpy(&packet->data.play_server.worldborder.wb_action.setcenter.z, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.setcenter.z, 8);
			} else if (packet->data.play_server.worldborder.action == 3) {
				if (ps < sizeof(double) * 4) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.worldborder.wb_action.initialize.x, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.initialize.x, 8);
				memcpy(&packet->data.play_server.worldborder.wb_action.initialize.z, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.initialize.z, 8);
				memcpy(&packet->data.play_server.worldborder.wb_action.initialize.oldradius, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.initialize.oldradius, 8);
				memcpy(&packet->data.play_server.worldborder.wb_action.initialize.newradius, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.worldborder.wb_action.initialize.newradius, 8);
				rx = readVarLong(&packet->data.play_server.worldborder.wb_action.initialize.speed, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				rx = readVarInt(&packet->data.play_server.worldborder.wb_action.initialize.portalBoundary, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				rx = readVarInt(&packet->data.play_server.worldborder.wb_action.initialize.warningTime, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				rx = readVarInt(&packet->data.play_server.worldborder.wb_action.initialize.warningBlocks, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else if (packet->data.play_server.worldborder.action == 4) {
				rx = readVarInt(&packet->data.play_server.worldborder.wb_action.warningTime, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else if (packet->data.play_server.worldborder.action == 5) {
				rx = readVarInt(&packet->data.play_server.worldborder.wb_action.warningBlocks, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else {
				free(pktbuf);
				return -1;
			}
		} else if (id == PKT_PLAY_SERVER_CAMERA) {
			int rx = readVarInt(&packet->data.play_server.camera.cameraID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_HELDITEMCHANGE) {
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.helditemchange.slot = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_DISPLAYSCOREBOARD) {
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.displayscoreboard.pos = pbuf[0];
			pbuf++;
			ps--;
			int rx = readString(&packet->data.play_server.displayscoreboard.name, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_ENTITYMETADATA) {
			int rx = readVarInt(&packet->data.play_server.entitymetadata.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			packet->data.play_server.entitymetadata.metadata_size = ps;
			packet->data.play_server.entitymetadata.metadata = malloc(ps);
			memcpy(packet->data.play_server.entitymetadata.metadata, pbuf, ps);
			pbuf += ps;
			ps = 0;
		} else if (id == PKT_PLAY_SERVER_ATTACHENTITY) {
			if (ps < 8) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.attachentity.entityID, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.attachentity.entityID, 4);
			memcpy(&packet->data.play_server.attachentity.vehicleID, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.attachentity.vehicleID, 4);
		} else if (id == PKT_PLAY_SERVER_ENTITYVELOCITY) {
			int rx = readVarInt(&packet->data.play_server.entityvelocity.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 6) {
				free(pktbuf);
				return -1;
			}
			int16_t vv;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.entityvelocity.velX = (float) vv / 8000.;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.entityvelocity.velY = (float) vv / 8000.;
			memcpy(&vv, pbuf, 2);
			pbuf += 2;
			ps -= 2;
			swapEndian(&vv, 2);
			packet->data.play_server.entityvelocity.velZ = (float) vv / 8000.;
		} else if (id == PKT_PLAY_SERVER_ENTITYEQUIPMENT) {
			int rx = readVarInt(&packet->data.play_server.entityequipment.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.entityequipment.slot, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readSlot(&packet->data.play_server.entityequipment.slotitem, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_SETEXPERIENCE) {
			if (ps < 4) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.setexperience.bar, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.setexperience.bar, 4);
			int rx = readVarInt(&packet->data.play_server.setexperience.level, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.setexperience.total, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_UPDATEHEALTH) {
			if (ps < 4) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.updatehealth.health, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.updatehealth.health, 4);
			int rx = readVarInt(&packet->data.play_server.updatehealth.food, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 4) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.updatehealth.saturation, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.updatehealth.saturation, 4);
		} else if (id == PKT_PLAY_SERVER_SCOREBOARDOBJECTIVE) {
			int rx = readString(&packet->data.play_server.scoreboardobjective.name, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.scoreboardobjective.mode = pbuf[0];
			pbuf++;
			ps--;
			if (packet->data.play_server.scoreboardobjective.mode == 0 || packet->data.play_server.scoreboardobjective.mode == 2) {
				rx = readString(&packet->data.play_server.scoreboardobjective.value, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				rx = readString(&packet->data.play_server.scoreboardobjective.type, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			}
		} else if (id == PKT_PLAY_SERVER_SETPASSENGERS) {
			int rx = readVarInt(&packet->data.play_server.setpassengers.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.setpassengers.passenger_count, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			packet->data.play_server.setpassengers.passengers = NULL;
			if (packet->data.play_server.setpassengers.passenger_count > 0) {
				packet->data.play_server.setpassengers.passengers = malloc(packet->data.play_server.setpassengers.passenger_count * sizeof(int32_t));
				for (int32_t i = 0; i < packet->data.play_server.setpassengers.passenger_count; i++) {
					rx = readVarInt(&packet->data.play_server.setpassengers.passengers[i], pbuf, ps);
					pbuf += rx;
					ps -= rx;
				}
			}
		} else if (id == PKT_PLAY_SERVER_TEAMS) {
			int rx = readString(&packet->data.play_server.teams.name, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(packet->data.play_server.teams.name);
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.teams.mode = pbuf[0];
			pbuf++;
			ps--;
			if (packet->data.play_server.teams.mode == 0 || packet->data.play_server.teams.mode == 2) {
				rx = readString(&packet->data.play_server.teams.displayName, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				rx = readString(&packet->data.play_server.teams.prefix, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				rx = readString(&packet->data.play_server.teams.suffix, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				if (ps < 1) {
					free(packet->data.play_server.teams.name);
					free(packet->data.play_server.teams.displayName);
					free(packet->data.play_server.teams.prefix);
					free(packet->data.play_server.teams.suffix);
					free(pktbuf);
					return -1;
				}
				packet->data.play_server.teams.friendlyFire = pbuf[0];
				pbuf++;
				ps--;
				rx = readString(&packet->data.play_server.teams.nameTagVisibility, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				rx = readString(&packet->data.play_server.teams.collisionRule, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				if (ps < 1) {
					free(packet->data.play_server.teams.name);
					free(packet->data.play_server.teams.displayName);
					free(packet->data.play_server.teams.prefix);
					free(packet->data.play_server.teams.suffix);
					free(packet->data.play_server.teams.nameTagVisibility);
					free(packet->data.play_server.teams.collisionRule);
					free(pktbuf);
					return -1;
				}
				packet->data.play_server.teams.color = pbuf[0];
				pbuf++;
				ps--;
			}
			if (packet->data.play_server.teams.mode == 0 || packet->data.play_server.teams.mode == 3 || packet->data.play_server.teams.mode == 4) {
				rx = readVarInt(&packet->data.play_server.teams.player_count, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				packet->data.play_server.teams.players = malloc(sizeof(char*) * packet->data.play_server.teams.player_count);
				for (int32_t i = 0; i < packet->data.play_server.teams.player_count; i++) {
					rx = readString(&packet->data.play_server.teams.players[i], pbuf, ps);
					pbuf += rx;
					ps -= rx;
				}
			}
		} else if (id == PKT_PLAY_SERVER_UPDATESCORE) {
			int rx = readString(&packet->data.play_server.updatescore.scorename, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(packet->data.play_server.updatescore.scorename);
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.updatescore.action = pbuf[0];
			pbuf++;
			ps--;
			rx = readString(&packet->data.play_server.updatescore.objname, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (packet->data.play_server.updatescore.action != 1) {
				rx = readVarInt(&packet->data.play_server.updatescore.value, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			}
		} else if (id == PKT_PLAY_SERVER_SPAWNPOSITION) {
			if (ps < sizeof(struct encpos)) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.spawnposition.pos, pbuf, sizeof(struct encpos));
			pbuf += sizeof(struct encpos);
			ps -= sizeof(struct encpos);
			swapEndian(&packet->data.play_server.spawnposition.pos, sizeof(struct encpos));
		} else if (id == PKT_PLAY_SERVER_TIMEUPDATE) {
			if (ps < 16) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.timeupdate.worldAge, pbuf, 8);
			pbuf += 8;
			ps -= 8;
			swapEndian(&packet->data.play_server.timeupdate.worldAge, 8);
			memcpy(&packet->data.play_server.timeupdate.time, pbuf, 8);
			pbuf += 8;
			ps -= 8;
			swapEndian(&packet->data.play_server.timeupdate.time, 8);
		} else if (id == PKT_PLAY_SERVER_TITLE) {
			int rx = readVarInt(&packet->data.play_server.title.action, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (packet->data.play_server.title.action == 0) {
				rx = readString(&packet->data.play_server.title.act.title, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else if (packet->data.play_server.title.action == 1) {
				rx = readString(&packet->data.play_server.title.act.subtitle, pbuf, ps);
				pbuf += rx;
				ps -= rx;
			} else if (packet->data.play_server.title.action == 2) {
				if (ps < 12) {
					free(pktbuf);
					return -1;
				}
				memcpy(&packet->data.play_server.title.act.set.fadein, pbuf, 4);
				pbuf += 4;
				ps -= 4;
				swapEndian(&packet->data.play_server.title.act.set.fadein, 4);
				memcpy(&packet->data.play_server.title.act.set.stay, pbuf, 4);
				pbuf += 4;
				ps -= 4;
				swapEndian(&packet->data.play_server.title.act.set.stay, 4);
				memcpy(&packet->data.play_server.title.act.set.fadeout, pbuf, 4);
				pbuf += 4;
				ps -= 4;
				swapEndian(&packet->data.play_server.title.act.set.fadeout, 4);
			}
		} else if (id == PKT_PLAY_SERVER_SOUNDEFFECT) {
			int rx = readVarInt(&packet->data.play_server.soundeffect.id, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.soundeffect.category, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 17) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.soundeffect.x, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.soundeffect.x, 4);
			memcpy(&packet->data.play_server.soundeffect.y, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.soundeffect.y, 4);
			memcpy(&packet->data.play_server.soundeffect.z, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.soundeffect.z, 4);
			memcpy(&packet->data.play_server.soundeffect.volume, pbuf, sizeof(float));
			pbuf += sizeof(float);
			ps -= sizeof(float);
			swapEndian(&packet->data.play_server.soundeffect.volume, 4);
			unsigned char pit = pbuf[0];
			pbuf++;
			ps--;
			packet->data.play_server.soundeffect.pitch = pit / 63.5;
		} else if (id == PKT_PLAY_SERVER_PLAYERLISTHEADERFOOTER) {
			int rx = readString(&packet->data.play_server.playerlistheaderfooter.header, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readString(&packet->data.play_server.playerlistheaderfooter.footer, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_COLLECTITEM) {
			int rx = readVarInt(&packet->data.play_server.collectitem.collectedEntityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.play_server.collectitem.collectorEntityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_PLAY_SERVER_ENTITYTELEPORT) {
			int rx = readVarInt(&packet->data.play_server.entityteleport.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 27) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.entityteleport.x, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.entityteleport.x, 8);
			memcpy(&packet->data.play_server.entityteleport.y, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.entityteleport.y, 8);
			memcpy(&packet->data.play_server.entityteleport.z, pbuf, sizeof(double));
			pbuf += sizeof(double);
			ps -= sizeof(double);
			swapEndian(&packet->data.play_server.entityteleport.z, 8);
			packet->data.play_server.entityteleport.yaw = ((float) pbuf[0] / 256. * 360.);
			pbuf++;
			ps--;
			packet->data.play_server.entityteleport.pitch = ((float) pbuf[0] / 256. * 360.);
			pbuf++;
			ps--;
			packet->data.play_server.entityteleport.onGround = pbuf[0];
			pbuf++;
			ps--;
		} else if (id == PKT_PLAY_SERVER_ENTITYPROPERTIES) {
			int rx = readVarInt(&packet->data.play_server.entityproperties.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 4) {
				free(pktbuf);
				return -1;
			}
			memcpy(&packet->data.play_server.entityproperties.properties_count, pbuf, 4);
			pbuf += 4;
			ps -= 4;
			swapEndian(&packet->data.play_server.entityproperties.properties_count, 4);
			packet->data.play_server.entityproperties.properties = malloc(packet->data.play_server.entityproperties.properties_count * sizeof(struct entity_properties));
			for (int32_t i = 0; i < packet->data.play_server.entityproperties.properties_count; i++) {
				rx = readString(&(packet->data.play_server.entityproperties.properties[i].key), pbuf, ps);
				pbuf += rx;
				ps -= rx;
				if (ps < sizeof(double)) {
					free(packet->data.play_server.entityproperties.properties[i].key);
					packet->data.play_server.entityproperties.properties_count = i;
					if (i == 0) {
						free(packet->data.play_server.entityproperties.properties);
						packet->data.play_server.entityproperties.properties = NULL;
					}
					break;
				}
				memcpy(&packet->data.play_server.entityproperties.properties[i].value, pbuf, sizeof(double));
				pbuf += sizeof(double);
				ps -= sizeof(double);
				swapEndian(&packet->data.play_server.entityproperties.properties[i].value, 4);
				rx = readVarInt(&packet->data.play_server.entityproperties.properties[i].mod_count, pbuf, ps);
				pbuf += rx;
				ps -= rx;
				if (packet->data.play_server.entityproperties.properties[i].mod_count == 0) {
					packet->data.play_server.entityproperties.properties[i].mods = NULL;
				} else {
					size_t px = packet->data.play_server.entityproperties.properties[i].mod_count * sizeof(struct entity_mod);
					if (ps < px) {
						free(packet->data.play_server.entityproperties.properties[i].key);
						packet->data.play_server.entityproperties.properties_count = i;
						if (i == 0) {
							free(packet->data.play_server.entityproperties.properties);
							packet->data.play_server.entityproperties.properties = NULL;
						}
						break;
					}
					packet->data.play_server.entityproperties.properties[i].mods = malloc(px);
					for (int32_t x = 0; x < packet->data.play_server.entityproperties.properties[i].mod_count; x++) {
						memcpy(&packet->data.play_server.entityproperties.properties[i].mods[x].uuid, pbuf, sizeof(struct uuid));
						pbuf += sizeof(struct uuid);
						ps -= sizeof(struct uuid);
						memcpy(&packet->data.play_server.entityproperties.properties[i].mods[x].amount, pbuf, sizeof(double));
						pbuf += sizeof(double);
						ps -= sizeof(double);
						swapEndian(&packet->data.play_server.entityproperties.properties[i].mods[x].amount, 4);
						packet->data.play_server.entityproperties.properties[i].mods[x].op = pbuf[0];
						pbuf++;
						ps--;
					}
				}
			}
		} else if (id == PKT_PLAY_SERVER_ENTITYEFFECT) {
			int rx = readVarInt(&packet->data.play_server.entityeffect.entityID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 2) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.entityeffect.effectID = pbuf[0];
			pbuf++;
			ps--;
			packet->data.play_server.entityeffect.amplifier = pbuf[0];
			pbuf++;
			ps--;
			rx = readVarInt(&packet->data.play_server.entityeffect.duration, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < 1) {
				free(pktbuf);
				return -1;
			}
			packet->data.play_server.entityeffect.hideParticles = pbuf[0];
			pbuf++;
			ps--;
		}
	} else if (conn->state == STATE_LOGIN) {
		if (id == PKT_LOGIN_SERVER_DISCONNECT) {
			int rx = readString(&packet->data.login_server.disconnect.reason, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_LOGIN_SERVER_ENCRYPTIONREQUEST) {
			int rx = readString(&packet->data.login_server.encryptionrequest.serverID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readVarInt(&packet->data.login_server.encryptionrequest.publicKey_size, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < packet->data.login_server.encryptionrequest.publicKey_size) {
				free(pktbuf);
				free(packet->data.login_server.encryptionrequest.serverID);
				return -1;
			}
			packet->data.login_server.encryptionrequest.publicKey = malloc(packet->data.login_server.encryptionrequest.publicKey_size);
			memcpy(packet->data.login_server.encryptionrequest.publicKey, pbuf, packet->data.login_server.encryptionrequest.publicKey_size);
			rx = readVarInt(&packet->data.login_server.encryptionrequest.verifyToken_size, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			if (ps < packet->data.login_server.encryptionrequest.verifyToken_size) {
				free(pktbuf);
				free(packet->data.login_server.encryptionrequest.serverID);
				free(packet->data.login_server.encryptionrequest.publicKey);
				return -1;
			}
			packet->data.login_server.encryptionrequest.verifyToken = malloc(packet->data.login_server.encryptionrequest.verifyToken_size);
			memcpy(packet->data.login_server.encryptionrequest.verifyToken, pbuf, packet->data.login_server.encryptionrequest.verifyToken_size);
		} else if (id == PKT_LOGIN_SERVER_LOGINSUCCESS) {
			int rx = readString(&packet->data.login_server.loginsuccess.UUID, pbuf, ps);
			pbuf += rx;
			ps -= rx;
			rx = readString(&packet->data.login_server.loginsuccess.username, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		} else if (id == PKT_LOGIN_SERVER_SETCOMPRESSION) {
			int rx = readVarInt(&packet->data.login_server.setcompression.threshold, pbuf, ps);
			pbuf += rx;
			ps -= rx;
		}
	}
	free(pktbuf);
	return 0;
}

int writePacket(struct conn* conn, struct packet* packet) {
	unsigned char* pktbuf = xmalloc(1024); // TODO free
	size_t ps = 1024;
	size_t pi = 0;
	pi += writeVarInt(packet->id, pktbuf + pi);
	if (conn->state == STATE_PLAY) {
		if (packet->id == PKT_PLAY_CLIENT_TELEPORTCONFIRM) {
			pi += writeVarInt(packet->data.play_client.teleportconfirm.teleportID, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_TABCOMPLETE) {
			pi += writeString(packet->data.play_client.tabcomplete.text, pktbuf + pi, ps - pi);
			if (pi > ps - 64) {
				ps += 64;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pktbuf[pi++] = packet->data.play_client.tabcomplete.assumecommand;
			pktbuf[pi++] = packet->data.play_client.tabcomplete.haspos;
			if (packet->data.play_client.tabcomplete.haspos) {
				memcpy(pktbuf + pi, &packet->data.play_client.tabcomplete.pos, sizeof(struct encpos));
				swapEndian(pktbuf + pi, sizeof(struct encpos));
				pi += sizeof(struct encpos);
			}
		} else if (packet->id == PKT_PLAY_CLIENT_CHATMESSAGE) {
			pi += writeString(packet->data.play_client.chatmessage.message, pktbuf + pi, ps - pi);
		} else if (packet->id == PKT_PLAY_CLIENT_CLIENTSTATUS) {
			pi += writeVarInt(packet->data.play_client.clientstatus.actionID, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_CLIENTSETTINGS) {
			pi += writeString(packet->data.play_client.clientsettings.locale, pktbuf + pi, ps - pi);
			if (pi > ps - 64) {
				ps += 64;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pktbuf[pi++] = packet->data.play_client.clientsettings.viewDistance;
			pi += writeVarInt(packet->data.play_client.clientsettings.chatMode, pktbuf + pi);
			pktbuf[pi++] = packet->data.play_client.clientsettings.chatColors;
			pktbuf[pi++] = packet->data.play_client.clientsettings.displayedSkin;
			pi += writeVarInt(packet->data.play_client.clientsettings.mainHand, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_CONFIRMTRANSACTION) {
			pktbuf[pi++] = packet->data.play_client.confirmtransaction.windowID;
			memcpy(pktbuf + pi, &packet->data.play_client.confirmtransaction.actionNumber, 2);
			pi += 2;
			pktbuf[pi++] = packet->data.play_client.confirmtransaction.accepted;
		} else if (packet->id == PKT_PLAY_CLIENT_ENCHANTITEM) {
			pktbuf[pi++] = packet->data.play_client.enchantitem.windowID;
			pktbuf[pi++] = packet->data.play_client.enchantitem.enchantment;
		} else if (packet->id == PKT_PLAY_CLIENT_CLICKWINDOW) {
			pktbuf[pi++] = packet->data.play_client.clickwindow.windowID;
			memcpy(pktbuf + pi, &packet->data.play_client.clickwindow.slot, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			pktbuf[pi++] = packet->data.play_client.clickwindow.button;
			memcpy(pktbuf + pi, &packet->data.play_client.clickwindow.actionNumber, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			pktbuf[pi++] = packet->data.play_client.clickwindow.mode;
			pi += writeSlot(&packet->data.play_client.clickwindow.clicked, pktbuf + pi, ps - pi);
		} else if (packet->id == PKT_PLAY_CLIENT_CLOSEWINDOW) {
			pktbuf[pi++] = packet->data.play_client.closewindow.windowID;
		} else if (packet->id == PKT_PLAY_CLIENT_PLUGINMESSAGE) {
			pi += writeString(packet->data.play_client.pluginmessage.channel, pktbuf + pi, ps - pi);
			ps = strlen(packet->data.play_client.pluginmessage.channel) + 64 + packet->data.play_client.pluginmessage.data_size;
			pktbuf = xrealloc(pktbuf, ps);
			memcpy(pktbuf + pi, packet->data.play_client.pluginmessage.data, packet->data.play_client.pluginmessage.data_size);
		} else if (packet->id == PKT_PLAY_CLIENT_USEENTITY) {
			pi += writeVarInt(packet->data.play_client.useentity.target, pktbuf + pi);
			pi += writeVarInt(packet->data.play_client.useentity.type, pktbuf + pi);
			if (packet->data.play_client.useentity.type == 2) {
				memcpy(pktbuf + pi, &packet->data.play_client.useentity.targetX, sizeof(float));
				swapEndian(pktbuf + pi, 4);
				pi += sizeof(float);
				memcpy(pktbuf + pi, &packet->data.play_client.useentity.targetY, sizeof(float));
				swapEndian(pktbuf + pi, 4);
				pi += sizeof(float);
				memcpy(pktbuf + pi, &packet->data.play_client.useentity.targetZ, sizeof(float));
				swapEndian(pktbuf + pi, 4);
				pi += sizeof(float);
			}
			if (packet->data.play_client.useentity.type == 2 || packet->data.play_client.useentity.type == 0) {
				pi += writeVarInt(packet->data.play_client.useentity.hand, pktbuf + pi);
			}
		} else if (packet->id == PKT_PLAY_CLIENT_KEEPALIVE) {
			pi += writeVarInt(packet->data.play_client.keepalive.key, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_PLAYERPOS) {
			memcpy(pktbuf + pi, &packet->data.play_client.playerpos.x, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.playerpos.y, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.playerpos.z, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.playerpos.onGround, 1);
			pi++;
		} else if (packet->id == PKT_PLAY_CLIENT_PLAYERPOSLOOK) {
			memcpy(pktbuf + pi, &packet->data.play_client.playerposlook.x, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.playerposlook.y, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.playerposlook.z, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.playerposlook.yaw, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			memcpy(pktbuf + pi, &packet->data.play_client.playerposlook.pitch, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			memcpy(pktbuf + pi, &packet->data.play_client.playerposlook.onGround, 1);
			pi++;
		} else if (packet->id == PKT_PLAY_CLIENT_PLAYERLOOK) {
			memcpy(pktbuf + pi, &packet->data.play_client.playerlook.yaw, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			memcpy(pktbuf + pi, &packet->data.play_client.playerlook.pitch, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			memcpy(pktbuf + pi, &packet->data.play_client.playerlook.onGround, 1);
			pi++;
		} else if (packet->id == PKT_PLAY_CLIENT_PLAYER) {
			memcpy(pktbuf + pi, &packet->data.play_client.player.onGround, 1);
			pi++;
		} else if (packet->id == PKT_PLAY_CLIENT_VEHICLEMOVE) {
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.x, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.y, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.z, sizeof(double));
			swapEndian(pktbuf + pi, 8);
			pi += sizeof(double);
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.yaw, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.pitch, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
		} else if (packet->id == PKT_PLAY_CLIENT_STEERBOAT) {
			pktbuf[pi++] = packet->data.play_client.steerboat.unk1;
			pktbuf[pi++] = packet->data.play_client.steerboat.unk2;
		} else if (packet->id == PKT_PLAY_CLIENT_PLAYERDIG) {
			memcpy(pktbuf + pi, &packet->data.play_client.playerdig.status, 1);
			pi++;
			memcpy(pktbuf + pi, &packet->data.play_client.playerdig.pos, sizeof(struct encpos));
			swapEndian(pktbuf + pi, sizeof(struct encpos));
			pi += sizeof(struct encpos);
			memcpy(pktbuf + pi, &packet->data.play_client.playerdig.face, 1);
			pi++;
		} else if (packet->id == PKT_PLAY_CLIENT_ENTITYACTION) {
			pi += writeVarInt(packet->data.play_client.entityaction.entityID, pktbuf + pi);
			pi += writeVarInt(packet->data.play_client.entityaction.actionID, pktbuf + pi);
			pi += writeVarInt(packet->data.play_client.entityaction.actionParameter, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_STEERVEHICLE) {
			memcpy(pktbuf + pi, &packet->data.play_client.steervehicle.sideways, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			memcpy(pktbuf + pi, &packet->data.play_client.steervehicle.forward, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			pktbuf[pi++] = packet->data.play_client.steervehicle.flags;
		} else if (packet->id == PKT_PLAY_CLIENT_RESOURCEPACKSTATUS) {
			pi += writeString(packet->data.play_client.resourcepackstatus.hash, pktbuf + pi, ps - pi);
			if (pi > ps - 64) {
				ps += 64;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pi += writeVarInt(packet->data.play_client.resourcepackstatus.result, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_HELDITEMCHANGE) {
			memcpy(pktbuf + pi, &packet->data.play_client.helditemchange.slot, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
		} else if (packet->id == PKT_PLAY_CLIENT_CREATIVEINVENTORYACTION) {
			memcpy(pktbuf + pi, &packet->data.play_client.creativeinventoryaction.slot, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			pi += writeSlot(&packet->data.play_client.creativeinventoryaction.clicked, pktbuf + pi, ps - pi);
		} else if (packet->id == PKT_PLAY_CLIENT_UPDATESIGN) {
			memcpy(pktbuf + pi, &packet->data.play_client.updatesign.pos, sizeof(struct encpos));
			swapEndian(pktbuf + pi, sizeof(struct encpos));
			pi += sizeof(struct encpos);
			pi += writeString(packet->data.play_client.updatesign.line1, pktbuf + pi, ps - pi);
			if (pi > ps - 512) {
				ps += 1024;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pi += writeString(packet->data.play_client.updatesign.line2, pktbuf + pi, ps - pi);
			if (pi > ps - 512) {
				ps += 1024;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pi += writeString(packet->data.play_client.updatesign.line3, pktbuf + pi, ps - pi);
			if (pi > ps - 512) {
				ps += 1024;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pi += writeString(packet->data.play_client.updatesign.line4, pktbuf + pi, ps - pi);
		} else if (packet->id == PKT_PLAY_CLIENT_ANIMATION) {
			pi += writeVarInt(packet->data.play_client.animation.hand, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_SPECTATE) {
			memcpy(pktbuf + pi, &packet->data.play_client.spectate.uuid, sizeof(struct uuid));
			pi += sizeof(struct uuid);
		} else if (packet->id == PKT_PLAY_CLIENT_PLAYERPLACE) {
			memcpy(pktbuf + pi, &packet->data.play_client.playerplace.loc, sizeof(struct encpos));
			swapEndian(pktbuf + pi, sizeof(struct encpos));
			pi += sizeof(struct encpos);
			pi += writeVarInt(packet->data.play_client.playerplace.face, pktbuf + pi);
			pi += writeVarInt(packet->data.play_client.playerplace.hand, pktbuf + pi);
			memcpy(pktbuf + pi, &packet->data.play_client.playerplace.curPosX, 1);
			pi++;
			memcpy(pktbuf + pi, &packet->data.play_client.playerplace.curPosY, 1);
			pi++;
			memcpy(pktbuf + pi, &packet->data.play_client.playerplace.curPosZ, 1);
			pi++;
		} else if (packet->id == PKT_PLAY_CLIENT_USEITEM) {
			pi += writeVarInt(packet->data.play_client.useitem.hand, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_CLIENT_PLAYERABILITIES) {
			pktbuf[pi++] = packet->data.play_client.playerabilities.flags;
			memcpy(pktbuf + pi, &packet->data.play_client.playerabilities.flyingSpeed, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
			memcpy(pktbuf + pi, &packet->data.play_client.playerabilities.walkingSpeed, sizeof(float));
			swapEndian(pktbuf + pi, 4);
			pi += sizeof(float);
		}
	} else if (conn->state == STATE_HANDSHAKE) {
		if (packet->id == PKT_HANDSHAKE_CLIENT_HANDSHAKE) {
			pi += writeVarInt(packet->data.handshake_client.handshake.protocol_version, pktbuf + pi);
			pi += writeString(packet->data.handshake_client.handshake.ip, pktbuf + pi, ps - pi);
			if (pi > ps - 64) {
				ps += 64;
				pktbuf = xrealloc(pktbuf, ps);
			}
			memcpy(pktbuf + pi, &packet->data.handshake_client.handshake.port, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			pi += writeVarInt(packet->data.handshake_client.handshake.state, pktbuf + pi);
		}
	} else if (conn->state == STATE_LOGIN) {
		if (packet->id == PKT_LOGIN_CLIENT_LOGINSTART) {
			pi += writeString(packet->data.login_client.loginstart.name, pktbuf + pi, ps - pi);
		} else if (packet->id == PKT_LOGIN_CLIENT_ENCRYPTIONRESPONSE) {
			ps = packet->data.login_client.encryptionresponse.sharedSecret_size + packet->data.login_client.encryptionresponse.verifyToken_size + 64;
			pktbuf = xrealloc(pktbuf, ps);
			pi += writeVarInt(packet->data.login_client.encryptionresponse.sharedSecret_size, pktbuf + pi);
			memcpy(pktbuf + pi, packet->data.login_client.encryptionresponse.sharedSecret, packet->data.login_client.encryptionresponse.sharedSecret_size);
			pi += packet->data.login_client.encryptionresponse.sharedSecret_size;
			pi += writeVarInt(packet->data.login_client.encryptionresponse.verifyToken_size, pktbuf + pi);
			memcpy(pktbuf + pi, packet->data.login_client.encryptionresponse.verifyToken, packet->data.login_client.encryptionresponse.verifyToken_size);
			pi += packet->data.login_client.encryptionresponse.verifyToken_size;
		}
	}
	int fpll = getVarIntSize(pi);
	void* wrt = NULL;
	size_t wrt_s = 0;
	int frp = 0;
	if (conn->comp >= 0 && (pi + fpll > conn->comp + 1) && (pi + fpll) <= 2097152) {
		frp = 1;
		z_stream strm;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		int dr = 0;
		if ((dr = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY)) != Z_OK) { // TODO: configurable level?
			return -1;
		}
		strm.avail_in = pi;
		strm.next_in = pktbuf;
		void* cdata = malloc(16384);
		size_t ts = 0;
		size_t cc = 16384;
		strm.avail_out = cc - ts;
		strm.next_out = cdata + ts;
		do {
			dr = deflate(&strm, Z_FINISH);
			ts = strm.total_out;
			if (ts >= cc) {
				cc = ts + 16384;
				cdata = realloc(cdata, cc);
			}
			if (dr == Z_STREAM_ERROR) {
				free(cdata);
				return -1;
			}
			strm.avail_out = cc - ts;
			strm.next_out = cdata + ts;
		}while (strm.avail_out == 0);
		deflateEnd(&strm);
		cdata = xrealloc(cdata, ts); // shrink
		wrt = cdata;
		wrt_s = ts;
	} else if (conn->comp >= 0) {
		writeVarInt_stream(pi + getVarIntSize(0), conn->fd);
		writeVarInt_stream(0, conn->fd);
		wrt = pktbuf;
		wrt_s = pi;
	} else {
		writeVarInt_stream(pi, conn->fd);
		wrt = pktbuf;
		wrt_s = pi;
	}
//TODO: encrypt
	size_t wi = 0;
	while (wi < wrt_s) {
#ifdef __MINGW32__
		ssize_t v = send(conn->fd, wrt + wi, wrt_s - wi, 0);
#else
		ssize_t v = write(conn->fd, wrt + wi, wrt_s - wi);
#endif
		if (v < 1) {
			if (frp) free(wrt);
			if (v == 0) errno = ECONNRESET;
			printf("err: %s\n", strerror(errno));
			return -1;
		}
		wi += v;
	}
	if (frp) xfree(wrt);
	xfree(pktbuf);
	//printf("write success\n");
	return 0;
}

#endif
