/*
 * tileentity.c
 *
 *  Created on: Dec 27, 2016
 *      Author: root
 */

#include "nbt.h"
#include <stdlib.h>
#include "util.h"
#include "xstring.h"
#include "network.h"
#include "block.h"
#include "inventory.h"
#include "tileentity.h"
#include "world.h"

void tetick_furnace(struct world* world, struct tile_entity* te) {
	update_furnace(world, te);
}

struct tile_entity* parseTileEntity(struct nbt_tag* tag) {
	struct tile_entity* te = xmalloc(sizeof(struct tile_entity));
	te->id = NULL;
	struct nbt_tag* tmp = NULL;
	tmp = getNBTChild(tag, "id");
	if (tmp == NULL || tmp->id != NBT_TAG_STRING) goto cer;
	te->id = xstrdup(tmp->data.nbt_string, 0);
	tmp = getNBTChild(tag, "x");
	if (tmp == NULL || tmp->id != NBT_TAG_INT) goto cer;
	te->x = tmp->data.nbt_int;
	tmp = getNBTChild(tag, "y");
	if (tmp == NULL || tmp->id != NBT_TAG_INT) goto cer;
	te->y = tmp->data.nbt_int;
	tmp = getNBTChild(tag, "z");
	if (tmp == NULL || tmp->id != NBT_TAG_INT) goto cer;
	te->z = tmp->data.nbt_int;
	te->tick = NULL;
	if (streq_nocase(te->id, "minecraft:chest")) {
		tmp = getNBTChild(tag, "CustomName");
		te->data.chest.inv = xmalloc(sizeof(struct inventory));
		newInventory(te->data.chest.inv, INVTYPE_CHEST, 2, 27);
		te->data.chest.inv->te = te;
		if (tmp == NULL) te->data.chest.inv->title = xstrdup("{\"text\": \"Chest\"}", 0);
		else {
			size_t tl = 17 + strlen(tmp->data.nbt_string);
			te->data.chest.inv->title = xmalloc(tl);
			snprintf(te->data.chest.inv->title, tl, "{\"text\": \"%s\"}", tmp->data.nbt_string);
		}
		tmp = getNBTChild(tag, "Lock");
		if (tmp == NULL) te->data.chest.lock = NULL;
		else te->data.chest.lock = xstrdup(tmp->data.nbt_string, 0);
		struct nbt_tag* items = getNBTChild(tag, "Items");
		if (items == NULL || items->id != NBT_TAG_LIST) {
		} else for (int i = 0; i < items->children_count; i++) {
			struct nbt_tag* nitem = items->children[i];
			if (nitem == NULL || nitem->id != NBT_TAG_COMPOUND) continue;
			tmp = getNBTChild(nitem, "Slot");
			if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
			uint8_t sli = tmp->data.nbt_byte;
			if (sli > 26) continue;
			struct slot* sl = xmalloc(sizeof(struct slot));
			tmp = getNBTChild(nitem, "id");
			if (tmp == NULL || tmp->id != NBT_TAG_STRING) continue;
			sl->item = getItemFromName(tmp->data.nbt_string);
			tmp = getNBTChild(nitem, "Count");
			if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
			sl->itemCount = tmp->data.nbt_byte;
			tmp = getNBTChild(nitem, "Damage");
			if (tmp == NULL || tmp->id != NBT_TAG_SHORT) continue;
			sl->damage = tmp->data.nbt_short;
			tmp = getNBTChild(nitem, "tag");
			if (tmp == NULL || tmp->id != NBT_TAG_COMPOUND) {
				sl->nbt = NULL;
			} else {
				sl->nbt = xmalloc(sizeof(struct nbt_tag));
				duplicateNBT(tmp, sl->nbt);
			}
			te->data.chest.inv->slots[sli] = sl;
		}
	} else if (streq_nocase(te->id, "minecraft:furnace")) {
		te->tick = tetick_furnace;
		tmp = getNBTChild(tag, "CustomName");
		te->data.furnace.inv = xmalloc(sizeof(struct inventory));
		newInventory(te->data.furnace.inv, INVTYPE_CHEST, 2, 27);
		te->data.furnace.inv->te = te;
		if (tmp == NULL) te->data.furnace.inv->title = xstrdup("{\"text\": \"Furnace\"}", 0);
		else {
			size_t tl = 17 + strlen(tmp->data.nbt_string);
			te->data.furnace.inv->title = xmalloc(tl);
			snprintf(te->data.furnace.inv->title, tl, "{\"text\": \"%s\"}", tmp->data.nbt_string);
		}
		tmp = getNBTChild(tag, "Lock");
		if (tmp == NULL) te->data.chest.lock = NULL;
		else te->data.furnace.lock = xstrdup(tmp->data.nbt_string, 0);
		tmp = getNBTChild(tag, "BurnTime");
		if (tmp == NULL) te->data.furnace.burnTime = 0;
		else te->data.furnace.burnTime = tmp->data.nbt_short;
		tmp = getNBTChild(tag, "CookTime");
		if (tmp == NULL) te->data.furnace.cookTime = 0;
		else te->data.furnace.cookTime = tmp->data.nbt_short;
		tmp = getNBTChild(tag, "CookTimeTotal");
		if (tmp == NULL) te->data.furnace.cookTimeTotal = 0;
		else te->data.furnace.cookTimeTotal = tmp->data.nbt_short;
		struct nbt_tag* items = getNBTChild(tag, "Items");
		if (items == NULL || items->id != NBT_TAG_LIST) {
		} else for (int i = 0; i < items->children_count; i++) {
			struct nbt_tag* nitem = items->children[i];
			if (nitem == NULL || nitem->id != NBT_TAG_COMPOUND) continue;
			tmp = getNBTChild(nitem, "Slot");
			if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
			uint8_t sli = tmp->data.nbt_byte;
			if (sli > 26) continue;
			struct slot* sl = xmalloc(sizeof(struct slot));
			tmp = getNBTChild(nitem, "id");
			if (tmp == NULL || tmp->id != NBT_TAG_STRING) continue;
			sl->item = getItemFromName(tmp->data.nbt_string);
			tmp = getNBTChild(nitem, "Count");
			if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
			sl->itemCount = tmp->data.nbt_byte;
			tmp = getNBTChild(nitem, "Damage");
			if (tmp == NULL || tmp->id != NBT_TAG_SHORT) continue;
			sl->damage = tmp->data.nbt_short;
			tmp = getNBTChild(nitem, "tag");
			if (tmp == NULL || tmp->id != NBT_TAG_COMPOUND) {
				sl->nbt = NULL;
			} else {
				sl->nbt = xmalloc(sizeof(struct nbt_tag));
				duplicateNBT(tmp, sl->nbt);
			}
			te->data.furnace.inv->slots[sli] = sl;
		}
		te->data.furnace.lastBurnMax = 0;
	}
	return te;
	cer: ;
	xfree(te->id);
	xfree(te);
	return NULL;
}

