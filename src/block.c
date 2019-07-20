#include "basin/packet.h"
#include <basin/inventory.h>
#include <basin/globals.h>
#include "basin/network.h"
#include <basin/world.h>
#include <basin/game.h>
#include <basin/smelting.h>
#include <basin/tileentity.h>
#include <basin/smelting.h>
#include <basin/item.h>
#include <basin/block.h>
#include <avuna/json.h>
#include <avuna/string.h>
#include <avuna/list.h>
#include <avuna/hash.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <avuna/util.h>

struct list* block_infos;
struct hashmap* block_materials;

uint8_t getFaceFromPlayer(struct player* player) {
    uint32_t h = (uint32_t) floor(player->entity->yaw / 90. + .5) & 3;
    uint8_t f = 0;
    if (h == 0) f = SOUTH;
    else if (h == 1) f = WEST;
    else if (h == 2) f = NORTH;
    else if (h == 3) f = EAST;
    return f;
}

void onBlockUpdate_checkPlace(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    struct block_info* bi = getBlockInfo(blk);
    if (bi != NULL && bi->canBePlaced != NULL && !(*bi->canBePlaced)(world, blk, x, y, z)) {
        world_set_block(world, 0, x, y, z);
        dropBlockDrops(world, blk, NULL, x, y, z);
    }
}

uint8_t getBlockPower_block(struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (blk >> 4 == BLK_REDSTONEDUST >> 4) {
        return blk & 0x0f;
    } else if (blk >> 4 == BLK_NOTGATE_1 >> 4) {
        return 16; // TODO: transmit thru blocks
    } else if (blk >> 4 == BLK_DIODE_1 >> 4) {
        uint8_t ori = blk & 0x03;
        if ((ori == 0 && face == NORTH) || (ori == 1 && face == EAST) || (ori == 2 && face == SOUTH) || (ori == 3 && face == WEST)) return 16;
    } else if (blk >> 4 == BLK_BLOCKREDSTONE >> 4 || blk == (BLK_PRESSUREPLATEWOOD | 0x01) || blk == (BLK_PRESSUREPLATESTONE | 0x01)) {
        return 16;
    } else if (blk >> 4 == BLK_WEIGHTEDPLATE_LIGHT >> 4 || blk >> 4 == BLK_WEIGHTEDPLATE_HEAVY >> 4 || blk >> 4 == BLK_DAYLIGHTDETECTOR >> 4) {
        return blk & 0x0f;
    } else if ((blk >> 4 == BLK_DETECTORRAIL >> 4 || blk >> 4 == BLK_TRIPWIRESOURCE >> 4 || blk >> 4 == BLK_LEVER >> 4 || blk >> 4 == BLK_BUTTON >> 4 || blk >> 4 == BLK_BUTTON_1 >> 4) && (blk & 0x08)) {
        return 16;
    } else if (blk >> 4 == BLK_COMPARATOR_1 >> 4) {
        //TODO:
    }
    return 0;
}

uint8_t getPropogatedPower_block(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z, uint8_t excludeFace) {
    uint8_t maxPower = 0;
    uint8_t cPower = 0;
    if (excludeFace != XP && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x + 1, y, z), x + 1, y, z, XN)) > maxPower) maxPower = cPower;
    if (excludeFace != XN && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x - 1, y, z), x - 1, y, z, XP)) > maxPower) maxPower = cPower;
    if (excludeFace != ZP && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y, z + 1), x, y, z + 1, ZN)) > maxPower) maxPower = cPower;
    if (excludeFace != ZN && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y, z - 1), x, y, z - 1, ZP)) > maxPower) maxPower = cPower;
    if (excludeFace != YP && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y + 1, z), x, y + 1, z, YN)) > maxPower) maxPower = cPower;
    if (excludeFace != YN && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y - 1, z), x, y - 1, z, YP)) > maxPower) maxPower = cPower;
    if (maxPower > 0) {
        maxPower--;
    }
    if (maxPower > 15) maxPower = 15;
    return maxPower;
}

uint8_t getSpecificPower_block(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z, uint8_t face) {
    uint8_t maxPower = 0;
    uint8_t cPower = 0;
    if (face == XP && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x + 1, y, z), x + 1, y, z, XN)) > maxPower) maxPower = cPower;
    if (face == XN && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x - 1, y, z), x - 1, y, z, XP)) > maxPower) maxPower = cPower;
    if (face == ZP && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y, z + 1), x, y, z + 1, ZN)) > maxPower) maxPower = cPower;
    if (face == ZN && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y, z - 1), x, y, z - 1, ZP)) > maxPower) maxPower = cPower;
    if (face == YP && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y + 1, z), x, y + 1, z, YN)) > maxPower) maxPower = cPower;
    if (face == YN && (cPower = getBlockPower_block(world, world_get_block_guess(world, ch, x, y - 1, z), x, y - 1, z, YP)) > maxPower) maxPower = cPower;
    if (maxPower > 0) {
        maxPower--;
    }
    if (maxPower > 15) maxPower = 15;
    return maxPower;
}

int canBePlaced_torch(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t ori = blk & 0x0f;
    int32_t ax = x;
    int32_t ay = y;
    int32_t az = z;
    uint8_t rori = YN;
    if (ori == 1) rori = EAST;
    else if (ori == 2) rori = WEST;
    else if (ori == 3) rori = NORTH;
    else if (ori == 4) rori = SOUTH;
    else if (ori == 5) rori = YP;
    offsetCoordByFace(&ax, &ay, &az, rori);
    block b = world_get_block(world, ax, ay, az);
    struct block_info* bi = getBlockInfo(b);
    if (rori == YN) {
        if (bi->lightOpacity == 255) return 1;
        if (b >> 4 == BLK_FENCE >> 4 || b >> 4 == BLK_NETHERFENCE >> 4 || b >> 4 == BLK_SPRUCEFENCE >> 4 || b >> 4 == BLK_BIRCHFENCE >> 4 || b >> 4 == BLK_JUNGLEFENCE >> 4 || b >> 4 == BLK_DARKOAKFENCE >> 4 || b >> 4 == BLK_ACACIAFENCE >> 4 || b >> 4 == BLK_GLASS >> 4 || b >> 4 == BLK_COBBLEWALL_NORMAL >> 4 || b >> 4 == BLK_STAINEDGLASS_WHITE >> 4) return 1;
    } else if (block_is_normal_cube(bi)) return 1;
    return 0;
}

block onBlockPlacedPlayer_lever(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    uint8_t f = getFaceFromPlayer(player);
    if (face == NORTH) return blk | 0x04;
    else if (face == SOUTH) return blk | 0x03;
    else if (face == EAST) return blk | 0x01;
    else if (face == WEST) return blk | 0x02;
    else if (face == YN) return blk | (f == EAST || f == WEST ? 0x00 : 0x07);
    else if (face == YP) return blk | (f == NORTH || f == SOUTH ? 0x05 : 0x06);
    offsetCoordByFace(&x, &y, &z, face);
    return block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z))) ? blk : 0;
}

//TODO: burnout on redstone torches

void onBlockUpdate_redstonetorch(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    onBlockUpdate_checkPlace(world, blk, x, y, z);
    uint8_t ori = blk & 0x0f;
    int32_t ax = x;
    int32_t ay = y;
    int32_t az = z;
    uint8_t rori = YP;
    if (ori == 1) rori = WEST;
    else if (ori == 2) rori = EAST;
    else if (ori == 3) rori = SOUTH;
    else if (ori == 4) rori = NORTH;
    else if (ori == 5) rori = YN;
    offsetCoordByFace(&ax, &ay, &az, rori);
    uint8_t arori = YN;
    if (rori == WEST) arori = EAST;
    else if (rori == EAST) arori = WEST;
    else if (rori == NORTH) arori = SOUTH;
    else if (rori == SOUTH) arori = NORTH;
    else if (rori == YN) arori = YP;
    uint8_t ppower = getPropogatedPower_block(world, world_get_chunk(world, ax >> 4, az >> 4), ax, ay, az, arori);
    if (blk >> 4 == BLK_NOTGATE >> 4 && ppower == 0) {
        world_set_block(world, BLK_NOTGATE_1 | (blk & 0x0f), x, y, z);
    } else if (blk >> 4 == BLK_NOTGATE_1 >> 4 && ppower > 1) {
        world_set_block(world, BLK_NOTGATE | (blk & 0x0f), x, y, z);
    }
}

void onBlockUpdate_redstonedust(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    struct chunk* ch = world_get_chunk(world, x >> 4, z >> 4);
    uint8_t maxPower = getPropogatedPower_block(world, ch, x, y, z, -1);
    if (maxPower != (blk & 0x0f)) {
        world_set_block_guess(world, ch, BLK_REDSTONEDUST | maxPower, x, y, z);
    }
    onBlockUpdate_checkPlace(world, blk, x, y, z);
}

void onBlockUpdate_repeater(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    onBlockUpdate_checkPlace(world, blk, x, y, z);
    if (world_is_block_tick_scheduled(world, x, y, z)) {
        return;
    }
    uint8_t ori = blk & 0x03;
    uint8_t excl = 0;
    if (ori == 0) {
        excl = SOUTH;
    } else if (ori == 1) {
        excl = WEST;
    } else if (ori == 2) {
        excl = NORTH;
    } else if (ori == 3) {
        excl = EAST;
    }
    uint8_t sPower = getSpecificPower_block(world, world_get_chunk(world, x >> 4, z >> 4), x, y, z, excl);
    uint8_t update = 0;
    if (blk >> 4 == BLK_DIODE_1 >> 4 && sPower <= 0) {
        update = 1;
    } else if (sPower > 0 && blk >> 4 == BLK_DIODE >> 4) {
        update = 1;
    }
    if (update) {
        float priority = -1.;
        {
            int32_t sx = x;
            int32_t sy = y;
            int32_t sz = z;
            offsetCoordByFace(&sx, &sy, &sz, excl);
            block b = world_get_block(world, sx, sy, sz);
            if (b >> 4 == BLK_DIODE_1 >> 4 || b >> 4 == BLK_DIODE >> 4) {
                priority = -3.;
            }
        }
        if (priority == -1. && blk >> 4 == BLK_DIODE_1 >> 4) {
            priority = -2.;
        }
        world_schedule_block_tick(world, x, y, z, 2 * (((blk & 0x0f) >> 2) + 1), priority);
    }
}

void onBlockUpdate_lamp(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    struct chunk* ch = world_get_chunk(world, x >> 4, z >> 4);
    uint8_t maxPower = getPropogatedPower_block(world, ch, x, y, z, -1);
    if (maxPower > 0 && blk >> 4 == BLK_REDSTONELIGHT >> 4) {
        world_set_block_guess(world, ch, BLK_REDSTONELIGHT_1, x, y, z);
    } else if (maxPower == 0 && blk >> 4 == BLK_REDSTONELIGHT_1 >> 4) {
        world_set_block_guess(world, ch, BLK_REDSTONELIGHT, x, y, z);
    }
}

void onBlockUpdate_tnt(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t maxPower = getPropogatedPower_block(world, world_get_chunk(world, x >> 4, z >> 4), x, y, z, -1);
    if (maxPower > 0) {
        world_set_block(world, 0, x, y, z);
        struct entity* e = entity_new(world, world->server->next_entity_id++, (double) x + .5, (double) y, (double) z + .5, ENT_PRIMEDTNT, 0., 0.);
        e->data.tnt.fuse = 80;
        e->objectData = blk >> 4;
        float ra = game_rand_float() * M_PI * 2.;
        e->motX = -sinf(ra) * .02;
        e->motY = .2;
        e->motZ = -cosf(ra) * .02;
        world_spawn_entity(world, e);
    }
}

int scheduledTick_repeater(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t ori = blk & 0x03;
    uint8_t excl = 0;
    uint8_t lock1 = NORTH;
    uint8_t mLock1 = 2;
    uint8_t mLock2 = 0;
    uint8_t lock2 = SOUTH;
    if (ori == 0) {
        excl = SOUTH;
        lock1 = EAST;
        lock2 = WEST;
        mLock1 = 3;
        mLock2 = 1;
    } else if (ori == 1) {
        excl = WEST;
    } else if (ori == 2) {
        excl = NORTH;
        lock1 = EAST;
        lock2 = WEST;
        mLock1 = 3;
        mLock2 = 1;
    } else if (ori == 3) {
        excl = EAST;
    }
    {
        int32_t sx = x;
        int32_t sy = y;
        int32_t sz = z;
        offsetCoordByFace(&sx, &sy, &sz, lock1);
        block b = world_get_block(world, sx, sy, sz);
        if (b >> 4 == BLK_DIODE_1 >> 4 && (b & 0x03) == mLock1) {
            return 0;
        }
    }
    {
        int32_t sx = x;
        int32_t sy = y;
        int32_t sz = z;
        offsetCoordByFace(&sx, &sy, &sz, lock2);
        block b = world_get_block(world, sx, sy, sz);
        if (b >> 4 == BLK_DIODE_1 >> 4 && (b & 0x03) == mLock2) {
            return 0;
        }
    }
    uint8_t sPower = getSpecificPower_block(world, world_get_chunk(world, x >> 4, z >> 4), x, y, z, excl);
    //if (sPower > 0 && blk >> 4 == BLK_DIODE >> 4) {
    //printf("attempt, power = %i\n", sPower);
    if (blk >> 4 == BLK_DIODE_1 >> 4 && sPower <= 0) {
        //} else if (sPower <= 0 && blk >> 4 == BLK_DIODE_1 >> 4) {
        //printf("close\n");
        world_set_block(world, BLK_DIODE | (blk & 0x0f), x, y, z);
    } else if (blk >> 4 == BLK_DIODE >> 4) {
        //printf("open\n");
        world_set_block(world, BLK_DIODE_1 | (blk & 0x0f), x, y, z);
        if (sPower <= 0) {
            world_schedule_block_tick(world, x, y, z, 2 * (((blk & 0x0f) >> 2) + 1), -2.);
            return -1;
        }
    }
    //}
    return 0;
}

