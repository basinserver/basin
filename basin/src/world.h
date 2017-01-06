/*
 * world.h
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#ifndef WORLD_H_
#define WORLD_H_

#include <stdint.h>
#include "network.h"
#include "inventory.h"
#include <pthread.h>

typedef uint16_t block;

#define BIOME_OCEAN 0
#define BIOME_PLAINS 1
#define BIOME_DESERT 2
#define BIOME_EXTREMEHILLS 3
#define BIOME_FOREST 4
#define BIOME_TAIGA 5
#define BIOME_SWAMPLAND 6
#define BIOME_RIVER 7
#define BIOME_HELL 8
#define BIOME_THEEND 9
#define BIOME_FROZENOCEAN 10
#define BIOME_FROZENRIVER 11
#define BIOME_ICEPLAINS 12
#define BIOME_ICEMOUNTAINS 13
#define BIOME_MUSHROOMISLAND 14
#define BIOME_MUSHROOMISLANDSHORE 15
#define BIOME_BEACH 16
#define BIOME_DESERTHILLS 17
#define BIOME_FORESTHILLS 18
#define BIOME_TAIGAHILLS 19
#define BIOME_EXTREMEHILLSEDGE 20
#define BIOME_JUNGLE 21
#define BIOME_JUNGLEHILLS 22
#define BIOME_JUNGLEEDGE 23
#define BIOME_DEEPOCEAN 24
#define BIOME_STONEBEACH 25
#define BIOME_COLDBEACH 26
#define BIOME_BIRCHFOREST 27
#define BIOME_BIRCHFORESTHILLS 28
#define BIOME_ROOFEDFOREST 29
#define BIOME_COLDTAIGA 30
#define BIOME_COLDTAIGAHILLS 31
#define BIOME_MEGATAIGA 32
#define BIOME_MEGATAIGAHILLS 33
#define BIOME_EXTREMEHILLSPLUS 34
#define BIOME_SAVANNA 35
#define BIOME_SAVANNAPLATEAU 36
#define BIOME_MESA 37
#define BIOME_MESAPLATEAUF 38
#define BIOME_MESAPLATEAU 39
#define BIOME_VOID 127
//begin variation
#define BIOME_PLAINSM 128
#define BIOME_SUNFLOWERPLAINS 129
#define BIOME_DESERTM 130
#define BIOME_EXTREMEHILLSM 131
#define BIOME_FLOWERFOREST 132
#define BIOME_TAIGAM 133
#define BIOME_SWAMPLANDM 134
#define BIOME_ICEPLAINSSPIKES 140
#define BIOME_JUNGLEM 149
#define BIOME_JUNGLEEDGEM 151
#define BIOME_BIRCHFORESTM 155
#define BIOME_BIRCHFORESTHILLSM 156
#define BIOME_ROOFEDFORESTM 157
#define BIOME_COLDTAIGAM 158
#define BIOME_MEGASPRUCETAIGA 160
#define BIOME_REDWOODTAIGAHILLSM 161
#define BIOME_EXTREMEHILLSPLUSM 162
#define BIOME_SAVANNAM 163
#define BIOME_SAVANNAPLEATEAUM 164
#define BIOME_MESABRYCE 165
#define BIOME_MESAPLATEAUFM 166
#define BIOME_MESAPLATEAUM 167

uint32_t nextEntityID;

struct chunk_req {
		struct player* pl;
		int32_t cx;
		int32_t cz;
		uint8_t load;
};

struct queue* chunk_input;

pthread_cond_t chunk_wake;
pthread_mutex_t chunk_wake_mut;

struct entity;

struct boundingbox {
		double minX;
		double maxX;
		double minY;
		double maxY;
		double minZ;
		double maxZ;
};

int boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2);

struct chunk_section {
		uint8_t* blocks; // y, z, x
		size_t block_size;
		size_t palette_count;
		block* palette;
		uint8_t bpb;
		unsigned char blockLight[2048]; // y, z, x(4-bit)
		unsigned char* skyLight; // if non-NULL, points to a 2048-byte nibble-array.
		int32_t mvs;
		uint16_t randomTickables;
};

struct chunk {
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

void chunkloadthr(size_t b);

uint64_t getChunkKey(struct chunk* ch);

uint64_t getChunkKey2(int32_t cx, int32_t cz);

struct region {
		int16_t x;
		int16_t z;
		int* fd; // -1 if not loaded, >= 0 is the FD.
		void** mapping; // NULL if not loaded
		char* file;
};

struct scheduled_tick {
		int32_t x;
		int32_t y;
		int32_t z;
		int32_t ticksLeft;
};

struct world {
		struct hashmap* entities;
		struct hashmap* players;
		struct hashmap* regions;
		struct hashmap* chunks;
		char* levelType;
		struct encpos spawnpos;
		int32_t dimension;
		uint64_t time;
		uint64_t age;
		struct nbt_tag* level;
		char* lpa;
		pthread_mutex_t tick_mut;
		pthread_cond_t tick_cond;
		size_t chl_count;
		struct hashmap* subworlds;
		uint8_t skylightSubtracted;
		struct hashmap* scheduledTicks;
};

struct subworld { // subworld for players thread
		struct world* world;
		struct hashmap* players;
		uint8_t defunct;
};

struct chunk_section* newChunkSection(struct chunk* chunk, int ymj, int skylight);

void scheduleBlockTick(struct world* world, int32_t x, int32_t y, int32_t z, int32_t ticksFromNow);

void setBlockWorld_noupdate(struct world* world, block blk, int32_t x, int32_t y, int32_t z);

void setBlockWorld_guess_noupdate(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z);

void tick_world(struct world* world);

struct chunk* getEntityChunk(struct entity* entity);

int isChunkLoaded(struct world* world, int32_t x, int32_t z);

int loadWorld(struct world* world, char* path);

int saveWorld(struct world* world, char* path);

struct chunk* getChunk(struct world* world, int32_t x, int32_t z);

void unloadChunk(struct world* world, struct chunk* chunk);

int getBiome(struct world* world, int32_t x, int32_t z);

block getBlockChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z);

block getBlockWorld(struct world* world, int32_t x, int32_t y, int32_t z);

block getBlockWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z);

struct chunk* newChunk(int16_t x, int16_t z);

void freeChunk(struct chunk* chunk);

struct region* newRegion(char* path, int16_t x, int16_t z, size_t chr_count);

void freeRegion(struct region* region);

void setBlockChunk(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z, int skylight);

void setBlockWorld(struct world* world, block blk, int32_t x, int32_t y, int32_t z);

void setBlockWorld_guess(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z);

struct tile_entity* getTileEntityChunk(struct chunk* chunk, int32_t x, int32_t y, int32_t z);

void setTileEntityChunk(struct chunk* chunk, struct tile_entity* te);

struct tile_entity* getTileEntityWorld(struct world* world, int32_t x, int32_t y, int32_t z);

void setTileEntityWorld(struct world* world, int32_t x, int32_t y, int32_t z, struct tile_entity* te);

void enableTileEntityTickable(struct world* world, struct tile_entity* te);

void disableTileEntityTickable(struct world* world, struct tile_entity* te);

struct world* newWorld(size_t chl_count);

void freeWorld(struct world* world);

void spawnEntity(struct world* world, struct entity* entity);

void spawnPlayer(struct world* world, struct player* player);

void despawnEntity(struct world* world, struct entity* entity);

void despawnPlayer(struct world* world, struct player* player);

struct entity* getEntity(struct world* world, int32_t id);

void updateBlockWorld(struct world* world, int32_t x, int32_t y, int32_t z);

void updateBlockWorld_guess(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z);

uint8_t getLightChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z, uint8_t subt);

uint8_t getLightWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z);

uint8_t getLightWorld(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t checkNeighbors);

void setLightWorld(struct world* world, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

void setLightWorld_guess(struct world* world, struct chunk* ch, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

uint8_t getRawLightWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

uint8_t getRawLightWorld(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

#endif /* WORLD_H_ */
