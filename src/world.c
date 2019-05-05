#include "light.h"
#include <basin/network.h>
#include <basin/packet.h>
#include <basin/game.h>
#include <basin/block.h>
#include <basin/tileentity.h>
#include <basin/entity.h>
#include <basin/world.h>
#include <basin/globals.h>
#include <basin/profile.h>
#include <basin/server.h>
#include <basin/ai.h>
#include <basin/plugin.h>
#include <basin/perlin.h>
#include <basin/biome.h>
#include <basin/entity.h>
#include <basin/worldgen.h>
#include <basin/player.h>
#include <basin/nbt.h>
#include <basin/worldmanager.h>
#include <avuna/prqueue.h>
#include <avuna/string.h>
#include <avuna/pmem.h>
#include <avuna/queue.h>
#include <avuna/hash.h>
#include <avuna/util.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <zlib.h>
#include <dirent.h>
#include <math.h>
#include <errno.h>


struct chunk* world_get_chunk(struct world* world, int32_t x, int32_t z) {
    struct chunk* ch = hashmap_getint(world->chunks, chunk_get_key_direct(x, z));
    return ch;
}

struct chunk* world_get_chunk_guess(struct world* world, struct chunk* ch, int32_t x, int32_t z) {
    if (ch == NULL) return world_get_chunk(world, x, z);
    if (ch->x == x && ch->z == z) return ch;
    if (abs(x - ch->x) > 3 || abs(z - ch->z) > 3) return world_get_chunk(world, x, z);
    struct chunk* cch = ch;
    while (cch != NULL) {
        if (cch->x > x) cch = cch->xn;
        else if (cch->x < x) cch = cch->xp;
        if (cch != NULL) {
            if (cch->z > z) cch = cch->zn;
            else if (cch->z < z) cch = cch->zp;
        }
        if (cch != NULL && cch->x == x && cch->z == z) return cch;
    }
    return world_get_chunk(world, x, z);
}


// WARNING: you almost certainly do not want to call this function. (hence not present in header)
// chunks are loaded asynchronously
// WARNING: this function should only be called from the chunk loading thread. *it is not thread safe*
struct chunk* world_load_chunk(struct world* world, int32_t x, int32_t z) {
    struct chunk* chunk = hashmap_getint(world->chunks, chunk_get_key_direct(x, z));
    if (chunk != NULL) return chunk;
    int16_t region_x = (int16_t) (x >> 5);
    int16_t region_z = (int16_t) (z >> 5);
    uint64_t region_index = (((uint64_t)(region_x) & 0xFFFF) << 16) | (((uint64_t) region_z) & 0xFFFF);
    struct region* region = hashmap_getint(world->regions, region_index);
    if (region == NULL) {
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/region/r.%i.%i.mca", world->world_folder, region_x, region_z);
        region = region_new(world->pool, path, region_x, region_z);
        hashmap_putint(world->regions, region_index, region);
    }
    chunk = region_load_chunk(region, (int8_t) (x & 0x1F), (int8_t) (z & 0x1F));
    if (chunk == NULL) {
        struct mempool* pool = mempool_new();
        pchild(region->pool, pool);
        chunk = chunk_new(pool, (int16_t) x, (int16_t) z);
        chunk = worldgen_gen_chunk(world, chunk);
    }
    if (chunk != NULL) {
        chunk->xp = world_get_chunk(world, x + 1, z);
        if (chunk->xp != NULL) chunk->xp->xn = chunk;
        chunk->xn = world_get_chunk(world, x - 1, z);
        if (chunk->xn != NULL) chunk->xn->xp = chunk;
        chunk->zp = world_get_chunk(world, x, z + 1);
        if (chunk->zp != NULL) chunk->zp->zn = chunk;
        chunk->zn = world_get_chunk(world, x, z - 1);
        if (chunk->zn != NULL) chunk->zn->zp = chunk;
        hashmap_putint(world->chunks, chunk_get_key(chunk), chunk);
        return chunk;
    }
    return NULL;
}

