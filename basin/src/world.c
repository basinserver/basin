/*
 * world.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */
#include "world.h"
#include "entity.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "collection.h"
#include <pthread.h>

struct chunk* getChunk(struct world* world, int16_t x, int16_t z) {
	pthread_rwlock_rdlock(&world->chunks->data_mutex);
	for (size_t i = 0; i < world->chunks->size; i++) {
		if (world->chunks->data[i] != NULL && ((struct chunk*) world->chunks->data[i])->x == x && ((struct chunk*) world->chunks->data[i])->z == z) {
			return world->chunks->data[i];
		}
	}
	pthread_rwlock_unlock(&world->chunks->data_mutex);
	return NULL;
}

void removeChunk(struct world* world, struct chunk* chunk) {
	chunk->kill = 1;
}

int getBiome(struct world* world, int32_t x, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 0;
	return chunk->biomes[z & 0x0f][x & 0x0f];
}

void addChunk(struct world* world, struct chunk* chunk) {
	add_collection(world->chunks, chunk);
}

block getBlockChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
	return chunk->blocks[x][z][y];
}

block getBlockWorld(struct world* world, int32_t x, uint8_t y, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 0;
	return getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
}

struct chunk* newChunk(int16_t x, int16_t z) {
	struct chunk* chunk = malloc(sizeof(struct chunk));
	memset(chunk, 0, sizeof(struct chunk));
	chunk->x = x;
	chunk->z = z;
	chunk->empty = 0;
	chunk->kill = 0;
	return chunk;
}

void freeChunk(struct chunk* chunk) {
	if (chunk->skyLight != NULL) free(chunk->skyLight);
	free(chunk);
}

void setBlockChunk(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
	chunk->blocks[x][z][y] = blk;
}

void setBlockWorld(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) {
		chunk = newChunk(x >> 4, z >> 4);
		addChunk(world, chunk);
	}
	setBlockChunk(chunk, blk, x & 0x0f, y, z & 0x0f);
}

struct world* newWorld() {
	struct world* world = malloc(sizeof(struct world));
	memset(world, 0, sizeof(struct world));
	world->chunks = new_collection(0);
	world->entities = new_collection(0);
	return world;
}

void freeWorld(struct world* world) {
	pthread_rwlock_wrlock(&world->chunks->data_mutex);
	for (size_t i = 0; i < world->chunks->size; i++) {
		if (world->chunks->data[i] != NULL) {
			freeChunk(world->chunks->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->chunks->data_mutex);
	pthread_rwlock_wrlock(&world->entities->data_mutex);
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL) {
			freeEntity(world->entities->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->entities->data_mutex);
	free(world);
}

void spawnEntity(struct world* world, struct entity* entity) {
	add_collection(world->entities, entity);
}

struct entity* despawnEntity(struct world* world, int32_t id) {
	pthread_rwlock_rdlock(&world->entities->data_mutex);
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL && ((struct entity*) world->entities->data[i])->id == id) {
			struct entity* ent = world->entities->data[i];
			world->entities->data[i] = NULL;
			return ent;
		}
	}
	pthread_rwlock_unlock(&world->entities->data_mutex);
	return NULL;
}

struct entity* getEntity(struct world* world, int32_t id) {
	pthread_rwlock_rdlock(&world->entities->data_mutex);
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL && ((struct entity*) world->entities->data[i])->id == id) {
			return world->entities->data[i];
		}
	}
	pthread_rwlock_unlock(&world->entities->data_mutex);
	return NULL;
}
