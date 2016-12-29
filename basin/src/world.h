/*
 * world.h
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#ifndef WORLD_H_
#define WORLD_H_

#include "entity.h"
#include <stdlib.h>
#include "network.h"
#include "collection.h"
#include "player.h"
#include <pthread.h>
#include "tileentity.h"

uint32_t nextEntityID;

struct boundingbox {
		double minX;
		double maxX;
		double minY;
		double maxY;
		double minZ;
		double maxZ;
};

int boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2);

typedef uint16_t block;

struct chunk_section {
		uint8_t* blocks; // y, z, x
		size_t block_size;
		size_t palette_count;
		block* palette;
		uint8_t bpb;
		unsigned char blockLight[16][16][8]; // y, z, x(4-bit)
		unsigned char* skyLight; // if non-NULL, points to a 2048-byte nibble-array.
		int32_t mvs;
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
		//TODO: tileTicks
};

struct region {
		int16_t x;
		int16_t z;
		struct chunk* chunks[32][32];
		uint8_t loaded[32][32];
		int fd; // -1 if not loaded, >= 0 is the FD.
		void* mapping; // NULL if not loaded
		char* file;
};

struct world {
		struct collection* entities;
		struct collection* players;
		struct collection* regions;
		struct collection* loadedChunks;
		char* levelType;
		struct encpos spawnpos;
		int32_t dimension;
		uint64_t time;
		uint64_t age;
		struct nbt_tag* level;
		char* lpa;
		pthread_rwlock_t chl;
};

int isChunkLoaded(struct world* world, int16_t x, int16_t z);

int loadWorld(struct world* world, char* path);

int saveWorld(struct world* world, char* path);

struct chunk* getChunk(struct world* world, int16_t x, int16_t z);

void unloadChunk(struct world* world, struct chunk* chunk);

int getBiome(struct world* world, int32_t x, int32_t z);

block getBlockChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z);

block getBlockWorld(struct world* world, int32_t x, uint8_t y, int32_t z);

struct chunk* newChunk(int16_t x, int16_t z);

void freeChunk(struct chunk* chunk);

struct region* newRegion(char* path, int16_t x, int16_t z);

void freeRegion(struct region* region);

void setBlockChunk(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z);

void setBlockWorld(struct world* world, block blk, int32_t x, int32_t y, int32_t z);

struct tile_entity* getTileEntityChunk(struct chunk* chunk, int32_t x, uint8_t y, int32_t z);

void setTileEntityChunk(struct chunk* chunk, struct tile_entity* te);

struct tile_entity* getTileEntityWorld(struct world* world, int32_t x, uint8_t y, int32_t z);

void setTileEntityWorld(struct world* world, int32_t x, uint8_t y, int32_t z, struct tile_entity* te);

void enableTileEntityTickable(struct world* world, struct tile_entity* te);

void disableTileEntityTickable(struct world* world, struct tile_entity* te);

struct world* newWorld();

void freeWorld(struct world* world);

void spawnEntity(struct world* world, struct entity* entity);

void spawnPlayer(struct world* world, struct player* player);

void despawnEntity(struct world* world, struct entity* entity);

void despawnPlayer(struct world* world, struct player* player);

struct entity* getEntity(struct world* world, int32_t id);

void onBlockDestroyed(struct world* world, int32_t x, int32_t y, int32_t z);

void onBlockPlaced(struct world* world, int32_t x, int32_t y, int32_t z);

void onBlockInteract(struct world* world, int32_t x, int32_t y, int32_t z, struct player* player);

void onBlockCollide(struct world* world, int32_t x, int32_t y, int32_t z, struct entity* entity);

void onBlockUpdate(struct world* world, int32_t x, int32_t y, int32_t z);

#endif /* WORLD_H_ */
