
#include <basin/entity_impl.h>
#include <basin/entity.h>
#include <basin/server.h>
#include <basin/game.h>
#include <basin/packet.h>
#include <avuna/pmem.h>
#include <math.h>
#include <string.h>


void onSpawned_minecart(struct world* world, struct entity* entity) {
    if (entity->type == ENT_MINECARTRIDEABLE) entity->objectData = 0;
    else if (entity->type == ENT_MINECARTCHEST) entity->objectData = 1;
    else if (entity->type == ENT_MINECARTFURNACE) entity->objectData = 2;
    else if (entity->type == ENT_MINECARTTNT) entity->objectData = 3;
    else if (entity->type == ENT_MINECARTSPAWNER) entity->objectData = 4;
    else if (entity->type == ENT_MINECARTHOPPER) entity->objectData = 5;
    else if (entity->type == ENT_MINECARTCOMMANDBLOCK) entity->objectData = 6;
}


void onTick_tnt(struct world* world, struct entity* entity) {
    if (entity->data.tnt.fuse-- <= 0) {
        world_despawn_entity(world, entity);
        world_explode(world, NULL, entity->x, entity->y + .5, entity->z, .4f);
    }
}

void onTick_fallingblock(struct world* world, struct entity* entity) {
    // TODO: mc has some methods to prevent dupes here, we should see if basin is afflicted
    if (entity->on_ground && entity->age > 1) {
        block b = world_get_block(world, (int32_t) floor(entity->x), (int32_t) floor(entity->y), (int32_t) floor(entity->z));
        if (!falling_canFallThrough(b)) {
            struct slot sl;
            sl.item = entity->data.fallingblock.b >> 4;
            sl.damage = (int16_t) (entity->data.fallingblock.b & 0x0f);
            sl.count = 1;
            sl.nbt = NULL;
            game_drop_block(world, &sl, (int32_t) floor(entity->x), (int32_t) floor(entity->y - .01), (int32_t) floor(entity->z));
        } else {
            world_set_block(world, entity->data.fallingblock.b, (int32_t) floor(entity->x), (int32_t) floor(entity->y), (int32_t) floor(entity->z));
        }
        //TODO: tile entities
        world_despawn_entity(world, entity);
    }
}

void onInteract_cow(struct world* world, struct entity* entity, struct player* interacter, struct slot* item, int16_t slot_index) {
    if (item->item == ITM_BUCKET && interacter->gamemode != 1) { // TODO: not child
        if (item->count == 1) {
            item->item = ITM_MILK;
            inventory_set_slot(interacter, interacter->inventory, slot_index, item, 1);
        } else {
            item->count--;
            inventory_set_slot(interacter, interacter->inventory, slot_index, item, 1);
            struct slot slot;
            slot.item = ITM_MILK;
            slot.count = 1;
            slot.damage = 0;
            slot.nbt = NULL;
            if (inventory_add_player(interacter, interacter->inventory, &slot, 1)) {
                dropPlayerItem(interacter, &slot);
            }
        }
    }
}

void onInteract_mooshroom(struct world* world, struct entity* entity, struct player* interacter, struct slot* item, int16_t slot_index) {
    if (item->item == ITM_BOWL && interacter->gamemode != 1) { // TODO: not child
        if (item->count == 1) {
            item->item = ITM_MUSHROOMSTEW;
            inventory_set_slot(interacter, interacter->inventory, slot_index, item, 1);
        } else {
            item->count--;
            inventory_set_slot(interacter, interacter->inventory, slot_index, item, 1);
            struct slot slot;
            slot.item = ITM_MUSHROOMSTEW;
            slot.count = 1;
            slot.damage = 0;
            slot.nbt = NULL;
            if (inventory_add_player(interacter, interacter->inventory, &slot, 1)) {
                dropPlayerItem(interacter, &slot);
            }
        }
    } else if (item->item == ITM_SHEARS && interacter->gamemode != 1) { // TODO: not child
        //TODO: explosion
        struct entity* ent = entity_new(world, world->server->next_entity_id++, entity->x, entity->y, entity->z, ENT_COW, entity->yaw, entity->pitch);
        ent->health = entity->health;
        memcpy(&ent->data, &entity->data, sizeof(union entity_data));
        world_spawn_entity(world, ent);
        world_despawn_entity(world, entity);
        struct item_info* info = getItemInfo(ITM_SHEARS);
        if (info != NULL && info->onItemAttacked != NULL) (*info->onItemAttacked)(world, interacter, slot_index, item, ent);
        struct slot slot;
        slot.item = BLK_MUSHROOM_1;
        slot.count = 5;
        slot.damage = 0;
        slot.nbt = NULL;
        dropEntityItem_explode(ent, &slot);	// TODO: fix?
    } else onInteract_cow(world, entity, interacter, item, slot_index);
}

