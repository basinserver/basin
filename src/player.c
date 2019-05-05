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

void player_cleanup(struct player* player) {
    struct packet* pkt = NULL;
    while ((pkt = queue_maybepop(player->incoming_packets)) != NULL) {
        pfree(pkt->pool);
    }
    while ((pkt = queue_maybepop(player->outgoing_packets)) != NULL) {
        pfree(pkt->pool);
    }
}

struct player* player_new(struct mempool* parent, struct server* server, struct connection* conn, struct world* world, struct entity* entity, char* name, struct uuid uuid, uint8_t gamemode) {
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
    player->incoming_packets = queue_new(0, 1, player->pool);
    player->outgoing_packets = queue_new(0, 1, player->pool);
    player->lastSwing = player->world->tick_counter;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    player->reachDistance = 6.f;
    phook(player->pool, player_cleanup, player);
    return player;
}

void player_send_entity_move(struct player* player, struct entity* entity) {
    double dist = entity_distsq_block(entity, entity->last_x, entity->last_y, entity->last_z);
    double delta_rotation = (entity->yaw - entity->last_yaw) * (entity->yaw - entity->last_yaw) + (entity->pitch - entity->last_pitch) * (entity->pitch - entity->last_pitch);

    if ((dist > .001 || delta_rotation > .01 || entity->type == ENT_PLAYER)) {
        int force_update = player->world->tick_counter % 200 == 0 || (entity->type == ENT_PLAYER && entity->data.player.player->last_teleport_id != 0);
        if (!force_update && dist <= .001 && delta_rotation <= .01) {
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITY);
            pkt->data.play_client.entity.entity_id = entity->id;
            queue_push(player->outgoing_packets, pkt);
        } else if (!force_update && delta_rotation > .01 && dist <= .001) {
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYLOOK);
            pkt->data.play_client.entitylook.entity_id = entity->id;
            pkt->data.play_client.entitylook.yaw = (uint8_t) ((entity->yaw / 360.) * 256.);
            pkt->data.play_client.entitylook.pitch = (uint8_t) ((entity->pitch / 360.) * 256.);
            pkt->data.play_client.entitylook.on_ground = entity->on_ground;
            queue_push(player->outgoing_packets, pkt);
            pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYHEADLOOK);
            pkt->data.play_client.entityheadlook.entity_id = entity->id;
            pkt->data.play_client.entityheadlook.head_yaw = (uint8_t) ((entity->yaw / 360.) * 256.);
            queue_push(player->outgoing_packets, pkt);
        } else if (!force_update && delta_rotation <= .01 && dist > .001 && dist < 64.) {
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYRELATIVEMOVE);
            pkt->data.play_client.entityrelativemove.entity_id = entity->id;
            pkt->data.play_client.entityrelativemove.delta_x = (int16_t) ((entity->x * 32. - entity->last_x * 32.) * 128.);
            pkt->data.play_client.entityrelativemove.delta_y = (int16_t) ((entity->y * 32. - entity->last_y * 32.) * 128.);
            pkt->data.play_client.entityrelativemove.delta_z = (int16_t) ((entity->z * 32. - entity->last_z * 32.) * 128.);
            pkt->data.play_client.entityrelativemove.on_ground = entity->on_ground;
            queue_push(player->outgoing_packets, pkt);
        } else if (!force_update && delta_rotation > .01 && dist > .001 && dist < 64.) {
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYLOOKANDRELATIVEMOVE);
            pkt->data.play_client.entitylookandrelativemove.entity_id = entity->id;
            pkt->data.play_client.entitylookandrelativemove.delta_x = (int16_t) ((entity->x * 32. - entity->last_x * 32.) * 128.);
            pkt->data.play_client.entitylookandrelativemove.delta_y = (int16_t) ((entity->y * 32. - entity->last_y * 32.) * 128.);
            pkt->data.play_client.entitylookandrelativemove.delta_z = (int16_t) ((entity->z * 32. - entity->last_z * 32.) * 128.);
            pkt->data.play_client.entitylookandrelativemove.yaw = (uint8_t) ((entity->yaw / 360.) * 256.);
            pkt->data.play_client.entitylookandrelativemove.pitch = (uint8_t) ((entity->pitch / 360.) * 256.);
            pkt->data.play_client.entitylookandrelativemove.on_ground = entity->on_ground;
            queue_push(player->outgoing_packets, pkt);
            pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYHEADLOOK);
            pkt->data.play_client.entityheadlook.entity_id = entity->id;
            pkt->data.play_client.entityheadlook.head_yaw = (uint8_t) ((entity->yaw / 360.) * 256.);
            queue_push(player->outgoing_packets, pkt);
        } else {
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYTELEPORT);
            pkt->data.play_client.entityteleport.entity_id = entity->id;
            pkt->data.play_client.entityteleport.x = entity->x;
            pkt->data.play_client.entityteleport.y = entity->y;
            pkt->data.play_client.entityteleport.z = entity->z;
            pkt->data.play_client.entityteleport.yaw = (uint8_t) ((entity->yaw / 360.) * 256.);
            pkt->data.play_client.entityteleport.pitch = (uint8_t) ((entity->pitch / 360.) * 256.);
            pkt->data.play_client.entityteleport.on_ground = entity->on_ground;
            queue_push(player->outgoing_packets, pkt);
            pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYHEADLOOK);
            pkt->data.play_client.entityheadlook.entity_id = entity->id;
            pkt->data.play_client.entityheadlook.head_yaw = (uint8_t) ((entity->yaw / 360.) * 256.);
            queue_push(player->outgoing_packets, pkt);
        }
    }
}