void world_chunkload_thread(struct world* world) {
    //TODO: ensure that on world change that the chunk queue is cleared for a player
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (1) {
        struct chunk_request* request = queue_pop(world->chunk_requests);
        if (request->load) {
            beginProfilerSection("chunkLoading_getChunk");
            // TODO: cache chunk network encodings/compression?
            // we probably send a chunk a lot more often than it gets mutated
            struct chunk* chunk = world_get_chunk(world, request->chunk_x, request->chunk_z);
            if (chunk == NULL) {
                ITER_MAP(plugins) {
                    struct plugin* plugin = value;
                    if (plugin->pre_chunk_loaded != NULL) {
                        struct chunk* new_chunk = plugin->pre_chunk_loaded(world, request->chunk_x, request->chunk_z);
                        if (new_chunk != NULL) {
                            chunk = new_chunk;
                        }
                    }
                    ITER_MAP_END();
                }
            }
            if (chunk == NULL) {
                chunk = world_load_chunk(request->world, request->chunk_x, request->chunk_z);
            }
            ITER_MAP(plugins) {
                struct plugin* plugin = value;
                if (plugin->post_chunk_loaded != NULL) {
                    struct chunk* new_chunk = plugin->post_chunk_loaded(world, chunk);
                    if (new_chunk != NULL) {
                        chunk = new_chunk;
                    }
                }
                ITER_MAP_END();
            }
            if (request->callback != NULL && chunk != NULL) {
                request->callback(request->request_arg, world, chunk);
            }
            endProfilerSection("chunkLoading_getChunk");
        } else {
            struct chunk* chunk = world_get_chunk(request->world, request->chunk_x, request->chunk_z);
            ITER_MAP(plugins) {
                struct plugin* plugin = value;
                if (plugin->pre_chunk_unload != NULL) {
                    if (plugin->pre_chunk_unload(world, chunk)) {
                        chunk = NULL;
                    }
                }
                ITER_MAP_END();
            }
            if (chunk != NULL && --chunk->playersLoaded <= 0) {
                world_unload_chunk(request->world, chunk);
            }
        }
        pprefree(world->chunk_request_pool, request);
/*
        BEGIN_HASHMAP_ITERATION (players)
        struct player* player = value;
        if (player->defunct || player->chunksSent >= 5 || player->chunkRequests->size == 0 || (player->conn != NULL && player->conn->writeBuffer_size > 1024 * 1024 * 128)) continue;
        struct chunk_request* chr = pop_nowait_queue(player->chunkRequests);
        if (chr == NULL) continue;
        if (chr->load) {
            if (chr->world != player->world) {
                xfree(chr);
                continue;
            }
            player->chunksSent++;
            if (contains_hashmap(player->loaded_chunks, chunk_get_key_direct(chr->chunk_x, chr->chunk_z))) {
                xfree(chr);
                continue;
            }
            beginProfilerSection("chunkLoading_getChunk");
            struct chunk* ch = world_load_chunk(player->world, chr->chunk_x, chr->chunk_z, b);
            if (player->loaded_chunks == NULL) {
                xfree(chr);
                endProfilerSection("chunkLoading_getChunk");
                continue;
            }
            endProfilerSection("chunkLoading_getChunk");
            if (ch != NULL) {
                ch->playersLoaded++;
                //beginProfilerSection("chunkLoading_sendChunk_add");
                put_hashmap(player->loaded_chunks, chunk_get_key(ch), ch);
                //endProfilerSection("chunkLoading_sendChunk_add");
                //beginProfilerSection("chunkLoading_sendChunk_malloc");
                struct packet* pkt = xmalloc(sizeof(struct packet));
                pkt->id = PKT_PLAY_CLIENT_CHUNKDATA;
                pkt->data.play_client.chunkdata.data = ch;
                pkt->data.play_client.chunkdata.cx = ch->x;
                pkt->data.play_client.chunkdata.cz = ch->z;
                pkt->data.play_client.chunkdata.ground_up_continuous = 1;
                pkt->data.play_client.chunkdata.number_of_block_entities = ch->tileEntities->count;
                //endProfilerSection("chunkLoading_sendChunk_malloc");
                //beginProfilerSection("chunkLoading_sendChunk_tileEntities");
                pkt->data.play_client.chunkdata.block_entities = ch->tileEntities->count == 0 ? NULL : xmalloc(sizeof(struct nbt_tag*) * ch->tileEntities->count);
                size_t ri = 0;
                for (size_t i = 0; i < ch->tileEntities->size; i++) {
                    if (ch->tileEntities->data[i] == NULL) continue;
                    struct tile_entity* te = ch->tileEntities->data[i];
                    pkt->data.play_client.chunkdata.block_entities[ri++] = tile_serialize(te, 1);
                }
                //endProfilerSection("chunkLoading_sendChunk_tileEntities");
                //beginProfilerSection("chunkLoading_sendChunk_dispatch");
                add_queue(player->outgoing_packets, pkt);
                flush_outgoing(player);
                //endProfilerSection("chunkLoading_sendChunk_dispatch");
            }
        } else {
            //beginProfilerSection("unchunkLoading");
            struct chunk* ch = world_get_chunk(player->world, chr->chunk_x, chr->chunk_z);
            uint64_t ck = chunk_get_key_direct(chr->chunk_x, chr->chunk_z);
            if (get_hashmap(player->loaded_chunks, ck) == NULL) {
                xfree(chr);
                continue;
            }
            put_hashmap(player->loaded_chunks, ck, NULL);
            if (ch != NULL && !ch->defunct) {
                if (--ch->playersLoaded <= 0) {
                    world_unload_chunk(player->world, ch);
                }
            }
            if (chr->world == player->world) {
                struct packet* pkt = xmalloc(sizeof(struct packet));
                pkt->id = PKT_PLAY_CLIENT_UNLOADCHUNK;
                pkt->data.play_client.unloadchunk.chunk_x = chr->chunk_x;
                pkt->data.play_client.unloadchunk.chunk_z = chr->chunk_z;
                pkt->data.play_client.unloadchunk.ch = NULL;
                add_queue(player->outgoing_packets, pkt);
                flush_outgoing(player);
            }
            //endProfilerSection("unchunkLoading");
        }
        xfree(chr);
        END_HASHMAP_ITERATION (players)
        */
    }
#pragma clang diagnostic pop
}

// WARNING: this function should only be called from the chunk loading thread. *it is not thread safe*
void world_unload_chunk(struct world* world, struct chunk* chunk) {
//TODO: save chunk
    pthread_rwlock_wrlock(&world->chunks->rwlock);
    if (chunk->xp != NULL) chunk->xp->xn = NULL;
    if (chunk->xn != NULL) chunk->xn->xp = NULL;
    if (chunk->zp != NULL) chunk->zp->zn = NULL;
    if (chunk->zn != NULL) chunk->zn->zp = NULL;
    pthread_rwlock_unlock(&world->chunks->rwlock);
    hashmap_putint(world->chunks, chunk_get_key(chunk), NULL);
    pfree(chunk->pool);
}

int world_get_biome(struct world* world, int32_t x, int32_t z) {
    struct chunk* chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return 0;
    return chunk->biomes[z & 0x0f][x & 0x0f];
}


uint8_t world_get_light_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 0;
    ch = world_get_chunk_guess(world, ch, x >> 4, z >> 4);
    if (ch != NULL) return chunk_get_light(ch, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f), world->skylightSubtracted);
    else return world_get_light(world, x, y, z, 0);
}

uint8_t world_get_raw_light_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
    if (y < 0 || y > 255) return 0;
    ch = world_get_chunk_guess(world, ch, x >> 4, z >> 4);
    if (ch != NULL) return chunk_get_raw_light(ch, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f), blocklight);
    else return world_get_raw_light(world, x, y, z, blocklight);
}

uint16_t world_height_guess(struct world* world, struct chunk* ch, int32_t x, int32_t z) {
    if (world->dimension != OVERWORLD) return 0;
    ch = world_get_chunk_guess(world, ch, x >> 4, z >> 4);
    if (ch == NULL) return 0;
    return ch->heightMap[z & 0x0f][x & 0x0f];
}

void world_set_light_guess(struct world* world, struct chunk* ch, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
    if (y < 0 || y > 255) return;
    ch = world_get_chunk_guess(world, ch, x >> 4, z >> 4);
    if (ch != NULL) return chunk_set_light(ch, (uint8_t) (light & 0x0f), (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f), blocklight, (uint8_t) (world->dimension == 0));
    else return world_set_light(world, (uint8_t) (light & 0x0f), x, y, z, blocklight);
}

