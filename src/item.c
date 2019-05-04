#include <basin/item.h>
#include <basin/block.h>
#include <basin/nbt.h>
#include <basin/game.h>
#include <basin/player.h>
#include <avuna/json.h>
#include <avuna/string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

int onItemInteract_bed(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (face != YP) return 0;
    if (!getBlockInfo(world_get_block(world, x, y, z))->material->replacable) offsetCoordByFace(&x, &y, &z, face);
    uint32_t h = (uint32_t) floor(player->entity->yaw / 90. + .5) & 3;
    uint8_t rf = 0;
    if (h == 0) rf = SOUTH;
    else if (h == 1) rf = WEST;
    else if (h == 2) rf = NORTH;
    else if (h == 3) rf = EAST;
    int32_t x2 = x;
    int32_t y2 = y;
    int32_t z2 = z;
    offsetCoordByFace(&x2, &y2, &z2, rf);
    block pre = world_get_block(world, x, y, z);
    block pre2 = world_get_block(world, x2, y2, z2);
    block b = BLK_BED | h;
    block b2 = b;
    b2 |= 0b1000;
    if (player_can_place_block(player, b, x, y, z, face) && !world_set_block_noupdate(player->world, b, x, y, z)) {
        if (!player_can_place_block(player, b2, x2, y2, z2, face) || world_set_block(player->world, b2, x2, y2, z2)) {
            world_set_block(player->world, pre, x, y, z);
            world_set_block(player->world, pre2, x2, y2, z2);
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
            return 0;
        } else if (player->gamemode != 1) {
            if (--slot->count <= 0) {
                slot = NULL;
            }
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
        }
        world_update_block_guess(world, NULL, x, y, z);
        world_update_block_guess(world, NULL, x + 1, y, z);
        world_update_block_guess(world, NULL, x - 1, y, z);
        world_update_block_guess(world, NULL, x, y + 1, z);
        world_update_block_guess(world, NULL, x, y - 1, z);
        world_update_block_guess(world, NULL, x, y, z + 1);
        world_update_block_guess(world, NULL, x, y, z - 1);
    } else {
        world_set_block(player->world, pre, x, y, z);
        world_set_block(player->world, pre2, x2, y2, z2);
        inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
    }
    return 0;
}