void player_hungerUpdate(struct player* player) {
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_UPDATEHEALTH);
    pkt->data.play_client.updatehealth.health = player->entity->health;
    pkt->data.play_client.updatehealth.food = player->food;
    pkt->data.play_client.updatehealth.food_saturation = player->saturation;
    queue_push(player->outgoing_packets, pkt);
}

void player_tick(struct player* player) {
    /*if (player->defunct) {
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
    }*/
    player->chunks_sent = 0;
    if (player->world->tick_counter % 200 == 0) {
        if (player->next_keep_alive != 0) {
            player->conn->disconnect = 1;
            return;
        }
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_KEEPALIVE);
        pkt->data.play_client.keepalive.keep_alive_id = rand();
        queue_push(player->outgoing_packets, pkt);
    } else if (player->world->tick_counter % 200 == 100) {
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_TIMEUPDATE);
        pkt->data.play_client.timeupdate.time_of_day = player->world->time;
        pkt->data.play_client.timeupdate.world_age = player->world->age;
        queue_push(player->outgoing_packets, pkt);
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
            } else if (player->server->difficulty > 0) {
                player->food -= 1;
                if (player->food < 0) player->food = 0;
            }
            player_hungerUpdate(player);
        }
        if (player->saturation > 0. && player->food >= 20 && player->server->difficulty == 0) {
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
                if (player->entity->health > 10. || player->server->difficulty == 3 || (player->entity->health > 1. && player->server->difficulty == 2)) damageEntity(player->entity, 1., 0);
                player->foodTimer = 0;
            }
        }
    }
    beginProfilerSection("chunks");
    int32_t chunk_x = ((int32_t) player->entity->x >> 4);
    int32_t chunk_z = ((int32_t) player->entity->z >> 4);
    int32_t last_chunk_x = ((int32_t) player->entity->last_x >> 4);
    int32_t last_chunk_z = ((int32_t) player->entity->last_z >> 4);
    if (player->loaded_chunks->entry_count == 0 || player->trigger_rechunk) { // || tick_counter % 200 == 0
        beginProfilerSection("chunkLoading_tick");
        pthread_mutex_lock(&player->world->chunk_requests->data_mutex);
        int first_chunks = player->loaded_chunks->entry_count == 0 || player->trigger_rechunk;
        if (player->trigger_rechunk) {
            pthread_rwlock_rdlock(&player->loaded_chunks->rwlock);
            ITER_MAP(player->loaded_chunks) {
                struct chunk* chunk = (struct chunk*) value;
                if (chunk->x < chunk_x - CHUNK_VIEW_DISTANCE || chunk->x > chunk_x + CHUNK_VIEW_DISTANCE || chunk->z < chunk_z - CHUNK_VIEW_DISTANCE || chunk->z > chunk_z + CHUNK_VIEW_DISTANCE) {
                    struct chunk_request* request = pcalloc(player->world->chunk_request_pool, sizeof(struct chunk_request));
                    request->chunk_x = chunk->x;
                    request->chunk_z = chunk->z;
                    request->world = player->world;
                    request->load = 0;
                    queue_push(player->world->chunk_requests, request);
                }
                ITER_MAP_END();
            }
            pthread_rwlock_unlock(&player->loaded_chunks->rwlock);
        }
        for (int r = 0; r <= CHUNK_VIEW_DISTANCE; r++) {
            int32_t x = chunk_x - r;
            int32_t z = chunk_z - r;
            for (int i = 0; i < ((r == 0) ? 1 : (r * 8)); i++) {
                if (first_chunks || !hashmap_getint(player->loaded_chunks, chunk_get_key_direct(x, z))) {
                    struct chunk_request* request = pcalloc(player->world->chunk_request_pool, sizeof(struct chunk_request));
                    request->chunk_x = x;
                    request->chunk_z = z;
                    request->world = player->world;
                    request->load = 1;
                    queue_push(player->world->chunk_requests, request);
                }
                if (i < 2 * r) x++;
                else if (i < 4 * r) z++;
                else if (i < 6 * r) x--;
                else if (i < 8 * r) z--;
            }
        }
        pthread_mutex_unlock(&player->world->chunk_requests->data_mutex);
        player->trigger_rechunk = 0;
        endProfilerSection("chunkLoading_tick");
    }
    if (last_chunk_x != chunk_x || last_chunk_z != chunk_z) {
        pthread_mutex_lock(&player->world->chunk_requests->data_mutex);
        for (int32_t fx = last_chunk_x; last_chunk_x < chunk_x ? (fx < chunk_x) : (fx > chunk_x); last_chunk_x < chunk_x ? fx++ : fx--) {
            for (int32_t fz = last_chunk_z - CHUNK_VIEW_DISTANCE; fz <= last_chunk_z + CHUNK_VIEW_DISTANCE; fz++) {
                beginProfilerSection("chunkUnloading_live");
                struct chunk_request* request = pcalloc(player->world->chunk_request_pool, sizeof(struct chunk_request));
                request->chunk_x = last_chunk_x < chunk_x ? (fx - CHUNK_VIEW_DISTANCE) : (fx + CHUNK_VIEW_DISTANCE);
                request->chunk_z = fz;
                request->load = 0;
                queue_push(player->world->chunk_requests, request);
                endProfilerSection("chunkUnloading_live");
                beginProfilerSection("chunkLoading_live");
                request = pcalloc(player->world->chunk_request_pool, sizeof(struct chunk_request));
                request->chunk_x = last_chunk_x < chunk_x ? (fx + CHUNK_VIEW_DISTANCE) : (fx - CHUNK_VIEW_DISTANCE);
                request->chunk_z = fz;
                request->world = player->world;
                request->load = 1;
                queue_push(player->world->chunk_requests, request);
                endProfilerSection("chunkLoading_live");
            }
        }
        for (int32_t fz = last_chunk_z; last_chunk_z < chunk_z ? (fz < chunk_z) : (fz > chunk_z); last_chunk_z < chunk_z ? fz++ : fz--) {
            for (int32_t fx = last_chunk_x - CHUNK_VIEW_DISTANCE; fx <= last_chunk_x + CHUNK_VIEW_DISTANCE; fx++) {
                beginProfilerSection("chunkUnloading_live");
                struct chunk_request* request = pcalloc(player->world->chunk_request_pool, sizeof(struct chunk_request));
                request->chunk_x = fx;
                request->chunk_z = last_chunk_z < chunk_z ? (fz - CHUNK_VIEW_DISTANCE) : (fz + CHUNK_VIEW_DISTANCE);
                request->load = 0;
                queue_push(player->world->chunk_requests, request);
                endProfilerSection("chunkUnloading_live");
                beginProfilerSection("chunkLoading_live");
                request = pcalloc(player->world->chunk_request_pool, sizeof(struct chunk_request));
                request->chunk_x = fx;
                request->chunk_z = last_chunk_z < chunk_z ? (fz + CHUNK_VIEW_DISTANCE) : (fz - CHUNK_VIEW_DISTANCE);
                request->load = 1;
                request->world = player->world;
                queue_push(player->world->chunk_requests, request);
                endProfilerSection("chunkLoading_live");
            }
        }
        pthread_mutex_unlock(&player->world->chunk_requests->data_mutex);
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
            if (ihi->onItemUse != NULL) (*ihi->onItemUse)(player->world, player, (uint8_t) (player->itemUseHand ? 45 : (36 + player->currentItem)), ihs, player->itemUseDuration);
            player->itemUseDuration = 0;
            player->entity->usingItemMain = 0;
            player->entity->usingItemOff = 0;
            updateMetadata(player->entity);
        } else {
            if (ihi->onItemUseTick != NULL) (*ihi->onItemUseTick)(player->world, player, (uint8_t) (player->itemUseHand ? 45 : (36 + player->currentItem)), ihs, player->itemUseDuration);
            player->itemUseDuration++;
        }
    }
    //if (((int32_t) player->entity->lx >> 4) != pcx || ((int32_t) player->entity->lz >> 4) != pcz || player->loaded_chunks->count < CHUNK_VIEW_DISTANCE * CHUNK_VIEW_DISTANCE * 4 || player->triggerRechunk) {
    //}
    if (player->digging >= 0.) {
        block bw = world_get_block(player->world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
        struct block_info* bi = getBlockInfo(bw);
        float digspeed = 0.;
        if (bi->hardness > 0.) {
            pthread_mutex_lock(&player->inventory->mutex);
            struct slot* ci = inventory_get(player, player->inventory, 36 + player->currentItem);
            struct item_info* cii = ci == NULL ? NULL : getItemInfo(ci->item);
            int hasProperTool = (cii == NULL ? 0 : tools_proficient(cii->toolType, cii->harvestLevel, bw));
            int hasProperTool2 = (cii == NULL ? 0 : tools_proficient(cii->toolType, 0xFF, bw));
            pthread_mutex_unlock(&player->inventory->mutex);
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
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_BLOCKBREAKANIMATION);
            pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
            memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
            pkt->data.play_client.blockbreakanimation.destroy_stage = (int8_t) (player->digging * 9);
            queue_push(bc_player->outgoing_packets, pkt);
        END_BROADCAST(player->world->players)
    }
    beginProfilerSection("entity_transmission");
    if (player->world->tick_counter % 20 == 0) {
        pthread_rwlock_wrlock(&player->loaded_entities->rwlock);
        ITER_MAP(player->loaded_entities)
        {
            struct entity* ent = (struct entity*) value;
            if (entity_distsq(ent, player->entity) > (CHUNK_VIEW_DISTANCE * 16.) * (CHUNK_VIEW_DISTANCE * 16.)) {
                struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_DESTROYENTITIES);
                pkt->data.play_client.destroyentities.count = 1;
                pkt->data.play_client.destroyentities.entity_ids = pmalloc(pkt->pool, sizeof(int32_t));
                pkt->data.play_client.destroyentities.entity_ids[0] = ent->id;
                queue_push(player->outgoing_packets, pkt);
                hashmap_putint(player->loaded_entities, (uint64_t) ent->id, NULL);
                hashmap_putint(ent->loadingPlayers, (uint64_t) player->entity->id, NULL);
            }
            ITER_MAP_END();
        }
        pthread_rwlock_unlock(&player->loaded_entities->rwlock);
    }
    endProfilerSection("entity_transmission");
    beginProfilerSection("player_transmission");
    ITER_MAP(player->world->entities) {
        struct entity* ent = (struct entity*) value;
        if (ent == player->entity || hashmap_getint(player->loaded_entities, (uint64_t) ent->id) || entity_distsq(player->entity, ent) > (CHUNK_VIEW_DISTANCE * 16.) * (CHUNK_VIEW_DISTANCE * 16.)) continue;
        if (ent->type == ENT_PLAYER) game_load_player(player, ent->data.player.player);
        else game_load_entity(player, ent);
        ITER_MAP_END();
    }
    endProfilerSection("player_transmission");
    ITER_MAP(plugins) {
        struct plugin* plugin = value;
        if (plugin->tick_player != NULL) (*plugin->tick_player)(player->world, player);
        ITER_MAP_END();
    }
}