void world_set_light(struct world* world, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
    if (y < 0 || y > 255) return;
    struct chunk* chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return;
    chunk_set_light(chunk, (uint8_t) (light & 0x0f), (uint8_t) (x & 0x0f), (uint8_t) (y > 255 ? 255 : y), (uint8_t) (z & 0x0f), blocklight, (uint8_t) (world->dimension == 0));
}

uint8_t world_get_light(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t checkNeighbors) {
    if (y < 0 || y > 255) return 0;
    struct chunk* chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return 15;
    if (checkNeighbors) {
        uint8_t yp = chunk_get_light(chunk, (uint8_t) (x & 0x0f), y, (uint8_t) (z & 0x0f), world->skylightSubtracted);
        uint8_t xp = world_get_light_guess(world, chunk, x + 1, y, z);
        uint8_t xn = world_get_light_guess(world, chunk, x - 1, y, z);
        uint8_t zp = world_get_light_guess(world, chunk, x, y, z + 1);
        uint8_t zn = world_get_light_guess(world, chunk, x, y, z - 1);
        if (xp > yp) yp = xp;
        if (xn > yp) yp = xn;
        if (zp > yp) yp = zp;
        if (zn > yp) yp = zn;
        return yp;
    } else if (y < 0) return 0;
    else {
        return chunk_get_light(chunk, (uint8_t) (x & 0x0f), (uint8_t) (y > 255 ? 255 : y), (uint8_t) (z & 0x0f), world->skylightSubtracted);
    }
}

uint8_t world_get_raw_light(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
    if (y < 0 || y > 255) return 0;
    struct chunk* chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return 15;
    return chunk_get_raw_light(chunk, (uint8_t) (x & 0x0f), (uint8_t) (y > 255 ? 255 : y), (uint8_t) (z & 0x0f), blocklight);
}

block world_get_block(struct world* world, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 0;
    struct chunk* chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return 0;
    return chunk_get_block(chunk, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f));
}

block world_get_block_guess(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 0;
    chunk = world_get_chunk_guess(world, chunk, x >> 4, z >> 4);
    if (chunk != NULL) return chunk_get_block(chunk, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f));
    else return world_get_block(world, x, y, z);
}

void world_explode(struct world* world, struct chunk* ch, double x, double y, double z, float strength) { // TODO: more plugin stuff?
    ch = world_get_chunk_guess(world, ch, (int32_t) floor(x) >> 4, (int32_t) floor(z) >> 4);
    for (int32_t j = 0; j < 16; j++) {
        for (int32_t k = 0; k < 16; k++) {
            for (int32_t l = 0; l < 16; l++) {
                if (!(j == 0 || j == 15 || k == 0 || k == 15 || l == 0 || l == 15)) continue;
                double dx = (double) j / 15.0 * 2.0 - 1.0;
                double dy = (double) k / 15.0 * 2.0 - 1.0;
                double dz = (double) l / 15.0 * 2.0 - 1.0;
                double d = sqrt(dx * dx + dy * dy + dz * dz);
                dx /= d;
                dy /= d;
                dz /= d;
                float modified_strength = strength * (.7f + game_rand_float() * .6f);
                double x2 = x;
                double y2 = y;
                double z2 = z;
                for (; modified_strength > 0.; modified_strength -= .225) {
                    int32_t ix = (int32_t) floor(x2);
                    int32_t iy = (int32_t) floor(y2);
                    int32_t iz = (int32_t) floor(z2);
                    block b = world_get_block_guess(world, ch, ix, iy, iz);
                    if (b >> 4 != 0) {
                        struct block_info* bi = getBlockInfo(b);
                        modified_strength -= ((bi->resistance / 5.) + .3) * .3;
                        if (modified_strength > 0.) { //TODO: check if entity will allow destruction
                            block b = world_get_block_guess(world, ch, ix, iy, iz);
                            world_set_block_guess(world, ch, 0, ix, iy, iz);
                            dropBlockDrops(world, b, NULL, ix, iy, iz); //TODO: randomizE?
                        }
                    }
                    x2 += dx * .3;
                    y2 += dy * .3;
                    z2 += dz * .3;
                }
            }
        }
    }
    //TODO: knockback & damage
}

void world_tile_set_tickable(struct world* world, struct tile_entity* tile) {
    if (tile == NULL || tile->y > 255 || tile->y < 0) return;
    struct chunk* chunk = world_get_chunk(world, tile->x >> 4, tile->z >> 4);
    if (chunk == NULL) return;
    uint64_t key = (uint64_t) (tile->x & 0x0f) << 16 | (uint64_t) tile->y << 8 | (uint64_t) (tile->z & 0x0f);
    hashmap_putint(chunk->tileEntitiesTickable, key, tile);
}

void world_tile_unset_tickable(struct world* world, struct tile_entity* tile) {
    if (tile == NULL || tile->y > 255 || tile->y < 0) return;
    struct chunk* chunk = world_get_chunk(world, tile->x >> 4, tile->z >> 4);
    if (chunk == NULL) return;
    uint64_t key = (uint64_t) (tile->x & 0x0f) << 16 | (uint64_t) tile->y << 8 | (uint64_t) (tile->z & 0x0f);
    hashmap_putint(chunk->tileEntitiesTickable, key, NULL);
}

struct tile_entity* world_get_tile(struct world* world, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return NULL;
    struct chunk* chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return NULL;
    return chunk_get_tile(chunk, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f));
}

void world_set_tile(struct world* world, int32_t x, int32_t y, int32_t z, struct tile_entity* tile) {
    if (y < 0 || y > 255) return;
    struct chunk* chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return;
    chunk_set_tile(chunk, tile, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f));
}

int world_set_block(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 1;
    struct chunk* ch = world_get_chunk(world, x >> 4, z >> 4);
    if (ch == NULL) return 1;
    return world_set_block_guess(world, ch, blk, x, y, z);
}

int world_set_block_noupdate(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 1;
    struct chunk* ch = world_get_chunk(world, x >> 4, z >> 4);
    if (ch == NULL) return 1;
    return world_set_block_guess_noupdate(world, ch, blk, x, y, z);
}

