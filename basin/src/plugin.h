/*
 * plugin.h
 *
 *  Created on: Jan 3, 2017
 *      Author: root
 */

#ifndef PLUGIN_H_
#define PLUGIN_H_

#include "hashmap.h"
#include "world.h"
#include <stdint.h>
#include "block.h"

struct hashmap* plugins;

struct plugin {
		void* hnd;
		char* filename;
		void (*tick_global)(); // called in global tick thread
		void (*tick_world)(struct world* world); // called in the world tick thread first
		void (*tick_playerthread)(struct world* world, struct subworld* subworld); // called in a subworld thread after packets are received, but before anything else.
		void (*tick_player)(struct world* world, struct player* player); // called at the start of a player's tick after defunct checks.
		void (*tick_entity)(struct world* world, struct entity* entity); // called at the start of an entities tick in the world thread
		void (*tick_chunk)(struct world* world, struct chunk* chunk); // called at the start of a chunks tick in the world thread
		void (*onConnect)(struct conn* conn); // called upon player connect, set conn->disconnect to disconnect.
		void (*onDisconnect)(struct conn* conn); // called upon player disconnect but before the player is defunct.
		int (*onBlockDestroyed)(struct world* world, block blk, int32_t x, int32_t y, int32_t z); // called before a block is destroyed.
		block (*onBlockPlaced)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face); // called when a block is placed. no block is placed if return = 0
		void (*onBlockInteract)(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face, float curPosX, float curPosY, float curPosZ); // called when a player interacts with a block.
		void (*onBlockCollide)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct entity* entity); // called when an entity collides with a block.
		void (*onItemUse)(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, uint16_t ticks); // called during an items usage, ticks = 0 for start.
		int (*onItemInteract)(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face); // called when a player right clicks on a block
		int (*onPlayerBreakBlock)(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z); // called when a player breaks a block, set return to cancel te break.
		float (*onEntityAttackedByPlayer)(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, struct entity* entity); // called when a player attacks an entity return = damage, 0 for cancel.
		float (*onEntityDamaged)(struct world* world, struct entity* entity, float damage); // called when any entity is damaged for any reason.
		int (*onInventoryUpdate)(struct world* world, struct player* player, struct inventory* inventory, int16_t slot, struct slot* item);
		int (*onPacketReceive)(int state, struct conn* conn, struct packet* packet); // UNSAFE! breaks protocol intercompatibilty - check conn->protocolVersion setting return = drop packet
		int (*onPacketSend)(int state, struct conn* conn, struct packet* packet); // UNSAFE! breaks protocol intercompatibilty - check conn->protocolVersion setting return = drop packet
		void (*onEntitySpawn)(struct world* world, struct entity* entity); // called after an entity spawns
		void (*onPlayerSpawn)(struct world* world, struct player* player); // called after a player spawns
		void (*onPlayerFullySpawned)(struct world* world, struct player* player); // called after a player spawns and has moved
};

void init_plugins();

#endif /* PLUGIN_H_ */