int player_onGround(struct player* player) {
    struct entity* entity = player->entity;
    struct boundingbox obb;
    entity_collision_bounding_box(entity, &obb);
    if (obb.minX == obb.maxX || obb.minZ == obb.maxZ || obb.minY == obb.maxY) {
        return 0;
    }
    obb.minY += -.08;
    struct boundingbox pbb;
    entity_collision_bounding_box(entity, &pbb);
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
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_DISCONNECT);
    size_t sl = strlen(message);
    pkt->data.play_client.disconnect.reason = pmalloc(pkt->pool, sl + 128);
    snprintf(pkt->data.play_client.disconnect.reason, sl + 128, "{\"text\": \"%s\", \"color\": \"red\"}", message);
    queue_push(player->outgoing_packets, pkt);
    if (player->conn != NULL) player->conn->disconnect = 1;
    broadcastf("red", "Kicked Player %s for reason: %s", player->name, message);
}

float player_getAttackStrength(struct player* player, float adjust) {
    float cooldownPeriod = 4f;
    struct slot* sl = inventory_get(player, player->inventory, 36 + player->currentItem);
    if (sl != NULL) {
        struct item_info* ii = getItemInfo(sl->item);
        if (ii != NULL) {
            cooldownPeriod = ii->attackSpeed;
        }
    }
    float str = ((float) (player->world->tick_counter - player->lastSwing) + adjust) * cooldownPeriod / 20f;
    if (str > 1.) str = 1f;
    if (str < 0.) str = 0f;
    return str;
}