void onBlockInteract_repeater(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    world_set_block(world, (blk >> 4 << 4) | (((((blk & 0x0f) >> 2) + 1) & 0x03) << 2) | (blk & 0x03), x, y, z);
}

void onBlockInteract_trapdoor(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    world_set_block(world, (blk >> 4 << 4) | (blk & 0b1011) | (blk & 0b0100 ? 0 : 0b0100), x, y, z);
}

void onBlockUpdate_trapdoor(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t maxPower = getPropogatedPower_block(world, world_get_chunk(world, x >> 4, z >> 4), x, y, z, -1);
    if (maxPower > 0 && !(blk & 0b0100)) {
        world_set_block(world, (blk >> 4 << 4) | (blk & 0b1011) | 0b0100, x, y, z);
    }		//TODO: closing?
    onBlockUpdate_checkPlace(world, blk, x, y, z);
}

block onBlockPlacedPlayer_trapdoor(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (face == YP || face == YN) {
        if (block_is_normal_cube(getBlockInfo(world_get_block(world, x + 1, y, z)))) face = XP;
        else if (block_is_normal_cube(getBlockInfo(world_get_block(world, x - 1, y, z)))) face = XN;
        else if (block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z + 1)))) face = ZP;
        else if (block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z - 1)))) face = ZN;
    }
    if (face == XN) blk |= 2;
    else if (face == XP) blk |= 3;
    else if (face == ZP) blk |= 1;
    return blk;
}

int canBePlaced_trapdoor(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t ori = blk & 0b0011;
    if (ori == 2 && block_is_normal_cube(getBlockInfo(world_get_block(world, x + 1, y, z)))) return 1;
    else if (ori == 3 && block_is_normal_cube(getBlockInfo(world_get_block(world, x - 1, y, z)))) return 1;
    else if (ori == 0 && block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z + 1)))) return 1;
    else if (ori == 1 && block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z - 1)))) return 1;
    return 0;
}

void onBlockInteract_lever(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    if (blk >> 4 == BLK_LEVER >> 4 && (blk & 0x08)) {
        world_set_block(world, BLK_LEVER | (blk & 0x07), x, y, z);
    } else if (blk >> 4 == BLK_LEVER >> 4 && !(blk & 0x08)) {
        world_set_block(world, BLK_LEVER | (blk & 0x07) | 0x08, x, y, z);
    }
}

void onBlockInteract_button(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    world_set_block(world, (blk ^ (blk & 0x0f)) | (blk & 0x07) | 0x08, x, y, z);
    world_schedule_block_tick(world, x, y, z, blk >> 4 == BLK_BUTTON >> 4 ? 20 : 30, 0.);
}

int scheduledTick_button(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    world_set_block(world, (blk ^ (blk & 0x0f)) | (blk & 0x07), x, y, z);
    return 0;
}

block onBlockPlacedPlayer_button(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    if (face == NORTH) return blk | 0x04;
    else if (face == SOUTH) return blk | 0x03;
    else if (face == EAST) return blk | 0x01;
    else if (face == WEST) return blk | 0x02;
    else if (face == YN) return blk | 0x00;
    else if (face == YP) return blk | 0x05;
    offsetCoordByFace(&x, &y, &z, face);
    return block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z))) ? blk : 0;
}

int canBePlaced_redstone(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    struct block_info* bi = getBlockInfo(b);
    return block_is_normal_cube(bi);
}

int canBePlaced_bed(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    struct block_info* bi = getBlockInfo(b);
    return block_is_normal_cube(bi);
}

void onBlockUpdate_bed(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    struct block_info* bi = getBlockInfo(b);
    if (!block_is_normal_cube(bi)) goto delete;
    int facing = blk & 0b0011;
    int head = blk & 0b1000;
    uint8_t rf = 0;
    if (facing == 0) rf = SOUTH;
    else if (facing == 1) rf = WEST;
    else if (facing == 2) rf = NORTH;
    else if (facing == 3) rf = EAST;
    if (head) {
        if (rf == SOUTH) rf = NORTH;
        else if (rf == NORTH) rf = SOUTH;
        else if (rf == EAST) rf = WEST;
        else if (rf == WEST) rf = EAST;
    }
    int32_t ox = x;
    int32_t oy = y;
    int32_t oz = z;
    offsetCoordByFace(&ox, &oy, &oz, rf);
    b = world_get_block(world, ox, oy, oz);
    if (b >> 4 == blk >> 4) return;
    delete:;
    world_set_block(world, 0, x, y, z);
    if (head) dropBlockDrops(world, blk, NULL, x, y, z);
}

int canBePlaced_door(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    struct block_info* bi = getBlockInfo(b);
    return (b >> 4 == blk >> 4 && !(b & 0b1000) && (blk & 0b1000)) || block_is_normal_cube(bi);
}

void onBlockUpdate_door(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    struct block_info* bi = getBlockInfo(b);
    int upper = blk & 0b1000;
    if (!upper && !block_is_normal_cube(bi)) goto delete;
    b = world_get_block(world, x, y + (upper ? -1 : 1), z);
    int upperD = upper ? (blk & 0x0f) : (b & 0x0f);
    if (b >> 4 == blk >> 4) {
        uint8_t upperPower = getPropogatedPower_block(world, world_get_chunk(world, x >> 4, z >> 4), x, y + (upper ? 0 : -1), z, -1);
        uint8_t lowerPower = getPropogatedPower_block(world, world_get_chunk(world, x >> 4, z >> 4), x, y + (upper ? -1 : 0), z, -1);
        uint8_t maxPower = upperPower > lowerPower ? upperPower : lowerPower;
        if ((upperD & 0x02) && maxPower <= 0) {
            world_set_block(world, (upper ? blk : b) ^ 0x02, x, y + (upper ? 0 : 1), z);
            world_set_block(world, (upper ? b : blk) ^ 0x04, x, y + (upper ? -1 : 0), z);
        } else if (!(upperD & 0x02) && maxPower > 0) {
            world_set_block(world, (upper ? blk : b) | 0x02, x, y + (upper ? 0 : 1), z);
            world_set_block(world, (upper ? b : blk) | 0x04, x, y + (upper ? -1 : 0), z);
        }
        return;
    }
    delete:;
    world_set_block(world, 0, x, y, z);
    if (!upper) dropBlockDrops(world, blk, NULL, x, y, z);
}

void onBlockInteract_woodendoor(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    uint8_t curMeta = blk & 0x0f;
    uint8_t isUpper = curMeta & 0b1000;
    uint8_t lowerMeta = isUpper ? world_get_block(world, x, y - 1, z) & 0x0f : curMeta;
    uint8_t open = (lowerMeta & 0b0100) >> 2;
    open = !open;
    lowerMeta = (lowerMeta & 0b1011) | (open << 2);
    world_set_block(world, (blk & 0xfff0) | lowerMeta, x, y + (isUpper ? -1 : 0), z);
}

void onBlockInteract_workbench(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    struct inventory* wb = inventory_new(psub(player->pool), INVTYPE_WORKBENCH, 1, 10, "{\"text\": \"Crafting Table\"}");
    player_openInventory(player, wb);
}

block onBlockPlaced_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replaced) {
    int16_t meta = blk & 0x0f;
    if (meta < 2 || meta > 5) meta = 2;
    struct tile_entity* tile = tile_new(mempool_new(), "minecraft:chest", x, y, z);
    tile->data.chest.lock = NULL;
    tile->data.chest.inv = inventory_new(tile->pool, INVTYPE_CHEST, 2, 27, "{\"text\": \"Chest\"}");
    tile->data.chest.inv->tile = tile;
    world_set_tile(world, x, y, z, tile);
    return (blk & ~0x0f) | meta;
}

block onBlockPlaced_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replaced) {
    if ((replaced >> 4) == (BLK_FURNACE_1 >> 4) || (replaced >> 4) == (BLK_FURNACE >> 4)) return blk;
    int16_t meta = blk & 0x0f;
    if (meta < 2 || meta > 5) meta = 2;
    struct tile_entity* tile = tile_new(mempool_new(), "minecraft:furnace", x, y, z);
    tile->data.furnace.lock = NULL;
    tile->data.furnace.inv = inventory_new(tile->pool, INVTYPE_FURNACE, 3, 3, "{\"text\": \"Furnace\"}");
    tile->data.furnace.inv->tile = tile;
    world_set_tile(world, x, y, z, tile);
    return (blk & ~0x0f) | meta;
}

int onBlockDestroyed_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replacedBy) {
    struct tile_entity* te = world_get_tile(world, x, y, z);
    if (te == NULL) return 0;
    for (size_t i = 0; i < te->data.chest.inv->slot_count; i++) {
        struct slot* sl = inventory_get(NULL, te->data.chest.inv, i);
        game_drop_block(world, sl, x, y, z);
    }
    world_set_tile(world, x, y, z, NULL);
    return 0;
}

void onBlockInteract_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    struct tile_entity* tile = world_get_tile(world, x, y, z);
    if (tile == NULL || !str_eq(tile->id, "minecraft:chest")) {
        onBlockPlaced_chest(world, blk, x, y, z, 0);
        tile = world_get_tile(world, x, y, z);
    }
    //TODO: impl locks, loot
    player_openInventory(player, tile->data.chest.inv);
    BEGIN_BROADCAST_DIST(player->entity, 128.);
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_BLOCKACTION);
    pkt->data.play_client.blockaction.location.x = x;
    pkt->data.play_client.blockaction.location.y = y;
    pkt->data.play_client.blockaction.location.z = z;
    pkt->data.play_client.blockaction.action_id = 1;
    pkt->data.play_client.blockaction.action_param = tile->data.chest.inv->watching_players->entry_count;
    pkt->data.play_client.blockaction.block_type = blk >> 4;
    queue_push(bc_player->outgoing_packets, pkt);
    END_BROADCAST(player->world->players)
}

int onBlockDestroyed_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replacedBy) {
    if ((replacedBy >> 4) == (BLK_FURNACE_1 >> 4) || (replacedBy >> 4) == (BLK_FURNACE >> 4)) return 0;
    struct tile_entity* tile = world_get_tile(world, x, y, z);
    if (tile == NULL) return 0;
    for (size_t i = 0; i < tile->data.furnace.inv->slot_count; i++) {
        struct slot* slot = inventory_get(NULL, tile->data.furnace.inv, i);
        game_drop_block(world, slot, x, y, z);
    }
    world_set_tile(world, x, y, z, NULL);
    return 0;
}

void onBlockInteract_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    struct tile_entity* tile = world_get_tile(world, x, y, z);
    if (tile == NULL || !str_eq(tile->id, "minecraft:furnace")) {
        onBlockPlaced_furnace(world, blk, x, y, z, 0);
        tile = world_get_tile(world, x, y, z);
    }
    //TODO: impl locks, loot
    player_openInventory(player, tile->data.furnace.inv);
    struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWPROPERTY);
    pkt->data.play_client.windowproperty.window_id = tile->data.furnace.inv->window;
    pkt->data.play_client.windowproperty.property = 1;
    pkt->data.play_client.windowproperty.value = tile->data.furnace.lastBurnMax;
    queue_push(player->outgoing_packets, pkt);
    pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWPROPERTY);
    pkt->data.play_client.windowproperty.window_id = tile->data.furnace.inv->window;
    pkt->data.play_client.windowproperty.property = 3;
    pkt->data.play_client.windowproperty.value = 200;
    queue_push(player->outgoing_packets, pkt);
}

