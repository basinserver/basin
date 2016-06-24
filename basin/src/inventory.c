/*
 * inventory.c
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#include "inventory.h"
#include "globals.h"
#include <GL/gl.h>
#include "item.h"
#include "network.h"
#include <string.h>
#include <math.h>

void newInventory(struct inventory* inv, int type, int id) {
	inv->title = NULL;
	inv->slots = NULL;
	inv->slot_count = 0;
	inv->props = NULL;
	inv->prop_count = 0;
	inv->type = type;
	inv->windowID = id;
}

void _freeInventorySlots(struct inventory* inv) {
	if (inv->slots != NULL) {
		for (size_t i = 0; i < inv->slot_count; i++) {
			if (inv->slots[i] == NULL) continue;
			if (inv->slots[i]->nbt != NULL) {
				freeNBT(inv->slots[i]->nbt);
				free(inv->slots[i]->nbt);
			}
			free(inv->slots[i]);
		}
		free(inv->slots);
		inv->slots = NULL;
	}
}

void freeInventory(struct inventory* inv) {
	if (inv->title != NULL) free(inv->title);
	if (inv->props != NULL) {
		for (size_t i = 0; i < inv->prop_count; i++) {
			free(inv->props[i]);
		}
		free(inv->props);
	}
	_freeInventorySlots(inv);
}

int validItemForSlot(int invtype, int si, struct slot* slot) {
	if (invtype == INVTYPE_PLAYERINVENTORY) {
		if (si == 0) return 0;
		if (si >= 5 && si <= 8) {
			return slot->item >= LEATHER_CAP && slot->item <= GOLDEN_BOOTS;
		} else if (si == 45) {
			return slot->item == SHIELD;
		}
	}
	return 1;
}

int maxStackSize(struct slot* slot) {
	if ((slot->item >= IRON_SHOVEL && slot->item <= FLINT_AND_STEEL) || slot->item == BOW || (slot->item >= IRON_SWORD && slot->item <= DIAMOND_AXE) || (slot->item >= MUSHROOM_STEW && slot->item <= GOLDEN_AXE) || (slot->item >= WOODEN_HOE && slot->item <= GOLDEN_HOE) || (slot->item >= LEATHER_CAP && slot->item <= GOLDEN_BOOTS) || (slot->item >= WATER_BUCKET && slot->item <= SADDLE) || slot->item == OAK_BOAT || slot->item == MILK || slot->item == MINECART_WITH_CHEST || slot->item == MINECART_WITH_FURNACE || slot->item == FISHING_ROD || slot->item == CAKE || slot->item == BED || slot->item == SHEARS || slot->item == POTION || slot->item == BOOK_AND_QUILL || slot->item == CARROT_ON_A_STICK || slot->item == ENCHANTED_BOOK || slot->item == MINECART_WITH_TNT || slot->item == MINECART_WITH_HOPPER || slot->item == RABBIT_STEW || slot->item == IRON_HORSE_ARMOR || slot->item == GOLD_HORSE_ARMOR || slot->item == DIAMOND_HORSE_ARMOR || slot->item == MINECART_WITH_COMMAND_BLOCK
			|| slot->item == BEETROOT_SOUP || slot->item == ITEM_SPLASH_POTION_NAME || slot->item == ITEM_LINGERING_POTION_NAME || slot->item >= SHIELD) return 1;
	if (slot->item == SIGN || slot->item == BUCKET || slot->item == SNOWBALL || slot->item == EGG || slot->item == ENDER_PEARL || slot->item == WRITTEN_BOOK || slot->item == ARMOR_STAND || slot->item == TILE_BANNER_NAME) return 16;
	return 64;
}

void setInventoryItems(struct inventory* inv, struct slot** slots, size_t slot_count) {
	_freeInventorySlots(inv);
	for (size_t i = 0; i < slot_count; i++) {
		if (slots[i]->item < 0) {
			if (slots[i]->nbt != NULL) freeNBT(slots[i]->nbt);
			free(slots[i]);
			slots[i] = NULL;
		}
	}
	inv->slots = slots;
	inv->slot_count = slot_count;
}

void setInventorySlot(struct inventory* inv, struct slot* slot, int index) {
	if (index < 0 || index >= inv->slot_count) return;
	if (inv->slots == NULL) {
		if (inv->slot_count < 1) return;
		inv->slots = malloc(sizeof(struct slot*) * inv->slot_count);
		for (int i = 0; i < inv->slot_count; i++) {
			inv->slots[i] = NULL;
		}
	}
	if (slot != NULL && slot->item < 0) {
		if (slot->nbt != NULL) {
			freeNBT(slot->nbt);
			free(slot->nbt);
		}
		free(slot);
		slot = NULL;
	}
	if (inv->slots[index] != NULL) {
		if (inv->slots[index]->nbt != NULL) {
			freeNBT(inv->slots[index]->nbt);
			free(inv->slots[index]->nbt);
		}
		free(inv->slots[index]);
	}
	inv->slots[index] = slot;
}

void setInventoryProperty(struct inventory* inv, int16_t name, int16_t value) {
	if (inv->props != NULL) {
		for (size_t i = 0; i < inv->prop_count; i++) {
			if (inv->props[i][0] == name) {
				inv->props[i][1] = value;
				return;
			}
		}
		inv->props = realloc(inv->props, sizeof(int16_t*) * (inv->prop_count + 1));
	} else {
		inv->props = malloc(sizeof(int16_t*));
		inv->prop_count = 0;
	}
	int16_t* dm = malloc(sizeof(int16_t) * 2);
	dm[0] = name;
	dm[1] = value;
	inv->props[inv->prop_count++] = dm;
}