void player_teleport(struct player* player, double x, double y, double z) {
    player->entity->x = x;
    player->entity->y = y;
    player->entity->z = z;
    player->entity->last_x = x;
    player->entity->last_y = y;
    player->entity->last_z = z;
    player->trigger_rechunk = 1;
    player->spawned_in = 0;
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYVELOCITY);
    pkt->data.play_client.entityvelocity.entity_id = player->entity->id;
    pkt->data.play_client.entityvelocity.velocity_x = 0;
    pkt->data.play_client.entityvelocity.velocity_y = 0;
    pkt->data.play_client.entityvelocity.velocity_z = 0;
    queue_push(player->outgoing_packets, pkt);
    pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK);
    pkt->data.play_client.playerpositionandlook.x = player->entity->x;
    pkt->data.play_client.playerpositionandlook.y = player->entity->y;
    pkt->data.play_client.playerpositionandlook.z = player->entity->z;
    pkt->data.play_client.playerpositionandlook.yaw = player->entity->yaw;
    pkt->data.play_client.playerpositionandlook.pitch = player->entity->pitch;
    pkt->data.play_client.playerpositionandlook.flags = 0x0;
    pkt->data.play_client.playerpositionandlook.teleport_id = (int32_t) player->world->tick_counter;
    player->last_teleport_id = pkt->data.play_client.playerpositionandlook.teleport_id;
    queue_push(player->outgoing_packets, pkt);
}

