/*
 * tileentity.c
 *
 *  Created on: Dec 27, 2016
 *      Author: root
 */

#include <basin/nbt.h>
#include <basin/network.h>
#include <basin/block.h>
#include <basin/inventory.h>
#include <basin/tileentity.h>
#include <basin/world.h>
#include <avuna/string.h>
#include <avuna/llist.h>
#include <avuna/pmem.h>
#include <stdlib.h>

void tetick_furnace(struct world* world, struct tile_entity* te) {
    update_furnace(world, te);
}

//TODO: tile entity type registry for plugins/mods

struct tile_entity* tile_parse(struct mempool* parent, struct nbt_tag* tag) {
    struct mempool* pool = mempool_new();
    pchild(parent, pool);
    struct tile_entity* tile = pmalloc(pool, sizeof(struct tile_entity));
    tile->pool = pool;
    tile->id = NULL;
    struct nbt_tag* tmp = NULL;
    tmp = nbt_get(tag, "id");
    if (tmp == NULL || tmp->id != NBT_TAG_STRING) return NULL;
    tile->id = str_dup(tmp->data.nbt_string, 0, tile->pool);
    tmp = nbt_get(tag, "x");
    if (tmp == NULL || tmp->id != NBT_TAG_INT) return NULL;
    tile->x = tmp->data.nbt_int;
    tmp = nbt_get(tag, "y");
    if (tmp == NULL || tmp->id != NBT_TAG_INT) return NULL;
    tile->y = (uint8_t) tmp->data.nbt_int;
    tmp = nbt_get(tag, "z");
    if (tmp == NULL || tmp->id != NBT_TAG_INT) return NULL;
    tile->z = tmp->data.nbt_int;
    tile->tick = NULL;
    if (str_eq(tile->id, "minecraft:chest")) {
        tmp = nbt_get(tag, "CustomName");
        char* title;
        if (tmp == NULL) title = "{\"text\": \"Chest\"}";
        else {
            size_t title_length = 17 + strlen(tmp->data.nbt_string);
            title = pmalloc(tile->pool, title_length);
            snprintf(title, title_length, "{\"text\": \"%s\"}", tmp->data.nbt_string);
        }

        tile->data.chest.inv = inventory_new(tile->pool, INVTYPE_CHEST, 2, 27, title);
        tile->data.chest.inv->tile = tile;
        tmp = nbt_get(tag, "Lock");
        if (tmp == NULL) tile->data.chest.lock = NULL;
        else tile->data.chest.lock = str_dup(tmp->data.nbt_string, 0, tile->pool);
        struct nbt_tag* items = nbt_get(tag, "Items");
        if (items == NULL || items->id != NBT_TAG_LIST) {
        } else {
            ITER_LLIST(items->children_list, value) {
                struct nbt_tag* child = value;
                if (child == NULL || child->id != NBT_TAG_COMPOUND) continue;
                tmp = nbt_get(child, "Slot");
                if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
                uint8_t index = (uint8_t) tmp->data.nbt_byte;
                if (index > 26) continue;
                struct slot* slot = pmalloc(tile->pool, sizeof(struct slot));
                tmp = nbt_get(child, "id");
                if (tmp == NULL || tmp->id != NBT_TAG_STRING) continue;
                slot->item = getItemFromName(tmp->data.nbt_string);
                tmp = nbt_get(child, "Count");
                if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
                slot->count = (unsigned char) tmp->data.nbt_byte;
                tmp = nbt_get(child, "Damage");
                if (tmp == NULL || tmp->id != NBT_TAG_SHORT) continue;
                slot->damage = tmp->data.nbt_short;
                tmp = nbt_get(child, "tag");
                if (tmp == NULL || tmp->id != NBT_TAG_COMPOUND) {
                    slot->nbt = NULL;
                } else {
                    slot->nbt = nbt_clone(tile->pool, tmp);
                }
                tile->data.chest.inv->slots[index] = slot;
                ITER_LLIST_END();
            }
        }
    } else if (str_eq(tile->id, "minecraft:furnace")) {
        tile->tick = tetick_furnace;
        tmp = nbt_get(tag, "CustomName");
        char* title;
        if (tmp == NULL) title = "{\"text\": \"Furnace\"}";
        else {
            size_t title_length = 17 + strlen(tmp->data.nbt_string);
            title = pmalloc(tile->pool, title_length);
            snprintf(title, title_length, "{\"text\": \"%s\"}", tmp->data.nbt_string);
        }

        tile->data.furnace.inv = inventory_new(tile->pool, INVTYPE_FURNACE, 2, 27, title);
        tile->data.furnace.inv->tile = tile;
        tmp = nbt_get(tag, "Lock");
        if (tmp == NULL) tile->data.chest.lock = NULL;
        else tile->data.furnace.lock = str_dup(tmp->data.nbt_string, 0, tile->pool);
        tmp = nbt_get(tag, "BurnTime");
        if (tmp == NULL) tile->data.furnace.burnTime = 0;
        else tile->data.furnace.burnTime = tmp->data.nbt_short;
        tmp = nbt_get(tag, "CookTime");
        if (tmp == NULL) tile->data.furnace.cookTime = 0;
        else tile->data.furnace.cookTime = tmp->data.nbt_short;
        tmp = nbt_get(tag, "CookTimeTotal");
        if (tmp == NULL) tile->data.furnace.cookTimeTotal = 0;
        else tile->data.furnace.cookTimeTotal = tmp->data.nbt_short;
        struct nbt_tag* items = nbt_get(tag, "Items");
        if (items == NULL || items->id != NBT_TAG_LIST) {
        } else {
            ITER_LLIST(items->children_list, value) {
                struct nbt_tag* child = value;
                if (child == NULL || child->id != NBT_TAG_COMPOUND) continue;
                tmp = nbt_get(child, "Slot");
                if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
                uint8_t index = (uint8_t) tmp->data.nbt_byte;
                if (index > 3) continue;
                struct slot* slot = pmalloc(tile->pool, sizeof(struct slot));
                tmp = nbt_get(child, "id");
                if (tmp == NULL || tmp->id != NBT_TAG_STRING) continue;
                slot->item = getItemFromName(tmp->data.nbt_string);
                tmp = nbt_get(child, "Count");
                if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
                slot->count = (unsigned char) tmp->data.nbt_byte;
                tmp = nbt_get(child, "Damage");
                if (tmp == NULL || tmp->id != NBT_TAG_SHORT) continue;
                slot->damage = tmp->data.nbt_short;
                tmp = nbt_get(child, "tag");
                if (tmp == NULL || tmp->id != NBT_TAG_COMPOUND) {
                    slot->nbt = NULL;
                } else {
                    slot->nbt = pmalloc(sizeof(struct nbt_tag));
                    slot->nbt = nbt_clone(tile->pool, tmp);
                }
                tile->data.furnace.inv->slots[index] = slot;
                ITER_LLIST_END();
            }
        }
        tile->data.furnace.lastBurnMax = 0;
    }
    return tile;
}

