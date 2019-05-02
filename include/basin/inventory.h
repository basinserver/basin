/*
 * inventory.h
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#ifndef BASIN_INVENTORY_H_
#define BASIN_INVENTORY_H_

#include <basin/player.h>
#include <basin/nbt.h>
#include <avuna/hash.h>
#include <stdint.h>
#include <pthread.h>

#define INVTYPE_PLAYERINVENTORY 0
#define INVTYPE_CHEST 1
#define INVTYPE_WORKBENCH 2
#define INVTYPE_FURNACE 3
#define INVTYPE_DISPENSER 4
#define INVTYPE_ENCHANTINGTABLE 5
#define INVTYPE_BREWINGSTAND 6
#define INVTYPE_VILLAGER 7
#define INVTYPE_BEACON 8
#define INVTYPE_ANVIL 9
#define INVTYPE_HOPPER 10
#define INVTYPE_DROPPER 11
#define INVTYPE_HORSE 12

int idef_width;
int idef_height;
int idef_wrap;
char** itemMap;
int* itemSizeMap;
int itemMapLength;

struct slot {
	int16_t item;
	unsigned char count;
	int16_t damage;
	struct nbt_tag* nbt;
};

struct inventory {
	struct mempool* pool;
	char* title;
	int type;
	struct slot** slots;
	size_t slot_count;
	int window;
	struct mempool* drag_pool;
	struct llist* drag_slot;
	struct hashmap* watching_players;
	struct tile_entity* tile;
	pthread_mutex_t mutex;
};

struct player;

struct inventory* inventory_new(struct mempool* pool, int type, int id, size_t slots, char* title);

void slot_duplicate(struct mempool* pool, struct slot* slot, struct slot* dup);

void inventory_set_slot(struct player* player, struct inventory* inv, int index, struct slot* slot, int broadcast);

struct slot* inventory_get(struct player* player, struct inventory* inv, int index);

void inventory_duplicate(struct mempool* pool, struct slot** from, struct slot** to, size_t size);

int inventory_validate(int invtype, int index, struct slot* slot);

int slot_max_size(struct slot* slot);

void inventory_swap(struct player* player, struct inventory* inv, int index1, int index2, int broadcast);

int slot_stackable(struct slot* s1, struct slot* s2);

int slot_stackable_damage_ignore(struct slot* s1, struct slot* s2);

int inventory_add_player(struct player* player, struct inventory* inv, struct slot* slot, int broadcast);

int inventory_add(struct player* player, struct inventory* inv, struct slot* slot, int start, int end, int broadcast);

#endif /* BASIN_INVENTORY_H_ */
