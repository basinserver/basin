//
// Created by p on 4/17/19.
//

#include <avuna/util.h>
#include "light.h"

/*
void light_proc(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z, uint8_t light) {
    uint8_t cl = world_get_raw_light_guess(world, chunk, x, y, z, 1);
    if (cl < light) world_set_light_guess(world, chunk, (uint8_t) (light & 0x0f), x, y, z, 1);
}
*/
// a layer-buffered BFS could do this a good bit faster due to re-iteration of nodes in this DFS at the cost of more memory overhead
// `avoid` is a low overhead way to prevent direct back-recursion, but ofc is not perfect to avoid overhead of a set
void light_check_floodfill_positive(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z, uint8_t blocklight, uint8_t new_level, uint8_t avoid) {
    int current_light = world_get_raw_light_guess(world, chunk, x, y, z, blocklight);
    if (new_level <= current_light) {
        return;
    }
    block b = world_get_block_guess(world, chunk, x, y, z);
    struct block_info* info = getBlockInfo(b);
    int opacity = info == NULL ? 1 : info->lightOpacity;
    if (opacity < 1) opacity = 1;
    int opacified_level = (int) new_level - opacity;
    if (opacified_level <= current_light || opacified_level < 0) {
        return;
    }
    world_set_light_guess(world, chunk, (uint8_t) opacified_level, x, y, z, blocklight);
    if (opacified_level > 1) { // if 1, they cannot increase from 0 as 0 would be their new minimum.
        if (avoid != 1) light_check_floodfill_positive(world, chunk, x + 1, y, z, blocklight, new_level, 0);
        if (avoid != 0) light_check_floodfill_positive(world, chunk, x - 1, y, z, blocklight, new_level, 1);
        if (avoid != 3) light_check_floodfill_positive(world, chunk, x, y + 1, z, blocklight, new_level, 2);
        if (avoid != 2) light_check_floodfill_positive(world, chunk, x, y - 1, z, blocklight, new_level, 3);
        if (avoid != 5) light_check_floodfill_positive(world, chunk, x, y, z + 1, blocklight, new_level, 4);
        if (avoid != 4) light_check_floodfill_positive(world, chunk, x, y, z - 1, blocklight, new_level, 5);
    }
}

void light_update(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z, uint8_t blocklight, uint8_t new_level) {
    if (y < 0 || y > 255) return;
    uint8_t current_light = world_get_raw_light_guess(world, chunk, x, y, z, blocklight);
    world_set_light_guess(world, chunk, (uint8_t) (new_level > 15 ? 15 : new_level), x, y, z, blocklight);
    if (new_level > current_light) {
        // increase
        light_check_floodfill_positive(world, chunk, x + 1, y, z, blocklight, new_level, 0);
        light_check_floodfill_positive(world, chunk, x - 1, y, z, blocklight, new_level, 1);
        light_check_floodfill_positive(world, chunk, x, y + 1, z, blocklight, new_level, 2);
        light_check_floodfill_positive(world, chunk, x, y - 1, z, blocklight, new_level, 3);
        light_check_floodfill_positive(world, chunk, x, y, z + 1, blocklight, new_level, 4);
        light_check_floodfill_positive(world, chunk, x, y, z - 1, blocklight, new_level, 5);
    } else {

        // clear area
        // fill back in reverse
    }
}

