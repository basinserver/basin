/*
 * game.c
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#include "basin/packet.h"
#include <basin/basin.h>
#include <basin/queue.h>
#include <basin/game.h>
#include <basin/crafting.h>
#include <basin/smelting.h>
#include <basin/tileentity.h>
#include <basin/network.h>
#include <basin/server.h>
#include <basin/command.h>
#include <basin/profile.h>
#include <basin/anticheat.h>
#include <avuna/hash.h>
#include <avuna/string.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

void flush_outgoing(struct player* player) {
	if (player->conn == NULL) return;
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
    slot_duplicate(from->inventory->slots == NULL ? NULL : from->inventory->slots[from->currentItem + 36], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 5;
    slot_duplicate(from->inventory->slots == NULL ? NULL : from->inventory->slots[5], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 4;
    slot_duplicate(from->inventory->slots == NULL ? NULL : from->inventory->slots[6], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 3;
    slot_duplicate(from->inventory->slots == NULL ? NULL : from->inventory->slots[7], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 2;
    slot_duplicate(from->inventory->slots == NULL ? NULL : from->inventory->slots[8], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
	pkt->data.play_client.entityequipment.entity_id = from->entity->id;
	pkt->data.play_client.entityequipment.slot = 1;
    slot_duplicate(from->inventory->slots == NULL ? NULL : from->inventory->slots[45], &pkt->data.play_client.entityequipment.item);
	add_queue(to->outgoingPacket, pkt);
	put_hashmap(to->loadedEntities, from->entity->id, from->entity);
	put_hashmap(from->entity->loadingPlayers, to->entity->id, to);
}

void loadEntity(struct player* to, struct entity* from) {
	struct entity_info* ei = getEntityInfo(from->type);
	uint32_t pt = ei->spawn_packet;
	uint32_t pid = ei->spawn_packet_id;
	if (pt == PKT_PLAY_CLIENT_SPAWNOBJECT) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNOBJECT;
		pkt->data.play_client.spawnobject.entity_id = from->id;
		struct uuid uuid;
		for (int i = 0; i < 4; i++)
			memcpy((void*) &uuid + 4 * i, &from->id, 4);
		memcpy(&pkt->data.play_client.spawnobject.object_uuid, &uuid, sizeof(struct uuid));
		pkt->data.play_client.spawnobject.type = pid;
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
		put_hashmap(to->loadedEntities, from->id, from);
		put_hashmap(from->loadingPlayers, to->entity->id, to);
	} else if (from->type == ENT_PLAYER) {
		return;
	} else if (pt == PKT_PLAY_CLIENT_SPAWNPAINTING) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNPAINTING;
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
		put_hashmap(to->loadedEntities, from->id, from);
		put_hashmap(from->loadingPlayers, to->entity->id, to);
	} else if (pt == PKT_PLAY_CLIENT_SPAWNEXPERIENCEORB) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNEXPERIENCEORB;
		pkt->data.play_client.spawnexperienceorb.entity_id = from->id;
		pkt->data.play_client.spawnexperienceorb.x = from->x;
		pkt->data.play_client.spawnexperienceorb.y = from->y;
		pkt->data.play_client.spawnexperienceorb.z = from->z;
		pkt->data.play_client.spawnexperienceorb.count = from->data.experienceorb.count;
		add_queue(to->outgoingPacket, pkt);
		put_hashmap(to->loadedEntities, from->id, from);
		put_hashmap(from->loadingPlayers, to->entity->id, to);
	} else if (pt == PKT_PLAY_CLIENT_SPAWNMOB) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SPAWNMOB;
		pkt->data.play_client.spawnmob.entity_id = from->id;
		struct uuid uuid;
		for (int i = 0; i < 4; i++)
			memcpy((void*) &uuid + 4 * i, &from->id, 4);
		memcpy(&pkt->data.play_client.spawnmob.entity_uuid, &uuid, sizeof(struct uuid));
		pkt->data.play_client.spawnmob.type = pid;
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
		put_hashmap(to->loadedEntities, from->id, from);
		put_hashmap(from->loadingPlayers, to->entity->id, to);
	}
}

void onInventoryUpdate(struct player* player, struct inventory* inv, int slot) {
	if (inv->type == INVTYPE_PLAYERINVENTORY) {
		if (slot == player->currentItem + 36) {
			if (player->itemUseDuration > 0 && player->itemUseHand == 0) {
				struct slot* ihs = inventory_get(player, player->inventory, player->itemUseHand ? 45 : (36 + player->currentItem));
				struct item_info* ihi = ihs == NULL ? NULL : getItemInfo(ihs->item);
				if (ihs == NULL || ihi == NULL) player->itemUseDuration = 0;
				else {
					if (ihi->onItemUseTick != NULL) (*ihi->onItemUseTick)(player->world, player, player->itemUseHand ? 45 : (36 + player->currentItem), ihs, -1);
					player->itemUseDuration = 0;
				}
			}
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			pkt->data.play_client.entityequipment.slot = 0;
                                slot_duplicate(inventory_get(player, inv, player->currentItem + 36), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			END_BROADCAST(player->world->players)
		} else if (slot >= 5 && slot <= 8) {
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			if (slot == 5) pkt->data.play_client.entityequipment.slot = 5;
			else if (slot == 6) pkt->data.play_client.entityequipment.slot = 4;
			else if (slot == 7) pkt->data.play_client.entityequipment.slot = 3;
			else if (slot == 8) pkt->data.play_client.entityequipment.slot = 2;
                                slot_duplicate(inventory_get(player, inv, slot), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			END_BROADCAST(player->world->players)
		} else if (slot == 45) {
			BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYEQUIPMENT;
			pkt->data.play_client.entityequipment.entity_id = player->entity->id;
			pkt->data.play_client.entityequipment.slot = 1;
                                slot_duplicate(inventory_get(player, inv, 45), &pkt->data.play_client.entityequipment.item);
			add_queue(bc_player->outgoingPacket, pkt);
			END_BROADCAST(player->world->players)
		} else if (slot >= 1 && slot <= 4) {
			inventory_set_slot(player, inv, 0, crafting_result(&inv->slots[1], 4), 1, 1);
		}
	} else if (inv->type == INVTYPE_WORKBENCH) {
		if (slot >= 1 && slot <= 9) {
			inventory_set_slot(player, inv, 0, crafting_result(&inv->slots[1], 9), 1, 1);
		}
	} else if (inv->type == INVTYPE_FURNACE) {
		if (slot >= 0 && slot <= 2 && player != NULL) {
			update_furnace(player->world, inv->tile);
		}
	}
}

float randFloat() {
	return ((float) rand() / (float) RAND_MAX);
}

void dropBlockDrop(struct world* world, struct slot* slot, int32_t x, int32_t y, int32_t z) {
	struct entity* item = newEntity(nextEntityID++, (double) x + .5, (double) y + .5, (double) z + .5, ENT_ITEM, randFloat() * 360., 0.);
	item->data.itemstack.slot = xmalloc(sizeof(struct slot));
	item->objectData = 1;
	item->motX = randFloat() * .2 - .1;
	item->motY = .2;
	item->motZ = randFloat() * .2 - .1;
	item->data.itemstack.delayBeforeCanPickup = 0;
    slot_duplicate(slot, item->data.itemstack.slot);
    world_spawn_entity(world, item);
	BEGIN_BROADCAST_DIST(item, 128.)
	loadEntity(bc_player, item);
	END_BROADCAST(item->world->players)
}

void dropPlayerItem(struct player* player, struct slot* drop) {
	struct entity* item = newEntity(nextEntityID++, player->entity->x, player->entity->y + 1.32, player->entity->z, ENT_ITEM, 0., 0.);
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
    slot_duplicate(drop, item->data.itemstack.slot);
    world_spawn_entity(player->world, item);
	BEGIN_BROADCAST_DIST(player->entity, 128.)
	loadEntity(bc_player, item);
	END_BROADCAST(player->world->players)
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
	END_BROADCAST(world->players)
}

void dropEntityItem_explode(struct entity* entity, struct slot* drop) {
	struct entity* item = newEntity(nextEntityID++, entity->x, entity->y + 1.32, entity->z, ENT_ITEM, 0., 0.);
	item->data.itemstack.slot = xmalloc(sizeof(struct slot));
	item->objectData = 1;
	float f1 = randFloat() * .5;
	float f2 = randFloat() * M_PI * 2.;
	item->motX = -sin(f2) * f1;
	item->motZ = cos(f2) * f1;
	item->motY = .2;
	item->data.itemstack.delayBeforeCanPickup = 20;
    slot_duplicate(drop, item->data.itemstack.slot);
    world_spawn_entity(entity->world, item);
	BEGIN_BROADCAST_DIST(entity, 128.)
	loadEntity(bc_player, item);
	END_BROADCAST(entity->world->players)
}

void dropBlockDrops(struct world* world, block blk, struct player* breaker, int32_t x, int32_t y, int32_t z) {
	struct block_info* bi = getBlockInfo(blk);
	int badtool = !bi->material->requiresnotool && breaker != NULL;
	if (badtool) {
		struct slot* ci = inventory_get(breaker, breaker->inventory, 36 + breaker->currentItem);
		if (ci != NULL) {
			struct item_info* ii = getItemInfo(ci->item);
			if (ii != NULL) badtool = !tools_proficient(ii->toolType, ii->harvestLevel, blk);
		}
	}
	if (badtool) return;
	if (bi->dropItems == NULL) {
		if (bi->drop <= 0 || bi->drop_max == 0 || bi->drop_max < bi->drop_min) return;
		struct slot dd;
		dd.item = bi->drop;
		dd.damage = bi->drop_damage;
		dd.count = bi->drop_min + ((bi->drop_max == bi->drop_min) ? 0 : (rand() % (bi->drop_max - bi->drop_min)));
		dd.nbt = NULL;
		dropBlockDrop(world, &dd, x, y, z);
	} else (*bi->dropItems)(world, blk, x, y, z, 0);
}

void player_openInventory(struct player* player, struct inventory* inv) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_OPENWINDOW;
	pkt->data.play_client.openwindow.window_id = inv->window;
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
	player->openInv = inv;
	put_hashmap(inv->watching_players, player->entity->id, player);
	if (inv->slot_count > 0) {
		pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_WINDOWITEMS;
		pkt->data.play_client.windowitems.window_id = inv->window;
		pkt->data.play_client.windowitems.count = inv->slot_count;
		pkt->data.play_client.windowitems.slot_data = xmalloc(sizeof(struct slot) * inv->slot_count);
		for (size_t i = 0; i < inv->slot_count; i++) {
            slot_duplicate(inv->slots[i], &pkt->data.play_client.windowitems.slot_data[i]);
		}
		add_queue(player->outgoingPacket, pkt);
	}
}

void sendMessageToPlayer(struct player* player, char* text, char* color) {
	if (player == NULL) {
		printf("%s\n", text);
	} else {
		size_t s = strlen(text) + 512;
		char* rsx = xstrdup(text, 512);
		char* rs = xmalloc(s);
		snprintf(rs, s, "{\"text\": \"%s\", \"color\": \"%s\"}", replace(replace(rsx, "\\", "\\\\"), "\"", "\\\""), color);
		//printf("<CHAT> %s\n", text);
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_CHATMESSAGE;
		pkt->data.play_client.chatmessage.position = 0;
		pkt->data.play_client.chatmessage.json_data = xstrdup(rs, 0);
		add_queue(player->outgoingPacket, pkt);
		xfree(rsx);
	}
}

void sendMsgToPlayerf(struct player* player, char* color, char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t len = varstrlen(fmt, args);
	char* bct = xmalloc(len);
	vsnprintf(bct, len, fmt, args);
	va_end(args);
	sendMessageToPlayer(player, bct, color);
	xfree(bct);
}

void broadcast(char* text, char* color) {
	size_t s = strlen(text) + 512;
	char* rsx = xstrdup(text, 512);
	char* rs = xmalloc(s);
	snprintf(rs, s, "{\"text\": \"%s\", \"color\": \"%s\"}", replace(replace(rsx, "\\", "\\\\"), "\"", "\\\""), color);
	printf("<CHAT> %s\n", text);
	BEGIN_BROADCAST (players)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_CHATMESSAGE;
	pkt->data.play_client.chatmessage.position = 0;
	pkt->data.play_client.chatmessage.json_data = xstrdup(rs, 0);
	add_queue(bc_player->outgoingPacket, pkt);
	END_BROADCAST (players)
	xfree(rs);
	xfree(rsx);
}

void broadcastf(char* color, char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t len = varstrlen(fmt, args);
	char* bct = xmalloc(len);
	vsnprintf(bct, len, fmt, args);
	va_end(args);
	broadcast(bct, color);
	xfree(bct);
}
