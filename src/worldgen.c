#include <basin/worldgen.h>
#include <basin/plugin.h>
#include <avuna/pmem.h>
#include <avuna/string.h>
#include <avuna/hash.h>
#include <math.h>

const uint16_t generableBiomes[] = { BIOME_OCEAN, BIOME_PLAINS, BIOME_DESERT, BIOME_EXTREME_HILLS, BIOME_FOREST, BIOME_TAIGA, BIOME_SWAMPLAND };
const uint16_t generableBiomesCount = 7;

struct chunk* worldgen_gen_chunk_standard(struct world* world, struct chunk* chunk) {
    for (int32_t chunk_x = 0; chunk_x < 16; chunk_x++) {
        for (int32_t chunk_z = 0; chunk_z < 16; chunk_z++) {
            int32_t x = chunk_x + ((int32_t) chunk->x) * 16;
            int32_t z = chunk_z + ((int32_t) chunk->z) * 16;
            double px = ((double) x + .5);
            double py = .5 * .05;
            double pz = ((double) z + .5);
            uint16_t bi = (uint16_t)(floor((perlin_octave(&world->perlin, px, py, pz, 7., .0005, 7, .6) + 3.5)));
            if (bi < 0) bi = 0;
            if (bi >= generableBiomesCount) bi = generableBiomesCount - 1;
            uint16_t biome = generableBiomes[bi];
            chunk->biomes[chunk_z][chunk_x] = (unsigned char) biome;
            double perlin_result = 0.;
            block topSoil = BLK_GRASS;
            block subSoil = BLK_DIRT;
            block oreContainer = BLK_STONE;
            if (biome == BIOME_OCEAN) {
                topSoil = BLK_SAND;
                subSoil = BLK_SAND;
                perlin_result = perlin_octave(&world->perlin, px, py, pz, 5., .05, 2, .25) + 2. - 32.;
            } else if (biome == BIOME_PLAINS) {
                perlin_result = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            } else if (biome == BIOME_DESERT) {
                topSoil = BLK_SAND;
                subSoil = BLK_SAND;
                perlin_result = perlin_octave(&world->perlin, px, py, pz, 3., .03, 2, .25) + 2.;
            } else if (biome == BIOME_EXTREME_HILLS) {
                perlin_result = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
                double hills = perlin_octave(&world->perlin, px, py, pz, 60., .05, 2, .25) + 20.;
                perlin_result += hills;
            } else if (biome == BIOME_FOREST) {
                perlin_result = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            } else if (biome == BIOME_TAIGA) {
                perlin_result = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            } else if (biome == BIOME_SWAMPLAND) {
                perlin_result = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            }
            int32_t terrainHeight = (int32_t)(perlin_result) + 64;
            if (terrainHeight < 0) terrainHeight = 0;
            if (terrainHeight > 255) terrainHeight = 255;
            for (int32_t y = 0; y < terrainHeight; y++) {
                chunk_set_block(chunk, (block) (y < 5 ? BLK_BEDROCK : (y == (terrainHeight - 1) ? topSoil : (y >= (terrainHeight - 4) ? subSoil : oreContainer))), chunk_x, y, chunk_z, world->dimension == OVERWORLD);
            }
            if (biome == BIOME_OCEAN || biome == BIOME_EXTREME_HILLS) {
                for (int32_t y = terrainHeight; y < 64; y++) {
                    chunk_set_block(chunk, BLK_WATER_1, chunk_x, y, chunk_z, world->dimension == OVERWORLD);
                }
            }
        }
    }
    return chunk;
}

struct chunk* worldgen_gen_chunk(struct world* world, struct chunk* chunk) {
    memset(chunk->sections, 0, sizeof(struct chunk_section*) * 16);
    int pluginChunked = 0;
    ITER_MAP(plugins) {
        struct plugin* plugin = value;
        if (plugin->generateChunk != NULL) {
            struct chunk* pchunk = (*plugin->generateChunk)(world, chunk);
            if (pchunk != NULL) {
                chunk = pchunk;
                pluginChunked = 1;
                goto post_plugin_gen;
            }
        }
        ITER_MAP_END();
    }
    post_plugin_gen:;
    if (!pluginChunked) {
        worldgen_gen_chunk_standard(world, chunk);
    }
}

uint64_t get_chunk_specific_seed(uint64_t base_seed, struct chunk* chunk) {
    return base_seed ^ (uint64_t) ((chunk->x << 8) | (chunk->z << 4));
}