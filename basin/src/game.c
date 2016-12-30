/*
 * game.c
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#include "block.h"
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
#include <math.h>
#include "crafting.h"
#include "inventory.h"
#include "item.h"
#include "network.h"
#include "smelting.h"
#include "server.h"
#include "command.h"
#include <unistd.h>
#include "tileentity.h"
#include <stdarg.h>
#include "xstring.h"

void flush_outgoing(struct player* player) {
	if (player->conn == NULL) return;
	uint8_t onec = 1;
	if (write(player->conn->work->pipes[1], &onec, 1) < 1) {
		printf("Failed to write to wakeup pipe! Things may slow down. %s\n", strerror(errno));
	}
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
			flush_outgoing(player);
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
			flush_outgoing(player);
		} else if (!ft && mp <= .01 && md > .001 && md < 64.) {
			pkt->id = PKT_PLAY_CLIENT_ENTITYRELATIVEMOVE;
			pkt->data.play_client.entityrelativemove.entity_id = ent->id;
			pkt->data.play_client.entityrelativemove.delta_x = (int16_t)((ent->x * 32. - ent->lx * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.delta_y = (int16_t)((ent->y * 32. - ent->ly * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.delta_z = (int16_t)((ent->z * 32. - ent->lz * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.on_ground = ent->onGround;
			add_queue(player->outgoingPacket, pkt);
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
			add_queue(player->outgoingPacket, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoingPacket, pkt);
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
			add_queue(player->outgoingPacket, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoingPacket, pkt);
			flush_outgoing(player);
		}
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
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 0;
	duplicateSlot(from->inventory->slots == NULL ? NULL : from->inventory->slots[from->currentItem + 36], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 5;
	duplicateSlot(from->inventory->slots == NULL ? NULL : from->inventory->slots[5], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 4;
	duplicateSlot(from->inventory->slots == NULL ? NULL : from->inventory->slots[6], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 3;
	duplicateSlot(from->inventory->slots == NULL ? NULL : from->inventory->slots[7], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 2;
	duplicateSlot(from->inventory->slots == NULL ? NULL : from->inventory->slots[8], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 1;
	duplicateSlot(from->inventory->slots == NULL ? NULL : from->inventory->slots[45], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	flush_outgoing(to);
	add_collection(to->loadedEntities, from->entity);
	add_collection(from->entity->loadingPlayers, to);
}

void loadEntity(struct player* to, struct entity* from) {
	if (shouldSendAsObject(from->type)) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNOBJECT;
		pkt->data.play_client.spawnobject.entity_id = from->id;
		struct uuid uuid;
		for (int i = 0; i < 4; i++)
			memcpy((void*) &uuid + 4 * i, &from->id, 4);
		memcpy(&pkt->data.play_client.spawnobject.object_uuid, &uuid, sizeof(struct uuid));
		pkt->data.play_client.spawnobject.type = networkEntConvert(0, from->type);
		pkt->data.play_client.spawnobject.x = from->x;
		pkt->data.play_client.spawnobject.y = from->y;
		pkt->data.play_client.spawnobject.z = from->z;
		pkt->data.play_client.spawnobject.yaw = (from->yaw / 360.) * 256.;
		pkt->data.play_client.spawnobject.pitch = (from->pitch / 360.) * 256.;
		pkt->data.play_client.spawnobject.data = from->objectData;
		pkt->data.play_client.spawnobject.velocity_x = (int16_t)(from->motX * 8000.);
		pkt->data.play_client.spawnobject.velocity_y = (int16_t)(from->motY * 8000.);
		pkt->data.play_client.spawnobject.velocity_z = (int16_t)(from->motZ * 8000.);
		add_queue(to->outgoingPacket, pkt);
		pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
		pkt->data.play_client.entitymetadata.entity_id = from->id;
		writeMetadata(from, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
		add_queue(to->outgoingPacket, pkt);
		flush_outgoing(to);
		add_collection(to->loadedEntities, from);
		add_collection(from->loadingPlayers, to);
	} else if (from->type == ENT_PLAYER) {
		return;
	} else if (from->type == ENT_PAINTING) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNOBJECT;
		pkt->data.play_client.spawnpainting.entity_id = from->id;
		struct uuid uuid;
		for (int i = 0; i < 4; i++)
			memcpy((void*) &uuid + 4 * i, &from->id, 4);
		memcpy(&pkt->data.play_client.spawnpainting.entity_uuid, &uuid, sizeof(struct uuid));
		pkt->data.play_client.spawnpainting.location.x = (int32_t) from->x;
		pkt->data.play_client.spawnpainting.location.y = (int32_t) from->y;
		pkt->data.play_client.spawnpainting.location.z = (int32_t) from->z;
		pkt->data.play_client.spawnpainting.title = xstrdup(from->data.painting.title, 0);
		pkt->data.play_client.spawnpainting.direction = from->data.painting.direction;
		add_queue(to->outgoingPacket, pkt);
		flush_outgoing(to);
		add_collection(to->loadedEntities, from);
		add_collection(from->loadingPlayers, to);
	} else if (from->type == ENT_EXPERIENCEORB) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNOBJECT;
		pkt->data.play_client.spawnexperienceorb.entity_id = from->id;
		pkt->data.play_client.spawnexperienceorb.x = from->x;
		pkt->data.play_client.spawnexperienceorb.y = from->y;
		pkt->data.play_client.spawnexperienceorb.z = from->z;
		pkt->data.play_client.spawnexperienceorb.count = from->data.experienceorb.count;
		add_queue(to->outgoingPacket, pkt);
		flush_outgoing(to);
		add_collection(to->loadedEntities, from);
		add_collection(from->loadingPlayers, to);
	} else {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNMOB;
		pkt->data.play_client.spawnmob.entity_id = from->id;
		struct uuid uuid;
		for (int i = 0; i < 4; i++)
			memcpy((void*) &uuid + 4 * i, &from->id, 4);
		memcpy(&pkt->data.play_client.spawnmob.entity_uuid, &uuid, sizeof(struct uuid));
		pkt->data.play_client.spawnmob.type = networkEntConvert(1, from->type);
		pkt->data.play_client.spawnmob.x = from->x;
		pkt->data.play_client.spawnmob.y = from->y;
		pkt->data.play_client.spawnmob.z = from->z;
		pkt->data.play_client.spawnmob.yaw = (from->yaw / 360.) * 256.;
		pkt->data.play_client.spawnmob.pitch = (from->pitch / 360.) * 256.;
		pkt->data.play_client.spawnmob.head_pitch = (from->pitch / 360.) * 256.;
		pkt->data.play_client.spawnmob.velocity_x = (int16_t)(from->motX * 8000.);
		pkt->data.play_client.spawnmob.velocity_y = (int16_t)(from->motY * 8000.);
		pkt->data.play_client.spawnmob.velocity_z = (int16_t)(from->motZ * 8000.);
		writeMetadata(from, &pkt->data.play_client.spawnmob.metadata.metadata, &pkt->data.play_client.spawnmob.metadata.metadata_size);
		add_queue(to->outgoingPacket, pkt);
		flush_outgoing(to);
		add_collection(to->loadedEntities, from);
		add_collection(from->loadingPlayers, to);
	}
}

void craftOnce(struct player* player, struct inventory* inv) {
	int cs = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	for (int i = 1; i <= cs; i++) {
		struct slot* invi = getSlot(player, inv, i);
		if (invi == NULL) continue;
		if (--invi->itemCount <= 0) {
			freeSlot(invi);
			xfree(invi);
			invi = NULL;
		}
		inv->slots[i] = invi;
	}
	onInventoryUpdate(player, inv, 1);
}

int craftAll(struct player* player, struct inventory* inv) {
	int cs = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	uint8_t ct = 64;
	for (int i = 1; i <= cs; i++) {
		struct slot* invi = getSlot(player, inv, i);
		if (invi == NULL) continue;
		if (invi->itemCount < ct) ct = invi->itemCount;
	}
	for (int i = 1; i <= cs; i++) {
		struct slot* invi = getSlot(player, inv, i);
		if (invi == NULL) continue;
		invi->itemCount -= ct;
		if (invi->itemCount <= 0) {
			freeSlot(invi);
			xfree(invi);
			invi = NULL;
		}
		inv->slots[i] = invi;
	}
	return ct;
}

struct slot* getCraftResult(struct slot** slots, size_t slot_count) { // 012/345/678 or 01/23
	struct slot** os = xmalloc(sizeof(struct slot*) * slot_count);
	memcpy(os, slots, slot_count * sizeof(struct slot*));
	if (slot_count == 4) {
		if (os[0] == NULL && os[1] == NULL) {
			os[0] = os[2];
			os[1] = os[3];
			os[2] = NULL;
			os[3] = NULL;
		}
		if (os[0] == NULL && os[2] == NULL) {
			os[0] = os[1];
			os[2] = os[3];
			os[1] = NULL;
			os[3] = NULL;
		}
	} else if (slot_count == 9) {
		if (!os[3] && !os[4] && !os[5]) {
			memmove(os + 3, os + 6, 3 * sizeof(struct slot*));
			memset(os + 6, 0, 3 * sizeof(struct slot*));
		}
		if (!os[0] && !os[1] && !os[2]) {
			memmove(os, os + 3, 6 * sizeof(struct slot*));
			memset(os + 6, 0, 3 * sizeof(struct slot*));
		}
		if (!os[0] && !os[3] && !os[6]) {
			os[0] = os[1];
			os[3] = os[4];
			os[6] = os[7];
			os[1] = os[2];
			os[4] = os[5];
			os[7] = os[8];
			os[2] = NULL;
			os[5] = NULL;
			os[8] = NULL;
			if (!os[0] && !os[3] && !os[6]) {
				os[0] = os[1];
				os[3] = os[4];
				os[6] = os[7];
				os[1] = NULL;
				os[4] = NULL;
				os[7] = NULL;
			}
		}
	}
//for (int x = 0; x < slot_count; x++) {
//	printf("post %i = %i\n", x, os[x] != NULL);
//}
	for (size_t i = 0; i < crafting_recipies->size; i++) {
		struct crafting_recipe* cr = (struct crafting_recipe*) crafting_recipies->data[i];
		if (cr == NULL || (cr->width > 2 && slot_count <= 4) || (slot_count == 4 && (cr->slot[6] != NULL || cr->slot[7] != NULL || cr->slot[8] != NULL))) continue;
		if (cr->shapeless) { // TODO: optimize
			int gs = 1;
			for (int ls = 0; ls <= slot_count; ls++) {
				struct slot* iv = slots[ls];
				if (iv == NULL) continue;
				int m = 0;
				for (int ri = 0; ri < 9; ri++) {
					struct slot* cv = cr->slot[ri];
					if (itemsStackable2(cv, iv)) {
						m = 1;
						break;
					}
				}
				if (!m) {
					gs = 0;
					break;
				}
			}
			if (gs) for (int ri = 0; ri < 9; ri++) {
				struct slot* cv = cr->slot[ri];
				if (cv == NULL) continue;
				int m = 0;
				for (int ls = 1; ls <= 4; ls++) {
					struct slot* iv = slots[ls];
					if (itemsStackable2(cv, iv)) {
						m = 1;
						break;
					}
				}
				if (!m) {
					gs = 0;
					break;
				}
			}
			if (gs) {
				return &cr->output;
			}
		} else {
			int gs = 1;
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					struct slot* cri = cr->slot[y * 3 + x];
					struct slot* oi = (slot_count == 4 && (x > 1 || y > 1)) ? NULL : os[y * (slot_count == 4 ? 2 : 3) + x];
					if (!itemsStackable2(cri, oi) && (cri != oi)) {
						gs = 0;
						goto ppbx;
					}
				}
			}
			ppbx: ;
			if (!gs) {
				gs = 1;
				for (int x = 0; x < 3; x++) {
					for (int y = 0; y < 3; y++) {
						struct slot* cri = x >= cr->width ? NULL : cr->slot[y * 3 + ((cr->width - 1) - x)];
						struct slot* oi = (slot_count == 4 && (x > 1 || y > 1)) ? NULL : os[y * (slot_count == 4 ? 2 : 3) + x];
						if (!itemsStackable2(cri, oi) && (cri != oi)) {
							gs = 0;
							goto pbx;
						}
					}
				}
			}
			pbx: ;
			if (gs) {
				return &cr->output;
			}
		}
	}
	return NULL;
}

void onInventoryUpdate(struct player* player, struct inventory* inv, int slot) {
	if (inv->type == INVTYPE_PLAYERINVENTORY) {
		if (slot == player->currentItem + 36) {
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			pkt->data.play_client.entityequipment.slot = 0;
			duplicateSlot(getSlot(player, inv, player->currentItem + 36), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (slot >= 5 && slot <= 8) {
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			if (slot == 5) pkt->data.play_client.entityequipment.slot = 5;
			else if (slot == 6) pkt->data.play_client.entityequipment.slot = 4;
			else if (slot == 7) pkt->data.play_client.entityequipment.slot = 3;
			else if (slot == 8) pkt->data.play_client.entityequipment.slot = 2;
			duplicateSlot(getSlot(player, inv, slot), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (slot == 45) {
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			pkt->data.play_client.entityequipment.slot = 1;
			duplicateSlot(getSlot(player, inv, 45), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (slot >= 1 && slot <= 4) {
			setSlot(player, inv, 0, getCraftResult(&inv->slots[1], 4), 1, 1);
		}
	} else if (inv->type == INVTYPE_WORKBENCH) {
		if (slot >= 1 && slot <= 9) {
			setSlot(player, inv, 0, getCraftResult(&inv->slots[1], 9), 1, 1);
		}
	} else if (inv->type == INVTYPE_FURNACE) {
		if (slot >= 0 && slot <= 2 && player != NULL) {
			update_furnace(player->world, inv->te);
		}
	}
}

float randFloat() {
	return ((float) rand() / (float) RAND_MAX);
}

void dropBlockDrop(struct world* world, struct slot* slot, int32_t x, int32_t y, int32_t z) {
	struct entity* item = newEntity(nextEntityID++, (double) x + .5, (double) y + .5, (double) z + .5, ENT_ITEMSTACK, randFloat() * 360., 0.);
	item->data.itemstack.slot = xmalloc(sizeof(struct slot));
	item->objectData = 1;
	item->motX = randFloat() * .2 - .1;
	item->motY = .2;
	item->motZ = randFloat() * .2 - .1;
	item->data.itemstack.delayBeforeCanPickup = 0;
	duplicateSlot(slot, item->data.itemstack.slot);
	spawnEntity(world, item);
	BEGIN_BROADCAST_DIST(item, 128.)
	loadEntity(bc_player, item);
	END_BROADCAST
}

void dropPlayerItem(struct player* player, struct slot* drop) {
	struct entity* item = newEntity(nextEntityID++, player->entity->x, player->entity->y + 1.32, player->entity->z, ENT_ITEMSTACK, 0., 0.);
	item->data.itemstack.slot = xmalloc(sizeof(struct slot));
	item->objectData = 1;
	item->motX = -sin(player->entity->yaw * M_PI / 180.) * cos(player->entity->pitch * M_PI / 180.) * .3;
	item->motZ = cos(player->entity->yaw * M_PI / 180.) * cos(player->entity->pitch * M_PI / 180.) * .3;
	item->motY = -sin(player->entity->pitch * M_PI / 180.) * .3 + .1;
	float nos = randFloat() * M_PI * 2.;
	float mag = .02 * randFloat();
	item->motX += cos(nos) * mag;
	item->motY += (randFloat() - randFloat()) * .1;
	item->motZ += sin(nos) * mag;
	item->data.itemstack.delayBeforeCanPickup = 20;
	duplicateSlot(drop, item->data.itemstack.slot);
	spawnEntity(player->world, item);
	BEGIN_BROADCAST_DIST(player->entity, 128.)
	loadEntity(bc_player, item);
	END_BROADCAST
}

void playSound(struct world* world, int32_t soundID, int32_t soundCategory, float x, float y, float z, float volume, float pitch) {
	BEGIN_BROADCAST_DISTXYZ(x, y, z, world->players, 64.)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_SOUNDEFFECT;
	pkt->data.play_client.soundeffect.sound_id = 316;
	pkt->data.play_client.soundeffect.sound_category = 8; //?
	pkt->data.play_client.soundeffect.effect_position_x = (int32_t)(x * 8.);
	pkt->data.play_client.soundeffect.effect_position_y = (int32_t)(y * 8.);
	pkt->data.play_client.soundeffect.effect_position_z = (int32_t)(z * 8.);
	pkt->data.play_client.soundeffect.volume = 1.;
	pkt->data.play_client.soundeffect.pitch = 1.;
	add_queue(bc_player->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST
}

void dropPlayerItem_explode(struct player* player, struct slot* drop) {
	struct entity* item = newEntity(nextEntityID++, player->entity->x, player->entity->y + 1.32, player->entity->z, ENT_ITEMSTACK, 0., 0.);
	item->data.itemstack.slot = xmalloc(sizeof(struct slot));
	item->objectData = 1;
	float f1 = randFloat() * .5;
	float f2 = randFloat() * M_PI * 2.;
	item->motX = -sin(f2) * f1;
	item->motZ = cos(f2) * f1;
	item->motY = .2;
	item->data.itemstack.delayBeforeCanPickup = 20;
	duplicateSlot(drop, item->data.itemstack.slot);
	spawnEntity(player->world, item);
	BEGIN_BROADCAST_DIST(player->entity, 128.)
	loadEntity(bc_player, item);
	END_BROADCAST
}

void dropBlockDrops(struct world* world, block blk, struct player* breaker, int32_t x, int32_t y, int32_t z) {
	struct block_info* bi = getBlockInfo(blk);
	int badtool = !bi->material->requiresnotool;
	if (badtool) {
		struct slot* ci = getSlot(breaker, breaker->inventory, 36 + breaker->currentItem);
		if (ci != NULL) {
			struct item_info* ii = getItemInfo(ci->item);
			if (ii != NULL) badtool = !isToolProficient(ii->toolType, ii->harvestLevel, blk);
		}
	}
	if (badtool) return;
	if (bi->dropItems == NULL) {
		if (bi->drop <= 0 || bi->drop_max == 0 || bi->drop_max < bi->drop_min) return;
		struct slot dd;
		dd.item = bi->drop;
		dd.damage = bi->drop_damage;
		dd.itemCount = bi->drop_min + ((bi->drop_max == bi->drop_min) ? 0 : (rand() % (bi->drop_max - bi->drop_min)));
		dd.nbt = NULL;
		dropBlockDrop(world, &dd, x, y, z);
	} else (*bi->dropItems)(world, blk, x, y, z, 0);
}

void player_openInventory(struct player* player, struct inventory* inv) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_OPENWINDOW;
	pkt->data.play_client.openwindow.window_id = inv->windowID;
	char* type = "";
	if (inv->type == INVTYPE_WORKBENCH) {
		type = "minecraft:crafting_table";
	} else if (inv->type == INVTYPE_CHEST) {
		type = "minecraft:chest";
	} else if (inv->type == INVTYPE_FURNACE) {
		type = "minecraft:furnace";
	}
	pkt->data.play_client.openwindow.window_type = xstrdup(type, 0);
	pkt->data.play_client.openwindow.window_title = xstrdup(inv->title, 0);
	pkt->data.play_client.openwindow.number_of_slots = inv->type == INVTYPE_WORKBENCH ? 0 : inv->slot_count;
	if (inv->type == INVTYPE_HORSE) pkt->data.play_client.openwindow.entity_id = 0; //TODO
	add_queue(player->outgoingPacket, pkt);
	flush_outgoing(player);
	player->openInv = inv;
	add_collection(inv->players, player);
	if (inv->slot_count > 0) {
		pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_WINDOWITEMS;
		pkt->data.play_client.windowitems.window_id = inv->windowID;
		pkt->data.play_client.windowitems.count = inv->slot_count;
		pkt->data.play_client.windowitems.slot_data = xmalloc(sizeof(struct slot) * inv->slot_count);
		for (size_t i = 0; i < inv->slot_count; i++) {
			duplicateSlot(inv->slots[i], &pkt->data.play_client.windowitems.slot_data[i]);
		}
		add_queue(player->outgoingPacket, pkt);
		flush_outgoing(player);
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
			flush_outgoing (bc_player);
			END_BROADCAST xfree(rs);
		}
	} else if (inp->id == PKT_PLAY_SERVER_PLAYER) {
		player->tps++;
		player->entity->lx = player->entity->x;
		player->entity->ly = player->entity->y;
		player->entity->lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		player->entity->onGround = inp->data.play_server.player.on_ground;
		BEGIN_BROADCAST(player->entity->loadingPlayers)
		sendEntityMove(bc_player, player->entity);
		END_BROADCAST
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERPOSITION) {
		player->tps++;
		double lx = player->entity->x;
		double ly = player->entity->y;
		double lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		if (moveEntity(player->entity, inp->data.play_server.playerposition.x - lx, inp->data.play_server.playerposition.feet_y - ly, inp->data.play_server.playerposition.z - lz)) {
			teleportPlayer(player, lx, ly, lz);
		} else {
			player->entity->lx = lx;
			player->entity->ly = ly;
			player->entity->lz = lz;
		}
		player->entity->onGround = inp->data.play_server.playerposition.on_ground;
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
			END_BROADCAST
		}
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERLOOK) {
		player->tps++;
		player->entity->lx = player->entity->x;
		player->entity->ly = player->entity->y;
		player->entity->lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		player->entity->onGround = inp->data.play_server.playerlook.on_ground;
		player->entity->yaw = inp->data.play_server.playerlook.yaw;
		player->entity->pitch = inp->data.play_server.playerlook.pitch;
		BEGIN_BROADCAST(player->entity->loadingPlayers)
		sendEntityMove(bc_player, player->entity);
		END_BROADCAST
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERPOSITIONANDLOOK) {
		player->tps++;
		double lx = player->entity->x;
		double ly = player->entity->y;
		double lz = player->entity->z;
		player->entity->lyaw = player->entity->yaw;
		player->entity->lpitch = player->entity->pitch;
		if (moveEntity(player->entity, inp->data.play_server.playerpositionandlook.x - lx, inp->data.play_server.playerpositionandlook.feet_y - ly, inp->data.play_server.playerpositionandlook.z - lz)) {
			teleportPlayer(player, lx, ly, lz);
		} else {
			player->entity->lx = lx;
			player->entity->ly = ly;
			player->entity->lz = lz;
		}
		player->entity->onGround = inp->data.play_server.playerpositionandlook.on_ground;
		player->entity->yaw = inp->data.play_server.playerpositionandlook.yaw;
		player->entity->pitch = inp->data.play_server.playerpositionandlook.pitch;
		float dx = player->entity->x - player->entity->lx;
		float dy = player->entity->y - player->entity->ly;
		float dz = player->entity->z - player->entity->lz;
		float d3 = dx * dx + dy * dy + dz * dz;
		if (!player->spawnedIn && d3 > 0.00000001) player->spawnedIn = 1;
		if (d3 > 5. * 5. * 5.) {
			printf("rst2\n");
			teleportPlayer(player, player->entity->lx, player->entity->ly, player->entity->lz);
			printf("Player '%s' attempted to move too fast!\n", player->name);
		} else {
			BEGIN_BROADCAST(player->entity->loadingPlayers)
			sendEntityMove(bc_player, player->entity);
			END_BROADCAST
		}
	} else if (inp->id == PKT_PLAY_SERVER_ANIMATION) {
		player->lastSwing = tick_counter;
		BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_ANIMATION;
		pkt->data.play_client.animation.entity_id = player->entity->id;
		pkt->data.play_client.animation.animation = inp->data.play_server.animation.hand == 0 ? 0 : 3;
		add_queue(bc_player->outgoingPacket, pkt);
		flush_outgoing (bc_player);
		END_BROADCAST
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERDIGGING) {
		if (entity_dist_block(player->entity, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z) > player->reachDistance) goto cont;
		if (inp->data.play_server.playerdigging.status == 0) {
			struct block_info* bi = getBlockInfo(getBlockWorld(player->world, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z));
			if (bi == NULL) goto cont;
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
				flush_outgoing (bc_player);
				END_BROADCAST
			} else if (hard == 0. || player->gamemode == 1) {
				if (player->gamemode == 1) {
					struct slot* slot = getSlot(player, player->inventory, 36 + player->currentItem);
					if (slot != NULL && (slot->item == ITM_SWORDWOOD || slot->item == ITM_SWORDGOLD || slot->item == ITM_SWORDSTONE || slot->item == ITM_SWORDIRON || slot->item == ITM_SWORDDIAMOND)) goto cont;
				}
				block blk = getBlockWorld(player->world, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z);
				setBlockWorld(player->world, 0, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z);
				if (player->gamemode != 1) dropBlockDrops(player->world, blk, player, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z);
				player->digging = -1.;
				memset(&player->digging_position, 0, sizeof(struct encpos));
			}
		} else if ((inp->data.play_server.playerdigging.status == 1 || inp->data.play_server.playerdigging.status == 2) && player->digging > 0.) {
			if (inp->data.play_server.playerdigging.status == 2) {
				block blk = getBlockWorld(player->world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
				if ((player->digging + player->digspeed) >= 1.) {
					setBlockWorld(player->world, 0, player->digging_position.x, player->digging_position.y, player->digging_position.z);
					if (player->gamemode != 1) dropBlockDrops(player->world, blk, player, player->digging_position.x, player->digging_position.y, player->digging_position.z);
				} else {
					setBlockWorld(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
				}
			}
			player->digging = -1.;
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_BLOCKBREAKANIMATION;
			pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
			memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
			memset(&player->digging_position, 0, sizeof(struct encpos));
			pkt->data.play_client.blockbreakanimation.destroy_stage = -1;
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (inp->data.play_server.playerdigging.status == 3) {
			if (player->openInv != NULL) goto cont;
			struct slot* ci = getSlot(player, player->inventory, 36 + player->currentItem);
			if (ci != NULL) {
				dropPlayerItem(player, ci);
				setSlot(player, player->inventory, 36 + player->currentItem, NULL, 1, 1);
			}
		} else if (inp->data.play_server.playerdigging.status == 4) {
			if (player->openInv != NULL) goto cont;
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
		}
	} else if (inp->id == PKT_PLAY_SERVER_CREATIVEINVENTORYACTION) {
		if (player->gamemode == 1) {
			if (inp->data.play_server.creativeinventoryaction.clicked_item.item >= 0 && inp->data.play_server.creativeinventoryaction.slot == -1) {
				dropPlayerItem(player, &inp->data.play_server.creativeinventoryaction.clicked_item);
			} else {
				int16_t sl = inp->data.play_server.creativeinventoryaction.slot;
				setSlot(player, player->inventory, sl, &inp->data.play_server.creativeinventoryaction.clicked_item, 0, 1);
			}
		}
	} else if (inp->id == PKT_PLAY_SERVER_HELDITEMCHANGE) {
		if (inp->data.play_server.helditemchange.slot >= 0 && inp->data.play_server.helditemchange.slot < 9) {
			player->currentItem = inp->data.play_server.helditemchange.slot;
			onInventoryUpdate(player, player->inventory, player->currentItem + 36);
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
		flush_outgoing (bc_player);
		END_BROADCAST
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERBLOCKPLACEMENT) {
		if (player->openInv != NULL) goto cont;
		if (entity_dist_block(player->entity, inp->data.play_server.playerblockplacement.location.x, inp->data.play_server.playerblockplacement.location.y, inp->data.play_server.playerblockplacement.location.z) > player->reachDistance) goto cont;
		struct slot* ci = getSlot(player, player->inventory, 36 + player->currentItem);
		int32_t x = inp->data.play_server.playerblockplacement.location.x;
		int32_t y = inp->data.play_server.playerblockplacement.location.y;
		int32_t z = inp->data.play_server.playerblockplacement.location.z;
		uint8_t face = inp->data.play_server.playerblockplacement.face;
		block b = getBlockWorld(player->world, x, y, z);
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
				if (tbi == NULL) goto cont;
				struct boundingbox pbb;
				int bad = 0;
				for (size_t x = 0; x < player->world->entities->size; x++) {
					struct entity* ent = player->world->entities->data[x];
					if (ent == NULL || entity_distsq_block(ent, x, y, z) > 8. * 8. || !isLiving(ent)) continue;
					getEntityCollision(ent, &pbb);
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
							break;
						}
					}
				}
				if (!bad) {
					if (getBlockInfo(tbb)->onBlockPlaced != NULL) tbb = (*getBlockInfo(tbb)->onBlockPlaced)(player->world, tbb, x, y, z);
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
	} else if (inp->id == PKT_PLAY_SERVER_CLICKWINDOW) {
		struct inventory* inv = NULL;
		if (inp->data.play_server.clickwindow.window_id == 0 && player->openInv == NULL) inv = player->inventory;
		else if (player->openInv != NULL && inp->data.play_server.clickwindow.window_id == player->openInv->windowID) inv = player->openInv;
		if (inv == NULL) goto cont;
		int8_t b = inp->data.play_server.clickwindow.button;
		int16_t act = inp->data.play_server.clickwindow.action_number;
		int32_t mode = inp->data.play_server.clickwindow.mode;
		int16_t slot = inp->data.play_server.clickwindow.slot;
		int s0ic = inv->type == INVTYPE_PLAYERINVENTORY || inv->type == INVTYPE_WORKBENCH;
		//printf("click mode=%i, b=%i, slot=%i\n", mode, b, slot);
		if (slot >= 0) {
			struct slot* invs = getSlot(player, inv, slot);
			if ((mode != 4 && mode != 2 && ((inp->data.play_server.clickwindow.clicked_item.item < 0) != (invs == NULL))) || (invs != NULL && inp->data.play_server.clickwindow.clicked_item.item >= 0 && !(itemsStackable(invs, &inp->data.play_server.clickwindow.clicked_item) && invs->itemCount == inp->data.play_server.clickwindow.clicked_item.itemCount))) {
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
					flush_outgoing(player);
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
				flush_outgoing(player);
				pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_SETSLOT;
				pkt->data.play_client.setslot.window_id = -1;
				pkt->data.play_client.setslot.slot = -1;
				duplicateSlot(player->inHand, &pkt->data.play_client.setslot.slot_data);
				add_queue(player->outgoingPacket, pkt);
				flush_outgoing(player);
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
					if (b == 1 || b == 5) {
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
						/*if (player->inHand->itemCount < mss) {
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
						 }*/
					}
				}
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_CONFIRMTRANSACTION;
				pkt->data.play_client.confirmtransaction.window_id = inv->windowID;
				pkt->data.play_client.confirmtransaction.action_number = act;
				pkt->data.play_client.confirmtransaction.accepted = 1;
				add_queue(player->outgoingPacket, pkt);
				flush_outgoing(player);
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
				if (b == 0 || b == 4) {
					memset(inv->dragSlot, 0, inv->slot_count * 2);
					inv->dragSlot_count = 0;
				} else if (b == 2 && inv->dragSlot_count > 0) {
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
				} else if (b == 6 && inv->dragSlot_count > 0) {
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
				}
			}
		}
	} else if (inp->id == PKT_PLAY_SERVER_CLOSEWINDOW) {
		player_closeWindow(player, inp->data.play_server.closewindow.window_id);
	} else if (inp->id == PKT_PLAY_SERVER_USEENTITY) {
		if (inp->data.play_server.useentity.type == 1) {
			struct entity* ent = getEntity(player->world, inp->data.play_server.useentity.target);
			if (ent != NULL && ent != player->entity && ent->health > 0. && ent->world == player->world && entity_dist(ent, player->entity) < 4. && (tick_counter - player->lastSwing) >= 3) {
				damageEntityWithItem(ent, player->entity, getSlot(player, player->inventory, 36 + player->currentItem));
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
			flush_outgoing(player);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_RESPAWN;
			pkt->data.play_client.respawn.dimension = player->world->dimension;
			pkt->data.play_client.respawn.difficulty = difficulty;
			pkt->data.play_client.respawn.gamemode = player->gamemode;
			pkt->data.play_client.respawn.level_type = xstrdup(player->world->levelType, 0);
			add_queue(player->outgoingPacket, pkt);
			flush_outgoing(player);
			teleportPlayer(player, (double) player->world->spawnpos.x + .5, (double) player->world->spawnpos.y, (double) player->world->spawnpos.z + .5); // TODO: make overworld
			setPlayerGamemode(player, -1);
		}
	}
	cont: ;
}

