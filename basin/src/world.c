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
#include "player.h"
#include "nbt.h"
#include <linux/limits.h>
#include <fcntl.h>
#include <stdio.h>

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
	struct chunk* chunk = xmalloc(sizeof(struct chunk));
	memset(chunk, 0, sizeof(struct chunk));
	chunk->x = x;
	chunk->z = z;
	for (int i = 0; i < 16; i++)
		chunk->empty[i] = 1;
	chunk->kill = 0;
	return chunk;
}

void freeChunk(struct chunk* chunk) {
	if (chunk->skyLight != NULL) xfree(chunk->skyLight);
	xfree(chunk);
}

void setBlockChunk(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
	chunk->blocks[x][z][y] = blk;
	if (blk != 0) chunk->empty[y >> 4] = 0;
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
	struct world* world = xmalloc(sizeof(struct world));
	memset(world, 0, sizeof(struct world));
	world->chunks = new_collection(0);
	world->entities = new_collection(0);
	world->players = new_collection(0);
	return world;
}

int loadWorld(struct world* world, char* path) {
	char lp[PATH_MAX];
	snprintf(lp, PATH_MAX, "%s/level.dat", path); // could have a double slash, but its okay
	int fd = open(lp, O_RDONLY);
	if (fd < 0) return -1;
	unsigned char* ld = xmalloc(1024);
	size_t ldc = 1024;
	size_t ldi = 0;
	ssize_t i = 0;
	while ((i = read(fd, ld + ldi, ldc - ldi)) > 0) {
		if (ldc - ldi < 512) {
			ldc += 1024;
			ld = xrealloc(ld, ldc);
		}
	}
	close(fd);
	if (readNBT(&world->level, ld, ldi) < 0) return -1;
	xfree(ld);
	world->lpa = xstrdup(path);
	return 0;
}

int saveWorld(struct world* world, char* path) {

	return 0;
}

void freeWorld(struct world* world) {
	pthread_rwlock_wrlock(&world->chunks->data_mutex);
	for (size_t i = 0; i < world->chunks->size; i++) {
		if (world->chunks->data[i] != NULL) {
			freeChunk(world->chunks->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->chunks->data_mutex);
	del_collection(world->chunks);
	pthread_rwlock_wrlock(&world->entities->data_mutex);
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL) {
			freeEntity(world->entities->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->entities->data_mutex);
	del_collection(world->entities);
	pthread_rwlock_wrlock(&world->players->data_mutex);
	for (size_t i = 0; i < world->players->size; i++) {
		if (world->players->data[i] != NULL) {
			freePlayer(world->players->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->players->data_mutex);
	del_collection(world->players);
	if (world->level != NULL) {
		freeNBT(world->level);
		xfree(world->level);
	}
	if (world->lpa != NULL) xfree(world->lpa);
	xfree(world);
}

void spawnEntity(struct world* world, struct entity* entity) {
	add_collection(world->entities, entity);
}

void spawnPlayer(struct world* world, struct player* player) {
	add_collection(world->players, player);
	add_collection(world->entities, player->entity);
}

void despawnPlayer(struct world* world, struct player* player) {
	rem_collection(world->entities, player->entity);
	rem_collection(world->players, player);
}

void despawnEntity(struct world* world, struct entity* entity) {
	if (entity->type == ENT_PLAYER) despawnPlayer(world, entity->player);
	else rem_collection(world->entities, entity);
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
