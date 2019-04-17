


#include "basin/packet.h"
#include <basin/globals.h>
#include <basin/inventory.h>
#include <basin/network.h>
#include <basin/nbt.h>
#include <basin/connection.h>
#include <basin/world.h>
#include <basin/player.h>
#include <avuna/string.h>
#include <stdint.h>
#include <zlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

struct packet* packet_new(struct mempool* pool, int32_t id) {
	struct packet* packet = pcalloc(pool, sizeof(struct packet));
	packet->pool = pool;
	packet->id = id;
	return packet;
}

#define ADRX if(rx == 0) goto rer;pbuf += rx;ps -= rx;
#define ADX(x) pbuf += x;ps -= x;
#define CPS(x) if(ps < x) goto rer;
#define CPS_OPT(x) if(ps >= x) {
#define ENS(x) if(ps-pi < x) { ps += (x > 256 ? x + 1024 : 1024); pktbuf = prealloc(packet->pool, pktbuf - 10, ps + 10) + 10; }

ssize_t packet_read(struct connection* conn, unsigned char* buf, size_t buflen, struct packet* packet) {
	void* pktbuf = buf;
	int32_t pktlen = (int32_t) buflen;
	if (conn->compression_state >= 0) {
		int32_t decompressed_length = 0;
		int read_length = readVarInt(&decompressed_length, pktbuf, buflen);
		if (read_length == 0) goto rer;
		pktlen -= read_length;
		pktbuf += read_length;
		if (decompressed_length > 0 && pktlen > 0) {
			pktlen = decompressed_length;
			void* decompressed = pmalloc(packet->pool, (size_t) decompressed_length);
			z_stream strm;
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			int dr = 0;
			if ((dr = inflateInit(&strm)) != Z_OK) {
				errlog(conn->server->logger, "Compression initialization error!\n");
				goto rer;
			}
			strm.avail_in = (uInt) pktlen;
			strm.next_in = pktbuf;
			strm.avail_out = (uInt) decompressed_length;
			strm.next_out = decompressed;
			do {
				dr = inflate(&strm, Z_FINISH);
				if (dr == Z_STREAM_ERROR) {
					printf("Compression Read Error\n");
					goto rer;
				}
				strm.avail_out = (uInt) (pktlen - strm.total_out);
				strm.next_out = decompressed + strm.total_out;
			} while (strm.avail_in > 0 || strm.total_out < pktlen);
			inflateEnd(&strm);
			pktbuf = decompressed;
			pktlen = decompressed_length;
		}
	}
	if (pktbuf == NULL) return 0;
	unsigned char* pbuf = (unsigned char*) pktbuf;
	size_t ps = (size_t) pktlen;
	int32_t id = 0;
	size_t t = (size_t) readVarInt(&id, pbuf, ps);
	pbuf += t;
	ps -= t;
	packet->id = id;
	int rx = 0;
	if (conn->protocol_state == STATE_HANDSHAKE) {
		if (id == PKT_HANDSHAKE_SERVER_HANDSHAKE) {
			//protocol_version
			rx = readVarInt(&packet->data.handshake_server.handshake.protocol_version, pbuf, ps);
			ADRX
			//server_address
			rx = readString(packet->pool, &packet->data.handshake_server.handshake.server_address, pbuf, ps);
			ADRX
			//server_port
			CPS(2)
			memcpy(&packet->data.handshake_server.handshake.server_port, pbuf, 2);
			swapEndian(&packet->data.handshake_server.handshake.server_port, 2);
			ADX(2)
			//next_state
			rx = readVarInt(&packet->data.handshake_server.handshake.next_state, pbuf, ps);
			ADRX
		}
	} else if (conn->protocol_state == STATE_PLAY) {
		if (id == PKT_PLAY_SERVER_TELEPORTCONFIRM) {
			//teleport_id
			rx = readVarInt(&packet->data.play_server.teleportconfirm.teleport_id, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_TABCOMPLETE) {
			//text
			rx = readString(packet->pool, &packet->data.play_server.tabcomplete.text, pbuf, ps);
			ADRX
			//assume_command
			CPS(1)
			memcpy(&packet->data.play_server.tabcomplete.assume_command, pbuf, 1);
			ADX(1)
			//has_position
			CPS(1)
			memcpy(&packet->data.play_server.tabcomplete.has_position, pbuf, 1);
			ADX(1)
			//looked_at_block
			CPS_OPT(8)
				memcpy(&packet->data.play_server.tabcomplete.looked_at_block, pbuf, 8);
				swapEndian(&packet->data.play_server.tabcomplete.looked_at_block, 8);
				ADX(8)
			}
		} else if (id == PKT_PLAY_SERVER_CHATMESSAGE) {
			//message
			rx = readString(packet->pool, &packet->data.play_server.chatmessage.message, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_CLIENTSTATUS) {
			//action_id
			rx = readVarInt(&packet->data.play_server.clientstatus.action_id, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_CLIENTSETTINGS) {
			//locale
			rx = readString(packet->pool, &packet->data.play_server.clientsettings.locale, pbuf, ps);
			ADRX
			//view_distance
			CPS(1)
			memcpy(&packet->data.play_server.clientsettings.view_distance, pbuf, 1);
			ADX(1)
			//chat_mode
			rx = readVarInt(&packet->data.play_server.clientsettings.chat_mode, pbuf, ps);
			ADRX
			//chat_colors
			CPS(1)
			memcpy(&packet->data.play_server.clientsettings.chat_colors, pbuf, 1);
			ADX(1)
			//displayed_skin_parts
			CPS(1)
			memcpy(&packet->data.play_server.clientsettings.displayed_skin_parts, pbuf, 1);
			ADX(1)
			//main_hand
			rx = readVarInt(&packet->data.play_server.clientsettings.main_hand, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_CONFIRMTRANSACTION) {
			//window_id
			CPS(1)
			memcpy(&packet->data.play_server.confirmtransaction.window_id, pbuf, 1);
			ADX(1)
			//action_number
			CPS(2)
			memcpy(&packet->data.play_server.confirmtransaction.action_number, pbuf, 2);
			swapEndian(&packet->data.play_server.confirmtransaction.action_number, 2);
			ADX(2)
			//accepted
			CPS(1)
			memcpy(&packet->data.play_server.confirmtransaction.accepted, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_ENCHANTITEM) {
			//window_id
			CPS(1)
			memcpy(&packet->data.play_server.enchantitem.window_id, pbuf, 1);
			ADX(1)
			//enchantment
			CPS(1)
			memcpy(&packet->data.play_server.enchantitem.enchantment, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_CLICKWINDOW) {
			//window_id
			CPS(1)
			memcpy(&packet->data.play_server.clickwindow.window_id, pbuf, 1);
			ADX(1)
			//slot
			CPS(2)
			memcpy(&packet->data.play_server.clickwindow.slot, pbuf, 2);
			swapEndian(&packet->data.play_server.clickwindow.slot, 2);
			ADX(2)
			//button
			CPS(1)
			memcpy(&packet->data.play_server.clickwindow.button, pbuf, 1);
			ADX(1)
			//action_number
			CPS(2)
			memcpy(&packet->data.play_server.clickwindow.action_number, pbuf, 2);
			swapEndian(&packet->data.play_server.clickwindow.action_number, 2);
			ADX(2)
			//mode
			rx = readVarInt(&packet->data.play_server.clickwindow.mode, pbuf, ps);
			ADRX
			//clicked_item
			rx = readSlot(packet->pool, &packet->data.play_server.clickwindow.clicked_item, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_CLOSEWINDOW) {
			//window_id
			CPS(1)
			memcpy(&packet->data.play_server.closewindow.window_id, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_PLUGINMESSAGE) {
			//channel
			rx = readString(packet->pool, &packet->data.play_server.pluginmessage.channel, pbuf, ps);
			ADRX
			//data
			CPS(ps)
			packet->data.play_server.pluginmessage.data = pmalloc(packet->pool, ps);
			memcpy(packet->data.play_server.pluginmessage.data, pbuf, ps);
			ADX(ps)
		} else if (id == PKT_PLAY_SERVER_USEENTITY) {
			//target
			rx = readVarInt(&packet->data.play_server.useentity.target, pbuf, ps);
			ADRX
			//type
			rx = readVarInt(&packet->data.play_server.useentity.type, pbuf, ps);
			ADRX
			//target_x
			CPS_OPT(4)
				memcpy(&packet->data.play_server.useentity.target_x, pbuf, 4);
				swapEndian(&packet->data.play_server.useentity.target_x, 4);
				ADX(4)
			}
			//target_y
			CPS_OPT(4)
				memcpy(&packet->data.play_server.useentity.target_y, pbuf, 4);
				swapEndian(&packet->data.play_server.useentity.target_y, 4);
				ADX(4)
			}
			//target_z
			CPS_OPT(4)
				memcpy(&packet->data.play_server.useentity.target_z, pbuf, 4);
				swapEndian(&packet->data.play_server.useentity.target_z, 4);
				ADX(4)
			}
			//hand
			CPS_OPT(1)
				rx = readVarInt(&packet->data.play_server.useentity.hand, pbuf, ps);
				ADRX
			}
		} else if (id == PKT_PLAY_SERVER_KEEPALIVE) {
			//keep_alive_id
			rx = readVarInt(&packet->data.play_server.keepalive.keep_alive_id, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_PLAYERPOSITION) {
			//x
			CPS(8)
			memcpy(&packet->data.play_server.playerposition.x, pbuf, 8);
			swapEndian(&packet->data.play_server.playerposition.x, 8);
			ADX(8)
			//feet_y
			CPS(8)
			memcpy(&packet->data.play_server.playerposition.feet_y, pbuf, 8);
			swapEndian(&packet->data.play_server.playerposition.feet_y, 8);
			ADX(8)
			//z
			CPS(8)
			memcpy(&packet->data.play_server.playerposition.z, pbuf, 8);
			swapEndian(&packet->data.play_server.playerposition.z, 8);
			ADX(8)
			//on_ground
			CPS(1)
			memcpy(&packet->data.play_server.playerposition.on_ground, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_PLAYERPOSITIONANDLOOK) {
			//x
			CPS(8)
			memcpy(&packet->data.play_server.playerpositionandlook.x, pbuf, 8);
			swapEndian(&packet->data.play_server.playerpositionandlook.x, 8);
			ADX(8)
			//feet_y
			CPS(8)
			memcpy(&packet->data.play_server.playerpositionandlook.feet_y, pbuf, 8);
			swapEndian(&packet->data.play_server.playerpositionandlook.feet_y, 8);
			ADX(8)
			//z
			CPS(8)
			memcpy(&packet->data.play_server.playerpositionandlook.z, pbuf, 8);
			swapEndian(&packet->data.play_server.playerpositionandlook.z, 8);
			ADX(8)
			//yaw
			CPS(4)
			memcpy(&packet->data.play_server.playerpositionandlook.yaw, pbuf, 4);
			swapEndian(&packet->data.play_server.playerpositionandlook.yaw, 4);
			ADX(4)
			//pitch
			CPS(4)
			memcpy(&packet->data.play_server.playerpositionandlook.pitch, pbuf, 4);
			swapEndian(&packet->data.play_server.playerpositionandlook.pitch, 4);
			ADX(4)
			//on_ground
			CPS(1)
			memcpy(&packet->data.play_server.playerpositionandlook.on_ground, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_PLAYERLOOK) {
			//yaw
			CPS(4)
			memcpy(&packet->data.play_server.playerlook.yaw, pbuf, 4);
			swapEndian(&packet->data.play_server.playerlook.yaw, 4);
			ADX(4)
			//pitch
			CPS(4)
			memcpy(&packet->data.play_server.playerlook.pitch, pbuf, 4);
			swapEndian(&packet->data.play_server.playerlook.pitch, 4);
			ADX(4)
			//on_ground
			CPS(1)
			memcpy(&packet->data.play_server.playerlook.on_ground, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_PLAYER) {
			//on_ground
			CPS(1)
			memcpy(&packet->data.play_server.player.on_ground, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_VEHICLEMOVE) {
			//x
			CPS(8)
			memcpy(&packet->data.play_server.vehiclemove.x, pbuf, 8);
			swapEndian(&packet->data.play_server.vehiclemove.x, 8);
			ADX(8)
			//y
			CPS(8)
			memcpy(&packet->data.play_server.vehiclemove.y, pbuf, 8);
			swapEndian(&packet->data.play_server.vehiclemove.y, 8);
			ADX(8)
			//z
			CPS(8)
			memcpy(&packet->data.play_server.vehiclemove.z, pbuf, 8);
			swapEndian(&packet->data.play_server.vehiclemove.z, 8);
			ADX(8)
			//yaw
			CPS(4)
			memcpy(&packet->data.play_server.vehiclemove.yaw, pbuf, 4);
			swapEndian(&packet->data.play_server.vehiclemove.yaw, 4);
			ADX(4)
			//pitch
			CPS(4)
			memcpy(&packet->data.play_server.vehiclemove.pitch, pbuf, 4);
			swapEndian(&packet->data.play_server.vehiclemove.pitch, 4);
			ADX(4)
		} else if (id == PKT_PLAY_SERVER_STEERBOAT) {
			//right_paddle_turning
			CPS(1)
			memcpy(&packet->data.play_server.steerboat.right_paddle_turning, pbuf, 1);
			ADX(1)
			//left_paddle_turning
			CPS(1)
			memcpy(&packet->data.play_server.steerboat.left_paddle_turning, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_PLAYERABILITIES) {
			//flags
			CPS(1)
			memcpy(&packet->data.play_server.playerabilities.flags, pbuf, 1);
			ADX(1)
			//flying_speed
			CPS(4)
			memcpy(&packet->data.play_server.playerabilities.flying_speed, pbuf, 4);
			swapEndian(&packet->data.play_server.playerabilities.flying_speed, 4);
			ADX(4)
			//walking_speed
			CPS(4)
			memcpy(&packet->data.play_server.playerabilities.walking_speed, pbuf, 4);
			swapEndian(&packet->data.play_server.playerabilities.walking_speed, 4);
			ADX(4)
		} else if (id == PKT_PLAY_SERVER_PLAYERDIGGING) {
			//status
			rx = readVarInt(&packet->data.play_server.playerdigging.status, pbuf, ps);
			ADRX
			//location
			CPS(8)
			memcpy(&packet->data.play_server.playerdigging.location, pbuf, 8);
			swapEndian(&packet->data.play_server.playerdigging.location, 8);
			ADX(8)
			//face
			CPS(1)
			memcpy(&packet->data.play_server.playerdigging.face, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_ENTITYACTION) {
			//entity_id
			rx = readVarInt(&packet->data.play_server.entityaction.entity_id, pbuf, ps);
			ADRX
			//action_id
			rx = readVarInt(&packet->data.play_server.entityaction.action_id, pbuf, ps);
			ADRX
			//jump_boost
			rx = readVarInt(&packet->data.play_server.entityaction.jump_boost, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_STEERVEHICLE) {
			//sideways
			CPS(4)
			memcpy(&packet->data.play_server.steervehicle.sideways, pbuf, 4);
			swapEndian(&packet->data.play_server.steervehicle.sideways, 4);
			ADX(4)
			//forward
			CPS(4)
			memcpy(&packet->data.play_server.steervehicle.forward, pbuf, 4);
			swapEndian(&packet->data.play_server.steervehicle.forward, 4);
			ADX(4)
			//flags
			CPS(1)
			memcpy(&packet->data.play_server.steervehicle.flags, pbuf, 1);
			ADX(1)
		} else if (id == PKT_PLAY_SERVER_RESOURCEPACKSTATUS) {
			//result
			rx = readVarInt(&packet->data.play_server.resourcepackstatus.result, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_HELDITEMCHANGE) {
			//slot
			CPS(2)
			memcpy(&packet->data.play_server.helditemchange.slot, pbuf, 2);
			swapEndian(&packet->data.play_server.helditemchange.slot, 2);
			ADX(2)
		} else if (id == PKT_PLAY_SERVER_CREATIVEINVENTORYACTION) {
			//slot
			CPS(2)
			memcpy(&packet->data.play_server.creativeinventoryaction.slot, pbuf, 2);
			swapEndian(&packet->data.play_server.creativeinventoryaction.slot, 2);
			ADX(2)
			//clicked_item
			rx = readSlot(packet->pool, &packet->data.play_server.creativeinventoryaction.clicked_item, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_UPDATESIGN) {
			//location
			CPS(8)
			memcpy(&packet->data.play_server.updatesign.location, pbuf, 8);
			swapEndian(&packet->data.play_server.updatesign.location, 8);
			ADX(8)
			//line_1
			rx = readString(packet->pool, &packet->data.play_server.updatesign.line_1, pbuf, ps);
			ADRX
			//line_2
			rx = readString(packet->pool, &packet->data.play_server.updatesign.line_2, pbuf, ps);
			ADRX
			//line_3
			rx = readString(packet->pool, &packet->data.play_server.updatesign.line_3, pbuf, ps);
			ADRX
			//line_4
			rx = readString(packet->pool, &packet->data.play_server.updatesign.line_4, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_ANIMATION) {
			//hand
			rx = readVarInt(&packet->data.play_server.animation.hand, pbuf, ps);
			ADRX
		} else if (id == PKT_PLAY_SERVER_SPECTATE) {
			//target_player
			CPS(16)
			memcpy(&packet->data.play_server.spectate.target_player, pbuf, 16);
			swapEndian(&packet->data.play_server.spectate.target_player, 16);
			ADX(16)
		} else if (id == PKT_PLAY_SERVER_PLAYERBLOCKPLACEMENT) {
			//location
			CPS(8)
			memcpy(&packet->data.play_server.playerblockplacement.location, pbuf, 8);
			swapEndian(&packet->data.play_server.playerblockplacement.location, 8);
			ADX(8)
			//face
			rx = readVarInt(&packet->data.play_server.playerblockplacement.face, pbuf, ps);
			ADRX
			//hand
			rx = readVarInt(&packet->data.play_server.playerblockplacement.hand, pbuf, ps);
			ADRX
			//cursor_position_x
			if (conn->protocol_version > 210) {
				CPS(4)
				memcpy(&packet->data.play_server.playerblockplacement.cursor_position_x, pbuf, 4);
				swapEndian(&packet->data.play_server.playerblockplacement.cursor_position_x, 4);
				ADX(4)
			} else {
				CPS(1)
				uint8_t tmp = (uint8_t)(packet->data.play_server.playerblockplacement.cursor_position_x * 15.);
				memcpy(&tmp, pbuf, 1);
				ADX(1)
			}
			//cursor_position_y
			if (conn->protocol_version > 210) {
				CPS(4)
				memcpy(&packet->data.play_server.playerblockplacement.cursor_position_y, pbuf, 4);
				swapEndian(&packet->data.play_server.playerblockplacement.cursor_position_y, 4);
				ADX(4)
			} else {
				CPS(1)
				uint8_t tmp = (uint8_t)(packet->data.play_server.playerblockplacement.cursor_position_y * 15.);
				memcpy(&tmp, pbuf, 1);
				ADX(1)
			}
			//cursor_position_z
			if (conn->protocol_version > 210) {
				CPS(4)
				memcpy(&packet->data.play_server.playerblockplacement.cursor_position_z, pbuf, 4);
				swapEndian(&packet->data.play_server.playerblockplacement.cursor_position_z, 4);
				ADX(4)
			} else {
				CPS(1)
				uint8_t tmp = (uint8_t)(packet->data.play_server.playerblockplacement.cursor_position_z * 15.);
				memcpy(&tmp, pbuf, 1);
				ADX(1)
			}
		} else if (id == PKT_PLAY_SERVER_USEITEM) {
			//hand
			rx = readVarInt(&packet->data.play_server.useitem.hand, pbuf, ps);
			ADRX
		}
	} else if (conn->protocol_state == STATE_STATUS) {
		if (id == PKT_STATUS_SERVER_REQUEST) {

		} else if (id == PKT_STATUS_SERVER_PING) {
			//payload
			CPS(8)
			memcpy(&packet->data.status_server.ping.payload, pbuf, 8);
			swapEndian(&packet->data.status_server.ping.payload, 8);
			ADX(8)
		}
	} else if (conn->protocol_state == STATE_LOGIN) {
		if (id == PKT_LOGIN_SERVER_LOGINSTART) {
			//name
			rx = readString(packet->pool, &packet->data.login_server.loginstart.name, pbuf, ps);
			ADRX
		} else if (id == PKT_LOGIN_SERVER_ENCRYPTIONRESPONSE) {
			//shared_secret_length
			rx = readVarInt(&packet->data.login_server.encryptionresponse.shared_secret_length, pbuf, ps);
			ADRX
			//shared_secret
			CPS(packet->data.login_server.encryptionresponse.shared_secret_length)
			packet->data.login_server.encryptionresponse.shared_secret = pmalloc(packet->pool,
																				 (size_t) packet->data.login_server.encryptionresponse.shared_secret_length);
			memcpy(packet->data.login_server.encryptionresponse.shared_secret, pbuf,
				   (size_t) packet->data.login_server.encryptionresponse.shared_secret_length);
			ADX(packet->data.login_server.encryptionresponse.shared_secret_length)
			//verify_token_length
			rx = readVarInt((int32_t*) &packet->data.login_server.encryptionresponse.verify_token_length, pbuf, ps);
			ADRX
			//verify_token
			CPS(packet->data.login_server.encryptionresponse.verify_token_length)
			packet->data.login_server.encryptionresponse.verify_token = pmalloc(packet->pool, packet->data.login_server.encryptionresponse.verify_token_length);
			memcpy(packet->data.login_server.encryptionresponse.verify_token, pbuf, packet->data.login_server.encryptionresponse.verify_token_length);
			ADX(packet->data.login_server.encryptionresponse.verify_token_length)
		}
	}
	goto rx;
	rer: ;
	pfree(packet->pool);
	return -1;
	rx: ;
	pfree(packet->pool);
	return buflen;
}

ssize_t packet_write(struct connection* conn, struct packet* packet) {
	if (conn->protocol_state == STATE_PLAY && packet->id == PKT_PLAY_CLIENT_CHUNKDATA) {
		if (world_get_chunk(conn->player->world, packet->data.play_client.chunkdata.cx, packet->data.play_client.chunkdata.cz) == NULL) return 0;
	}
	unsigned char* pktbuf = pmalloc(packet->pool, 522);
	pktbuf += 10;
	size_t ps = 512;
	size_t pi = 0;
	size_t slb = 0;
	int32_t id = packet->id;
	ENS(4)
	pi += writeVarInt(id, pktbuf + pi);
	if (conn->protocol_state == STATE_HANDSHAKE) {

	} else if (conn->protocol_state == STATE_PLAY) {
		//printf("write %i\n", id);
		if (id == PKT_PLAY_CLIENT_SPAWNOBJECT) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.spawnobject.entity_id, pktbuf + pi);
			//object_uuid
			ENS(16)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.object_uuid, 16);
			swapEndian(pktbuf + pi, 16);
			pi += 16;
			//type
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.type, 1);
			pi += 1;
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//pitch
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.pitch, 1);
			pi += 1;
			//yaw
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.yaw, 1);
			pi += 1;
			//data
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.data, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//velocity_x
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.velocity_x, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//velocity_y
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.velocity_y, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//velocity_z
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnobject.velocity_z, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
		} else if (id == PKT_PLAY_CLIENT_SPAWNEXPERIENCEORB) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.spawnexperienceorb.entity_id, pktbuf + pi);
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnexperienceorb.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnexperienceorb.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnexperienceorb.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//count
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnexperienceorb.count, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
		} else if (id == PKT_PLAY_CLIENT_SPAWNGLOBALENTITY) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.spawnglobalentity.entity_id, pktbuf + pi);
			//type
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnglobalentity.type, 1);
			pi += 1;
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnglobalentity.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnglobalentity.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnglobalentity.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
		} else if (id == PKT_PLAY_CLIENT_SPAWNMOB) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.spawnmob.entity_id, pktbuf + pi);
			//entity_uuid
			ENS(16)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.entity_uuid, 16);
			swapEndian(pktbuf + pi, 16);
			pi += 16;
			//type
			if (conn->protocol_version > 210) {
				ENS(4)
				pi += writeVarInt(packet->data.play_client.spawnmob.type, pktbuf + pi);
			} else {
				ENS(1)
				memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.type, 1);
				pi += 1;
			}
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//yaw
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.yaw, 1);
			pi += 1;
			//pitch
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.pitch, 1);
			pi += 1;
			//head_pitch
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.head_pitch, 1);
			pi += 1;
			//velocity_x
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.velocity_x, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//velocity_y
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.velocity_y, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//velocity_z
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnmob.velocity_z, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//metadata
			ENS(packet->data.play_client.spawnmob.metadata.metadata_size)
			memcpy(pktbuf + pi, packet->data.play_client.spawnmob.metadata.metadata, packet->data.play_client.spawnmob.metadata.metadata_size);
			pi += packet->data.play_client.spawnmob.metadata.metadata_size;
		} else if (id == PKT_PLAY_CLIENT_SPAWNPAINTING) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.spawnpainting.entity_id, pktbuf + pi);
			//entity_uuid
			ENS(16)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnpainting.entity_uuid, 16);
			swapEndian(pktbuf + pi, 16);
			pi += 16;
			//title
			slb = strlen(packet->data.play_client.spawnpainting.title) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.spawnpainting.title, pktbuf + pi, ps - pi);
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnpainting.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//direction
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnpainting.direction, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_SPAWNPLAYER) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.spawnplayer.entity_id, pktbuf + pi);
			//player_uuid
			ENS(16)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnplayer.player_uuid, 16);
			swapEndian(pktbuf + pi, 16);
			pi += 16;
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnplayer.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnplayer.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnplayer.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//yaw
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnplayer.yaw, 1);
			pi += 1;
			//pitch
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnplayer.pitch, 1);
			pi += 1;
			//metadata
			ENS(packet->data.play_client.spawnplayer.metadata.metadata_size)
			memcpy(pktbuf + pi, packet->data.play_client.spawnplayer.metadata.metadata, packet->data.play_client.spawnplayer.metadata.metadata_size);
			pi += packet->data.play_client.spawnplayer.metadata.metadata_size;
		} else if (id == PKT_PLAY_CLIENT_ANIMATION) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.animation.entity_id, pktbuf + pi);
			//animation
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.animation.animation, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_STATISTICS) {
			//TODO: Manual Implementation
		} else if (id == PKT_PLAY_CLIENT_BLOCKBREAKANIMATION) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.blockbreakanimation.entity_id, pktbuf + pi);
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.blockbreakanimation.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//destroy_stage
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.blockbreakanimation.destroy_stage, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_UPDATEBLOCKENTITY) {
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.updateblockentity.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//action
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.updateblockentity.action, 1);
			pi += 1;
			//nbt_data
			ENS(512)
			pi += nbt_write(packet->data.play_client.updateblockentity.nbt_data, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_BLOCKACTION) {
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.blockaction.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//action_id_byte_1
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.blockaction.action_id, 1);
			pi += 1;
			//action_param_byte_2
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.blockaction.action_param, 1);
			pi += 1;
			//block_type
			ENS(4)
			pi += writeVarInt(packet->data.play_client.blockaction.block_type, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_BLOCKCHANGE) {
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.blockchange.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//block_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.blockchange.block_id, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_BOSSBAR) {
			//TODO: Manual Implementation
		} else if (id == PKT_PLAY_CLIENT_SERVERDIFFICULTY) {
			//difficulty
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.serverdifficulty.difficulty, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_TABCOMPLETE) {
			//count
			ENS(4)
			pi += writeVarInt(packet->data.play_client.tabcomplete.count, pktbuf + pi);
			//matches
			for (int32_t i = 0; i < packet->data.play_client.tabcomplete.count; i++) {
				slb = strlen(packet->data.play_client.tabcomplete.matches[i]) + 4;
				ENS(slb)
				pi += writeString(packet->data.play_client.tabcomplete.matches[i], pktbuf + pi, ps - pi);
			}
		} else if (id == PKT_PLAY_CLIENT_CHATMESSAGE) {
			//json_data
			slb = strlen(packet->data.play_client.chatmessage.json_data) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.chatmessage.json_data, pktbuf + pi, ps - pi);
			//position
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.chatmessage.position, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_MULTIBLOCKCHANGE) {
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.multiblockchange.chunk_x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.multiblockchange.chunk_z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			ENS(4)
			pi += writeVarInt(packet->data.play_client.multiblockchange.record_count, pktbuf + pi);
			for (int32_t i = 0; i < packet->data.play_client.multiblockchange.record_count; i++) {
				ENS(2)
				memcpy(pktbuf + pi, ((uint8_t*) &packet->data.play_client.multiblockchange.records[i]), 2);
				swapEndian(pktbuf + pi, 2);
				pi += 2;
				ENS(4)
				pi += writeVarInt(packet->data.play_client.multiblockchange.records[i].block_id, pktbuf + pi);
			}
		} else if (id == PKT_PLAY_CLIENT_CONFIRMTRANSACTION) {
			//window_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.confirmtransaction.window_id, 1);
			pi += 1;
			//action_number
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.confirmtransaction.action_number, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//accepted
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.confirmtransaction.accepted, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_CLOSEWINDOW) {
			//window_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.closewindow.window_id, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_OPENWINDOW) {
			//window_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.openwindow.window_id, 1);
			pi += 1;
			//window_type
			slb = strlen(packet->data.play_client.openwindow.window_type) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.openwindow.window_type, pktbuf + pi, ps - pi);
			//window_title
			slb = strlen(packet->data.play_client.openwindow.window_title) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.openwindow.window_title, pktbuf + pi, ps - pi);
			//number_of_slots
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.openwindow.number_of_slots, 1);
			pi += 1;
			//entity_id
			if (str_eq(packet->data.play_client.openwindow.window_type, "EntityHorse")) {
				ENS(4)
				memcpy(pktbuf + pi, &packet->data.play_client.openwindow.entity_id, 4);
				swapEndian(pktbuf + pi, 4);
				pi += 4;
			}
		} else if (id == PKT_PLAY_CLIENT_WINDOWITEMS) {
			//window_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.windowitems.window_id, 1);
			pi += 1;
			//count
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.windowitems.count, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//slot_data
			for (int16_t i = 0; i < packet->data.play_client.windowitems.count; i++) {
				ENS(512)
				pi += writeSlot(&packet->data.play_client.windowitems.slot_data[i], pktbuf + pi, ps - pi);
			}
		} else if (id == PKT_PLAY_CLIENT_WINDOWPROPERTY) {
			//window_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.windowproperty.window_id, 1);
			pi += 1;
			//property
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.windowproperty.property, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//value
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.windowproperty.value, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
		} else if (id == PKT_PLAY_CLIENT_SETSLOT) {
			//window_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.setslot.window_id, 1);
			pi += 1;
			//slot
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.setslot.slot, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//slot_data
			ENS(512)
			pi += writeSlot(&packet->data.play_client.setslot.slot_data, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_SETCOOLDOWN) {
			//item_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.setcooldown.item_id, pktbuf + pi);
			//cooldown_ticks
			ENS(4)
			pi += writeVarInt(packet->data.play_client.setcooldown.cooldown_ticks, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_PLUGINMESSAGE) {
			//channel
			slb = strlen(packet->data.play_client.pluginmessage.channel) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.pluginmessage.channel, pktbuf + pi, ps - pi);
			//data
			ENS(packet->data.play_client.pluginmessage.data_size)
			memcpy(pktbuf + pi, packet->data.play_client.pluginmessage.data, packet->data.play_client.pluginmessage.data_size);
			pi += packet->data.play_client.pluginmessage.data_size;
		} else if (id == PKT_PLAY_CLIENT_NAMEDSOUNDEFFECT) {
			//sound_name
			slb = strlen(packet->data.play_client.namedsoundeffect.sound_name) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.namedsoundeffect.sound_name, pktbuf + pi, ps - pi);
			//sound_category
			ENS(4)
			pi += writeVarInt(packet->data.play_client.namedsoundeffect.sound_category, pktbuf + pi);
			//effect_position_x
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.namedsoundeffect.effect_position_x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//effect_position_y
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.namedsoundeffect.effect_position_y, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//effect_position_z
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.namedsoundeffect.effect_position_z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//volume
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.namedsoundeffect.volume, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//pitch
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.namedsoundeffect.pitch, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_DISCONNECT) {
			//reason
			slb = strlen(packet->data.play_client.disconnect.reason) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.disconnect.reason, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_ENTITYSTATUS) {
			//entity_id
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.entitystatus.entity_id, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//entity_status
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entitystatus.entity_status, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_EXPLOSION) {
			//x
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//y
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.y, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//z
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//radius
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.radius, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//record_count
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.record_count, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//records
			ENS(packet->data.play_client.explosion.record_count * 3)
			memcpy(pktbuf + pi, packet->data.play_client.explosion.records,
				   (size_t) (packet->data.play_client.explosion.record_count * 3));
			pi += packet->data.play_client.explosion.record_count * 3;
			//player_motion_x
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.player_motion_x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//player_motion_y
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.player_motion_y, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//player_motion_z
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.explosion.player_motion_z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_UNLOADCHUNK) {
			//chunk_x
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.unloadchunk.chunk_x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//chunk_z
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.unloadchunk.chunk_z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//packet->data.play_client.unloadchunk.ch->playersLoaded--;
		} else if (id == PKT_PLAY_CLIENT_CHANGEGAMESTATE) {
			//reason
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.changegamestate.reason, 1);
			pi += 1;
			//value
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.changegamestate.value, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_KEEPALIVE) {
			//keep_alive_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.keepalive.keep_alive_id, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_CHUNKDATA) {
			//chunk_x
			ENS(4)
			int32_t cx = packet->data.play_client.chunkdata.data->x;
			int32_t cz = packet->data.play_client.chunkdata.data->z;
			memcpy(pktbuf + pi, &cx, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//chunk_z
			ENS(4)
			memcpy(pktbuf + pi, &cz, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//ground_up_continuous
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.chunkdata.ground_up_continuous, 1);
			pi += 1;
			//primary_bit_mask
			uint16_t primary_bit_mask = 0;
			int32_t ccs = 0;
			for (int ymj = 0; ymj < 16; ymj++) {
				if (packet->data.play_client.chunkdata.data->sections[ymj] == NULL) continue;
				primary_bit_mask |= (uint16_t) pow(2, ymj);
				ccs += packet->data.play_client.chunkdata.data->sections[ymj]->bpb * 512 + 4 + 2048;
				if (packet->data.play_client.chunkdata.data->sections[ymj]->skyLight != NULL) ccs += 2048;
				for (int i = 0; i < packet->data.play_client.chunkdata.data->sections[ymj]->palette_count; i++) {
					ccs += getVarIntSize(packet->data.play_client.chunkdata.data->sections[ymj]->palette[i]);
				}
			}
			ENS(4)
			pi += writeVarInt(primary_bit_mask, pktbuf + pi);
			//size
			ENS(4)
			pi += writeVarInt(ccs + (packet->data.play_client.chunkdata.ground_up_continuous ? 256 : 0), pktbuf + pi);
			//data
			for (int ymj = 0; ymj < 16; ymj++) {
				if (packet->data.play_client.chunkdata.data->sections[ymj] == NULL) continue;
				uint8_t bpb = packet->data.play_client.chunkdata.data->sections[ymj]->bpb;
				ENS(1)
				memcpy(pktbuf + pi, &bpb, 1);
				pi += 1;
				ENS(4)
				pi += writeVarInt(
					(int32_t) (bpb < 9 ? packet->data.play_client.chunkdata.data->sections[ymj]->palette_count : 0), pktbuf + pi);
				for (int i = 0; i < packet->data.play_client.chunkdata.data->sections[ymj]->palette_count; i++) {
					ENS(4)
					pi += writeVarInt(packet->data.play_client.chunkdata.data->sections[ymj]->palette[i], pktbuf + pi);
				}
				ENS(4)
				pi += writeVarInt(bpb * 64, pktbuf + pi);
				ENS(bpb * 512)
				memcpy(pktbuf + pi, packet->data.play_client.chunkdata.data->sections[ymj]->blocks, bpb * 512);
				int64_t* es = (int64_t*) (pktbuf + pi);
				for (int i = 0; i < bpb * 64; i++) {
					swapEndian(&es[i], 8);
				}
				pi += bpb * 512;
				ENS(2048);
				memcpy(pktbuf + pi, packet->data.play_client.chunkdata.data->sections[ymj]->blockLight, 2048);
				//memset(pktbuf + pi, 0xFF, 2048);
				pi += 2048;
				if (packet->data.play_client.chunkdata.data->sections[ymj]->skyLight != NULL) {
					ENS(2048);
					memcpy(pktbuf + pi, packet->data.play_client.chunkdata.data->sections[ymj]->skyLight, 2048);
					//memset(pktbuf + pi, 0xFF, 2048);
					pi += 2048;
				}
			}
			//biomes
			if (packet->data.play_client.chunkdata.ground_up_continuous) {
				ENS(256)
				memcpy(pktbuf + pi, packet->data.play_client.chunkdata.data->biomes, 256);
				pi += 256;
			}
			//number_of_block_entities
			ENS(4)
			pi += writeVarInt(packet->data.play_client.chunkdata.number_of_block_entities, pktbuf + pi);
			//block_entities
			for (int32_t i = 0; i < packet->data.play_client.chunkdata.number_of_block_entities; i++) {
				ENS(512)
				pi += nbt_write(packet->data.play_client.chunkdata.block_entities[i], pktbuf + pi, ps - pi);
			}
		} else if (id == PKT_PLAY_CLIENT_EFFECT) {
			//effect_id
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.effect.effect_id, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.effect.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//data
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.effect.data, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//disable_relative_volume
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.effect.disable_relative_volume, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_PARTICLE) {
			//particle_id
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.particle_id, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//long_distance
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.long_distance, 1);
			pi += 1;
			//x
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//y
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.y, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//z
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//offset_x
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.offset_x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//offset_y
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.offset_y, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//offset_z
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.offset_z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//particle_data
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.particle_data, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//particle_count
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.particle.particle_count, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//data
			/* TODO: Array
			 ENS(4)
			 pi += writeVarInt(packet->data.play_client.particle.data, pktbuf + pi);
			 */
		} else if (id == PKT_PLAY_CLIENT_JOINGAME) {
			//entity_id
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.joingame.entity_id, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//gamemode
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.joingame.gamemode, 1);
			pi += 1;
			//dimension
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.joingame.dimension, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//difficulty
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.joingame.difficulty, 1);
			pi += 1;
			//max_players
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.joingame.max_players, 1);
			pi += 1;
			//level_type
			slb = strlen(packet->data.play_client.joingame.level_type) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.joingame.level_type, pktbuf + pi, ps - pi);
			//reduced_debug_info
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.joingame.reduced_debug_info, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_MAP) {
			//TODO: Manual Implementation
		} else if (id == PKT_PLAY_CLIENT_ENTITYRELATIVEMOVE) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityrelativemove.entity_id, pktbuf + pi);
			//delta_x
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entityrelativemove.delta_x, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//delta_y
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entityrelativemove.delta_y, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//delta_z
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entityrelativemove.delta_z, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//on_ground
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityrelativemove.on_ground, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_ENTITYLOOKANDRELATIVEMOVE) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entitylookandrelativemove.entity_id, pktbuf + pi);
			//delta_x
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylookandrelativemove.delta_x, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//delta_y
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylookandrelativemove.delta_y, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//delta_z
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylookandrelativemove.delta_z, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//yaw
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylookandrelativemove.yaw, 1);
			pi += 1;
			//pitch
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylookandrelativemove.pitch, 1);
			pi += 1;
			//on_ground
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylookandrelativemove.on_ground, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_ENTITYLOOK) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entitylook.entity_id, pktbuf + pi);
			//yaw
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylook.yaw, 1);
			pi += 1;
			//pitch
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylook.pitch, 1);
			pi += 1;
			//on_ground
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entitylook.on_ground, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_ENTITY) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entity.entity_id, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_VEHICLEMOVE) {
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//yaw
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.yaw, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//pitch
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.vehiclemove.pitch, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_OPENSIGNEDITOR) {
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.opensigneditor.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
		} else if (id == PKT_PLAY_CLIENT_PLAYERABILITIES) {
			//flags
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.playerabilities.flags, 1);
			pi += 1;
			//flying_speed
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.playerabilities.flying_speed, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//field_of_view_modifier
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.playerabilities.field_of_view_modifier, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_COMBATEVENT) {
			//TODO: Manual Implementation
		} else if (id == PKT_PLAY_CLIENT_PLAYERLISTITEM) {
			//action_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.playerlistitem.action_id, pktbuf + pi);
			//number_of_players
			ENS(4)
			pi += writeVarInt(packet->data.play_client.playerlistitem.number_of_players, pktbuf + pi);
			for (int32_t i = 0; i < packet->data.play_client.playerlistitem.number_of_players; i++) {
				ENS(16)
				memcpy(pktbuf + pi, &packet->data.play_client.playerlistitem.players[i].uuid, 16);
				swapEndian(pktbuf + pi, 16);
				pi += 16;
				if (packet->data.play_client.playerlistitem.action_id == 0) {
					slb = strlen(packet->data.play_client.playerlistitem.players[i].action.addplayer.name) + 4;
					ENS(slb)
					pi += writeString(packet->data.play_client.playerlistitem.players[i].action.addplayer.name, pktbuf + pi, ps - pi);
					ENS(4)
					pi += writeVarInt(packet->data.play_client.playerlistitem.players[i].action.addplayer.number_of_properties, pktbuf + pi);
					for (int32_t x = 0; x < packet->data.play_client.playerlistitem.players[i].action.addplayer.number_of_properties; x++) {
						slb = strlen(packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].name) + 4;
						ENS(slb)
						pi += writeString(packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].name, pktbuf + pi, ps - pi);
						slb = strlen(packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].value) + 4;
						ENS(slb)
						pi += writeString(packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].value, pktbuf + pi, ps - pi);
						ENS(1)
						memcpy(pktbuf + pi, &packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].isSigned, 1);
						pi += 1;
						if (packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].isSigned) {
							slb = strlen(packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].signature) + 4;
							ENS(slb)
							pi += writeString(packet->data.play_client.playerlistitem.players[i].action.addplayer.properties[x].signature, pktbuf + pi, ps - pi);
						}
					}
					ENS(4)
					pi += writeVarInt(packet->data.play_client.playerlistitem.players[i].action.addplayer.gamemode, pktbuf + pi);
					ENS(4)
					pi += writeVarInt(packet->data.play_client.playerlistitem.players[i].action.addplayer.ping, pktbuf + pi);
					ENS(1)
					memcpy(pktbuf + pi, &packet->data.play_client.playerlistitem.players[i].action.addplayer.has_display_name, 1);
					pi += 1;
					if (packet->data.play_client.playerlistitem.players[i].action.addplayer.has_display_name) {
						slb = strlen(packet->data.play_client.playerlistitem.players[i].action.addplayer.display_name) + 4;
						ENS(slb)
						pi += writeString(packet->data.play_client.playerlistitem.players[i].action.addplayer.display_name, pktbuf + pi, ps - pi);
					}
				} else if (packet->data.play_client.playerlistitem.action_id == 1) {
					ENS(4)
					pi += writeVarInt(packet->data.play_client.playerlistitem.players[i].action.updategamemode_gamemode, pktbuf + pi);
				} else if (packet->data.play_client.playerlistitem.action_id == 2) {
					ENS(4)
					pi += writeVarInt(packet->data.play_client.playerlistitem.players[i].action.updatelatency_ping, pktbuf + pi);
				} else if (packet->data.play_client.playerlistitem.action_id == 3) {
					ENS(1)
					memcpy(pktbuf + pi, &packet->data.play_client.playerlistitem.players[i].action.updatedisplayname.has_display_name, 1);
					pi += 1;
					if (packet->data.play_client.playerlistitem.players[i].action.updatedisplayname.has_display_name) {
						slb = strlen(packet->data.play_client.playerlistitem.players[i].action.updatedisplayname.display_name) + 4;
						ENS(slb)
						pi += writeString(packet->data.play_client.playerlistitem.players[i].action.updatedisplayname.display_name, pktbuf + pi, ps - pi);
					}
				} else if (packet->data.play_client.playerlistitem.action_id == 4) {
					//none
				}
			}
		} else if (id == PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK) {
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.playerpositionandlook.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.playerpositionandlook.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.playerpositionandlook.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//yaw
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.playerpositionandlook.yaw, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//pitch
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.playerpositionandlook.pitch, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//flags
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.playerpositionandlook.flags, 1);
			pi += 1;
			//teleport_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.playerpositionandlook.teleport_id, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_USEBED) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.usebed.entity_id, pktbuf + pi);
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.usebed.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
		} else if (id == PKT_PLAY_CLIENT_DESTROYENTITIES) {
			//count
			ENS(4)
			pi += writeVarInt(packet->data.play_client.destroyentities.count, pktbuf + pi);
			//entity_ids
			ENS(4 * packet->data.play_client.destroyentities.count)
			for (size_t i = 0; i < packet->data.play_client.destroyentities.count; i++)
				pi += writeVarInt(packet->data.play_client.destroyentities.entity_ids[i], pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_REMOVEENTITYEFFECT) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.removeentityeffect.entity_id, pktbuf + pi);
			//effect_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.removeentityeffect.effect_id, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_RESOURCEPACKSEND) {
			//url
			slb = strlen(packet->data.play_client.resourcepacksend.url) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.resourcepacksend.url, pktbuf + pi, ps - pi);
			//hash
			slb = strlen(packet->data.play_client.resourcepacksend.hash) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.resourcepacksend.hash, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_RESPAWN) {
			//dimension
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.respawn.dimension, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//difficulty
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.respawn.difficulty, 1);
			pi += 1;
			//gamemode
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.respawn.gamemode, 1);
			pi += 1;
			//level_type
			slb = strlen(packet->data.play_client.respawn.level_type) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.respawn.level_type, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_ENTITYHEADLOOK) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityheadlook.entity_id, pktbuf + pi);
			//head_yaw
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityheadlook.head_yaw, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_WORLDBORDER) {
			//TODO: Manual Implementation
		} else if (id == PKT_PLAY_CLIENT_CAMERA) {
			//camera_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.camera.camera_id, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_HELDITEMCHANGE) {
			//slot
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.helditemchange.slot, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_DISPLAYSCOREBOARD) {
			//position
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.displayscoreboard.position, 1);
			pi += 1;
			//score_name
			slb = strlen(packet->data.play_client.displayscoreboard.score_name) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.displayscoreboard.score_name, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_ENTITYMETADATA) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entitymetadata.entity_id, pktbuf + pi);
			//metadata
			ENS(packet->data.play_client.entitymetadata.metadata.metadata_size)
			memcpy(pktbuf + pi, packet->data.play_client.entitymetadata.metadata.metadata, packet->data.play_client.entitymetadata.metadata.metadata_size);
			pi += packet->data.play_client.entitymetadata.metadata.metadata_size;
		} else if (id == PKT_PLAY_CLIENT_ATTACHENTITY) {
			//attached_entity_id
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.attachentity.attached_entity_id, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//holding_entity_id
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.attachentity.holding_entity_id, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_ENTITYVELOCITY) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityvelocity.entity_id, pktbuf + pi);
			//velocity_x
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entityvelocity.velocity_x, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//velocity_y
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entityvelocity.velocity_y, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
			//velocity_z
			ENS(2)
			memcpy(pktbuf + pi, &packet->data.play_client.entityvelocity.velocity_z, 2);
			swapEndian(pktbuf + pi, 2);
			pi += 2;
		} else if (id == PKT_PLAY_CLIENT_ENTITYEQUIPMENT) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityequipment.entity_id, pktbuf + pi);
			//slot
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityequipment.slot, pktbuf + pi);
			//item
			ENS(512)
			pi += writeSlot(&packet->data.play_client.entityequipment.item, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_SETEXPERIENCE) {
			//experience_bar
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.setexperience.experience_bar, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//level
			ENS(4)
			pi += writeVarInt(packet->data.play_client.setexperience.level, pktbuf + pi);
			//total_experience
			ENS(4)
			pi += writeVarInt(packet->data.play_client.setexperience.total_experience, pktbuf + pi);
		} else if (id == PKT_PLAY_CLIENT_UPDATEHEALTH) {
			//health
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.updatehealth.health, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//food
			ENS(4)
			pi += writeVarInt(packet->data.play_client.updatehealth.food, pktbuf + pi);
			//food_saturation
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.updatehealth.food_saturation, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_SCOREBOARDOBJECTIVE) {
			//objective_name
			slb = strlen(packet->data.play_client.scoreboardobjective.objective_name) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.scoreboardobjective.objective_name, pktbuf + pi, ps - pi);
			//mode
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.scoreboardobjective.mode, 1);
			pi += 1;
			//objective_value
			/* TODO: Optional
			 slb = strlen(packet->data.play_client.scoreboardobjective.objective_value) + 4;
			 ENS(slb)
			 pi += writeString(packet->data.play_client.scoreboardobjective.objective_value, pktbuf + pi, ps - pi);
			 */
			//type
			/* TODO: Optional
			 slb = strlen(packet->data.play_client.scoreboardobjective.type) + 4;
			 ENS(slb)
			 pi += writeString(packet->data.play_client.scoreboardobjective.type, pktbuf + pi, ps - pi);
			 */
		} else if (id == PKT_PLAY_CLIENT_SETPASSENGERS) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.setpassengers.entity_id, pktbuf + pi);
			//passenger_count
			ENS(4)
			pi += writeVarInt(packet->data.play_client.setpassengers.passenger_count, pktbuf + pi);
			//passengers
			/* TODO: Array
			 ENS(4)
			 pi += writeVarInt(packet->data.play_client.setpassengers.passengers, pktbuf + pi);
			 */
		} else if (id == PKT_PLAY_CLIENT_TEAMS) {
			//TODO: Manual Implementation
		} else if (id == PKT_PLAY_CLIENT_UPDATESCORE) {
			//score_name
			slb = strlen(packet->data.play_client.updatescore.score_name) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.updatescore.score_name, pktbuf + pi, ps - pi);
			//action
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.updatescore.action, 1);
			pi += 1;
			//objective_name
			slb = strlen(packet->data.play_client.updatescore.objective_name) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.updatescore.objective_name, pktbuf + pi, ps - pi);
			//value
			/* TODO: Optional
			 ENS(4)
			 pi += writeVarInt(packet->data.play_client.updatescore.value, pktbuf + pi);
			 */
		} else if (id == PKT_PLAY_CLIENT_SPAWNPOSITION) {
			//location
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.spawnposition.location, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
		} else if (id == PKT_PLAY_CLIENT_TIMEUPDATE) {
			//world_age
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.timeupdate.world_age, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//time_of_day
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.timeupdate.time_of_day, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
		} else if (id == PKT_PLAY_CLIENT_TITLE) {
			//TODO: Manual Implementation
		} else if (id == PKT_PLAY_CLIENT_SOUNDEFFECT) {
			//sound_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.soundeffect.sound_id, pktbuf + pi);
			//sound_category
			ENS(4)
			pi += writeVarInt(packet->data.play_client.soundeffect.sound_category, pktbuf + pi);
			//effect_position_x
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.soundeffect.effect_position_x, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//effect_position_y
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.soundeffect.effect_position_y, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//effect_position_z
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.soundeffect.effect_position_z, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//volume
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.soundeffect.volume, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			//pitch
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.soundeffect.pitch, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
		} else if (id == PKT_PLAY_CLIENT_PLAYERLISTHEADERANDFOOTER) {
			//header
			slb = strlen(packet->data.play_client.playerlistheaderandfooter.header) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.playerlistheaderandfooter.header, pktbuf + pi, ps - pi);
			//footer
			slb = strlen(packet->data.play_client.playerlistheaderandfooter.footer) + 4;
			ENS(slb)
			pi += writeString(packet->data.play_client.playerlistheaderandfooter.footer, pktbuf + pi, ps - pi);
		} else if (id == PKT_PLAY_CLIENT_COLLECTITEM) {
			//collected_entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.collectitem.collected_entity_id, pktbuf + pi);
			//collector_entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.collectitem.collector_entity_id, pktbuf + pi);
			//pickup_item_count
			if (conn->protocol_version > 210) {
				ENS(4)
				pi += writeVarInt(packet->data.play_client.collectitem.pickup_item_count, pktbuf + pi);
			}
		} else if (id == PKT_PLAY_CLIENT_ENTITYTELEPORT) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityteleport.entity_id, pktbuf + pi);
			//x
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.entityteleport.x, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//y
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.entityteleport.y, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//z
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.play_client.entityteleport.z, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
			//yaw
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityteleport.yaw, 1);
			pi += 1;
			//pitch
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityteleport.pitch, 1);
			pi += 1;
			//on_ground
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityteleport.on_ground, 1);
			pi += 1;
		} else if (id == PKT_PLAY_CLIENT_ENTITYPROPERTIES) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityproperties.entity_id, pktbuf + pi);
			ENS(4)
			memcpy(pktbuf + pi, &packet->data.play_client.entityproperties.number_of_properties, 4);
			swapEndian(pktbuf + pi, 4);
			pi += 4;
			for (int32_t i = 0; i < packet->data.play_client.entityproperties.number_of_properties; i++) {
				struct entity_property* property = &packet->data.play_client.entityproperties.properties[i];
				slb = strlen(property->key) + 4;
				ENS(slb)
				pi += writeString(property->key, pktbuf + pi, ps - pi);
				ENS(8)
				memcpy(pktbuf + pi, &property->value, 8);
				swapEndian(pktbuf + pi, 8);
				pi += 8;
				ENS(4)
				pi += writeVarInt(property->number_of_modifiers, pktbuf + pi);
				for (int32_t x = 0; x < property->number_of_modifiers; x++) {
					struct entity_property_modifier* epm = &property->modifiers[x];
					ENS(16)
					memcpy(pktbuf + pi, &epm->uuid, 16);
					swapEndian(pktbuf + pi, 16);
					pi += 16;
					ENS(8)
					memcpy(pktbuf + pi, &epm->amount, 8);
					swapEndian(pktbuf + pi, 8);
					pi += 8;
					ENS(1)
					memcpy(pktbuf + pi, &epm->operation, 1);
					pi += 1;
				}
			}
		} else if (id == PKT_PLAY_CLIENT_ENTITYEFFECT) {
			//entity_id
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityeffect.entity_id, pktbuf + pi);
			//effect_id
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityeffect.effect_id, 1);
			pi += 1;
			//amplifier
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityeffect.amplifier, 1);
			pi += 1;
			//duration
			ENS(4)
			pi += writeVarInt(packet->data.play_client.entityeffect.duration, pktbuf + pi);
			//flags
			ENS(1)
			memcpy(pktbuf + pi, &packet->data.play_client.entityeffect.flags, 1);
			pi += 1;
		}
	} else if (conn->protocol_state == STATE_STATUS) {
		if (id == PKT_STATUS_CLIENT_RESPONSE) {
			//json_response
			slb = strlen(packet->data.status_client.response.json_response) + 4;
			ENS(slb)
			pi += writeString(packet->data.status_client.response.json_response, pktbuf + pi, ps - pi);
		} else if (id == PKT_STATUS_CLIENT_PONG) {
			//payload
			ENS(8)
			memcpy(pktbuf + pi, &packet->data.status_client.pong.payload, 8);
			swapEndian(pktbuf + pi, 8);
			pi += 8;
		}
	} else if (conn->protocol_state == STATE_LOGIN) {
		if (id == PKT_LOGIN_CLIENT_DISCONNECT) {
			//reason
			slb = strlen(packet->data.login_client.disconnect.reason) + 4;
			ENS(slb)
			pi += writeString(packet->data.login_client.disconnect.reason, pktbuf + pi, ps - pi);
		} else if (id == PKT_LOGIN_CLIENT_ENCRYPTIONREQUEST) {
			//server_id
			slb = strlen(packet->data.login_client.encryptionrequest.server_id) + 4;
			ENS(slb)
			pi += writeString(packet->data.login_client.encryptionrequest.server_id, pktbuf + pi, ps - pi);
			//public_key_length
			ENS(4)
			pi += writeVarInt(packet->data.login_client.encryptionrequest.public_key_length, pktbuf + pi);
			//public_key
			ENS(packet->data.login_client.encryptionrequest.public_key_length)
			memcpy(pktbuf + pi, packet->data.login_client.encryptionrequest.public_key,
				   (size_t) packet->data.login_client.encryptionrequest.public_key_length);
			pi += packet->data.login_client.encryptionrequest.public_key_length;
			//verify_token_length
			ENS(4)
			pi += writeVarInt(packet->data.login_client.encryptionrequest.verify_token_length, pktbuf + pi);
			//verify_token
			ENS(packet->data.login_client.encryptionrequest.verify_token_length)
			memcpy(pktbuf + pi, packet->data.login_client.encryptionrequest.verify_token,
				   (size_t) packet->data.login_client.encryptionrequest.verify_token_length);
			pi += packet->data.login_client.encryptionrequest.verify_token_length;
		} else if (id == PKT_LOGIN_CLIENT_LOGINSUCCESS) {
			//uuid
			slb = strlen(packet->data.login_client.loginsuccess.uuid) + 4;
			ENS(slb)
			pi += writeString(packet->data.login_client.loginsuccess.uuid, pktbuf + pi, ps - pi);
			//username
			slb = strlen(packet->data.login_client.loginsuccess.username) + 4;
			ENS(slb)
			pi += writeString(packet->data.login_client.loginsuccess.username, pktbuf + pi, ps - pi);
		} else if (id == PKT_LOGIN_CLIENT_SETCOMPRESSION) {
			//threshold
			ENS(4)
			pi += writeVarInt(packet->data.login_client.setcompression.threshold, pktbuf + pi);
		}
	}
	int fpll = getVarIntSize(pi);
	void* wrt = NULL;
	size_t wrt_s = 0;
	unsigned char prep[10];
	uint8_t preps = 0;
	if (conn->compression_state >= 0 && (pi + fpll > conn->compression_state + 1) && (pi + fpll) <= 2097152) {
		z_stream strm;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		int dr = 0;
		if ((dr = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY)) != Z_OK) { // TODO: configurable level?
			goto rret;
		}
		strm.avail_in = (uInt) pi;
		strm.next_in = pktbuf;
		size_t cc = pi + 32;
		void* cdata = pmalloc(packet->pool, cc + 10);
		size_t ts = 0;
		strm.avail_out = (uInt) (cc - ts);
		strm.next_out = cdata + ts;
		do {
			dr = deflate(&strm, Z_FINISH);
			ts = strm.total_out;
			if (ts >= cc) {
				cc *= 2;
				cdata = prealloc(packet->pool, cdata - 10, cc + 10) + 10;
			}
			if (dr == Z_STREAM_ERROR) {
				goto rret;
			}
			strm.avail_out = (uInt) (cc - ts);
			strm.next_out = cdata + ts;
		} while (strm.avail_out == 0);
		deflateEnd(&strm);
		preps += writeVarInt((int32_t) (ts + getVarIntSize((int32_t) pi)), prep + preps);
		preps += writeVarInt((int32_t) pi, prep + preps);
		wrt = cdata;
		wrt_s = ts;
	} else if (conn->compression_state >= 0) {
		preps += writeVarInt((int32_t) (pi + getVarIntSize(0)), prep + preps);
		preps += writeVarInt(0, prep + preps);
		wrt = pktbuf;
		wrt_s = pi;
	} else {
		preps += writeVarInt((int32_t) pi, prep + preps);
		wrt = pktbuf;
		wrt_s = pi;
	}
	if (preps >= 10) goto rret;
	void* xfer_address = wrt - 10;
	// ^ explode
	wrt -= preps; // preallocated!
	memcpy(wrt, prep, preps);
	wrt_s += preps;
	if (conn->aes_ctx_enc != NULL) {
		size_t encrypted_length = wrt_s + 32; // 16 extra just in case
		void* encrypted = pmalloc(packet->pool, encrypted_length);
		if (EVP_EncryptUpdate(conn->aes_ctx_enc, encrypted, &encrypted_length, wrt, (int) wrt_s) != 1) {
			wrt_s = (size_t) -1;
			goto rret;
		}
		xfer_address = wrt = encrypted;
		wrt_s = encrypted_length;
	}
	pxfer(packet->pool, conn->managed_conn->write_buffer.pool, xfer_address);
	buffer_push_partial(&conn->managed_conn->write_buffer, xfer_address, wrt, wrt_s);
	rret: ;
	return wrt_s;
}