void update_furnace(struct world* world, struct tile_entity* tile) {
    struct slot* input_slot = inventory_get(NULL, tile->data.furnace.inv, 0);
    struct slot* fuel_slot = inventory_get(NULL, tile->data.furnace.inv, 1);
    struct slot* actual_output_slot = inventory_get(NULL, tile->data.furnace.inv, 2);
    struct slot* output_slot = smelting_output(input_slot);
    int16_t burn_time = smelting_burnTime(fuel_slot);
    int burning = 0;
    if (output_slot != NULL && ((slot_stackable(output_slot, actual_output_slot) && (actual_output_slot->count + output_slot->count) <= slot_max_size(output_slot)) || actual_output_slot == NULL) && (tile->data.furnace.burnTime > 0 || burn_time > 0)) {
        burning = 1;
        if (!tile->tick) {
            tile->tick = &tetick_furnace;
            world_tile_set_tickable(world, tile);
            world_set_block(world, BLK_FURNACE_1 | (world_get_block(world, tile->x, tile->y, tile->z) & 0x0f), tile->x, tile->y, tile->z);
        }
        if (tile->data.furnace.burnTime <= 0 && burn_time > 0 && fuel_slot != NULL) {
            tile->data.furnace.burnTime += burn_time + 1;
            if (--fuel_slot->count == 0) fuel_slot = NULL;
            inventory_set_slot(NULL, tile->data.furnace.inv, 1, fuel_slot, 1);
            tile->data.furnace.lastBurnMax = burn_time;
            BEGIN_BROADCAST(tile->data.furnace.inv->watching_players){
                struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWPROPERTY);
                pkt->data.play_client.windowproperty.window_id = tile->data.furnace.inv->window;
                pkt->data.play_client.windowproperty.property = 1;
                pkt->data.play_client.windowproperty.value = tile->data.furnace.lastBurnMax;
                queue_push(bc_player->outgoing_packets, pkt);
                END_BROADCAST(tile->data.furnace.inv->watching_players)
            }
        }
        if (tile->data.furnace.cookTime <= 0) {
            tile->data.furnace.cookTime++;
        } else if (tile->data.furnace.cookTime < 200) {
            tile->data.furnace.cookTime++;
        } else if (tile->data.furnace.cookTime == 200) {
            tile->data.furnace.cookTime = 0;
            if (actual_output_slot == NULL) inventory_set_slot(NULL, tile->data.furnace.inv, 2, output_slot, 1);
            else {
                actual_output_slot->count++;
                inventory_set_slot(NULL, tile->data.furnace.inv, 2, actual_output_slot, 1);
            }
            if (--input_slot->count == 0) input_slot = NULL;
            inventory_set_slot(NULL, tile->data.furnace.inv, 0, input_slot, 1);
            tile->data.furnace.burnTime++; // freebie for accounting
        }
    } else tile->data.furnace.cookTime = 0;
    if (tile->data.furnace.burnTime > 0) {
        tile->data.furnace.burnTime--;
    } else {
        if (tile->tick) {
            BEGIN_BROADCAST(tile->data.furnace.inv->watching_players){
                struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWPROPERTY);
                pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
                pkt->data.play_client.windowproperty.window_id = tile->data.furnace.inv->window;
                pkt->data.play_client.windowproperty.property = 0;
                pkt->data.play_client.windowproperty.value = 0;
                queue_push(bc_player->outgoing_packets, pkt);
                pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWPROPERTY);
                pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
                pkt->data.play_client.windowproperty.window_id = tile->data.furnace.inv->window;
                pkt->data.play_client.windowproperty.property = 2;
                pkt->data.play_client.windowproperty.value = 0;
                queue_push(bc_player->outgoing_packets, pkt);
                END_BROADCAST(tile->data.furnace.inv->watching_players)
            }
            world_tile_unset_tickable(world, tile);
            tile->tick = NULL;
            world_set_block(world, BLK_FURNACE | (world_get_block(world, tile->x, tile->y, tile->z) & 0x0f), tile->x, tile->y, tile->z);
        }
        tile->data.furnace.cookTime = 0;
    }
    if (burning && world->server->tick_counter % 5 == 0) {
        BEGIN_BROADCAST(tile->data.furnace.inv->watching_players) {
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWPROPERTY);
            pkt->data.play_client.windowproperty.window_id = tile->data.furnace.inv->window;
            pkt->data.play_client.windowproperty.property = 0;
            pkt->data.play_client.windowproperty.value = tile->data.furnace.burnTime;
            queue_push(bc_player->outgoing_packets, pkt);
            pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWPROPERTY);
            pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
            pkt->data.play_client.windowproperty.window_id = tile->data.furnace.inv->window;
            pkt->data.play_client.windowproperty.property = 2;
            pkt->data.play_client.windowproperty.value = tile->data.furnace.cookTime;
            queue_push(bc_player->outgoing_packets, pkt);
            END_BROADCAST(tile->data.furnace.inv->watching_players)
        }
    }
}

void dropItems_gravel(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
    if (fortune > 3) fortune = 3;
    if (fortune == 3 || (rand() % (10 - fortune * 3)) == 0) {
        struct slot drop;
        drop.item = ITM_FLINT;
        drop.damage = 0;
        drop.count = 1;
        drop.nbt = NULL;
        game_drop_block(world, &drop, x, y, z);
    } else {
        struct slot drop;
        drop.item = BLK_GRAVEL >> 4;
        drop.damage = BLK_GRAVEL & 0x0f;
        drop.count = 1;
        drop.nbt = NULL;
        game_drop_block(world, &drop, x, y, z);
    }
}

void dropItems_leaves(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
    int chance = 20;
    if (fortune > 0) chance -= 2 << fortune;
    if (chance < 10) chance = 10;
    if (rand() % chance == 0) {
        struct slot drop;
        uint8_t m = blk & 0x0f;
        if (blk >> 4 == BLK_LEAVES_OAK >> 4) {
            if ((m & 0x03) == 0) {
                drop.item = BLK_SAPLING_OAK >> 4;
                drop.damage = 0;
            } else if ((m & 0x03) == 1) {
                drop.item = BLK_SAPLING_SPRUCE >> 4;
                drop.damage = 1;
            } else if ((m & 0x03) == 2) {
                drop.item = BLK_SAPLING_BIRCH >> 4;
                drop.damage = 2;
            } else if ((m & 0x03) == 3) {
                drop.item = BLK_SAPLING_JUNGLE >> 4;
                drop.damage = 3;
            }
        } else if (blk >> 4 == BLK_LEAVES_ACACIA >> 4) {
            if ((m & 0x03) == 0) {
                drop.item = BLK_SAPLING_ACACIA >> 4;
                drop.damage = 4;
            } else if ((m & 0x03) == 1) {
                drop.item = BLK_SAPLING_BIG_OAK >> 4;
                drop.damage = 5;
            } else return;
        } else return;
        drop.count = 1;
        drop.nbt = NULL;
        game_drop_block(world, &drop, x, y, z);
    }
    if (blk == BLK_LEAVES_OAK || blk == BLK_LEAVES_BIG_OAK) {
        chance = 200;
        if (fortune > 0) {
            chance -= 10 << fortune;
            if (chance < 40) chance = 40;
        }
        if (rand() % chance == 0) {
            struct slot drop;
            drop.item = ITM_APPLE;
            drop.damage = 0;
            drop.count = 1;
            drop.nbt = NULL;
            game_drop_block(world, &drop, x, y, z);
        }
    }
}

void dropItems_tallgrass(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
    if (rand() % 8 == 0) {
        struct slot drop;
        drop.item = ITM_SEEDS;
        drop.damage = 0;
        drop.count = 1 + (rand() % (fortune * 2 + 1));
        drop.nbt = NULL;
        game_drop_block(world, &drop, x, y, z);
    }
}

int falling_canFallThrough(block b) {
    struct block_info* block_info = getBlockInfo(b);
    return block_info == NULL || str_eq(block_info->material->name, "fire") || str_eq(block_info->material->name, "air") || str_eq(block_info->material->name, "water") || str_eq(block_info->material->name, "lava");
}

void onBlockUpdate_falling(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    if (y > 0 && falling_canFallThrough(world_get_block(world, x, y - 1, z))) {
        world_schedule_block_tick(world, x, y, z, 4, 0.);
    }
}

int scheduledTick_falling(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    if (y > 0 && falling_canFallThrough(world_get_block(world, x, y - 1, z))) {
        world_set_block(world, 0, x, y, z);
        struct entity* e = entity_new(world, world->server->next_entity_id++, (double) x + .5, (double) y, (double) z + .5, ENT_FALLINGBLOCK, 0., 0.);
        e->data.fallingblock.b = blk;
        e->objectData = blk >> 4;
        world_spawn_entity(world, e);
    }
    return 0;
}

void sponge_floodfill(struct world* world, int32_t x, int32_t y, int32_t z, int32_t* pos, uint32_t* removed) {
    if (*removed > 64) return;
    block b = world_get_block(world, x, y, z);
    if (b >> 4 == BLK_WATER >> 4 || b >> 4 == BLK_WATER_1 >> 4) {
        pos[*removed * 3] = x;
        pos[*removed * 3 + 1] = y;
        pos[*removed * 3 + 2] = z;
        (*removed)++;
        if (*removed > 64) return;
    }
}

void onBlockUpdate_sponge(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    if ((blk & 0x0f) == 0) {
        uint32_t removed = 0;
        int32_t pos[65][3];
        uint32_t lRemoved = 0;
        while (removed < 65) {
            if (removed == 0) {
                sponge_floodfill(world, x - 1, y, z, pos, &removed);
                sponge_floodfill(world, x + 1, y, z, pos, &removed);
                sponge_floodfill(world, x, y + 1, z, pos, &removed);
                sponge_floodfill(world, x, y - 1, z, pos, &removed);
                sponge_floodfill(world, x, y, z + 1, pos, &removed);
                sponge_floodfill(world, x, y, z - 1, pos, &removed);
                if (removed == 0) break;
                else world_set_block(world, blk | 1, x, y, z);
            }
            uint32_t rRemoved = removed;
            for (uint32_t i = lRemoved; i < removed; i++) {
                world_set_block(world, 0, pos[i][0], pos[i][1], pos[i][2]);
                sponge_floodfill(world, pos[i][0] - 1, pos[i][1], pos[i][2], pos, &rRemoved);
                sponge_floodfill(world, pos[i][0] + 1, pos[i][1], pos[i][2], pos, &rRemoved);
                sponge_floodfill(world, pos[i][0], pos[i][1] + 1, pos[i][2], pos, &rRemoved);
                sponge_floodfill(world, pos[i][0], pos[i][1] - 1, pos[i][2], pos, &rRemoved);
                sponge_floodfill(world, pos[i][0], pos[i][1], pos[i][2] + 1, pos, &rRemoved);
                sponge_floodfill(world, pos[i][0], pos[i][1], pos[i][2] - 1, pos, &rRemoved);
            }
            if (lRemoved == removed) break;
            lRemoved = removed;
            removed = rRemoved;
        }
    }
}

int canBePlaced_ladder(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t m = blk & 0x0f;
    block b = 0;
    if (m == 3) b = world_get_block(world, x, y, z - 1);
    else if (m == 4) b = world_get_block(world, x + 1, y, z);
    else if (m == 5) b = world_get_block(world, x - 1, y, z);
    else b = world_get_block(world, x, y, z + 1);
    return block_is_normal_cube(getBlockInfo(b));
}

block onBlockPlacedPlayer_ladder(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    int zpg = 0;
    int zng = 0;
    int xng = 0;
    int xpg = 0;
    if ((zpg = block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z - 1)))) && face == ZP) return (blk & ~0xf) | 3;
    else if ((zng = block_is_normal_cube(getBlockInfo(world_get_block(world, x, y, z + 1)))) && face == ZN) return (blk & ~0xf) | 2;
    else if ((xpg = block_is_normal_cube(getBlockInfo(world_get_block(world, x + 1, y, z)))) && face == XN) return (blk & ~0xf) | 4;
    else if ((xng = block_is_normal_cube(getBlockInfo(world_get_block(world, x - 1, y, z)))) && face == XP) return (blk & ~0xf) | 5;
    else if (zpg) return (blk & ~0xf) | 2;
    else if (zng) return (blk & ~0xf) | 3;
    else if (xng) return (blk & ~0xf) | 4;
    else if (xpg) return (blk & ~0xf) | 5;
    else return 0;
}

block onBlockPlacedPlayer_vine(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    block out = blk & ~0x0f;
    block b = world_get_block(world, x, y, z + 1);
    struct block_info* bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) out |= 0x1;
    b = world_get_block(world, x - 1, y, z);
    bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) out |= 0x2;
    b = world_get_block(world, x, y, z - 1);
    bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) out |= 0x4;
    b = world_get_block(world, x + 1, y, z);
    bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) out |= 0x8;
    b = world_get_block(world, x, y + 1, z);
    bi = getBlockInfo(b);
//printf("block placed vine = ");
    if ((b >> 4) == (BLK_VINE >> 4)) {
        //printf("%i\n", b);
        return b;
    } else if (!(bi != NULL && bi->fullCube && bi->material->blocksMovement) && (out & 0x0f) == 0) {
        //printf("0 -- %i, %i, %i, %i, %i\n", b, bi != NULL, bi->fullCube, bi->material->blocksMovement, out);
        return 0;
    }
//printf("2/%i\n", out);
    return out;
}

void onBlockUpdate_vine(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
//printf("bu vine <%i, %i, %i> ", x, y, z);
    block b = onBlockPlacedPlayer_vine(NULL, world, blk, x, y, z, -1);
//printf("%i != %i\n", b, blk);
    if (b != blk) {
        if (b >> 4 == blk >> 4 && b >> 4 == BLK_VINE >> 4) world_set_block_noupdate(world, b, x, y, z);
        else world_set_block(world, b, x, y, z);
    }
}

int canBePlaced_vine(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
//printf("cbp vine at\n");
    block b = world_get_block(world, x, y, z + 1);
    struct block_info* bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) return 1;
    b = world_get_block(world, x - 1, y, z);
    bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) return 1;
    b = world_get_block(world, x, y, z - 1);
    bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) return 1;
    b = world_get_block(world, x + 1, y, z);
    bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) return 1;
    b = world_get_block(world, x, y + 1, z);
    if ((b >> 4) == (BLK_VINE >> 4)) return 1;
    bi = getBlockInfo(b);
    if (bi != NULL && bi->fullCube && bi->material->blocksMovement) return 1;
//printf("cbp vine at --- cannot be placed\n");
    return 0;
}

void randomTick_vine(struct world* world, struct chunk* ch, block blk, int32_t x, int32_t y, int32_t z) {
    if (rand() % 4 != 0) return;
    block below = world_get_block_guess(world, ch, x, y - 1, z);
    if (below == 0) {
        //printf("rtv grow\n");
        world_set_block_guess(world, ch, onBlockPlacedPlayer_vine(NULL, world, BLK_VINE, x, y, z, -1), x, y - 1, z);
        //printf("rtv growdun\n");
    }
}

