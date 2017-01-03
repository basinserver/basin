/*
 * inventory.c
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#include "network.h"
#include "packet.h"
#include "globals.h"
#include "item.h"
#include <string.h>
#include <math.h>
#include "util.h"
#include "game.h"
#include "queue.h"
#include "player.h"
#include "smelting.h"
#include "nbt.h"
#include <stdint.h>
#include "tileentity.h"
#include "inventory.h"
#include "hashmap.h"
#include <unistd.h>

void newInventory(struct inventory* inv, int type, int id, int slots) {
	inv->title = NULL;
	inv->slot_count = slots;
	inv->slots = xcalloc(sizeof(struct slot*) * inv->slot_count);
	inv->props = NULL;
	inv->prop_count = 0;
	inv->type = type;
	inv->windowID = id;
	inv->players = new_hashmap(1, 1);
	inv->dragSlot = xcalloc(2 * inv->slot_count);
	inv->dragSlot_count = 0;
	inv->te = NULL;
	pthread_mutex_init(&inv->mut, NULL);
}

void setSlot(struct player* player, struct inventory* inv, int index, struct slot* slot, int broadcast, int frx) {
	if (inv == NULL) return;
	if (inv->type != INVTYPE_PLAYERINVENTORY && player != NULL && index >= inv->slot_count) {
		index -= inv->slot_count;
		index += 9;
		inv = player->inventory;
	}
	if (index >= inv->slot_count || index < 0) return;
	if (inv->slots[index] != slot && inv->slots[index] != NULL && frx) {
		freeSlot(inv->slots[index]);
		xfree(inv->slots[index]);
	}
	if (slot == NULL || slot->item == -1) inv->slots[index] = NULL;
	else if (slot == inv->slots[index]) ;
	else {
		inv->slots[index] = xmalloc(sizeof(struct slot));
		duplicateSlot(slot, inv->slots[index]);
	}
	onInventoryUpdate(player, inv, index);
	if (broadcast) {
		BEGIN_BROADCAST(inv->players)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SETSLOT;
		pkt->data.play_client.setslot.window_id = inv->windowID;
		pkt->data.play_client.setslot.slot = index;
		duplicateSlot(inv->slots[index], &pkt->data.play_client.setslot.slot_data);
		add_queue(bc_player->outgoingPacket, pkt);
		END_BROADCAST(inv->players)
	} else {
		BEGIN_BROADCAST_EXCEPT(inv->players, player)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_SETSLOT;
		pkt->data.play_client.setslot.window_id = inv->windowID;
		pkt->data.play_client.setslot.slot = index;
		duplicateSlot(inv->slots[index], &pkt->data.play_client.setslot.slot_data);
		add_queue(bc_player->outgoingPacket, pkt);
		END_BROADCAST(inv->players)
	}
	//printf("set %i = %i broadcast=%i pc=%i\n", index, slot == NULL ? -1 : slot->item, broadcast, inv->players->count);
}

struct slot* getSlot(struct player* player, struct inventory* inv, int index) {
	if (inv == NULL) return NULL;
	if (inv->type != INVTYPE_PLAYERINVENTORY && player != NULL && index >= inv->slot_count) {
		index -= inv->slot_count;
		index += 9;
		inv = player->inventory;
	}
	if (index >= inv->slot_count || index < 0) return NULL;
	return inv->slots[index];
}

void copyInventory(struct slot** from, struct slot** to, int size) {
	for (int i = 0; i < size; i++) {
		if (to[i] != NULL) {
			freeSlot(to[i]);
			xfree(to[i]);
			to[i] = NULL;
		}
		if (from[i] != NULL) {
			to[i] = xmalloc(sizeof(struct slot*));
			duplicateSlot(from[i], to[i]);
		}
	}
}

void _freeInventorySlots(struct inventory* inv) {
	if (inv->slots != NULL) {
		for (size_t i = 0; i < inv->slot_count; i++) {
			if (inv->slots[i] == NULL) continue;
			freeSlot(inv->slots[i]);
			xfree(inv->slots[i]);
		}
		xfree(inv->slots);
		inv->slots = NULL;
	}
}

void freeInventory(struct inventory* inv) {
	if (inv->title != NULL) xfree(inv->title);
	if (inv->props != NULL) {
		for (size_t i = 0; i < inv->prop_count; i++) {
			xfree(inv->props[i]);
		}
		xfree(inv->props);
	}
	del_hashmap(inv->players);
	_freeInventorySlots(inv);
	xfree(inv->dragSlot);
	pthread_mutex_destroy(&inv->mut);
}

void freeSlot(struct slot* slot) {
	if (slot->nbt != NULL) {
		freeNBT(slot->nbt);
		xfree(slot->nbt);
	}
}

int validItemForSlot(int invtype, int si, struct slot* slot) {
	if (invtype == INVTYPE_PLAYERINVENTORY) {
		if (si == 0) return 0;
		if (si == 5) return getItemInfo(slot->item)->armorType == ARMOR_HELMET;
		else if (si == 6) return getItemInfo(slot->item)->armorType == ARMOR_CHESTPLATE;
		else if (si == 7) return getItemInfo(slot->item)->armorType == ARMOR_LEGGINGS;
		else if (si == 8) return getItemInfo(slot->item)->armorType == ARMOR_BOOTS;
	} else if (invtype == INVTYPE_WORKBENCH) {
		if (si == 0) return 0;
	} else if (invtype == INVTYPE_FURNACE) {
		if (si == 2) return 0;
		else if (si == 1) return getSmeltingFuelBurnTime(slot) > 0;
	}
	return 1;
}

int itemsStackable(struct slot* s1, struct slot* s2) {
	return s1 != NULL && s2 != NULL && s1->item != -1 && s2->item != -1 && s1->item == s2->item && s1->damage == s2->damage && (s1->nbt == NULL || s1->nbt->id == 0) && (s2->nbt == NULL || s2->nbt->id == 0);
}

int itemsStackable2(struct slot* s1, struct slot* s2) {
	return s1 != NULL && s2 != NULL && s1->item != -1 && s2->item != -1 && s1->item == s2->item && (s1->damage == s2->damage || s1->damage == -1 || s2->damage == -1) && (s1->nbt == NULL || s1->nbt->id == 0) && (s2->nbt == NULL || s2->nbt->id == 0);
}

void swapSlots(struct player* player, struct inventory* inv, int i1, int i2, int broadcast) {
	struct slot* s1 = getSlot(player, inv, i1);
	struct slot* s2 = getSlot(player, inv, i2);
	if (!validItemForSlot(inv->type, i2, s1) || !validItemForSlot(inv->type, i1, s2)) return;
	setSlot(player, inv, i1, s2, broadcast, 0);
	setSlot(player, inv, i2, s1, broadcast, 0);
}

int maxStackSize(struct slot* slot) {
	struct item_info* ii = getItemInfo(slot->item);
	return ii == NULL ? 64 : ii->maxStackSize;
}

int addInventoryItem_PI(struct player* player, struct inventory* inv, struct slot* slot, int broadcast) {
	int r = addInventoryItem(player, inv, slot, 36, 45, broadcast);
	if (r > 0) r = addInventoryItem(player, inv, slot, 9, 36, broadcast);
	return r;
}

int addInventoryItem(struct player* player, struct inventory* inv, struct slot* slot, int start, int end, int broadcast) {
	size_t cap = inv->slot_count;
	if (inv->type != INVTYPE_PLAYERINVENTORY && player != NULL) cap += 36;
	if (start < 0 || end < 0 || start > cap || end > cap || slot == NULL) return -1;
	int mss = maxStackSize(slot);
	if (mss > 1) for (size_t i = start; start < end ? (i < end) : (i > end); start < end ? i++ : i--) {
		struct slot* invi = getSlot(player, inv, i);
		if (itemsStackable(invi, slot) && invi->itemCount < mss) {
			int t = (invi->itemCount + slot->itemCount);
			invi->itemCount = t <= mss ? t : mss;
			int s = t - invi->itemCount;
			if (s <= 0) {
				slot = NULL;
			} else slot->itemCount -= s;
			setSlot(player, inv, i, invi, broadcast, 1);
		}
	}
	for (size_t i = start; start < end ? (i < end) : (i > end); start < end ? i++ : i--) {
		struct slot* invi = getSlot(player, inv, i);
		if (invi == NULL) {
			invi = xmalloc(sizeof(struct slot));
			duplicateSlot(slot, invi);
			setSlot(player, inv, i, invi, broadcast, 1);
			return 0;
		}
	}
	return slot == NULL ? 0 : slot->itemCount;
}

void setInventoryProperty(struct inventory* inv, int16_t name, int16_t value) {
	if (inv->props != NULL) {
		for (size_t i = 0; i < inv->prop_count; i++) {
			if (inv->props[i][0] == name) {
				inv->props[i][1] = value;
				return;
			}
		}
		inv->props = xrealloc(inv->props, sizeof(int16_t*) * (inv->prop_count + 1));
	} else {
		inv->props = xmalloc(sizeof(int16_t*));
		inv->prop_count = 0;
	}
	int16_t* dm = xmalloc(sizeof(int16_t) * 2);
	dm[0] = name;
	dm[1] = value;
	inv->props[inv->prop_count++] = dm;
}

