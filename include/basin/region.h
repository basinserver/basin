//
// Created by p on 4/17/19.
//

#ifndef BASIN_REGION_H
#define BASIN_REGION_H

#include <basin/chunk.h>
#include <stdint.h>
#include <stdlib.h>

struct region {
    struct mempool* pool;
    int16_t x;
    int16_t z;
    int fd; // -1 if not loaded, >= 0 is the FD.
    void* mapping; // NULL if not loaded
    char* file;
};

struct region* region_new(char* path, int16_t x, int16_t z);

// WARNING: do not use this function for casual reference to chunks, as region does not track loaded chunks, it only loads them

// use world functions to load chunks

struct chunk* region_load_chunk(struct region* region, int8_t local_chunk_x, int8_t local_chunk_z);

#endif //BASIN_REGION_H