/*
int light_floodfill(struct world* world, struct chunk* chunk, struct world_lightpos* lp, int skylight, int subtract, struct hashmap* updates) {
    if (lp->y < 0 || lp->y > 255) return 0;
    if (skylight && lp->y <= world_height_guess(world, chunk, lp->x, lp->z)) {
        struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, lp->x, lp->y + 1, lp->z));
        if (bi != NULL && bi->lightOpacity <= 16) {
            struct world_lightpos lp2;
            lp2.x = lp->x;
            lp2.y = lp->y + 1;
            lp2.z = lp->z;
            light_floodfill(world, chunk, &lp2, skylight, subtract, updates);
        }
    }
    struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, lp->x, lp->y, lp->z));
    int lo = bi == NULL ? 1 : bi->lightOpacity;
    if (lo < 1) lo = 1;
    int le = bi == NULL ? 0 : bi->lightEmission;
    int16_t maxl = 0;
    uint8_t xpl = world_get_raw_light_guess(world, chunk, lp->x + 1, lp->y, lp->z, !skylight);
    uint8_t xnl = world_get_raw_light_guess(world, chunk, lp->x - 1, lp->y, lp->z, !skylight);
    uint8_t ypl = world_get_raw_light_guess(world, chunk, lp->x, lp->y + 1, lp->z, !skylight);
    uint8_t ynl = world_get_raw_light_guess(world, chunk, lp->x, lp->y - 1, lp->z, !skylight);
    uint8_t zpl = world_get_raw_light_guess(world, chunk, lp->x, lp->y, lp->z + 1, !skylight);
    uint8_t znl = world_get_raw_light_guess(world, chunk, lp->x, lp->y, lp->z - 1, !skylight);
    if (subtract) {
        maxl = (int16_t) (world_get_raw_light_guess(world, chunk, lp->x, lp->y, lp->z, !skylight) - subtract);
    } else {
        maxl = xpl;
        if (xnl > maxl) maxl = xnl;
        if (ypl > maxl) maxl = ypl;
        if (ynl > maxl) maxl = ynl;
        if (zpl > maxl) maxl = zpl;
        if (znl > maxl) maxl = znl;
    }
    if (!subtract) maxl -= lo;
    else maxl += lo;
//printf("%s %i at %i, %i, %i\n", subtract ? "subtract" : "add", maxl, lp->x, lp->y, lp->z);
//if (maxl < 15) {
    int ks = 0;
    if ((subtract - lo) > 0) {
        subtract -= lo;
    } else {
        ks = 1;
    }
//}
    int sslf = 0;
    if (skylight) {
        int hm = world_height_guess(world, chunk, lp->x, lp->z);
        if (lp->y >= hm) {
            maxl = 15;
            if (subtract) sslf = 1;
        }
        //else maxl = -1;
    }
    if (maxl < le && !skylight) maxl = le;
    if (maxl < 0) maxl = 0;
    if (maxl > 15) maxl = 15;
    uint8_t pl = world_get_raw_light_guess(world, chunk, lp->x, lp->y, lp->z, !skylight);
    if (pl == maxl) return sslf;
    world_set_light_guess(world, chunk, maxl, lp->x, lp->y, lp->z, !skylight);
    if (ks) return 1;
    if (subtract ? (maxl < 15) : (maxl > 0)) {

        if (subtract ? xpl > maxl : xpl < maxl) {
            lp->x++;
            if (light_floodfill(world, chunk, lp, skylight, subtract, updates) && subtract) {
                lp->x--;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(updates, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->x--;
        }
        if (subtract ? xnl > maxl : xnl < maxl) {
            lp->x--;
            if (light_floodfill(world, chunk, lp, skylight, subtract, updates) && subtract) {
                lp->x++;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(updates, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->x++;
        }
        if (!skylight && (subtract ? ypl > maxl : ypl < maxl)) {
            lp->y++;
            if (light_floodfill(world, chunk, lp, skylight, subtract, updates) && subtract) {
                lp->y--;
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->y--;
        }
        if (subtract ? ynl > maxl : ynl < maxl) {
            lp->y--;
            if (light_floodfill(world, chunk, lp, skylight, subtract, updates) && subtract) {
                lp->y++;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(updates, (uint64_t) n, n);
                //light_update(world, chunk, lp, skylight, 0, 0);
            } else lp->y++;
        }
        if (subtract ? zpl > maxl : zpl < maxl) {
            lp->z++;
            if (light_floodfill(world, chunk, lp, skylight, subtract, updates) && subtract) {
                lp->z--;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(updates, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->z--;
        }
        if (subtract ? znl > maxl : znl < maxl) {
            lp->z--;
            if (light_floodfill(world, chunk, lp, skylight, subtract, updates) && subtract) {
                lp->z++;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(updates, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->z++;
        }
    }
    return sslf;
}*/