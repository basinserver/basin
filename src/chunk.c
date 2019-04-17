//
// Created by p on 4/17/19.
//

#include <basin/chunk.h>
#include <basin/tileentity.h>


uint64_t chunk_get_key(struct chunk* ch) {
    return (uint64_t)(((int64_t) ch->x) << 32) | (((int64_t) ch->z) & 0xFFFFFFFF);
}

uint64_t chunk_get_key_direct(int32_t cx, int32_t cz) {
    return (uint64_t)(((int64_t) cx) << 32) | (((int64_t) cz) & 0xFFFFFFFF);
}


struct tile_entity* chunk_get_tile(struct chunk* chunk, int32_t x, int32_t y, int32_t z) { // TODO: optimize
    if (y > 255 || y < 0) return NULL;
    for (size_t i = 0; i < chunk->tileEntities->size; i++) {
        struct tile_entity* tile = (struct tile_entity*) chunk->tileEntities->data[i];
        if (tile == NULL) continue;
        if (tile->x == x && tile->y == y && tile->z == z) return tile;
    }
    return NULL;
}


block chunk_get_block(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z) {
    if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
    struct chunk_section* section = chunk->sections[y >> 4];
    if (section == NULL) return 0;
    uint32_t block_index = ((y & 0x0fu) << 8u) | (z << 4u) | x;
    uint32_t bit_index = section->bpb * block_index;
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


void chunk_set_tile(struct chunk* chunk, struct tile_entity* te, int32_t x, uint8_t y, int32_t z) {
    for (size_t i = 0; i < chunk->tileEntities->size; i++) {
        struct tile_entity* te2 = (struct tile_entity*) chunk->tileEntities->data[i];
        if (te2 == NULL) continue;
        if (te2->x == x && te2->y == y && te2->z == z) {
            if (te2->tick) rem_collection(chunk->tileEntitiesTickable, te2);
            freeTileEntity(te2);
            chunk->tileEntities->data[i] = te;
            if (te == NULL) {
                chunk->tileEntities->count--;
                if (i == chunk->tileEntities->size - 1) chunk->tileEntities->size--;
            } else if (te->tick) add_collection(chunk->tileEntitiesTickable, te);
            return;
        }
    }
    add_collection(chunk->tileEntities, te);
    if (te->tick) add_collection(chunk->tileEntitiesTickable, te);
}


struct chunk* chunk_new(struct mempool* pool, int16_t x, int16_t z) {
    struct chunk* chunk = xcalloc(sizeof(struct chunk));
    memset(chunk, 0, sizeof(struct chunk));
    chunk->pool = pool;
    chunk->x = x;
    chunk->z = z;
    memset(chunk->sections, 0, sizeof(struct chunk_section*) * 16);
    chunk->playersLoaded = 0;
    chunk->tileEntities = new_collection(0, 0);
    chunk->tileEntitiesTickable = new_collection(0, 0);
    chunk->defunct = 0;
    chunk->xp = NULL;
    chunk->xn = NULL;
    chunk->zp = NULL;
    chunk->zn = NULL;
//chunk->entities = new_hashmap(1, 0);
    return chunk;
}

void chunk_free(struct chunk* chunk) {
    for (int i = 0; i < 16; i++) {
        struct chunk_section* cs = chunk->sections[i];
        if (cs == NULL) continue;
        if (cs->blocks != NULL) xfree(cs->blocks);
        if (cs->palette != NULL) xfree(cs->palette);
        if (cs->skyLight != NULL) xfree(cs->skyLight);
        xfree(cs);
    }
    for (size_t i = 0; i < chunk->tileEntities->size; i++) {
        if (chunk->tileEntities->data[i] != NULL) {
            freeTileEntity(chunk->tileEntities->data[i]);
        }
    }
    del_collection(chunk->tileEntities);
    del_collection(chunk->tileEntitiesTickable);
//BEGIN_HASHMAP_ITERATION(chunk->entities)
//freeEntity (value);
//END_HASHMAP_ITERATION(chunk->entities)
//del_hashmap(chunk->entities);
    xfree(chunk);
}


struct chunk_section* chunk_new_section(struct chunk* chunk, int ymj, int skylight) {
    chunk->sections[ymj] = xcalloc(sizeof(struct chunk_section));
    struct chunk_section* cs = chunk->sections[ymj];
    cs->bpb = 4;
    cs->block_size = 512 * 4;
    cs->blocks = xcalloc(512 * 4 + 4);
    cs->palette_count = 1;
    cs->palette = xmalloc(sizeof(block));
    cs->palette[0] = BLK_AIR;
    cs->block_mask = 0xf;
    memset(cs->blockLight, 0, 2048);
    if (skylight) {
        cs->skyLight = xmalloc(2048);
        memset(cs->skyLight, 0xFF, 2048);
    }
    return cs;
}

void chunk_set_block(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z, int skylight) {
    if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
    struct chunk_section* cs = chunk->sections[y >> 4];
    if (cs == NULL && blk != 0) {
        cs = chunk_new_section(chunk, y >> 4, skylight);
    } else if (cs == NULL) return;
    if (skylight) {
        struct block_info* bii = getBlockInfo(blk);
        if (bii != NULL) {
            if (bii->lightOpacity >= 1) {
                if (chunk->heightMap[z][x] <= y) chunk->heightMap[z][x] = y + 1;
                else {
                    for (int ny = chunk->heightMap[z][x] - 1; ny >= 0; ny--) {
                        struct block_info* nbi = getBlockInfo(chunk_get_block(chunk, x, y, z));
                        if (nbi != NULL && nbi->lightOpacity >= 1) {
                            chunk->heightMap[z][x] = ny + 1;
                            break;
                        }
                    }
                }
            } else if (chunk->heightMap[z][x] > y) {
                for (int ny = chunk->heightMap[z][x] - 1; ny >= 0; ny--) {
                    struct block_info* nbi = getBlockInfo(chunk_get_block(chunk, x, y, z));
                    if (nbi != NULL && nbi->lightOpacity >= 1) {
                        chunk->heightMap[z][x] = ny + 1;
                        break;
                    }
                }
            }
        }
    }
    block ts = blk;
    if (cs->bpb < 9) {
        for (int i = 0; i < cs->palette_count; i++) {
            if (cs->palette[i] == blk) {
                ts = i;
                goto pp;
            }
        }
        uint32_t room = pow(2, cs->bpb) - cs->palette_count;
        if (room < 1) {
            uint8_t nbpb = cs->bpb + 1;
            if (nbpb >= 9) nbpb = 13;
            uint8_t* ndata = xcalloc(nbpb * 512 + 4);
            uint32_t bir = 0;
            uint32_t biw = 0;
            int32_t nmvs = cs->block_mask | (1 << (nbpb - 1));
            if (nbpb == 13) nmvs = 0x1FFF;
            for (int i = 0; i < 4096; i++) {
                int32_t rcv = *((int32_t*) (&cs->blocks[bir / 8]));
                int32_t rsbi = bir % 8;
                int32_t b = (rcv >> rsbi) & cs->block_mask;
                if (nbpb == 13) b = cs->palette[b];
                int32_t wcv = *((int32_t*) (&ndata[biw / 8]));
                int32_t wsbi = biw % 8;
                wcv = (wcv & ~(nmvs << wsbi)) | (b << wsbi);
                *((int32_t*) &ndata[biw / 8]) = wcv;
                bir += cs->bpb;
                biw += nbpb;
            }
            uint8_t* odata = cs->blocks;
            cs->blocks = ndata;
            cs->block_size = nbpb * 512;
            xfree(odata);
            cs->block_mask = nmvs;
            cs->bpb = nbpb;
        }
        ts = cs->palette_count;
        cs->palette = xrealloc(cs->palette, sizeof(block) * (cs->palette_count + 1));
        cs->palette[cs->palette_count++] = blk;
        pp: ;
    }
    uint32_t i = ((y & 0x0f) << 8) | (z << 4) | x;
    uint32_t bi = cs->bpb * i;
    int32_t b = ((int32_t) ts) & cs->block_mask;
    int32_t cv = *((int32_t*) (&cs->blocks[bi / 8]));
    int32_t sbi = bi % 8;
    cv = (cv & ~(cs->block_mask << sbi)) | (b << sbi);
    *((int32_t*) &cs->blocks[bi / 8]) = cv;
}

int world_set_block(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 1;
    struct chunk* ch = world_get_chunk(world, x >> 4, z >> 4);
    if (ch == NULL) return 1;
    return world_set_block_guess(world, ch, blk, x, y, z);
}

int world_set_block_noupdate(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
    if (y < 0 || y > 255) return 1;
    struct chunk* ch = world_get_chunk(world, x >> 4, z >> 4);
    if (ch == NULL) return 1;
    return world_set_block_guess_noupdate(world, ch, blk, x, y, z);
}