void tick_arrow(struct world* world, struct entity* entity) {
    if (entity->data.arrow.ticksInGround == 0) {
        double hx = 0.;
        double hy = 0.;
        double hz = 0.;
        int hf = world_rayTrace(entity->world, entity->x, entity->y, entity->z, entity->x + entity->motX, entity->y + entity->motY, entity->z + entity->motZ, 0, 1, 0, &hx, &hy, &hz);
        struct entity* ehit = NULL;
        struct entity* shooter = world_get_entity(world, entity->objectData - 1);
        double bd = 999.;
        BEGIN_HASHMAP_ITERATION(world->entities);
        struct entity* e2 = value;
        double rd = entity_distsq_block(e2, entity->x + entity->motX, entity->y + entity->motY, entity->z + entity->motZ);
        if (rd > 4) {
            //printf("4d %f\n", rd);
        }
        if (e2 != entity && e2 != shooter && hasFlag(getEntityInfo(e2->type), "livingbase")) {		//todo: ticksInAir >= 5?
            struct boundingbox eb;
            getEntityCollision(e2, &eb);
            //eb.minX -= .3;
            //eb.maxX += .3;
            //eb.minY -= .3;
            //eb.maxY += .3;
            //eb.minZ -= .3;
            //eb.maxZ += .3;
            double rx = 0.;
            double ry = 0.;
            double rz = 0.;
            int face = world_blockRayTrace(&eb, 0, 0, 0, entity->x, entity->y, entity->z, entity->x + entity->motX, entity->y + entity->motY, entity->z + entity->motZ, &rx, &ry, &rz);
            if (face >= 0) {
                double dist = (entity->x + entity->motX - rx) * (entity->x + entity->motX - rx) + (entity->y + entity->motY - ry) * (entity->y + entity->motY - ry) + (entity->z + entity->motZ - rz) * (entity->z + entity->motZ - rz);		//entity_distsq_block(entity, rx, ry, rz);
                //printf("%f\n", dist);
                if (dist < bd) {
                    bd = dist;
                    ehit = e2;
                }
            }
        }
        END_HASHMAP_ITERATION(world->entities);
        if (ehit != NULL) {
            float speed = sqrtf(entity->motX * entity->motX + entity->motY * entity->motY + entity->motZ * entity->motZ);
            int damage = ceil(speed * entity->data.arrow.damage);
            if (entity->data.arrow.isCritical) damage += rand() % (damage / 2 + 2);
            //TODO: if burning and not enderman, set entity on fire for 5 ticks.
            if (damageEntity(ehit, damage, 1)) {
                //TODO: entity arrow count ++;
                if (entity->data.arrow.knockback > 0.) {
                    float hspeed = sqrtf(entity->motX * entity->motX + entity->motZ * entity->motZ);
                    if (hspeed > 0.) applyVelocity(ehit, entity->motX * entity->data.arrow.knockback * .6 / hspeed, .1, entity->motZ * entity->data.arrow.knockback * .6 / hspeed);
                }
            }				//TODO: else bounce
            //TODO: arrow sound
            if (ehit->type != ENT_ENDERMAN) {
                world_despawn_entity(world, entity);
                freeEntity(entity);
                return 1;
            }
        } else if (hf >= 0) {
            entity->x = hx;
            entity->y = hy;
            entity->z = hz;
            if (hf == YN) entity->y -= .001;
            if (hf == XN) entity->x -= .001;
            if (hf == ZN) entity->z -= .001;
            entity->motX = hx - entity->x;
            entity->motY = hy - entity->y;
            entity->motZ = hz - entity->z;
            //float ds = sqrt(entity->motX * entity->motX + entity->motY * entity->motY + entity->motZ * entity->motZ);
            //entity->x -= entity->motX / ds * 0.05000000074505806;
            //entity->y -= entity->motY / ds * 0.05000000074505806;
            //entity->z -= entity->motZ / ds * 0.05000000074505806;
            entity->data.arrow.ticksInGround = 1;
            entity->data.arrow.isCritical = 0;
            entity->immovable = 1;
            entity->last_x = 0.;
            entity->last_y = 0.;
            entity->last_z = 0.;
        }
    } else {
        if (entity->data.arrow.ticksInGround == 1) {
            entity->motX = 0.;
            entity->motY = 0.;
            entity->motZ = 0.;
        }
        entity->data.arrow.ticksInGround++;
        if (entity->data.arrow.ticksInGround == 1200) {
            world_despawn_entity(world, entity);
            freeEntity(entity);
            return 1;
        }
    }

    if (entity->data.arrow.ticksInGround == 0) {
        float dhz = sqrtf(entity->motX * entity->motX + entity->motZ * entity->motZ);
        entity->yaw = atan2f(entity->motX, entity->motZ) * 180. / M_PI;
        entity->pitch = atan2f(entity->motY, dhz) * 180. / M_PI;
        if ((entity->last_yaw == 0. && entity->last_pitch == 0.)) {
            entity->last_yaw = entity->yaw;
            entity->last_pitch = entity->pitch;
        } else {
            while (entity->pitch - entity->last_pitch < -180.)
                entity->last_pitch -= 360.;
            while (entity->pitch - entity->last_pitch >= 180.)
                entity->last_pitch += 360.;
            while (entity->yaw - entity->last_yaw < -180.)
                entity->last_yaw -= 360.;
            while (entity->yaw - entity->last_yaw >= 180.)
                entity->last_yaw += 360.;
            entity->pitch = entity->last_pitch + (entity->pitch - entity->last_pitch) * .2;
            entity->yaw = entity->last_yaw + (entity->yaw - entity->last_yaw) * .2;
        }
    }
    return 0;
}

