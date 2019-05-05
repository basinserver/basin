/*
 * inventory.c
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#include "basin/packet.h"
#include <basin/network.h>
#include <basin/globals.h>
#include <basin/item.h>
#include <basin/game.h>
#include <basin/player.h>
#include <basin/smelting.h>
#include <basin/nbt.h>
#include <basin/tileentity.h>
#include <basin/inventory.h>
#include <avuna/queue.h>
#include <avuna/hash.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>

struct inventory* inventory_new(struct mempool* pool, int type, int id, size_t slots, char* title) {
    struct inventory* inv = pcalloc(pool, sizeof(struct inventory));
    inv->pool = pool;
    inv->title = title;
    inv->slot_count = slots;
    inv->slots = pcalloc(inv->pool, sizeof(struct slot*) * inv->slot_count);
    inv->type = type;
    inv->window = id;
    inv->watching_players = hashmap_thread_new(8, inv->pool);
    inv->drag_pool = mempool_new();
    pchild(inv->pool, inv->drag_pool);
    inv->drag_slot = llist_new(inv->drag_pool);
    pthread_mutex_init(&inv->mutex, NULL);
    phook(inv->pool, (void (*)(void*)) pthread_mutex_destroy, &inv->mutex);
    return inv;
}


void slot_duplicate(struct mempool* pool, struct slot* slot, struct slot* dup) {
    if (slot == NULL) {
        memset(dup, 0, sizeof(struct slot));
        dup->item = -1;
        return;
    }
    dup->item = slot->item;
    dup->damage = slot->damage;
    dup->count = slot->count;
    dup->nbt = nbt_clone(pool, slot->nbt);
}

void inventory_set_slot(struct player* player, struct inventory* inv, int index, struct slot* slot, int broadcast) {
    if (inv == NULL) return;
    if (inv->type != INVTYPE_PLAYERINVENTORY && player != NULL && index >= inv->slot_count) {
        index -= inv->slot_count;
        index += 9;
        inv = player->inventory;
    }
    if (index >= inv->slot_count || index < 0) return;
    if (inv->slots[index] != slot && inv->slots[index] != NULL) {
        if (inv->slots[index]->nbt != NULL) {
            pfree(slot->nbt->pool);
        }
        inv->slots[index] = NULL;
    }
    if (slot == NULL || slot->item == -1) inv->slots[index] = NULL;
    else if (slot == inv->slots[index]) ;
    else {
        inv->slots[index] = pmalloc(inv->pool, sizeof(struct slot));
        slot_duplicate(inv->pool, slot, inv->slots[index]);
    }
    game_update_inventory(player, inv, index);
    if (broadcast) {
        BEGIN_BROADCAST(inv->watching_players)
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_SETSLOT);
        pkt->data.play_client.setslot.window_id = (int8_t) inv->window;
        pkt->data.play_client.setslot.slot = (int16_t) index;
        slot_duplicate(pkt->pool, inv->slots[index], &pkt->data.play_client.setslot.slot_data);
        queue_push(bc_player->outgoing_packets, pkt);
        END_BROADCAST(inv->watching_players)
    } else {
        BEGIN_BROADCAST_EXCEPT(inv->watching_players, player)
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_SETSLOT);
        pkt->data.play_client.setslot.window_id = (int8_t) inv->window;
        pkt->data.play_client.setslot.slot = (int16_t) index;
        slot_duplicate(pkt->pool, inv->slots[index], &pkt->data.play_client.setslot.slot_data);
        queue_push(bc_player->outgoing_packets, pkt);
        END_BROADCAST(inv->watching_players)
    }
}

struct slot* inventory_get(struct player* player, struct inventory* inv, int index) {
    if (inv == NULL) return NULL;
    if (inv->type != INVTYPE_PLAYERINVENTORY && player != NULL && index >= inv->slot_count) {
        index -= inv->slot_count;
        index += 9;
        inv = player->inventory;
    }
    if (index >= inv->slot_count || index < 0) return NULL;
    return inv->slots[index];
}

void inventory_duplicate(struct mempool* pool, struct slot** from, struct slot** to, size_t size) {
    for (int i = 0; i < size; i++) {
        if (from[i] != NULL) {
            to[i] = pmalloc(pool, sizeof(struct slot*));
            slot_duplicate(pool, from[i], to[i]);
        }
    }
}

int inventory_validate(int invtype, int index, struct slot* slot) {
    if (invtype == INVTYPE_PLAYERINVENTORY) {
        if (index == 0) return 0;
        if (index == 5) return getItemInfo(slot->item)->armorType == ARMOR_HELMET;
        else if (index == 6) return getItemInfo(slot->item)->armorType == ARMOR_CHESTPLATE;
        else if (index == 7) return getItemInfo(slot->item)->armorType == ARMOR_LEGGINGS;
        else if (index == 8) return getItemInfo(slot->item)->armorType == ARMOR_BOOTS;
    } else if (invtype == INVTYPE_WORKBENCH) {
        if (index == 0) return 0;
    } else if (invtype == INVTYPE_FURNACE) {
        if (index == 2) return 0;
        else if (index == 1) return smelting_burnTime(slot) > 0;
    }
    return 1;
}

int slot_stackable(struct slot* s1, struct slot* s2) {
    return s1 != NULL && s2 != NULL && s1->item != -1 && s2->item != -1 && s1->item == s2->item && s1->damage == s2->damage && (s1->nbt == NULL || s1->nbt->id == 0) && (s2->nbt == NULL || s2->nbt->id == 0);
}

int slot_stackable_damage_ignore(struct slot* s1, struct slot* s2) {
    return s1 != NULL && s2 != NULL && s1->item != -1 && s2->item != -1 && s1->item == s2->item && (s1->damage == s2->damage || s1->damage == -1 || s2->damage == -1) && (s1->nbt == NULL || s1->nbt->id == 0) && (s2->nbt == NULL || s2->nbt->id == 0);
}

void inventory_swap(struct player* player, struct inventory* inv, int index1, int index2, int broadcast) {
    struct slot* slot1 = inventory_get(player, inv, index1);
    struct slot* slot2 = inventory_get(player, inv, index2);
    if (!inventory_validate(inv->type, index2, slot1) || !inventory_validate(inv->type, index1, slot2)) return;
    inventory_set_slot(player, inv, index1, slot2, broadcast, 0);
    inventory_set_slot(player, inv, index2, slot1, broadcast, 0);
}

int slot_max_size(struct slot* slot) {
    struct item_info* ii = getItemInfo(slot->item);
    return ii == NULL ? 64 : ii->maxStackSize;
}

int inventory_add_player(struct player* player, struct inventory* inv, struct slot* slot, int broadcast) {
    int r = inventory_add(player, inv, slot, 36, 45, broadcast);
    if (r > 0) r = inventory_add(player, inv, slot, 9, 36, broadcast);
    return r;
}

int inventory_add(struct player* player, struct inventory* inv, struct slot* slot, int start, int end, int broadcast) {
    size_t cap = inv->slot_count;
    if (inv->type != INVTYPE_PLAYERINVENTORY && player != NULL) cap += 36;
    if (start < 0 || end < 0 || start > cap || end > cap || slot == NULL) return -1;
    int mss = slot_max_size(slot);
    if (mss > 1) for (int i = start; start < end ? (i < end) : (i > end); start < end ? i++ : i--) {
        struct slot* sub_slot = inventory_get(player, inv, i);
        if (slot_stackable(sub_slot, slot) && sub_slot->count < mss) {
            int t = (sub_slot->count + slot->count);
            sub_slot->count = (unsigned char) (t <= mss ? t : mss);
            int s = t - sub_slot->count;
            if (s <= 0) {
                slot = NULL;
            } else slot->count -= s;
            inventory_set_slot(player, inv, i, sub_slot, broadcast);
        }
    }
    for (int i = start; start < end ? (i < end) : (i > end); start < end ? i++ : i--) {
        struct slot* sub_slot = inventory_get(player, inv, i);
        if (sub_slot == NULL) {
            sub_slot = pmalloc(inv->pool, sizeof(struct slot));
            slot_duplicate(slot, sub_slot);
            inventory_set_slot(player, inv, i, sub_slot, broadcast);
            return 0;
        }
    }
    return slot == NULL ? 0 : slot->count;
}