void world_schedule_block_tick(struct world* world, int32_t x, int32_t y, int32_t z, int32_t ticksFromNow, float priority) {
    if (y < 0 || y > 255) return;
    struct scheduled_tick* tick = pmalloc(world->pool, sizeof(struct scheduled_tick));
    tick->x = x;
    tick->y = y;
    tick->z = z;
    tick->ticksLeft = ticksFromNow;
    tick->priority = priority;
    tick->src = world_get_block(world, x, y, z);
    struct encpos pos;
    pos.x = x;
    pos.y = y;
    pos.z = z;
    hashmap_putint(world->scheduledTicks, *((uint64_t*) &pos), tick);
}

int32_t world_is_block_tick_scheduled(struct world* world, int32_t x, int32_t y, int32_t z) {
    struct encpos pos;
    pos.x = x;
    pos.y = y;
    pos.z = z;
    struct scheduled_tick* tick = hashmap_getint(world->scheduledTicks, *((uint64_t*) &pos));
    if (tick == NULL) {
        return 0;
    }
    return tick->ticksLeft;
}


int world_set_block_guess(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 1;
    chunk = world_get_chunk_guess(world, chunk, x >> 4, z >> 4);
    if (chunk == NULL) chunk = world_get_chunk(world, x >> 4, z >> 4);
    if (chunk == NULL) return 1;
    block current_block = chunk_get_block(chunk, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f));
    struct block_info* current_info = getBlockInfo(current_block);
    uint16_t heightmap_y = (uint16_t) (world->dimension == OVERWORLD ? chunk->heightMap[z & 0x0f][x & 0x0f] : 0);
    // struct world_lightpos lp;
    // int pchm = 0;
    // struct hashmap* light_updates = NULL;
    if (current_info != NULL) {
        int ict = 0;
        if (current_block != blk) {
            if (current_info->onBlockDestroyed != NULL) ict = (*current_info->onBlockDestroyed)(world, current_block, x, y, z, blk);
            if (!ict) {
                ITER_MAP (plugins) {
                    struct plugin* plugin = value;
                    if (plugin->onBlockDestroyed != NULL && (*plugin->onBlockDestroyed)(world, current_block, x, y, z, blk)) {
                        ict = 1;
                        goto break_map_iter;
                    }
                    ITER_MAP_END();
                }
                break_map_iter:;
            }
            if (ict) return 1;
        }
        /*if (world->dimension == OVERWORLD && ((y >= heightmap_y && current_info->lightOpacity >= 1) || (y < heightmap_y && current_info->lightOpacity == 0))) {
            pchm = 1;
            // light_updates = hashmap_new(8, )
            lp.x = x;
            lp.y = heightmap_y;
            lp.z = z;
            light_floodfill(world, chunk, &lp, 1, 15, light_updates);
        }*/
    }
    restart_block_placed: ;
    struct block_info* new_info = getBlockInfo(blk);
    if (new_info != NULL && blk != current_block) {
        int plugin_cancelled = 0;
        block obb = blk;
        if (new_info->onBlockPlaced != NULL) blk = (*new_info->onBlockPlaced)(world, blk, x, y, z, current_block);
        if (blk == 0 && obb != 0) return 1;
        else if (blk != obb) goto restart_block_placed;
        ITER_MAP (plugins) {
            struct plugin* plugin = value;
            if (plugin->onBlockPlaced != NULL) {
                blk = (*plugin->onBlockPlaced)(world, blk, x, y, z, current_block);
                if (blk == 0 && obb != 0) {
                    return 1;
                } else if (blk != obb) goto restart_block_placed;
            }
            ITER_MAP_END();
        }
    }
    chunk_set_block(chunk, blk, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f), world->dimension == 0);
    uint16_t nhm = (uint16_t) (world->dimension == OVERWORLD ? chunk->heightMap[z & 0x0f][x & 0x0f] : 0);
    BEGIN_BROADCAST_DISTXYZ((double) x + .5, (double) y + .5, (double) z + .5, world->players, CHUNK_VIEW_DISTANCE * 16.)
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_BLOCKCHANGE);
    pkt->data.play_client.blockchange.location.x = x;
    pkt->data.play_client.blockchange.location.y = y;
    pkt->data.play_client.blockchange.location.z = z;
    pkt->data.play_client.blockchange.block_id = blk;
    queue_push(bc_player->outgoing_packets, pkt);
    END_BROADCAST(world->players)
    beginProfilerSection("block_update");
    world_update_block_guess(world, chunk, x, y, z);
    world_update_block_guess(world, chunk, x + 1, y, z);
    world_update_block_guess(world, chunk, x - 1, y, z);
    world_update_block_guess(world, chunk, x, y, z + 1);
    world_update_block_guess(world, chunk, x, y, z - 1);
    world_update_block_guess(world, chunk, x, y + 1, z);
    world_update_block_guess(world, chunk, x, y - 1, z);
    endProfilerSection("block_update");
    beginProfilerSection("skylight_update");
    if (new_info == NULL || current_info == NULL) return 0;
    // if (world->dimension == OVERWORLD) {
        // if (pchm || current_info->lightOpacity != new_info->lightOpacity) {
            /*setLightWorld_guess(world, chunk, 15, x, nhm, z, 0);
             struct world_lightpos lp;
             if (heightmap_y < nhm) {
             for (int y = heightmap_y; y < nhm; y++) {
             setLightWorld_guess(world, chunk, 0, x, y, z, 0);
             }
             for (int y = heightmap_y; y < nhm; y++) {
             lp.x = x;
             lp.y = y;
             lp.z = z;
             light_floodfill(world, chunk, &lp, 1);
             }
             } else {
             for (int y = nhm; y < heightmap_y; y++) {
             world_set_light_guess(world, chunk, 15, x, y, z, 0);
             }
             for (int y = nhm; y < heightmap_y; y++) {
             lp.x = x;
             lp.y = y;
             lp.z = z;
             light_floodfill(world, chunk, &lp, 1);
             }
             }
             lp.x = x;
             lp.y = nhm;
             lp.z = z;
             light_update(world, chunk, &lp, 1);*/

            // beginProfilerSection("skylight_rst");
            /*for (int32_t lx = x - 16; lx <= x + 16; lx++) {
             for (int32_t ly = heightmap_y - 16; ly <= heightmap_y + 16; ly++) {
             for (int32_t lz = z - 16; lz <= z + 16; lz++) {
             world_set_light_guess(world, chunk, 0, lx, ly, lz, 0);
             }
             }
             }*/
            /*for (int32_t lx = x - 16; lx <= x + 16; lx++) {
             for (int32_t lz = z - 16; lz <= z + 16; lz++) {
             int16_t hm = world_height_guess(world, chunk, lx, lz);
             int16_t lb = heightmap_y - 16;
             int16_t ub = heightmap_y + 16;
             if (hm < ub && hm > lb) ub = hm;
             for (int32_t ly = lb; ly <= ub; ly++) {
             if (world_get_raw_light_guess(world, chunk, lx, ly, lz, 0) != 0) world_set_light_guess(world, chunk, 0, lx, ly, lz, 0);
             }
             }
             }*/
            //world_set_light_guess(world, chunk, 0, x, heightmap_y, z, 0);
            /*if (pchm) { // todo: remove the light before block change
                BEGIN_HASHMAP_ITERATION (light_updates)
                struct world_lightpos* nlp = value;
                light_floodfill(world, chunk, nlp, 1, 0, 0);
                xfree (value);
                END_HASHMAP_ITERATION (light_updates)
                del_hashmap(light_updates);
                //light_update(world, chunk, &lp, 1, 0, 0);
                world_set_light_guess(world, chunk, 15, x, nhm, z, 0);
                lp.x = x;
                lp.y = nhm;
                lp.z = z;
                light_floodfill(world, chunk, &lp, 1, 0, NULL);
            }
            lp.x = x;
            lp.y = y;
            lp.z = z;
            light_floodfill(world, chunk, &lp, 1, 0, NULL);
            endProfilerSection("skylight_rst");*/
            //TODO: pillar lighting?
            /*beginProfilerSection("skylight_set");
             for (int32_t lz = z - 16; lz <= z + 16; lz++) {
             for (int32_t lx = x - 16; lx <= x + 16; lx++) {
             uint16_t hm = world_height_guess(world, chunk, lx, lz);
             if (hm > 255) continue;
             world_set_light_guess(world, chunk, 15, lx, hm, lz, 0);
             }
             }
             endProfilerSection("skylight_set");*/
            /*beginProfilerSection("skylight_fill");
             for (int32_t lz = z - 16; lz <= z + 16; lz++) {
             for (int32_t lx = x - 16; lx <= x + 16; lx++) {
             uint16_t hm = world_height_guess(world, chunk, lx, lz);
             if (hm > 255) continue;
             lp.x = lx;
             lp.y = hm;
             lp.z = lz;
             light_update(world, chunk, &lp, 1, 0, 0);
             }
             }
             endProfilerSection("skylight_fill");*/
        // }
    // }
    // endProfilerSection("skylight_update");
    beginProfilerSection("blocklight_update");
    if (current_info->lightEmission != new_info->lightEmission || current_info->lightOpacity != new_info->lightOpacity) {
        light_update(world, chunk, x, y, z, 1, new_info->lightEmission);
        /*if (current_info->lightEmission <= new_info->lightEmission) {
            beginProfilerSection("blocklight_update_equals");
            struct world_lightpos lp;
            lp.x = x;
            lp.y = y;
            lp.z = z;
            light_floodfill(world, chunk, &lp, 0, 0, NULL);
            endProfilerSection("blocklight_update_equals");
        } else {
            beginProfilerSection("blocklight_update_remlight");
            light_updates = new_hashmap(1, 0);
            lp.x = x;
            lp.y = y;
            lp.z = z;
            light_floodfill(world, chunk, &lp, 0, current_info->lightEmission, light_updates); // todo remove light_updates duplicates
            BEGIN_HASHMAP_ITERATION (light_updates)
            struct world_lightpos* nlp = value;
            light_floodfill(world, chunk, nlp, 0, 0, 0);
            xfree (value);
            END_HASHMAP_ITERATION (light_updates)
            del_hashmap(light_updates);*/
            //light_update(world, chunk, &lp, 1, 0, 0);
            /*for (int32_t lx = x - 16; lx <= x + 16; lx++) {
             for (int32_t ly = y - 16; ly <= y + 16; ly++) {
             for (int32_t lz = z - 16; lz <= z + 16; lz++) {
             world_set_light_guess(world, chunk, 0, lx, ly, lz, 1);
             }
             }
             }
             for (int32_t lx = x - 16; lx <= x + 16; lx++) {
             for (int32_t ly = y - 16; ly <= y + 16; ly++) {
             struct world_lightpos lp;
             lp.x = lx;
             lp.y = ly;
             lp.z = z - 16;
             light_floodfill(world, chunk, &lp, 0, 0, NULL);
             lp.z = z + 16;
             light_floodfill(world, chunk, &lp, 0, 0, NULL);
             }
             }
             for (int32_t lz = z - 16; lz <= z + 16; lz++) {
             for (int32_t ly = y - 16; ly <= y + 16; ly++) {
             struct world_lightpos lp;
             lp.x = x - 16;
             lp.y = ly;
             lp.z = lz;
             light_floodfill(world, chunk, &lp, 0, 0, NULL);
             lp.x = x + 16;
             light_floodfill(world, chunk, &lp, 0, 0, NULL);
             }
             }
             for (int32_t lz = z - 16; lz <= z + 16; lz++) {
             for (int32_t lx = x - 16; lx <= x + 16; lx++) {
             struct world_lightpos lp;
             lp.x = lx;
             lp.y = y - 16;
             lp.z = lz;
             light_floodfill(world, chunk, &lp, 0, 0, NULL);
             lp.y = y + 16;
             light_update(world, chunk, &lp, 0, 0, NULL);
             }
             }*/
            //endProfilerSection("blocklight_update_remlight");
        //}
    }
    endProfilerSection("blocklight_update");
    return 0;
}