struct player* player_get_by_name(struct server* server, char* name) {
    //TODO: this should be a map lookup
    pthread_rwlock_rdlock(&server->players_by_entity_id->rwlock);
    ITER_MAP (server->players_by_entity_id) {
        struct player* player = (struct player*) value;
        if (player != NULL && str_eq(name, player->name)) {
            pthread_rwlock_unlock(&server->players_by_entity_id->rwlock);
            return player;
        }
        ITER_MAP_END();
    }
    pthread_rwlock_unlock(&server->players_by_entity_id->rwlock);
    return NULL;
}

void player_closeWindow(struct player* player, uint16_t windowID) {
    struct inventory* inventory = NULL;
    if (windowID == 0 && player->open_inventory == NULL) {
        inventory = player->inventory;
    } else if (player->open_inventory != NULL && windowID == player->open_inventory->window) {
        inventory = player->open_inventory;
    }
    if (inventory != NULL) {
        pthread_mutex_lock(&inventory->mutex);
        if (player->inventory_holding != NULL) {
            dropPlayerItem(player, player->inventory_holding);
            player->inventory_holding = NULL;
        }
        if (inventory->type == INVTYPE_PLAYERINVENTORY) {
            for (int i = 1; i < 5; i++) {
                if (inventory->slots[i] != NULL) {
                    dropPlayerItem(player, inventory->slots[i]);
                    inventory_set_slot(player, inventory, i, NULL, 0);
                }
            }
        } else if (inventory->type == INVTYPE_WORKBENCH) {
            for (int i = 1; i < 10; i++) {
                if (inventory->slots[i] != NULL) {
                    dropPlayerItem(player, inventory->slots[i]);
                    inventory_set_slot(player, inventory, i, NULL, 0);
                }
            }
            pthread_mutex_unlock(&inventory->mutex);
            pfree(inventory->pool);
            inventory = NULL;
        } else if (inventory->type == INVTYPE_CHEST) {
            if (inventory->tile != NULL) {
                BEGIN_BROADCAST_DIST(player->entity, 128.){
                    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_BLOCKACTION);
                    pkt->data.play_client.blockaction.location.x = inventory->tile->x;
                    pkt->data.play_client.blockaction.location.y = inventory->tile->y;
                    pkt->data.play_client.blockaction.location.z = inventory->tile->z;
                    pkt->data.play_client.blockaction.action_id = 1;
                    pkt->data.play_client.blockaction.action_param = (uint8_t) (inventory->watching_players->entry_count - 1);
                    pkt->data.play_client.blockaction.block_type = world_get_block(player->world, inventory->tile->x, inventory->tile->y, inventory->tile->z) >> 4;
                    queue_push(bc_player->outgoing_packets, pkt);
                    END_BROADCAST(player->entity->world->players)
                }
            }
        }
        if (inventory != NULL) {
            if (inventory->type != INVTYPE_PLAYERINVENTORY) {
                hashmap_putint(inventory->watching_players, (uint64_t) player->entity->id, NULL);
            }
            pthread_mutex_unlock(&inventory->mutex);
        }
    }
    player->open_inventory = NULL; // TODO: free sometimes?

}