struct tile_entity* tile_new(struct mempool* parent, char* id, int32_t x, uint8_t y, int32_t z) {
    struct mempool* pool = mempool_new();
    pchild(parent, pool);
    struct tile_entity* tile = pmalloc(pool, sizeof(struct tile_entity));
    tile->pool = pool;
    tile->id = str_dup(id, 0, tile->pool);
    tile->x = x;
    tile->y = y;
    tile->z = z;
    tile->tick = NULL;
    memset(&tile->data, 0, sizeof(union tile_entity_data));
    return tile;
}

struct nbt_tag* tile_serialize(struct mempool* parent, struct tile_entity* tile, int forClient) {
    struct mempool* pool = mempool_new();
    pchild(parent, pool);
    struct nbt_tag* nbt = nbt_new(pool, NBT_TAG_COMPOUND);
    /*if (str_eq(tile->id, "minecraft:chest") && !forClient) {
        nbt->children_count = 5;
    }*/
    struct nbt_tag* id = nbt_new(pool, NBT_TAG_STRING);
    id->name = "id";
    id->data.nbt_string = tile->id;
    nbt_put(nbt, id);
    struct nbt_tag* x = nbt_new(pool, NBT_TAG_INT);
    id->name = "x";
    id->data.nbt_int = tile->x;
    nbt_put(nbt, x);
    struct nbt_tag* y = nbt_new(pool, NBT_TAG_INT);
    id->name = "y";
    id->data.nbt_int = tile->y;
    nbt_put(nbt, y);
    struct nbt_tag* z = nbt_new(pool, NBT_TAG_INT);
    id->name = "z";
    id->data.nbt_int = tile->z;
    nbt_put(nbt, z);

    if (str_eq(tile->id, "minecraft:chest") && !forClient) {
        /*nbt->children[4] = xmalloc(sizeof(struct nbt_tag));
         nbt->children[4]->id = NBT_TAG_INT;
         nbt->children[4]->name = xstrdup("z", 0);
         nbt->children[4]->data.nbt_int = tile->z;
         nbt->children[4]->children_count = 0;
         nbt->children[4]->children = NULL;
         */
    }
    return nbt;
}