int world_set_block_guess_noupdate(struct world* world, struct chunk* chunk, block requested_block, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 1;
    chunk = world_get_chunk_guess(world, chunk, x >> 4, z >> 4);
    if (chunk == NULL) return 1;
    block original_block = chunk_get_block(chunk, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f));
    struct block_info* original_info = getBlockInfo(original_block);
    if (original_info != NULL) {
        int skip_destroyed = 0;
        if (original_block != requested_block) {
            if (original_info->onBlockDestroyed != NULL) {
                skip_destroyed = (*original_info->onBlockDestroyed)(world, original_block, x, y, z, requested_block);
            }
            if (!skip_destroyed) {
                ITER_MAP(plugins) {
                    struct plugin* plugin = value;
                    if (plugin->onBlockDestroyed != NULL && (*plugin->onBlockDestroyed)(world, original_block, x, y, z, requested_block)) {
                        skip_destroyed = 1;
                        goto post_plugin_destroyed;
                    }
                    ITER_MAP_END();
                }
                post_plugin_destroyed:;
            }
            if (skip_destroyed) return 1;
        }
    }
    restart_stage_2: ;
    struct block_info* requested_info = getBlockInfo(requested_block);
    if (requested_info != NULL && requested_block != original_block) {
        int skip_placed = 0;
        block old_block = requested_block;
        if (requested_info->onBlockPlaced != NULL) requested_block = (*requested_info->onBlockPlaced)(world, requested_block, x, y, z, original_block);
        if (requested_block == 0 && old_block != 0) skip_placed = 1;
        else if (requested_block != old_block) goto restart_stage_2;
        if (!skip_placed) {
            ITER_MAP(plugins) {
                struct plugin* plugin = value;
                if (plugin->onBlockPlaced != NULL) {
                    requested_block = (*plugin->onBlockPlaced)(world, requested_block, x, y, z, original_block);
                    if (requested_block == 0 && old_block != 0) {
                        skip_placed = 1;
                        goto post_plugin_place;
                    } else if (requested_block != old_block) goto restart_stage_2;
                }
                ITER_MAP_END();
            }
            post_plugin_place:;
        }
        if (skip_placed) return 1;
    }
    chunk_set_block(chunk, requested_block, (uint8_t) (x & 0x0f), (uint8_t) y, (uint8_t) (z & 0x0f), world->dimension == 0);
    BEGIN_BROADCAST_DISTXYZ((double) x + .5, (double) y + .5, (double) z + .5, world->players, CHUNK_VIEW_DISTANCE * 16.)
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_BLOCKCHANGE);
    pkt->data.play_client.blockchange.location.x = x;
    pkt->data.play_client.blockchange.location.y = y;
    pkt->data.play_client.blockchange.location.z = z;
    pkt->data.play_client.blockchange.block_id = requested_block;
    queue_push(bc_player->outgoing_packets, pkt);
    END_BROADCAST(world->players)
    return 0;
}

