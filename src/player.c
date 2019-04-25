/*
 * player.c
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#include "accept.h"
#include "basin/packet.h"
#include <basin/network.h>
#include <basin/inventory.h>
#include <basin/tileentity.h>
#include <basin/server.h>
#include <basin/profile.h>
#include <basin/basin.h>
#include <basin/anticheat.h>
#include <basin/command.h>
#include <basin/smelting.h>
#include <basin/crafting.h>
#include <basin/plugin.h>
#include <avuna/string.h>
#include <avuna/queue.h>
#include <avuna/pmem.h>
#include <math.h>

struct player* player_new(struct mempool* parent, struct server* server, struct conn* conn, struct world* world, struct entity* entity, char* name, struct uuid uuid, uint8_t gamemode) {
	struct mempool* pool = mempool_new();
	pchild(parent, pool);
	struct player* player = pcalloc(pool, sizeof(struct player));
	player->pool = pool;
    player->server = server;
    player->conn = conn;
	player->world = world;
	player->entity = entity;
	player->name = name;
	player->uuid = uuid;
	player->gamemode = gamemode;
	entity->data.player.player = player;
	player->food = 20;
	player->foodTick = 0;
	player->digging = -1.f;
	player->digspeed = 0.f;
	player->inventory = inventory_new(player->pool, INVTYPE_PLAYERINVENTORY, 0, 46, "Inventory");
	hashmap_putint(player->inventory->watching_players, (uint64_t) player->entity->id, player);
	player->loaded_chunks = hashmap_thread_new(128, player->pool);
	player->loaded_entities = hashmap_thread_new(128, player->pool);
	player->incoming_packets = queue_new(0, 1);
	player->outgoing_packets = queue_new(0, 1);
	player->lastSwing = player->server->tick_counter;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	player->reachDistance = 6.f;
	return player;
}

void player_send_entity_move(struct player* player, struct entity* ent) {
	double md = entity_distsq_block(ent, ent->last_x, ent->last_y, ent->last_z);
	double mp = (ent->yaw - ent->last_yaw) * (ent->yaw - ent->last_yaw) + (ent->pitch - ent->last_pitch) * (ent->pitch - ent->last_pitch);

	//printf("mp = %f, md = %f\n", mp, md);
	if ((md > .001 || mp > .01 || ent->type == ENT_PLAYER)) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		int ft = tick_counter % 200 == 0 || (ent->type == ENT_PLAYER && ent->data.player.player->last_teleport_id != 0);
		if (!ft && md <= .001 && mp <= .01) {
			pkt->id = PKT_PLAY_CLIENT_ENTITY;
			pkt->data.play_client.entity.entity_id = ent->id;
			add_queue(player->outgoing_packets, pkt);
		} else if (!ft && mp > .01 && md <= .001) {
			pkt->id = PKT_PLAY_CLIENT_ENTITYLOOK;
			pkt->data.play_client.entitylook.entity_id = ent->id;
			pkt->data.play_client.entitylook.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			pkt->data.play_client.entitylook.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
			pkt->data.play_client.entitylook.on_ground = ent->on_ground;
			add_queue(player->outgoing_packets, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoing_packets, pkt);
		} else if (!ft && mp <= .01 && md > .001 && md < 64.) {
			pkt->id = PKT_PLAY_CLIENT_ENTITYRELATIVEMOVE;
			pkt->data.play_client.entityrelativemove.entity_id = ent->id;
			pkt->data.play_client.entityrelativemove.delta_x = (int16_t)((ent->x * 32. - ent->last_x * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.delta_y = (int16_t)((ent->y * 32. - ent->last_y * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.delta_z = (int16_t)((ent->z * 32. - ent->last_z * 32.) * 128.);
			pkt->data.play_client.entityrelativemove.on_ground = ent->on_ground;
			add_queue(player->outgoing_packets, pkt);
		} else if (!ft && mp > .01 && md > .001 && md < 64.) {
			pkt->id = PKT_PLAY_CLIENT_ENTITYLOOKANDRELATIVEMOVE;
			pkt->data.play_client.entitylookandrelativemove.entity_id = ent->id;
			pkt->data.play_client.entitylookandrelativemove.delta_x = (int16_t)((ent->x * 32. - ent->last_x * 32.) * 128.);
			pkt->data.play_client.entitylookandrelativemove.delta_y = (int16_t)((ent->y * 32. - ent->last_y * 32.) * 128.);
			pkt->data.play_client.entitylookandrelativemove.delta_z = (int16_t)((ent->z * 32. - ent->last_z * 32.) * 128.);
			pkt->data.play_client.entitylookandrelativemove.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			pkt->data.play_client.entitylookandrelativemove.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
			pkt->data.play_client.entitylookandrelativemove.on_ground = ent->on_ground;
			add_queue(player->outgoing_packets, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoing_packets, pkt);
		} else {
			pkt->id = PKT_PLAY_CLIENT_ENTITYTELEPORT;
			pkt->data.play_client.entityteleport.entity_id = ent->id;
			pkt->data.play_client.entityteleport.x = ent->x;
			pkt->data.play_client.entityteleport.y = ent->y;
			pkt->data.play_client.entityteleport.z = ent->z;
			pkt->data.play_client.entityteleport.yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			pkt->data.play_client.entityteleport.pitch = (uint8_t)((ent->pitch / 360.) * 256.);
			pkt->data.play_client.entityteleport.on_ground = ent->on_ground;
			add_queue(player->outgoing_packets, pkt);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_ENTITYHEADLOOK;
			pkt->data.play_client.entityheadlook.entity_id = ent->id;
			pkt->data.play_client.entityheadlook.head_yaw = (uint8_t)((ent->yaw / 360.) * 256.);
			add_queue(player->outgoing_packets, pkt);
		}
	}
}

void player_hungerUpdate(struct player* player) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_UPDATEHEALTH;
	pkt->data.play_client.updatehealth.health = player->entity->health;
	pkt->data.play_client.updatehealth.food = player->food;
	pkt->data.play_client.updatehealth.food_saturation = player->saturation;
	add_queue(player->outgoing_packets, pkt);
}

void player_tick(struct world* world, struct player* player) {
	if (player->defunct) {
		put_hashmap(players, player->entity->id, NULL);
		BEGIN_BROADCAST (players)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_PLAYERLISTITEM;
		pkt->data.play_client.playerlistitem.action_id = 4;
		pkt->data.play_client.playerlistitem.number_of_players = 1;
		pkt->data.play_client.playerlistitem.players = xmalloc(sizeof(struct listitem_player));
		memcpy(&pkt->data.play_client.playerlistitem.players->uuid, &player->uuid, sizeof(struct uuid));
		add_queue(bc_player->outgoing_packets, pkt);
		END_BROADCAST (players)
		world_despawn_player(player->world, player);
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
		add_queue(player->outgoing_packets, pkt);
	} else if (tick_counter % 200 == 100) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_TIMEUPDATE;
		pkt->data.play_client.timeupdate.time_of_day = player->world->time;
		pkt->data.play_client.timeupdate.world_age = player->world->age;
		add_queue(player->outgoing_packets, pkt);
	}
	if (player->gamemode != 1 && player->gamemode != 3) {
		float dt = entity_dist_block(player->entity, player->entity->last_x, player->entity->last_y, player->entity->last_z);
		if (dt > 0.) {
			if (player->entity->inWater) player->foodExhaustion += .01 * dt;
			else if (player->entity->on_ground) {
				if (player->entity->sprinting) player->foodExhaustion += .1 * dt;
			}
		}
		if (player->foodExhaustion > 4.) {
			player->foodExhaustion -= 4.;
			if (player->saturation > 0.) {
				player->saturation -= 1.;
				if (player->saturation < 0.) player->saturation = 0.;
			} else if (difficulty > 0) {
				player->food -= 1;
				if (player->food < 0) player->food = 0.;
			}
			player_hungerUpdate(player);
		}
		if (player->saturation > 0. && player->food >= 20 && difficulty == 0) {
			if (++player->foodTimer >= 10) {
				if (player->entity->health < player->entity->maxHealth) {
					healEntity(player->entity, player->saturation < 6. ? player->saturation / 6. : 1.);
					player->foodExhaustion += (player->saturation < 6. ? player->saturation : 6.) / 6.;
				}
				player->foodTimer = 0;
			}
		} else if (player->food >= 18) {
			if (++player->foodTimer >= 80) {
				if (player->entity->health < player->entity->maxHealth) {
					healEntity(player->entity, 1.);
					player->foodExhaustion += 1.;
				}
				player->foodTimer = 0;
			}
		} else if (player->food <= 0) {
			if (++player->foodTimer >= 80) {
				if (player->entity->health > 10. || difficulty == 3 || (player->entity->health > 1. && difficulty == 2)) damageEntity(player->entity, 1., 0);
				player->foodTimer = 0;
			}
		}
	}
	beginProfilerSection("chunks");
	int32_t pcx = ((int32_t) player->entity->x >> 4);
	int32_t pcz = ((int32_t) player->entity->z >> 4);
	int32_t lpcx = ((int32_t) player->entity->last_x >> 4);
	int32_t lpcz = ((int32_t) player->entity->last_z >> 4);
	if (player->loaded_chunks->entry_count == 0 || player->triggerRechunk) { // || tick_counter % 200 == 0
		beginProfilerSection("chunkLoading_tick");
		pthread_mutex_lock(&player->chunkRequests->data_mutex);
		int we0 = player->loaded_chunks->entry_count == 0 || player->triggerRechunk;
		if (player->triggerRechunk) {
			BEGIN_HASHMAP_ITERATION(player->loaded_chunks)
			struct chunk* ch = (struct chunk*) value;
			if (ch->x < pcx - CHUNK_VIEW_DISTANCE || ch->x > pcx + CHUNK_VIEW_DISTANCE || ch->z < pcz - CHUNK_VIEW_DISTANCE || ch->z > pcz + CHUNK_VIEW_DISTANCE) {
				struct chunk_request* cr = xmalloc(sizeof(struct chunk_request));
				cr->chunk_x = ch->x;
				cr->chunk_z = ch->z;
				cr->world = world;
				cr->load = 0;
				add_queue(player->chunkRequests, cr);
			}
			END_HASHMAP_ITERATION(player->loaded_chunks)
		}
		for (int r = 0; r <= CHUNK_VIEW_DISTANCE; r++) {
			int32_t x = pcx - r;
			int32_t z = pcz - r;
			for (int i = 0; i < ((r == 0) ? 1 : (r * 8)); i++) {
				if (we0 || !contains_hashmap(player->loaded_chunks, chunk_get_key_direct(x, z))) {
					struct chunk_request* cr = xmalloc(sizeof(struct chunk_request));
					cr->chunk_x = x;
					cr->chunk_z = z;
					cr->world = world;
					cr->load = 1;
					add_queue(player->chunkRequests, cr);
				}
				if (i < 2 * r) x++;
				else if (i < 4 * r) z++;
				else if (i < 6 * r) x--;
				else if (i < 8 * r) z--;
			}
		}
		pthread_mutex_unlock(&player->chunkRequests->data_mutex);
		player->triggerRechunk = 0;
		pthread_cond_signal (&chunk_wake);
		endProfilerSection("chunkLoading_tick");
	}
	if (lpcx != pcx || lpcz != pcz) {
		pthread_mutex_lock(&player->chunkRequests->data_mutex);
		for (int32_t fx = lpcx; lpcx < pcx ? (fx < pcx) : (fx > pcx); lpcx < pcx ? fx++ : fx--) {
			for (int32_t fz = lpcz - CHUNK_VIEW_DISTANCE; fz <= lpcz + CHUNK_VIEW_DISTANCE; fz++) {
				beginProfilerSection("chunkUnloading_live");
				struct chunk_request* cr = xmalloc(sizeof(struct chunk_request));
				cr->chunk_x = lpcx < pcx ? (fx - CHUNK_VIEW_DISTANCE) : (fx + CHUNK_VIEW_DISTANCE);
				cr->chunk_z = fz;
				cr->load = 0;
				add_queue(player->chunkRequests, cr);
				endProfilerSection("chunkUnloading_live");
				beginProfilerSection("chunkLoading_live");
				cr = xmalloc(sizeof(struct chunk_request));
				cr->chunk_x = lpcx < pcx ? (fx + CHUNK_VIEW_DISTANCE) : (fx - CHUNK_VIEW_DISTANCE);
				cr->chunk_z = fz;
				cr->world = world;
				cr->load = 1;
				add_queue(player->chunkRequests, cr);
				endProfilerSection("chunkLoading_live");
			}
		}
		for (int32_t fz = lpcz; lpcz < pcz ? (fz < pcz) : (fz > pcz); lpcz < pcz ? fz++ : fz--) {
			for (int32_t fx = lpcx - CHUNK_VIEW_DISTANCE; fx <= lpcx + CHUNK_VIEW_DISTANCE; fx++) {
				beginProfilerSection("chunkUnloading_live");
				struct chunk_request* cr = xmalloc(sizeof(struct chunk_request));
				cr->chunk_x = fx;
				cr->chunk_z = lpcz < pcz ? (fz - CHUNK_VIEW_DISTANCE) : (fz + CHUNK_VIEW_DISTANCE);
				cr->load = 0;
				add_queue(player->chunkRequests, cr);
				endProfilerSection("chunkUnloading_live");
				beginProfilerSection("chunkLoading_live");
				cr = xmalloc(sizeof(struct chunk_request));
				cr->chunk_x = fx;
				cr->chunk_z = lpcz < pcz ? (fz + CHUNK_VIEW_DISTANCE) : (fz - CHUNK_VIEW_DISTANCE);
				cr->load = 1;
				cr->world = world;
				add_queue(player->chunkRequests, cr);
				endProfilerSection("chunkLoading_live");
			}
		}
		pthread_mutex_unlock(&player->chunkRequests->data_mutex);
		pthread_cond_signal (&chunk_wake);
	}
	endProfilerSection("chunks");
	if (player->itemUseDuration > 0) {
		struct slot* ihs = inventory_get(player, player->inventory, player->itemUseHand ? 45 : (36 + player->currentItem));
		struct item_info* ihi = ihs == NULL ? NULL : getItemInfo(ihs->item);
		if (ihs == NULL || ihi == NULL) {
			player->itemUseDuration = 0;
			player->entity->usingItemMain = 0;
			player->entity->usingItemOff = 0;
			updateMetadata(player->entity);
		} else if (ihi->maxUseDuration <= player->itemUseDuration) {
			if (ihi->onItemUse != NULL) (*ihi->onItemUse)(world, player, player->itemUseHand ? 45 : (36 + player->currentItem), ihs, player->itemUseDuration);
			player->itemUseDuration = 0;
			player->entity->usingItemMain = 0;
			player->entity->usingItemOff = 0;
			updateMetadata(player->entity);
		} else {
			if (ihi->onItemUseTick != NULL) (*ihi->onItemUseTick)(world, player, player->itemUseHand ? 45 : (36 + player->currentItem), ihs, player->itemUseDuration);
			player->itemUseDuration++;
		}
	}
	//if (((int32_t) player->entity->lx >> 4) != pcx || ((int32_t) player->entity->lz >> 4) != pcz || player->loaded_chunks->count < CHUNK_VIEW_DISTANCE * CHUNK_VIEW_DISTANCE * 4 || player->triggerRechunk) {
	//}
	if (player->digging >= 0.) {
		block bw = world_get_block(world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
		struct block_info* bi = getBlockInfo(bw);
		float digspeed = 0.;
		if (bi->hardness > 0.) {
			pthread_mutex_lock(&player->inventory->mut);
			struct slot* ci = inventory_get(player, player->inventory, 36 + player->currentItem);
			struct item_info* cii = ci == NULL ? NULL : getItemInfo(ci->item);
			int hasProperTool = (cii == NULL ? 0 : tools_proficient(cii->toolType, cii->harvestLevel, bw));
			int hasProperTool2 = (cii == NULL ? 0 : tools_proficient(cii->toolType, 0xFF, bw));
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
			if (!player->entity->on_ground) ds /= 5.;
			digspeed = ds / (bi->hardness * ((hasProperTool || rnt) ? 30. : 100.));
		}
		player->digging += (player->digging == 0. ? 2. : 1.) * digspeed;
		BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, 128.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_BLOCKBREAKANIMATION;
		pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
		memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
		pkt->data.play_client.blockbreakanimation.destroy_stage = (int8_t)(player->digging * 9);
		add_queue(bc_player->outgoing_packets, pkt);
		END_BROADCAST(player->world->players)
	}
	beginProfilerSection("entity_transmission");
	if (tick_counter % 20 == 0) {
		BEGIN_HASHMAP_ITERATION(player->loaded_entities)
		struct entity* ent = (struct entity*) value;
		if (entity_distsq(ent, player->entity) > (CHUNK_VIEW_DISTANCE * 16.) * (CHUNK_VIEW_DISTANCE * 16.)) {
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_DESTROYENTITIES;
			pkt->data.play_client.destroyentities.count = 1;
			pkt->data.play_client.destroyentities.entity_ids = xmalloc(sizeof(int32_t));
			pkt->data.play_client.destroyentities.entity_ids[0] = ent->id;
			add_queue(player->outgoing_packets, pkt);
			put_hashmap(player->loaded_entities, ent->id, NULL);
			put_hashmap(ent->loadingPlayers, player->entity->id, NULL);
		}
		END_HASHMAP_ITERATION(player->loaded_entities)
	}
	endProfilerSection("entity_transmission");
	beginProfilerSection("player_transmission");
	//int32_t chunk_x = ((int32_t) player->entity->x) >> 4;
	//int32_t chunk_z = ((int32_t) player->entity->z) >> 4;
	//for (int32_t icx = chunk_x - CHUNK_VIEW_DISTANCE; icx <= chunk_x + CHUNK_VIEW_DISTANCE; icx++)
	//for (int32_t icz = chunk_z - CHUNK_VIEW_DISTANCE; icz <= chunk_z + CHUNK_VIEW_DISTANCE; icz++) {
	//struct chunk* ch = world_get_chunk(player->world, icx, icz);
	//if (ch != NULL) {
	BEGIN_HASHMAP_ITERATION(player->world->entities)
	struct entity* ent = (struct entity*) value;
	if (ent == player->entity || contains_hashmap(player->loaded_entities, ent->id) || entity_distsq(player->entity, ent) > (CHUNK_VIEW_DISTANCE * 16.) * (CHUNK_VIEW_DISTANCE * 16.)) continue;
	if (ent->type == ENT_PLAYER) loadPlayer(player, ent->data.player.player);
	else loadEntity(player, ent);
	END_HASHMAP_ITERATION(player->world->entities)
	//}
	//}
	endProfilerSection("player_transmission");
	BEGIN_HASHMAP_ITERATION (plugins)
	struct plugin* plugin = value;
	if (plugin->tick_player != NULL) (*plugin->tick_player)(player->world, player);
	END_HASHMAP_ITERATION (plugins)
	//printf("%i\n", player->loaded_chunks->size);
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
				block b = world_get_block(entity->world, x, y, z);
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

void player_kick(struct player* player, char* message) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_DISCONNECT;
	size_t sl = strlen(message);
	pkt->data.play_client.disconnect.reason = xmalloc(sl + 512);
	snprintf(pkt->data.play_client.disconnect.reason, sl + 512, "{\"text\": \"%s\", \"color\": \"red\"}", message);
	add_queue(player->outgoing_packets, pkt);
	if (player->conn != NULL) player->conn->disconnect = 1;
	broadcastf("red", "Kicked Player %s for reason: %s", player->name, message);
}

float player_getAttackStrength(struct player* player, float adjust) {
	float cooldownPeriod = 4.;
	struct slot* sl = inventory_get(player, player->inventory, 36 + player->currentItem);
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

void player_teleport(struct player* player, double x, double y, double z) {
	player->entity->x = x;
	player->entity->y = y;
	player->entity->z = z;
	player->entity->last_x = x;
	player->entity->last_y = y;
	player->entity->last_z = z;
	player->triggerRechunk = 1;
	player->spawnedIn = 0;
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYVELOCITY;
	pkt->data.play_client.entityvelocity.entity_id = player->entity->id;
	pkt->data.play_client.entityvelocity.velocity_x = 0;
	pkt->data.play_client.entityvelocity.velocity_y = 0;
	pkt->data.play_client.entityvelocity.velocity_z = 0;
	add_queue(player->outgoing_packets, pkt);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK;
	pkt->data.play_client.playerpositionandlook.x = player->entity->x;
	pkt->data.play_client.playerpositionandlook.y = player->entity->y;
	pkt->data.play_client.playerpositionandlook.z = player->entity->z;
	pkt->data.play_client.playerpositionandlook.yaw = player->entity->yaw;
	pkt->data.play_client.playerpositionandlook.pitch = player->entity->pitch;
	pkt->data.play_client.playerpositionandlook.flags = 0x0;
	pkt->data.play_client.playerpositionandlook.teleport_id = tick_counter;
	player->last_teleport_id = pkt->data.play_client.playerpositionandlook.teleport_id;
	add_queue(player->outgoing_packets, pkt);
	/*BEGIN_HASHMAP_ITERATION(player->entity->loadingPlayers)
	 struct player* bp = value;
	 struct packet* pkt = xmalloc(sizeof(struct packet));
	 pkt->id = PKT_PLAY_CLIENT_DESTROYENTITIES;
	 pkt->data.play_client.destroyentities.count = 1;
	 pkt->data.play_client.destroyentities.entity_ids = xmalloc(sizeof(int32_t));
	 pkt->data.play_client.destroyentities.entity_ids[0] = player->entity->id;
	 add_queue(bp->outgoing_packets, pkt);
	 put_hashmap(bp->loaded_entities, player->entity->id, NULL);
	 pthread_rwlock_unlock(&player->entity->loadingPlayers->data_mutex);
	 put_hashmap(player->entity->loadingPlayers, bp->entity->id, NULL);
	 pthread_rwlock_rdlock(&player->entity->loadingPlayers->data_mutex);
	 END_HASHMAP_ITERATION(player->entity->loadingPlayers)
	 BEGIN_HASHMAP_ITERATION(player->loaded_entities)
	 struct entity* be = value;
	 if (be->type != ENT_PLAYER) continue;
	 struct packet* pkt = xmalloc(sizeof(struct packet));
	 pkt->id = PKT_PLAY_CLIENT_DESTROYENTITIES;
	 pkt->data.play_client.destroyentities.count = 1;
	 pkt->data.play_client.destroyentities.entity_ids = xmalloc(sizeof(int32_t));
	 pkt->data.play_client.destroyentities.entity_ids[0] = be->id;
	 add_queue(player->outgoing_packets, pkt);
	 put_hashmap(player->loaded_entities, be->id, NULL);
	 put_hashmap(be->loadingPlayers, player->entity->id, NULL);
	 END_HASHMAP_ITERATION(player->loaded_entities)*/
