//
// Created by p on 4/17/19.
//

#ifndef BASIN_WORLDGEN_H
#define BASIN_WORLDGEN_H

#include <basin/world.h>
#include <basin/chunk.h>

struct chunk* worldgen_gen_chunk_standard(struct world* world, struct chunk* chunk);

struct chunk* worldgen_gen_chunk(struct world* world, struct chunk* chunk);

#endif //BASIN_WORLDGEN_H