struct world* world_new(struct server* server) {
    struct mempool* pool = mempool_new();
    pchild(server->pool, pool);
    struct world* world = pcalloc(pool, sizeof(struct world));
    world->server = server;
    world->pool = pool;
    world->regions = hashmap_thread_new(16, world->pool);
    world->entities = hashmap_thread_new(64, world->pool);
    world->players = hashmap_thread_new(32, world->pool);
    world->chunks = hashmap_thread_new(32, world->pool);
    world->skylightSubtracted = 0;
    world->scheduledTicks = hashmap_thread_new(16, world->pool);
    world->tps = 0;
    world->ticksInSecond = 0;
    world->seed = 9876543;
    perlin_init(&world->perlin, world->seed);
    world->chunk_request_pool = mempool_new();
    pchild(world->pool, world->chunk_request_pool);
    world->chunk_requests = queue_new(0, 1, world->chunk_request_pool);
    return world;
}

int world_load(struct world* world, char* path) {
    char level_path[PATH_MAX];
    snprintf(level_path, PATH_MAX, "%s/level.dat", path); // could have a double slash, but its okay
    size_t level_length = 0;
    uint8_t* level_file = read_file_fully(world->pool, level_path, &level_length);
    if (level_file == NULL) {
        return -1;
    }
    unsigned char* decompressed_level = NULL;
    ssize_t decompressed_size = nbt_decompress(world->pool, level_file, level_length, (void**) &decompressed_level);
    pprefree(world->pool, level_file);
    if (decompressed_size < 0) {
        return -1;
    }
    if (nbt_read(world->pool, &world->level, decompressed_level, (size_t) decompressed_size) < 0) return -1;
    pprefree(world->pool, decompressed_level);
    struct nbt_tag* data = nbt_get(world->level, "Data");
    world->level_type = nbt_get(data, "generatorName")->data.nbt_string;
    world->spawnpos.x = nbt_get(data, "SpawnX")->data.nbt_int;
    world->spawnpos.y = nbt_get(data, "SpawnY")->data.nbt_int;
    world->spawnpos.z = nbt_get(data, "SpawnZ")->data.nbt_int;
    world->time = (uint64_t) nbt_get(data, "DayTime")->data.nbt_long;
    world->age = (uint64_t) nbt_get(data, "Time")->data.nbt_long;
    world->world_folder = str_dup(path, 0, world->pool);
    pthread_mutex_init(&world->tick_mut, NULL);
    phook(world->pool, (void (*)(void*)) pthread_mutex_destroy, &world->tick_mut);
    pthread_cond_init(&world->tick_cond, NULL);
    phook(world->pool, (void (*)(void*)) pthread_cond_destroy, &world->tick_cond);
    acclog(world->server->logger, "spawn: %i, %i, %i", world->spawnpos.x, world->spawnpos.y, world->spawnpos.z);
    snprintf(level_path, PATH_MAX, "%s/region/", path);
    DIR* dir = opendir(level_path);
    if (dir != NULL) {
        struct dirent* dirent = NULL;
        while ((dirent = readdir(dir)) != NULL) {
            if (!str_suffixes(dirent->d_name, ".mca")) continue;
            snprintf(level_path, PATH_MAX, "%s/region/%s", path, dirent->d_name);
            char* xs = strstr(level_path, "/r.") + 3;
            char* zs = strchr(xs, '.') + 1;
            if (zs == NULL) continue;
            struct region* reg = region_new(world->pool, level_path, (int16_t) strtol(xs, NULL, 10), (int16_t) strtol(zs, NULL, 10));
            uint64_t region_index = (((uint64_t)(reg->x) & 0xFFFF) << 16) | (((uint64_t) reg->z) & 0xFFFF);
            hashmap_putint(world->regions, region_index, reg);
        }
        closedir(dir);
    }
    ITER_MAP(plugins) {
        struct plugin* plugin = value;
        if (plugin->onWorldLoad != NULL) (*plugin->onWorldLoad)(world);
        ITER_MAP_END()
    }
    return 0;
}

