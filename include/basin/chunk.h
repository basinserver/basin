//
// Created by p on 4/17/19.
//

#ifndef BASIN_CHUNK_H
#define BASIN_CHUNK_H

#include <basin/block.h>

struct chunk_section {
    uint8_t* blocks; // y, z, x
    size_t block_size;
    size_t palette_count;
    block* palette;
    uint8_t bpb;
    unsigned char blockLight[2048]; // y, z, x(4-bit)
    unsigned char* skyLight; // if non-NULL, points to a 2048-byte nibble-array.
    int32_t block_mask;
    uint16_t randomTickables;
};

struct chunk {
    struct mempool* pool;
    int16_t x;
    int16_t z;
    struct chunk_section* sections[16];
    unsigned char biomes[16][16]; // z, x
    int lightpopulated;
    int terrainpopulated;
    uint64_t inhabitedticks;
    uint16_t heightMap[16][16]; // z, x
    struct collection* tileEntities;
    struct collection* tileEntitiesTickable;
    uint32_t playersLoaded;
    uint8_t defunct;
    struct chunk* xp;
    struct chunk* xn;
    struct chunk* zp;
    struct chunk* zn;
    //struct hashmap* entities;
    //TODO: tileTicks
};


uint64_t chunk_get_key(struct chunk* ch);

uint64_t chunk_get_key_direct(int32_t cx, int32_t cz);

struct chunk_section* chunk_new_section(struct chunk* chunk, int ymj, int skylight);

struct chunk* chunk_get_entity_chunk(struct entity* entity);

block chunk_get_block(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z);

void chunk_set_block(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z, int skylight);

struct chunk* chunk_new(struct mempool* pool, int16_t x, int16_t z);

void chunk_free(struct chunk* chunk);

struct tile_entity* chunk_get_tile(struct chunk* chunk, int32_t x, int32_t y, int32_t z);

void chunk_set_tile(struct chunk* chunk, struct tile_entity* te, int32_t x, uint8_t y, int32_t z);

uint8_t chunk_get_light(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z, uint8_t subtract);

uint8_t chunk_get_raw_light(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z, uint8_t blocklight);

void chunk_set_light(struct chunk* chunk, uint8_t light, uint8_t x, uint8_t y, uint8_t z, uint8_t blocklight, uint8_t skylight);


#endif //BASIN_CHUNK_H
