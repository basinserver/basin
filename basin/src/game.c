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
#include <math.h>
#include "inventory.h"
#include "item.h"
#include "craft.h"

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
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 0;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[from->currentItem + 36], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 5;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[5], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 4;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[6], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 3;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[7], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 2;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[8], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 1;
	duplicateSlot(from->inventory.slots == NULL ? NULL : from->inventory.slots[45], &pkt->data.play_client.entityequipment.item);
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

void craftOnce(struct inventory* inv) {
	int cs = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	for (int i = 1; i <= cs; i++) {
		if (inv->slots[i] == NULL) continue;
		if (--inv->slots[i]->itemCount <= 0) {
			freeSlot(inv->slots[i]);
			xfree(inv->slots[i]);
			inv->slots[i] = NULL;
		}
	}
	onInventoryUpdate(inv, 1); // 2-cs would just repeat the calculation
}

int craftAll(struct inventory* inv) {
	int cs = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	uint8_t ct = 64;
	for (int i = 1; i <= cs; i++) {
		if (inv->slots[i] == NULL) continue;
		if (inv->slots[i]->itemCount < ct) ct = inv->slots[i]->itemCount;
	}
	for (int i = 1; i <= cs; i++) {
		if (inv->slots[i] == NULL) continue;
		inv->slots[i]->itemCount -= ct;
		if (inv->slots[i]->itemCount <= 0) {
			freeSlot(inv->slots[i]);
			xfree(inv->slots[i]);
			inv->slots[i] = NULL;
		}
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
		for (int z = 0; z < 2; z++)
			for (int x = 0; x < 2; x++) {
				if (os[3 * x] == NULL && os[3 * x + 1] == NULL && os[3 * x + 2] == NULL) {
					os[3 * x] = os[3 * (x + 1)];
					os[3 * x + 1] = os[3 * (x + 1) + 1];
					os[3 * x + 2] = os[3 * (x + 1) + 2];
					os[3 * (x + 1)] = NULL;
					os[3 * (x + 1) + 1] = NULL;
					os[3 * (x + 1) + 2] = NULL;
				}
			}
		for (int z = 0; z < 2; z++)
			for (int x = 1; x >= 0; x--) {
				if (os[x] == NULL && os[x + 3] == NULL && os[x + 6] == NULL) {
					os[x] = os[x + 1];
					os[x + 3] = os[x + 4];
					os[x + 6] = os[x + 7];
					os[x + 1] = NULL;
					os[x + 4] = NULL;
					os[x + 7] = NULL;
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
						goto pbx;
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

void onInventoryUpdate(struct inventory* inv, int slot) {
	if (inv->type == INVTYPE_PLAYERINVENTORY) {
		struct player* player = (struct player*) inv->players->data[0];
		if (slot == player->currentItem + 36) {
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			pkt->data.play_client.entityequipment.slot = 0;
			duplicateSlot(getInventorySlot(inv, player->currentItem + 36), &pkt->data.play_client.entityequipment.item);
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
			duplicateSlot(getInventorySlot(inv, slot), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (slot == 45) {
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			pkt->data.play_client.entityequipment.slot = 1;
			duplicateSlot(getInventorySlot(inv, 45), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		} else if (slot >= 1 && slot <= 4) {
			setInventorySlot(inv, getCraftResult(&inv->slots[1], 4), 0);
		}
	} else if (inv->type == INVTYPE_WORKBENCH) {
		if (slot >= 1 && slot <= 9) {
			setInventorySlot(inv, getCraftResult(&inv->slots[1], 9), 0);
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

void dropBlockDrops(struct world* world, block blk, struct player* breaker, int32_t x, int32_t y, int32_t z) {
	if (block_infos[blk].getDrop == NULL) {
		struct slot dd;
		dd.item = blk >> 4;
		dd.damage = blk & 0x0f;
		dd.itemCount = 1;
		dd.nbt = NULL;
		dropBlockDrop(world, &dd, x, y, z);
	} else (*block_infos[blk].getDrop)(world, blk, x, y, z, 0);
}

void player_openInventory(struct player* player, struct inventory* inv) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_OPENWINDOW;
	pkt->data.play_client.openwindow.window_id = inv->windowID;
	char* type = "";
	if (inv->type == INVTYPE_WORKBENCH) {
		type = "minecraft:crafting_table";
	}
	pkt->data.play_client.openwindow.window_type = xstrdup(type, 0);
	pkt->data.play_client.openwindow.window_title = xstrdup(inv->title, 0);
	pkt->data.play_client.openwindow.number_of_slots = inv->type == INVTYPE_WORKBENCH ? 0 : inv->slot_count;
	if (inv->type == INVTYPE_HORSE) pkt->data.play_client.openwindow.entity_id = 0; //TODO
	add_queue(player->outgoingPacket, pkt);
	flush_outgoing(player);
	player->openInv = inv;
	if (inv->slot_count > 0 || inv->type == INVTYPE_WORKBENCH) {
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
	if (inp->id == PKT_PLAY_SERVER_KEEPALIVE) {
		if (inp->data.play_server.keepalive.keep_alive_id == player->nextKeepAlive) {
			player->nextKeepAlive = 0;
		}
	} else if (inp->id == PKT_PLAY_SERVER_CHATMESSAGE) {
		char* msg = inp->data.play_server.chatmessage.message;
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
	} else if (inp->id == PKT_PLAY_SERVER_PLAYER) {
		player->entity->onGround = inp->data.play_server.player.on_ground;
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERPOSITION) {
		player->entity->onGround = inp->data.play_server.playerposition.on_ground;
		player->entity->x = inp->data.play_server.playerposition.x;
		player->entity->y = inp->data.play_server.playerposition.feet_y;
		player->entity->z = inp->data.play_server.playerposition.z;
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERLOOK) {
		player->entity->onGround = inp->data.play_server.playerlook.on_ground;
		player->entity->yaw = inp->data.play_server.playerlook.yaw;
		player->entity->pitch = inp->data.play_server.playerlook.pitch;
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERPOSITIONANDLOOK) {
		player->entity->onGround = inp->data.play_server.playerpositionandlook.on_ground;
		player->entity->x = inp->data.play_server.playerpositionandlook.x;
		player->entity->y = inp->data.play_server.playerpositionandlook.feet_y;
		player->entity->z = inp->data.play_server.playerpositionandlook.z;
		player->entity->yaw = inp->data.play_server.playerpositionandlook.yaw;
		player->entity->pitch = inp->data.play_server.playerpositionandlook.pitch;
	} else if (inp->id == PKT_PLAY_SERVER_ANIMATION) {
		BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_ANIMATION;
		pkt->data.play_client.animation.entity_id = player->entity->id;
		pkt->data.play_client.animation.animation = inp->data.play_server.animation.hand == 0 ? 0 : 3;
		add_queue(bc_player->outgoingPacket, pkt);
		flush_outgoing (bc_player);
		END_BROADCAST
	} else if (inp->id == PKT_PLAY_SERVER_PLAYERDIGGING) {
		if (inp->data.play_server.playerdigging.status == 0) {
			float hard = block_infos[getBlockWorld(player->world, inp->data.play_server.playerdigging.location.x, inp->data.play_server.playerdigging.location.y, inp->data.play_server.playerdigging.location.z)].hardness;
			if (hard > 0. && player->gamemode == 0) {
				player->digging = 0.;
				memcpy(&player->digging_position, &inp->data.play_server.playerdigging.location, sizeof(struct encpos));
				BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_BLOCKBREAKANIMATION;
				pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
				memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
				pkt->data.play_client.blockbreakanimation.destroy_stage = 0;
				add_queue(bc_player->outgoingPacket, pkt);
				flush_outgoing (bc_player);
				END_BROADCAST
			} else if (hard == 0. || player->gamemode == 1) {
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
			struct slot* ci = getInventorySlot(&player->inventory, 36 + player->currentItem);
			if (ci != NULL) {
				dropPlayerItem(player, ci);
				setInventorySlot(&player->inventory, NULL, 36 + player->currentItem);
			}
		} else if (inp->data.play_server.playerdigging.status == 4) {
			if (player->openInv != NULL) goto cont;
			struct slot* ci = getInventorySlot(&player->inventory, 36 + player->currentItem);
			if (ci != NULL) {
				uint8_t ss = ci->itemCount;
				ci->itemCount = 1;
				dropPlayerItem(player, ci);
				ci->itemCount = ss;
				if (--ci->itemCount == 0) {
					ci = NULL;
				}
				setInventorySlot(&player->inventory, ci, 36 + player->currentItem);
			}
		}
	} else if (inp->id == PKT_PLAY_SERVER_CREATIVEINVENTORYACTION) {
		if (player->gamemode == 1) {
			if (inp->data.play_server.creativeinventoryaction.clicked_item.item >= 0 && inp->data.play_server.creativeinventoryaction.slot == -1) {
				dropPlayerItem(player, &inp->data.play_server.creativeinventoryaction.clicked_item);
			} else {
				int16_t sl = inp->data.play_server.creativeinventoryaction.slot;
				setInventorySlot(&player->inventory, &inp->data.play_server.creativeinventoryaction.clicked_item, sl);
			}
		}
	} else if (inp->id == PKT_PLAY_SERVER_HELDITEMCHANGE) {
		if (inp->data.play_server.helditemchange.slot >= 0 && inp->data.play_server.helditemchange.slot < 9) {
			player->currentItem = inp->data.play_server.helditemchange.slot;
			onInventoryUpdate(&player->inventory, player->currentItem + 36);
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
		struct slot* ci = getInventorySlot(&player->inventory, 36 + player->currentItem);
		int32_t x = inp->data.play_server.playerblockplacement.location.x;
		int32_t y = inp->data.play_server.playerblockplacement.location.y;
		int32_t z = inp->data.play_server.playerblockplacement.location.z;
		uint8_t face = inp->data.play_server.playerblockplacement.face;
		block b = getBlockWorld(player->world, x, y, z);
		if (!player->entity->sneaking && block_infos[b].onBlockInteract != NULL) {
			(*block_infos[b].onBlockInteract)(player->world, b, x, y, z, player, face, inp->data.play_server.playerblockplacement.cursor_position_x, inp->data.play_server.playerblockplacement.cursor_position_y, inp->data.play_server.playerblockplacement.cursor_position_z);
		} else if (ci != NULL && ci->item < 256) {
			if (!block_materials[block_infos[b].material].replacable) {
				if (face == 0) y--;
				else if (face == 1) y++;
				else if (face == 2) z--;
				else if (face == 3) z++;
				else if (face == 4) x--;
				else if (face == 5) x++;
			}
			setBlockWorld(player->world, (ci->item << 4) | (ci->damage & 0x0f), x, y, z);
			if (player->gamemode != 1) {
				if (--ci->itemCount <= 0) {
					ci = NULL;
				}
				setInventorySlot(&player->inventory, ci, 36 + player->currentItem);
			}
		}
	} else if (inp->id == PKT_PLAY_SERVER_CLICKWINDOW) {
		struct inventory* inv = NULL;
		if (inp->data.play_server.clickwindow.window_id == 0 && player->openInv == NULL) inv = &player->inventory;
		else if (player->openInv != NULL && inp->data.play_server.clickwindow.window_id == player->openInv->windowID) inv = player->openInv;
		int8_t b = inp->data.play_server.clickwindow.button;
		int16_t act = inp->data.play_server.clickwindow.action_number;
		int32_t mode = inp->data.play_server.clickwindow.mode;
		int16_t slot = inp->data.play_server.clickwindow.slot;
		int s0ic = inv->type == INVTYPE_PLAYERINVENTORY || inv->type == INVTYPE_WORKBENCH;
		//printf("click mode=%i, b=%i, slot=%i\n", mode, b, slot);
		if (slot >= 0 && slot < inv->slot_count) {
			if ((mode != 4 && mode != 2 && ((inp->data.play_server.clickwindow.clicked_item.item < 0) != (inv->slots[slot] == NULL))) || (inv->slots[slot] != NULL && inp->data.play_server.clickwindow.clicked_item.item >= 0 && !(itemsStackable(inv->slots[slot], &inp->data.play_server.clickwindow.clicked_item) && inv->slots[slot]->itemCount == inp->data.play_server.clickwindow.clicked_item.itemCount))) {
				printf("desync\n");
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_CONFIRMTRANSACTION;
				pkt->data.play_client.confirmtransaction.window_id = inv->windowID;
				pkt->data.play_client.confirmtransaction.action_number = act;
				pkt->data.play_client.confirmtransaction.accepted = 0;
				add_queue(player->outgoingPacket, pkt);
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
			} else {
				if (mode == 0) {
					if (s0ic && slot == 0) {
						printf("craft\n");
						if (inv->inHand == NULL) {
							inv->inHand = inv->slots[0];
							inv->slots[0] = NULL;
							craftOnce(inv);
						} else if (itemsStackable(inv->inHand, inv->slots[0]) && (inv->inHand->itemCount + inv->slots[0]->itemCount) < maxStackSize(inv->inHand)) {
							inv->inHand->itemCount += inv->slots[0]->itemCount;
							freeSlot(inv->slots[0]);
							xfree(inv->slots[0]);
							inv->slots[0] = NULL;
							craftOnce(inv);
						}
					} else if (b == 0) {
						if (itemsStackable(inv->inHand, inv->slots[slot])) {
							uint8_t os = inv->slots[slot]->itemCount;
							int mss = maxStackSize(inv->inHand);
							inv->slots[slot]->itemCount += inv->inHand->itemCount;
							if (inv->slots[slot]->itemCount > mss) inv->slots[slot]->itemCount = mss;
							int dr = inv->slots[slot]->itemCount - os;
							if (dr >= inv->inHand->itemCount) {
								freeSlot(inv->inHand);
								xfree(inv->inHand);
								inv->inHand = NULL;
							} else {
								inv->inHand->itemCount -= dr;
							}
						} else {
							struct slot* t = inv->slots[slot];
							inv->slots[slot] = inv->inHand;
							inv->inHand = t;
							onInventoryUpdate(inv, slot);
						}
					} else if (b == 1) {
						if (inv->inHand == NULL && inv->slots[slot] != NULL) {
							inv->inHand = xmalloc(sizeof(struct slot));
							duplicateSlot(inv->slots[slot], inv->inHand);
							uint8_t os = inv->slots[slot]->itemCount;
							inv->slots[slot]->itemCount /= 2;
							inv->inHand->itemCount = os - inv->slots[slot]->itemCount;
							onInventoryUpdate(inv, slot);
						} else if (inv->inHand != NULL && (inv->slots[slot] == NULL || (itemsStackable(inv->inHand, inv->slots[slot]) && inv->inHand->itemCount + inv->slots[slot]->itemCount < maxStackSize(inv->inHand)))) {
							if (inv->slots[slot] == NULL) {
								inv->slots[slot] = xmalloc(sizeof(struct slot));
								duplicateSlot(inv->inHand, inv->slots[slot]);
								inv->slots[slot]->itemCount = 1;
							} else {
								inv->slots[slot]->itemCount++;
							}
							if (--inv->inHand->itemCount <= 0) {
								freeSlot(inv->inHand);
								xfree(inv->inHand);
								inv->inHand = NULL;
							}
							onInventoryUpdate(inv, slot);
						}
					}
				} else if (mode == 1) {
					if (inv->type == INVTYPE_PLAYERINVENTORY && inv->slots[slot] != NULL) {
						int16_t it = inv->slots[slot]->item;
						if (slot == 0) {
							int amt = craftAll(inv);
							for (int i = 0; i < amt; i++)
								addInventoryItem(inv, inv->slots[0], 44, 8);
							onInventoryUpdate(inv, 1); // 2-4 would just repeat the calculation
						} else if (slot != 45 && it == ITM_SHIELD && inv->slots[45] == NULL) {
							swapSlots(inv, 45, slot);
						} else if (slot != 5 && inv->slots[5] == NULL && (it == BLK_PUMPKIN || it == ITM_HELMETCLOTH || it == ITM_HELMETCHAIN || it == ITM_HELMETIRON || it == ITM_HELMETDIAMOND || it == ITM_HELMETGOLD)) {
							swapSlots(inv, 5, slot);
						} else if (slot != 6 && inv->slots[6] == NULL && (it == ITM_CHESTPLATECLOTH || it == ITM_CHESTPLATECHAIN || it == ITM_CHESTPLATEIRON || it == ITM_CHESTPLATEDIAMOND || it == ITM_CHESTPLATEGOLD)) {
							swapSlots(inv, 6, slot);
						} else if (slot != 7 && inv->slots[7] == NULL && (it == ITM_LEGGINGSCLOTH || it == ITM_LEGGINGSCHAIN || it == ITM_LEGGINGSIRON || it == ITM_LEGGINGSDIAMOND || it == ITM_LEGGINGSGOLD)) {
							swapSlots(inv, 7, slot);
						} else if (slot != 8 && inv->slots[8] == NULL && (it == ITM_BOOTSCLOTH || it == ITM_BOOTSCHAIN || it == ITM_BOOTSIRON || it == ITM_BOOTSDIAMOND || it == ITM_BOOTSGOLD)) {
							swapSlots(inv, 8, slot);
						} else if (slot > 35 && slot < 45 && inv->slots[slot] != NULL) {
							int r = addInventoryItem(inv, inv->slots[slot], 9, 36);
							if (r <= 0) {
								freeSlot(inv->slots[slot]);
								xfree(inv->slots[slot]);
								inv->slots[slot] = NULL;
							}
						} else if (slot > 8 && slot < 36 && inv->slots[slot] != NULL) {
							int r = addInventoryItem(inv, inv->slots[slot], 36, 45);
							if (r <= 0) {
								freeSlot(inv->slots[slot]);
								xfree(inv->slots[slot]);
								inv->slots[slot] = NULL;
							}
						} else if ((slot == 45 || slot < 9) && inv->slots[slot] != NULL) {
							int r = addInventoryItem(inv, inv->slots[slot], 9, 36);
							if (r <= 0) {
								freeSlot(inv->slots[slot]);
								xfree(inv->slots[slot]);
								inv->slots[slot] = NULL;
							} else {
								r = addInventoryItem(inv, inv->slots[slot], 36, 45);
								if (r <= 0) {
									freeSlot(inv->slots[slot]);
									xfree(inv->slots[slot]);
									inv->slots[slot] = NULL;
								}
							}
						}
					} else if (inv->type == INVTYPE_WORKBENCH) {
						int16_t it = inv->slots[slot]->item;
						if (slot == 0) {
							int amt = craftAll(inv);
							for (int i = 0; i < amt; i++)
								addInventoryItem(inv, inv->slots[0], 45, 9);
							onInventoryUpdate(inv, 1); // 2-4 would just repeat the calculation
						} else if (slot <= 9) {
							int r = addInventoryItem(inv, inv->slots[slot], 10, 46);
							if (r <= 0) {
								freeSlot(inv->slots[slot]);
								xfree(inv->slots[slot]);
								inv->slots[slot] = NULL;
							}
						} else if (slot > 9 && slot < 37 && inv->slots[slot] != NULL) {
							int r = addInventoryItem(inv, inv->slots[slot], 37, 46);
							if (r <= 0) {
								freeSlot(inv->slots[slot]);
								xfree(inv->slots[slot]);
								inv->slots[slot] = NULL;
							}
						} else if (slot > 36 && slot < 46 && inv->slots[slot] != NULL) {
							int r = addInventoryItem(inv, inv->slots[slot], 10, 37);
							if (r <= 0) {
								freeSlot(inv->slots[slot]);
								xfree(inv->slots[slot]);
								inv->slots[slot] = NULL;
							}
						}
					}
				} else if (mode == 2) {
					if (inv->type == INVTYPE_PLAYERINVENTORY) {
						if (slot == 0) {
							if (inv->slots[36 + b] == NULL) {
								inv->slots[36 + b] = inv->slots[0];
								inv->slots[0] = NULL;
								craftOnce(inv);
								onInventoryUpdate(inv, 36 + b);
							}
						} else if (b >= 0 && b <= 8) swapSlots(inv, 36 + b, slot);
					} else if (inv->type == INVTYPE_WORKBENCH) {
						if (slot == 0) {
							if (inv->slots[37 + b] == NULL) {
								inv->slots[37 + b] = inv->slots[0];
								inv->slots[0] = NULL;
								craftOnce(inv);
								onInventoryUpdate(inv, 37 + b);
							}
						} else if (b >= 0 && b <= 8) swapSlots(inv, 37 + b, slot);
					}
				} else if (mode == 3) {
					//middle click, NOP in survival?
				} else if (mode == 4 && inv->slots[slot] != NULL) {
					if (b == 0) {
						uint8_t p = inv->slots[slot]->itemCount;
						inv->slots[slot]->itemCount = 1;
						dropPlayerItem(player, inv->slots[slot]);
						inv->slots[slot]->itemCount = p - 1;
						if (inv->slots[slot]->itemCount == 0) {
							freeSlot(inv->slots[slot]);
							xfree(inv->slots[slot]);
							inv->slots[slot] = NULL;
						}
						onInventoryUpdate(inv, slot);
					} else if (b == 1) {
						dropPlayerItem(player, inv->slots[slot]);
						freeSlot(inv->slots[slot]);
						xfree(inv->slots[slot]);
						inv->slots[slot] = NULL;
						onInventoryUpdate(inv, slot);
					}
					if (s0ic && slot == 0) craftOnce(inv);
				} else if (mode == 5) {
					if (b == 1 || b == 5) {
						int ba = 0;
						if (inv->type == INVTYPE_PLAYERINVENTORY) {
							if (slot == 0 || (slot >= 5 && slot <= 8)) ba = 1;
						}
						if (!ba && inv->dragSlot_count < inv->slot_count && (inv->slots[slot] == NULL || itemsStackable(inv->slots[slot], inv->inHand))) {
							inv->dragSlot[inv->dragSlot_count++] = slot;
						}
					}
				} else if (mode == 6 && inv->inHand != NULL) {
					int mss = maxStackSize(inv->inHand);
					if (inv->inHand->itemCount < mss) {
						for (size_t i = 9; i < 36; i++) {
							if (itemsStackable(inv->inHand, inv->slots[i])) {
								uint8_t oc = inv->inHand->itemCount;
								inv->inHand->itemCount += inv->slots[i]->itemCount;
								if (inv->inHand->itemCount > mss) inv->inHand->itemCount = mss;
								inv->slots[i]->itemCount -= inv->inHand->itemCount - oc;
								if (inv->slots[i]->itemCount <= 0) {
									freeSlot(inv->slots[i]);
									xfree(inv->slots[i]);
									inv->slots[i] = NULL;
								}
								onInventoryUpdate(inv, i);
								if (inv->inHand->itemCount >= mss) break;
							}
						}
					}
					if (inv->inHand->itemCount < mss) {
						for (size_t i = 36; i < 45; i++) {
							if (itemsStackable(inv->inHand, inv->slots[i])) {
								uint8_t oc = inv->inHand->itemCount;
								inv->inHand->itemCount += inv->slots[i]->itemCount;
								if (inv->inHand->itemCount > mss) inv->inHand->itemCount = mss;
								inv->slots[i]->itemCount -= inv->inHand->itemCount - oc;
								if (inv->slots[i]->itemCount <= 0) {
									freeSlot(inv->slots[i]);
									xfree(inv->slots[i]);
									inv->slots[i] = NULL;
								}
								onInventoryUpdate(inv, i);
								if (inv->inHand->itemCount >= mss) break;
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
				flush_outgoing(player);
			}
		} else if (slot == -999) {
			if (mode == 0 && inv->inHand != NULL) {
				if (b == 1) {
					uint8_t p = inv->inHand->itemCount;
					inv->inHand->itemCount = 1;
					dropPlayerItem(player, inv->inHand);
					inv->inHand->itemCount = p - 1;
					if (inv->inHand->itemCount == 0) {
						freeSlot(inv->inHand);
						xfree(inv->inHand);
						inv->inHand = NULL;
					}
				} else if (b == 0) {
					dropPlayerItem(player, inv->inHand);
					freeSlot(inv->inHand);
					xfree(inv->inHand);
					inv->inHand = NULL;
				}
			} else if (mode == 5 && inv->inHand != NULL) {
				if (b == 0 || b == 4) {
					memset(inv->dragSlot, 0, inv->slot_count * 2);
					inv->dragSlot_count = 0;
				} else if (b == 2 && inv->dragSlot_count > 0) {
					uint8_t total = inv->inHand->itemCount;
					uint8_t per = total / inv->dragSlot_count;
					if (per == 0) per = 1;
					for (int i = 0; i < inv->dragSlot_count; i++) {
						int sl = inv->dragSlot[i];
						if (inv->slots[sl] == NULL) {
							inv->slots[sl] = xmalloc(sizeof(struct slot));
							duplicateSlot(inv->inHand, inv->slots[sl]);
							inv->slots[sl]->itemCount = per;
							onInventoryUpdate(inv, sl);
							if (per >= inv->inHand->itemCount) {
								freeSlot(inv->inHand);
								xfree(inv->inHand);
								inv->inHand = NULL;
								break;
							}
							inv->inHand->itemCount -= per;
						} else {
							uint8_t os = inv->slots[sl]->itemCount;
							inv->slots[sl]->itemCount += per;
							int mss = maxStackSize(inv->inHand);
							if (inv->slots[sl]->itemCount > mss) inv->slots[sl]->itemCount = mss;
							onInventoryUpdate(inv, sl);
							if (per >= inv->inHand->itemCount) {
								freeSlot(inv->inHand);
								xfree(inv->inHand);
								inv->inHand = NULL;
								break;
							}
							inv->inHand->itemCount -= inv->slots[sl]->itemCount - os;
						}
					}
				} else if (b == 6 && inv->dragSlot_count > 0) {
					uint8_t per = 1;
					for (int i = 0; i < inv->dragSlot_count; i++) {
						int sl = inv->dragSlot[i];
						if (inv->slots[sl] == NULL) {
							inv->slots[sl] = xmalloc(sizeof(struct slot));
							duplicateSlot(inv->inHand, inv->slots[sl]);
							inv->slots[sl]->itemCount = per;
							onInventoryUpdate(inv, sl);
							if (per >= inv->inHand->itemCount) {
								freeSlot(inv->inHand);
								xfree(inv->inHand);
								inv->inHand = NULL;
								break;
							}
							inv->inHand->itemCount -= per;
						} else {
							uint8_t os = inv->slots[sl]->itemCount;
							inv->slots[sl]->itemCount += per;
							int mss = maxStackSize(inv->inHand);
							if (inv->slots[sl]->itemCount > mss) inv->slots[sl]->itemCount = mss;
							onInventoryUpdate(inv, sl);
							if (per >= inv->inHand->itemCount) {
								freeSlot(inv->inHand);
								xfree(inv->inHand);
								inv->inHand = NULL;
								break;
							}
							inv->inHand->itemCount -= inv->slots[sl]->itemCount - os;
						}
					}
				}
			}
		}
	} else if (inp->id == PKT_PLAY_SERVER_CLOSEWINDOW) {
		struct inventory* inv = NULL;
		if (inp->data.play_server.closewindow.window_id == 0 && player->openInv == NULL) inv = &player->inventory;
		else if (player->openInv != NULL && inp->data.play_server.closewindow.window_id == player->openInv->windowID) inv = player->openInv;
		if (inv != NULL) {
			if (inv->inHand != NULL) {
				dropPlayerItem(player, inv->inHand);
				freeSlot(inv->inHand);
				xfree(inv->inHand);
				inv->inHand = NULL;
			}
			if (inv->type == INVTYPE_PLAYERINVENTORY) {
				for (int i = 1; i < 5; i++) {
					if (inv->slots[i] != NULL) {
						dropPlayerItem(player, inv->slots[i]);
						freeSlot(inv->slots[i]);
						xfree(inv->slots[i]);
						inv->slots[i] = NULL;
					}
				}
				onInventoryUpdate(inv, 1);
			} else if (inv->type == INVTYPE_WORKBENCH) {
				for (int i = 1; i < 10; i++) {
					if (inv->slots[i] != NULL) {
						dropPlayerItem(player, inv->slots[i]);
						freeSlot(inv->slots[i]);
						xfree(inv->slots[i]);
						inv->slots[i] = NULL;
					}
				}
				copyInventory(inv->slots + 10, player->inventory.slots + 9, 36);
				freeInventory(inv);
			}
		}
		player->openInv = NULL; // TODO: free sometimes?
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
	}
	int32_t pcx = ((int32_t) player->entity->x >> 4);
	int32_t pcz = ((int32_t) player->entity->z >> 4);
	if (((int32_t) player->entity->lx >> 4) != pcx || ((int32_t) player->entity->lz >> 4) != pcz || player->loadedChunks->count < CHUNK_VIEW_DISTANCE * CHUNK_VIEW_DISTANCE * 4) {
		int mcl = 0;
		for (int x = -CHUNK_VIEW_DISTANCE + pcx; x <= CHUNK_VIEW_DISTANCE + pcx; x++) {
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
					pkt->data.play_client.chunkdata.number_of_block_entities = 0;
					pkt->data.play_client.chunkdata.block_entities = NULL;
					add_queue(player->outgoingPacket, pkt);
					flush_outgoing(player);
					//printf("client load %i, %i\n", ch->x, ch->z);
					if (mcl++ > 5) goto xn;
				}
			}
		}
		xn: ;
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
				add_queue(player->outgoingPacket, pkt);
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
		double md = entity_distsq_block(ent, ent->lx, ent->ly, ent->lz);
		double mp = (ent->yaw - ent->lyaw) * (ent->yaw - ent->lyaw) + (ent->pitch - ent->lpitch) * (ent->pitch - ent->lpitch);
		//printf("mp = %f, md = %f\n", mp, md);
		if ((ent->type != ENT_PLAYER || ent->data.player.player != player) && (md > .001 || mp > .01 || ent->type == ENT_PLAYER)) {
			struct packet* pkt = xmalloc(sizeof(struct packet));
			int ft = tick_counter % 20 == 0;
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
	for (size_t i = 0; i < player->world->entities->size; i++) {
		struct entity* ent = (struct entity*) player->world->entities->data[i];
		if (ent == NULL || ent == player->entity || entity_distsq(ent, player->entity) > 16384. || contains_collection(player->loadedEntities, ent)) continue;
		if (ent->type == ENT_PLAYER) loadPlayer(player, ent->data.player.player);
		else loadEntity(player, ent);
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
	cebb.minX -= .375;
	cebb.maxX += .375;
	cebb.minZ -= .375;
	cebb.maxZ += .375;
	struct boundingbox oebb;
	for (size_t i = 0; i < world->entities->size; i++) {
		struct entity* oe = (struct entity*) world->entities->data[i];
		if (oe == NULL || oe == entity || entity_distsq(entity, oe) > 16. * 16.) continue;
		if (oe->type == ENT_PLAYER) {
			getEntityCollision(oe, &oebb);
			if (boundingbox_intersects(&oebb, &cebb)) {
				int os = entity->data.itemstack.slot->itemCount;
				int r = addInventoryItem_PI(&oe->data.player.player->inventory, entity->data.itemstack.slot);
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
				oebb.minX -= .375;
				oebb.maxX += .375;
				oebb.minZ -= .375;
				oebb.maxZ += .375;
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
	if (entity->type > ENT_PLAYER) {
		double friction_modifier = .98;
		if (entity->type == ENT_ITEMSTACK) entity->motY -= 0.04;
		moveEntity(entity);
		double friction = .98; // todo per block
		if (entity->onGround) friction = block_infos[getBlockWorld(entity->world, floor_int(entity->x), floor_int(entity->y) - 1, floor_int(entity->z))].slipperiness * friction_modifier;
		entity->motX *= friction;
		entity->motY *= .98;
		entity->motZ *= friction;
		if (entity->onGround) entity->motY *= -.5;
	}
	if (entity->type == ENT_ITEMSTACK) tick_itemstack(world, entity);
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