void world_pretick(struct world* world) {
    if (++world->time >= 24000) world->time = 0;
    world->age++;
    float pday = ((float) world->time / 24000.f) - .25f;
    if (pday < 0.f) pday++;
    if (pday > 1.f) pday--;
    float cel_angle = 1.f - ((cosf(pday * (float) M_PI) + 1.f) / 2.f);
    cel_angle = pday + (cel_angle - pday) / 3.f;
    float psubs = 1.f - (cosf(cel_angle * (float) M_PI * 2.f) * 2.f + .5f);
    if (psubs < 0.f) psubs = 0.f;
    if (psubs > 1.f) psubs = 1.f;
//TODO: rain, thunder
    world->skylightSubtracted = (uint8_t)(psubs * 11.f);
}

void world_tick(struct world* world) {
    int32_t lcg = rand();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (1) {
        pthread_mutex_lock(&world->tick_mut);
        pthread_cond_wait(&world->tick_cond, &world->tick_mut);
        pthread_mutex_unlock(&world->tick_mut);
        beginProfilerSection("world_tick");
        if (world->tick_counter % 20 == 0) {
            world->tps = world->ticksInSecond;
            world->ticksInSecond = 0;
        }
        ++world->tick_counter;
        ++world->ticksInSecond;
        world_pretick(world);
        beginProfilerSection("player_receive_packet");
        pthread_rwlock_rdlock(&world->players->rwlock);
        ITER_MAP(world->players) {
            struct player* player = (struct player*) value;
            if (player->incoming_packets->size == 0) continue;
            struct packet* packet = queue_maybepop(player->incoming_packets);
            while (packet != NULL) {
                player_receive_packet(player, packet);
                pfree(packet->pool);
                packet = queue_maybepop(player->incoming_packets);
            }
            ITER_MAP_END();
        }
        pthread_rwlock_unlock(&world->players->rwlock);
        endProfilerSection("player_receive_packet");
        beginProfilerSection("tick_player");
        pthread_rwlock_rdlock(&world->players->rwlock);
        ITER_MAP(world->players) {
            struct player* player = (struct player*) value;
            player_tick(world, player);
            tick_entity(world, player->entity); // might need to be moved into separate loop later
            ITER_MAP_END();
        }
        pthread_rwlock_unlock(&world->players->rwlock);
        endProfilerSection("tick_player");
        beginProfilerSection("tick_entity");
        pthread_rwlock_rdlock(&world->entities->rwlock);
        ITER_MAP(world->entities) {
            struct entity* entity = (struct entity*) value;
            if (entity->type != ENT_PLAYER) tick_entity(world, entity);
            ITER_MAP_END();
        }
        pthread_rwlock_unlock(&world->entities->rwlock);
        endProfilerSection("tick_entity");
        beginProfilerSection("tick_chunks");
        pthread_rwlock_rdlock(&world->chunks->rwlock);
        ITER_MAP(world->chunks) {
            struct chunk* chunk = (struct chunk*) value;
            beginProfilerSection("tick_chunk_tileentity");
            ITER_MAPR(chunk->tileEntitiesTickable, tile_value) {
                struct tile_entity* tile = tile_value;
                (*tile->tick)(world, tile);
                ITER_MAP_END();
            }
            endProfilerSection("tick_chunk_tileentity");
            beginProfilerSection("tick_chunk_randomticks");
            if (RANDOM_TICK_SPEED > 0){
                for (int t = 0; t < 16; t++) {
                    struct chunk_section* section = chunk->sections[t];
                    if (section != NULL) {
                        for (int z = 0; z < RANDOM_TICK_SPEED; z++) {
                            lcg = lcg * 3 + 1013904223;
                            int32_t ctotal = lcg >> 2;
                            uint8_t x = (uint8_t) (ctotal & 0x0f);
                            uint8_t z = (ctotal >> 8) & 0x0f;
                            uint8_t y = (uint8_t) ((ctotal >> 16) & 0x0f);
                            block b = chunk_get_block(chunk, x, (uint8_t) (y + (t << 4)), z);
                            struct block_info* info = getBlockInfo(b);
                            if (info != NULL && info->randomTick != NULL) {
                                (*info->randomTick)(world, chunk, b, x + (chunk->x << 4), y + (t << 4), z + (chunk->z << 4));
                            }
                        }
                    }
                }
            }
            endProfilerSection("tick_chunk_randomticks");
            ITER_MAP_END();
        }
        pthread_rwlock_unlock(&world->chunks->rwlock);

        beginProfilerSection("tick_chunk_scheduledticks");
        struct mempool* scheduled_tick_pool = mempool_new();
        struct prqueue* priority_queue = prqueue_new(scheduled_tick_pool, 0, 0);
        pthread_rwlock_rdlock(&world->scheduledTicks->rwlock);
        ITER_MAP(world->scheduledTicks) {
            struct scheduled_tick* tick = value;
            if (--tick->ticksLeft <= 0) { //
                prqueue_add(priority_queue, tick, tick->priority);
                /*block b = world_get_block(world, tick->x, tick->y, tick->z);
                 struct block_info* bi = getBlockInfo(b);
                 int k = 0;
                 if (bi->scheduledTick != NULL) k = (*bi->scheduledTick)(world, b, tick->x, tick->y, tick->z);
                 if (k > 0) {
                 tick->ticksLeft = k;
                 } else {
                 struct encpos ep;
                 ep.x = tick->x;
                 ep.y = tick->y;
                 ep.z = tick->z;
                 put_hashmap(world->scheduledTicks, *((uint64_t*) &ep), NULL);
                 xfree(tick);
                 }*/
            }
            ITER_MAP_END();
        }
        pthread_rwlock_unlock(&world->scheduledTicks->rwlock);
        struct scheduled_tick* current_tick = NULL;
        while ((current_tick = prqueue_pop(priority_queue)) != NULL) {
            block b = world_get_block(world, current_tick->x, current_tick->y, current_tick->z);
            if (current_tick->src != b) {
                struct encpos ep;
                ep.x = current_tick->x;
                ep.y = current_tick->y;
                ep.z = current_tick->z;
                hashmap_putint(world->scheduledTicks, *((uint64_t*) &ep), NULL);
                pprefree(world->pool, current_tick);
                continue;
            }
            struct block_info* info = getBlockInfo(b);
            int k = 0;
            if (info->scheduledTick != NULL) k = (*info->scheduledTick)(world, b, current_tick->x, current_tick->y, current_tick->z);
            if (k > 0) {
                current_tick->ticksLeft = k;
                current_tick->src = world_get_block(world, current_tick->x, current_tick->y, current_tick->z);
            } else if (k < 0) {
                pprefree(world->pool, current_tick);
            } else {
                struct encpos ep;
                ep.x = current_tick->x;
                ep.y = current_tick->y;
                ep.z = current_tick->z;
                hashmap_putint(world->scheduledTicks, *((uint64_t*) &ep), NULL);
                pprefree(world->pool, current_tick);
            }
        }
        pfree(scheduled_tick_pool);
        endProfilerSection("tick_chunk_scheduledticks");
        endProfilerSection("tick_chunks");
        ITER_MAP(plugins) {
            struct plugin* plugin = value;
            if (plugin->tick_world != NULL) (*plugin->tick_world)(world);
            ITER_MAP_END();
        }
        endProfilerSection("world_tick");
    }
#pragma clang diagnostic pop
}