/*
 struct timespec ts;
 clock_gettime(CLOCK_MONOTONIC, &ts);
 double start = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
 clock_gettime(CLOCK_MONOTONIC, &ts);
 double end = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
 printf("chunk section load took %f ms\n", end - start);

 */

void tick_player(struct world* world, struct player* player) {
	if (player->defunct) {
		despawnPlayer(player->world, player);
		freeEntity(player->entity);
		freePlayer(player);
		return;
	}
	if (tick_counter % 200 == 0) {
		if (player->nextKeepAlive != 0) {
			player->conn->disconnect = 1;
			flush_outgoing(player);
			return;
		}
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_KEEPALIVE;
		pkt->data.play_client.keepalive.keep_alive_id = rand();
		add_queue(player->outgoingPacket, pkt);
		flush_outgoing(player);
	} else if (tick_counter % 200 == 100) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_TIMEUPDATE;
		pkt->data.play_client.timeupdate.time_of_day = player->world->time;
		pkt->data.play_client.timeupdate.world_age = player->world->age;
		add_queue(player->outgoingPacket, pkt);
		flush_outgoing(player);
	}
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	double ct = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
	if (player->lastTPSCalculation + 1000. <= ct) {
		float time = (float) (ct - player->lastTPSCalculation) / 1000.;
		player->lastTPSCalculation = ct;
		if ((float) player->tps / 20. > (time + .25)) {
			kickPlayer(player, "Ticks per second Too high! (FastHeal, Teleport, or Lag?)");
			return;
		}
		player->tps = 0;
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
	int32_t pcx = ((int32_t) player->entity->x >> 4);
	int32_t pcz = ((int32_t) player->entity->z >> 4);
	if (((int32_t) player->entity->lx >> 4) != pcx || ((int32_t) player->entity->lz >> 4) != pcz || player->loadedChunks->count < CHUNK_VIEW_DISTANCE * CHUNK_VIEW_DISTANCE * 4 || player->triggerRechunk) {
		player->triggerRechunk = 0;
		int mcl = 0;
		for (int r = 0; r <= CHUNK_VIEW_DISTANCE; r++) {
			int32_t x = pcx - r;
			int32_t z = pcz - r;
			for (int i = 0; i < ((r == 0) ? 1 : (r * 8)); i++) {
				struct chunk* ch = getChunk(player->world, x, z);
				if (ch != NULL && !contains_collection(player->loadedChunks, ch)) {
					ch->playersLoaded++;
					add_collection(player->loadedChunks, ch);
					struct packet* pkt = xmalloc(sizeof(struct packet));
					pkt->id = PKT_PLAY_CLIENT_CHUNKDATA;
					pkt->data.play_client.chunkdata.data = ch;
					pkt->data.play_client.chunkdata.cx = ch->x;
					pkt->data.play_client.chunkdata.cz = ch->z;
					pkt->data.play_client.chunkdata.ground_up_continuous = 1;
					pkt->data.play_client.chunkdata.number_of_block_entities = ch->tileEntities->count;
					pkt->data.play_client.chunkdata.block_entities = xmalloc(sizeof(struct nbt_tag*) * ch->tileEntities->count);
					size_t ri = 0;
					for (size_t i = 0; i < ch->tileEntities->size; i++) {
						if (ch->tileEntities->data[i] == NULL) continue;
						struct tile_entity* te = ch->tileEntities->data[i];
						pkt->data.play_client.chunkdata.block_entities[ri++] = serializeTileEntity(te, 1);
					}
					add_queue(player->outgoingPacket, pkt);
					flush_outgoing(player);
					if (mcl++ > 5) goto xn;
				}
				if (i < 2 * r) x++;
				else if (i < 4 * r) z++;
				else if (i < 6 * r) x--;
				else if (i < 8 * r) z--;
			}
		}
		/*for (int x = -CHUNK_VIEW_DISTANCE + pcx; x <= CHUNK_VIEW_DISTANCE + pcx; x++) {
		 for (int z = -CHUNK_VIEW_DISTANCE + pcz; z <= CHUNK_VIEW_DISTANCE + pcz; z++) {
		 struct chunk* ch = getChunk(player->world, x, z);
		 if (ch != NULL && !contains_collection(player->loadedChunks, ch)) {
		 ch->playersLoaded++;
		 add_collection(player->loadedChunks, ch);
		 struct packet* pkt = xmalloc(sizeof(struct packet));
		 pkt->id = PKT_PLAY_CLIENT_CHUNKDATA;
		 pkt->data.play_client.chunkdata.data = ch;
		 pkt->data.play_client.chunkdata.cx = ch->x;
		 pkt->data.play_client.chunkdata.cz = ch->z;
		 pkt->data.play_client.chunkdata.ground_up_continuous = 1;
		 pkt->data.play_client.chunkdata.number_of_block_entities = ch->tileEntities->count;
		 pkt->data.play_client.chunkdata.block_entities = xmalloc(sizeof(struct nbt_tag*) * ch->tileEntities->count);
		 size_t ri = 0;
		 for (size_t i = 0; i < ch->tileEntities->size; i++) {
		 if (ch->tileEntities->data[i] == NULL) continue;
		 struct tile_entity* te = ch->tileEntities->data[i];
		 pkt->data.play_client.chunkdata.block_entities[ri++] = serializeTileEntity(te, 1);
		 }
		 add_queue(player->outgoingPacket, pkt);
		 flush_outgoing(player);
		 if (mcl++ > 5) goto xn;
		 }
		 }
		 }*/
		xn: ;
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
				add_queue(player->outgoingPacket, pkt);
				flush_outgoing(player);
				player->loadedChunks->data[i] = NULL;
				player->loadedChunks->count--;
				if (--ch->playersLoaded <= 0) {
					unloadChunk(player->world, ch);
				}
			}
		}
	}
	if (player->digging >= 0.) {
		block bw = getBlockWorld(world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
		struct block_info* bi = getBlockInfo(bw);
		float digspeed = 0.;
		if (bi->hardness > 0.) {
			struct slot* ci = getSlot(player, player->inventory, 36 + player->currentItem);
			struct item_info* cii = ci == NULL ? NULL : getItemInfo(ci->item);
			int hasProperTool = (cii == NULL ? 0 : isToolProficient(cii->toolType, cii->harvestLevel, bw));
			int hasProperTool2 = (cii == NULL ? 0 : isToolProficient(cii->toolType, 0xFF, bw));
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
		flush_outgoing (bc_player);
		END_BROADCAST
	}
	for (size_t i = 0; i < player->loadedEntities->size; i++) {
		struct entity* ent = (struct entity*) player->loadedEntities->data[i];
		if (ent == NULL) continue;
		if (entity_distsq(ent, player->entity) > 16384.) {
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_DESTROYENTITIES;
			pkt->data.play_client.destroyentities.count = 1;
			pkt->data.play_client.destroyentities.entity_ids = xmalloc(sizeof(int32_t));
			pkt->data.play_client.destroyentities.entity_ids[0] = ent->id;
			add_queue(player->outgoingPacket, pkt);
			flush_outgoing(player);
			rem_collection(player->loadedEntities, ent);
			rem_collection(ent->loadingPlayers, player);
		}
		if (ent->type != ENT_PLAYER) sendEntityMove(player, ent);
	}
	for (size_t i = 0; i < player->world->entities->size; i++) {
		struct entity* ent = (struct entity*) player->world->entities->data[i];
		if (ent == NULL || ent == player->entity || entity_distsq(ent, player->entity) > 16384. || contains_collection(player->loadedEntities, ent)) continue;
		if (ent->type == ENT_PLAYER) loadPlayer(player, ent->data.player.player);
		else loadEntity(player, ent);
	}
	//printf("%i\n", player->loadedChunks->size);
	if (player->spawnedIn && !player->entity->inWater && !player->entity->inLava && (player->llTick + 5 < tick_counter) && player->gamemode != 1 && player->entity->health > 0.) {
		int nrog = player_onGround(player);
		float dy = player->entity->ly - player->entity->y;
		if (dy >= 0.) {
			if (!player->real_onGround) player->offGroundTime++;
			else player->offGroundTime = 0;
		} else player->offGroundTime = 0.;
		float edy = .0668715 * (float) (player->offGroundTime - .5) + .0357839;
		//printf("dy = %f ogt = %i edy = %f, %f, %f\n", dy, player->offGroundTime, pedy, edy, nedy);
		if (dy >= 0.) {
			float edd = fabs(edy - dy);
			if (!nrog && !player->real_onGround) {
				if (edd > .1) {
					player->flightInfraction += 2;
					//printf("slowfall\n");
				}
			}
		}
		if (dy < player->ldy && !player->real_onGround) { // todo: cobwebs
			if (fabs(dy + .42) < .001 && player->real_onGround) {
				player->lastJump = tick_counter;
				//printf("jump\n");
			} else {
				if (!nrog) {
					//printf("inf\n");
					player->flightInfraction += 2;
				} // else printf("nrog\n");

			}
		}
		player->real_onGround = nrog;
		player->ldy = dy;
		if (player->real_onGround != player->entity->onGround) {
			player->flightInfraction += 2;
			//printf("nog %i %i\n", player->real_onGround, player->entity->onGround);
		}
		if (player->flightInfraction > 0) if (player->flightInfraction-- > 25) {
			kickPlayer(player, "Flying is not enabled on this server");
		}
	}
}

int32_t floor_int(double f) {
	return (int32_t) floor(f);
}

void tick_itemstack(struct world* world, struct entity* entity) {
	if (entity->data.itemstack.delayBeforeCanPickup > 0) {
		entity->data.itemstack.delayBeforeCanPickup--;
		return;
	}
	if (entity->age >= 6000) {
		despawnEntity(world, entity);
		freeEntity(entity);
	}
	if (tick_counter % 10 != 0) return;
	struct boundingbox cebb;
	getEntityCollision(entity, &cebb);
	cebb.minX -= .625;
	cebb.maxX += .625;
	cebb.maxY += .75;
	cebb.minZ -= .625;
	cebb.maxZ += .625;
	struct boundingbox oebb;
	for (size_t i = 0; i < world->entities->size; i++) {
		struct entity* oe = (struct entity*) world->entities->data[i];
		if (oe == NULL || oe == entity || entity_distsq(entity, oe) > 16. * 16.) continue;
		if (oe->type == ENT_PLAYER && oe->health > 0.) {
			getEntityCollision(oe, &oebb);
			if (boundingbox_intersects(&oebb, &cebb)) {
				int os = entity->data.itemstack.slot->itemCount;
				int r = addInventoryItem_PI(oe->data.player.player, oe->data.player.player->inventory, entity->data.itemstack.slot, 1);
				if (r <= 0) {
					BEGIN_BROADCAST_DIST(entity, 32.)
					struct packet* pkt = xmalloc(sizeof(struct packet));
					pkt->id = PKT_PLAY_CLIENT_COLLECTITEM;
					pkt->data.play_client.collectitem.collected_entity_id = entity->id;
					pkt->data.play_client.collectitem.collector_entity_id = oe->id;
					pkt->data.play_client.collectitem.pickup_item_count = os - r;
					add_queue(bc_player->outgoingPacket, pkt);
					flush_outgoing (bc_player);
					END_BROADCAST despawnEntity(world, entity);
					freeEntity(entity);
				} else {
					BEGIN_BROADCAST_DIST(entity, 128.)
					struct packet* pkt = xmalloc(sizeof(struct packet));
					pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
					pkt->data.play_client.entitymetadata.entity_id = entity->id;
					writeMetadata(entity, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
					add_queue(bc_player->outgoingPacket, pkt);
					flush_outgoing (bc_player);
					END_BROADCAST
				}
				break;
			}
		} else if (oe->type == ENT_ITEMSTACK) {
			if (oe->data.itemstack.slot->item == entity->data.itemstack.slot->item && oe->data.itemstack.slot->damage == entity->data.itemstack.slot->damage && oe->data.itemstack.slot->itemCount + entity->data.itemstack.slot->itemCount <= maxStackSize(entity->data.itemstack.slot)) {
				getEntityCollision(oe, &oebb);
				oebb.minX -= .625;
				oebb.maxX += .625;
				cebb.maxY += .75;
				oebb.minZ -= .625;
				oebb.maxZ += .625;
				if (boundingbox_intersects(&oebb, &cebb)) {
					despawnEntity(world, oe);
					entity->data.itemstack.slot->itemCount += oe->data.itemstack.slot->itemCount;
					freeEntity(oe);
					BEGIN_BROADCAST_DIST(entity, 128.)
					struct packet* pkt = xmalloc(sizeof(struct packet));
					pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
					pkt->data.play_client.entitymetadata.entity_id = entity->id;
					writeMetadata(entity, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
					add_queue(bc_player->outgoingPacket, pkt);
					flush_outgoing (bc_player);
					END_BROADCAST
				}
			}
		}
	}
}

void tick_entity(struct world* world, struct entity* entity) {
	entity->age++;
	if (entity->invincibilityTicks > 0) entity->invincibilityTicks--;
	if (entity->type > ENT_PLAYER) {
		double friction_modifier = .98;
		if (entity->type == ENT_ITEMSTACK) entity->motY -= 0.04;
		moveEntity(entity, entity->motX, entity->motY, entity->motZ);
		double friction = .98; // todo per block
		if (entity->onGround) {
			struct block_info* bi = getBlockInfo(getBlockWorld(entity->world, floor_int(entity->x), floor_int(entity->y) - 1, floor_int(entity->z)));
			if (bi != NULL) friction = bi->slipperiness * friction_modifier;
		}
		entity->motX *= friction;
		entity->motY *= .98;
		entity->motZ *= friction;
		if (entity->onGround) entity->motY *= -.5;
	}
	if (entity->type == ENT_ITEMSTACK) tick_itemstack(world, entity);
	if (isLiving(entity->type)) {
		entity->inWater = entity_inBlock(entity, BLK_WATER, .4, 0) || entity_inBlock(entity, BLK_WATER_1, .4, 0);
		entity->inLava = entity_inBlock(entity, BLK_LAVA, .4, 0) || entity_inBlock(entity, BLK_LAVA_1, .4, 0);
		if ((entity->inWater || entity->inLava) && entity->type == ENT_PLAYER) entity->data.player.player->llTick = tick_counter;
		//TODO: ladders
		if (entity->type == ENT_PLAYER && entity->data.player.player->gamemode == 1) goto bfd;
		if (entity->inLava) {
			damageEntity(entity, 4., 1);
			//TODO: fire
		}
		if (!entity->onGround) {
			float dy = entity->ly - entity->y;
			if (dy > 0.) {
				entity->fallDistance += dy;
			}
		} else if (entity->fallDistance > 0.) {
			if (entity->fallDistance > 3. && !entity->inWater && !entity->inLava) {
				damageEntity(entity, entity->fallDistance - 3., 0);
				playSound(entity->world, 312, 8, entity->x, entity->y, entity->z, 1., 1.);
			}
			entity->fallDistance = 0.;
		}
		bfd: ;
	}
	if (entity->type != ENT_PLAYER) {
		entity->lx = entity->x;
		entity->ly = entity->y;
		entity->lz = entity->z;
		entity->lyaw = entity->yaw;
		entity->lpitch = entity->pitch;
	}
}

void tick_world(struct world* world) { //TODO: separate tick threads
	if (++world->time >= 24000) world->time = 0;
	world->age++;
	for (size_t i = 0; i < world->players->size; i++) {
		struct player* player = (struct player*) world->players->data[i];
		if (player == NULL) continue;
		struct packet* wp = pop_nowait_queue(player->incomingPacket);
		while (wp != NULL) {
			player_receive_packet(player, wp);
			freePacket(STATE_PLAY, 0, wp);
			xfree(wp);
			wp = pop_nowait_queue(player->incomingPacket);
		}
	}
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
	for (size_t i = 0; i < world->loadedChunks->size; i++) {
		struct chunk* chunk = (struct chunk*) world->loadedChunks->data[i];
		if (chunk == NULL) continue;
		for (size_t x = 0; x < chunk->tileEntitiesTickable->size; x++) {
			struct tile_entity* te = (struct tile_entity*) chunk->tileEntitiesTickable->data[x];
			if (te == NULL) continue;
			(*te->tick)(world, te);
		}
	}
}

void sendMessageToPlayer(struct player* player, char* text) {
	if (player == NULL) {
		printf("%s\n", text);
	} else {
		size_t s = strlen(text) + 512;
		char* rsx = xstrdup(text, 512);
		char* rs = xmalloc(512);
		snprintf(rs, s, "{\"text\": \"%s\"}", replace(replace(rsx, "\\", "\\\\"), "\"", "\\\""));
		//printf("<CHAT> %s\n", text);
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_CHATMESSAGE;
		pkt->data.play_client.chatmessage.position = 0;
		pkt->data.play_client.chatmessage.json_data = xstrdup(rs, 0);
		add_queue(player->outgoingPacket, pkt);
		flush_outgoing(player);
		xfree(rsx);
	}
}

void sendMsgToPlayerf(struct player* player, char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t len = varstrlen(fmt, args);
	char* bct = xmalloc(len);
	vsnprintf(bct, len, fmt, args);
	va_end(args);
	sendMessageToPlayer(player, bct);
	xfree(bct);
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
	add_queue(bc_player->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST xfree(rs);
	xfree(rsx);
}

void broadcastf(char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t len = varstrlen(fmt, args);
	char* bct = xmalloc(len);
	vsnprintf(bct, len, fmt, args);
	va_end(args);
	broadcast(bct);
	xfree(bct);
}
