//
// Created by p on 4/17/19.
//

#include <basin/chunk.h>
#include <basin/tileentity.h>
#include <avuna/list.h>
#include <avuna/hash.h>
#include <avuna/string.h>
#include <math.h>


uint64_t chunk_get_key(struct chunk* ch) {
    return (uint64_t)(((int64_t) ch->x) << 32) | (((int64_t) ch->z) & 0xFFFFFFFF);
}

uint64_t chunk_get_key_direct(int32_t cx, int32_t cz) {
    return (uint64_t)(((int64_t) cx) << 32) | (((int64_t) cz) & 0xFFFFFFFF);
}


struct tile_entity* chunk_get_tile(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z) {
    if (y > 255 || y < 0 || chunk->tileEntities == NULL || x > 15 | z > 15) return NULL;
    return hashmap_getint(chunk->tileEntities, (uint64_t) x << 16 | (uint64_t) y << 8 | (uint64_t) z);
}


void chunk_set_tile(struct chunk* chunk, struct tile_entity* tile, uint8_t x, uint8_t y, uint8_t z) {
    if (y > 255 || y < 0 || chunk->tileEntities == NULL || x > 15 | z > 15) return;
    uint64_t key = (uint64_t) x << 16 | (uint64_t) y << 8 | (uint64_t) z;
    struct tile_entity* current_tile = hashmap_getint(chunk->tileEntities, key);
    if (current_tile != NULL) {
        if (current_tile->tick) {
            hashmap_putint(chunk->tileEntitiesTickable, key, tile);
        }
        pfree(current_tile->pool);
    }
    hashmap_putint(chunk->tileEntities, key, tile);
}

block chunk_get_block(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z) {
    if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
    struct chunk_section* section = chunk->sections[y >> 4];
    if (section == NULL) return 0;
    uint32_t block_index = ((y & 0x0fu) << 8u) | (z << 4u) | x;
    uint32_t bit_index = section->bits_per_block * block_index;
    uint32_t byte_index = *((uint32_t*) (&section->blocks[bit_index / 8]));
    uint32_t remainder_bits = bit_index % 8;
    block b = (block) ((byte_index >> remainder_bits) & section->block_mask);
    if (section->palette != NULL && b < section->palette_count) b = section->palette[b];
    return b;
}

uint8_t chunk_get_light(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z, uint8_t subtract) {
    if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
    struct chunk_section* section = chunk->sections[y >> 4];
    if (section == NULL) return 0;
    uint32_t block_index = ((y & 0x0fu) << 8u) | (z << 4u) | x;
    uint32_t bit_index = 4 * block_index;
    uint8_t skylight = 0;
    if (section->skyLight != NULL) {
        uint8_t this_skylight = section->skyLight[bit_index / 8];
        if (block_index % 2 == 1) {
            this_skylight &= 0xf0;
            this_skylight >>= 4;
        } else this_skylight &= 0x0f;
        skylight = this_skylight;
    }
    skylight -= subtract;
    uint8_t blocklight = section->blockLight[bit_index / 8];
    if (block_index % 2 == 1) {
        blocklight &= 0xf0;
        blocklight >>= 4;
    } else blocklight &= 0x0f;
    if (blocklight > skylight) skylight = blocklight;
    if (skylight < 0) skylight = 0;
    return skylight;
}

uint8_t chunk_get_raw_light(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z, uint8_t blocklight) {
    if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
    struct chunk_section* section = chunk->sections[y >> 4];
    if (section == NULL) return 0;
    uint32_t block_index = ((y & 0x0fu) << 8u) | (z << 4u) | x;
    uint32_t bit_index = 4 * block_index;
    uint8_t* target = blocklight ? section->blockLight : section->skyLight;
    if (target == NULL) {
        return 0;
    }
    uint8_t light = target[bit_index / 8];
    if (block_index % 2 == 1) {
        light &= 0xf0;
        light >>= 4;
    } else light &= 0x0f;
    return light;
}

void chunk_set_light(struct chunk* chunk, uint8_t light, uint8_t x, uint8_t y, uint8_t z, uint8_t blocklight, uint8_t skylight) { // skylight is only for making new chunk sections, not related to set!
    if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
    struct chunk_section* section = chunk->sections[y >> 4];
    if (section == NULL) {
        section = chunk_new_section(chunk, y >> 4, skylight);
    }
    uint32_t block_index = ((y & 0x0fu) << 8u) | (z << 4u) | x;
    uint32_t bit_index = 4 * block_index;
    uint8_t* target = blocklight ? section->blockLight : section->skyLight;
    uint8_t current_light = target[bit_index / 8];
    if (block_index % 2 == 1) {
        current_light &= 0x0f;
        current_light |= (light & 0x0f) << 4;
    } else {
        current_light &= 0xf0;
        current_light |= light & 0x0f;
    }
    target[bit_index / 8] = current_light;
}



struct chunk* chunk_new(struct mempool* pool, int16_t x, int16_t z) {
    struct chunk* chunk = pcalloc(pool, sizeof(struct chunk));
    chunk->pool = pool;
    chunk->x = x;
    chunk->z = z;
    chunk->playersLoaded = 0;
    chunk->tileEntities = hashmap_thread_new(16, 0);
    chunk->tileEntitiesTickable = hashmap_thread_new(8, 0);
    chunk->defunct = 0;
    chunk->xp = NULL;
    chunk->xn = NULL;
    chunk->zp = NULL;
    chunk->zn = NULL;
    return chunk;
}

