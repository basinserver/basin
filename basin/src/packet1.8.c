/*
 * packet1.8.c
 *
 *  Created on: Jun 23, 2016
 *      Author: root
 */

#include "globals.h"

#ifdef MC_VERSION_1_8_9

#include "network.h"
#include "packet1.8.h"
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
#include "util.h"

#define ADRX if(rx == 0) goto rer;pbuf += rx;ps -= rx;
#define ADX(x) pbuf += x;ps -= x;

ssize_t readPacket(struct conn* conn, unsigned char* buf, size_t buflen, struct packet* packet) {
	void* pktbuf = buf;
	int32_t pktlen = buflen;
	int tf = 0;
	if (conn->comp >= 0) {
		int32_t dl = 0;
		//printf("reading pl\n");
		//printf("pl = %i\n", pl);
		int rx = readVarInt(&dl, pktbuf, pktlen);
		if(rx == 0) return -1;
		pktlen -= rx;
		pktbuf += rx;
		//printf("dl = %i\n", dl);
		if(dl > 0 && pktlen > 0) {
			pktlen = dl;
			void* decmpbuf = malloc(dl);
			//
			z_stream strm;
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			int dr = 0;
			if ((dr = inflateInit(&strm)) != Z_OK) {
				free(decmpbuf);
				printf("Compression initialization error!\n");
				return -1;
			}
			strm.avail_in = pktlen;
			strm.next_in = pktbuf;
			strm.avail_out = dl;
			strm.next_out = decmpbuf;
			do {
				dr = inflate(&strm, Z_FINISH);
				if (dr == Z_STREAM_ERROR) {
					free(decmpbuf);
					printf("Compression Read Error\n");
					return -1;
				}
				strm.avail_out = pktlen - strm.total_out;
				strm.next_out = decmpbuf + strm.total_out;
			}while (strm.avail_in > 0 || strm.total_out < pktlen);
			inflateEnd(&strm);
			pktbuf = decmpbuf;
			pktlen = dl;
			tf = 1;
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
		if (id == PKT_PLAY_CLIENT_KEEPALIVE) {
			int rx = readVarInt(&packet->data.play_client.keepalive.key, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_CLIENT_CHATMESSAGE) {
			int rx = readString(&packet->data.play_client.chatmessage.message, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_CLIENT_USEENTITY) {
			int rx = readVarInt(&packet->data.play_client.useentity.target, pbuf, ps);
			ADRX
			rx = readVarInt(&packet->data.play_client.useentity.type, pbuf, ps);
			ADRX
			if(packet->data.play_client.useentity.type == 2) {
				if(ps < 12) goto rer;
				memcpy(&packet->data.play_client.useentity.targetX, pbuf, 4);
				ADX(4)
				memcpy(&packet->data.play_client.useentity.targetY, pbuf, 4);
				ADX(4)
				memcpy(&packet->data.play_client.useentity.targetZ, pbuf, 4);
				ADX(4)
			} else {
				packet->data.play_client.useentity.targetX = packet->data.play_client.useentity.targetY = packet->data.play_client.useentity.targetZ = 0.;
			}
		} else if (id == PKT_PLAY_CLIENT_PLAYER) {
			if (ps < 1) goto rer;
			memcpy(&packet->data.play_client.player.onGround, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_PLAYERPOSITION) {
			if (ps < 25) goto rer;
			memcpy(&packet->data.play_client.playerposition.x, pbuf, 8);
			ADX(8)
			memcpy(&packet->data.play_client.playerposition.y, pbuf, 8);
			ADX(8)
			memcpy(&packet->data.play_client.playerposition.z, pbuf, 8);
			ADX(8)
			memcpy(&packet->data.play_client.playerposition.onGround, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_PLAYERLOOK) {
			if (ps < 9) goto rer;
			memcpy(&packet->data.play_client.playerlook.yaw, pbuf, 4);
			ADX(4)
			memcpy(&packet->data.play_client.playerlook.pitch, pbuf, 4);
			ADX(4)
			memcpy(&packet->data.play_client.playerlook.onGround, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK) {
			if (ps < 33) goto rer;
			memcpy(&packet->data.play_client.playerpositionandlook.x, pbuf, 8);
			ADX(8)
			memcpy(&packet->data.play_client.playerpositionandlook.y, pbuf, 8);
			ADX(8)
			memcpy(&packet->data.play_client.playerpositionandlook.z, pbuf, 8);
			ADX(8)
			memcpy(&packet->data.play_client.playerpositionandlook.yaw, pbuf, 4);
			ADX(4)
			memcpy(&packet->data.play_client.playerpositionandlook.pitch, pbuf, 4);
			ADX(4)
			memcpy(&packet->data.play_client.playerpositionandlook.onGround, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_PLAYERDIG) {
			if (ps < 2 + sizeof(struct encpos)) goto rer;
			memcpy(&packet->data.play_client.playerdig.status, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.playerdig.pos, pbuf, sizeof(struct encpos));
			ADX(sizeof(struct encpos))
			memcpy(&packet->data.play_client.playerdig.face, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_PLAYERPLACE) {
			if (ps < 2 + sizeof(struct encpos)) goto rer;
			memcpy(&packet->data.play_client.playerplace.loc, pbuf, sizeof(struct encpos));
			ADX(sizeof(struct encpos))
			memcpy(&packet->data.play_client.playerplace.face, pbuf, 1);
			ADX(1)
			int rx = readSlot(&packet->data.play_client.playerplace.slot, pbuf, ps);
			ADRX
			if (ps < 3) goto rer;
			memcpy(&packet->data.play_client.playerplace.curPosX, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.playerplace.curPosY, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.playerplace.curPosZ, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_HELDITEMCHANGE) {
			if(ps < 2) goto rer;
			memcpy(&packet->data.play_client.helditemchange.slot, pbuf, 2);
			swapEndian(&packet->data.play_client.helditemchange.slot, 2);
			ADX(2)
		} else if (id == PKT_PLAY_CLIENT_ANIMATION) {

		} else if (id == PKT_PLAY_CLIENT_ENTITYACTION) {
			int rx = readVarInt(&packet->data.play_client.entityaction.entityID, pbuf, ps);
			ADRX
			rx = readVarInt(&packet->data.play_client.entityaction.actionID, pbuf, ps);
			ADRX
			rx = readVarInt(&packet->data.play_client.entityaction.actionParameter, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_CLIENT_STEERVEHICLE) {
			if(ps < 9) goto rer;
			memcpy(&packet->data.play_client.steervehicle.sideways, pbuf, 4);
			ADX(4)
			memcpy(&packet->data.play_client.steervehicle.forward, pbuf, 4);
			ADX(4)
			memcpy(&packet->data.play_client.steervehicle.flags, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_CLOSEWINDOW) {
			if(ps < 1) goto rer;
			memcpy(&packet->data.play_client.closewindow.windowID, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_CLICKWINDOW) {
			if(ps < 8) goto rer;
			memcpy(&packet->data.play_client.clickwindow.windowID, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.clickwindow.slot, pbuf, 2);
			swapEndian(&packet->data.play_client.clickwindow.slot, 2);
			ADX(2)
			memcpy(&packet->data.play_client.clickwindow.button, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.clickwindow.actionNumber, pbuf, 2);
			swapEndian(&packet->data.play_client.clickwindow.actionNumber, 2);
			ADX(2)
			memcpy(&packet->data.play_client.clickwindow.mode, pbuf, 1);
			ADX(1)
			int rx = readSlot(&packet->data.play_client.clickwindow.clicked, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_CLIENT_CONFIRMTRANSACTION) {
			if(ps < 4) goto rer;
			memcpy(&packet->data.play_client.confirmtransaction.windowID, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.confirmtransaction.actionNumber, pbuf, 2);
			swapEndian(&packet->data.play_client.confirmtransaction.actionNumber, 2);
			ADX(2)
			memcpy(&packet->data.play_client.confirmtransaction.accepted, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_CREATIVEINVENTORYACTION) {
			if(ps < 3) goto rer;
			memcpy(&packet->data.play_client.creativeinventoryaction.slot, pbuf, 2);
			swapEndian(&packet->data.play_client.creativeinventoryaction.slot, 2);
			ADX(2)
			int rx = readSlot(&packet->data.play_client.creativeinventoryaction.clicked, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_CLIENT_ENCHANTITEM) {
			if(ps < 2) goto rer;
			memcpy(&packet->data.play_client.enchantitem.windowID, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.enchantitem.enchantment, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_UPDATESIGN) {
			if(ps < sizeof(struct encpos)) goto rer;
			memcpy(&packet->data.play_client.updatesign.pos, pbuf, sizeof(struct encpos));
			ADX(sizeof(struct encpos))
			int rx = readString(&packet->data.play_client.updatesign.line[0], pbuf, ps);
			ADRX
			rx = readString(&packet->data.play_client.updatesign.line[1], pbuf, ps);
			ADRX
			rx = readString(&packet->data.play_client.updatesign.line[2], pbuf, ps);
			ADRX
			rx = readString(&packet->data.play_client.updatesign.line[3], pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_CLIENT_PLAYERABILITIES) {
			if(ps < 9) goto rer;
			memcpy(&packet->data.play_client.playerabilities.flags, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.playerabilities.flyingSpeed, pbuf, 4);
			ADX(4)
			memcpy(&packet->data.play_client.playerabilities.walkingSpeed, pbuf, 4);
			ADX(4)
		} else if (id == PKT_PLAY_CLIENT_TABCOMPLETE) {
			int rx = readString(&packet->data.play_client.tabcomplete.text, pbuf, ps);
			ADRX
			if(ps < 1) goto rer;
			memcpy(&packet->data.play_client.tabcomplete.haspos, pbuf, 1);
			ADX(1)
			if(packet->data.play_client.tabcomplete.haspos) {
				if(ps < sizeof(struct encpos)) goto rer;
				memcpy(&packet->data.play_client.tabcomplete.pos, pbuf, sizeof(struct encpos));
				ADX(sizeof(struct encpos))
			} else {
				memset(&packet->data.play_client.tabcomplete.pos, 0, sizeof(struct encpos));
			}
		} else if (id == PKT_PLAY_CLIENT_CLIENTSETTINGS) {
			int rx = readString(&packet->data.play_client.clientsettings.locale, pbuf, ps);
			ADRX
			if(ps < 4) goto rer;
			memcpy(&packet->data.play_client.clientsettings.viewDistance, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.clientsettings.chatMode, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.clientsettings.chatColors, pbuf, 1);
			ADX(1)
			memcpy(&packet->data.play_client.clientsettings.displayedSkin, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_CLIENT_CLIENTSTATUS) {
			int rx = readVarInt(&packet->data.play_client.clientstatus.actionID, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_CLIENT_PLUGINMESSAGE) {
			int rx = readString(&packet->data.play_client.pluginmessage.channel, pbuf, ps);
			ADRX
			packet->data.play_client.pluginmessage.data_size = ps;
			packet->data.play_client.pluginmessage.data = ps == 0 ? NULL : xmalloc(ps);
			if(ps > 0) memcpy(&packet->data.play_client.pluginmessage.data, pbuf, ps);
			ADX(ps)
		} else if (id == PKT_PLAY_CLIENT_SPECTATE) {
			if(ps < sizeof(struct uuid)) goto rer;
			memcpy(&packet->data.play_client.spectate.uuid, pbuf, sizeof(struct uuid));
			ADX(sizeof(struct uuid))
		} else if (id == PKT_PLAY_CLIENT_RESOURCEPACKSTATUS) {
			int rx = readString(&packet->data.play_client.resourcepackstatus.hash, pbuf, ps);
			ADRX
			rx = readVarInt(&packet->data.play_client.resourcepackstatus.result, pbuf, ps);
			ADRX
		}
	} else if (conn->state == STATE_HANDSHAKE) {
		if(id == PKT_HANDSHAKE_CLIENT_HANDSHAKE) {
			int rx = readVarInt(&packet->data.handshake_client.handshake.protocol_version, pbuf, ps);
			ADRX
			rx = readString(&packet->data.handshake_client.handshake.ip, pbuf, ps);
			ADRX
			if(ps < 2) goto rer;
			memcpy(&packet->data.handshake_client.handshake.port, pbuf, 2);
			ADX(2)
			swapEndian(&packet->data.handshake_client.handshake.port, 2);
			rx = readVarInt(&packet->data.handshake_client.handshake.state, pbuf, ps);
			ADRX
		}
	} else if (conn->state == STATE_LOGIN) {
		if (id == PKT_LOGIN_CLIENT_LOGINSTART) {
			int rx = readString(&packet->data.login_client.loginstart.name, pbuf, ps);
			ADRX
		} else if (id == PKT_LOGIN_CLIENT_ENCRYPTIONRESPONSE) {
			int rx = readVarInt(&packet->data.login_client.encryptionresponse.sharedSecret_size, pbuf, ps);
			ADRX
			swapEndian(&packet->data.login_client.encryptionresponse.sharedSecret_size, 4);
			if(packet->data.login_client.encryptionresponse.sharedSecret_size > 1024) goto rer;
			if(ps < packet->data.login_client.encryptionresponse.sharedSecret_size) goto rer;
			packet->data.login_client.encryptionresponse.sharedSecret = xmalloc(packet->data.login_client.encryptionresponse.sharedSecret_size);
			memcpy(packet->data.login_client.encryptionresponse.sharedSecret, pbuf, packet->data.login_client.encryptionresponse.sharedSecret_size);
			ADX(packet->data.login_client.encryptionresponse.sharedSecret_size)
			rx = readVarInt(&packet->data.login_client.encryptionresponse.verifyToken_size, pbuf, ps);
			ADRX
			swapEndian(&packet->data.login_client.encryptionresponse.verifyToken_size, 4);
			if(packet->data.login_client.encryptionresponse.verifyToken_size > 1024) goto rer;
			if(ps < packet->data.login_client.encryptionresponse.verifyToken_size) goto rer;
			packet->data.login_client.encryptionresponse.verifyToken = xmalloc(packet->data.login_client.encryptionresponse.verifyToken_size);
			memcpy(packet->data.login_client.encryptionresponse.verifyToken, pbuf, packet->data.login_client.encryptionresponse.verifyToken_size);
			ADX(packet->data.login_client.encryptionresponse.verifyToken_size)
		}
	} else if (conn->state == STATE_STATUS) {
		if(id == PKT_STATUS_CLIENT_PING) {
			if(ps < 8) goto rer;
			memcpy(&packet->data.status_client.ping.payload, pbuf, 8);
			ADX(8)
		}
	}
	goto rx;
	rer:;
	return -1;
	rx:;
	if(tf) xfree(pktbuf);
	return buflen;
}

ssize_t writePacket(struct conn* conn, struct packet* packet) {
	unsigned char* pktbuf = xmalloc(1024); // TODO free
	size_t ps = 1024;
	size_t pi = 0;
	int32_t id = packet->id;
	pi += writeVarInt(id, pktbuf + pi);
	if (conn->state == STATE_PLAY) {
		if (packet->id == PKT_PLAY_SERVER_KEEPALIVE) {
			pi += writeVarInt(packet->data.play_server.keepalive.key, pktbuf + pi);
		} else if (packet->id == PKT_PLAY_SERVER_JOINGAME) {
			memcpy(pktbuf + pi, &packet->data.play_server.joingame.eid, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			pktbuf[pi++] = packet->data.play_server.joingame.gamemode;
			pktbuf[pi++] = packet->data.play_server.joingame.dimension;
			pktbuf[pi++] = packet->data.play_server.joingame.difficulty;
			pktbuf[pi++] = packet->data.play_server.joingame.maxPlayers;
			pi += writeString(packet->data.play_server.joingame.levelType, pktbuf + pi, ps - pi);
			pktbuf[pi++] = packet->data.play_server.joingame.reducedDebugInfo;
		} else if (packet->id == PKT_PLAY_SERVER_CHATMESSAGE) {
			if(strlen(packet->data.play_server.chatmessage.json) > 1000) {
				ps = strlen(packet->data.play_server.chatmessage.json) + 32;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pi += writeString(packet->data.play_server.chatmessage.json, pktbuf + pi, ps - pi);
			pktbuf[pi++] = packet->data.play_server.chatmessage.pos;
		} else if (packet->id == PKT_PLAY_SERVER_TIMEUPDATE) {
			memcpy(pktbuf + pi, &packet->data.play_server.timeupdate.worldAge, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			memcpy(pktbuf + pi, &packet->data.play_server.timeupdate.time, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
		} else if (packet->id == PKT_PLAY_SERVER_ENTITYEQUIPMENT) {
			//TODO
		} else if (packet->id == PKT_PLAY_SERVER_SPAWNPOSITION) {
			memcpy(pktbuf + pi, &packet->data.play_server.spawnposition.pos, sizeof(struct encpos));
			pi += sizeof(struct encpos);
		} else if (packet->id == PKT_PLAY_SERVER_UPDATEHEALTH) {
			memcpy(pktbuf + pi, &packet->data.play_server.updatehealth.health, 4);
			pi += 4;
			pi += writeVarInt(packet->data.play_server.updatehealth.food, pktbuf + pi);
			memcpy(pktbuf + pi, &packet->data.play_server.updatehealth.saturation, 4);
			pi += 4;
		} else if (packet->id == PKT_PLAY_SERVER_RESPAWN) {
			memcpy(pktbuf + pi, &packet->data.play_server.respawn.dimension, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			pktbuf[pi++] = packet->data.play_server.respawn.difficulty;
			pktbuf[pi++] = packet->data.play_server.respawn.gamemode;
			pi += writeString(packet->data.play_server.respawn.levelType, pktbuf + pi, ps - pi);
		} else if (packet->id == PKT_PLAY_SERVER_PLAYERPOSITIONANDLOOK) {
			memcpy(pktbuf + pi, &packet->data.play_server.playerpositionandlook.x, 8);
			pi += 8;
			memcpy(pktbuf + pi, &packet->data.play_server.playerpositionandlook.y, 8);
			pi += 8;
			memcpy(pktbuf + pi, &packet->data.play_server.playerpositionandlook.z, 8);
			pi += 8;
			memcpy(pktbuf + pi, &packet->data.play_server.playerpositionandlook.yaw, 4);
			pi += 4;
			memcpy(pktbuf + pi, &packet->data.play_server.playerpositionandlook.pitch, 4);
			pi += 4;
			pktbuf[pi++] = packet->data.play_server.playerpositionandlook.flags;
		} else if (packet->id == PKT_PLAY_SERVER_HELDITEMCHANGE) {
			pktbuf[pi++] = packet->data.play_server.helditemchange.slot;
		} else if (packet->id == PKT_PLAY_SERVER_USEBED) {
			//TODO
		} else if (packet->id == PKT_PLAY_SERVER_ANIMATION) {
			pi += writeVarInt(packet->data.play_server.animation.entityID, pktbuf + pi);
			pktbuf[pi++] = packet->data.play_server.animation.anim;
		} else if (packet->id == PKT_PLAY_SERVER_SPAWNPLAYER) {
			pi += writeVarInt(packet->data.play_server.spawnplayer.entityID, pktbuf + pi);
			memcpy(pktbuf + pi, &packet->data.play_server.spawnplayer.uuid, sizeof(struct uuid));
			pi += sizeof(struct uuid);
			int32_t fp = (int32_t)(packet->data.play_server.spawnplayer.x * 5.);
			memcpy(pktbuf + pi, &fp, 4);
			pi += 4;
			fp = (int32_t)(packet->data.play_server.spawnplayer.y * 5.);
			memcpy(pktbuf + pi, &fp, 4);
			pi += 4;
			fp = (int32_t)(packet->data.play_server.spawnplayer.z * 5.);
			memcpy(pktbuf + pi, &fp, 4);
			pi += 4;
			pktbuf[pi++] = (unsigned char)(packet->data.play_server.spawnplayer.yaw / 360. * 256.);
			pktbuf[pi++] = (unsigned char)(packet->data.play_server.spawnplayer.pitch / 360. * 256.);
			if(packet->data.play_server.spawnplayer.metadata != NULL) {
				memcpy(pktbuf + pi, packet->data.play_server.spawnplayer.metadata, packet->data.play_server.spawnplayer.metadata_size);
				pi += packet->data.play_server.spawnplayer.metadata_size;
			}
		} else if (packet->id == PKT_PLAY_SERVER_COLLECTITEM) {

		} else if (packet->id == PKT_PLAY_SERVER_SPAWNOBJECT) {

		} else if (packet->id == PKT_PLAY_SERVER_SPAWNMOB) {

		} else if (packet->id == PKT_PLAY_SERVER_SPAWNPAINTING) {

		} else if (packet->id == PKT_PLAY_SERVER_SPAWNEXPERIENCEORB) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYVELOCITY) {

		} else if (packet->id == PKT_PLAY_SERVER_DESTROYENTITIES) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITY) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYRELMOVE) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYLOOK) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYLOOKANDRELMOVE) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYTELEPORT) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYHEADLOOK) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYSTATUS) {

		} else if (packet->id == PKT_PLAY_SERVER_ATTACHENTITY) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYMETADATA) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYEFFECT) {

		} else if (packet->id == PKT_PLAY_SERVER_REMOVEENTITYEFFECT) {

		} else if (packet->id == PKT_PLAY_SERVER_SETEXPERIENCE) {

		} else if (packet->id == PKT_PLAY_SERVER_ENTITYPROPERTIES) {

		} else if (packet->id == PKT_PLAY_SERVER_CHUNKDATA) {

		} else if (packet->id == PKT_PLAY_SERVER_MULTIBLOCKCHANGE) {

		} else if (packet->id == PKT_PLAY_SERVER_BLOCKCHANGE) {

		} else if (packet->id == PKT_PLAY_SERVER_BLOCKACTION) {

		} else if (packet->id == PKT_PLAY_SERVER_BLOCKBREAKANIMATION) {

		} else if (packet->id == PKT_PLAY_SERVER_MAPCHUNKBULK) {
			pktbuf[pi++] = packet->data.play_server.mapchunkbulk.skyLight;
			pi += writeVarInt(packet->data.play_server.mapchunkbulk.netchunk_count, pktbuf + pi);
			size_t ns = pi + 4 * packet->data.play_server.mapchunkbulk.netchunk_count;
			for (int32_t i = 0;i<packet->data.play_server.mapchunkbulk.netchunk_count;i++) {
				memcpy(pktbuf + pi, packet->data.play_server.mapchunkbulk.netchunks[i].chunkX, 4);
				swapEndian(pktbuf + pi, 4);
				pi += 4;
				memcpy(pktbuf + pi, packet->data.play_server.mapchunkbulk.netchunks[i].chunkZ, 4);
				swapEndian(pktbuf + pi, 4);
				pi += 4;
				memcpy(pktbuf + pi, packet->data.play_server.mapchunkbulk.netchunks[i].bitMask, 2);
				swapEndian(pktbuf + pi, 2);
				pi += 2;
				ns += packet->data.play_server.mapchunkbulk.netchunks[i].data_size + 128;
			}
			pktbuf = xrealloc(pktbuf, ns);
			for (int32_t i = 0;i<packet->data.play_server.mapchunkbulk.netchunk_count;i++) {
				memcpy(pktbuf + pi, packet->data.play_server.mapchunkbulk.netchunks[i].data, packet->data.play_server.mapchunkbulk.netchunks[i].data_size);
				pi += packet->data.play_server.mapchunkbulk.netchunks[i].data_size;
			}
		} else if (packet->id == PKT_PLAY_SERVER_EXPLOSION) {

		} else if (packet->id == PKT_PLAY_SERVER_EFFECT) {

		} else if (packet->id == PKT_PLAY_SERVER_SOUNDEFFECT) {

		} else if (packet->id == PKT_PLAY_SERVER_PARTICLE) {

		} else if (packet->id == PKT_PLAY_SERVER_CHANGEGAMESTATE) {

		} else if (packet->id == PKT_PLAY_SERVER_SPAWNGLOBALENTITY) {

		} else if (packet->id == PKT_PLAY_SERVER_OPENWINDOW) {

		} else if (packet->id == PKT_PLAY_SERVER_CLOSEWINDOW) {

		} else if (packet->id == PKT_PLAY_SERVER_SETSLOT) {

		} else if (packet->id == PKT_PLAY_SERVER_WINDOWITEMS) {

		} else if (packet->id == PKT_PLAY_SERVER_WINDOWPROPERTY) {

		} else if (packet->id == PKT_PLAY_SERVER_CONFIRMTRANSACTION) {

		} else if (packet->id == PKT_PLAY_SERVER_UPDATESIGN) {

		} else if (packet->id == PKT_PLAY_SERVER_MAP) {

		} else if (packet->id == PKT_PLAY_SERVER_UPDATEBLOCKENTITY) {

		} else if (packet->id == PKT_PLAY_SERVER_OPENSIGNEDITOR) {

		} else if (packet->id == PKT_PLAY_SERVER_STATISTICS) {

		} else if (packet->id == PKT_PLAY_SERVER_PLAYERLISTITEM) {

		} else if (packet->id == PKT_PLAY_SERVER_PLAYERABILITIES) {
			pktbuf[pi++] = packet->data.play_server.playerabilities.flags;
			memcpy(pktbuf + pi, &packet->data.play_server.playerabilities.flyingSpeed, 4);
			pi += 4;
			memcpy(pktbuf + pi, &packet->data.play_server.playerabilities.fov, 4);
			pi += 4;
		} else if (packet->id == PKT_PLAY_SERVER_TABCOMPLETE) {

		} else if (packet->id == PKT_PLAY_SERVER_SCOREBOARDOBJECTIVE) {

		} else if (packet->id == PKT_PLAY_SERVER_UPDATESCORE) {

		} else if (packet->id == PKT_PLAY_SERVER_DISPLAYSCOREBOARD) {

		} else if (packet->id == PKT_PLAY_SERVER_TEAMS) {

		} else if (packet->id == PKT_PLAY_SERVER_PLUGINMESSAGE) {
			pi += writeString(packet->data.play_server.pluginmessage.channel, pktbuf + pi, ps - pi);
			ps = packet->data.play_server.pluginmessage.data_size + pi + 16;
			pktbuf = xrealloc(pktbuf, ps);
			memcpy(pktbuf + pi, packet->data.play_server.pluginmessage.data);
			pi += packet->data.play_server.pluginmessage.data_size;
		} else if (packet->id == PKT_PLAY_SERVER_DISCONNECT) {
			pi += writeString(packet->data.play_server.disconnect.reason, pktbuf + pi, ps - pi);
		} else if (packet->id == PKT_PLAY_SERVER_SERVERDIFFICULTY) {
			pktbuf[pi++] = packet->data.play_server.serverdifficulty.difficulty;
		} else if (packet->id == PKT_PLAY_SERVER_COMBATEVENT) {

		} else if (packet->id == PKT_PLAY_SERVER_CAMERA) {

		} else if (packet->id == PKT_PLAY_SERVER_WORLDBORDER) {

		} else if (packet->id == PKT_PLAY_SERVER_TITLE) {

		} else if (packet->id == PKT_PLAY_SERVER_SETCOMPRESSION) {

		} else if (packet->id == PKT_PLAY_SERVER_PLAYERLISTHEADERFOOTER) {

		} else if (packet->id == PKT_PLAY_SERVER_RESOURCEPACKSEND) {

		} else if (packet->id == PKT_PLAY_SERVER_UPDATEENTITYNBT) {

		}
	} else if (conn->state == STATE_LOGIN) {
		if (id == PKT_LOGIN_SERVER_DISCONNECT) {
			pi += writeString(packet->data.login_server.disconnect.reason, pktbuf + pi, ps - pi);
		} else if (id == PKT_LOGIN_SERVER_ENCRYPTIONREQUEST) {
			pi += writeString(packet->data.login_server.encryptionrequest.serverID, pktbuf + pi, ps - pi);
			if(ps - pi < packet->data.login_server.encryptionrequest.publicKey_size) {
				ps = 256 + packet->data.login_server.encryptionrequest.publicKey_size + packet->data.login_server.encryptionrequest.verifyToken_size;
				pktbuf = xrealloc(pktbuf, ps);
			}
			pi += writeVarInt(packet->data.login_server.encryptionrequest.publicKey_size, pktbuf + pi);
			memcpy(pktbuf + pi, packet->data.login_server.encryptionrequest.publicKey, packet->data.login_server.encryptionrequest.publicKey_size);
			pi += packet->data.login_server.encryptionrequest.publicKey_size;
			pi += writeVarInt(packet->data.login_server.encryptionrequest.verifyToken_size, pktbuf + pi);
			memcpy(pktbuf + pi, packet->data.login_server.encryptionrequest.verifyToken, packet->data.login_server.encryptionrequest.verifyToken_size);
			pi += packet->data.login_server.encryptionrequest.verifyToken_size;
		} else if (id == PKT_LOGIN_SERVER_LOGINSUCCESS) {
			pi += writeString(packet->data.login_server.loginsuccess.UUID, pktbuf + pi, ps - pi);
			pi += writeString(packet->data.login_server.loginsuccess.username, pktbuf + pi, ps - pi);
		} else if (id == PKT_LOGIN_SERVER_SETCOMPRESSION) {
			pi += writeVarInt(packet->data.login_server.setcompression.threshold, pktbuf + pi);
		}
	} else if (conn->state == STATE_STATUS) {
		if (id == PKT_STATUS_SERVER_RESPONSE) {
			pi += writeString(packet->data.status_server.response.json, pktbuf + pi, ps - pi);
		} else if (id == PKT_STATUS_SERVER_PONG) {
			memcpy(&packet->data.status_server.pong.payload, pktbuf + pi, 8);
			pi += 8;
		}
	}
	int fpll = getVarIntSize(pi);
	void* wrt = NULL;
	size_t wrt_s = 0;
	int frp = 0;
	unsigned char prep[10];
	uint8_t preps = 0;
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
		preps += writeVarInt(ts + getVarIntSize(pi), prep + preps);
		preps += writeVarInt(pi, prep + preps);
		wrt = cdata;
		wrt_s = ts;
	} else if (conn->comp >= 0) {
		preps += writeVarInt(pi + getVarIntSize(0), prep + preps);
		preps += writeVarInt(0, prep + preps);
		wrt = pktbuf;
		wrt_s = pi;
	} else {
		preps += writeVarInt(pi, prep + preps);
		wrt = pktbuf;
		wrt_s = pi;
	}
//TODO: encrypt
	if(conn->writeBuffer == NULL) {
		conn->writeBuffer = xmalloc(preps);
		memcpy(conn->writeBuffer, prep, preps);
		conn->writeBuffer_size = preps;
	} else {
		conn->writeBuffer = xrealloc(conn->writeBuffer, conn->writeBuffer_size + preps);
		memcpy(conn->writeBuffer + conn->writeBuffer_size, prep, preps);
		conn->writeBuffer_size += preps;
	}
	conn->writeBuffer = xrealloc(conn->writeBuffer, conn->writeBuffer_size + wrt_s);
	memcpy(conn->writeBuffer + conn->writeBuffer_size, wrt, wrt_s);
	conn->writeBuffer_size += wrt_s;
	if (frp) xfree(wrt);
	xfree(pktbuf);
	//printf("write success\n");
	return wrt_s;
}

#endif
