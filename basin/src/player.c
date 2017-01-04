/*
 * player.c
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#include "network.h"
#include "accept.h"
#include "util.h"
#include "inventory.h"
#include "xstring.h"
#include "queue.h"
#include "packet.h"
#include "collection.h"
#include "tileentity.h"
#include <math.h>
#include "server.h"
#include "profile.h"
#include "basin.h"
#include "anticheat.h"
#include "command.h"
#include "smelting.h"
#include "crafting.h"

struct player* newPlayer(struct entity* entity, char* name, struct uuid uuid, struct conn* conn, uint8_t gamemode) {
	struct player* player = xmalloc(sizeof(struct player));
	entity->data.player.player = player;
	player->entity = entity;
	player->conn = conn;
	player->name = name;
	player->uuid = uuid;
	player->gamemode = gamemode;
	player->currentItem = 0;
	player->ping = 0;
	player->stage = 0;
	player->invulnerable = 0;
	player->mayfly = 0;
	player->instabuild = 0;
	player->walkSpeed = 0;
	player->flySpeed = 0;
	player->maybuild = 0;
	player->flying = 0;
	player->xpseed = 0;
	player->xptotal = 0;
	player->xplevel = 0;
	player->score = 0;
	player->saturation = 0.;
	player->sleeping = 0;
	player->fire = 0;
	//TODO: enderitems
	player->food = 20;
	player->foodexhaustion = 0.;
	player->foodTick = 0;
	player->nextKeepAlive = 0;
	player->inHand = NULL;
	memset(&player->digging_position, 0, sizeof(struct encpos));
	player->digging = -1.;
	player->digspeed = 0.;
	player->inventory = xmalloc(sizeof(struct inventory));
	newInventory(player->inventory, INVTYPE_PLAYERINVENTORY, 0, 46);
	player->openInv = NULL;
	put_hashmap(player->inventory->players, player->entity->id, player);
	player->loadedChunks = new_hashmap(1, 1);
	player->loadedEntities = new_hashmap(1, 1);
	player->incomingPacket = new_queue(0, 1);
	player->outgoingPacket = new_queue(0, 1);
	player->defunct = 0;
	player->lastSwing = tick_counter;
	player->foodTimer = 0;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	player->reachDistance = 6.f;
	player->spawnedIn = 0;
	player->llTick = 0;
	player->triggerRechunk = 0;
	player->chunksSent = 0;
	player->world = NULL;
	return player;
}

void sendEntityMove(struct player* player, struct entity* ent) {
	double md = entity_distsq_block(ent, ent->lx, ent->ly, ent->lz);
	double mp = (ent->yaw - ent->lyaw) * (ent->yaw - ent->lyaw) + (ent->pitch - ent->lpitch) * (ent->pitch - ent->lpitch);
	//printf("mp = %f, md = %f\n", mp, md);
	if ((md > .001 || mp > .01 || ent->type == ENT_PLAYER)) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		int ft = tick_counter % 200 == 0;
		if (!ft && md <= .001 && mp <= .01) {
			pkt->id = PKT_PLAY_CLIENT_ENTITY;
			pkt->data.play_client.entity.entity_id = ent->id;
			add_queue(player->outgoingPacket, pkt);
		} else if (!ft && mp > .01 && md <= .001) {
			pkt->id = PKT_PLAY_CLIENT_ENTITYLOOK;
			pkt->data.play_client.entitylook.entity_id = ent->id;
			pkt->data.play_client.entitylook.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			pkt->data.play_client.entitylook.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
			pkt->data.play_client.entitylook.on_ground = ent->onGround;
			add_queue(player->outgoingPacket, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoingPacket, pkt);
		} else if (!ft && mp <= .01 && md > .001 && md < 64.) {
			pkt->id = PKT_PLAY_CLIENT_ENTITYRELATIVEMOVE;
			pkt->data.play_client.entityrelativemove.entity_id = ent->id;
			pkt->data.play_client.entityrelativemove.delta_x = (int16_t)((ent->x * 32. - ent->lx * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.delta_y = (int16_t)((ent->y * 32. - ent->ly * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.delta_z = (int16_t)((ent->z * 32. - ent->lz * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.on_ground = ent->onGround;
			add_queue(player->outgoingPacket, pkt);
		} else if (!ft && mp > .01 && md > .001 && md < 64.) {
			pkt->id = PKT_PLAY_CLIENT_ENTITYLOOKANDRELATIVEMOVE;
			pkt->data.play_client.entitylookandrelativemove.entity_id = ent->id;
			pkt->data.play_client.entitylookandrelativemove.delta_x = (int16_t)((ent->x * 32. - ent->lx * 32.) * 128.);
			pkt->data.play_client.entitylookandrelativemove.delta_y = (int16_t)((ent->y * 32. - ent->ly * 32.) * 128.);
			pkt->data.play_client.entitylookandrelativemove.delta_z = (int16_t)((ent->z * 32. - ent->lz * 32.) * 128.);
			pkt->data.play_client.entitylookandrelativemove.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			pkt->data.play_client.entitylookandrelativemove.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
			pkt->data.play_client.entitylookandrelativemove.on_ground = ent->onGround;
			add_queue(player->outgoingPacket, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoingPacket, pkt);
		} else {
			pkt->id = PKT_PLAY_CLIENT_ENTITYTELEPORT;
			pkt->data.play_client.entityteleport.entity_id = ent->id;
			pkt->data.play_client.entityteleport.x = ent->x;
			pkt->data.play_client.entityteleport.y = ent->y;
			pkt->data.play_client.entityteleport.z = ent->z;
			pkt->data.play_client.entityteleport.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			pkt->data.play_client.entityteleport.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
			pkt->data.play_client.entityteleport.on_ground = ent->onGround;
			add_queue(player->outgoingPacket, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoingPacket, pkt);
		}
	}
}

void player_receive_packet(struct player* player, struct packet* inp) {
	if (player->entity->health <= 0. && inp->id != PKT_PLAY_SERVER_KEEPALIVE && inp->id != PKT_PLAY_SERVER_CLIENTSTATUS) goto cont;
	if (inp->id == PKT_PLAY_SERVER_KEEPALIVE) {
		if (inp->data.play_server.keepalive.keep_alive_id == player->nextKeepAlive) {
			player->nextKeepAlive = 0;
		}
	} else if (inp->id == PKT_PLAY_SERVER_CHATMESSAGE) {
		char* msg = inp->data.play_server.chatmessage.message;
		if (ac_chat(player, msg)) goto cont;
		if (startsWith(msg, "/")) {
			callCommand(player, msg + 1);
		} else {
			size_t s = strlen(msg) + 512;
			char* rs = xmalloc(s);
			snprintf(rs, s, "{\"text\": \"<%s>: %s\"}", player->name, replace(replace(msg, "\\", "\\\\"), "\"", "\\\""));
			printf("<CHAT> <%s>: %s\n", player->name, msg);
			BEGIN_BROADCAST (players)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_CHATMESSAGE;
			pkt->data.play_client.chatmessage.position = 0;
			pkt->data.play_client.chatmessage.json_data = xstrdup(rs, 0);
			add_queue(bc_player->outgoingPacket, pkt);
			END_BROADCAST (players)
			xfree(rs);
		}
	} else if (inp->id == PKT_PLAY_SERVER_PLAYER) {
		struct pkt_play_server_player pkt = inp->data.play_server.player;
		if (ac_tick(player, pkt.on_ground)) goto cont;
		player->entity->lx = player->entity->x;
		player->entity->ly = player->entity->y;
		player->entity->lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		player->entity->onGround = pkt.on_ground;
		BEGIN_BROADCAST(player->entity->loadingPlayers)
		sendEntityMove(bc_player, player->entity);
		END_BROADCAST(player->entity->loadingPlayers)
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERPOSITION) {
		struct pkt_play_server_playerposition pkt = inp->data.play_server.playerposition;
		if (ac_tick(player, pkt.on_ground)) goto cont;
		if (ac_tickpos(player, pkt.x, pkt.feet_y, pkt.z)) goto cont;

		double lx = player->entity->x;
		double ly = player->entity->y;
		double lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		if (moveEntity(player->entity, pkt.x - lx, pkt.feet_y - ly, pkt.z - lz, .05)) {
			teleportPlayer(player, lx, ly, lz);
		} else {
			player->entity->lx = lx;
			player->entity->ly = ly;
			player->entity->lz = lz;
		}
		player->entity->onGround = pkt.on_ground;
		float dx = player->entity->x - player->entity->lx;
		float dy = player->entity->y - player->entity->ly;
		float dz = player->entity->z - player->entity->lz;
		float d3 = dx * dx + dy * dy + dz * dz;
		if (!player->spawnedIn && d3 > 0.00000001) player->spawnedIn = 1;
		if (d3 > 5. * 5. * 5.) {
			teleportPlayer(player, player->entity->lx, player->entity->ly, player->entity->lz);
			kickPlayer(player, "You attempted to move too fast!");
		} else {
			BEGIN_BROADCAST(player->entity->loadingPlayers)
			sendEntityMove(bc_player, player->entity);
			END_BROADCAST(player->entity->loadingPlayers)
		}
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERLOOK) {
		struct pkt_play_server_playerlook pkt = inp->data.play_server.playerlook;
		if (ac_tick(player, pkt.on_ground)) goto cont;
		if (ac_ticklook(player, pkt.yaw, pkt.pitch)) goto cont;

		player->entity->lx = player->entity->x;
		player->entity->ly = player->entity->y;
		player->entity->lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		player->entity->onGround = pkt.on_ground;
		player->entity->yaw = pkt.yaw;
		player->entity->pitch = pkt.pitch;
		BEGIN_BROADCAST(player->entity->loadingPlayers)
		sendEntityMove(bc_player, player->entity);
		END_BROADCAST(player->entity->loadingPlayers)
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERPOSITIONANDLOOK) {
		struct pkt_play_server_playerpositionandlook pkt = inp->data.play_server.playerpositionandlook;
		if (ac_tick(player, pkt.on_ground)) goto cont;
		if (ac_tickpos(player, pkt.x, pkt.feet_y, pkt.z)) goto cont;
		if (ac_ticklook(player, pkt.yaw, pkt.pitch)) goto cont;
		double lx = player->entity->x;
		double ly = player->entity->y;
		double lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		if (moveEntity(player->entity, pkt.x - lx, pkt.feet_y - ly, pkt.z - lz, .05)) {
			teleportPlayer(player, lx, ly, lz);
		} else {
			player->entity->lx = lx;
			player->entity->ly = ly;
			player->entity->lz = lz;
		}
		player->entity->onGround = pkt.on_ground;
		player->entity->yaw = pkt.yaw;
		player->entity->pitch = pkt.pitch;
		float dx = player->entity->x - player->entity->lx;
		float dy = player->entity->y - player->entity->ly;
		float dz = player->entity->z - player->entity->lz;
		float d3 = dx * dx + dy * dy + dz * dz;
		if (!player->spawnedIn && d3 > 0.00000001) player->spawnedIn = 1;
		if (d3 > 5. * 5. * 5.) {
			teleportPlayer(player, player->entity->lx, player->entity->ly, player->entity->lz);
			printf("Player '%s' attempted to move too fast!\n", player->name);
		} else {
			BEGIN_BROADCAST(player->entity->loadingPlayers)
			sendEntityMove(bc_player, player->entity);
			END_BROADCAST(player->entity->loadingPlayers)
		}
	} else if (inp->id == PKT_PLAY_SERVER_ANIMATION) {
		player->lastSwing = tick_counter;
		BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, CHUNK_VIEW_DISTANCE * 16.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_ANIMATION;
		pkt->data.play_client.animation.entity_id = player->entity->id;
		pkt->data.play_client.animation.animation = inp->data.play_server.animation.hand == 0 ? 0 : 3;
		add_queue(bc_player->outgoingPacket, pkt);
		END_BROADCAST(player->world->players)
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERDIGGING) {
		pthread_mutex_lock(&player->inventory->mut);
		struct slot* ci = getSlot(player, player->inventory, player->currentItem + 36);
		struct item_info* ii = ci == NULL ? NULL : getItemInfo(ci->item);
		if (inp->data.play_server.playerdigging.status == 0) {
			if (entity_dist_block(player->entity, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z) > player->reachDistance) {
				pthread_mutex_unlock(&player->inventory->mut);
				goto cont;
			}
			struct block_info* bi = getBlockInfo(getBlockWorld(player->world, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z));
			if (bi == NULL) {
				pthread_mutex_unlock(&player->inventory->mut);
				goto cont;
			}
			float hard = bi->hardness;
			if (hard > 0. && player->gamemode == 0) {
				player->digging = 0.;
				memcpy(&player->digging_position, &inp->data.play_server.playerdigging.location, sizeof(struct encpos));
				BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, CHUNK_VIEW_DISTANCE * 16.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_BLOCKBREAKANIMATION;
				pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
				memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
				pkt->data.play_client.blockbreakanimation.destroy_stage = 0;
				add_queue(bc_player->outgoingPacket, pkt);
				END_BROADCAST(player->world->players)
			} else if (hard == 0. || player->gamemode == 1) {
				if (player->gamemode == 1) {
					struct slot* slot = getSlot(player, player->inventory, 36 + player->currentItem);
					if (slot != NULL && (slot->item == ITM_SWORDWOOD || slot->item == ITM_SWORDGOLD || slot->item == ITM_SWORDSTONE || slot->item == ITM_SWORDIRON || slot->item == ITM_SWORDDIAMOND)) {
						pthread_mutex_unlock(&player->inventory->mut);
						goto cont;
					}
				}
				block blk = getBlockWorld(player->world, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z);
				int nb = 0;
				if (ii != NULL && ii->onItemBreakBlock != NULL) {
					if ((*ii->onItemBreakBlock)(player->world, player, player->currentItem + 36, ci, player->digging_position.x, player->digging_position.y, player->digging_position.z)) {
						setBlockWorld(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
						nb = 1;
					}
				}
				if (!nb) {
					setBlockWorld(player->world, 0, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z);
					if (player->gamemode != 1) dropBlockDrops(player->world, blk, player, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z);
				}
				player->digging = -1.;
				memset(&player->digging_position, 0, sizeof(struct encpos));
			}
			pthread_mutex_unlock(&player->inventory->mut);
		} else if (inp->data.play_server.playerdigging.status == 1 || inp->data.play_server.playerdigging.status == 2) {
			if (entity_dist_block(player->entity, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z) > player->reachDistance || player->digging <= 0.) {
				block blk = getBlockWorld(player->world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
				setBlockWorld(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
				pthread_mutex_unlock(&player->inventory->mut);
				goto cont;
			}
			if (inp->data.play_server.playerdigging.status == 2) {
				block blk = getBlockWorld(player->world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
				if ((player->digging + player->digspeed) >= 1.) {
					int nb = 0;
					if (ii != NULL && ii->onItemBreakBlock != NULL) {
						if ((*ii->onItemBreakBlock)(player->world, player, player->currentItem + 36, ci, player->digging_position.x, player->digging_position.y, player->digging_position.z)) {
							setBlockWorld(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
							nb = 1;
						}
					}
					if (!nb) {
						setBlockWorld(player->world, 0, player->digging_position.x, player->digging_position.y, player->digging_position.z);
						if (player->gamemode != 1) dropBlockDrops(player->world, blk, player, player->digging_position.x, player->digging_position.y, player->digging_position.z);
					}
				} else {
					setBlockWorld(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
				}
			}
			player->digging = -1.;
			pthread_mutex_unlock(&player->inventory->mut);
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_BLOCKBREAKANIMATION;
			pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
			memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
			memset(&player->digging_position, 0, sizeof(struct encpos));
			pkt->data.play_client.blockbreakanimation.destroy_stage = -1;
			add_queue(bc_player->outgoingPacket, pkt);
			END_BROADCAST(player->world->players)
		} else if (inp->data.play_server.playerdigging.status == 3) {
			if (player->openInv != NULL) {
				pthread_mutex_unlock(&player->inventory->mut);
				goto cont;
			}
			struct slot* ci = getSlot(player, player->inventory, 36 + player->currentItem);
			if (ci != NULL) {
				dropPlayerItem(player, ci);
				setSlot(player, player->inventory, 36 + player->currentItem, NULL, 1, 1);
			}
			pthread_mutex_unlock(&player->inventory->mut);
		} else if (inp->data.play_server.playerdigging.status == 4) {
			if (player->openInv != NULL) {
				pthread_mutex_unlock(&player->inventory->mut);
				goto cont;
			}
			struct slot* ci = getSlot(player, player->inventory, 36 + player->currentItem);
			if (ci != NULL) {
				uint8_t ss = ci->itemCount;
				ci->itemCount = 1;
				dropPlayerItem(player, ci);
				ci->itemCount = ss;
				if (--ci->itemCount == 0) {
					ci = NULL;
				}
				setSlot(player, player->inventory, 36 + player->currentItem, ci, 1, 1);
			}
			pthread_mutex_unlock(&player->inventory->mut);
		} else if (inp->data.play_server.playerdigging.status == 5) {
			//TODO
			pthread_mutex_unlock(&player->inventory->mut);
		} else if (inp->data.play_server.playerdigging.status == 6) {
			swapSlots(player, player->inventory, 45, 36 + player->currentItem, 1);
			pthread_mutex_unlock(&player->inventory->mut);
		} else pthread_mutex_unlock(&player->inventory->mut);
	} else if (inp->id == PKT_PLAY_SERVER_CREATIVEINVENTORYACTION) {
		if (player->gamemode == 1) {
			struct inventory* inv = NULL;
			if (player->openInv == NULL) inv = player->inventory;
			else if (player->openInv != NULL) inv = player->openInv;
			if (inv == NULL) goto cont;
			pthread_mutex_lock(&inv->mut);
			if (inp->data.play_server.creativeinventoryaction.clicked_item.item >= 0 && inp->data.play_server.creativeinventoryaction.slot == -1) {
				dropPlayerItem(player, &inp->data.play_server.creativeinventoryaction.clicked_item);
			} else {
				int16_t sl = inp->data.play_server.creativeinventoryaction.slot;
				setSlot(player, inv, sl, &inp->data.play_server.creativeinventoryaction.clicked_item, 1, 1);
			}
			pthread_mutex_unlock(&inv->mut);
		}
	} else if (inp->id == PKT_PLAY_SERVER_HELDITEMCHANGE) {
		if (inp->data.play_server.helditemchange.slot >= 0 && inp->data.play_server.helditemchange.slot < 9) {
			pthread_mutex_lock(&player->inventory->mut);
			player->currentItem = inp->data.play_server.helditemchange.slot;
			onInventoryUpdate(player, player->inventory, player->currentItem + 36);
			pthread_mutex_unlock(&player->inventory->mut);
		}
	} else if (inp->id == PKT_PLAY_SERVER_ENTITYACTION) {
		if (inp->data.play_server.entityaction.action_id == 0) player->entity->sneaking = 1;
		else if (inp->data.play_server.entityaction.action_id == 1) player->entity->sneaking = 0;
		else if (inp->data.play_server.entityaction.action_id == 3) player->entity->sprinting = 1;
		else if (inp->data.play_server.entityaction.action_id == 4) player->entity->sprinting = 0;
		BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
		pkt->data.play_client.entitymetadata.entity_id = player->entity->id;
		writeMetadata(player->entity, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
		add_queue(bc_player->outgoingPacket, pkt);
		END_BROADCAST(player->world->players)
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERBLOCKPLACEMENT) {
		if (player->openInv != NULL) goto cont;
		if (entity_dist_block(player->entity, inp->data.play_server.playerblockplacement.location.x, inp->data.play_server.playerblockplacement.location.y, inp->data.play_server.playerblockplacement.location.z) > player->reachDistance) goto cont;
		pthread_mutex_lock(&player->inventory->mut);
		struct slot* ci = getSlot(player, player->inventory, 36 + player->currentItem);
		int32_t x = inp->data.play_server.playerblockplacement.location.x;
		int32_t y = inp->data.play_server.playerblockplacement.location.y;
		int32_t z = inp->data.play_server.playerblockplacement.location.z;
		uint8_t face = inp->data.play_server.playerblockplacement.face;
		block b = getBlockWorld(player->world, x, y, z);
		if (ci != NULL) {
			struct item_info* ii = getItemInfo(ci->item);
			if (ii != NULL && ii->onItemInteract != NULL) {
				if ((*ii->onItemInteract)(player->world, player, player->currentItem + 36, ci, x, y, z, face)) goto pbp_cont;
			}
		}
		struct block_info* bi = getBlockInfo(b);
		if (!player->entity->sneaking && bi != NULL && bi->onBlockInteract != NULL) {
			(*bi->onBlockInteract)(player->world, b, x, y, z, player, face, inp->data.play_server.playerblockplacement.cursor_position_x, inp->data.play_server.playerblockplacement.cursor_position_y, inp->data.play_server.playerblockplacement.cursor_position_z);
		} else if (ci != NULL && ci->item < 256) {
			if (bi == NULL || !bi->material->replacable) {
				if (face == 0) y--;
				else if (face == 1) y++;
				else if (face == 2) z--;
				else if (face == 3) z++;
				else if (face == 4) x--;
				else if (face == 5) x++;
			}
			block b2 = getBlockWorld(player->world, x, y, z);
			struct block_info* bi2 = getBlockInfo(b2);
			if (b2 == 0 || bi2 == NULL || bi2->material->replacable) {
				block tbb = (ci->item << 4) | (ci->damage & 0x0f);
				struct block_info* tbi = getBlockInfo(tbb);
				if (tbi == NULL) goto pbp_cont;
				struct boundingbox pbb;
				int bad = 0;
				//int32_t cx = x >> 4;
				//int32_t cz = z >> 4;
				//for (int32_t icx = cx - 1; icx <= cx + 1; icx++)
				//for (int32_t icz = cz - 1; icz <= cz + 1; icz++) {
				//struct chunk* ch = getChunk(player->world, icx, icz);
				//if (ch != NULL) {
				BEGIN_HASHMAP_ITERATION(player->world->entities)
				struct entity* ent = (struct entity*) value;
				if (ent == NULL || entity_distsq_block(ent, x, y, z) > 8. * 8. || !isLiving(ent->type)) continue;
				getEntityCollision(ent, &pbb);
				pbb.minX += .01;
				pbb.minY += .01;
				pbb.minZ += .01;
				pbb.maxX -= .001;
				pbb.maxY -= .001;
				pbb.maxZ -= .001;
				for (size_t i = 0; i < tbi->boundingBox_count; i++) {
					struct boundingbox* bb = &tbi->boundingBoxes[i];
					if (bb == NULL) continue;
					struct boundingbox abb;
					abb.minX = bb->minX + x;
					abb.maxX = bb->maxX + x;
					abb.minY = bb->minY + y;
					abb.maxY = bb->maxY + y;
					abb.minZ = bb->minZ + z;
					abb.maxZ = bb->maxZ + z;
					if (boundingbox_intersects(&abb, &pbb)) {
						bad = 1;
						BREAK_HASHMAP_ITERATION(player->world->entities);
						goto phi;
					}
				}
				END_HASHMAP_ITERATION(player->world->entities)
				phi: ;
				//}
				//}
				if (!bad) {
					if (getBlockInfo(tbb)->onBlockPlaced != NULL) tbb = (*getBlockInfo(tbb)->onBlockPlaced)(player, player->world, tbb, x, y, z, face);
					if (tbi->canBePlaced != NULL && !(*tbi->canBePlaced)(player->world, tbb, x, y, z)) {
						setBlockWorld(player->world, b2, x, y, z);
						goto pbp_cont;
					}
					setBlockWorld(player->world, tbb, x, y, z);
					if (player->gamemode != 1) {
						if (--ci->itemCount <= 0) {
							ci = NULL;
						}
						setSlot(player, player->inventory, 36 + player->currentItem, ci, 1, 1);
					}
				} else {
					setBlockWorld(player->world, b2, x, y, z);
				}
			} else {
				setBlockWorld(player->world, b2, x, y, z);
			}
		}
		pbp_cont: ;
		pthread_mutex_unlock(&player->inventory->mut);
	} else if (inp->id == PKT_PLAY_SERVER_CLICKWINDOW) {
		struct inventory* inv = NULL;
		if (inp->data.play_server.clickwindow.window_id == 0 && player->openInv == NULL) inv = player->inventory;
		else if (player->openInv != NULL && inp->data.play_server.clickwindow.window_id == player->openInv->windowID) inv = player->openInv;
		if (inv == NULL) goto cont;
		int8_t b = inp->data.play_server.clickwindow.button;
		int16_t act = inp->data.play_server.clickwindow.action_number;
		int32_t mode = inp->data.play_server.clickwindow.mode;
		int16_t slot = inp->data.play_server.clickwindow.slot;
		pthread_mutex_lock(&inv->mut);
		int s0ic = inv->type == INVTYPE_PLAYERINVENTORY || inv->type == INVTYPE_WORKBENCH;
		//printf("click mode=%i, b=%i, slot=%i\n", mode, b, slot);
		int force_desync = 0;
		if (mode == 5 && b == 10 && player->gamemode != 1) force_desync = 1;
		if (slot >= 0 || force_desync) {
			struct slot* invs = force_desync ? NULL : getSlot(player, inv, slot);
			if ((mode != 4 && mode != 2 && mode != 5 && mode != 3 && ((inp->data.play_server.clickwindow.clicked_item.item < 0) != (invs == NULL))) || (invs != NULL && inp->data.play_server.clickwindow.clicked_item.item >= 0 && !(itemsStackable(invs, &inp->data.play_server.clickwindow.clicked_item) && invs->itemCount == inp->data.play_server.clickwindow.clicked_item.itemCount)) || force_desync) {
				//printf("desync\n");
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_CONFIRMTRANSACTION;
				pkt->data.play_client.confirmtransaction.window_id = inv->windowID;
				pkt->data.play_client.confirmtransaction.action_number = act;
				pkt->data.play_client.confirmtransaction.accepted = 0;
				add_queue(player->outgoingPacket, pkt);
				if (inv != player->inventory) {
					pkt = xmalloc(sizeof(struct packet));
					pkt->id = PKT_PLAY_CLIENT_WINDOWITEMS;
					pkt->data.play_client.windowitems.window_id = inv->windowID;
					pkt->data.play_client.windowitems.count = inv->slot_count;
					pkt->data.play_client.windowitems.slot_data = xmalloc(sizeof(struct slot) * inv->slot_count);
					for (size_t i = 0; i < inv->slot_count; i++) {
						duplicateSlot(inv->slots[i], &pkt->data.play_client.windowitems.slot_data[i]);
					}
					add_queue(player->outgoingPacket, pkt);
				}
				pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_WINDOWITEMS;
				pkt->data.play_client.windowitems.window_id = player->inventory->windowID;
				pkt->data.play_client.windowitems.count = player->inventory->slot_count;
				pkt->data.play_client.windowitems.slot_data = xmalloc(sizeof(struct slot) * player->inventory->slot_count);
				for (size_t i = 0; i < player->inventory->slot_count; i++) {
					duplicateSlot(player->inventory->slots[i], &pkt->data.play_client.windowitems.slot_data[i]);
				}
				add_queue(player->outgoingPacket, pkt);
				pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_SETSLOT;
				pkt->data.play_client.setslot.window_id = -1;
				pkt->data.play_client.setslot.slot = -1;
				duplicateSlot(player->inHand, &pkt->data.play_client.setslot.slot_data);
				add_queue(player->outgoingPacket, pkt);
			} else {
				if (mode != 5 && inv->dragSlot_count > 0) {
					inv->dragSlot_count = 0;
				}
				int sto = 0;
				if (inv->type == INVTYPE_FURNACE) sto = slot == 2;
				if (mode == 0) {
					if (s0ic && slot == 0 && invs != NULL) {
						if (player->inHand == NULL) {
							player->inHand = invs;
							invs = NULL;
							setSlot(player, inv, 0, invs, 0, 0);
							craftOnce(player, inv);
						} else if (itemsStackable(player->inHand, invs) && (player->inHand->itemCount + invs->itemCount) < maxStackSize(player->inHand)) {
							player->inHand->itemCount += invs->itemCount;
							invs = NULL;
							setSlot(player, inv, 0, invs, 0, 1);
							craftOnce(player, inv);
						}
					} else if (b == 0) {
						if (itemsStackable(player->inHand, invs)) {
							if (sto) {
								uint8_t os = player->inHand->itemCount;
								int mss = maxStackSize(player->inHand);
								player->inHand->itemCount += invs->itemCount;
								if (player->inHand->itemCount > mss) player->inHand->itemCount = mss;
								int dr = player->inHand->itemCount - os;
								if (dr >= invs->itemCount) {
									invs = NULL;
								} else invs->itemCount -= dr;
								setSlot(player, inv, slot, invs, 0, 1);
							} else {
								uint8_t os = invs->itemCount;
								int mss = maxStackSize(player->inHand);
								invs->itemCount += player->inHand->itemCount;
								if (invs->itemCount > mss) invs->itemCount = mss;
								setSlot(player, inv, slot, invs, 0, 1);
								int dr = invs->itemCount - os;
								if (dr >= player->inHand->itemCount) {
									freeSlot(player->inHand);
									xfree(player->inHand);
									player->inHand = NULL;
								} else {
									player->inHand->itemCount -= dr;
								}
							}
						} else if (player->inHand == NULL || validItemForSlot(inv->type, slot, player->inHand)) {
							struct slot* t = invs;
							invs = player->inHand;
							player->inHand = t;
							setSlot(player, inv, slot, invs, 0, 0);
						}
					} else if (b == 1) {
						if (player->inHand == NULL && invs != NULL) {
							player->inHand = xmalloc(sizeof(struct slot));
							duplicateSlot(invs, player->inHand);
							uint8_t os = invs->itemCount;
							invs->itemCount /= 2;
							player->inHand->itemCount = os - invs->itemCount;
							setSlot(player, inv, slot, invs, 0, 1);
						} else if (player->inHand != NULL && !sto && validItemForSlot(inv->type, slot, player->inHand) && (invs == NULL || (itemsStackable(player->inHand, invs) && player->inHand->itemCount + invs->itemCount < maxStackSize(player->inHand)))) {
							if (invs == NULL) {
								invs = xmalloc(sizeof(struct slot));
								duplicateSlot(player->inHand, invs);
								invs->itemCount = 1;
							} else {
								invs->itemCount++;
							}
							if (--player->inHand->itemCount <= 0) {
								freeSlot(player->inHand);
								xfree(player->inHand);
								player->inHand = NULL;
							}
							setSlot(player, inv, slot, invs, 0, 1);
						}
					}
				} else if (mode == 1 && invs != NULL) {
					if (inv->type == INVTYPE_PLAYERINVENTORY) {
						int16_t it = invs->item;
						if (slot == 0) {
							int amt = craftAll(player, inv);
							for (int i = 0; i < amt; i++)
								addInventoryItem(player, inv, invs, 44, 8, 0);
							onInventoryUpdate(player, inv, 1); // 2-4 would just repeat the calculation
						} else if (slot != 45 && it == ITM_SHIELD && inv->slots[45] == NULL) {
							swapSlots(player, inv, 45, slot, 0);
						} else if (slot != 5 && inv->slots[5] == NULL && (it == BLK_PUMPKIN || it == ITM_HELMETCLOTH || it == ITM_HELMETCHAIN || it == ITM_HELMETIRON || it == ITM_HELMETDIAMOND || it == ITM_HELMETGOLD)) {
							swapSlots(player, inv, 5, slot, 0);
						} else if (slot != 6 && inv->slots[6] == NULL && (it == ITM_CHESTPLATECLOTH || it == ITM_CHESTPLATECHAIN || it == ITM_CHESTPLATEIRON || it == ITM_CHESTPLATEDIAMOND || it == ITM_CHESTPLATEGOLD)) {
							swapSlots(player, inv, 6, slot, 0);
						} else if (slot != 7 && inv->slots[7] == NULL && (it == ITM_LEGGINGSCLOTH || it == ITM_LEGGINGSCHAIN || it == ITM_LEGGINGSIRON || it == ITM_LEGGINGSDIAMOND || it == ITM_LEGGINGSGOLD)) {
							swapSlots(player, inv, 7, slot, 0);
						} else if (slot != 8 && inv->slots[8] == NULL && (it == ITM_BOOTSCLOTH || it == ITM_BOOTSCHAIN || it == ITM_BOOTSIRON || it == ITM_BOOTSDIAMOND || it == ITM_BOOTSGOLD)) {
							swapSlots(player, inv, 8, slot, 0);
						} else if (slot > 35 && slot < 45 && invs != NULL) {
							int r = addInventoryItem(player, inv, invs, 9, 36, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						} else if (slot > 8 && slot < 36 && invs != NULL) {
							int r = addInventoryItem(player, inv, invs, 36, 45, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						} else if ((slot == 45 || slot < 9) && invs != NULL) {
							int r = addInventoryItem(player, inv, invs, 9, 36, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
							else {
								r = addInventoryItem(player, inv, invs, 36, 45, 0);
								if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
							}
						}
					} else if (inv->type == INVTYPE_WORKBENCH) {
						if (slot == 0) {
							int amt = craftAll(player, inv);
							for (int i = 0; i < amt; i++)
								addInventoryItem(player, inv, inv->slots[0], 45, 9, 0);
							onInventoryUpdate(player, inv, 1); // 2-4 would just repeat the calculation
						} else if (slot <= 9) {
							int r = addInventoryItem(player, inv, invs, 10, 46, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						} else if (slot > 9 && slot < 37) {
							int r = addInventoryItem(player, inv, invs, 37, 46, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						} else if (slot > 36 && slot < 46) {
							int r = addInventoryItem(player, inv, invs, 10, 37, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						}
					} else if (inv->type == INVTYPE_CHEST) {
						if (slot < 27) {
							int r = addInventoryItem(player, inv, invs, 62, 26, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						} else if (slot >= 27) {
							int r = addInventoryItem(player, inv, invs, 0, 27, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						}
					} else if (inv->type == INVTYPE_FURNACE) {
						if (slot > 2) {
							if (getSmeltingOutput(invs) != NULL) swapSlots(player, inv, 0, slot, 0);
							else if (getSmeltingFuelBurnTime(invs) > 0) swapSlots(player, inv, 1, slot, 0);
							else if (slot > 29) {
								int r = addInventoryItem(player, inv, invs, 3, 30, 0);
								if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
							} else if (slot < 30) {
								int r = addInventoryItem(player, inv, invs, 30, 39, 0);
								if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
							}
						} else {
							int r = addInventoryItem(player, inv, invs, 38, 2, 0);
							if (r <= 0) setSlot(player, inv, slot, NULL, 0, 1);
						}
					}
				} else if (mode == 2) {
					if (inv->type == INVTYPE_PLAYERINVENTORY) {
						if (slot == 0) {
							struct slot* invb = getSlot(player, inv, 36 + b);
							if (invb == NULL) {
								setSlot(player, inv, 36 + b, inv->slots[0], 0, 1);
								setSlot(player, inv, 0, NULL, 0, 1);
								craftOnce(player, inv);
							}
						} else if (b >= 0 && b <= 8) swapSlots(player, inv, 36 + b, slot, 0);
					} else if (inv->type == INVTYPE_WORKBENCH) {
						if (slot == 0) {
							struct slot* invb = getSlot(player, inv, 37 + b);
							if (invb == NULL) {
								setSlot(player, inv, 37 + b, inv->slots[0], 0, 1);
								setSlot(player, inv, 0, NULL, 0, 1);
								craftOnce(player, inv);
							}
						} else if (b >= 0 && b <= 8) swapSlots(player, inv, 37 + b, slot, 0);
					} else if (inv->type == INVTYPE_CHEST) {
						if (b >= 0 && b <= 8) swapSlots(player, inv, 54 + b, slot, 0);
					} else if (inv->type == INVTYPE_FURNACE) {
						if (b >= 0 && b <= 8) swapSlots(player, inv, 30 + b, slot, 0);
					}
				} else if (mode == 3) {
					if (player->gamemode == 1) {
						if (invs != NULL && player->inHand == NULL) {
							player->inHand = xmalloc(sizeof(struct slot));
							duplicateSlot(invs, player->inHand);
							player->inHand->itemCount = maxStackSize(player->inHand);
						}
					}
					//middle click, NOP in survival?
				} else if (mode == 4 && invs != NULL) {
					if (b == 0) {
						uint8_t p = invs->itemCount;
						invs->itemCount = 1;
						dropPlayerItem(player, invs);
						invs->itemCount = p - 1;
						if (invs->itemCount == 0) invs = NULL;
						setSlot(player, inv, slot, invs, 1, 1);
					} else if (b == 1) {
						dropPlayerItem(player, invs);
						invs = NULL;
						setSlot(player, inv, slot, invs, 1, 1);
					}
					if (s0ic && slot == 0) craftOnce(player, inv);
				} else if (mode == 5) {
					if (b == 1 || b == 5 || b == 9) {
						int ba = 0;
						if (inv->type == INVTYPE_PLAYERINVENTORY) {
							if (slot == 0 || (slot >= 5 && slot <= 8)) ba = 1;
						}
						if (!ba && inv->dragSlot_count < inv->slot_count && validItemForSlot(inv->type, slot, player->inHand) && (invs == NULL || itemsStackable(invs, player->inHand))) {
							inv->dragSlot[inv->dragSlot_count++] = slot;
						}
					}
				} else if (mode == 6 && player->inHand != NULL) {
					int mss = maxStackSize(player->inHand);
					if (inv->type == INVTYPE_PLAYERINVENTORY) {
						if (player->inHand->itemCount < mss) {
							for (size_t i = 9; i < 36; i++) {
								struct slot* invi = getSlot(player, inv, i);
								if (itemsStackable(player->inHand, invi)) {
									uint8_t oc = player->inHand->itemCount;
									player->inHand->itemCount += invi->itemCount;
									if (player->inHand->itemCount > mss) player->inHand->itemCount = mss;
									invi->itemCount -= player->inHand->itemCount - oc;
									if (invi->itemCount <= 0) invi = NULL;
									setSlot(player, inv, i, invi, 0, 1);
									if (player->inHand->itemCount >= mss) break;
								}
							}
						}
						if (player->inHand->itemCount < mss) {
							for (size_t i = 36; i < 45; i++) {
								struct slot* invi = getSlot(player, inv, i);
								if (itemsStackable(player->inHand, invi)) {
									uint8_t oc = player->inHand->itemCount;
									player->inHand->itemCount += invi->itemCount;
									if (player->inHand->itemCount > mss) player->inHand->itemCount = mss;
									invi->itemCount -= player->inHand->itemCount - oc;
									if (invi->itemCount <= 0) invi = NULL;
									setSlot(player, inv, i, invi, 0, 1);
									if (player->inHand->itemCount >= mss) break;
								}
							}
						}
					} else if (inv->type == INVTYPE_WORKBENCH) {
						if (player->inHand->itemCount < mss) {
							for (size_t i = 1; i < 46; i++) {
								struct slot* invi = getSlot(player, inv, i);
								if (itemsStackable(player->inHand, invi)) {
									uint8_t oc = player->inHand->itemCount;
									player->inHand->itemCount += invi->itemCount;
									if (player->inHand->itemCount > mss) player->inHand->itemCount = mss;
									invi->itemCount -= player->inHand->itemCount - oc;
									if (invi->itemCount <= 0) invi = NULL;
									setSlot(player, inv, i, invi, 0, 1);
									if (player->inHand->itemCount >= mss) break;
								}
							}
						}
					} else if (inv->type == INVTYPE_CHEST) {
						if (player->inHand->itemCount < mss) {
							for (size_t i = 0; i < 63; i++) {
								struct slot* invi = getSlot(player, inv, i);
								if (itemsStackable(player->inHand, invi)) {
									uint8_t oc = player->inHand->itemCount;
									player->inHand->itemCount += invi->itemCount;
									if (player->inHand->itemCount > mss) player->inHand->itemCount = mss;
									invi->itemCount -= player->inHand->itemCount - oc;
									if (invi->itemCount <= 0) invi = NULL;
									setSlot(player, inv, i, invi, 0, 1);
									if (player->inHand->itemCount >= mss) break;
								}
							}
						}
					} else if (inv->type == INVTYPE_FURNACE) {
						if (player->inHand->itemCount < mss) {
							for (size_t i = 0; i < 63; i++) {
								struct slot* invi = getSlot(player, inv, i);
								if (itemsStackable(player->inHand, invi)) {
									uint8_t oc = player->inHand->itemCount;
									player->inHand->itemCount += invi->itemCount;
									if (player->inHand->itemCount > mss) player->inHand->itemCount = mss;
									invi->itemCount -= player->inHand->itemCount - oc;
									if (invi->itemCount <= 0) invi = NULL;
									setSlot(player, inv, i, invi, 0, 1);
									if (player->inHand->itemCount >= mss) break;
								}
							}
						}
					}
				}
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_CONFIRMTRANSACTION;
				pkt->data.play_client.confirmtransaction.window_id = inv->windowID;
				pkt->data.play_client.confirmtransaction.action_number = act;
				pkt->data.play_client.confirmtransaction.accepted = 1;
				add_queue(player->outgoingPacket, pkt);
			}
		} else if (slot == -999) {
			if (mode != 5 && inv->dragSlot_count > 0) {
				inv->dragSlot_count = 0;
			}
			if (mode == 0 && player->inHand != NULL) {
				if (b == 1) {
					uint8_t p = player->inHand->itemCount;
					player->inHand->itemCount = 1;
					dropPlayerItem(player, player->inHand);
					player->inHand->itemCount = p - 1;
					if (player->inHand->itemCount == 0) {
						freeSlot(player->inHand);
						xfree(player->inHand);
						player->inHand = NULL;
					}
				} else if (b == 0) {
					dropPlayerItem(player, player->inHand);
					freeSlot(player->inHand);
					xfree(player->inHand);
					player->inHand = NULL;
				}
			} else if (mode == 5 && player->inHand != NULL) {
				if (b == 0 || b == 4 || b == 8) {
					memset(inv->dragSlot, 0, inv->slot_count * 2);
					inv->dragSlot_count = 0;
				} else if (inv->dragSlot_count > 0) {
					if (b == 2) {
						uint8_t total = player->inHand->itemCount;
						uint8_t per = total / inv->dragSlot_count;
						if (per == 0) per = 1;
						for (int i = 0; i < inv->dragSlot_count; i++) {
							int sl = inv->dragSlot[i];
							struct slot* ssl = getSlot(player, inv, sl);
							if (ssl == NULL) {
								ssl = xmalloc(sizeof(struct slot));
								duplicateSlot(player->inHand, ssl);
								ssl->itemCount = per;
								setSlot(player, inv, sl, ssl, 0, 1);
								if (per >= player->inHand->itemCount) {
									freeSlot(player->inHand);
									xfree(player->inHand);
									player->inHand = NULL;
									break;
								}
								player->inHand->itemCount -= per;
							} else {
								uint8_t os = ssl->itemCount;
								ssl->itemCount += per;
								int mss = maxStackSize(player->inHand);
								if (ssl->itemCount > mss) ssl->itemCount = mss;
								setSlot(player, inv, sl, ssl, 0, 1);
								if (per >= player->inHand->itemCount) {
									freeSlot(player->inHand);
									xfree(player->inHand);
									player->inHand = NULL;
									break;
								}
								player->inHand->itemCount -= ssl->itemCount - os;
							}
						}
					} else if (b == 6) {
						uint8_t per = 1;
						for (int i = 0; i < inv->dragSlot_count; i++) {
							int sl = inv->dragSlot[i];
							struct slot* ssl = getSlot(player, inv, sl);
							if (ssl == NULL) {
								ssl = xmalloc(sizeof(struct slot));
								duplicateSlot(player->inHand, ssl);
								ssl->itemCount = per;
								setSlot(player, inv, sl, ssl, 0, 1);
								if (per >= player->inHand->itemCount) {
									freeSlot(player->inHand);
									xfree(player->inHand);
									player->inHand = NULL;
									break;
								}
								player->inHand->itemCount -= per;
							} else {
								uint8_t os = ssl->itemCount;
								ssl->itemCount += per;
								int mss = maxStackSize(player->inHand);
								if (ssl->itemCount > mss) ssl->itemCount = mss;
								setSlot(player, inv, sl, ssl, 0, 1);
								if (per >= player->inHand->itemCount) {
									freeSlot(player->inHand);
									xfree(player->inHand);
									player->inHand = NULL;
									break;
								}
								player->inHand->itemCount -= ssl->itemCount - os;
							}
						}
					} else if (b == 10 && player->gamemode == 1) {
						uint8_t per = maxStackSize(player->inHand);
						for (int i = 0; i < inv->dragSlot_count; i++) {
							int sl = inv->dragSlot[i];
							struct slot* ssl = getSlot(player, inv, sl);
							if (ssl == NULL) {
								ssl = xmalloc(sizeof(struct slot));
								duplicateSlot(player->inHand, ssl);
								ssl->itemCount = per;
								setSlot(player, inv, sl, ssl, 0, 1);
							} else {
								ssl->itemCount = per;
								setSlot(player, inv, sl, ssl, 0, 1);
							}
						}
					}
				}
			}
		}
		pthread_mutex_unlock(&inv->mut);
	} else if (inp->id == PKT_PLAY_SERVER_CLOSEWINDOW) {
		player_closeWindow(player, inp->data.play_server.closewindow.window_id);
	} else if (inp->id == PKT_PLAY_SERVER_USEENTITY) {
		if (inp->data.play_server.useentity.type == 1) {
			struct entity* ent = getEntity(player->world, inp->data.play_server.useentity.target);
			if (ent != NULL && ent != player->entity && ent->health > 0. && ent->world == player->world && entity_dist(ent, player->entity) < 4. && (tick_counter - player->lastSwing) >= 3) {
				pthread_mutex_lock(&player->inventory->mut);
				damageEntityWithItem(ent, player->entity, 36 + player->currentItem, getSlot(player, player->inventory, 36 + player->currentItem));
				pthread_mutex_unlock(&player->inventory->mut);
			}
			player->lastSwing = tick_counter;
		}
	} else if (inp->id == PKT_PLAY_SERVER_CLIENTSTATUS) {
		if (inp->data.play_server.clientstatus.action_id == 0 && player->entity->health <= 0.) {
			despawnEntity(player->world, player->entity);
			spawnEntity(player->world, player->entity);
			player->entity->health = 20.;
			player->food = 20;
			player->saturation = 0.; // TODO
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_UPDATEHEALTH;
			pkt->data.play_client.updatehealth.health = player->entity->health;
			pkt->data.play_client.updatehealth.food = player->food;
			pkt->data.play_client.updatehealth.food_saturation = player->saturation;
			add_queue(player->outgoingPacket, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_RESPAWN;
			pkt->data.play_client.respawn.dimension = player->world->dimension;
			pkt->data.play_client.respawn.difficulty = difficulty;
			pkt->data.play_client.respawn.gamemode = player->gamemode;
			pkt->data.play_client.respawn.level_type = xstrdup(player->world->levelType, 0);
			add_queue(player->outgoingPacket, pkt);
			teleportPlayer(player, (double) player->world->spawnpos.x + .5, (double) player->world->spawnpos.y, (double) player->world->spawnpos.z + .5); // TODO: make overworld
			setPlayerGamemode(player, -1);
		}
	} else if (inp->id == PKT_PLAY_SERVER_USEITEM) {
		int32_t hand = inp->data.play_server.useitem.hand;
		pthread_mutex_lock(&player->inventory->mut);
		struct slot* cs = getSlot(player, player->inventory, hand ? 45 : (36 + player->currentItem));
		if (cs != NULL) {
			struct item_info* ii = getItemInfo(cs->item);
			if (ii != NULL && ii->onItemUse != NULL) (*ii->onItemUse)(player->world, player, hand ? 45 : (36 + player->currentItem), cs, 0);
		}
		pthread_mutex_unlock(&player->inventory->mut);
	}
	cont: ;
}

void tick_player(struct world* world, struct player* player) {
	if (player->defunct) {
		put_hashmap(players, player->entity->id, NULL);
		BEGIN_BROADCAST (players)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_PLAYERLISTITEM;
		pkt->data.play_client.playerlistitem.action_id = 4;
		pkt->data.play_client.playerlistitem.number_of_players = 1;
		pkt->data.play_client.playerlistitem.players = xmalloc(sizeof(struct listitem_player));
		memcpy(&pkt->data.play_client.playerlistitem.players->uuid, &player->uuid, sizeof(struct uuid));
		add_queue(bc_player->outgoingPacket, pkt);
		END_BROADCAST (players)
		despawnPlayer(player->world, player);
		add_collection(defunctPlayers, player);
		return;
	}
	player->chunksSent = 0;
	if (tick_counter % 200 == 0) {
		if (player->nextKeepAlive != 0) {
			player->conn->disconnect = 1;
			return;
		}
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_KEEPALIVE;
		pkt->data.play_client.keepalive.keep_alive_id = rand();
		add_queue(player->outgoingPacket, pkt);
	} else if (tick_counter % 200 == 100) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_TIMEUPDATE;
		pkt->data.play_client.timeupdate.time_of_day = player->world->time;
		pkt->data.play_client.timeupdate.world_age = player->world->age;
		add_queue(player->outgoingPacket, pkt);
	}
	if (player->saturation > 0. && player->food >= 20) {
		if (++player->foodTimer >= 10) {
			healEntity(player->entity, player->saturation > 6. ? player->saturation / 6. : 1.);
			player->foodTimer = 0;
		}
	} else if (player->food >= 18) {
		if (++player->foodTimer >= 80) {
			healEntity(player->entity, 1.);
			player->foodTimer = 0;
		}
	} else if (player->food <= 0) {
		if (++player->foodTimer >= 80) {
			if (player->entity->health > 10. || difficulty == 3 || (player->entity->health > 1. && difficulty == 2)) damageEntity(player->entity, 1., 0);
			player->foodTimer = 0;
		}
	}
	beginProfilerSection("chunks");
	int32_t pcx = ((int32_t) player->entity->x >> 4);
	int32_t pcz = ((int32_t) player->entity->z >> 4);
	int32_t lpcx = ((int32_t) player->entity->lx >> 4);
	int32_t lpcz = ((int32_t) player->entity->lz >> 4);
	if (player->loadedChunks->entry_count == 0 || player->triggerRechunk) {
		beginProfilerSection("chunkLoading_tick");
		pthread_mutex_lock(&chunk_input->data_mutex);
		if (player->triggerRechunk) {
			BEGIN_HASHMAP_ITERATION(player->loadedChunks)
			struct chunk* ch = (struct chunk*) value;
			if (ch->x < pcx - CHUNK_VIEW_DISTANCE || ch->x > pcx + CHUNK_VIEW_DISTANCE || ch->z < pcz - CHUNK_VIEW_DISTANCE || ch->z > pcz + CHUNK_VIEW_DISTANCE) {
				struct chunk_req* cr = xmalloc(sizeof(struct chunk_req));
				cr->pl = player;
				cr->cx = ch->x;
				cr->cz = ch->z;
				cr->load = 0;
				add_queue(chunk_input, cr);
			}
			END_HASHMAP_ITERATION(player->loadedChunks)
		}
		for (int r = 0; r <= CHUNK_VIEW_DISTANCE; r++) {
			int32_t x = pcx - r;
			int32_t z = pcz - r;
			for (int i = 0; i < ((r == 0) ? 1 : (r * 8)); i++) {
				if (!player->triggerRechunk || !contains_hashmap(player->loadedChunks, getChunkKey2(x, z))) {
					struct chunk_req* cr = xmalloc(sizeof(struct chunk_req));
					cr->pl = player;
					cr->cx = x;
					cr->cz = z;
					cr->load = 1;
					add_queue(chunk_input, cr);
				}
				if (i < 2 * r) x++;
				else if (i < 4 * r) z++;
				else if (i < 6 * r) x--;
				else if (i < 8 * r) z--;
			}
		}
		pthread_mutex_unlock(&chunk_input->data_mutex);
		player->triggerRechunk = 0;
		endProfilerSection("chunkLoading_tick");
	}
	if (lpcx != pcx || lpcz != pcz) {
		pthread_mutex_lock(&chunk_input->data_mutex);
		for (int32_t fx = lpcx; lpcx < pcx ? (fx < pcx) : (fx > pcx); lpcx < pcx ? fx++ : fx--) {
			for (int32_t fz = lpcz - CHUNK_VIEW_DISTANCE; fz <= lpcz + CHUNK_VIEW_DISTANCE; fz++) {
				beginProfilerSection("chunkUnloading_live");
				struct chunk_req* cr = xmalloc(sizeof(struct chunk_req));
				cr->pl = player;
				cr->cx = lpcx < pcx ? (fx - CHUNK_VIEW_DISTANCE) : (fx + CHUNK_VIEW_DISTANCE);
				cr->cz = fz;
				cr->load = 0;
				add_queue(chunk_input, cr);
				endProfilerSection("chunkUnloading_live");
				beginProfilerSection("chunkLoading_live");
				cr = xmalloc(sizeof(struct chunk_req));
				cr->pl = player;
				cr->cx = lpcx < pcx ? (fx + CHUNK_VIEW_DISTANCE) : (fx - CHUNK_VIEW_DISTANCE);
				cr->cz = fz;
				cr->load = 1;
				add_queue(chunk_input, cr);
				endProfilerSection("chunkLoading_live");
			}
		}
		for (int32_t fz = lpcz; lpcz < pcz ? (fz < pcz) : (fz > pcz); lpcz < pcz ? fz++ : fz--) {
			for (int32_t fx = lpcx - CHUNK_VIEW_DISTANCE; fx <= lpcx + CHUNK_VIEW_DISTANCE; fx++) {
				beginProfilerSection("chunkUnloading_live");
				struct chunk_req* cr = xmalloc(sizeof(struct chunk_req));
				cr->pl = player;
				cr->cx = fx;
				cr->cz = lpcz < pcz ? (fz - CHUNK_VIEW_DISTANCE) : (fz + CHUNK_VIEW_DISTANCE);
				cr->load = 0;
				add_queue(chunk_input, cr);
				endProfilerSection("chunkUnloading_live");
				beginProfilerSection("chunkLoading_live");
				cr = xmalloc(sizeof(struct chunk_req));
				cr->pl = player;
				cr->cx = fx;
				cr->cz = lpcz < pcz ? (fz + CHUNK_VIEW_DISTANCE) : (fz - CHUNK_VIEW_DISTANCE);
				cr->load = 1;
				add_queue(chunk_input, cr);
				endProfilerSection("chunkLoading_live");
			}
		}
		pthread_mutex_unlock(&chunk_input->data_mutex);
	}
	endProfilerSection("chunks");

	//if (((int32_t) player->entity->lx >> 4) != pcx || ((int32_t) player->entity->lz >> 4) != pcz || player->loadedChunks->count < CHUNK_VIEW_DISTANCE * CHUNK_VIEW_DISTANCE * 4 || player->triggerRechunk) {
	//}
	if (player->digging >= 0.) {
		block bw = getBlockWorld(world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
		struct block_info* bi = getBlockInfo(bw);
		float digspeed = 0.;
		if (bi->hardness > 0.) {
			pthread_mutex_lock(&player->inventory->mut);
			struct slot* ci = getSlot(player, player->inventory, 36 + player->currentItem);
			struct item_info* cii = ci == NULL ? NULL : getItemInfo(ci->item);
			int hasProperTool = (cii == NULL ? 0 : isToolProficient(cii->toolType, cii->harvestLevel, bw));
			int hasProperTool2 = (cii == NULL ? 0 : isToolProficient(cii->toolType, 0xFF, bw));
			pthread_mutex_unlock(&player->inventory->mut);
			int rnt = bi->material->requiresnotool;
			float ds = hasProperTool2 ? cii->toolProficiency : 1.;
			//efficiency enchantment
			for (size_t i = 0; i < player->entity->effect_count; i++) {
				if (player->entity->effects[i].effectID == POT_HASTE) {
					ds *= 1. + (float) (player->entity->effects[i].amplifier + 1) * .2;
				}
			}
			for (size_t i = 0; i < player->entity->effect_count; i++) {
				if (player->entity->effects[i].effectID == POT_MINING_FATIGUE) {
					if (player->entity->effects[i].amplifier == 0) ds *= .3;
					else if (player->entity->effects[i].amplifier == 1) ds *= .09;
					else if (player->entity->effects[i].amplifier == 2) ds *= .0027;
					else ds *= .00081;
				}
			}
			//if in water and not in aqua affintity enchant ds /= 5.;
			if (!player->entity->onGround) ds /= 5.;
			digspeed = ds / (bi->hardness * ((hasProperTool || rnt) ? 30. : 100.));
		}
		player->digging += (player->digging == 0. ? 2. : 1.) * digspeed;
		BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_BLOCKBREAKANIMATION;
		pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
		memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
		pkt->data.play_client.blockbreakanimation.destroy_stage = (int8_t)(player->digging * 9);
		add_queue(bc_player->outgoingPacket, pkt);
		END_BROADCAST(player->world->players)
	}
	beginProfilerSection("entity_transmission");
	BEGIN_HASHMAP_ITERATION(player->loadedEntities)
	struct entity* ent = (struct entity*) value;
	if (entity_distsq(ent, player->entity) > (CHUNK_VIEW_DISTANCE * 16.) * (CHUNK_VIEW_DISTANCE * 16.)) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_DESTROYENTITIES;
		pkt->data.play_client.destroyentities.count = 1;
		pkt->data.play_client.destroyentities.entity_ids = xmalloc(sizeof(int32_t));
		pkt->data.play_client.destroyentities.entity_ids[0] = ent->id;
		add_queue(player->outgoingPacket, pkt);
		pthread_rwlock_unlock(&player->loadedEntities->data_mutex);
		put_hashmap(player->loadedEntities, ent->id, NULL);
		pthread_rwlock_rdlock(&player->loadedEntities->data_mutex);
		put_hashmap(ent->loadingPlayers, player->entity->id, NULL);
	}
	if (ent->type != ENT_PLAYER) {
		sendEntityMove(player, ent);
	}
	END_HASHMAP_ITERATION(player->loadedEntities)
	endProfilerSection("entity_transmission");
	beginProfilerSection("player_transmission");
	//int32_t cx = ((int32_t) player->entity->x) >> 4;
	//int32_t cz = ((int32_t) player->entity->z) >> 4;
	//for (int32_t icx = cx - CHUNK_VIEW_DISTANCE; icx <= cx + CHUNK_VIEW_DISTANCE; icx++)
	//for (int32_t icz = cz - CHUNK_VIEW_DISTANCE; icz <= cz + CHUNK_VIEW_DISTANCE; icz++) {
	//struct chunk* ch = getChunk(player->world, icx, icz);
	//if (ch != NULL) {
	BEGIN_HASHMAP_ITERATION(player->world->entities)
	struct entity* ent = (struct entity*) value;
	if (ent == player->entity || contains_hashmap(player->loadedEntities, ent->id) || entity_distsq(player->entity, ent) > (CHUNK_VIEW_DISTANCE * 16.) * (CHUNK_VIEW_DISTANCE * 16.)) continue;
	if (ent->type == ENT_PLAYER) loadPlayer(player, ent->data.player.player);
	else loadEntity(player, ent);
	END_HASHMAP_ITERATION(player->world->entities)
	//}
	//}
	endProfilerSection("player_transmission");
	//printf("%i\n", player->loadedChunks->size);
}

int player_onGround(struct player* player) {
	struct entity* entity = player->entity;
	struct boundingbox obb;
	getEntityCollision(entity, &obb);
	if (obb.minX == obb.maxX || obb.minZ == obb.maxZ || obb.minY == obb.maxY) {
		return 0;
	}
	obb.minY += -.08;
	struct boundingbox pbb;
	getEntityCollision(entity, &pbb);
	double ny = -.08;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				if (b == 0) continue;
				struct block_info* bi = getBlockInfo(b);
				if (bi == NULL) continue;
				for (size_t i = 0; i < bi->boundingBox_count; i++) {
					struct boundingbox* bb = &bi->boundingBoxes[i];
					if (b > 0 && bb->minX != bb->maxX && bb->minY != bb->maxY && bb->minZ != bb->maxZ) {
						if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
							if (pbb.maxX > bb->minX + x && pbb.minX < bb->maxX + x && pbb.maxZ > bb->minZ + z && pbb.minZ < bb->maxZ + z) {
								double t;
								if (ny > 0. && pbb.maxY <= bb->minY + y) {
									t = bb->minY + y - pbb.maxY;
									if (t < ny) {
										ny = t;
									}
								} else if (ny < 0. && pbb.minY >= bb->maxY + y) {
									t = bb->maxY + y - pbb.minY;
									if (t > ny) {
										ny = t;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return fabs(-.08 - ny) > .001;
}

void kickPlayer(struct player* player, char* message) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_DISCONNECT;
	size_t sl = strlen(message);
	pkt->data.play_client.disconnect.reason = xmalloc(sl + 512);
	snprintf(pkt->data.play_client.disconnect.reason, sl + 512, "{\"text\": \"%s\", \"color\": \"red\"}", message);
	add_queue(player->outgoingPacket, pkt);
	if (player->conn != NULL) player->conn->disconnect = 1;
	broadcastf("red", "Kicked Player %s for reason: %s", player->name, message);
}

float player_getAttackStrength(struct player* player, float adjust) {
	float cooldownPeriod = 4.;
	struct slot* sl = getSlot(player, player->inventory, 36 + player->currentItem);
	if (sl != NULL) {
		struct item_info* ii = getItemInfo(sl->item);
		if (ii != NULL) {
			cooldownPeriod = ii->attackSpeed;
		}
	}
	float str = ((float) (tick_counter - player->lastSwing) + adjust) * cooldownPeriod / 20.;
	if (str > 1.) str = 1.;
	if (str < 0.) str = 0.;
	return str;
}

void teleportPlayer(struct player* player, double x, double y, double z) {
	player->entity->x = x;
	player->entity->y = y;
	player->entity->z = z;
	player->entity->lx = x;
	player->entity->ly = y;
	player->entity->lz = z;
	player->triggerRechunk = 1;
	player->spawnedIn = 0;
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYVELOCITY;
	pkt->data.play_client.entityvelocity.entity_id = player->entity->id;
	pkt->data.play_client.entityvelocity.velocity_x = 0;
	pkt->data.play_client.entityvelocity.velocity_y = 0;
	pkt->data.play_client.entityvelocity.velocity_z = 0;
	add_queue(player->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK;
	pkt->data.play_client.playerpositionandlook.x = player->entity->x;
	pkt->data.play_client.playerpositionandlook.y = player->entity->y;
	pkt->data.play_client.playerpositionandlook.z = player->entity->z;
	pkt->data.play_client.playerpositionandlook.yaw = player->entity->yaw;
	pkt->data.play_client.playerpositionandlook.pitch = player->entity->pitch;
	pkt->data.play_client.playerpositionandlook.flags = 0x0;
	pkt->data.play_client.playerpositionandlook.teleport_id = 0;
	add_queue(player->outgoingPacket, pkt);
//	if (player->tps > 0) player->tps--;
}

struct player* getPlayerByName(char* name) {
	BEGIN_HASHMAP_ITERATION (players)
	struct player* player = (struct player*) value;
	if (player != NULL && streq_nocase(name, player->name)) {
		BREAK_HASHMAP_ITERATION(players);
		return player;
	}
	END_HASHMAP_ITERATION (players)
	return NULL;
}

void player_closeWindow(struct player* player, uint16_t windowID) {
	struct inventory* inv = NULL;
	if (windowID == 0 && player->openInv == NULL) inv = player->inventory;
	else if (player->openInv != NULL && windowID == player->openInv->windowID) inv = player->openInv;
	if (inv != NULL) {
		pthread_mutex_lock(&inv->mut);
		if (player->inHand != NULL) {
			dropPlayerItem(player, player->inHand);
			freeSlot(player->inHand);
			xfree(player->inHand);
			player->inHand = NULL;
		}
		if (inv->type == INVTYPE_PLAYERINVENTORY) {
			for (int i = 1; i < 5; i++) {
				if (inv->slots[i] != NULL) {
					dropPlayerItem(player, inv->slots[i]);
					setSlot(player, inv, i, NULL, 0, 1);
				}
			}
		} else if (inv->type == INVTYPE_WORKBENCH) {
			for (int i = 1; i < 10; i++) {
				if (inv->slots[i] != NULL) {
					dropPlayerItem(player, inv->slots[i]);
					setSlot(player, inv, i, NULL, 0, 1);
				}
			}
			pthread_mutex_unlock(&inv->mut);
			freeInventory(inv);
			inv = NULL;
		} else if (inv->type == INVTYPE_CHEST) {
			if (inv->te != NULL) {
				BEGIN_BROADCAST_DIST(player->entity, 128.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_BLOCKACTION;
				pkt->data.play_client.blockaction.location.x = inv->te->x;
				pkt->data.play_client.blockaction.location.y = inv->te->y;
				pkt->data.play_client.blockaction.location.z = inv->te->z;
				pkt->data.play_client.blockaction.action_id = 1;
				pkt->data.play_client.blockaction.action_param = inv->players->entry_count - 1;
				pkt->data.play_client.blockaction.block_type = getBlockWorld(player->world, inv->te->x, inv->te->y, inv->te->z) >> 4;
				add_queue(bc_player->outgoingPacket, pkt);
				END_BROADCAST(player->world->players)
			}
		}
		if (inv != NULL) {
			if (inv->type != INVTYPE_PLAYERINVENTORY) put_hashmap(inv->players, player->entity->id, NULL);
			pthread_mutex_unlock(&inv->mut);
		}
	}
	player->openInv = NULL; // TODO: free sometimes?

}

void setPlayerGamemode(struct player* player, int gamemode) {
	if (gamemode != -1) {
		player->gamemode = gamemode;
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_CHANGEGAMESTATE;
		pkt->data.play_client.changegamestate.reason = 3;
		pkt->data.play_client.changegamestate.value = gamemode;
		add_queue(player->outgoingPacket, pkt);
	}
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_PLAYERABILITIES;
	pkt->data.play_client.playerabilities.flags = player->gamemode == 1 ? (0x04 | 0x08) : 0x0;
	pkt->data.play_client.playerabilities.flying_speed = .05;
	pkt->data.play_client.playerabilities.field_of_view_modifier = .1;
	add_queue(player->outgoingPacket, pkt);
}

void freePlayer(struct player* player) {
	struct packet* pkt = NULL;
	while ((pkt = pop_nowait_queue(player->incomingPacket)) != NULL) {
		freePacket(STATE_PLAY, 0, pkt);
		xfree(pkt);
	}
	del_queue(player->incomingPacket);
	while ((pkt = pop_nowait_queue(player->outgoingPacket)) != NULL) {
		freePacket(STATE_PLAY, 1, pkt);
		xfree(pkt);
	}
	del_queue(player->outgoingPacket);
	if (player->inHand != NULL) {
		freeSlot(player->inHand);
		xfree(player->inHand);
	}
	del_hashmap(player->loadedChunks);
	del_hashmap(player->loadedEntities);
	freeInventory(player->inventory);
	xfree(player->inventory);
	xfree(player->name);
	xfree(player);
}