int world_save(struct world* world, char* path) {

    return 0;
}

struct chunk* world_get_entity_chunk(struct entity* entity) {
    return world_get_chunk(entity->world, ((int32_t) entity->x) >> 4, ((int32_t) entity->z) >> 4);
}

void world_spawn_entity(struct world* world, struct entity* entity) {
    entity->world = world;
    if (entity->loadingPlayers == NULL) {
        entity->loadingPlayers = hashmap_thread_new(8, entity->pool);
    }
    if (entity->attackers == NULL) {
        entity->attackers = hashmap_new(4, entity->pool);
    }
    hashmap_putint(world->entities, entity->id, entity);
    struct entity_info* info = entity_get_info(entity->type);
    if (info != NULL) {
        if (info->initAI != NULL) {
            entity->ai = pcalloc(entity->pool, sizeof(struct aicontext));
            entity->ai->tasks = hashmap_new(8, entity->pool);
            (*info->initAI)(world, entity);
        }
        if (info->onSpawned != NULL) (*info->onSpawned)(world, entity);
    }
    ITER_MAP(plugins) {
        struct plugin* plugin = value;
        if (plugin->onEntitySpawn != NULL) (*plugin->onEntitySpawn)(world, entity);
        ITER_MAP_END();
    }
}

void world_spawn_player(struct world* world, struct player* player) {
    player->world = world;
    if (player->loaded_entities == NULL) {
        player->loaded_entities = hashmap_new(32, player->pool);
    }
    if (player->loaded_chunks == NULL) {
        player->loaded_chunks = hashmap_thread_new(32, player->pool);
    }
    hashmap_putint(world->players, (uint64_t) player->entity->id, player);
    world_spawn_entity(world, player->entity);
    ITER_MAP(plugins) {
        struct plugin* plugin = value;
        if (plugin->onPlayerSpawn != NULL) (*plugin->onPlayerSpawn)(world, player);
        ITER_MAP_END();
    }
}

void world_despawn_player(struct world* world, struct player* player) {
    if (player->open_inventory != NULL) {
        player_closeWindow(player, (uint16_t) player->open_inventory->window);
    }
    world_despawn_entity(world, player->entity);
    ITER_MAP(player->loaded_entities) {
        if (value == NULL || value == player->entity) {
            continue;
        }
        struct entity* entity = (struct entity*) value;
        hashmap_putint(entity->loadingPlayers, (uint64_t) player->entity->id, NULL);
        ITER_MAP_END();
    }
    pthread_rwlock_wrlock(&player->loaded_chunks->rwlock);
    ITER_MAP(player->loaded_chunks) {
        struct chunk* chunk = (struct chunk*) value;
        if (--chunk->playersLoaded <= 0) {
            world_unload_chunk(world, chunk);
        }
        ITER_MAP_END();
    }
    pthread_rwlock_unlock(&player->loaded_chunks->rwlock);
    player->loaded_chunks = NULL;
    hashmap_putint(world->players, (uint64_t) player->entity->id, NULL);
}

void world_despawn_entity(struct world* world, struct entity* entity) {
    hashmap_putint(world->entities, (uint64_t) entity->id, NULL);
    BEGIN_BROADCAST(entity->loadingPlayers)
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_DESTROYENTITIES);
    pkt->data.play_client.destroyentities.count = 1;
    pkt->data.play_client.destroyentities.entity_ids = pmalloc(pkt->pool, sizeof(int32_t));
    pkt->data.play_client.destroyentities.entity_ids[0] = entity->id;
    queue_push(bc_player->outgoing_packets, pkt);
    hashmap_putint(bc_player->loaded_entities, entity->id, NULL);
    END_BROADCAST(entity->loadingPlayers);
    entity->loadingPlayers = NULL;
    ITER_MAP(entity->attackers) {
        struct entity* attacker = value;
        if (attacker->attacking == entity) attacker->attacking = NULL;
        ITER_MAP_END();
    }
    entity->attackers = NULL;
    entity->attacking = NULL;
    entity->despawn = 1;
    //TODO: do we need remove ourselves from entity->attacking->attackers?
}

struct entity* world_get_entity(struct world* world, int32_t id) {
    return hashmap_getint(world->entities, (uint64_t) id);
}

void world_update_block_guess(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return;
    block b = world_get_block_guess(world, chunk, x, y, z);
    struct block_info* bi = getBlockInfo(b);
    if (bi != NULL && bi->onBlockUpdate != NULL) bi->onBlockUpdate(world, b, x, y, z);
}

void world_update_block(struct world* world, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return;
    block b = world_get_block(world, x, y, z);
    struct block_info* bi = getBlockInfo(b);
    if (bi != NULL && bi->onBlockUpdate != NULL) bi->onBlockUpdate(world, b, x, y, z);
}
