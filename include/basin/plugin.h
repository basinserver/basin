/*
 * plugin.h
 *
 *  Created on: Jan 3, 2017
 *      Author: root
 */

#ifndef PLUGIN_H_
#define PLUGIN_H_

#include <basin/world.h>
#include <basin/block.h>
#include <avuna/hash.h>
#include <stdint.h>
#include <jni.h>

struct hashmap* plugins;

#define PLUGIN_BASIN 0
#define PLUGIN_BUKKIT 1
#define PLUGIN_LUA 2

struct plugin {
	void* hnd;
	char* filename;
	void (*onWorldLoad)(struct world* world); // called at the end of loaded world
	void (*tick_global)(); // called in global tick thread
	void (*tick_world)(struct world* world); // called in the world tick thread first
	void (*tick_player)(struct world* world, struct player* player); // called at the start of a player's tick after defunct checks.
	void (*tick_entity)(struct world* world, struct entity* entity); // called at the start of an entities tick in the world thread
	void (*tick_chunk)(struct world* world, struct chunk* chunk); // called at the start of a chunks tick in the world thread
	void (*onConnect)(struct conn* conn); // called upon player connect, set conn->disconnect to disconnect.
	void (*onDisconnect)(struct conn* conn); // called upon player disconnect but before the player is defunct.
	int (*onBlockDestroyed)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replacedBy); // called before a block is destroyed.
	int (*onBlockDestroyedPlayer)(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z); // called before a block is destroyed.
	block (*onBlockPlaced)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replaced); // called when a block is placed. no block is placed if return = 0
	block (*onBlockPlacedPlayer)(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face); // called when a block is placed. no block is placed if return = 0
	void (*onBlockInteract)(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face, float curPosX, float curPosY, float curPosZ); // called when a player interacts with a block.
	void (*onBlockCollide)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct entity* entity); // called when an entity collides with a block.
	void (*onItemUse)(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, uint16_t ticks); // called during an items usage, ticks = 0 for start.
	int (*onItemInteract)(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face); // called when a player right clicks on a block
	int (*onPlayerBreakBlock)(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z); // called when a player breaks a block, set return to cancel tile break.
	float (*onEntityAttacked)(struct world* world, struct entity* attacker, uint8_t slot_index, struct slot* slot, struct entity* attacked, float damage); // called when a player attacks an entity return = damage, 0 for cancel.
	float (*onEntityDamaged)(struct world* world, struct entity* entity, float damage); // called when any entity is damaged for any reason.
	int (*onInventoryUpdate)(struct world* world, struct player* player, struct inventory* inventory, int16_t slot, struct slot* item);
	int (*onPacketReceive)(int state, struct conn* conn, struct packet* packet); // UNSAFE! breaks protocol intercompatibilty - check conn->protocol_version setting return = drop packet
	int (*onPacketSend)(int state, struct conn* conn, struct packet* packet); // UNSAFE! breaks protocol intercompatibilty - check conn->protocol_version setting return = drop packet
	void (*onEntitySpawn)(struct world* world, struct entity* entity); // called after an entity spawns
	void (*onPlayerSpawn)(struct world* world, struct player* player); // called after a player spawns
	void (*onPlayerFullySpawned)(struct world* world, struct player* player); // called after a player spawns and has moved
	int (*pre_chunk_unload)(struct world* world, struct chunk* chunk); // if 1 is returned, a chunk unload will be cancelled
	void (*post_chunk_unload)(struct world* world, int32_t x, int32_t y);
	struct chunk* (*pre_chunk_loaded)(struct world* world, int32_t x, int32_t y); // if set and returns a non-null valid chunk, will replace a loaded chunk before it is loaded
	struct chunk* (*post_chunk_loaded)(struct world* world, struct chunk* chunk); // if set and returns a non-null valid chunk, will replace a loaded chunk
	struct chunk* (*generateChunk)(struct world* world, struct chunk* chunk); // if set and returns a non-null valid chunk, will use it as a world generator
	uint8_t type;
	JavaVM* javaVM;
	JNIEnv* jniEnv;
	jclass mainClass;
};

void init_plugins();

#endif /* PLUGIN_H_ */
