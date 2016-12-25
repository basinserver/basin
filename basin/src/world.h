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

uint32_t nextEntityID;

struct boundingbox {
		double minX;
		double maxX;
		double minY;
		double maxY;
		double minZ;
		double maxZ;
};

typedef uint16_t block;

struct chunk {
		int16_t x;
		int16_t z;
		block blocks[256][16][16]; // y, z, x
		unsigned char biomes[16][16]; // z, x
		unsigned char blockLight[256][16][8]; // y, z, x(4-bit)
		unsigned char* skyLight; // if non-NULL, points to a 2048-byte nibble-array.
		int empty[16];
		int lightpopulated;
		int terrainpopulated;
		uint64_t inhabitedticks;
		uint16_t heightMap[16][16]; // z, x
		size_t tileEntity_count;
		struct nbt_tag** tileEntities;
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
		char* levelType;
		struct encpos spawnpos;
		int32_t dimension;
		uint64_t time;
		uint64_t age;
		struct nbt_tag* level;
		char* lpa;
};

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

struct world* newWorld();

void freeWorld(struct world* world);

void spawnEntity(struct world* world, struct entity* entity);

void spawnPlayer(struct world* world, struct player* player);

void despawnEntity(struct world* world, struct entity* entity);

void despawnPlayer(struct world* world, struct player* player);

struct entity* getEntity(struct world* world, int32_t id);

#endif /* WORLD_H_ */