int canBePlaced_requiredirt(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    return b == BLK_DIRT || b == BLK_GRASS;
}

int canBePlaced_sapling(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    return b == BLK_DIRT || b == BLK_GRASS || b == BLK_FARMLAND;
}

int canBePlaced_doubleplant(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    block b2 = world_get_block(world, x, y + 1, z);
    if ((b == blk && b2 != 0) || (b2 == blk && b != BLK_GRASS && b != BLK_DIRT)) {
        return 0;
    }
    return 1;
}

int canBePlaced_requiresand(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    return b == BLK_SAND;
}

int canBePlaced_cactus(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    if (b != BLK_SAND && b != BLK_CACTUS) return 0;
    b = world_get_block(world, x, y, z + 1);
    if (b != 0) return 0;
    b = world_get_block(world, x, y, z - 1);
    if (b != 0) return 0;
    b = world_get_block(world, x + 1, y, z);
    if (b != 0) return 0;
    b = world_get_block(world, x - 1, y, z);
    if (b != 0) return 0;
    return 1;
}

int canBePlaced_reeds(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    if (b == BLK_REEDS) return 1;
    if (b != BLK_DIRT && b != BLK_GRASS && b != BLK_SAND) return 0;
    b = world_get_block(world, x, y - 1, z + 1);
    if ((b >> 4) == (BLK_WATER >> 4) || (b >> 4) == (BLK_WATER_1 >> 4)) return 1;
    b = world_get_block(world, x, y - 1, z - 1);
    if ((b >> 4) == (BLK_WATER >> 4) || (b >> 4) == (BLK_WATER_1 >> 4)) return 1;
    b = world_get_block(world, x + 1, y - 1, z);
    if ((b >> 4) == (BLK_WATER >> 4) || (b >> 4) == (BLK_WATER_1 >> 4)) return 1;
    b = world_get_block(world, x - 1, y - 1, z);
    if ((b >> 4) == (BLK_WATER >> 4) || (b >> 4) == (BLK_WATER_1 >> 4)) return 1;
    return 0;
}

int canBePlaced_requirefarmland(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    return b >> 4 == BLK_FARMLAND >> 4;
}

int canBePlaced_requiresoulsand(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    block b = world_get_block(world, x, y - 1, z);
    return b == BLK_HELLSAND;
}

void dropItems_hugemushroom(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
    int ct = rand() % 10 - 7;
    if (ct > 0) {
        struct slot drop;
        drop.item = ((blk >> 4) == (BLK_MUSHROOM_2 >> 4) ? 39 : 40);
        drop.damage = 0;
        drop.count = ct;
        drop.nbt = NULL;
        game_drop_block(world, &drop, x, y, z);
    }
}

void dropItems_crops(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
    struct slot drop;
    uint8_t age = blk & 0x0f;
    uint8_t maxAge = 0;
    item seed = 0;
    if ((blk >> 4) == (BLK_CROPS >> 4)) {
        maxAge = 7;
        if (age >= maxAge) {
            drop.item = ITM_WHEAT;
            drop.count = 1;
            drop.damage = 0;
            drop.nbt = NULL;
            game_drop_block(world, &drop, x, y, z);
        }
        seed = ITM_SEEDS;
    } else if ((blk >> 4) == (BLK_PUMPKINSTEM >> 4)) {
        drop.item = ITM_SEEDS_PUMPKIN;
        drop.count = 1;
        drop.damage = 0;
        drop.nbt = NULL;
        for (int i = 0; i < 3; i++) {
            if (rand() % 15 <= age) {
                game_drop_block(world, &drop, x, y, z);
            }
        }
        seed = ITM_SEEDS_PUMPKIN;
        maxAge = 7;
    } else if ((blk >> 4) == (BLK_PUMPKINSTEM_1 >> 4)) {
        drop.item = ITM_SEEDS_MELON;
        drop.count = 1;
        drop.damage = 0;
        drop.nbt = NULL;
        for (int i = 0; i < 3; i++) {
            if (rand() % 15 <= age) {
                game_drop_block(world, &drop, x, y, z);
            }
        }
        seed = ITM_SEEDS_PUMPKIN;
        maxAge = 7;
    } else if ((blk >> 4) == (BLK_CARROTS >> 4)) {
        maxAge = 7;
        if (age >= maxAge) {
            drop.item = ITM_CARROTS;
            drop.count = 1;
            drop.damage = 0;
            drop.nbt = NULL;
            game_drop_block(world, &drop, x, y, z);
        }
        seed = ITM_CARROTS;
    } else if ((blk >> 4) == (BLK_POTATOES >> 4)) {
        maxAge = 7;
        seed = ITM_POTATO;
        if (age >= maxAge) {
            drop.item = ITM_POTATO;
            drop.count = 1;
            drop.damage = 0;
            drop.nbt = NULL;
            game_drop_block(world, &drop, x, y, z);
            if (rand() % 50 == 0) {
                drop.item = ITM_POTATOPOISONOUS;
                drop.count = 1;
                drop.damage = 0;
                drop.nbt = NULL;
                game_drop_block(world, &drop, x, y, z);
            }
        }
    } else if ((blk >> 4) == (BLK_NETHERSTALK >> 4)) {
        int rct = 1;
        drop.item = ITM_NETHERSTALKSEEDS;
        drop.count = 1;
        drop.damage = 0;
        drop.nbt = NULL;
        if (age >= 3) {
            rct += 2 + rand() % 3;
            if (fortune > 0) rct += rand() % (fortune + 1);
        }
        for (int i = 0; i < rct; i++)
            game_drop_block(world, &drop, x, y, z);
        return;
    } else if ((blk >> 4) == (BLK_BEETROOTS >> 4)) {
        maxAge = 3;
        if (age >= maxAge) {
            drop.item = ITM_BEETROOT;
            drop.count = 1;
            drop.damage = 0;
            drop.nbt = NULL;
            game_drop_block(world, &drop, x, y, z);
        }
        seed = ITM_BEETROOT_SEEDS;
    } else if ((blk >> 4) == (BLK_COCOA >> 4)) {
        int rc = 1;
        if (age >= 2) rc = 3;
        drop.item = ITM_DYEPOWDER_BLACK;
        drop.count = 1;
        drop.damage = 3;
        drop.nbt = NULL;
        for (int i = 0; i < rc; i++)
            game_drop_block(world, &drop, x, y, z);
        return;
    }
    if (seed > 0) {
        drop.item = seed;
        drop.count = 1;
        drop.damage = 0;
        drop.nbt = NULL;
        if (age >= maxAge) {
            int rct = 3 + fortune;
            for (int i = 0; i < rct; i++) {
                if (rand() % (2 * maxAge) <= age) game_drop_block(world, &drop, x, y, z);
            }
        }
        game_drop_block(world, &drop, x, y, z);
    }
}

float crops_getGrowthChance(struct world* world, block b, int32_t x, int32_t y, int32_t z) {
    float chance = 1.;
    for (int32_t sx = x - 1; sx <= x + 1; sx++) {
        for (int32_t sz = x - 1; sz <= x + 1; sz++) {
            block b2 = world_get_block(world, sx, y - 1, sz);
            float subchance = 0.;
            if ((b2 >> 4) == (BLK_FARMLAND >> 4)) {
                subchance = 1.;
                if ((b2 & 0x0f) > 0) {
                    subchance = 3;
                }
            }
            if (sx != 0 || sz != 0) {
                subchance /= 4.;
            }
            chance += subchance;
        }
    }
    int xSame = world_get_block(world, x - 1, y, z) >> 4 == b >> 4 || world_get_block(world, x + 1, y, z) >> 4 == b >> 4;
    int zSame = world_get_block(world, x, y, z - 1) >> 4 == b >> 4 || world_get_block(world, x, y, z + 1) >> 4 == b >> 4;
    if (xSame && zSame) {
        chance /= 2.;
    } else {
        int dSame = world_get_block(world, x - 1, y, z - 1) >> 4 == b >> 4 || world_get_block(world, x + 1, y, z - 1) >> 4 == b >> 4 || world_get_block(world, x - 1, y, z + 1) >> 4 == b >> 4 || world_get_block(world, x + 1, y, z + 1) >> 4 == b >> 4;
        if (dSame) {
            chance /= 2.;
        }
    }
    return chance;
}

void randomTick_crops(struct world* world, struct chunk* ch, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t lwu = world_get_light_guess(world, ch, x, y + 1, z);
    if (lwu > 9) {
        uint8_t age = blk & 0x0f;
        if (blk >> 4 == BLK_CROPS >> 4) {
            if (age < 7) {
                float gChance = crops_getGrowthChance(world, blk, x, y, z);
                if (rand() % ((int) (25. / gChance) + 1) == 0) {
                    block nb = (blk & 0xfff0) | ((blk & 0x0f) + 1);
                    world_set_block_guess(world, ch, nb, x, y, z);
                }
            }
        }
    }
}

void randomTick_farmland(struct world* world, struct chunk* ch, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t moisture = blk & 0x0f;
    uint8_t hasWaterOrRain = 0; // TODO: true if raining
    if (!hasWaterOrRain) for (int32_t sx = x - 4; sx <= x + 4; sx++) {
        for (int32_t sz = z - 4; sz <= z + 4; sz++) {
            if (str_eq(getBlockInfo(world_get_block_guess(world, ch, sx, y, sz))->material->name, "water")) {
                hasWaterOrRain = 1;
                goto postWater;
            }
        }
    }
    postWater: ;
    if (hasWaterOrRain && moisture < 7) {
        block nb = (blk & 0xfff0) | 7;
        world_set_block_guess(world, ch, nb, x, y, z);
    } else if (moisture > 0 && !hasWaterOrRain) {
        block nb = (blk & 0xfff0) | (moisture - 1);
        world_set_block_guess(world, ch, nb, x, y, z);
    }
}

void randomTick_grass(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z) {
    struct block_info* ni = getBlockInfo(world_get_block_guess(world, chunk, x, y + 1, z));
    uint8_t lwu = world_get_light(world, x, y, z, 1);
    if (lwu < 4 && ni != NULL && ni->lightOpacity > 2) {
        world_set_block_guess(world, chunk, BLK_DIRT, x, y, z);
    } else if (lwu >= 9) {
        for (int i = 0; i < 4; i++) {
            int32_t gx = x + rand() % 3 - 1;
            int32_t gy = y + rand() % 5 - 3;
            int32_t gz = z + rand() % 3 - 1;
            block up = world_get_block_guess(world, chunk, gx, gy + 1, gz);
            block blk = world_get_block_guess(world, chunk, gx, gy, gz);
            ni = getBlockInfo(up);
            if (blk == BLK_DIRT && (ni == NULL || ni->lightOpacity <= 2) && world_get_light(world, gx, gy + 1, gz, 1) >= 4) {
                world_set_block_guess(world, chunk, BLK_GRASS, gx, gy, gz);
            }
        }
    }
}

int tree_canGrowInto(block b) {
    struct block_info* bi = getBlockInfo(b);
    if (bi == NULL) return 1;
    char* mn = bi->material->name;
    return b == BLK_GRASS || b == BLK_DIRT || (b >> 4) == (BLK_LOG_OAK >> 4) || (b >> 4) == (BLK_LOG_ACACIA >> 4) || (b >> 4) == (BLK_SAPLING_OAK >> 4) || b == BLK_VINE || str_eq(mn, "air") || str_eq(mn, "leaves");
}

void tree_addHangingVine(struct world* world, struct chunk* chunk, block b, int32_t x, int32_t y, int32_t z) {
    world_set_block_guess(world, chunk, b, x, y, z);
    int32_t iy = y;
    for (y--; y > iy - 4 && world_get_block_guess(world, chunk, x, y, z) == 0; y--) {
        world_set_block_guess(world, chunk, b, x, y, z);
    }
}

int tree_checkAndPlaceLeaf(struct world* world, struct chunk* chunk, block b, int32_t x, int32_t y, int32_t z) {
    struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, x, y, z));
    if (bi == NULL || str_eq(bi->material->name, "air") || str_eq(bi->material->name, "leaves")) {
        world_set_block_guess(world, chunk, b, x, y, z);
        return 1;
    }
    return 0;
}