struct tile_entity* newTileEntity(char* id, int32_t x, uint8_t y, int32_t z) {
	struct tile_entity* te = xmalloc(sizeof(struct tile_entity));
	te->id = xstrdup(id, 0);
	te->x = x;
	te->y = y;
	te->z = z;
	te->tick = NULL;
	memset(&te->data, 0, sizeof(union tile_entity_data));
	return te;
}

struct nbt_tag* serializeTileEntity(struct tile_entity* te, int forClient) {
	struct nbt_tag* nbt = xmalloc(sizeof(struct nbt_tag));
	nbt->id = NBT_TAG_COMPOUND;
	nbt->name = NULL;
	nbt->children_count = 4;
	if (streq_nocase(te->id, "minecraft:chest") && !forClient) {
		nbt->children_count = 5;
	}
	nbt->children = xmalloc(sizeof(struct nbt_tag*) * nbt->children_count);
	nbt->children[0] = xmalloc(sizeof(struct nbt_tag));
	nbt->children[0]->id = NBT_TAG_STRING;
	nbt->children[0]->name = xstrdup("id", 0);
	nbt->children[0]->data.nbt_string = xstrdup(te->id, 0);
	nbt->children[0]->children_count = 0;
	nbt->children[0]->children = NULL;
	nbt->children[1] = xmalloc(sizeof(struct nbt_tag));
	nbt->children[1]->id = NBT_TAG_INT;
	nbt->children[1]->name = xstrdup("x", 0);
	nbt->children[1]->data.nbt_int = te->x;
	nbt->children[1]->children_count = 0;
	nbt->children[1]->children = NULL;
	nbt->children[2] = xmalloc(sizeof(struct nbt_tag));
	nbt->children[2]->id = NBT_TAG_INT;
	nbt->children[2]->name = xstrdup("y", 0);
	nbt->children[2]->data.nbt_int = te->y;
	nbt->children[2]->children_count = 0;
	nbt->children[2]->children = NULL;
	nbt->children[3] = xmalloc(sizeof(struct nbt_tag));
	nbt->children[3]->id = NBT_TAG_INT;
	nbt->children[3]->name = xstrdup("z", 0);
	nbt->children[3]->data.nbt_int = te->z;
	nbt->children[3]->children_count = 0;
	nbt->children[3]->children = NULL;
	if (streq_nocase(te->id, "minecraft:chest") && !forClient) {
		/*nbt->children[4] = xmalloc(sizeof(struct nbt_tag));
		 nbt->children[4]->id = NBT_TAG_INT;
		 nbt->children[4]->name = xstrdup("z", 0);
		 nbt->children[4]->data.nbt_int = te->z;
		 nbt->children[4]->children_count = 0;
		 nbt->children[4]->children = NULL;
		 */
	}
	return nbt;
}

void freeTileEntity(struct tile_entity* te) {
	if (streq_nocase(te->id, "minecraft:chest")) {
		freeInventory(te->data.chest.inv);
		xfree(te->data.chest.inv);
		xfree(te->data.chest.lock);
	}
	xfree(te->id);
	xfree(te);
}