//	if (player->tps > 0) player->tps--;
}

struct player* player_get_by_name(char* name) {
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
	else if (player->open_inventory != NULL && windowID == player->open_inventory->windowID) inv = player->openInv;
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
					inventory_set_slot(player, inv, i, NULL, 0, 1);
				}
			}
		} else if (inv->type == INVTYPE_WORKBENCH) {
			for (int i = 1; i < 10; i++) {
				if (inv->slots[i] != NULL) {
					dropPlayerItem(player, inv->slots[i]);
					inventory_set_slot(player, inv, i, NULL, 0, 1);
				}
			}
			pthread_mutex_unlock(&inv->mut);
			freeInventory(inv);
			inv = NULL;
		} else if (inv->type == INVTYPE_CHEST) {
			if (inv->tile != NULL) {
				BEGIN_BROADCAST_DIST(player->entity, 128.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_BLOCKACTION;
				pkt->data.play_client.blockaction.location.x = inv->tile->x;
				pkt->data.play_client.blockaction.location.y = inv->tile->y;
				pkt->data.play_client.blockaction.location.z = inv->tile->z;
				pkt->data.play_client.blockaction.action_id = 1;
				pkt->data.play_client.blockaction.action_param = inv->players->entry_count - 1;
				pkt->data.play_client.blockaction.block_type = world_get_block(player->world, inv->tile->x, inv->tile->y, inv->tile->z) >> 4;
				add_queue(bc_player->outgoing_packets, pkt);
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

void player_set_gamemode(struct player* player, int gamemode) {
	if (gamemode != -1) {
		player->gamemode = gamemode;
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_CHANGEGAMESTATE;
		pkt->data.play_client.changegamestate.reason = 3;
		pkt->data.play_client.changegamestate.value = gamemode;
		add_queue(player->outgoing_packets, pkt);
	}
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_PLAYERABILITIES;
	pkt->data.play_client.playerabilities.flags = player->gamemode == 1 ? (0x04 | 0x08) : 0x0;
	pkt->data.play_client.playerabilities.flying_speed = .05;
	pkt->data.play_client.playerabilities.field_of_view_modifier = .1;
	add_queue(player->outgoing_packets, pkt);
}

void player_free(struct player* player) {
	struct packet* pkt = NULL;
	while ((pkt = pop_nowait_queue(player->incoming_packets)) != NULL) {
		freePacket(STATE_PLAY, 0, pkt);
		xfree(pkt);
	}
	del_queue(player->incoming_packets);
	while ((pkt = pop_nowait_queue(player->outgoing_packets)) != NULL) {
		freePacket(STATE_PLAY, 1, pkt);
		xfree(pkt);
	}
	del_queue(player->outgoing_packets);
	struct chunk_request* cr;
	while ((cr = pop_nowait_queue(player->chunkRequests)) != NULL) {
		xfree(cr);
	}
	del_queue(player->chunkRequests);
	if (player->inHand != NULL) {
		freeSlot(player->inHand);
		xfree(player->inHand);
	}
	del_hashmap(player->loaded_chunks);
	del_hashmap(player->loaded_entities);
	freeInventory(player->inventory);
	xfree(player->inventory);
	xfree(player->name);
	xfree(player);
}

block player_can_place_block(struct player* player, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
	struct block_info* bi = getBlockInfo(blk);
	block tbb = blk;
	if (bi != NULL && bi->onBlockPlacedPlayer != NULL) tbb = (*bi->onBlockPlacedPlayer)(player, player->world, tbb, x, y, z, face);
	BEGIN_HASHMAP_ITERATION (plugins)
	struct plugin* plugin = value;
	if (plugin->onBlockPlacedPlayer != NULL) tbb = (*plugin->onBlockPlacedPlayer)(player, player->world, tbb, x, y, z, face);
	END_HASHMAP_ITERATION (plugins)
	bi = getBlockInfo(blk);
	if (bi != NULL && bi->canBePlaced != NULL && !(*bi->canBePlaced)(player->world, tbb, x, y, z)) {
		return 0;
	}
	block b = world_get_block(player->world, x, y, z);
	return (b == 0 || getBlockInfo(b)->material->replacable) ? tbb : 0;
}
