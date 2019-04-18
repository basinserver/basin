//
// Created by p on 4/17/19.
//


void world_doLightProc(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z, uint8_t light) {
    uint8_t cl = world_get_raw_light_guess(world, chunk, x, y, z, 1);
    if (cl < light) world_set_light(world, chunk, (uint8_t) (light & 0x0f), x, y, z, 1);
}

struct world_lightpos {
    int32_t x;
    int32_t y;
    int32_t z;
};

int light_floodfill(struct world* world, struct chunk* chunk, struct world_lightpos* lp, int skylight, int subtract, struct hashmap* subt_upd) {
    if (lp->y < 0 || lp->y > 255) return 0;
    if (skylight && lp->y <= world_height_guess(world, chunk, lp->x, lp->z)) {
        struct block_info* bi = getBlockInfo(world_get_block_guess(world, chunk, lp->x, lp->y + 1, lp->z));
        if (bi != NULL && bi->lightOpacity <= 16) {
            struct world_lightpos lp2;
            lp2.x = lp->x;
            lp2.y = lp->y + 1;
            lp2.z = lp->z;
            light_floodfill(world, chunk, &lp2, skylight, subtract, subt_upd);
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
    world_set_light(world, chunk, maxl, lp->x, lp->y, lp->z, !skylight);
    if (ks) return 1;
    if (subtract ? (maxl < 15) : (maxl > 0)) {

        if (subtract ? xpl > maxl : xpl < maxl) {
            lp->x++;
            if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
                lp->x--;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(subt_upd, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->x--;
        }
        if (subtract ? xnl > maxl : xnl < maxl) {
            lp->x--;
            if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
                lp->x++;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(subt_upd, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->x++;
        }
        if (!skylight && (subtract ? ypl > maxl : ypl < maxl)) {
            lp->y++;
            if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
                lp->y--;
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->y--;
        }
        if (subtract ? ynl > maxl : ynl < maxl) {
            lp->y--;
            if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
                lp->y++;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(subt_upd, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->y++;
        }
        if (subtract ? zpl > maxl : zpl < maxl) {
            lp->z++;
            if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
                lp->z--;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(subt_upd, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->z--;
        }
        if (subtract ? znl > maxl : znl < maxl) {
            lp->z--;
            if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
                lp->z++;
                void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
                put_hashmap(subt_upd, (uint64_t) n, n);
                //light_floodfill(world, chunk, lp, skylight, 0, 0);
            } else lp->z++;
        }
    }
    return sslf;
}