struct chunk_section* chunk_new_section(struct chunk* chunk, int ymj, int skylight) {
    chunk->sections[ymj] = pcalloc(chunk->pool, sizeof(struct chunk_section));
    struct chunk_section* cs = chunk->sections[ymj];
    cs->bits_per_block = 4;
    cs->block_size = 512 * 4;
    cs->blocks = pcalloc(chunk->pool, 512 * 4 + 4);
    cs->palette_count = 1;
    cs->palette = pmalloc(chunk->pool, sizeof(block));
    cs->palette[0] = BLK_AIR;
    cs->block_mask = 0xf;
    if (skylight) {
        cs->skyLight = pmalloc(chunk->pool, 2048);
        memset(cs->skyLight, 0xFF, 2048);
    }
    return cs;
}

void chunk_set_block(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z, int skylight) {
    if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
    struct chunk_section* section = chunk->sections[y >> 4];
    if (section == NULL && blk != 0) {
        section = chunk_new_section(chunk, y >> 4, skylight);
    } else if (section == NULL) return;
    if (skylight) {
        struct block_info* info = getBlockInfo(blk);
        if (info != NULL) {
            if (info->lightOpacity >= 1) {
                if (chunk->heightMap[z][x] <= y) chunk->heightMap[z][x] = (uint16_t) (y + 1);
                else {
                    for (int ny = chunk->heightMap[z][x] - 1; ny >= 0; ny--) {
                        struct block_info* new_info = getBlockInfo(chunk_get_block(chunk, x, y, z));
                        if (new_info != NULL && new_info->lightOpacity >= 1) {
                            chunk->heightMap[z][x] = (uint16_t) (ny + 1);
                            break;
                        }
                    }
                }
            } else if (chunk->heightMap[z][x] > y) {
                for (int ny = chunk->heightMap[z][x] - 1; ny >= 0; ny--) {
                    struct block_info* new_info = getBlockInfo(chunk_get_block(chunk, x, y, z));
                    if (new_info != NULL && new_info->lightOpacity >= 1) {
                        chunk->heightMap[z][x] = (uint16_t) (ny + 1);
                        break;
                    }
                }
            }
        }
    }
    block wanted_block = blk;
    if (section->bits_per_block < 9) {
        for (int i = 0; i < section->palette_count; i++) {
            if (section->palette[i] == blk) {
                wanted_block = (block) i;
                goto post_palette;
            }
        }
        uint32_t room = (uint32_t) (pow(2, section->bits_per_block) - section->palette_count);
        if (room < 1) {
            uint8_t modified_bits_per_block = (uint8_t) (section->bits_per_block + 1);
            if (modified_bits_per_block >= 9) modified_bits_per_block = 13;
            uint8_t* new_blocks = pcalloc(chunk->pool, (size_t) (modified_bits_per_block * 512 + 4));
            uint32_t encoded_bit_length = 0;
            uint32_t encoded_bit_length_modified = 0;
            int32_t nmvs = section->block_mask | (1 << (modified_bits_per_block - 1));
            if (modified_bits_per_block == 13) nmvs = 0x1FFF;
            for (int i = 0; i < 4096; i++) {
                int32_t received_bytes = *((int32_t*) (&section->blocks[encoded_bit_length / 8]));
                int32_t bit_index = encoded_bit_length % 8;
                int32_t bits = (received_bytes >> bit_index) & section->block_mask;
                if (modified_bits_per_block == 13) bits = section->palette[bits];
                int32_t new_current_bytes = *((int32_t*) (&new_blocks[encoded_bit_length_modified / 8]));
                int32_t new_bit_index = encoded_bit_length_modified % 8;
                new_current_bytes = (new_current_bytes & ~(nmvs << new_bit_index)) | (bits << new_bit_index);
                *((int32_t*) &new_blocks[encoded_bit_length_modified / 8]) = new_current_bytes;
                encoded_bit_length += section->bits_per_block;
                encoded_bit_length_modified += modified_bits_per_block;
            }
            uint8_t* old_blocks = section->blocks;
            section->blocks = new_blocks;
            section->block_size = (size_t) (modified_bits_per_block * 512);
            pprefree(chunk->pool, old_blocks);
            section->block_mask = nmvs;
            section->bits_per_block = modified_bits_per_block;
        }
        wanted_block = (block) section->palette_count;
        section->palette = prealloc(chunk->pool, section->palette, sizeof(block) * (section->palette_count + 1));
        section->palette[section->palette_count++] = blk;
        post_palette: ;
    }
    uint32_t i = ((y & 0x0fu) << 8u) | (z << 4u) | x;
    uint32_t final_bit_index = section->bits_per_block * i;
    int32_t final_bits = ((int32_t) wanted_block) & section->block_mask;
    int32_t final_selected_bits = *((int32_t*) (&section->blocks[final_bit_index / 8]));
    int32_t selected_bits_index = final_bit_index % 8;
    final_selected_bits = (final_selected_bits & ~(section->block_mask << selected_bits_index)) | (final_bits << selected_bits_index);
    *((int32_t*) &section->blocks[final_bit_index / 8]) = final_selected_bits;
}
