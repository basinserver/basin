//
// Created by p on 4/17/19.
//


const uint16_t generableBiomes[] = { BIOME_OCEAN, BIOME_PLAINS, BIOME_DESERT, BIOME_EXTREME_HILLS, BIOME_FOREST, BIOME_TAIGA, BIOME_SWAMPLAND };
const uint16_t generableBiomesCount = 7;

struct chunk* generateRegularChunk(struct world* world, struct chunk* chunk) {
    for (int32_t cx = 0; cx < 16; cx++) {
        for (int32_t cz = 0; cz < 16; cz++) {
            int32_t x = cx + ((int32_t) chunk->x) * 16;
            int32_t z = cz + ((int32_t) chunk->z) * 16;
            double px = ((double) x + .5);
            double py = .5 * .05;
            double pz = ((double) z + .5);
            uint16_t bi = (uint16_t)(floor((perlin_octave(&world->perlin, px, py, pz, 7., .0005, 7, .6) + 3.5)));
            if (bi < 0) bi = 0;
            if (bi >= generableBiomesCount) bi = generableBiomesCount - 1;
            uint16_t biome = generableBiomes[bi];
            chunk->biomes[cz][cx] = biome;
            double ph = 0.;
            block topSoil = BLK_GRASS;
            block subSoil = BLK_DIRT;
            block oreContainer = BLK_STONE;
            if (biome == BIOME_OCEAN) {
                topSoil = BLK_SAND;
                subSoil = BLK_SAND;
                ph = perlin_octave(&world->perlin, px, py, pz, 5., .05, 2, .25) + 2. - 32.;
            } else if (biome == BIOME_PLAINS) {
                ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            } else if (biome == BIOME_DESERT) {
                topSoil = BLK_SAND;
                subSoil = BLK_SAND;
                ph = perlin_octave(&world->perlin, px, py, pz, 3., .03, 2, .25) + 2.;
            } else if (biome == BIOME_EXTREME_HILLS) {
                ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
                double hills = perlin_octave(&world->perlin, px, py, pz, 60., .05, 2, .25) + 20.;
                ph += hills;
            } else if (biome == BIOME_FOREST) {
                ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            } else if (biome == BIOME_TAIGA) {
                ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            } else if (biome == BIOME_SWAMPLAND) {
                ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
            }
            int32_t terrainHeight = (int32_t)(ph) + 64;
            if (terrainHeight < 0) terrainHeight = 0;
            if (terrainHeight > 255) terrainHeight = 255;
            for (int32_t y = 0; y < terrainHeight; y++) {
                chunk_set_block(chunk, y < 5 ? BLK_BEDROCK : (y == (terrainHeight - 1) ? topSoil : (y >= (terrainHeight - 4) ? subSoil : oreContainer)), cx, y, cz, world->dimension == OVERWORLD);
            }
            if (biome == BIOME_OCEAN || biome == BIOME_EXTREME_HILLS) {
                for (int32_t y = terrainHeight; y < 64; y++) {
                    chunk_set_block(chunk, BLK_WATER_1, cx, y, cz, world->dimension == OVERWORLD);
                }
            }
        }
    }
    return chunk;
}

struct chunk* generateChunk(struct world* world, struct chunk* chunk) {
    memset(chunk->sections, 0, sizeof(struct chunk_section*) * 16);
    int pluginChunked = 0;
    BEGIN_HASHMAP_ITERATION (plugins)
    struct plugin* plugin = value;
    if (plugin->generateChunk != NULL) {
        struct chunk* pchunk = (*plugin->generateChunk)(world, chunk);
        if (pchunk != NULL) {
            chunk = pchunk;
            pluginChunked = 1;
            BREAK_HASHMAP_ITERATION (plugins)
            break;
        }
    }
    END_HASHMAP_ITERATION (plugins)
    if (!pluginChunked) {
        generateRegularChunk(world, chunk);
    }
}
