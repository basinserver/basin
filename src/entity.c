/*
 * entity.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#include <basin/packet.h>
#include <basin/basin.h>
#include <basin/network.h>
#include <basin/entity.h>
#include <basin/ai.h>
#include <basin/game.h>
#include <basin/plugin.h>
#include <basin/entity_impl.h>
#include <avuna/string.h>
#include <avuna/json.h>
#include <avuna/queue.h>
#include <avuna/util.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

struct entity_info* entity_get_info(uint32_t id) {
    if (id < 0 || id > entity_infos->size) return NULL;
    return entity_infos->data[id];
}

void init_entities() {
    entities_pool = mempool_new();
    entity_infos = list_new(64, entities_pool);
    entity_infos_by_name = hashmap_thread_new(64, entities_pool);
    char* json_file = (char*) read_file_fully(entities_pool, "smelting.json", NULL);
    if (json_file == NULL) {
        errlog(delog, "Error reading entity information: %s\n", strerror(errno));
        return;
    }
    struct json_object* json = NULL;
    json_parse(entities_pool, &json, json_file);
    pprefree(entities_pool, json_file);
    ITER_LLIST(json->children_list, value) {
        struct json_object* child_json = value;
        struct entity_info* info = pcalloc(entities_pool, sizeof(struct entity_info));
        struct json_object* tmp = json_get(child_json, "id");
        if (tmp == NULL || tmp->type != JSON_NUMBER) {
            goto entity_error;
        }
        uint32_t id = (uint32_t) tmp->data.number;
        if (id < 0) goto entity_error;
        tmp = json_get(child_json, "maxHealth");
        if (tmp == NULL || tmp->type != JSON_NUMBER) {
            goto entity_error;
        }
        info->maxHealth = (float) tmp->data.number;
        tmp = json_get(child_json, "width");
        if (tmp == NULL || tmp->type != JSON_NUMBER) {
            goto entity_error;
        }
        info->width = (float) tmp->data.number;
        tmp = json_get(child_json, "height");
        if (tmp == NULL || tmp->type != JSON_NUMBER) {
            goto entity_error;
        }
        info->height = (float) tmp->data.number;
        tmp = json_get(child_json, "loot");
        if (tmp != NULL && tmp->type == JSON_ARRAY) {
            info->loot_count = tmp->children_list->size;
            info->loots = pcalloc(entities_pool, info->loot_count * sizeof(struct entity_loot));
            size_t loot_index = 0;
            ITER_LLIST(tmp->children_list, loot_value) {
                struct json_object* loot_entry = loot_value;
                if (loot_entry == NULL || loot_entry->type != JSON_OBJECT) {
                    goto entity_error;
                }
                struct json_object* id_json = json_get(loot_entry, "id");
                if (id_json == NULL || id_json->type != JSON_NUMBER) {
                    goto entity_error;
                }
                info->loots[loot_index].id = (item) id_json->data.number;
                struct json_object* amin = json_get(loot_entry, "amountMin");
                struct json_object* amax = json_get(loot_entry, "amountMax");
                if ((amin == NULL) != (amax == NULL) || (amin != NULL && (amin->type != JSON_NUMBER || amax->type != JSON_NUMBER))) {
                    goto entity_error;
                }
                if (amin != NULL) {
                    info->loots[loot_index].amountMin = (uint8_t) amin->data.number;
                    info->loots[loot_index].amountMax = (uint8_t) amax->data.number;
                }
                amin = json_get(loot_entry, "metaMin");
                amax = json_get(loot_entry, "metaMax");
                if ((amin == NULL) != (amax == NULL) || (amin != NULL && (amin->type != JSON_NUMBER || amax->type != JSON_NUMBER))) {
                    goto entity_error;
                }
                if (amin != NULL) {
                    info->loots[loot_index].metaMin = (uint8_t) amin->data.number;
                    info->loots[loot_index].metaMax = (uint8_t) amax->data.number;
                }
                ++loot_index;
                ITER_LLIST_END();
            }
        }
        tmp = json_get(child_json, "flags");
        if (tmp == NULL || tmp->type != JSON_ARRAY) {
            goto entity_error;
        }
        info->flags = hashset_new(8, entities_pool);
        size_t flag_index = 0;
        ITER_LLIST(tmp->children_list, flag_value) {
            struct json_object* flag_json = flag_value;
            if (flag_json == NULL || flag_json->type != JSON_STRING) {
                goto entity_error;
            }
            hashset_add(info->flags, str_dup(str_tolower(str_trim(flag_json->data.string)), 0, entities_pool));
            ++flag_index;
            ITER_LLIST_END();
        }
        tmp = json_get(child_json, "packetType");
        if (tmp == NULL || tmp->type != JSON_STRING) goto entity_error;
        if (str_eq(tmp->data.string, "mob")) {
            info->spawn_packet = PKT_PLAY_CLIENT_SPAWNMOB;
        } else if (str_eq(tmp->data.string, "object")) {
            info->spawn_packet = PKT_PLAY_CLIENT_SPAWNOBJECT;
        } else if (str_eq(tmp->data.string, "exp")) {
            info->spawn_packet = PKT_PLAY_CLIENT_SPAWNEXPERIENCEORB;
        } else if (str_eq(tmp->data.string, "painting")) {
            info->spawn_packet = PKT_PLAY_CLIENT_SPAWNPAINTING;
        }
        tmp = json_get(child_json, "packetID");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto entity_error;
        info->spawn_packet_id = (int32_t) tmp->data.number;
        tmp = json_get(child_json, "dataname");
        if (tmp == NULL || tmp->type != JSON_STRING) goto entity_error;
        info->dataname = str_dup(tmp->data.string, 0, entities_pool);
        list_set(entity_infos, id, info);
        hashmap_put(entity_infos_by_name, str_tolower(str_dup(info->dataname, 0, entities_pool)), info);
        continue;
        entity_error: ;
        printf("[WARNING] Error Loading Entity \"%s\"! Skipped.\n", child_json->name);
        ITER_LLIST_END();
    }
    pfree(json->pool);
    entity_get_info(ENT_ZOMBIE)->onAITick = &ai_handletasks;
    entity_get_info(ENT_ZOMBIE)->initAI = &initai_zombie;
    entity_get_info(ENT_FALLINGBLOCK)->onTick = &onTick_fallingblock;
    entity_get_info(ENT_PRIMEDTNT)->onTick = &onTick_tnt;
    entity_get_info(ENT_ITEM)->onTick = &tick_itemstack;
    entity_get_info(ENT_ARROW)->onTick = &tick_arrow;
    entity_get_info(ENT_SPECTRALARROW)->onTick = &tick_arrow;
    entity_get_info(ENT_COW)->onInteract = &onInteract_cow;
    entity_get_info(ENT_MUSHROOMCOW)->onInteract = &onInteract_mooshroom;
    entity_get_info(ENT_MINECARTRIDEABLE)->onSpawned = &onSpawned_minecart;
    entity_get_info(ENT_MINECARTCHEST)->onSpawned = &onSpawned_minecart;
    entity_get_info(ENT_MINECARTFURNACE)->onSpawned = &onSpawned_minecart;
    entity_get_info(ENT_MINECARTTNT)->onSpawned = &onSpawned_minecart;
    entity_get_info(ENT_MINECARTSPAWNER)->onSpawned = &onSpawned_minecart;
    entity_get_info(ENT_MINECARTHOPPER)->onSpawned = &onSpawned_minecart;
    entity_get_info(ENT_MINECARTCOMMANDBLOCK)->onSpawned = &onSpawned_minecart;
}

void entity_animation(struct entity* entity, int animation_id) { // 0 to swing arm
    BEGIN_BROADCAST_DIST(entity, CHUNK_VIEW_DISTANCE * 16.) {
        struct packet* packet = packet_new(mempool_new(), PKT_PLAY_CLIENT_ANIMATION);
        packet->data.play_client.animation.entity_id = entity->id;
        packet->data.play_client.animation.animation = animation_id;
        queue_push(bc_player->outgoing_packets, packet);
        END_BROADCAST(entity->world->players);
    }
}

void entity_collision_bounding_box(struct entity* entity, struct boundingbox* bb) {
    //TODO: plugin hook here?
    bb->minX = entity->x - entity->info->width / 2;
    bb->maxX = entity->x + entity->info->width / 2;
    bb->minZ = entity->z - entity->info->width / 2;
    bb->maxZ = entity->z + entity->info->width / 2;
    bb->minY = entity->y;
    bb->maxY = entity->y + entity->info->height;
}

void entity_jump(struct entity* entity) {
    //TODO: plugin hook here?
    //TODO: some entities may jump/swim diferently
    if (entity->inWater || entity->inLava) entity->motY += .04;
    else if (entity->on_ground) {
        entity->motY = .42;
    }
}

struct entity* entity_new(struct world* world, int32_t id, double x, double y, double z, uint32_t type, float yaw, float pitch) {
    struct mempool* pool = mempool_new();
    pchild(world->pool, pool);
    struct entity* entity = pcalloc(pool, sizeof(struct entity));
    entity->pool = pool;
    entity->world = world;
    struct entity_info* info = entity_get_info(type);
    entity->info = info;
    entity->id = id;
    entity->type = type;
    entity->last_x = entity->x = x;
    entity->last_y = entity->y = y;
    entity->last_z = entity->z = z;
    entity->last_yaw = entity->yaw = yaw;
    entity->last_pitch = entity->pitch = pitch;
    entity->maxHealth = info == NULL ? 20f : info->maxHealth;
    entity->health = entity->maxHealth;
    entity->loadingPlayers = hashmap_thread_new(16, entity->pool);
    // entity->attackers = new_hashmap(1, 0);
    return entity;
}

double entity_dist(struct entity* ent1, struct entity* ent2) {
    return ent1->world == ent2->world ? sqrt((ent1->x - ent2->x) * (ent1->x - ent2->x) + (ent1->y - ent2->y) * (ent1->y - ent2->y) + (ent1->z - ent2->z) * (ent1->z - ent2->z)) : 999.;
}

double entity_distsq(struct entity* ent1, struct entity* ent2) {
    return (ent1->x - ent2->x) * (ent1->x - ent2->x) + (ent1->y - ent2->y) * (ent1->y - ent2->y) + (ent1->z - ent2->z) * (ent1->z - ent2->z);
}

double entity_distsq_block(struct entity* ent1, double x, double y, double z) {
    return (ent1->x - x) * (ent1->x - x) + (ent1->y - y) * (ent1->y - y) + (ent1->z - z) * (ent1->z - z);
}

double entity_dist_block(struct entity* ent1, double x, double y, double z) {
    return sqrt((ent1->x - x) * (ent1->x - x) + (ent1->y - y) * (ent1->y - y) + (ent1->z - z) * (ent1->z - z));
}

void entitymeta_handle_byte(struct entity* ent, int index, uint8_t b) {
    if (index == 0) {
        ent->sneaking = b & 0x02 ? 1 : 0;
        ent->sprinting = b & 0x08 ? 1 : 0;
        ent->usingItemMain = b & 0x10 ? 1 : 0;
        ent->usingItemOff = 0;
    } else if (hashset_has(ent->info->flags, "living") && index == 6) {
        ent->usingItemMain = b & 0x01;
        ent->usingItemOff = (b & 0x02) ? 1 : 0;
    }
}

void entitymeta_handle_varint(struct entity* ent, int index, int32_t i) {
    if ((ent->type == ENT_SKELETON || ent->type == ENT_SLIME || ent->type == ENT_LAVASLIME || ent->type == ENT_GUARDIAN) && index == 11) {
        ent->subtype = i;
    }
}

void entitymeta_handle_float(struct entity* ent, int index, float f) {

}

void entitymeta_handle_string(struct entity* ent, int index, char* str) {

}

void entitymeta_handle_slot(struct entity* ent, int index, struct slot* slot) {
    if (ent->type == ENT_ITEM && index == 6) {
        ent->data.itemstack.slot = slot;
    }
}

void entitymeta_handle_vector(struct entity* ent, int index, float f1, float f2, float f3) {

}

void entitymeta_handle_encpos(struct entity* ent, int index, struct encpos* pos) {

}

void entitymeta_handle_uuid(struct entity* ent, int index, struct uuid* pos) {

}


void entitymeta_read(struct entity* ent, uint8_t* meta, size_t size) {
    struct mempool* pool = mempool_new();
    while (size > 0) {
        uint8_t index = meta[0];
        meta++;
        size--;
        if (index == 0xff || size == 0) break;
        uint8_t type = meta[0];
        meta++;
        size--;
        if (type == 0 || type == 6) {
            if (size == 0) break;
            uint8_t b = meta[0];
            meta++;
            size--;
            entitymeta_handle_byte(ent, index, b);
        } else if (type == 1 || type == 10 || type == 12) {
            if ((type == 10 || type == 12) && size == 0) break;
            int32_t vi = 0;
            int r = readVarInt(&vi, meta, size);
            meta += r;
            size -= r;
            entitymeta_handle_varint(ent, index, vi);
        } else if (type == 2) {
            if (size < 4) break;
            float f = 0.;
            memcpy(&f, meta, 4);
            meta += 4;
            size -= 4;
            swapEndian(&f, 4);
            entitymeta_handle_float(ent, index, f);
        } else if (type == 3 || type == 4) {
            char* str = NULL;
            int r = readString(pool, &str, meta, size);
            meta += r;
            size -= r;
            entitymeta_handle_string(ent, index, str);
        } else if (type == 5) {
            struct slot slot;
            int r = readSlot(pool, &slot, meta, size);
            meta += r;
            size -= r;
            entitymeta_handle_slot(ent, index, &slot);
        } else if (type == 7) {
            if (size < 12) break;
            float f1 = 0f;
            float f2 = 0f;
            float f3 = 0f;
            memcpy(&f1, meta, 4);
            meta += 4;
            size -= 4;
            memcpy(&f2, meta, 4);
            meta += 4;
            size -= 4;
            memcpy(&f3, meta, 4);
            meta += 4;
            size -= 4;
            swapEndian(&f1, 4);
            swapEndian(&f2, 4);
            swapEndian(&f3, 4);
            entitymeta_handle_vector(ent, index, f1, f2, f3);
        } else if (type == 8) {
            struct encpos pos;
            if (size < sizeof(struct encpos)) break;
            memcpy(&pos, meta, sizeof(struct encpos));
            meta += sizeof(struct encpos);
            size -= sizeof(struct encpos);
            entitymeta_handle_encpos(ent, index, &pos);
        } else if (type == 9) {
            if (size == 0) break;
            signed char b = meta[0];
            meta++;
            size--;
            if (b) {
                struct encpos pos;
                if (size < sizeof(struct encpos)) break;
                memcpy(&pos, meta, sizeof(struct encpos));
                meta += sizeof(struct encpos);
                size -= sizeof(struct encpos);
                entitymeta_handle_encpos(ent, index, &pos);
            }
        } else if (type == 11) {
            if (size == 0) break;
            signed char b = meta[0];
            meta++;
            size--;
            if (b) {
                struct uuid uuid;
                if (size < sizeof(struct uuid)) break;
                memcpy(&uuid, meta, sizeof(struct uuid));
                meta += sizeof(struct uuid);
                size -= sizeof(struct uuid);
                entitymeta_handle_uuid(ent, index, &uuid);
            }
        }
    }
    pfree(pool);
}

void entitymeta_write_byte(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {
    *loc = prealloc(pool, *loc, (*size) + 10);
    (*loc)[(*size)++] = 0;
    (*loc)[(*size)++] = 0;
    (*loc)[*size] = entity->sneaking ? 0x02 : 0;
    (*loc)[*size] |= entity->sprinting ? 0x08 : 0;
    (*loc)[(*size)++] |= (entity->usingItemMain || entity->usingItemOff) ? 0x08 : 0;
    if (hashset_has(entity->info->flags, "livingbase")) {
        (*loc)[(*size)++] = 6;
        (*loc)[(*size)++] = 0;
        (*loc)[*size] = entity->usingItemMain ? 0x01 : 0;
        (*loc)[(*size)++] |= entity->usingItemOff ? 0x02 : 0;
    }

}

void entitymeta_write_varint(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {
    if ((entity->type == ENT_SKELETON || entity->type == ENT_SLIME || entity->type == ENT_LAVASLIME || entity->type == ENT_GUARDIAN)) {
        *loc = prealloc(pool, *loc, *size + 2 + getVarIntSize(entity->subtype));
        (*loc)[(*size)++] = 11;
        (*loc)[(*size)++] = 1;
        *size += writeVarInt(entity->subtype, *loc + *size);
    }
}

void entitymeta_write_float(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {
    if (hashset_has(entity->info->flags, "livingbase")) {
        *loc = prealloc(pool, *loc, *size + 6);
        (*loc)[(*size)++] = 7;
        (*loc)[(*size)++] = 2;
        memcpy(*loc + *size, &entity->health, 4);
        swapEndian(*loc + *size, 4);
        *size += 4;
    }
}

void entitymeta_write_string(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {

}

void entitymeta_write_slot(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {
    if (entity->type == ENT_ITEM) {
        *loc = prealloc(pool, *loc, *size + 1024);
        (*loc)[(*size)++] = 6;
        (*loc)[(*size)++] = 5;
        *size += writeSlot(entity->data.itemstack.slot, *loc + *size, *size + 1022);
    }
}

void entitymeta_write_vector(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {

}

void entitymeta_write_encpos(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {

}

void entitymeta_write_uuid(struct entity* entity, uint8_t** loc, size_t* size, struct mempool* pool) {

}

void entitymeta_write(struct entity* entity, uint8_t** data, size_t* size, struct mempool* pool) {
    *data = NULL;
    *size = 0;
    entitymeta_write_byte(entity, data, size, pool);
    entitymeta_write_varint(entity, data, size, pool);
    entitymeta_write_float(entity, data, size, pool);
    entitymeta_write_string(entity, data, size, pool);
    entitymeta_write_slot(entity, data, size, pool);
    entitymeta_write_vector(entity, data, size, pool);
    entitymeta_write_encpos(entity, data, size, pool);
    entitymeta_write_uuid(entity, data, size, pool);
    *data = prealloc(pool, *data, *size + 1);
    (*data)[(*size)++] = 0xFF;
}

void entity_broadcast_metadata(struct entity* entity) {
    BEGIN_BROADCAST(entity->loadingPlayers)
    struct packet* packet = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYMETADATA);
    packet->data.play_client.entitymetadata.entity_id = entity->id;
    entitymeta_write(entity, &packet->data.play_client.entitymetadata.metadata.metadata, &packet->data.play_client.entitymetadata.metadata.metadata_size, packet->pool);
    queue_push(bc_player->outgoing_packets, packet);
    END_BROADCAST(entity->loadingPlayers)
}

int entity_swing_time(struct entity* entity) {
    for (size_t i = 0; i < entity->effect_count; i++) {
        if (entity->effects[i].effectID == 3) {
            return 6 - (1 + entity->effects[i].amplifier);
        } else if (entity->effects[i].effectID == 4) {
            return 6 + (1 + entity->effects[i].amplifier) * 2;
        }
    }
    return 6;
}

block entity_adjustCollision(struct world* world, struct entity* entity, struct chunk* chunk, block block, int32_t x, int32_t y, int32_t z) {
    if (block >> 4 == BLK_DOORIRON >> 4 || block >> 4 == BLK_DOOROAK >> 4 || block >> 4 == BLK_DOORSPRUCE >> 4 || block >> 4 == BLK_DOORBIRCH >> 4 || block >> 4 == BLK_DOORJUNGLE >> 4 || block >> 4 == BLK_DOORACACIA >> 4 || block >> 4 == BLK_DOORDARKOAK >> 4) {
        if (block & 0b1000) return world_get_block_guess(world, chunk, x, y - 1, z);
    }
    return block;
}

void entity_adjustBoundingBox(struct world* world, struct entity* entity, struct chunk* ch, block block, int32_t x, int32_t y, int32_t z, struct boundingbox* in, struct boundingbox* out) {
    memcpy(in, out, sizeof(struct boundingbox));
    if (block >> 4 == BLK_DOORIRON >> 4 || block >> 4 == BLK_DOOROAK >> 4 || block >> 4 == BLK_DOORSPRUCE >> 4 || block >> 4 == BLK_DOORBIRCH >> 4 || block >> 4 == BLK_DOORJUNGLE >> 4 || block >> 4 == BLK_DOORACACIA >> 4 || block >> 4 == BLK_DOORDARKOAK >> 4) {
        block low_block = block;
        block high_block = block;
        if (block & 0b1000) {
            low_block = world_get_block_guess(world, ch, x, y - 1, z);
        } else {
            high_block = world_get_block_guess(world, ch, x, y + 1, z);
        }
        if ((low_block & 0b0010) && (high_block & 0b0001)) {
            uint8_t face = (uint8_t) (low_block & 0b0011);
            if (face == 3 || face == 1) {
                out->maxZ = 1. - in->maxZ;
                out->minZ = 1. - in->minZ;
            } else {
                out->maxX = 1. - in->maxX;
                out->minX = 1. - in->minX;
            }
        }
    }
}

int entity_move(struct entity* entity, double* motionX, double* motionY, double* motionZ, float shrink) {
    if (entity->immovable) return 0;
    struct boundingbox obb;
    entity_collision_bounding_box(entity, &obb);
    if (entity->type == ENT_ARROW || entity->type == ENT_SPECTRALARROW || obb.minX == obb.maxX || obb.minZ == obb.maxZ || obb.minY == obb.maxY) {
        entity->x += *motionX;
        entity->y += *motionY;
        entity->z += *motionZ;
        return 0;
    }
    obb.minX += shrink;
    obb.maxX -= shrink;
    obb.minY += shrink;
    obb.maxY -= shrink;
    obb.minZ += shrink;
    obb.maxZ -= shrink;
    if (*motionX < 0.) {
        obb.minX += *motionX;
    } else {
        obb.maxX += *motionX;
    }
    if (*motionY < 0.) {
        obb.minY += *motionY;
    } else {
        obb.maxY += *motionY;
    }
    if (*motionZ < 0.) {
        obb.minZ += *motionZ;
    } else {
        obb.maxZ += *motionZ;
    }
    struct boundingbox pbb;
    entity_collision_bounding_box(entity, &pbb);
    pbb.minX += shrink;
    pbb.maxX -= shrink;
    pbb.minY += shrink;
    pbb.maxY -= shrink;
    pbb.minZ += shrink;
    pbb.maxZ -= shrink;
    double ny = *motionY;
    struct chunk* ch = world_get_chunk(entity->world, (int32_t) entity->x / 16, (int32_t) entity->z / 16);
    for (int32_t x = (int32_t) floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
        for (int32_t z = (int32_t) floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
            for (int32_t y = (int32_t) floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
                block b = world_get_block_guess(entity->world, ch, x, y, z);
                if (b == 0) continue;
                b = entity_adjustCollision(entity->world, entity, ch, b, x, y, z);
                struct block_info* bi = getBlockInfo(b);
                if (bi == NULL) continue;
                for (size_t i = 0; i < bi->boundingBox_count; i++) {
                    struct boundingbox* bbx = &bi->boundingBoxes[i];
                    struct boundingbox bbd;
                    struct boundingbox* bb = &bbd;
                    entity_adjustBoundingBox(entity->world, entity, ch, b, x, y, z, bbx, &bbd);
                    if (bb != NULL && bb->minX != bb->maxX && bb->minY != bb->maxY && bb->minZ != bb->maxZ) {
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
    entity->y += ny;
    pbb.minY += ny;
    pbb.maxY += ny;
    double nx = *motionX;
    for (int32_t x = (int32_t) floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
        for (int32_t z = (int32_t) floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
            for (int32_t y = (int32_t) floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
                block b = world_get_block_guess(entity->world, ch, x, y, z);
                if (b == 0) continue;
                b = entity_adjustCollision(entity->world, entity, ch, b, x, y, z);
                struct block_info* bi = getBlockInfo(b);
                if (bi == NULL) continue;
                for (size_t i = 0; i < bi->boundingBox_count; i++) {
                    struct boundingbox* bbx = &bi->boundingBoxes[i];
                    struct boundingbox bbd;
                    struct boundingbox* bb = &bbd;
                    entity_adjustBoundingBox(entity->world, entity, ch, b, x, y, z, bbx, &bbd);
                    if (bb != NULL && bb->minX != bb->maxX && bb->minY != bb->maxY && bb->minZ != bb->maxZ) {
                        if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
                            if (pbb.maxY > bb->minY + y && pbb.minY < bb->maxY + y && pbb.maxZ > bb->minZ + z && pbb.minZ < bb->maxZ + z) {
                                double t;
                                if (nx > 0. && pbb.maxX <= bb->minX + x) {
                                    t = bb->minX + x - pbb.maxX;
                                    if (t < nx) {
                                        nx = t;
                                    }
                                } else if (nx < 0. && pbb.minX >= bb->maxX + x) {
                                    t = bb->maxX + x - pbb.minX;
                                    if (t > nx) {
                                        nx = t;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    entity->x += nx;
    pbb.minX += nx;
    pbb.maxX += nx;
    double nz = *motionZ;
    for (int32_t x = (int32_t) floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
        for (int32_t z = (int32_t) floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
            for (int32_t y = (int32_t) floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
                block b = world_get_block_guess(entity->world, ch, x, y, z);
                if (b == 0) continue;
                b = entity_adjustCollision(entity->world, entity, ch, b, x, y, z);
                struct block_info* bi = getBlockInfo(b);
                if (bi == NULL) continue;
                for (size_t i = 0; i < bi->boundingBox_count; i++) {
                    struct boundingbox* bbx = &bi->boundingBoxes[i];
                    struct boundingbox bbd;
                    struct boundingbox* bb = &bbd;
                    entity_adjustBoundingBox(entity->world, entity, ch, b, x, y, z, bbx, &bbd);
                    if (bb != NULL && bb->minX != bb->maxX && bb->minY != bb->maxY && bb->minZ != bb->maxZ) {
                        if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
                            if (pbb.maxX > bb->minX + x && pbb.minX < bb->maxX + x && pbb.maxY > bb->minY + y && pbb.minY < bb->maxY + y) {
                                double t;
                                if (nz > 0. && pbb.maxZ <= bb->minZ + z) {
                                    t = bb->minZ + z - pbb.maxZ;
                                    if (t < nz) {
                                        nz = t;
                                    }
                                } else if (nz < 0. && pbb.minZ >= bb->maxZ + z) {
                                    t = bb->maxZ + z - pbb.minZ;
                                    if (t > nz) {
                                        nz = t;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
//TODO step
    entity->z += nz;
    pbb.minZ += nz;
    pbb.maxZ += nz;
    entity->collidedHorizontally = *motionX != nx || *motionZ != nz;
    entity->collidedVertically = *motionY != ny;
    entity->on_ground = entity->collidedVertically && *motionY < 0.;
    int32_t bx = (int32_t) floor(entity->x);
    int32_t by = (int32_t) floor(entity->y - .20000000298023224);
    int32_t bz = (int32_t) floor(entity->z);
    block lb = world_get_block_guess(entity->world, ch, bx, by, bz);
    if (lb == BLK_AIR) {
        block lbb = world_get_block_guess(entity->world, ch, bx, by - 1, bz);
        uint16_t lbi = lbb >> 4;
        if ((lbi >= (BLK_FENCE >> 4) && lbi <= (BLK_ACACIAFENCE >> 4)) || (lbi >= (BLK_FENCEGATE >> 4) && lbi <= (BLK_ACACIAFENCEGATE >> 4)) || lbi == BLK_COBBLEWALL_NORMAL >> 4) {
            lb = lbb;
            by--;
        }
    }
    if (*motionX != nx) *motionX = 0.;
    if (*motionZ != nz) *motionZ = 0.;
    if (*motionY != ny) {
        if (lb != BLK_SLIME || entity->sneaking) {
            *motionY = 0.;
        } else {
            *motionY = -(*motionY);
        }
    }
    return ny != *motionY || nx != *motionX || nz != *motionZ;
}

void entity_apply_velocity(struct entity* entity, double x, double y, double z) {
    entity->motX += x;
    entity->motY += y;
    entity->motZ += z;
    BEGIN_BROADCAST(entity->loadingPlayers) {
        struct packet* packet = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYVELOCITY);
        packet->data.play_client.entityvelocity.entity_id = entity->id;
        packet->data.play_client.entityvelocity.velocity_x = (int16_t) (entity->motX * 8000.);
        packet->data.play_client.entityvelocity.velocity_y = (int16_t) (entity->motY * 8000.);
        packet->data.play_client.entityvelocity.velocity_z = (int16_t) (entity->motZ * 8000.);
        queue_push(bc_player->outgoing_packets, packet);
        END_BROADCAST(entity->loadingPlayers)
    }
    if (entity->type == ENT_PLAYER) {
        struct packet* packet = packet_new(mempool_new(), PKT_PLAY_CLIENT_ENTITYVELOCITY);
        packet->data.play_client.entityvelocity.entity_id = entity->id;
        packet->data.play_client.entityvelocity.velocity_x = (int16_t)(entity->motX * 8000.);
        packet->data.play_client.entityvelocity.velocity_y = (int16_t)(entity->motY * 8000.);
        packet->data.play_client.entityvelocity.velocity_z = (int16_t)(entity->motZ * 8000.);
        queue_push(entity->data.player.player->outgoing_packets, packet);
    }
}

void entity_apply_knockback(struct entity* entity, float yaw, float strength) {
    float kb_resistance = 0f;
    if (game_rand_float() > kb_resistance) {
        float x_component = sinf((float) (yaw / 360f * 2f * M_PI));
        float z_component = -cosf((float) (yaw / 360f * 2f * M_PI));
        float m = sqrtf(x_component * x_component + z_component * z_component);
        entity->motX /= 2.;
        entity->motZ /= 2.;
        entity->motX -= x_component / m * strength;
        entity->motZ -= z_component / m * strength;
        if (entity->on_ground) {
            entity->motY /= 2.;
            entity->motY += strength;
            if (entity->motY > .4) entity->motY = .4;
        }
        entity_apply_velocity(entity, 0., 0., 0.);
    }
}

int damageEntityWithItem(struct entity* attacked, struct entity* attacker, uint8_t slot_index, struct slot* item) {
    if (attacked == NULL || attacked->invincibilityTicks > 0 || !hashset_has(attacked->info->flags, "livingbase") || attacked->health <= 0.) {
        return 0;
    }
    if (attacked->type == ENT_PLAYER && attacked->data.player.player->gamemode == 1) {
        return 0;
    }
    float damage = 1f;
    if (item != NULL) {
        struct item_info* info = getItemInfo(item->item);
        if (info != NULL) {
            damage = info->damage;
            if (attacker->type == ENT_PLAYER) {
                if (info->onItemAttacked != NULL) {
                    damage = (*info->onItemAttacked)(attacker->world, attacker->data.player.player, (uint8_t) (attacker->data.player.player->currentItem + 36), item, attacked);
                }
            }
        }
    }
    float knockback_strength = 1f;
    if (attacker->sprinting) knockback_strength++;
    if (attacker->type == ENT_PLAYER) {
        struct player* player = attacker->data.player.player;
        float as = player_getAttackStrength(player, .5);
        damage *= .2 + as * as * .8;
        knockback_strength *= .2 + as * as * .8;
    }
    if (attacker->y > attacked->y && (attacker->last_y - attacker->y) > 0 && !attacker->on_ground && !attacker->sprinting) { // todo water/ladder
        damage *= 1.5;
    }
    ITER_MAP(plugins) {
        struct plugin* plugin = value;
        if (plugin->onEntityAttacked != NULL) damage = (*plugin->onEntityAttacked)(attacker->world, attacker, slot_index, item, attacked, damage);
        ITER_MAP_END();
    }
    if (damage == 0.) {
        return 0;
    }
    damageEntity(attacked, damage, 1);
//TODO: enchantment
    knockback_strength *= .2;
    entity_apply_knockback(attacked, attacker->yaw, knockback_strength);
    if (attacker->type == ENT_PLAYER) {
        attacker->data.player.player->foodExhaustion += .1;
    }
    return 1;
}

int damageEntity(struct entity* attacked, float damage, int armorable) {
    if (attacked == NULL || damage <= 0. || attacked->invincibilityTicks > 0 || !hashset_has(attacked->info->flags, "livingbase") || attacked->health <= 0.) return 0;
    float armor = 0;
    if (attacked->type == ENT_PLAYER) {
        struct player* player = attacked->data.player.player;
        if (player->gamemode == 1 && player->entity->y >= -64.) return 0;
        if (armorable) for (int i = 5; i <= 8; i++) {
            struct slot* sl = inventory_get(player, player->inventory, i);
            if (sl != NULL) {
                struct item_info* ii = getItemInfo(sl->item);
                if (ii != NULL) {
                    if ((i == 5 && ii->armorType == ARMOR_HELMET) || (i == 6 && ii->armorType == ARMOR_CHESTPLATE) || (i == 7 && ii->armorType == ARMOR_LEGGINGS) || (i == 8 && ii->armorType == ARMOR_BOOTS)) armor += (float) ii->armor;
                }
            }
        }
    }
    if (armorable) {
        float mod = armor - damage / 2f;
        if (mod < armor * .2) mod = armor * .2f;
        if (mod > 20.) mod = 20f;
        damage *= 1. - mod / 25.;
    }
    if (attacked->type == ENT_PLAYER) {
        struct player* player = attacked->data.player.player;
        if (player->gamemode == 1 && player->entity->y >= -64.) return 0;
        if (armorable) for (int i = 5; i <= 8; i++) {
            struct slot* sl = inventory_get(player, player->inventory, i);
            if (sl != NULL) {
                struct item_info* info = getItemInfo(sl->item);
                if (info != NULL && info->onEntityHitWhileWearing != NULL) {
                    damage = (*info->onEntityHitWhileWearing)(player->world, player, i, sl, damage);
                }
            }
        }
    }
    attacked->health -= damage;
    attacked->invincibilityTicks = 10;
    if (attacked->type == ENT_PLAYER) {
        struct packet* packet = packet_new(mempool_new(), PKT_PLAY_CLIENT_UPDATEHEALTH);
        packet->data.play_client.updatehealth.health = attacked->health;
        packet->data.play_client.updatehealth.food = attacked->data.player.player->food;
        packet->data.play_client.updatehealth.food_saturation = attacked->data.player.player->saturation;
        queue_push(attacked->data.player.player->outgoing_packets, packet);
    }
    if (attacked->health <= 0.) {
        if (attacked->type == ENT_PLAYER) {
            struct player* player = attacked->data.player.player;
            broadcastf("default", "%s died", player->name);
            for (size_t i = 0; i < player->inventory->slot_count; i++) {
                struct slot* slot = inventory_get(player, player->inventory, (int) i);
                if (slot != NULL) dropEntityItem_explode(player->entity, slot);
                inventory_set_slot(player, player->inventory, (int) i, 0, 1);
            }
            if (player->inventory_holding != NULL) {
                dropEntityItem_explode(player->entity, player->inventory_holding);
                player->inventory_holding = NULL;
            }
            if (player->open_inventory != NULL) {
                player_closeWindow(player, (uint16_t) player->open_inventory->window);
            }
        } else {
            struct entity_info* info = entity_get_info(attacked->type);
            if (info != NULL) {
                for (size_t i = 0; i < info->loot_count; i++) {
                    struct entity_loot* el = &info->loots[i];
                    int amt = el->amountMax == el->amountMin ? el->amountMax : (rand() % (el->amountMax - el->amountMin) + el->amountMin);
                    if (amt <= 0) continue;
                    struct slot it;
                    it.item = el->id;
                    it.count = (unsigned char) amt;
                    it.damage = (int16_t) (el->metaMax == el->metaMin ? el->metaMax : (rand() % (el->metaMax - el->metaMin) + el->metaMin));
                    it.nbt = NULL;
                    dropEntityItem_explode(attacked, &it);
                }
            }
        }
    }
    entity_animation(attacked, 1);
    if (attacked->health <= 0.) {
        entity_broadcast_metadata(attacked);
    }
    if (attacked->type == ENT_PLAYER) playSound(attacked->world, 316, 8, attacked->x, attacked->y, attacked->z, 1f, 1f);
    return 1;
}

void healEntity(struct entity* healed, float amount) {
    if (healed == NULL || amount <= 0. || healed->health <= 0. || !hashset_has(healed->info->flags, "livingbase")) {
        return;
    }
    float original_health = healed->health;
    healed->health += amount;
    if (healed->health > healed->maxHealth) {
        healed->health = 20f;
    }
    if (healed->type == ENT_PLAYER && original_health != healed->health) {
        struct packet* packet = packet_new(mempool_new(), PKT_PLAY_CLIENT_UPDATEHEALTH);
        packet->data.play_client.updatehealth.health = healed->health;
        packet->data.play_client.updatehealth.food = healed->data.player.player->food;
        packet->data.play_client.updatehealth.food_saturation = healed->data.player.player->saturation;
        queue_push(healed->data.player.player->outgoing_packets, packet);
    }
}


int entity_inFluid(struct entity* entity, uint16_t blk, float ydown, int meta_check) {
    struct boundingbox pbb;
    entity_collision_bounding_box(entity, &pbb);
    if (pbb.minX == pbb.maxX || pbb.minZ == pbb.maxZ || pbb.minY == pbb.maxY) return 0;
    pbb.minX += .001;
    pbb.minY += .001;
    pbb.minY += ydown;
    pbb.minZ += .001;
    pbb.maxX -= .001;
    pbb.maxY -= .001;
    pbb.maxZ -= .001;
    for (int32_t x = (int32_t) floor(pbb.minX); x < floor(pbb.maxX + 1.); x++) {
        for (int32_t z = (int32_t) floor(pbb.minZ); z < floor(pbb.maxZ + 1.); z++) {
            for (int32_t y = (int32_t) floor(pbb.minY); y < floor(pbb.maxY + 1.); y++) {
                block b = world_get_block(entity->world, x, y, z);
                if (meta_check ? (b != blk) : ((b >> 4) != (blk >> 4))) continue;
                struct block_info* bi = getBlockInfo(b);
                if (bi == NULL) continue;
                struct boundingbox bb2;
                bb2.minX = 0. + (double) x;
                bb2.maxX = 1. + (double) x;
                bb2.minY = 0. + (double) y;
                bb2.maxY = 1. + (double) y;
                bb2.minZ = 0. + (double) z;
                bb2.maxZ = 1. + (double) z;
                if (boundingbox_intersects(&bb2, &pbb)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int entity_inBlock(struct entity* ent, block blk) { // blk = 0 for any block
    struct boundingbox obb;
    entity_collision_bounding_box(ent, &obb);
    if (obb.minX == obb.maxX || obb.minY == obb.maxY || obb.minZ == obb.maxZ) return 0;
    obb.minX += .01;
    obb.minY += .01;
    obb.minZ += .01;
    obb.maxX -= .01;
    obb.maxY -= .01;
    obb.maxZ -= .01;
    for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
        for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
            for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
                block b = world_get_block(ent->world, x, y, z);
                if (b == 0 || (blk != 0 && blk != b)) continue;
                struct block_info* bi = getBlockInfo(b);
                if (bi == NULL) continue;
                for (size_t i = 0; i < bi->boundingBox_count; i++) {
                    struct boundingbox* bb = &bi->boundingBoxes[i];
                    struct boundingbox nbb;
                    nbb.minX = bb->minX + (double) x;
                    nbb.maxX = bb->maxX + (double) x;
                    nbb.minY = bb->minY + (double) y;
                    nbb.maxY = bb->maxY + (double) y;
                    nbb.minZ = bb->minZ + (double) z;
                    nbb.maxZ = bb->maxZ + (double) z;
                    if (boundingbox_intersects(&nbb, &obb)) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

void entity_push_out_of_blocks(struct entity* ent) {
    if (!entity_inBlock(ent, 0)) {
        return;
    }
    struct boundingbox ebb;
    entity_collision_bounding_box(ent, &ebb);
    double x = ent->x;
    double y = (ebb.maxY + ebb.minY) / 2.;
    double z = ent->z;
    int32_t bx = (int32_t) x;
    int32_t by = (int32_t) y;
    int32_t bz = (int32_t) z;
    double dx = x - (double) bx;
    double dy = y - (double) by;
    double dz = z - (double) bz;
    double ld = 100.;
    float fbx = 0f;
    float fby = 0f;
    float fbz = 0f;
    if (dx < ld && !isNormalCube(getBlockInfo(world_get_block(ent->world, bx - 1, by, bz)))) {
        ld = dx;
        fbx = -1f;
        fby = 0f;
        fbz = 0f;
    }
    if ((1. - dx) < ld && !isNormalCube(getBlockInfo(world_get_block(ent->world, bx + 1, by, bz)))) {
        ld = 1. - dx;
        fbx = 1f;
        fby = 0f;
        fbz = 0f;
    }
    if (dz < ld && !isNormalCube(getBlockInfo(world_get_block(ent->world, bx, by, bz - 1)))) {
        ld = dx;
        fbx = 0f;
        fby = 0f;
        fbz = -1f;
    }
    if ((1. - dz) < ld && !isNormalCube(getBlockInfo(world_get_block(ent->world, bx, by, bz + 1)))) {
        ld = 1. - dz;
        fbx = 0f;
        fby = 0f;
        fbz = 1f;
    }
    if ((1. - dy) < ld && !isNormalCube(getBlockInfo(world_get_block(ent->world, bx, by + 1, bz)))) {
        ld = 1. - dy;
        fbx = 0f;
        fby = 1f;
        fbz = 0f;
    }
    float mag = game_rand_float() * .2f + .1f;
    if (fbx == 0. && fbz == 0.) {
        ent->motX *= .75;
        ent->motY = mag * fby;
        ent->motZ *= .75;
    } else if (fbx == 0. && fby == 0.) {
        ent->motX *= .75;
        ent->motY *= .75;
        ent->motZ = mag * fbz;
    } else if (fbz == 0. && fby == 0.) {
        ent->motX = mag * fbx;
        ent->motY *= .75;
        ent->motZ *= .75;
    }
}

void tick_entity(struct world* world, struct entity* entity) {
    if (entity->type != ENT_PLAYER) {
        entity->last_x = entity->x;
        entity->last_y = entity->y;
        entity->last_z = entity->z;
        entity->last_yaw = entity->yaw;
        entity->last_pitch = entity->pitch;
    }
    entity->age++;
    if (entity->invincibilityTicks > 0) entity->invincibilityTicks--;
    struct entity_info* ei = entity_get_info(entity->type);
    if (entity->ai != NULL) {
        if (ei != NULL && ei->onAITick != NULL) ei->onAITick(world, entity);
        lookHelper_tick(entity);
    }
    if (entity->y < -64.) {
        damageEntity(entity, 1., 0);
    }
    if (ei != NULL && ei->onTick != NULL) {
        (*ei->onTick)(world, entity);
        if (entity->despawn) {
            return;
        }
    }
    if (entity->type > ENT_PLAYER) {
        if (entity->motX != 0. || entity->motY != 0. || entity->motZ != 0.) entity_move(entity, &entity->motX, &entity->motY, &entity->motZ, 0.);
        double gravity = 0.;
        int is_arrow = entity->type == ENT_ARROW || entity->type == ENT_SPECTRALARROW;
        float friction = .98;
        if (is_arrow) {
            friction = .99;
            if (entity->inWater) friction = .6;
            entity->motX *= friction;
            entity->motY *= friction;
            entity->motZ *= friction;
        }
        if (entity->type == ENT_ITEM) gravity = .04;
        else if (is_arrow) gravity = .05;
        else if (hashset_has(entity->info->flags, "livingbase")) {
            if (entity->inLava) {
                entity->motX *= .5;
                entity->motY *= .5;
                entity->motZ *= .5;
                entity->motY -= .02;
                //TODO: upswells
            } else if (entity->inWater) {
                //TODO: depth strider
                entity->motX *= .8;
                entity->motY *= .8;
                entity->motZ *= .8;
                entity->motY -= .02;
                //TODO: upswells
            } else entity->motY -= .08;
        } else if (entity->type == ENT_FALLINGBLOCK || entity->type == ENT_PRIMEDTNT) {
            gravity = .04;
        }
        if (gravity != 0. && !entity->immovable) entity->motY -= gravity;
        if (!is_arrow) {
            if (entity->on_ground) {
                struct block_info* bi = getBlockInfo(world_get_block(entity->world, (int32_t) floor(entity->x), (int32_t) floor(entity->y) - 1, (int32_t) floor(entity->z)));
                if (bi != NULL) friction = bi->slipperiness * .98;
            }
            entity->motX *= friction;
            entity->motY *= .98;
            entity->motZ *= friction;
        }
        if (fabs(entity->motX) < .0001) entity->motX = 0.;
        if (fabs(entity->motY) < .0001) entity->motY = 0.;
        if (fabs(entity->motZ) < .0001) entity->motZ = 0.;
        if (entity->type == ENT_ITEM && entity->on_ground && entity->motX == 0. && entity->motY == 0. && entity->motZ == 0.) entity->motY *= -.5;
    }
    if (entity->type == ENT_ITEM || entity->type == ENT_XPORB) entity_push_out_of_blocks(entity);
    if (hashset_has(entity->info->flags, "livingbase")) {
        entity->inWater = (uint8_t) (entity_inFluid(entity, BLK_WATER, .2, 0) || entity_inFluid(entity, BLK_WATER_1, .2, 0));
        entity->inLava = (uint8_t) (entity_inFluid(entity, BLK_LAVA, .2, 0) || entity_inFluid(entity, BLK_LAVA_1, .2, 0));
        //TODO: ladders
        if (entity->type == ENT_PLAYER && entity->data.player.player->gamemode == 1) goto bfd;
        if (entity->inLava) {
            damageEntity(entity, 4f, 1);
            //TODO: fire
        }
        if (!entity->on_ground) {
            double dy = entity->last_y - entity->y;
            if (dy > 0.) {
                entity->fallDistance += dy;
            }
        } else if (entity->fallDistance > 0.) {
            if (entity->fallDistance > 3. && !entity->inWater && !entity->inLava) {
                damageEntity(entity, entity->fallDistance - 3f, 0);
                playSound(entity->world, 312, 8, entity->x, entity->y, entity->z, 1f, 1f);
            }
            entity->fallDistance = 0f;
        }
        bfd: ;
    }
    if (entity->type != ENT_PLAYER) {
        double md = entity_distsq_block(entity, entity->last_x, entity->last_y, entity->last_z);
        double mp = (entity->yaw - entity->last_yaw) * (entity->yaw - entity->last_yaw) + (entity->pitch - entity->last_pitch) * (entity->pitch - entity->last_pitch);
        if (md > .001 || mp > .01) {
            pthread_rwlock_rdlock(&entity->loadingPlayers->rwlock);
            ITER_MAP(entity->loadingPlayers) {
                player_send_entity_move(value, entity);
                ITER_MAP_END();
            }
            pthread_rwlock_unlock(&entity->loadingPlayers->rwlock);
        }
    }
}