int onItemInteract_painting(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    int32_t ox = x;
    int32_t oy = y;
    int32_t oz = z;
    offsetCoordByFace(&ox, &oy, &oz, face);
    if (face != YP && face != YN && player_can_place_block(player, ITM_PAINTING, ox, oy, oz, face)) {
        struct entity* ent = newEntity(nextEntityID++, (double) ox, (double) oy, (double) oz, ENT_PAINTING, 0., 0.);
        if (face == NORTH) ent->data.painting.direction = 2;
        if (face == SOUTH) ent->data.painting.direction = 0;
        if (face == WEST) ent->data.painting.direction = 1;
        if (face == EAST) ent->data.painting.direction = 3;
        block b = world_get_block(world, x, y, z);
        if (!isNormalCube(getBlockInfo(b))) return 0;
        b = world_get_block(world, ox, oy, oz);
        if (b != 0) return 0;
        int32_t p = -1;
        for (int32_t i = 0; i < 50; i++) {
            p = rand() % 26;
            if (p < 7) {

            }
            if (p >= 7) {
                b = world_get_block(world, x + (face == ZP ? 1 : (face == ZN ? -1 : 0)), y, z + (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (!isNormalCube(getBlockInfo(b))) continue;
                b = world_get_block(world, ox + (face == ZP ? 1 : (face == ZN ? -1 : 0)), oy, oz + (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (b != 0) return 0;
            }
            if (p >= 12) {
                b = world_get_block(world, x, y + 1, z);
                if (!isNormalCube(getBlockInfo(b))) continue;
                b = world_get_block(world, ox, oy + 1, oz);
                if (b != 0) return 0;
            }
            if (p >= 14) {
                b = world_get_block(world, x + (face == ZP ? 1 : (face == ZN ? -1 : 0)), y + 1, z + (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (!isNormalCube(getBlockInfo(b))) continue;
                b = world_get_block(world, ox + (face == ZP ? 1 : (face == ZN ? -1 : 0)), oy + 1, oz + (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (b != 0) return 0;
            }
            if (p >= 20) {
                b = world_get_block(world, x + (face == ZP ? 1 : (face == ZN ? -1 : 0)) * 2, y, z + (face == XP ? 1 : (face == XN ? -1 : 0)) * 2);
                if (!isNormalCube(getBlockInfo(b))) continue;
                b = world_get_block(world, x - (face == ZP ? 1 : (face == ZN ? -1 : 0)), y, z - (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (!isNormalCube(getBlockInfo(b))) continue;
                b = world_get_block(world, x + (face == ZP ? 1 : (face == ZN ? -1 : 0)) * 2, y + 1, z + (face == XP ? 1 : (face == XN ? -1 : 0))) * 2;
                if (!isNormalCube(getBlockInfo(b))) continue;
                b = world_get_block(world, x - (face == ZP ? 1 : (face == ZN ? -1 : 0)), y + 1, z - (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (!isNormalCube(getBlockInfo(b))) continue;
                b = world_get_block(world, ox + (face == ZP ? 1 : (face == ZN ? -1 : 0)) * 2, oy, oz + (face == XP ? 1 : (face == XN ? -1 : 0)) * 2);
                if (b != 0) return 0;
                b = world_get_block(world, ox - (face == ZP ? 1 : (face == ZN ? -1 : 0)), oy, oz - (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (b != 0) return 0;
                b = world_get_block(world, ox + (face == ZP ? 1 : (face == ZN ? -1 : 0)) * 2, oy + 1, oz + (face == XP ? 1 : (face == XN ? -1 : 0))) * 2;
                if (b != 0) return 0;
                b = world_get_block(world, ox - (face == ZP ? 1 : (face == ZN ? -1 : 0)), oy + 1, oz - (face == XP ? 1 : (face == XN ? -1 : 0)));
                if (b != 0) return 0;
            }
            if (p >= 21) {
                for (int32_t i = 2; i <= (p < 24 ? 2 : 3); i++) {
                    b = world_get_block(world, x + (face == ZP ? 1 : (face == ZN ? -1 : 0)) * 2, y + i, z + (face == XP ? 1 : (face == XN ? -1 : 0)) * 2);
                    if (!isNormalCube(getBlockInfo(b))) continue;
                    b = world_get_block(world, x + (face == ZP ? 1 : (face == ZN ? -1 : 0)), y + i, z + (face == XP ? 1 : (face == XN ? -1 : 0)));
                    if (!isNormalCube(getBlockInfo(b))) continue;
                    b = world_get_block(world, x * 2, y + i, z);
                    if (!isNormalCube(getBlockInfo(b))) continue;
                    b = world_get_block(world, x - (face == ZP ? 1 : (face == ZN ? -1 : 0)), y + i, z - (face == XP ? 1 : (face == XN ? -1 : 0)));
                    if (!isNormalCube(getBlockInfo(b))) continue;
                    b = world_get_block(world, ox + (face == ZP ? 1 : (face == ZN ? -1 : 0)) * 2, oy + i, oz + (face == XP ? 1 : (face == XN ? -1 : 0)) * 2);
                    if (b != 0) return 0;
                    b = world_get_block(world, ox + (face == ZP ? 1 : (face == ZN ? -1 : 0)), oy + i, oz + (face == XP ? 1 : (face == XN ? -1 : 0)));
                    if (b != 0) return 0;
                    b = world_get_block(world, ox, oy + i, oz);
                    if (b != 0) return 0;
                    b = world_get_block(world, ox - (face == ZP ? 1 : (face == ZN ? -1 : 0)), oy + i, oz - (face == XP ? 1 : (face == XN ? -1 : 0)));
                    if (b != 0) return 0;
                }
            }
            break;
        }
        if (p < 0) {
            freeEntity(ent);
            return 0;
        }
        if (p == 0) ent->data.painting.title = "Kebab";
        else if (p == 1) ent->data.painting.title = "Aztec";
        else if (p == 2) ent->data.painting.title = "Alban";
        else if (p == 3) ent->data.painting.title = "Aztec2";
        else if (p == 4) ent->data.painting.title = "Bomb";
        else if (p == 5) ent->data.painting.title = "Plant";
        else if (p == 6) ent->data.painting.title = "Wasteland";
        else if (p == 7) ent->data.painting.title = "Pool";
        else if (p == 8) ent->data.painting.title = "Courbet";
        else if (p == 9) ent->data.painting.title = "Sea";
        else if (p == 10) ent->data.painting.title = "Sunset";
        else if (p == 11) ent->data.painting.title = "Creebet";
        else if (p == 12) ent->data.painting.title = "Wanderer";
        else if (p == 13) ent->data.painting.title = "Graham";
        else if (p == 14) ent->data.painting.title = "Match";
        else if (p == 15) ent->data.painting.title = "Bust";
        else if (p == 16) ent->data.painting.title = "Stage";
        else if (p == 17) ent->data.painting.title = "Void";
        else if (p == 18) ent->data.painting.title = "SkullAndRoses";
        else if (p == 19) ent->data.painting.title = "Wither";
        else if (p == 20) ent->data.painting.title = "Fighters";
        else if (p == 21) ent->data.painting.title = "Pointer";
        else if (p == 22) ent->data.painting.title = "Pigscene";
        else if (p == 23) ent->data.painting.title = "BurningSkull";
        else if (p == 24) ent->data.painting.title = "Skeleton";
        else if (p == 25) ent->data.painting.title = "DonkeyKong";
        world_spawn_entity(world, ent);
        if (player->gamemode != 1) inventory_set_slot(player, player->inventory, 36 + player->currentItem, NULL, 1, 1);
    }
    return 0;

}

int onItemInteract_minecart(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    block b = world_get_block(world, x, y, z);
    if (b >> 4 != BLK_RAIL >> 4 && b >> 4 != BLK_ACTIVATORRAIL >> 4 && b >> 4 != BLK_GOLDENRAIL >> 4 && b >> 4 != BLK_DETECTORRAIL >> 4) return 0;
    double dy = 0.;
    if (b & 0b0110) dy += .5;
    uint16_t et = ENT_MINECARTRIDEABLE;
    if (slot->item == ITM_MINECARTCHEST) et = ENT_MINECARTCHEST;
    else if (slot->item == ITM_MINECARTFURNACE) et = ENT_MINECARTFURNACE;
    else if (slot->item == ITM_MINECARTTNT) et = ENT_MINECARTTNT;
    else if (slot->item == ITM_MINECARTHOPPER) et = ENT_MINECARTHOPPER;
    else if (slot->item == ITM_MINECARTCOMMANDBLOCK) et = ENT_MINECARTCOMMANDBLOCK;
    struct entity* ent = newEntity(nextEntityID++, (double) x + .5, (double) y + dy, (double) z + .5, et, 0., 0.);
    world_spawn_entity(world, ent);
    if (player->gamemode != 1) inventory_set_slot(player, player->inventory, 36 + player->currentItem, NULL, 1, 1);
    return 0;

}

int onItemInteract_door(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face, float cx, float cy, float cz) {
    if (face != YP) return 0;
    if (!getBlockInfo(world_get_block(world, x, y, z))->material->replacable) offsetCoordByFace(&x, &y, &z, face);
    block pre = world_get_block(world, x, y, z);
    block pre2 = world_get_block(world, x, y + 1, z);
    block b = BLK_DOOROAK;
    if (slot->item == ITM_DOORSPRUCE) b = BLK_DOORSPRUCE;
    else if (slot->item == ITM_DOORBIRCH) b = BLK_DOORBIRCH;
    else if (slot->item == ITM_DOORJUNGLE) b = BLK_DOORJUNGLE;
    else if (slot->item == ITM_DOORACACIA) b = BLK_DOORACACIA;
    else if (slot->item == ITM_DOORDARKOAK) b = BLK_DOORDARKOAK;
    else if (slot->item == ITM_DOORIRON) b = BLK_DOORIRON;
    block b2 = b;
    uint32_t h = (uint32_t) floor(player->entity->yaw / 90. + .5) & 3;
    if (h == 0) b |= 1;
    else if (h == 1) b |= 2;
    else if (h == 2) b |= 3; // should fold into "f" below
    uint8_t f = getFaceFromPlayer(player);
    if ((f == XN && cz < .5) || (f == XP && cz > .5) || (f == ZN && cx > .5) || (f == ZP && cx < .5)) {
        b2 |= 0x01;
    }
    b2 |= 0b1000;
    if (player_can_place_block(player, b, x, y, z, face) && !world_set_block_noupdate(player->world, b, x, y, z)) {
        if (!player_can_place_block(player, b2, x, y + 1, z, face) || world_set_block(player->world, b2, x, y + 1, z)) {
            world_set_block(player->world, pre, x, y, z);
            world_set_block(player->world, pre2, x, y + 1, z);
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
            return 0;
        } else if (player->gamemode != 1) {
            if (--slot->count <= 0) {
                slot = NULL;
            }
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
        }
        world_update_block_guess(world, NULL, x, y, z);
        world_update_block_guess(world, NULL, x + 1, y, z);
        world_update_block_guess(world, NULL, x - 1, y, z);
        world_update_block_guess(world, NULL, x, y + 1, z);
        world_update_block_guess(world, NULL, x, y - 1, z);
        world_update_block_guess(world, NULL, x, y, z + 1);
        world_update_block_guess(world, NULL, x, y, z - 1);
    } else {
        world_set_block(player->world, pre, x, y, z);
        world_set_block(player->world, pre2, x, y + 1, z);
        inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
    }
    return 0;
}

int onItemInteract_itemblock(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (!getBlockInfo(world_get_block(world, x, y, z))->material->replacable) offsetCoordByFace(&x, &y, &z, face);
    block pre = world_get_block(world, x, y, z);
    uint8_t f = 0;
    uint32_t h = (uint32_t) floor(player->entity->yaw / 90. + .5) & 3;
    if (h == 0) f = SOUTH;
    else if (h == 1) f = WEST;
    else if (h == 2) f = NORTH;
    else if (h == 3) f = EAST;
    block b = pre;
    if (slot->item == ITM_STRING) {
        b = BLK_TRIPWIRE;
    } else if (slot->item == ITM_REDSTONE) {
        b = BLK_REDSTONEDUST;
    } else if (slot->item == ITM_CAKE) {
        b = BLK_CAKE;
    } else if (slot->item == ITM_DIODE) {
        b = BLK_DIODE;
        if (f == SOUTH) {
            b |= 2;
        } else if (f == WEST) {
            b |= 3;
        } else if (f == NORTH) {
            b |= 0;
        } else if (f == EAST) {
            b |= 1;
        }
    } else if (slot->item == ITM_BREWINGSTAND) {
        b = BLK_BREWINGSTAND;
    } else if (slot->item == ITM_CAULDRON) {
        b = BLK_CAULDRON;
    } else if (slot->item == ITM_FLOWERPOT) {
        b = BLK_FLOWERPOT;
    } else if (slot->item == ITM_COMPARATOR) {
        b = BLK_COMPARATOR;
    }
    if (player_can_place_block(player, b, x, y, z, face)) {
        if (world_set_block(player->world, b, x, y, z)) {
            world_set_block(player->world, pre, x, y, z);
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
        } else if (player->gamemode != 1) {
            if (--slot->count <= 0) {
                slot = NULL;
            }
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
        }
    } else {
        world_set_block(player->world, pre, x, y, z);
        inventory_set_slot(player, player->inventory, 36 + player->currentItem, slot, 1);
    }
    return 0;
}

int onItemBreakBlock_tool(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z) {
    if (slot == NULL) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    return 0;
}

float onItemAttacked_tool(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, struct entity* entity) {
    if (slot == NULL) return 1.;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 1.;
    if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    return ii->damage;
}

void offsetCoordByFace(int32_t* x, int32_t* y, int32_t* z, uint8_t face) {
    if (face == YN) (*y)--;
    else if (face == YP) (*y)++;
    else if (face == ZN) (*z)--;
    else if (face == ZP) (*z)++;
    else if (face == XN) (*x)--;
    else if (face == XP) (*x)++;
}

int onItemInteract_flintandsteel(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL) return 0;
    offsetCoordByFace(&x, &y, &z, face);
    if (world_get_block(world, x, y, z) != 0) return 0;
    if (!player_can_place_block(player, BLK_FIRE, x, y, z, face)) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    world_set_block(world, BLK_FIRE, x, y, z);
    return 0;
}

int onItemInteract_spawnegg(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL || slot->nbt == NULL) return 0;
    //TODO: mob spawners
    if (getBlockInfo(world_get_block(world, x, y, z))->boundingBox_count > 0) offsetCoordByFace(&x, &y, &z, face);
    //if (world_get_block(world, x, y, z) != 0) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    struct nbt_tag* et = nbt_get(slot->nbt, "EntityTag");
    if (et == NULL) return 0;
    struct nbt_tag* tmp = nbt_get(et, "id");
    if (tmp == NULL || tmp->id != NBT_TAG_STRING) return 0;
    uint32_t etx = getIDFromEntityDataName(tmp->data.nbt_string);
    struct entity* ent = newEntity(nextEntityID++, (float) x + .5, (float) y, (float) z + .5, etx, 0., 0.);
    world_spawn_entity(world, ent);
    if (player->gamemode != 1 && --slot->count <= 0) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    return 0;
}

int onItemInteract_reeds(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL) return 0;
    if (!getBlockInfo(world_get_block(world, x, y, z))->material->replacable) offsetCoordByFace(&x, &y, &z, face);
    if (!player_can_place_block(player, BLK_REEDS, x, y, z, face)) return 0;
    if (world_get_block(world, x, y, z) != 0) return 0;
    if (!canBePlaced_reeds(world, BLK_REEDS, x, y, z)) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    if (player->gamemode != 1 && --slot->count <= 0) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    world_set_block(world, BLK_REEDS, x, y, z);
    return 0;
}

int onItemInteract_bucket(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL) return 0;
    if (!getBlockInfo(world_get_block(world, x, y, z))->material->replacable) offsetCoordByFace(&x, &y, &z, face);
    block b = world_get_block(world, x, y, z);
    struct block_info* bi = getBlockInfo(b);
    if (bi == NULL) return 0;
    uint16_t ba = b >> 4;
    if (slot->item == ITM_BUCKETWATER) {
        if (b != 0 && !bi->material->replacable && ba != BLK_WATER >> 4 && ba != BLK_WATER_1 >> 4) return 0;
        if (!player_can_place_block(player, BLK_WATER_1, x, y, z, face)) return 0;
        world_set_block(world, BLK_WATER_1, x, y, z);
        if (player->gamemode != 1) {
            slot->item = ITM_BUCKET;
            inventory_set_slot(player, player->inventory, slot_index, slot, 1);
        }
    } else if (slot->item == ITM_BUCKETLAVA) {
        if (b != 0 && !bi->material->replacable && ba != BLK_LAVA >> 4 && ba != BLK_LAVA_1 >> 4) return 0;
        if (!player_can_place_block(player, BLK_LAVA_1, x, y, z, face)) return 0;
        world_set_block(world, BLK_LAVA_1, x, y, z);
        if (player->gamemode != 1) {
            slot->item = ITM_BUCKET;
            inventory_set_slot(player, player->inventory, slot_index, slot, 1);
        }
    } else if (slot->item == ITM_BUCKET) {
        if ((b & 0x0f) != 0) return 0;
        if (ba == BLK_WATER >> 4 || ba == BLK_WATER_1 >> 4) {
            world_set_block(world, 0, x, y, z);
            if (player->gamemode != 1) {
                slot->item = ITM_BUCKETWATER;
                inventory_set_slot(player, player->inventory, slot_index, slot, 1);
            }
        } else if (ba == BLK_LAVA >> 4 || ba == BLK_LAVA_1 >> 4) {
            world_set_block(world, 0, x, y, z);
            if (player->gamemode != 1) {
                slot->item = ITM_BUCKETLAVA;
                inventory_set_slot(player, player->inventory, slot_index, slot, 1);
            }
        }
    }
    return 0;
}

int onItemInteract_bonemeal(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL) return 0;
    if (!player_can_place_block(player, ITM_DYEPOWDER_BLACK, x, y, z, face)) return 0;
    block b = world_get_block(world, x, y, z);
    uint16_t ba = b >> 4;
    if (ba == (BLK_CROPS >> 4) || (ba == BLK_PUMPKINSTEM >> 4) || (ba == BLK_PUMPKINSTEM_1 >> 4) || (ba == BLK_CARROTS >> 4) || (ba == BLK_POTATOES >> 4) || (ba == BLK_BEETROOTS >> 4) || (ba == BLK_COCOA >> 4)) {
        uint16_t grow = 2 + rand() % 3;
        uint16_t maxAge = 7;
        if (b == BLK_BEETROOTS) maxAge = 3;
        uint16_t age = b & 0x0f;
        age += grow;
        if (age > maxAge) age = maxAge;
        world_set_block(world, (ba << 4) | age, x, y, z);
    } else if (ba == (BLK_SAPLING_OAK >> 4)) {
        randomTick_sapling(world, world_get_chunk(world, x >> 4, z >> 4), b, x, y, z);
    } else if (b == BLK_GRASS || b == BLK_DIRT) {
        for (int i = 0; i < 128; i++) {
            int j = 0;
            int32_t bx = x;
            uint8_t by = y + 1;
            int32_t bz = z;
            while (1) {
                block bb = world_get_block(world, bx, by - 1, bz);
                block b = world_get_block(world, bx, by, bz);
                struct block_info* bi = getBlockInfo(b);
                if (j >= i / 16) {
                    if (streq_nocase(bi->material->name, "air")) {
                        if (rand() % 8 == 0) {
                            block nb = rand() % 3 > 0 ? BLK_FLOWER1_DANDELION : BLK_FLOWER2_POPPY; // TODO: biome registry
                            if (bb == BLK_DIRT || bb == BLK_GRASS) {
                                world_set_block(world, nb, bx, by, bz);
                            }
                        } else {
                            block nb = BLK_TALLGRASS_GRASS;
                            if (bb == BLK_DIRT || bb == BLK_GRASS) {
                                world_set_block(world, nb, bx, by, bz);
                            }
                        }
                    }
                    break;
                }
                bx += rand() % 3 - 1;
                by += (rand() % 3 - 1) * (rand() % 3) / 2;
                bz += rand() % 3 - 1;
                if (bb != BLK_GRASS || isNormalCube(bi)) break;
                j++;
            }
        }
    } else return 0;
    if (player->gamemode != 1 && --slot->count <= 0) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    return 0;
}

int onItemInteract_seeds(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL) return 0;
    if (!getBlockInfo(world_get_block(world, x, y, z))->material->replacable) offsetCoordByFace(&x, &y, &z, face);
    if (!player_can_place_block(player, ITM_DYEPOWDER_BLACK, x, y, z, face)) return 0;
    block b = world_get_block(world, x, y, z);
    if (slot->item == ITM_DYEPOWDER_BLACK) {
        if (slot->damage == 3 && (face == 1 || face == 0 || b != BLK_LOG_JUNGLE)) return 0;
        if (slot->damage == 15) return onItemInteract_bonemeal(world, player, slot_index, slot, x, y, z, face);
        else if (slot->damage != 3) return 0;
    } else if (face != 1 || (b >> 4) != (slot->item == ITM_NETHERSTALKSEEDS ? (BLK_HELLSAND >> 4) : (BLK_FARMLAND >> 4))) return 0;
    b = world_get_block(world, x, y, z);
    if (b != 0) return 0;
    block tp = 0;
    if (slot->item == ITM_SEEDS) tp = BLK_CROPS;
    else if (slot->item == ITM_SEEDS_PUMPKIN) tp = BLK_PUMPKINSTEM;
    else if (slot->item == ITM_SEEDS_MELON) tp = BLK_PUMPKINSTEM_1;
    else if (slot->item == ITM_CARROTS) tp = BLK_CARROTS;
    else if (slot->item == ITM_POTATO) tp = BLK_POTATOES;
    else if (slot->item == ITM_NETHERSTALKSEEDS) tp = BLK_NETHERSTALK;
    else if (slot->item == ITM_BEETROOT_SEEDS) tp = BLK_BEETROOTS;
    else if (slot->item == ITM_DYEPOWDER_BLACK) tp = BLK_COCOA;
    if (player->gamemode != 1 && --slot->count <= 0) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    world_set_block(world, tp, x, y, z);
    return 0;
}

int onItemInteract_hoe(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    block b = world_get_block(world, x, y, z);
    if (b == BLK_DIRT || b == BLK_GRASS) {
        if (!player_can_place_block(player, slot->item, x, y, z, face)) return 0;
        if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
            slot = NULL;
        }
        inventory_set_slot(player, player->inventory, slot_index, slot, 1);
        world_set_block(world, BLK_FARMLAND, x, y, z);
    }
    return 0;
}

int onItemInteract_shovel(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (slot == NULL) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    block b = world_get_block(world, x, y, z);
    if (b == BLK_GRASS) {
        if (!player_can_place_block(player, slot->item, x, y, z, face)) return 0;
        if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
            slot = NULL;
        }
        inventory_set_slot(player, player->inventory, slot_index, slot, 1);
        world_set_block(world, BLK_GRASSPATH, x, y, z);
    }
    return 0;
}

void onItemUse_armor(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, uint32_t ticks) {
    if (slot == NULL) return;
    uint16_t sli = 5;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii->armorType == ARMOR_HELMET) sli = 5;
    else if (ii->armorType == ARMOR_CHESTPLATE) sli = 6;
    else if (ii->armorType == ARMOR_LEGGINGS) sli = 7;
    else if (ii->armorType == ARMOR_BOOTS) sli = 8;
    if (inventory_get(player, player->inventory, sli) != NULL) return;
    inventory_set_slot(player, player->inventory, sli, slot, 1);
    inventory_set_slot(player, player->inventory, slot_index, NULL, 1, 0);
}

float onEntityHitWhileWearing_armor(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, float damage) {
    if (slot == NULL) return damage;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return damage;
    if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
        slot = NULL;
    }
    inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    return damage;
}

int bow_isArrow(struct slot* slot) {
    item i = slot == NULL ? 0 : slot->item;
    return i == ITM_ARROW || i == ITM_SPECTRAL_ARROW || i == ITM_TIPPED_ARROW || i == ITM_LINGERING_POTION;
}

int bow_findAmmo(struct player* player) {
    struct slot* tmp = inventory_get(player, player->inventory, 45);
    if (bow_isArrow(tmp)) return 45;
    tmp = inventory_get(player, player->inventory, 36 + player->currentItem);
    if (bow_isArrow(tmp)) return 36 + player->currentItem;
    for (int i = 0; i < player->inventory->slot_count; i++) {
        struct slot* tmp = inventory_get(player, player->inventory, i);
        if (bow_isArrow(tmp)) return i;
    }
    return -1;
}

void onItemUse_bow(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, uint32_t ticks) {
    if (slot == NULL) return;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return;
    int bs = bow_findAmmo(player);
    struct slot* ammo = NULL;
    if (player->gamemode != 1) {
        if (bs < 0) return;
        //TODO: or infinity enchantment
        ammo = inventory_get(player, player->inventory, bs);
    }
    float velocity = (float) ticks / 20.;
    if (velocity > 1.) velocity = 1.;
    velocity = (velocity * velocity + velocity * 2.) / 3.;
    if (velocity > 1.) velocity = 1.;
    if (velocity >= .1) {
        int sp = ammo != NULL && ammo->item == ITM_SPECTRAL_ARROW;
        struct entity* arrow = newEntity(nextEntityID++, player->entity->x, player->entity->y + 1.52, player->entity->z, sp ? ENT_SPECTRALARROW : ENT_ARROW, player->entity->yaw, player->entity->pitch);
        //player->entity->pitch = 0.;
        //player->entity->yaw = 0.;
        float x = -sinf(player->entity->yaw / 360. * 2 * M_PI) * cosf(player->entity->pitch / 360. * 2 * M_PI);
        float y = -sinf(player->entity->pitch / 360. * 2 * M_PI);
        float z = cosf(player->entity->yaw / 360. * 2 * M_PI) * cosf(player->entity->pitch / 360. * 2 * M_PI);
        float s = sqrtf(x * x + y * y + z * z);
        x /= s;
        y /= s;
        z /= s;
        //TODO: inaccuracy calc gaussian
        x *= velocity * 3.;
        y *= velocity * 3.;
        z *= velocity * 3.;
        arrow->motX = x;
        arrow->motY = y;
        arrow->motZ = z;
        float sr = sqrtf(x * x + z * z);
        arrow->yaw = atan2f(x, z) * 180. / M_PI;
        arrow->pitch = atan2f(y, sr) * 180. / M_PI;
        arrow->last_yaw = arrow->yaw;
        arrow->last_pitch = arrow->pitch;
        //arrow->motX += player->entity->motX;
        //arrow->motZ += player->entity->motZ;
        //if (!player->entity->onGround) arrow->motY += player->entity->motY;
        if (velocity == 1.) arrow->data.arrow.isCritical = 1;
        arrow->data.arrow.damage = 2.;
        arrow->objectData = 1 + player->entity->id;
        arrow->data.arrow.knockback = 1.;

        //TODO: power enchant
        //TODO: punch enchant
        //TODO: flame enchant
        if (ammo != NULL) {
            if (--ammo->count <= 0) {
                ammo = NULL;
            }
            inventory_set_slot(player, player->inventory, bs, ammo, 1, 1);
        }
        world_spawn_entity(player->world, arrow);
    }
}

int canUseItem_bow(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot) {
    if (slot == NULL) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    if (player->gamemode != 1 && bow_findAmmo(player) < 0) return 0;
    return 1;
}

int canUseItem_food(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot) {
    if (slot == NULL) return 0;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return 0;
    struct itemfood_arg* arg = ii->callback_arg;
    if (!arg->alwaysEat && player->food >= 20) return 0;
    return 1;
}

void onItemUse_food(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, uint32_t ticks) {
    if (slot == NULL) return;
    struct item_info* ii = getItemInfo(slot->item);
    if (ii == NULL) return;
    struct itemfood_arg* arg = ii->callback_arg;
    if (ticks >= 32) {
        player->food += arg->food;
        if (player->food > 20) player->food = 20;
        player->saturation += arg->saturation * arg->food * 2.;
        if (player->saturation > player->food) player->saturation = player->food;
        player_hungerUpdate(player);
        if (--slot->count <= 0) slot = NULL;
        inventory_set_slot(player, player->inventory, slot_index, slot, 1);
    }
}

void init_food(item i, int food, float saturation, int alwaysEat) {
    struct itemfood_arg* ia = xmalloc(sizeof(struct itemfood_arg));
    ia->food = food;
    ia->saturation = saturation;
    ia->alwaysEat = alwaysEat;
    struct item_info* ii = getItemInfo(i);
    ii->callback_arg = ia;
    ii->onItemUse = &onItemUse_food;
    ii->canUseItem = &canUseItem_food;
}

struct collection* item_infos;

struct item_info* getItemInfo(item id) {
    if (id < 0 || id > item_infos->size) return NULL;
    return item_infos->data[id];
}

void init_items() {
    item_infos = new_collection(128, 0);
    char* jsf = xmalloc(4097);
    size_t jsfs = 4096;
    size_t jsr = 0;
    int fd = open("items.json", O_RDONLY);
    ssize_t r = 0;
    while ((r = read(fd, jsf + jsr, jsfs - jsr)) > 0) {
        jsr += r;
        if (jsfs - jsr < 512) {
            jsfs += 4096;
            jsf = xrealloc(jsf, jsfs + 1);
        }
    }
    jsf[jsr] = 0;
    if (r < 0) {
        printf("Error reading item information: %s\n", strerror(errno));
    }
    close(fd);
    struct json_object json;
    json_parse(&json, jsf);
    for (size_t i = 0; i < json.child_count; i++) {
        struct json_object* ur = json.children[i];
        struct item_info* ii = xcalloc(sizeof(struct item_info));
        struct json_object* tmp = json_get(ur, "id");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        item id = (item) tmp->data.number;
        if (id < 0) goto cerr;
        tmp = json_get(ur, "toolType");
        if (tmp == NULL || tmp->type != JSON_STRING) goto cerr;
        if (streq_nocase(tmp->data.string, "none")) ii->toolType = NULL;
        else ii->toolType = tools_get(tmp->data.string);
        tmp = json_get(ur, "harvestLevel");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->harvestLevel = (uint8_t) tmp->data.number;
        tmp = json_get(ur, "maxStackSize");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->maxStackSize = (uint8_t) tmp->data.number;
        if (ii->maxStackSize > 64) goto cerr;
        tmp = json_get(ur, "maxDamage");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->maxDamage = (int16_t) tmp->data.number;
        tmp = json_get(ur, "maxUseDuration");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->maxUseDuration = (uint32_t) tmp->data.number;
        tmp = json_get(ur, "toolProficiency");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->toolProficiency = (float) tmp->data.number;
        tmp = json_get(ur, "damage");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->damage = (float) tmp->data.number;
        tmp = json_get(ur, "armorType");
        if (tmp == NULL || tmp->type != JSON_STRING) goto cerr;
        if (streq_nocase(tmp->data.string, "none")) ii->armorType = ARMOR_NONE;
        else if (streq_nocase(tmp->data.string, "helmet")) ii->armorType = ARMOR_HELMET;
        else if (streq_nocase(tmp->data.string, "chestplate")) ii->armorType = ARMOR_CHESTPLATE;
        else if (streq_nocase(tmp->data.string, "leggings")) ii->armorType = ARMOR_LEGGINGS;
        else if (streq_nocase(tmp->data.string, "boots")) ii->armorType = ARMOR_BOOTS;
        else goto cerr;
        tmp = json_get(ur, "armor");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->armor = (uint8_t) tmp->data.number;
        tmp = json_get(ur, "attackSpeed");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
        ii->attackSpeed = (float) tmp->data.number;
        ensure_collection(item_infos, id + 1);
        item_infos->data[id] = ii;
        if (item_infos->size < id) item_infos->size = id;
        item_infos->count++;
        continue;
        cerr: ;
        printf("[WARNING] Error Loading Item \"%s\"! Skipped.\n", ur->name);
    }
    freeJSON(&json);
    xfree(jsf);
//
    for (item i = ITM_SHOVELIRON; i <= ITM_HATCHETIRON; i++) {
        getItemInfo(i)->onItemBreakBlock = &onItemBreakBlock_tool;
        getItemInfo(i)->onItemAttacked = &onItemAttacked_tool;
    }
    getItemInfo(ITM_FLINTANDSTEEL)->onItemInteract = &onItemInteract_flintandsteel;
    for (item i = ITM_SWORDIRON; i <= ITM_HATCHETDIAMOND; i++) {
        getItemInfo(i)->onItemBreakBlock = &onItemBreakBlock_tool;
        getItemInfo(i)->onItemAttacked = &onItemAttacked_tool;
    }
    for (item i = ITM_SWORDGOLD; i <= ITM_HATCHETGOLD; i++) {
        getItemInfo(i)->onItemBreakBlock = &onItemBreakBlock_tool;
        getItemInfo(i)->onItemAttacked = &onItemAttacked_tool;
    }
    getItemInfo(ITM_SHEARS)->onItemBreakBlock = &onItemBreakBlock_tool;
    for (item i = ITM_HOEWOOD; i <= ITM_HOEGOLD; i++) {
        getItemInfo(i)->onItemInteract = &onItemInteract_hoe;
    }
    for (item i = ITM_HELMETCLOTH; i <= ITM_BOOTSGOLD; i++) {
        getItemInfo(i)->onEntityHitWhileWearing = &onEntityHitWhileWearing_armor;
        getItemInfo(i)->onItemUse = &onItemUse_armor;
    }
    getItemInfo(ITM_DOOROAK)->onItemInteract = &onItemInteract_door;
    getItemInfo(ITM_DOORSPRUCE)->onItemInteract = &onItemInteract_door;
    getItemInfo(ITM_DOORBIRCH)->onItemInteract = &onItemInteract_door;
    getItemInfo(ITM_DOORJUNGLE)->onItemInteract = &onItemInteract_door;
    getItemInfo(ITM_DOORACACIA)->onItemInteract = &onItemInteract_door;
    getItemInfo(ITM_DOORDARKOAK)->onItemInteract = &onItemInteract_door;
    getItemInfo(ITM_DOORIRON)->onItemInteract = &onItemInteract_door;
    getItemInfo(ITM_SEEDS)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_SEEDS_PUMPKIN)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_SEEDS_MELON)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_CARROTS)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_POTATO)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_NETHERSTALKSEEDS)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_BEETROOT_SEEDS)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_DYEPOWDER_BLACK)->onItemInteract = &onItemInteract_seeds;
    getItemInfo(ITM_SHOVELWOOD)->onItemInteract = &onItemInteract_shovel;
    getItemInfo(ITM_SHOVELGOLD)->onItemInteract = &onItemInteract_shovel;
    getItemInfo(ITM_SHOVELSTONE)->onItemInteract = &onItemInteract_shovel;
    getItemInfo(ITM_SHOVELIRON)->onItemInteract = &onItemInteract_shovel;
    getItemInfo(ITM_SHOVELDIAMOND)->onItemInteract = &onItemInteract_shovel;
    getItemInfo(ITM_REEDS)->onItemInteract = &onItemInteract_reeds;
    getItemInfo(ITM_BUCKET)->onItemInteract = &onItemInteract_bucket;
    getItemInfo(ITM_BUCKETWATER)->onItemInteract = &onItemInteract_bucket;
    getItemInfo(ITM_BUCKETLAVA)->onItemInteract = &onItemInteract_bucket;
    getItemInfo(ITM_MONSTERPLACER)->onItemInteract = &onItemInteract_spawnegg;
    getItemInfo(ITM_BOW)->onItemUse = &onItemUse_bow;
    getItemInfo(ITM_BOW)->canUseItem = &canUseItem_bow;
    getItemInfo(ITM_STRING)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_REDSTONE)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_CAKE)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_DIODE)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_BREWINGSTAND)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_CAULDRON)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_FLOWERPOT)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_COMPARATOR)->onItemInteract = &onItemInteract_itemblock;
    getItemInfo(ITM_MINECART)->onItemInteract = &onItemInteract_minecart;
    getItemInfo(ITM_MINECARTCHEST)->onItemInteract = &onItemInteract_minecart;
    getItemInfo(ITM_MINECARTFURNACE)->onItemInteract = &onItemInteract_minecart;
    getItemInfo(ITM_MINECARTTNT)->onItemInteract = &onItemInteract_minecart;
    getItemInfo(ITM_MINECARTHOPPER)->onItemInteract = &onItemInteract_minecart;
    getItemInfo(ITM_MINECARTCOMMANDBLOCK)->onItemInteract = &onItemInteract_minecart;
    getItemInfo(ITM_PAINTING)->onItemInteract = &onItemInteract_painting;
    getItemInfo(ITM_BED)->onItemInteract = &onItemInteract_bed;
    init_food(ITM_APPLE, 4, 0.3, 0);
    init_food(ITM_MUSHROOMSTEW, 6, 0.6, 0);
    init_food(ITM_BREAD, 5, 0.6, 0);
    init_food(ITM_PORKCHOPRAW, 3, 0.3, 0);
    init_food(ITM_PORKCHOPCOOKED, 8, 0.8, 0);
    init_food(ITM_APPLEGOLD, 4, 1.2, 1);
    init_food(ITM_FISH_COD_RAW, 2, 0.1, 0);
    init_food(ITM_FISH_COD_COOKED, 5, 0.6, 0);
    init_food(ITM_COOKIE, 2, 0.1, 0);
    init_food(ITM_MELON, 2, 0.3, 0);
    init_food(ITM_BEEFRAW, 3, 0.3, 0);
    init_food(ITM_BEEFCOOKED, 8, 0.8, 0);
    init_food(ITM_CHICKENRAW, 2, 0.3, 0);
    init_food(ITM_CHICKENCOOKED, 6, 0.6, 0);
    init_food(ITM_ROTTENFLESH, 4, 0.1, 0);
    init_food(ITM_SPIDEREYE, 2, 0.8, 0);
    init_food(ITM_CARROTS, 3, 0.6, 0);
    init_food(ITM_POTATO, 1, 0.3, 0);
    init_food(ITM_POTATOBAKED, 5, 0.6, 0);
    init_food(ITM_POTATOPOISONOUS, 2, 0.3, 0);
    init_food(ITM_CARROTGOLDEN, 6, 1.2, 0);
    init_food(ITM_PUMPKINPIE, 8, 0.3, 0);
    init_food(ITM_RABBITRAW, 3, 0.3, 0);
    init_food(ITM_RABBITCOOKED, 5, 0.6, 0);
    init_food(ITM_RABBITSTEW, 10, 0.6, 0);
    init_food(ITM_MUTTONRAW, 2, 0.3, 0);
    init_food(ITM_MUTTONCOOKED, 6, 0.8, 0);
    init_food(ITM_CHORUSFRUIT, 4, 0.3, 1);
    init_food(ITM_BEETROOT, 1, 0.6, 0);
    init_food(ITM_BEETROOT_SOUP, 6, 0.6, 0);
}

void add_item(item id, struct item_info* info) {
    ensure_collection(item_infos, id + 1);
    if (item_infos->data[id] != NULL) {
        xfree(item_infos->data[id]);
    }
    item_infos->data[id] = info;
    if (item_infos->size < id) item_infos->size = id;
    item_infos->count++;
}
