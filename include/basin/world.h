/*
 * world.h
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#ifndef WORLD_H_
#define WORLD_H_

#include <basin/network.h>
#include <basin/biome.h>
#include <basin/chunk.h>
#include <basin/region.h>
#include <basin/boundingbox.h>
#include <basin/block.h>
#include <basin/inventory.h>
#include <basin/perlin.h>
#include <avuna/hash.h>
#include <avuna/pmem.h>
#include <avuna/queue.h>
#include <stdint.h>
#include <pthread.h>

struct chunk_request {
		struct world* world;
		int32_t cx;
		int32_t cz;
		uint8_t load;
};

pthread_cond_t chunk_wake;
pthread_mutex_t chunk_wake_mut;

struct entity;

struct queue* globalChunkQueue;

void chunkloadthr(size_t b);

struct scheduled_tick {
		int32_t x;
		int32_t y;
		int32_t z;
		int32_t ticksLeft;
		float priority;
		block src; // TODO: this should be checked before activating!
};

struct world {
	struct mempool* pool;
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
	char* world_folder;
	size_t chl_count;
	uint8_t skylightSubtracted;
	struct hashmap* scheduledTicks;
	uint16_t ticksInSecond;
	float tps;
	uint64_t seed;
	struct perlin perlin;
};

// "*_guess" functions accept a chunk guess and call the proper function if the chunk is a miss. used to optimize chunk lookups in intensive local algorithms. probably overused.

uint16_t world_height_guess(struct world* world, struct chunk* ch, int32_t x, int32_t z);

int world_blockRayTrace(struct boundingbox* bb, int32_t x, int32_t y, int32_t z, double px, double py, double pz, double ex, double ey, double ez, double *qx, double* qy, double* qz);

int world_rayTrace(struct world* world, double x, double y, double z, double ex, double ey, double ez, int stopOnLiquid, int ignoreNonCollidable, int returnLast, double* rx, double* ry, double* rz);

void world_schedule_block_tick(struct world* world, int32_t x, int32_t y, int32_t z, int32_t ticksFromNow, float priority);

int world_set_block_noupdate(struct world* world, block blk, int32_t x, int32_t y, int32_t z);

int world_set_block_guess_noupdate(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z);

void world_tick(struct world* world);

void world_explode(struct world* world, struct chunk* ch, double x, double y, double z, float strength);

int32_t world_is_block_tick_scheduled(struct world* world, int32_t x, int32_t y, int32_t z);

int world_load(struct world* world, char* path);

int world_save(struct world* world, char* path);

// uses world coordinates << 4 (that is, chunk coordinates)
struct chunk* world_get_chunk(struct world* world, int32_t x, int32_t z);

struct chunk* world_get_chunk_guess(struct world* world, struct chunk* ch, int32_t x, int32_t z);

void world_unload_chunk(struct world* world, struct chunk* chunk);

int world_get_biome(struct world* world, int32_t x, int32_t z);

//TODO: make world_get_block and set_block autoload/unload chunks as necessary for plugins and such

block world_get_block(struct world* world, int32_t x, int32_t y, int32_t z);

block world_get_block_guess(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z);

int world_set_block(struct world* world, block blk, int32_t x, int32_t y, int32_t z);

int world_set_block_guess(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z);

struct tile_entity* world_get_tile(struct world* world, int32_t x, int32_t y, int32_t z);

void world_set_tile(struct world* world, int32_t x, int32_t y, int32_t z, struct tile_entity* te);

void world_tile_set_tickable(struct world* world, struct tile_entity* te);

void world_tile_unset_tickable(struct world* world, struct tile_entity* te);

struct world* world_new(size_t chl_count);

void world_free(struct world* world);

void world_spawn_entity(struct world* world, struct entity* entity);

void world_spawn_player(struct world* world, struct player* player);

void world_despawn_entity(struct world* world, struct entity* entity);

void world_despawn_player(struct world* world, struct player* player);

struct entity* world_get_entity(struct world* world, int32_t id);

void world_update_block(struct world* world, int32_t x, int32_t y, int32_t z);

void world_update_block_guess(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z);

uint8_t world_get_light_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z);

uint8_t world_get_light(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t checkNeighbors);

void world_set_light_guess(struct world* world, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

void world_set_light(struct world* world, struct chunk* ch, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

uint8_t world_get_raw_light_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

uint8_t world_get_raw_light(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t blocklight);

#endif /* WORLD_H_ */