void randomTick_sapling(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z) {
    uint8_t lwu = world_get_light(world, x, y + 1, z, 1);
    if (lwu >= 9 && rand() % 7 == 0) {
        if ((blk & 0x8) == 0x8) {
            uint8_t type = blk & 0x7;
            block log = BLK_LOG_OAK;
            block leaf = BLK_LEAVES_OAK_2;
            if (type == 1) {
                log = BLK_LOG_SPRUCE;
                leaf = BLK_LEAVES_SPRUCE_2;
            } else if (type == 2) {
                log = BLK_LOG_BIRCH;
                leaf = BLK_LEAVES_BIRCH_2;
            } else if (type == 3) {
                log = BLK_LOG_JUNGLE;
                leaf = BLK_LEAVES_JUNGLE_2;
            } else if (type == 4) {
                log = BLK_LOG_ACACIA_1;
                leaf = BLK_LEAVES_ACACIA_2;
            } else if (type == 5) {
                log = BLK_LOG_BIG_OAK_1;
                leaf = BLK_LEAVES_BIG_OAK_2;
            }
            uint8_t biome = world_get_biome(world, x, z);
            uint8_t vines = type == 0 && (biome == BIOME_SWAMPLAND || biome == BIOME_SWAMPLAND_M);
            uint8_t cocoa = type == 3;
            int big = type == 0 && rand() % 10 == 0;
            if (!big) {
                uint8_t height = 0;
                if (type == 0) height = rand() % 3 + 4;
                else if (type == 1) height = 6 + rand() % 4;
                else if (type == 2) height = rand() % 3 + 5;
                else if (type == 3) height = 3 + rand() % 10;
                else if (type == 4) height = 5 + rand() % 6;
                if (y < 1 || y + height + 1 > 256) return;
                for (int ty = y; ty <= y + height + 1; ty++) {
                    if (ty < 0 || ty >= 256) return;
                    int width = 1;
                    if (ty == y) width = 0;
                    if (ty >= y + 1 + height - 2) width = 2;
                    for (int tx = x - width; tx <= x + width; tx++) {
                        for (int tz = z - width; tz <= z + width; tz++) {
                            if (!tree_canGrowInto(world_get_block_guess(world, chunk, tx, ty, tz))) return;
                        }
                    }
                }
                block down = world_get_block_guess(world, chunk, x, y - 1, z);
                if ((down != BLK_GRASS && down != BLK_DIRT && down != BLK_FARMLAND) || y >= 256 - height - 1) return;
                if (down == BLK_FARMLAND && type == 4) return;
                world_set_block_guess(world, chunk, BLK_DIRT, x, y - 1, z);
                if (type == 4) {
                    int k2 = height - rand() % 4 - 1;
                    int l2 = 3 - rand() % 3;
                    int i3 = x;
                    int j1 = z;
                    int k1 = 0;
                    int rx = rand() % 4;
                    int ox = 0;
                    int oz = 0;
                    if (rx == 0) ox++;
                    else if (rx == 1) ox--;
                    else if (rx == 2) oz++;
                    else if (rx == 3) oz--;
                    for (int l1 = 0; l1 < height; l1++) {
                        int i2 = y + l1;
                        if (l1 >= k2 && l2 > 0) {
                            i3 += ox;
                            j1 += oz;
                            l2--;
                        }
                        struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, i3, i2, j1));
                        if (bi != NULL && (str_eq(bi->material->name, "air") || str_eq(bi->material->name, "leaves") || str_eq(bi->material->name, "vine") || str_eq(bi->material->name, "plants"))) {
                            world_set_block_guess(world, chunk, log, i3, i2, j1);
                            k1 = i2;
                        }
                    }
                    for (int j3 = -3; j3 <= 3; j3++) {
                        for (int i4 = -3; i4 <= +3; i4++) {
                            if (abs(j3) != 3 || abs(i4) != 3) {
                                tree_checkAndPlaceLeaf(world, chunk, leaf, i3 + j3, k1, j1 + i4);
                            }
                            if (abs(j3) <= 1 || abs(i4) <= 1) {
                                tree_checkAndPlaceLeaf(world, chunk, leaf, i3 + j3, k1 + 1, j1 + i4);
                            }
                        }
                    }
                    tree_checkAndPlaceLeaf(world, chunk, leaf, i3 - 2, k1 + 1, j1);
                    tree_checkAndPlaceLeaf(world, chunk, leaf, i3 + 2, k1 + 1, j1);
                    tree_checkAndPlaceLeaf(world, chunk, leaf, i3, k1 + 1, j1 - 2);
                    tree_checkAndPlaceLeaf(world, chunk, leaf, i3, k1 + 1, j1 + 1);
                    i3 = x;
                    j1 = z;
                    int nrx = rand() % 4;
                    if (rx == nrx) return;
                    rx = nrx;
                    ox = 0;
                    oz = 0;
                    if (rx == 0) ox++;
                    else if (rx == 1) ox--;
                    else if (rx == 2) oz++;
                    else if (rx == 3) oz--;
                    int l3 = k2 - rand() % 2 - 1;
                    int k4 = 1 + rand() % 3;
                    k1 = 0;
                    for (int l4 = l3; l4 < height && k4 > 0; k4--) {
                        if (l4 >= 1) {
                            int j2 = y + l4;
                            i3 += ox;
                            j1 += oz;
                            if (tree_checkAndPlaceLeaf(world, chunk, log, i3, j2, j1)) {
                                k1 = j2;
                            }
                        }
                        l4++;
                    }
                    if (k1 > 0) {
                        for (int i5 = -2; i5 <= 2; i5++) {
                            for (int k5 = -2; k5 <= 2; k5++) {
                                if (abs(i5) != 2 || abs(k5) != 2) tree_checkAndPlaceLeaf(world, chunk, leaf, i3 + i5, k1, j1 + k5);
                                if (abs(i5) <= 1 && abs(k5) <= 1) tree_checkAndPlaceLeaf(world, chunk, leaf, i3 + i5, k1 + 1, j1 + k5);
                            }
                        }
                    }
                } else if (type == 1) {
                    int32_t j = 1 + rand() % 2;
                    int k = height - j;
                    int l = 2 + rand() % 2;
                    int i3 = rand() % 2;
                    int j3 = 1;
                    int k3 = 0;
                    for (int l3 = 0; l3 <= k; l3++) {
                        int yoff = y + height - l3;
                        for (int i2 = x - i3; i2 <= x + i3; i2++) {
                            int xoff = i2 - x;
                            for (int k2 = z - i3; k2 <= z + i3; k2++) {
                                int zoff = k2 - z;
                                if (abs(xoff) != i3 || abs(zoff) != i3 || i3 <= 0) {
                                    struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, i2, yoff, k2));
                                    if (bi == NULL || !bi->fullCube) {
                                        world_set_block_guess(world, chunk, leaf, i2, yoff, k2);
                                    }
                                }
                            }
                        }
                        if (i3 >= j3) {
                            i3 = j3;
                            k3 = 1;
                            j3++;
                            if (j3 > l) {
                                j3 = l;
                            }
                        } else {
                            i3++;
                        }
                    }
                } else for (int ty = y - 3 + height; ty <= y + height; ty++) {
                    int dist_from_top = ty - y - height;
                    int width = 1 - dist_from_top / 2;
                    for (int tx = x - width; tx <= x + width; tx++) {
                        int tx2 = tx - x;
                        for (int tz = z - width; tz <= z + width; tz++) {
                            int tz2 = tz - z;
                            if (abs(tx2) != width || abs(tz2) != width || (rand() % 2 == 0 && dist_from_top != 0)) {
                                struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, tx, ty, tz));
                                if (bi != NULL && (str_eq(bi->material->name, "air") || str_eq(bi->material->name, "leaves") || str_eq(bi->material->name, "vine"))) {
                                    world_set_block_guess(world, chunk, leaf, tx, ty, tz);
                                }
                            }
                        }
                    }
                }
                if (type != 4) {
                    for (int th = 0; th < height - (type == 1 ? (rand() % 3) : 0); th++) {
                        struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, x, y + th, z));
                        if (bi != NULL && (str_eq(bi->material->name, "air") || str_eq(bi->material->name, "leaves") || str_eq(bi->material->name, "vine") || str_eq(bi->material->name, "plants"))) {
                            world_set_block_guess(world, chunk, log, x, y + th, z);
                            if (vines && th > 0) {
                                if (rand() % 3 > 0 && world_get_block_guess(world, chunk, x - 1, y + th, z) == 0) world_set_block_guess(world, chunk, BLK_VINE | 0x8, x - 1, y + th, z);
                                if (rand() % 3 > 0 && world_get_block_guess(world, chunk, x + 1, y + th, z) == 0) world_set_block_guess(world, chunk, BLK_VINE | 0x2, x + 1, y + th, z);
                                if (rand() % 3 > 0 && world_get_block_guess(world, chunk, x, y + th, z - 1) == 0) world_set_block_guess(world, chunk, BLK_VINE | 0x1, x + 1, y + th, z - 1);
                                if (rand() % 3 > 0 && world_get_block_guess(world, chunk, x, y + th, z + 1) == 0) world_set_block_guess(world, chunk, BLK_VINE | 0x4, x + 1, y + th, z + 1);
                            }
                        }
                    }
                    if (vines) {
                        for (int ty = y - 3 + height; ty <= y + height; ty++) {
                            int dist_from_top = ty - y - height;
                            int width = 2 - dist_from_top / 2;
                            for (int tx = x - width; tx <= x + width; tx++) {
                                for (int tz = z - width; tz <= z + width; tz++) {
                                    struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, tx, ty, tz));
                                    if (bi != NULL && str_eq(bi->material->name, "leaves")) {
                                        if (rand() % 4 == 0 && world_get_block_guess(world, chunk, tx - 1, ty, tz) == 0) tree_addHangingVine(world, chunk, BLK_VINE | 0x8, tx - 1, ty, tz);
                                        if (rand() % 4 == 0 && world_get_block_guess(world, chunk, tx + 1, ty, tz) == 0) tree_addHangingVine(world, chunk, BLK_VINE | 0x2, tx + 1, ty, tz);
                                        if (rand() % 4 == 0 && world_get_block_guess(world, chunk, tx, ty, tz - 1) == 0) tree_addHangingVine(world, chunk, BLK_VINE | 0x1, tx + 1, ty, tz - 1);
                                        if (rand() % 4 == 0 && world_get_block_guess(world, chunk, tx, ty, tz + 1) == 0) tree_addHangingVine(world, chunk, BLK_VINE | 0x4, tx + 1, ty, tz + 1);
                                    }
                                }
                            }
                        }
                        if (cocoa) if (rand() % 5 == 0 && height > 5) {
                            for (int ty = y; ty < y + 2; ty++) {
                                if (rand() % (4 - ty - y) == 0) {
                                    //TODO: cocoa
                                }
                            }
                        }
                    }
                }
            } else {

            }
        } else {
            world_set_block_guess(world, chunk, (blk | 0x8), x, y, z);
        }
    }
}

block onBlockPlacedPlayer_log(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face) {
    block nb = blk & ~0xC;
    if (face == XP || face == XN) {
        nb |= 0x4;
    } else if (face == ZP || face == ZN) {
        nb |= 0x8;
    }
    return nb;
}

void onBlockInteract_fencegate(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
    blk ^= (block) 0b0100; // toggle opened bit
    world_set_block_guess(world, NULL, blk, x, y, z);
}

int fluid_getDepth(int water, block b) {
    uint16_t ba = b >> 4;
    if (water && (ba != BLK_WATER >> 4 && ba != BLK_WATER_1 >> 4)) return -1;
    else if (!water && (ba != BLK_LAVA >> 4 && ba != BLK_LAVA_1 >> 4)) return -1;
    return b & 0x0f;
}

int fluid_checkAdjacentBlock(int water, struct world* world, struct chunk* ch, int32_t x, uint8_t y, int32_t z, int cmin, int* adj) {
    block b = world_get_block_guess(world, ch, x, y, z);
    int m = fluid_getDepth(water, b);
    if (m < 0) return cmin;
    else {
        if (m == 0) (*adj)++;
        if (m >= 8) m = 0;
        return cmin >= 0 && m >= cmin ? cmin : m;
    }
}

int fluid_isUnblocked(int water, block b, struct block_info* bi) {
    uint16_t ba = b >> 4;
    if (ba == BLK_DOOROAK >> 4 || ba == BLK_DOORIRON >> 4 || ba == BLK_SIGN >> 4 || ba == BLK_LADDER >> 4 || ba == BLK_REEDS >> 4) return 0;
    if (bi == NULL) return 0;
    if (!str_eq(bi->material->name, "portal") && !str_eq(bi->material->name, "structure_void") ? bi->material->blocksMovement : 1) return 0;
    return 1;
}

int fluid_canFlowInto(int water, block b) {
    struct block_info* bi = getBlockInfo(b);
    if (bi == NULL) return 1;
    if (!((water ? !str_eq(bi->material->name, "water") : 1) && !str_eq(bi->material->name, "lava"))) return 0;
    return fluid_isUnblocked(water, b, bi);
}

int lava_checkForMixing(struct world* world, struct chunk* ch, block blk, int32_t x, uint8_t y, int32_t z) {
    int cww = 0;
    block b = world_get_block_guess(world, ch, x + 1, y, z);
    if (b >> 4 == BLK_WATER >> 4 || b >> 4 == BLK_WATER_1 >> 4) cww = 1;
    if (!cww) {
        b = world_get_block_guess(world, ch, x - 1, y, z);
        if (b >> 4 == BLK_WATER >> 4 || b >> 4 == BLK_WATER_1 >> 4) cww = 1;
    }
    if (!cww) {
        b = world_get_block_guess(world, ch, x, y, z + 1);
        if (b >> 4 == BLK_WATER >> 4 || b >> 4 == BLK_WATER_1 >> 4) cww = 1;
    }
    if (!cww) {
        b = world_get_block_guess(world, ch, x, y, z - 1);
        if (b >> 4 == BLK_WATER >> 4 || b >> 4 == BLK_WATER_1 >> 4) cww = 1;
    }
    if (!cww) {
        b = world_get_block_guess(world, ch, x, y + 1, z);
        if (b >> 4 == BLK_WATER >> 4 || b >> 4 == BLK_WATER_1 >> 4) cww = 1;
    }
    if (cww) {
        if ((blk & 0x0f) == 0) {
            world_set_block_guess(world, ch, BLK_OBSIDIAN, x, y, z);
            return 1;
        } else if ((blk & 0x0f) <= 4) {
            world_set_block_guess(world, ch, BLK_COBBLESTONE, x, y, z);
            return 1;
        }
    }
    return 0;
}

