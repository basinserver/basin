/*
 * inventory.c
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#include "inventory.h"
#include "globals.h"
#include "item.h"
#include "network.h"
#include <string.h>
#include <math.h>
#include "util.h"
#include "game.h"
#include "queue.h"

void newInventory(struct inventory* inv, int type, int id, int slots) {
	inv->title = NULL;
	inv->slots = NULL;
	inv->slot_count = slots;
	inv->props = NULL;
	inv->prop_count = 0;
	inv->type = type;
	inv->windowID = id;
	inv->players = new_collection(0, 0);
	inv->slots = xcalloc(sizeof(struct slot*) * inv->slot_count);
	inv->inHand = NULL;
	inv->dragSlot = xcalloc(2 * inv->slot_count);
	inv->dragSlot_count = 0;
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
	del_collection(inv->players);
	if (inv->inHand != NULL) {
		freeSlot(inv->inHand);
		xfree(inv->inHand);
	}
	_freeInventorySlots(inv);
	xfree(inv->dragSlot);
}

void freeSlot(struct slot* slot) {
	if (slot->nbt != NULL) {
		freeNBT(slot->nbt);
		xfree(slot->nbt);
	}
}

int validItemForSlot(int invtype, int si, struct slot* slot) { // TODO
	if (invtype == INVTYPE_PLAYERINVENTORY) {
		if (si == 0) return 0;
		if (si >= 5 && si <= 8) {
			return slot->item >= ITM_HELMETCLOTH && slot->item <= ITM_BOOTSGOLD;
		} else if (si == 45) {
			return slot->item == ITM_SHIELD;
		}
	}
	return 1;
}

int itemsStackable(struct slot* s1, struct slot* s2) {
	return s1 != NULL && s2 != NULL && s1->item != -1 && s2->item != -1 && s1->item == s2->item && s1->damage == s2->damage && (s1->nbt == NULL || s1->nbt->id == 0) && (s2->nbt == NULL || s2->nbt->id == 0);
}

int itemsStackable2(struct slot* s1, struct slot* s2) {
	return s1 != NULL && s2 != NULL && s1->item != -1 && s2->item != -1 && s1->item == s2->item && (s1->damage == s2->damage || s1->damage == -1 || s2->damage == -1) && (s1->nbt == NULL || s1->nbt->id == 0) && (s2->nbt == NULL || s2->nbt->id == 0);
}

void broadcast_setslot(struct inventory* inv, int index) {
	BEGIN_BROADCAST(inv->players)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_SETSLOT;
	pkt->data.play_client.setslot.window_id = inv->windowID;
	pkt->data.play_client.setslot.slot = index;
	duplicateSlot(inv->slots[index], &pkt->data.play_client.setslot.slot_data);
	add_queue(bc_player->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST
}

void swapSlots(struct inventory* inv, int i1, int i2) {
	struct slot* s1 = inv->slots[i1];
	inv->slots[i1] = inv->slots[i2];
	inv->slots[i2] = s1;
	onInventoryUpdate(inv, i1);
	onInventoryUpdate(inv, i2);
}

int maxStackSize(struct slot* slot) {
	if ((slot->item >= ITM_SHOVELIRON && slot->item <= ITM_FLINTANDSTEEL) || slot->item == ITM_BOW || (slot->item >= ITM_SWORDIRON && slot->item <= ITM_HATCHETDIAMOND) || (slot->item >= ITM_MUSHROOMSTEW && slot->item <= ITM_HATCHETGOLD) || (slot->item >= ITM_HOEWOOD && slot->item <= ITM_HOEGOLD) || (slot->item >= ITM_HELMETCLOTH && slot->item <= ITM_BOOTSGOLD) || (slot->item >= ITM_BUCKETWATER && slot->item <= ITM_SADDLE) || slot->item == ITM_BOAT_OAK || slot->item == ITM_MILK || slot->item == ITM_MINECARTCHEST || slot->item == ITM_MINECARTFURNACE || slot->item == ITM_FISHINGROD || slot->item == ITM_CAKE || slot->item == ITM_BED || slot->item == ITM_SHEARS || slot->item == ITM_POTION || slot->item == ITM_WRITINGBOOK || slot->item == ITM_CARROTONASTICK || slot->item == ITM_ENCHANTEDBOOK || slot->item == ITM_MINECARTTNT || slot->item == ITM_MINECARTHOPPER || slot->item == ITM_RABBITSTEW || slot->item == ITM_HORSEARMORMETAL || slot->item == ITM_HORSEARMORGOLD
			|| slot->item == ITM_HORSEARMORDIAMOND || slot->item == ITM_MINECARTCOMMANDBLOCK || slot->item == ITM_BEETROOT_SOUP || slot->item == ITM_SPLASH_POTION || slot->item == ITM_LINGERING_POTION || slot->item >= ITM_SHIELD) return 1;
	if (slot->item == ITM_SIGN || slot->item == ITM_BUCKET || slot->item == ITM_SNOWBALL || slot->item == ITM_EGG || slot->item == ITM_ENDERPEARL || slot->item == ITM_WRITTENBOOK || slot->item == ITM_ARMORSTAND || slot->item == ITM_TOTEM) return 16;
	return 64;
}

void setInventoryItems(struct inventory* inv, struct slot** slots, size_t slot_count) {
	_freeInventorySlots(inv);
	for (size_t i = 0; i < slot_count; i++) {
		if (slots[i]->item < 0) {
			if (slots[i]->nbt != NULL) freeNBT(slots[i]->nbt);
			xfree(slots[i]->nbt);
			xfree(slots[i]);
			slots[i] = NULL;
		}
	}
	inv->slots = slots;
	inv->slot_count = slot_count;
}

void setInventorySlot(struct inventory* inv, struct slot* slot, int index) {
	if (index < 0 || index >= inv->slot_count) return;
	if (inv->slots[index] != NULL && inv->slots[index] != slot) {
		if (inv->slots[index]->nbt != NULL) {
			freeNBT(inv->slots[index]->nbt);
			xfree(inv->slots[index]->nbt);
		}
		xfree(inv->slots[index]);
	}
	if (slot != NULL && slot->item == -1) slot = NULL;
	if (slot == NULL) inv->slots[index] = NULL;
	else {
		inv->slots[index] = xmalloc(sizeof(struct slot));
		duplicateSlot(slot, inv->slots[index]);
	}
	broadcast_setslot(inv, index);
	onInventoryUpdate(inv, index);
}

int addInventoryItem_PI(struct inventory* inv, struct slot* slot) {
	int r = addInventoryItem(inv, slot, 36, 45);
	if (r > 0) r = addInventoryItem(inv, slot, 9, 36);
	return r;
}

int addInventoryItem(struct inventory* inv, struct slot* slot, int start, int end) {
	if (start < 0 || end < 0 || start > inv->slot_count || end > inv->slot_count || slot == NULL) return -1;
	int mss = maxStackSize(slot);
	if (mss > 1) for (size_t i = start; start < end ? (i < end) : (i > end); start < end ? i++ : i--) {
		if (itemsStackable(inv->slots[i], slot) && inv->slots[i]->itemCount < mss) {
			int t = (inv->slots[i]->itemCount + slot->itemCount);
			inv->slots[i]->itemCount = t <= mss ? t : mss;
			int s = t - inv->slots[i]->itemCount;
			broadcast_setslot(inv, i);
			if (s <= 0) {
				slot = NULL;
				return 0;
			} else slot->itemCount -= s;
			onInventoryUpdate(inv, i);
		}
	}
	for (size_t i = start; start < end ? (i < end) : (i > end); start < end ? i++ : i--) {
		if (inv->slots[i] == NULL) {
			inv->slots[i] = xmalloc(sizeof(struct slot));
			duplicateSlot(slot, inv->slots[i]);
			broadcast_setslot(inv, i);
			onInventoryUpdate(inv, i);
			return 0;
		}
	}
	return slot == NULL ? 0 : slot->itemCount;
}

struct slot* getInventorySlot(struct inventory* inv, int index) {
	if (inv->slots == NULL) return NULL;
	return inv->slots[index];
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

