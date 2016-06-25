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
		block blocks[16][16][256]; // x, z, y
		unsigned char biomes[16][16]; // x, z
		unsigned char blockLight[16][16][128]; // x, z, y(4-bit)
		unsigned char* skyLight; // if non-NULL, points to a 2048-byte nibble-array.
		int empty[16];
		int kill;
};

struct world {
		struct collection* entities;
		struct collection* players;
		struct collection* chunks;
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

void removeChunk(struct world* world, struct chunk* chunk);

int getBiome(struct world* world, int32_t x, int32_t z);

void addChunk(struct world* world, struct chunk* chunk);

block getBlockChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z);

block getBlockWorld(struct world* world, int32_t x, uint8_t y, int32_t z);

struct chunk* newChunk(int16_t x, int16_t z);

void freeChunk(struct chunk* chunk);

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