void fluid_doFlowInto(int water, struct world* world, struct chunk* ch, int32_t x, uint8_t y, int32_t z, int level, block b) {
// potentially triggerMixEffects
    struct block_info* bi = getBlockInfo(b);
    if (bi != NULL && !str_eq(bi->material->name, "air") && !str_eq(bi->material->name, "lava")) {
        dropBlockDrops(world, b, NULL, x, y, z);
    }
    world_set_block_guess(world, ch, (water ? BLK_WATER : BLK_LAVA) | level, x, y, z);
    world_schedule_block_tick(world, x, y, z, water ? 5 : (world->dimension == -1 ? 10 : 30), 0.);
}

void onBlockUpdate_staticfluid(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    int water = (blk >> 4) == (BLK_WATER >> 4) || (blk >> 4) == (BLK_WATER_1 >> 4);
    if (water || !lava_checkForMixing(world, NULL, blk, x, y, z)) {
        world_schedule_block_tick(world, x, y, z, water ? 5 : (world->dimension == -1 ? 10 : 30), 0.);
        if (water) world_set_block_noupdate(world, BLK_WATER | (blk & 0x0f), x, y, z);
        else if (!water) world_set_block_noupdate(world, BLK_LAVA | (blk & 0x0f), x, y, z);
    }
}

void randomTick_staticlava(struct world* world, struct chunk* ch, block blk, int32_t x, int32_t y, int32_t z) {
    int t = rand() % 3;
    if (t == 0) {
        for (int i = 0; i < 3; i++) {
            int32_t nx = x + rand() % 3 - 1;
            int32_t nz = z + rand() % 3 - 1;
            if (world_get_block_guess(world, ch, nx, y + 1, nz) == 0) {
                struct block_info* bi = getBlockInfo(world_get_block_guess(world, ch, nx, y, nz));
                if (bi != NULL && bi->material->flammable) {
                    world_set_block_guess(world, ch, BLK_FIRE, nx, y + 1, nz);
                }
            }
        }
    } else {
        for (int i = 0; i < t; i++) {
            int32_t nx = x + rand() % 3 - 1;
            int32_t nz = z + rand() % 3 - 1;
            block mb = world_get_block_guess(world, ch, nx, y + 1, nz);
            struct block_info* bi = getBlockInfo(mb);
            if (mb == 0 || bi == NULL) {
                int g = 0;
                bi = getBlockInfo(world_get_block_guess(world, ch, nx + 1, y + 1, nz));
                if (bi != NULL && bi->material->flammable) g = 1;
                if (!g) {
                    bi = getBlockInfo(world_get_block_guess(world, ch, nx - 1, y + 1, nz));
                    if (bi != NULL && bi->material->flammable) g = 1;
                }
                if (!g) {
                    bi = getBlockInfo(world_get_block_guess(world, ch, nx, y + 1, nz - 1));
                    if (bi != NULL && bi->material->flammable) g = 1;
                }
                if (!g) {
                    bi = getBlockInfo(world_get_block_guess(world, ch, nx, y + 1, nz + 1));
                    if (bi != NULL && bi->material->flammable) g = 1;
                }
                if (!g) {
                    bi = getBlockInfo(world_get_block_guess(world, ch, nx, y, nz - 1));
                    if (bi != NULL && bi->material->flammable) g = 1;
                }
                if (!g) {
                    bi = getBlockInfo(world_get_block_guess(world, ch, nx, y + 2, nz - 1));
                    if (bi != NULL && bi->material->flammable) g = 1;
                }
                if (g) {
                    world_set_block_guess(world, ch, BLK_FIRE, nx, y + 1, nz);
                    return;
                }
            } else if (bi->material->blocksMovement) return;
        }
    }
}

void onBlockUpdate_flowinglava(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    lava_checkForMixing(world, NULL, blk, x, y, z);
}

int scheduledTick_flowingfluid(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    struct chunk* ch = world_get_chunk(world, x >> 4, z >> 4);
    if (ch == NULL) return 0;
    int level = blk & 0x0f;
    int resistance = 1;
    if (((blk >> 4) == (BLK_LAVA >> 4) || (blk >> 4) == (BLK_LAVA_1 >> 4)) && world->dimension != -1) resistance++;
    int adjSource = 0;
    int water = (blk >> 4) == (BLK_WATER >> 4) || (blk >> 4) == (BLK_WATER_1 >> 4);
    int tickRate = water ? 5 : (world->dimension == -1 ? 10 : 30);
    if (level > 0) {
        int l = -100;
        l = fluid_checkAdjacentBlock(water, world, ch, x + 1, y, z, l, &adjSource);
        l = fluid_checkAdjacentBlock(water, world, ch, x - 1, y, z, l, &adjSource);
        l = fluid_checkAdjacentBlock(water, world, ch, x, y, z + 1, l, &adjSource);
        l = fluid_checkAdjacentBlock(water, world, ch, x, y, z - 1, l, &adjSource);
        int i1 = l + resistance;
        if (i1 >= 8 || l < 0) i1 = -1;
        int ha = fluid_getDepth(water, world_get_block_guess(world, ch, x, y + 1, z));
        if (ha >= 0) {
            if (ha >= 8) i1 = ha;
            else i1 = ha + 8;
        }
        if (adjSource >= 2 && water) {
            block below = world_get_block_guess(world, ch, x, y - 1, z);
            if ((below >> 4) == (BLK_WATER >> 4) || (below >> 4) == (BLK_WATER_1 >> 4)) {
                i1 = 0;
            } else {
                struct block_info* bi = getBlockInfo(below);
                if (bi != NULL && bi->material->solid) i1 = 0;
            }
        }
        if (!water && level < 8 && i1 < 8 && i1 > level && rand() % 4 > 0) {
            tickRate *= 4;
        }
        if (i1 == level) {
            if (water) world_set_block_noupdate(world, BLK_WATER_1 | (i1), x, y, z);
            else if (!water) world_set_block_noupdate(world, BLK_LAVA_1 | (i1), x, y, z);
            tickRate = 0;
        } else {
            level = i1;
            if (i1 < 0) world_set_block_guess(world, ch, 0, x, y, z);
            else {
                world_set_block_guess(world, ch, (blk & ~0x0f) | i1, x, y, z);
            }
        }
    } else {
        if (water) world_set_block_noupdate(world, BLK_WATER_1 | (level), x, y, z);
        else if (!water) world_set_block_noupdate(world, BLK_LAVA_1 | (level), x, y, z);
        tickRate = 0;
    }
    block down = world_get_block_guess(world, ch, x, y - 1, z);
    if (fluid_canFlowInto(water, down)) {
        if (!water && ((down >> 4) == (BLK_WATER >> 4) || (down >> 4) == (BLK_WATER_1 >> 4))) {
            world_set_block_guess(world, ch, BLK_STONE, x, y - 1, z);
            //trigger mix effects
            return tickRate;
        }
        fluid_doFlowInto(water, world, ch, x, y - 1, z, level >= 8 ? level : (level + 8), down);
    } else if (level >= 0) {
        if (level == 0 || !fluid_isUnblocked(water, down, getBlockInfo(down))) {
            int k1 = level + resistance;
            if (level >= 8) k1 = 1;
            if (k1 >= 8) return tickRate;
            block b = world_get_block_guess(world, ch, x + 1, y, z);
            if (fluid_canFlowInto(water, b)) fluid_doFlowInto(water, world, ch, x + 1, y, z, k1, b);
            b = world_get_block_guess(world, ch, x - 1, y, z);
            if (fluid_canFlowInto(water, b)) fluid_doFlowInto(water, world, ch, x - 1, y, z, k1, b);
            b = world_get_block_guess(world, ch, x, y, z + 1);
            if (fluid_canFlowInto(water, b)) fluid_doFlowInto(water, world, ch, x, y, z + 1, k1, b);
            b = world_get_block_guess(world, ch, x, y, z - 1);
            if (fluid_canFlowInto(water, b)) fluid_doFlowInto(water, world, ch, x, y, z - 1, k1, b);
        }
    }
    return tickRate;
}

//

struct block_info* getBlockInfo(block b) {
    if (b >= block_infos->size) {
        b &= ~0x0f;
        if (b >= block_infos->size) return NULL;
        else return block_infos->data[b];
    }
    if (block_infos->data[b] != NULL) return block_infos->data[b];
    return block_infos->data[b & ~0x0f];
}

struct block_info* getBlockInfoLoose(block b) {
    return block_infos->data[b & ~0x0f];
}