void player_set_gamemode(struct player* player, int gamemode) {
    if (gamemode != -1) {
        player->gamemode = (uint8_t) gamemode;
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_CHANGEGAMESTATE);
        pkt->id = PKT_PLAY_CLIENT_CHANGEGAMESTATE;
        pkt->data.play_client.changegamestate.reason = 3;
        pkt->data.play_client.changegamestate.value = gamemode;
        queue_push(player->outgoing_packets, pkt);
    }
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_PLAYERABILITIES);
    pkt->data.play_client.playerabilities.flags = (int8_t) (player->gamemode == 1 ? (0x04 | 0x08) : 0x0);
    pkt->data.play_client.playerabilities.flying_speed = .05;
    pkt->data.play_client.playerabilities.field_of_view_modifier = .1;
    queue_push(player->outgoing_packets, pkt);
}

block player_can_place_block(struct player* player, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    struct block_info* info = getBlockInfo(blk);
    block blk_updated = blk;
    if (info != NULL && info->onBlockPlacedPlayer != NULL) blk_updated = (*info->onBlockPlacedPlayer)(player, player->world, blk_updated, x, y, z, face);
    ITER_MAP(plugins) {
        struct plugin* plugin = value;
        if (plugin->onBlockPlacedPlayer != NULL) blk_updated = (*plugin->onBlockPlacedPlayer)(player, player->world, blk_updated, x, y, z, face);
        ITER_MAP_END();
    }
    info = getBlockInfo(blk);
    if (info != NULL && info->canBePlaced != NULL && !(*info->canBePlaced)(player->world, blk_updated, x, y, z)) {
        return 0;
    }
    block new_block = world_get_block(player->world, x, y, z);
    return (block) ((new_block == 0 || getBlockInfo(new_block)->material->replacable) ? blk_updated : 0);
}
