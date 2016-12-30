/*
 * inventory.h
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#ifndef INVENTORY_H_
#define INVENTORY_H_

#include <stdint.h>

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
		unsigned char itemCount;
		int16_t damage;
		struct nbt_tag* nbt;
};

struct inventory {
		char* title;
		struct slot** slots;
		size_t slot_count;
		int type;
		int16_t** props;
		size_t prop_count;
		int windowID;
		struct collection* players;
		uint16_t* dragSlot;
		uint16_t dragSlot_count;
		struct tile_entity* te;
};

struct player;

void setSlot(struct player* player, struct inventory* inv, int index, struct slot* slot, int broadcast, int free);

int validItemForSlot(int invtype, int si, struct slot* slot);

struct slot* getSlot(struct player* player, struct inventory* inv, int index);

int maxStackSize(struct slot* slot);

void copyInventory(struct slot** from, struct slot** to, int size);

void swapSlots(struct player* player, struct inventory* inv, int i1, int i2, int broadcast);

int itemsStackable(struct slot* s1, struct slot* s2);

int itemsStackable2(struct slot* s1, struct slot* s2);

void newInventory(struct inventory* inv, int type, int id, int slots);

void freeInventory(struct inventory* inv);

void freeSlot(struct slot* slot);

int addInventoryItem_PI(struct player* player, struct inventory* inv, struct slot* slot, int broadcast);

int addInventoryItem(struct player* player, struct inventory* inv, struct slot* slot, int start, int end, int broadcast);

void setInventoryProperty(struct inventory* inv, int16_t name, int16_t value);

#endif /* INVENTORY_H_ */
