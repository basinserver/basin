#ifndef BASIN_LIGHT_H
#define BASIN_LIGHT_H


#include <basin/world.h>

/*
struct world_lightpos {
    int32_t x;
    int32_t y;
    int32_t z;
};
*/
//void light_proc(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z, uint8_t light);

//int light_floodfill(struct world* world, struct chunk* chunk, struct world_lightpos* lp, int skylight, int subtract, struct hashmap* updates);

void light_update(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z, uint8_t blocklight, uint8_t new_level);

#endif //BASIN_LIGHT_H
