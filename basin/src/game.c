/*
 * game.c
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#include "world.h"
#include "globals.h"
#include "util.h"
#include "queue.h"
#include "collection.h"
#include "packet.h"
#include <errno.h>
#include <stdint.h>
#include "xstring.h"
#include <stdio.h>
#include "entity.h"
#include "game.h"
#include "block.h"

void flush_outgoing(struct player* player) {
	uint8_t onec = 1;
	if (write(player->conn->work->pipes[1], &onec, 1) < 1) {
		printf("Failed to write to wakeup pipe! Things may slow down. %s\n", strerror(errno));
	}
}

void loadPlayer(struct player* to, struct player* from) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_SPAWNPLAYER;
	pkt->data.play_client.spawnplayer.entity_id = from->entity->id;
	memcpy(&pkt->data.play_client.spawnplayer.player_uuid, &from->entity->data.player.player->uuid, sizeof(struct uuid));
	pkt->data.play_client.spawnplayer.x = from->entity->x;
	pkt->data.play_client.spawnplayer.y = from->entity->y;
	pkt->data.play_client.spawnplayer.z = from->entity->z;
	pkt->data.play_client.spawnplayer.yaw = (from->entity->yaw / 360.) * 256.;
	pkt->data.play_client.spawnplayer.pitch = (from->entity->pitch / 360.) * 256.;
	writeMetadata(from->entity, &pkt->data.play_client.spawnplayer.metadata.metadata, &pkt->data.play_client.spawnplayer.metadata.metadata_size);
	add_queue(to->conn->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 0;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[from->currentItem + 36], &pkt->data.play_client.entityequipment.item);
	add_queue(to->conn->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 5;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[5], &pkt->data.play_client.entityequipment.item);
	add_queue(to->conn->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 4;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[6], &pkt->data.play_client.entityequipment.item);
	add_queue(to->conn->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 3;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[7], &pkt->data.play_client.entityequipment.item);
	add_queue(to->conn->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 2;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[8], &pkt->data.play_client.entityequipment.item);
	add_queue(to->conn->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 1;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[45], &pkt->data.play_client.entityequipment.item);
	add_queue(to->conn->outgoingPacket, pkt);
	flush_outgoing(to);
}

void onInventoryUpdate(struct inventory* inventory, int slot) {
	if (inventory->type == INVTYPE_PLAYERINVENTORY) {
		if (slot == inventory->player->currentItem + 36) {
			BEGIN_BROADCAST_EXCEPT_DIST(inventory->player, inventory->player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = inventory->player->entity->id;
			pkt->data.play_client.entityequipment.slot = 0;
			duplicateSlot(inventory->slots == NULL ? NULL : inventory->slots[inventory->player->currentItem + 36], &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->conn->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (slot >= 5 && slot <= 8) {
			BEGIN_BROADCAST_EXCEPT_DIST(inventory->player, inventory->player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = inventory->player->entity->id;
			if (slot == 5) pkt->data.play_client.entityequipment.slot = 5;
			else if (slot == 6) pkt->data.play_client.entityequipment.slot = 4;
			else if (slot == 7) pkt->data.play_client.entityequipment.slot = 3;
			else if (slot == 8) pkt->data.play_client.entityequipment.slot = 2;
			duplicateSlot(inventory->slots == NULL ? NULL : inventory->slots[slot], &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->conn->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (slot == 45) {
			BEGIN_BROADCAST_EXCEPT_DIST(inventory->player, inventory->player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = inventory->player->entity->id;
			pkt->data.play_client.entityequipment.slot = 1;
			duplicateSlot(inventory->slots == NULL ? NULL : inventory->slots[45], &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->conn->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		}
	}
}

void tick_player(struct world* world, struct player* player) {
	if (tick_counter % 200 == 0) {
		if (player->nextKeepAlive != 0) {
			player->conn->disconnect = 1;
			flush_outgoing(player);
			return;
		}
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_KEEPALIVE;
		pkt->data.play_client.keepalive.keep_alive_id = rand();
		add_queue(player->conn->outgoingPacket, pkt);
		flush_outgoing(player);
	}
	int32_t pcx = ((int32_t) player->entity->x >> 4);
	int32_t pcz = ((int32_t) player->entity->z >> 4);
	if (((int32_t) player->entity->lx >> 4) != pcx || ((int32_t) player->entity->lz >> 4) != pcz || player->loadedChunks->count == 0) {
		for (int x = -CHUNK_VIEW_DISTANCE + pcx; x <= CHUNK_VIEW_DISTANCE + pcx; x++) {
			for (int z = -CHUNK_VIEW_DISTANCE + pcz; z <= CHUNK_VIEW_DISTANCE + pcz; z++) {
				struct chunk* ch = getChunk(player->world, x, z);
				if (ch != NULL && !contains_collection(player->loadedChunks, ch)) {
					ch->playersLoaded++;
					add_collection(player->loadedChunks, ch);
					struct packet* pkt = xmalloc(sizeof(struct packet));
					pkt->id = PKT_PLAY_CLIENT_CHUNKDATA;
					pkt->data.play_client.chunkdata.data = ch;
					pkt->data.play_client.chunkdata.ground_up_continuous = 1;
					pkt->data.play_client.chunkdata.number_of_block_entities = 0;
					pkt->data.play_client.chunkdata.block_entities = NULL;
					add_queue(player->conn->outgoingPacket, pkt);
					flush_outgoing(player);
					//printf("client load %i, %i\n", ch->x, ch->z);
				}
			}
		}
		pthread_rwlock_wrlock(&player->loadedChunks->data_mutex);
		for (int i = 0; i < player->loadedChunks->size; i++) {
			struct chunk* ch = player->loadedChunks->data[i];
			if (ch == NULL) continue;
			int dx = (ch->x - pcx);
			if (dx < 0) dx = -dx;
			int dz = (ch->z - pcz);
			if (dz < 0) dz = -dz;
			if (dx > CHUNK_VIEW_DISTANCE || dz > CHUNK_VIEW_DISTANCE) {
				//printf("client unload %i, %i\n", ch->x, ch->z);
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_UNLOADCHUNK;
				pkt->data.play_client.unloadchunk.chunk_x = ch->x;
				pkt->data.play_client.unloadchunk.chunk_z = ch->z;
				add_queue(player->conn->outgoingPacket, pkt);
				flush_outgoing(player);
				player->loadedChunks->data[i] = NULL;
				player->loadedChunks->count--;
				if (--ch->playersLoaded <= 0) {
					//printf("server unload %i, %i\n", ch->x, ch->z);
					unloadChunk(player->world, ch);
				}
			}
		}
		pthread_rwlock_unlock(&player->loadedChunks->data_mutex);
	}
	if (player->digging >= 0.) {
		struct block_info* bi = &block_infos[getBlockWorld(world, player->digging_position.x, player->digging_position.y, player->digging_position.z)];
		float digspeed = 0.;
		if (bi->hardness > 0.) {
			int hasProperTool = 0 || block_materials[bi->material].requiresnotool; // TODO: proper tool not 0
			float ds = 1.; // todo modify by tool
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
			digspeed = ds / (bi->hardness * (hasProperTool ? 30. : 100.));
		}
		player->digging += (player->digging == 0. ? 2. : 1.) * digspeed;
		BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_BLOCKBREAKANIMATION;
		pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
		memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
		pkt->data.play_client.blockbreakanimation.destroy_stage = (int8_t)(player->digging * 9);
		add_queue(player->conn->outgoingPacket, pkt);
		flush_outgoing(player);
		END_BROADCAST
	}
	for (size_t i = 0; i < player->world->entities->size; i++) {
		struct entity* ent = (struct entity*) player->world->entities->data[i];
		if (ent == NULL || entity_distsq(ent, player->entity) > 16384.) continue;
		double md = entity_distsq_block(ent, ent->lx, ent->ly, ent->lz);
		double mp = (ent->yaw - ent->lyaw) * (ent->yaw - ent->lyaw) + (ent->pitch - ent->lpitch) * (ent->pitch - ent->lpitch);
		//printf("mp = %f, md = %f\n", mp, md);
		if ((ent->type != ENT_PLAYER || ent->data.player.player != player) && (md > .001 || mp > .01 || ent->type == ENT_PLAYER)) {
			struct packet* pkt = xmalloc(sizeof(struct packet));
			int ft = tick_counter % 20 == 0;
			if (!ft && md <= .001 && mp <= .01) {
				pkt->id = PKT_PLAY_CLIENT_ENTITY;
				pkt->data.play_client.entity.entity_id = ent->id;
				add_queue(player->conn->outgoingPacket, pkt);
				flush_outgoing(player);
			} else if (!ft && mp > .01 && md <= .001) {
				pkt->id = PKT_PLAY_CLIENT_ENTITYLOOK;
				pkt->data.play_client.entitylook.entity_id = ent->id;
				pkt->data.play_client.entitylook.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
				pkt->data.play_client.entitylook.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
				pkt->data.play_client.entitylook.on_ground = ent->onGround;
				add_queue(player->conn->outgoingPacket, pkt);
				pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
				pkt->data.play_client.entityheadlook.entity_id = ent->id;
				pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
				add_queue(player->conn->outgoingPacket, pkt);
				flush_outgoing(player);
			} else if (!ft && mp <= .01 && md > .001 && md < 64.) {
				pkt->id = PKT_PLAY_CLIENT_ENTITYRELATIVEMOVE;
				pkt->data.play_client.entityrelativemove.entity_id = ent->id;
				pkt->data.play_client.entityrelativemove.delta_x = (int16_t)((ent->x * 32. - ent->lx * 32.) * 128.);
				pkt->data.play_client.entityrelativemove.delta_y = (int16_t)((ent->y * 32. - ent->ly * 32.) * 128.);
				pkt->data.play_client.entityrelativemove.delta_z = (int16_t)((ent->z * 32. - ent->lz * 32.) * 128.);
				pkt->data.play_client.entityrelativemove.on_ground = ent->onGround;
				add_queue(player->conn->outgoingPacket, pkt);
				flush_outgoing(player);
			} else if (!ft && mp > .01 && md > .001 && md < 64.) {
				pkt->id = PKT_PLAY_CLIENT_ENTITYLOOKANDRELATIVEMOVE;
				pkt->data.play_client.entitylookandrelativemove.entity_id = ent->id;
				pkt->data.play_client.entitylookandrelativemove.delta_x = (int16_t)((ent->x * 32. - ent->lx * 32.) * 128.);
				pkt->data.play_client.entitylookandrelativemove.delta_y = (int16_t)((ent->y * 32. - ent->ly * 32.) * 128.);
				pkt->data.play_client.entitylookandrelativemove.delta_z = (int16_t)((ent->z * 32. - ent->lz * 32.) * 128.);
				pkt->data.play_client.entitylookandrelativemove.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
				pkt->data.play_client.entitylookandrelativemove.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
				pkt->data.play_client.entitylookandrelativemove.on_ground = ent->onGround;
				add_queue(player->conn->outgoingPacket, pkt);
				pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
				pkt->data.play_client.entityheadlook.entity_id = ent->id;
				pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
				add_queue(player->conn->outgoingPacket, pkt);
				flush_outgoing(player);
			} else {
				pkt->id = PKT_PLAY_CLIENT_ENTITYTELEPORT;
				pkt->data.play_client.entityteleport.entity_id = ent->id;
				pkt->data.play_client.entityteleport.x = ent->x;
				pkt->data.play_client.entityteleport.y = ent->y;
				pkt->data.play_client.entityteleport.z = ent->z;
				pkt->data.play_client.entityteleport.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
				pkt->data.play_client.entityteleport.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
				pkt->data.play_client.entityteleport.on_ground = ent->onGround;
				add_queue(player->conn->outgoingPacket, pkt);
				pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
				pkt->data.play_client.entityheadlook.entity_id = ent->id;
				pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
				add_queue(player->conn->outgoingPacket, pkt);
				flush_outgoing(player);
			}
		}
	}

}

void tick_entity(struct world* world, struct entity* entity) {
	entity->lx = entity->x;
	entity->ly = entity->y;
	entity->lz = entity->z;
	entity->lyaw = entity->yaw;
	entity->lpitch = entity->pitch;
}

void tick_world(struct world* world) { //TODO: separate tick threads
	for (size_t i = 0; i < world->players->size; i++) {
		struct player* player = (struct player*) world->players->data[i];
		if (player == NULL) continue;
		tick_player(world, player);
	}
	for (size_t i = 0; i < world->entities->size; i++) {
		struct entity* entity = (struct entity*) world->entities->data[i];
		if (entity == NULL) continue;
		tick_entity(world, entity);
	}
}

void broadcast(char* text) {
	size_t s = strlen(text) + 512;
	char* rsx = xstrdup(text, 512);
	char* rs = xmalloc(512);
	snprintf(rs, s, "{\"text\": \"%s\"}", replace(replace(rsx, "\\", "\\\\"), "\"", "\\\""));
	printf("<CHAT> %s\n", text);
	BEGIN_BROADCAST (players)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_CHATMESSAGE;
	pkt->data.play_client.chatmessage.position = 0;
	pkt->data.play_client.chatmessage.json_data = xstrdup(rs, 0);
	add_queue(bc_player->conn->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST xfree(rs);
	xfree(rsx);
}