// index = item ID
const char* nameToIDMap[] = { "minecraft:air", "minecraft:stone", "minecraft:grass", "minecraft:dirt", "minecraft:cobblestone", "minecraft:planks", "minecraft:sapling", "minecraft:bedrock", "", "", "", "", "minecraft:sand", "minecraft:gravel", "minecraft:gold_ore", "minecraft:iron_ore", "minecraft:coal_ore", "minecraft:log", "minecraft:leaves", "minecraft:sponge", "minecraft:glass", "minecraft:lapis_ore", "minecraft:lapis_block", "minecraft:dispenser", "minecraft:sandstone", "minecraft:noteblock", "", "minecraft:golden_rail", "minecraft:detector_rail", "minecraft:sticky_piston", "minecraft:web", "minecraft:tallgrass", "minecraft:deadbush", "minecraft:piston", "", "minecraft:wool", "", "minecraft:yellow_flower", "minecraft:red_flower", "minecraft:brown_mushroom", "minecraft:red_mushroom", "minecraft:gold_block", "minecraft:iron_block", "", "minecraft:stone_slab", "minecraft:brick_block", "minecraft:tnt", "minecraft:bookshelf", "minecraft:mossy_cobblestone", "minecraft:obsidian",
        "minecraft:torch", "", "minecraft:mob_spawner", "minecraft:oak_stairs", "minecraft:chest", "", "minecraft:diamond_ore", "minecraft:diamond_block", "minecraft:crafting_table", "", "minecraft:farmland", "minecraft:furnace", "", "", "", "minecraft:ladder", "minecraft:rail", "minecraft:stone_stairs", "", "minecraft:lever", "minecraft:stone_pressure_plate", "", "minecraft:wooden_pressure_plate", "minecraft:redstone_ore", "", "", "minecraft:redstone_torch", "minecraft:stone_button", "minecraft:snow_layer", "minecraft:ice", "minecraft:snow", "minecraft:cactus", "minecraft:clay", "", "minecraft:jukebox", "minecraft:fence", "minecraft:pumpkin", "minecraft:netherrack", "minecraft:soul_sand", "minecraft:glowstone", "", "minecraft:lit_pumpkin", "", "", "", "minecraft:stained_glass", "minecraft:trapdoor", "minecraft:monster_egg", "minecraft:stonebrick", "minecraft:brown_mushroom_block", "minecraft:red_mushroom_block", "minecraft:iron_bars", "minecraft:glass_pane", "minecraft:melon_block",
        "", "", "minecraft:vine", "minecraft:fence_gate", "minecraft:brick_stairs", "minecraft:stone_brick_stairs", "minecraft:mycelium", "minecraft:waterlily", "minecraft:nether_brick", "minecraft:nether_brick_fence", "minecraft:nether_brick_stairs", "", "minecraft:enchanting_table", "", "", "", "minecraft:end_portal_frame", "minecraft:end_stone", "minecraft:dragon_egg", "minecraft:redstone_lamp", "", "", "minecraft:wooden_slab", "", "minecraft:sandstone_stairs", "minecraft:emerald_ore", "minecraft:ender_chest", "minecraft:tripwire_hook", "", "minecraft:emerald_block", "minecraft:spruce_stairs", "minecraft:birch_stairs", "minecraft:jungle_stairs", "minecraft:command_block", "minecraft:beacon", "minecraft:cobblestone_wall", "", "", "", "minecraft:wooden_button", "", "minecraft:anvil", "minecraft:trapped_chest", "minecraft:light_weighted_pressure_plate", "minecraft:heavy_weighted_pressure_plate", "", "", "minecraft:daylight_detector", "minecraft:redstone_block", "minecraft:quartz_ore",
        "minecraft:hopper", "minecraft:quartz_block", "minecraft:quartz_stairs", "minecraft:activator_rail", "minecraft:dropper", "minecraft:stained_hardened_clay", "minecraft:stained_glass_pane", "minecraft:leaves2", "minecraft:log2", "minecraft:acacia_stairs", "minecraft:dark_oak_stairs", "minecraft:slime", "minecraft:barrier", "minecraft:iron_trapdoor", "minecraft:prismarine", "minecraft:sea_lantern", "minecraft:hay_block", "minecraft:carpet", "minecraft:hardened_clay", "minecraft:coal_block", "minecraft:packed_ice", "minecraft:double_plant", "", "", "", "minecraft:red_sandstone", "minecraft:red_sandstone_stairs", "", "minecraft:stone_slab2", "minecraft:spruce_fence_gate", "minecraft:birch_fence_gate", "minecraft:jungle_fence_gate", "minecraft:dark_oak_fence_gate", "minecraft:acacia_fence_gate", "minecraft:spruce_fence", "minecraft:birch_fence", "minecraft:jungle_fence", "minecraft:dark_oak_fence", "minecraft:acacia_fence", "", "", "", "", "", "minecraft:end_rod",
        "minecraft:chorus_plant", "minecraft:chorus_flower", "minecraft:purpur_block", "minecraft:purpur_pillar", "minecraft:purpur_stairs", "", "minecraft:purpur_slab", "minecraft:end_bricks", "", "minecraft:grass_path", "", "minecraft:repeating_command_block", "minecraft:chain_command_block", "", "minecraft:magma", "minecraft:nether_wart_block", "minecraft:red_nether_brick", "minecraft:bone_block", "minecraft:structure_void", "minecraft:observer", "minecraft:white_shulker_box", "minecraft:orange_shulker_box", "minecraft:magenta_shulker_box", "minecraft:light_blue_shulker_box", "minecraft:yellow_shulker_box", "minecraft:lime_shulker_box", "minecraft:pink_shulker_box", "minecraft:gray_shulker_box", "minecraft:silver_shulker_box", "minecraft:cyan_shulker_box", "minecraft:purple_shulker_box", "minecraft:blue_shulker_box", "minecraft:brown_shulker_box", "minecraft:green_shulker_box", "minecraft:red_shulker_box", "minecraft:black_shulker_box", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "minecraft:structure_block", "minecraft:iron_shovel", "minecraft:iron_pickaxe", "minecraft:iron_axe", "minecraft:flint_and_steel", "minecraft:apple", "minecraft:bow", "minecraft:arrow", "minecraft:coal", "minecraft:diamond", "minecraft:iron_ingot", "minecraft:gold_ingot", "minecraft:iron_sword", "minecraft:wooden_sword", "minecraft:wooden_shovel", "minecraft:wooden_pickaxe", "minecraft:wooden_axe", "minecraft:stone_sword", "minecraft:stone_shovel", "minecraft:stone_pickaxe", "minecraft:stone_axe", "minecraft:diamond_sword", "minecraft:diamond_shovel", "minecraft:diamond_pickaxe", "minecraft:diamond_axe", "minecraft:stick", "minecraft:bowl", "minecraft:mushroom_stew", "minecraft:golden_sword", "minecraft:golden_shovel", "minecraft:golden_pickaxe", "minecraft:golden_axe", "minecraft:string", "minecraft:feather", "minecraft:gunpowder", "minecraft:wooden_hoe", "minecraft:stone_hoe", "minecraft:iron_hoe", "minecraft:diamond_hoe",
        "minecraft:golden_hoe", "minecraft:wheat_seeds", "minecraft:wheat", "minecraft:bread", "minecraft:leather_helmet", "minecraft:leather_chestplate", "minecraft:leather_leggings", "minecraft:leather_boots", "minecraft:chainmail_helmet", "minecraft:chainmail_chestplate", "minecraft:chainmail_leggings", "minecraft:chainmail_boots", "minecraft:iron_helmet", "minecraft:iron_chestplate", "minecraft:iron_leggings", "minecraft:iron_boots", "minecraft:diamond_helmet", "minecraft:diamond_chestplate", "minecraft:diamond_leggings", "minecraft:diamond_boots", "minecraft:golden_helmet", "minecraft:golden_chestplate", "minecraft:golden_leggings", "minecraft:golden_boots", "minecraft:flint", "minecraft:porkchop", "minecraft:cooked_porkchop", "minecraft:painting", "minecraft:golden_apple", "minecraft:sign", "minecraft:wooden_door", "minecraft:bucket", "minecraft:water_bucket", "minecraft:lava_bucket", "minecraft:minecart", "minecraft:saddle", "minecraft:iron_door", "minecraft:redstone",
        "minecraft:snowball", "minecraft:boat", "minecraft:leather", "minecraft:milk_bucket", "minecraft:brick", "minecraft:clay_ball", "minecraft:reeds", "minecraft:paper", "minecraft:book", "minecraft:slime_ball", "minecraft:chest_minecart", "minecraft:furnace_minecart", "minecraft:egg", "minecraft:compass", "minecraft:fishing_rod", "minecraft:clock", "minecraft:glowstone_dust", "minecraft:fish", "minecraft:cooked_fish", "minecraft:dye", "minecraft:bone", "minecraft:sugar", "minecraft:cake", "minecraft:bed", "minecraft:repeater", "minecraft:cookie", "minecraft:filled_map", "minecraft:shears", "minecraft:melon", "minecraft:pumpkin_seeds", "minecraft:melon_seeds", "minecraft:beef", "minecraft:cooked_beef", "minecraft:chicken", "minecraft:cooked_chicken", "minecraft:rotten_flesh", "minecraft:ender_pearl", "minecraft:blaze_rod", "minecraft:ghast_tear", "minecraft:gold_nugget", "minecraft:nether_wart", "minecraft:potion", "minecraft:glass_bottle", "minecraft:spider_eye",
        "minecraft:fermented_spider_eye", "minecraft:blaze_powder", "minecraft:magma_cream", "minecraft:brewing_stand", "minecraft:cauldron", "minecraft:ender_eye", "minecraft:speckled_melon", "minecraft:spawn_egg", "minecraft:experience_bottle", "minecraft:fire_charge", "minecraft:writable_book", "minecraft:written_book", "minecraft:emerald", "minecraft:item_frame", "minecraft:flower_pot", "minecraft:carrot", "minecraft:potato", "minecraft:baked_potato", "minecraft:poisonous_potato", "minecraft:map", "minecraft:golden_carrot", "minecraft:skull", "minecraft:carrot_on_a_stick", "minecraft:nether_star", "minecraft:pumpkin_pie", "minecraft:fireworks", "minecraft:firework_charge", "minecraft:enchanted_book", "minecraft:comparator", "minecraft:netherbrick", "minecraft:quartz", "minecraft:tnt_minecart", "minecraft:hopper_minecart", "minecraft:prismarine_shard", "minecraft:prismarine_crystals", "minecraft:rabbit", "minecraft:cooked_rabbit", "minecraft:rabbit_stew", "minecraft:rabbit_foot",
        "minecraft:rabbit_hide", "minecraft:armor_stand", "minecraft:iron_horse_armor", "minecraft:golden_horse_armor", "minecraft:diamond_horse_armor", "minecraft:lead", "minecraft:name_tag", "minecraft:command_block_minecart", "minecraft:mutton", "minecraft:cooked_mutton", "", "minecraft:end_crystal", "minecraft:spruce_door", "minecraft:birch_door", "minecraft:jungle_door", "minecraft:acacia_door", "minecraft:dark_oak_door", "minecraft:chorus_fruit", "minecraft:chorus_fruit_popped", "minecraft:beetroot", "minecraft:beetroot_seeds", "minecraft:beetroot_soup", "minecraft:dragon_breath", "minecraft:splash_potion", "minecraft:spectral_arrow", "minecraft:tipped_arrow", "minecraft:lingering_potion", "minecraft:shield", "minecraft:elytra", "minecraft:spruce_boat", "minecraft:birch_boat", "minecraft:jungle_boat", "minecraft:acacia_boat", "minecraft:dark_oak_boat", "minecraft:totem", "minecraft:shulker_shell", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "minecraft:record_13", "minecraft:record_cat", "minecraft:record_blocks", "minecraft:record_chirp", "minecraft:record_far", "minecraft:record_mall", "minecraft:record_mellohi", "minecraft:record_stal", "minecraft:record_strad", "minecraft:record_ward", "minecraft:record_11", "minecraft:record_wait" };

item getItemFromName(const char* name) {
    for (size_t i = 0; i < (sizeof(nameToIDMap) / sizeof(char*)); i++) {
        if (str_eq(nameToIDMap[i], name)) return i;
    }
    return -1;
}

struct block_material* get_block_material(char* name) {
    return hashmap_get(block_materials, name);
}

size_t get_block_count() {
    return block_infos->size;
}

void add_block_material(struct block_material* material) {
    hashmap_put(block_materials, material->name, material);
}

void add_block_info(block blk, struct block_info* info) {
    list_ensure_capacity(block_infos, blk + 1);
    block_infos->data[blk] = info;
    if (block_infos->size < blk) block_infos->size = blk;
    block_infos->count++;
}

struct mempool* block_pool;

void init_materials() {
    if (block_pool == NULL) {
        block_pool = mempool_new();
    }
    block_materials = hashmap_new(64, block_pool);
    char* json_file = (char*) read_file_fully(block_pool, "materials.json", NULL);
    if (json_file == NULL) {
        errlog(delog, "Error reading material data: %s\n", strerror(errno));
        return;
    }
    struct json_object* json = NULL;
    json_parse(block_pool, &json, json_file);
    pprefree(block_pool, json_file);

    ITER_LLIST(json->children_list, value) {
        struct json_object* material_json = value;
        struct block_material* material = pcalloc(block_pool, sizeof(struct block_material));
        material->name = str_dup(material_json->name, 0, block_pool);
        struct json_object* tmp = json_get(material_json, "flammable");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->flammable = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "replaceable");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->replacable = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "requiresnotool");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->requiresnotool = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "mobility");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_material;
        material->mobility = (uint8_t) tmp->data.number;
        tmp = json_get(material_json, "adventure_exempt");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->adventure_exempt = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "liquid");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->liquid = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "solid");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->solid = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "blocksLight");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->blocksLight = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "blocksMovement");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->blocksMovement = tmp->type == JSON_TRUE;
        tmp = json_get(material_json, "opaque");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_material;
        material->opaque = tmp->type == JSON_TRUE;
        add_block_material(material);
        continue;
        format_error_material:;
        printf("[WARNING] Error Loading Material \"%s\"! Skipped.\n", material_json->name);
        ITER_LLIST_END();
    }
    pfree(json->pool);
}

int block_is_normal_cube(struct block_info* info) {
    return info == NULL ? 0 : (info->material->opaque && info->fullCube && !info->canProvidePower);
}

void init_blocks() {
    if (block_pool == NULL) {
        block_pool = mempool_new();
    }
    block_infos = list_new(128, block_pool);
    char* json_file = (char*) read_file_fully(block_pool, "blocks.json", NULL);
    if (json_file == NULL) {
        errlog(delog, "Error reading block data: %s\n", strerror(errno));
        return;
    }
    struct json_object* json = NULL;
    json_parse(block_pool, &json, json_file);
    pprefree(block_pool, json_file);

    ITER_LLIST(json->children_list, value) {
        struct json_object* block_json = value;
        struct block_info* info = pcalloc(block_pool, sizeof(struct block_info));
        struct json_object* tmp = json_get(block_json, "id");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        block id = (block) tmp->data.number;
        if (id < 0) goto format_error_block;
        struct json_object* colls = json_get(block_json, "collision");
        if (colls == NULL || colls->type != JSON_OBJECT) goto format_error_block;
        info->boundingBox_count = colls->children_list->size;
        info->boundingBoxes = info->boundingBox_count == 0 ? NULL : pmalloc(block_pool, sizeof(struct boundingbox) * info->boundingBox_count);
        size_t x = 0;
        ITER_LLIST(json->children_list, collision_json) {
            struct json_object* coll = collision_json;
            struct boundingbox* box = &info->boundingBoxes[x];
            tmp = json_get(coll, "minX");
            if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
            box->minX = tmp->data.number;
            tmp = json_get(coll, "maxX");
            if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
            box->maxX = tmp->data.number;
            tmp = json_get(coll, "minY");
            if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
            box->minY = tmp->data.number;
            tmp = json_get(coll, "maxY");
            if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
            box->maxY = tmp->data.number;
            tmp = json_get(coll, "minZ");
            if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
            box->minZ = tmp->data.number;
            tmp = json_get(coll, "maxZ");
            if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
            box->maxZ = tmp->data.number;
            ++x;
            ITER_LLIST_END();
        }
        tmp = json_get(block_json, "dropItem");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->drop = (item) tmp->data.number;
        tmp = json_get(block_json, "dropDamage");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->drop_damage = (int16_t) tmp->data.number;
        tmp = json_get(block_json, "dropAmountMin");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->drop_min = (uint8_t) tmp->data.number;
        tmp = json_get(block_json, "dropAmountMax");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->drop_max = (uint8_t) tmp->data.number;
        tmp = json_get(block_json, "hardness");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->hardness = (float) tmp->data.number;
        tmp = json_get(block_json, "material");
        if (tmp == NULL || tmp->type != JSON_STRING) goto format_error_block;
        info->material = get_block_material(tmp->data.string);
        tmp = json_get(block_json, "slipperiness");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->slipperiness = (float) tmp->data.number;
        tmp = json_get(block_json, "isFullCube");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_block;
        info->fullCube = tmp->type == JSON_TRUE;
        tmp = json_get(block_json, "canProvidePower");
        if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto format_error_block;
        info->canProvidePower = tmp->type == JSON_TRUE;
        tmp = json_get(block_json, "lightOpacity");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->lightOpacity = (uint8_t) tmp->data.number;
        tmp = json_get(block_json, "lightEmission");
        if (tmp == NULL || tmp->type != JSON_NUMBER) goto format_error_block;
        info->lightEmission = (uint8_t) tmp->data.number;
        tmp = json_get(block_json, "resistance");
        if (tmp == NULL || tmp->type != JSON_NUMBER) {
            info->resistance = info->hardness * 5;
        } else {
            info->resistance = (float) tmp->data.number * 3;
        }
        add_block_info(id, info);
        continue;
        format_error_block: ;
        printf("[WARNING] Error Loading Block \"%s\"! Skipped.\n", block_json->name);
        ITER_LLIST_END();
    }
    pfree(json->pool);
    struct block_info* tmp = getBlockInfo(BLK_CHEST);
    tmp->onBlockInteract = &onBlockInteract_chest;
    tmp->onBlockDestroyed = &onBlockDestroyed_chest;
    tmp->onBlockPlaced = &onBlockPlaced_chest;
    tmp = getBlockInfo(BLK_WORKBENCH);
    tmp->onBlockInteract = &onBlockInteract_workbench;
    tmp = getBlockInfo(BLK_FURNACE);
    tmp->onBlockInteract = &onBlockInteract_furnace;
    tmp->onBlockDestroyed = &onBlockDestroyed_furnace;
    tmp->onBlockPlaced = &onBlockPlaced_furnace;
    tmp = getBlockInfo(BLK_FURNACE_1);
    tmp->onBlockInteract = &onBlockInteract_furnace;
    tmp->onBlockDestroyed = &onBlockDestroyed_furnace;
    tmp->onBlockPlaced = &onBlockPlaced_furnace;
    tmp = getBlockInfo(BLK_CHESTTRAP);
    tmp->onBlockInteract = &onBlockInteract_chest;
    tmp->onBlockDestroyed = &onBlockDestroyed_chest;
    tmp->onBlockPlaced = &onBlockPlaced_chest;
    tmp = getBlockInfo(BLK_GRAVEL);
    tmp->dropItems = &dropItems_gravel;
    getBlockInfo(BLK_LEAVES_OAK)->dropItems = &dropItems_leaves;
    getBlockInfo(BLK_LEAVES_SPRUCE)->dropItems = &dropItems_leaves;
    getBlockInfo(BLK_LEAVES_BIRCH)->dropItems = &dropItems_leaves;
    getBlockInfo(BLK_LEAVES_JUNGLE)->dropItems = &dropItems_leaves;
    getBlockInfo(BLK_LEAVES_ACACIA)->dropItems = &dropItems_leaves;
    getBlockInfo(BLK_LEAVES_BIG_OAK)->dropItems = &dropItems_leaves;
    tmp = getBlockInfo(BLK_TALLGRASS_GRASS);
    tmp->dropItems = &dropItems_tallgrass;
    tmp->canBePlaced = &canBePlaced_requiredirt;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_TALLGRASS_FERN);
    tmp->canBePlaced = &canBePlaced_requiredirt;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_TALLGRASS_SHRUB);
    tmp->canBePlaced = &canBePlaced_requiresand;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_CACTUS);
    tmp->canBePlaced = &canBePlaced_cactus;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_REEDS);
    tmp->canBePlaced = &canBePlaced_reeds;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    getBlockInfo(BLK_MUSHROOM_2)->dropItems = &dropItems_hugemushroom;
    getBlockInfo(BLK_MUSHROOM_3)->dropItems = &dropItems_hugemushroom;
    tmp = getBlockInfo(BLK_CROPS);
    tmp->dropItems = &dropItems_crops;
    tmp->canBePlaced = &canBePlaced_requirefarmland;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp->randomTick = &randomTick_crops;
    tmp = getBlockInfo(BLK_PUMPKINSTEM);
    tmp->dropItems = &dropItems_crops;
    tmp->canBePlaced = &canBePlaced_requirefarmland;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_PUMPKINSTEM_1);
    tmp->dropItems = &dropItems_crops;
    tmp->canBePlaced = &canBePlaced_requirefarmland;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_CARROTS);
    tmp->dropItems = &dropItems_crops;
    tmp->canBePlaced = &canBePlaced_requirefarmland;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_POTATOES);
    tmp->dropItems = &dropItems_crops;
    tmp->canBePlaced = &canBePlaced_requirefarmland;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_NETHERSTALK);
    tmp->dropItems = &dropItems_crops;
    tmp->canBePlaced = &canBePlaced_requiresoulsand;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_BEETROOTS);
    tmp->dropItems = &dropItems_crops;
    tmp->canBePlaced = &canBePlaced_requirefarmland;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    tmp = getBlockInfo(BLK_COCOA);
    tmp->dropItems = &dropItems_crops;