void tick_itemstack(struct world* world, struct entity* entity) {
    if (entity->data.itemstack.delayBeforeCanPickup > 0) {
        entity->data.itemstack.delayBeforeCanPickup--;
        return 0;
    }
    if (entity->age >= 6000) {
        world_despawn_entity(world, entity);
        freeEntity(entity);
        return 1;
    }
    if (tick_counter % 10 != 0) return 0;
    struct boundingbox cebb;
    getEntityCollision(entity, &cebb);
    cebb.minX -= .625;
    cebb.maxX += .625;
    cebb.maxY += .75;
    cebb.minZ -= .625;
    cebb.maxZ += .625;
    struct boundingbox oebb;
    /*
    int32_t chunk_x = ((int32_t) entity->x) >> 4;
    int32_t chunk_z = ((int32_t) entity->z) >> 4;
    for (int32_t icx = chunk_x - 1; icx <= chunk_x + 1; icx++)
    for (int32_t icz = chunk_z - 1; icz <= chunk_z + 1; icz++) {
    struct chunk* ch = world_get_chunk(entity->world, icx, icz);
    */
    if (ch != NULL) {
        BEGIN_HASHMAP_ITERATION(entity->world->entities);
        struct entity* oe = (struct entity*) value;
        if (oe == entity || entity_distsq(entity, oe) > 16. * 16.) {

        }
        if (oe->type == ENT_PLAYER && oe->health > 0.) {
            getEntityCollision(oe, &oebb);
            //printf("%f, %f, %f vs %f, %f, %f\n", entity->x, entity->y, entity->z, oe->x, oe->y, oe->z);
            if (boundingbox_intersects(&oebb, &cebb)) {
                int os = entity->data.itemstack.slot->count;
                pthread_mutex_lock(&oe->data.player.player->inventory->mut);
                int r = inventory_add_player(oe->data.player.player, oe->data.player.player->inventory, entity->data.itemstack.slot, 1);
                pthread_mutex_unlock(&oe->data.player.player->inventory->mut);
                if (r <= 0) {
                    BEGIN_BROADCAST_DIST(entity, 32.);
                    struct packet* pkt = xmalloc(sizeof(struct packet));
                    pkt->id = PKT_PLAY_CLIENT_COLLECTITEM;
                    pkt->data.play_client.collectitem.collected_entity_id = entity->id;
                    pkt->data.play_client.collectitem.collector_entity_id = oe->id;
                    pkt->data.play_client.collectitem.pickup_item_count = os - r;
                    add_queue(bc_player->outgoing_packets, pkt);
                    END_BROADCAST(entity->world->players);
                    world_despawn_entity(world, entity);
                    freeEntity(entity);
            } else {
                    BEGIN_BROADCAST_DIST(entity, 128.);
                    struct packet* pkt = xmalloc(sizeof(struct packet));
                    pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
                    pkt->data.play_client.entitymetadata.entity_id = entity->id;
                    writeMetadata(entity, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
                    add_queue(bc_player->outgoing_packets, pkt);
                    END_BROADCAST(entity->world->players);
                    BREAK_HASHMAP_ITERATION(entity->world->entities);
            }
        }
    } else if (oe->type == ENT_ITEM) {
        if (oe->data.itemstack.slot->item == entity->data.itemstack.slot->item && oe->data.itemstack.slot->damage == entity->data.itemstack.slot->damage && oe->data.itemstack.slot->count + entity->data.itemstack.slot->count <= slot_max_size(entity->data.itemstack.slot)) {
            getEntityCollision(oe, &oebb);
            oebb.minX -= .625;
            oebb.maxX += .625;
            cebb.maxY += .75;
            oebb.minZ -= .625;
            oebb.maxZ += .625;
            if (boundingbox_intersects(&oebb, &cebb)) {
                world_despawn_entity(world, entity);
                oe->data.itemstack.slot->count += entity->data.itemstack.slot->count;
                freeEntity(entity);
                BEGIN_BROADCAST_DIST(oe, 128.);
                struct packet* pkt = xmalloc(sizeof(struct packet));
                pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
                pkt->data.play_client.entitymetadata.entity_id = oe->id;
                writeMetadata(oe, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
                add_queue(bc_player->outgoing_packets, pkt);
                END_BROADCAST(oe->world->players)
            }
        }
    }
    END_HASHMAP_ITERATION(entity->world->entities);
    return 0;
}