//tmp->onBlockUpdate = &onBlockUpdate_cocoa;
    for (block b = BLK_FLOWER1_DANDELION; b <= BLK_FLOWER2_OXEYEDAISY; b++) {
        tmp = getBlockInfo(b);
        tmp->canBePlaced = &canBePlaced_requiredirt;
        tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    }
    for (block b = BLK_SAPLING_OAK; b <= BLK_SAPLING_OAK_5; b++) {
        tmp = getBlockInfo(b);
        tmp->canBePlaced = &canBePlaced_sapling;
        tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    }
    for (block b = BLK_DOUBLEPLANT_SUNFLOWER; b <= BLK_DOUBLEPLANT_SUNFLOWER_8; b++) {
        tmp = getBlockInfo(b);
        tmp->canBePlaced = &canBePlaced_doubleplant;
        tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    }
    for (block b = BLK_LOG_OAK; b < BLK_LOG_OAK_8; b++)
        getBlockInfo(b)->onBlockPlacedPlayer = &onBlockPlacedPlayer_log;
    for (block b = BLK_LOG_ACACIA_1; b < BLK_LOG_OAK_14; b++)
        getBlockInfo(b)->onBlockPlacedPlayer = &onBlockPlacedPlayer_log;
    getBlockInfo(BLK_GRASS)->randomTick = &randomTick_grass;
    tmp = getBlockInfo(BLK_VINE);
    tmp->onBlockPlacedPlayer = &onBlockPlacedPlayer_vine;
    tmp->onBlockUpdate = &onBlockUpdate_vine;
    tmp->canBePlaced = &canBePlaced_vine;
    tmp->randomTick = &randomTick_vine;
    tmp = getBlockInfo(BLK_LADDER);
    tmp->onBlockPlacedPlayer = &onBlockPlacedPlayer_ladder;
    tmp->canBePlaced = &canBePlaced_ladder;
    tmp->onBlockUpdate = &onBlockUpdate_checkPlace;
    for (block b = BLK_FENCEGATE; b < BLK_FENCEGATE + 16; b++)
        getBlockInfo(b)->onBlockInteract = &onBlockInteract_fencegate;
    getBlockInfo(BLK_WATER)->scheduledTick = &scheduledTick_flowingfluid;
    getBlockInfo(BLK_WATER_1)->onBlockUpdate = &onBlockUpdate_staticfluid;
    tmp = getBlockInfo(BLK_LAVA);
    tmp->scheduledTick = &scheduledTick_flowingfluid;
    tmp->onBlockUpdate = &onBlockUpdate_flowinglava;
    tmp = getBlockInfo(BLK_LAVA_1);
    tmp->onBlockUpdate = &onBlockUpdate_staticfluid;
    tmp->randomTick = &randomTick_staticlava;
    tmp = getBlockInfo(BLK_FARMLAND);
    tmp->randomTick = &randomTick_farmland;
    getBlockInfo(BLK_SAND)->onBlockUpdate = &onBlockUpdate_falling;
    getBlockInfo(BLK_SAND)->scheduledTick = &scheduledTick_falling;
    getBlockInfo(BLK_GRAVEL)->onBlockUpdate = &onBlockUpdate_falling;
    getBlockInfo(BLK_GRAVEL)->scheduledTick = &scheduledTick_falling;
    getBlockInfo(BLK_ANVIL)->onBlockUpdate = &onBlockUpdate_falling;
    getBlockInfo(BLK_ANVIL)->scheduledTick = &scheduledTick_falling;
    getBlockInfo(BLK_SPONGE_DRY)->onBlockUpdate = &onBlockUpdate_sponge;
    for (block b = BLK_DOOROAK; b < BLK_DOOROAK + 16; b++) {
        tmp = getBlockInfo(b);
        tmp->onBlockInteract = &onBlockInteract_woodendoor;
        tmp->canBePlaced = &canBePlaced_door;
        tmp->onBlockUpdate = &onBlockUpdate_door;
    }
    for (block b = BLK_DOORSPRUCE; b < BLK_DOORSPRUCE + 16; b++) {
        tmp = getBlockInfo(b);
        tmp->onBlockInteract = &onBlockInteract_woodendoor;
        tmp->canBePlaced = &canBePlaced_door;
        tmp->onBlockUpdate = &onBlockUpdate_door;
    }
    for (block b = BLK_DOORBIRCH; b < BLK_DOORBIRCH + 16; b++) {
        tmp = getBlockInfo(b);
        tmp->onBlockInteract = &onBlockInteract_woodendoor;
        tmp->canBePlaced = &canBePlaced_door;
        tmp->onBlockUpdate = &onBlockUpdate_door;
    }
    for (block b = BLK_DOORJUNGLE; b < BLK_DOORJUNGLE + 16; b++) {
        tmp = getBlockInfo(b);
        tmp->onBlockInteract = &onBlockInteract_woodendoor;
        tmp->canBePlaced = &canBePlaced_door;
        tmp->onBlockUpdate = &onBlockUpdate_door;
    }
    for (block b = BLK_DOORACACIA; b < BLK_DOORACACIA + 16; b++) {
        tmp = getBlockInfo(b);
        tmp->onBlockInteract = &onBlockInteract_woodendoor;
        tmp->canBePlaced = &canBePlaced_door;
        tmp->onBlockUpdate = &onBlockUpdate_door;
    }
    for (block b = BLK_DOORDARKOAK; b < BLK_DOORDARKOAK + 16; b++) {
        tmp = getBlockInfo(b);
        tmp->onBlockInteract = &onBlockInteract_woodendoor;
        tmp->canBePlaced = &canBePlaced_door;
        tmp->onBlockUpdate = &onBlockUpdate_door;
    }
    for (block b = BLK_DOORIRON; b < BLK_DOORIRON + 16; b++) {
        tmp = getBlockInfo(b);
        tmp->canBePlaced = &canBePlaced_door;
        tmp->onBlockUpdate = &onBlockUpdate_door;
    }
    tmp = getBlockInfo(BLK_BED);
    tmp->canBePlaced = &canBePlaced_bed;
    tmp->onBlockUpdate = &onBlockUpdate_bed;
    getBlockInfo(BLK_REDSTONEDUST)->onBlockUpdate = &onBlockUpdate_redstonedust;
    getBlockInfo(BLK_REDSTONEDUST)->canBePlaced = &canBePlaced_redstone;
    getBlockInfo(BLK_TORCH)->canBePlaced = &canBePlaced_torch;
    getBlockInfo(BLK_TORCH)->onBlockUpdate = &onBlockUpdate_checkPlace;
    getBlockInfo(BLK_NOTGATE)->canBePlaced = &canBePlaced_torch;
    getBlockInfo(BLK_NOTGATE_1)->canBePlaced = &canBePlaced_torch;
    getBlockInfo(BLK_NOTGATE)->onBlockUpdate = &onBlockUpdate_redstonetorch;
    getBlockInfo(BLK_NOTGATE_1)->onBlockUpdate = &onBlockUpdate_redstonetorch;
    getBlockInfo(BLK_DIODE)->onBlockUpdate = &onBlockUpdate_repeater;
    getBlockInfo(BLK_DIODE_1)->onBlockUpdate = &onBlockUpdate_repeater;
    getBlockInfo(BLK_DIODE)->scheduledTick = &scheduledTick_repeater;
    getBlockInfo(BLK_DIODE_1)->scheduledTick = &scheduledTick_repeater;
    getBlockInfo(BLK_DIODE)->onBlockInteract = &onBlockInteract_repeater;
    getBlockInfo(BLK_DIODE_1)->onBlockInteract = &onBlockInteract_repeater;
    getBlockInfo(BLK_DIODE)->canBePlaced = &canBePlaced_redstone;
    getBlockInfo(BLK_DIODE_1)->canBePlaced = &canBePlaced_redstone;
    getBlockInfo(BLK_REDSTONELIGHT)->onBlockUpdate = &onBlockUpdate_lamp;
    getBlockInfo(BLK_REDSTONELIGHT_1)->onBlockUpdate = &onBlockUpdate_lamp;
    getBlockInfo(BLK_LEVER)->onBlockInteract = &onBlockInteract_lever;
    getBlockInfo(BLK_LEVER)->onBlockPlacedPlayer = &onBlockPlacedPlayer_lever;
    getBlockInfo(BLK_BUTTON)->onBlockPlacedPlayer = &onBlockPlacedPlayer_button;
    getBlockInfo(BLK_BUTTON)->onBlockInteract = &onBlockInteract_button;
    getBlockInfo(BLK_BUTTON)->scheduledTick = &scheduledTick_button;
    getBlockInfo(BLK_BUTTON_1)->onBlockPlacedPlayer = &onBlockPlacedPlayer_button;
    getBlockInfo(BLK_BUTTON_1)->onBlockInteract = &onBlockInteract_button;
    getBlockInfo(BLK_BUTTON_1)->scheduledTick = &scheduledTick_button;
    getBlockInfo(BLK_TNT)->onBlockUpdate = &onBlockUpdate_tnt;
    getBlockInfo(BLK_TRAPDOOR)->onBlockInteract = &onBlockInteract_trapdoor;
    getBlockInfo(BLK_TRAPDOOR)->onBlockUpdate = &onBlockUpdate_trapdoor;
    getBlockInfo(BLK_IRONTRAPDOOR)->onBlockUpdate = &onBlockUpdate_trapdoor;
    getBlockInfo(BLK_TRAPDOOR)->canBePlaced = &canBePlaced_trapdoor;
    getBlockInfo(BLK_IRONTRAPDOOR)->canBePlaced = &canBePlaced_trapdoor;
    getBlockInfo(BLK_TRAPDOOR)->onBlockPlacedPlayer = &onBlockPlacedPlayer_trapdoor;
    getBlockInfo(BLK_IRONTRAPDOOR)->onBlockPlacedPlayer = &onBlockPlacedPlayer_trapdoor;
//TODO: redstone torch burnout
}
