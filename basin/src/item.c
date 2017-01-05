/*
 * item.c
 *
 *  Created on: Dec 27, 2016
 *      Author: root
 */

#include "item.h"
#include "collection.h"
#include "json.h"
#include <fcntl.h>
#include "xstring.h"
#include <errno.h>
#include "util.h"
#include <unistd.h>
#include "block.h"

int onItemBreakBlock_tool(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z) {
	if (slot == NULL) return 0;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii == NULL) return 0;
	if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
		slot = NULL;
	}
	setSlot(player, player->inventory, slot_index, slot, 1, 1);
	return 0;
}

float onItemAttacked_tool(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, struct entity* entity) {
	if (slot == NULL) return 1.;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii == NULL) return 1.;
	if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
		slot = NULL;
	}
	setSlot(player, player->inventory, slot_index, slot, 1, 1);
	return ii->damage;
}

void offsetCoordByFace(int32_t* x, uint8_t* y, int32_t* z, uint8_t face) {
	if (face == YN) (*y)--;
	else if (face == YP) (*y)++;
	else if (face == ZN) (*z)--;
	else if (face == ZP) (*z)++;
	else if (face == XN) (*x)--;
	else if (face == XP) (*x)++;
}

int onItemInteract_flintandsteel(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face) {
	if (slot == NULL) return 0;
	offsetCoordByFace(&x, &y, &z, face);
	if (getBlockWorld(world, x, y, z) != 0) return 0;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii == NULL) return 0;
	if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
		slot = NULL;
	}
	setSlot(player, player->inventory, slot_index, slot, 1, 1);
	setBlockWorld(world, BLK_FIRE, x, y, z);
	return 0;
}

int onItemInteract_reeds(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face) {
	if (slot == NULL) return 0;
	offsetCoordByFace(&x, &y, &z, face);
	if (getBlockWorld(world, x, y, z) != 0) return 0;
	if (!canBePlaced_reeds(world, BLK_REEDS, x, y, z)) return 0;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii == NULL) return 0;
	if (player->gamemode != 1 && --slot->itemCount <= 0) {
		slot = NULL;
	}
	setSlot(player, player->inventory, slot_index, slot, 1, 1);
	setBlockWorld(world, BLK_REEDS, x, y, z);
	return 0;
}

int onItemInteract_bucket(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face) {
	if (slot == NULL) return 0;
	offsetCoordByFace(&x, &y, &z, face);
	block b = getBlockWorld(world, x, y, z);
	struct block_info* bi = getBlockInfo(b);
	if (bi == NULL) return 0;
	uint16_t ba = b >> 4;
	if (slot->item == ITM_BUCKETWATER) {
		if (b != 0 && !bi->material->replacable && ba != BLK_WATER >> 4 && ba != BLK_WATER_1 >> 4) return 0;
		setBlockWorld(world, BLK_WATER_1, x, y, z);
		if (player->gamemode != 1) {
			slot->item = ITM_BUCKET;
			setSlot(player, player->inventory, slot_index, slot, 1, 1);
		}
	} else if (slot->item == ITM_BUCKETLAVA) {
		if (b != 0 && !bi->material->replacable && ba != BLK_LAVA >> 4 && ba != BLK_LAVA_1 >> 4) return 0;
		setBlockWorld(world, BLK_LAVA_1, x, y, z);
		if (player->gamemode != 1) {
			slot->item = ITM_BUCKET;
			setSlot(player, player->inventory, slot_index, slot, 1, 1);
		}
	} else if (slot->item == ITM_BUCKET) {
		if ((b & 0x0f) != 0) return 0;
		if (ba == BLK_WATER >> 4 || ba == BLK_WATER_1 >> 4) {
			setBlockWorld(world, 0, x, y, z);
			if (player->gamemode != 1) {
				slot->item = ITM_BUCKETWATER;
				setSlot(player, player->inventory, slot_index, slot, 1, 1);
			}
		} else if (ba == BLK_LAVA >> 4 || ba == BLK_LAVA_1 >> 4) {
			setBlockWorld(world, 0, x, y, z);
			if (player->gamemode != 1) {
				slot->item = ITM_BUCKETLAVA;
				setSlot(player, player->inventory, slot_index, slot, 1, 1);
			}
		}
	}
	return 0;
}

int onItemInteract_bonemeal(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face) {
	if (slot == NULL) return 0;
	block b = getBlockWorld(world, x, y, z);
	uint16_t ba = b >> 4;
	if (ba == (BLK_CROPS >> 4) || (ba == BLK_PUMPKINSTEM >> 4) || (ba == BLK_PUMPKINSTEM_1 >> 4) || (ba == BLK_CARROTS >> 4) || (ba == BLK_POTATOES >> 4) || (ba == BLK_BEETROOTS >> 4) || (ba == BLK_COCOA >> 4)) {
		uint16_t grow = 2 + rand() % 3;
		uint16_t maxAge = 7;
		if (b == BLK_BEETROOTS) maxAge = 3;
		uint16_t age = b & 0x0f;
		age += grow;
		if (age > maxAge) age = maxAge;
		setBlockWorld(world, (ba << 4) | age, x, y, z);
	} else if (ba == (BLK_SAPLING_OAK >> 4)) {
		randomTick_sapling(world, getChunk(world, x >> 4, z >> 4), b, x, y, z);
	} else if (b == BLK_GRASS || b == BLK_DIRT) {
		for (int i = 0; i < 128; i++) {
			int j = 0;
			int32_t bx = x;
			uint8_t by = y + 1;
			int32_t bz = z;
			while (1) {
				block bb = getBlockWorld(world, bx, by - 1, bz);
				block b = getBlockWorld(world, bx, by, bz);
				struct block_info* bi = getBlockInfo(b);
				if (j >= i / 16) {
					if (streq_nocase(bi->material->name, "air")) {
						if (rand() % 8 == 0) {
							block nb = rand() % 3 > 0 ? BLK_FLOWER1_DANDELION : BLK_FLOWER2_POPPY; // TODO: biome registry
							if (bb == BLK_DIRT || bb == BLK_GRASS) {
								setBlockWorld(world, nb, bx, by, bz);
							}
						} else {
							block nb = BLK_TALLGRASS_GRASS;
							if (bb == BLK_DIRT || bb == BLK_GRASS) {
								setBlockWorld(world, nb, bx, by, bz);
							}
						}
					}
					break;
				}
				bx += rand() % 3 - 1;
				by += (rand() % 3 - 1) * (rand() % 3) / 2;
				bz += rand() % 3 - 1;
				if (bb != BLK_GRASS || isNormalCube(bi)) break;
				j++;
			}
		}
	} else return 0;
	if (player->gamemode != 1 && --slot->itemCount <= 0) {
		slot = NULL;
	}
	setSlot(player, player->inventory, slot_index, slot, 1, 1);
	return 0;
}

int onItemInteract_seeds(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face) {
	if (slot == NULL) return 0;
	block b = getBlockWorld(world, x, y, z);
	if (slot->item == ITM_DYEPOWDER_BLACK) {
		if (slot->damage == 3 && (face == 1 || face == 0 || b != BLK_LOG_JUNGLE)) return 0;
		if (slot->damage == 15) return onItemInteract_bonemeal(world, player, slot_index, slot, x, y, z, face);
		else if (slot->damage != 3) return 0;
	} else if (face != 1 || (b >> 4) != (slot->item == ITM_NETHERSTALKSEEDS ? (BLK_HELLSAND >> 4) : (BLK_FARMLAND >> 4))) return 0;
	offsetCoordByFace(&x, &y, &z, face);
	b = getBlockWorld(world, x, y, z);
	if (b != 0) return 0;
	block tp = 0;
	if (slot->item == ITM_SEEDS) tp = BLK_CROPS;
	else if (slot->item == ITM_SEEDS_PUMPKIN) tp = BLK_PUMPKINSTEM;
	else if (slot->item == ITM_SEEDS_MELON) tp = BLK_PUMPKINSTEM_1;
	else if (slot->item == ITM_CARROTS) tp = BLK_CARROTS;
	else if (slot->item == ITM_POTATO) tp = BLK_POTATOES;
	else if (slot->item == ITM_NETHERSTALKSEEDS) tp = BLK_NETHERSTALK;
	else if (slot->item == ITM_BEETROOT_SEEDS) tp = BLK_BEETROOTS;
	else if (slot->item == ITM_DYEPOWDER_BLACK) tp = BLK_COCOA;
	if (player->gamemode != 1 && --slot->itemCount <= 0) {
		slot = NULL;
	}
	setSlot(player, player->inventory, slot_index, slot, 1, 1);
	setBlockWorld(world, tp, x, y, z);
	return 0;
}

int onItemInteract_hoe(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face) {
	if (slot == NULL) return 0;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii == NULL) return 0;
	block b = getBlockWorld(world, x, y, z);
	if (b == BLK_DIRT || b == BLK_GRASS) {
		if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
			slot = NULL;
		}
		setSlot(player, player->inventory, slot_index, slot, 1, 1);
		setBlockWorld(world, BLK_FARMLAND, x, y, z);
	}
	return 0;
}

int onItemInteract_shovel(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face) {
	if (slot == NULL) return 0;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii == NULL) return 0;
	block b = getBlockWorld(world, x, y, z);
	if (b == BLK_GRASS) {
		if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
			slot = NULL;
		}
		setSlot(player, player->inventory, slot_index, slot, 1, 1);
		setBlockWorld(world, BLK_GRASSPATH, x, y, z);
	}
	return 0;
}

int onItemUse_armor(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, uint16_t ticks) {
	if (slot == NULL) return 0;
	uint16_t sli = 5;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii->armorType == ARMOR_HELMET) sli = 5;
	else if (ii->armorType == ARMOR_CHESTPLATE) sli = 6;
	else if (ii->armorType == ARMOR_LEGGINGS) sli = 7;
	else if (ii->armorType == ARMOR_BOOTS) sli = 8;
	if (getSlot(player, player->inventory, sli) != NULL) return 0;
	setSlot(player, player->inventory, sli, slot, 1, 1);
	setSlot(player, player->inventory, slot_index, NULL, 1, 0);
}

float onEntityHitWhileWearing_armor(struct world* world, struct player* player, uint8_t slot_index, struct slot* slot, float damage) {
	if (slot == NULL) return damage;
	struct item_info* ii = getItemInfo(slot->item);
	if (ii == NULL) return damage;
	if (player->gamemode != 1 && ++slot->damage >= ii->maxDamage) {
		slot = NULL;
	}
	setSlot(player, player->inventory, slot_index, slot, 1, 1);
	return damage;
}

struct collection* item_infos;

struct item_info* getItemInfo(item id) {
	if (id < 0 || id > item_infos->size) return NULL;
	return item_infos->data[id];
}

void init_items() {
	item_infos = new_collection(128, 0);
	char* jsf = xmalloc(4097);
	size_t jsfs = 4096;
	size_t jsr = 0;
	int fd = open("items.json", O_RDONLY);
	ssize_t r = 0;
	while ((r = read(fd, jsf + jsr, jsfs - jsr)) > 0) {
		jsr += r;
		if (jsfs - jsr < 512) {
			jsfs += 4096;
			jsf = xrealloc(jsf, jsfs + 1);
		}
	}
	jsf[jsr] = 0;
	if (r < 0) {
		printf("Error reading item information: %s\n", strerror(errno));
	}
	close(fd);
	struct json_object json;
	parseJSON(&json, jsf);
	for (size_t i = 0; i < json.child_count; i++) {
		struct json_object* ur = json.children[i];
		struct item_info* ii = xcalloc(sizeof(struct item_info));
		struct json_object* tmp = getJSONValue(ur, "id");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		item id = (item) tmp->data.number;
		if (id <= 0) goto cerr;
		tmp = getJSONValue(ur, "toolType");
		if (tmp == NULL || tmp->type != JSON_STRING) goto cerr;
		if (streq_nocase(tmp->data.string, "none")) ii->toolType = NULL;
		else ii->toolType = getToolInfo(tmp->data.string);
		tmp = getJSONValue(ur, "harvestLevel");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->harvestLevel = (uint8_t) tmp->data.number;
		tmp = getJSONValue(ur, "maxStackSize");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->maxStackSize = (uint8_t) tmp->data.number;
		if (ii->maxStackSize > 64) goto cerr;
		tmp = getJSONValue(ur, "maxDamage");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->maxDamage = (int16_t) tmp->data.number;
		tmp = getJSONValue(ur, "toolProficiency");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->toolProficiency = (float) tmp->data.number;
		tmp = getJSONValue(ur, "damage");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->damage = (float) tmp->data.number;
		tmp = getJSONValue(ur, "armorType");
		if (tmp == NULL || tmp->type != JSON_STRING) goto cerr;
		if (streq_nocase(tmp->data.string, "none")) ii->armorType = ARMOR_NONE;
		else if (streq_nocase(tmp->data.string, "helmet")) ii->armorType = ARMOR_HELMET;
		else if (streq_nocase(tmp->data.string, "chestplate")) ii->armorType = ARMOR_CHESTPLATE;
		else if (streq_nocase(tmp->data.string, "leggings")) ii->armorType = ARMOR_LEGGINGS;
		else if (streq_nocase(tmp->data.string, "boots")) ii->armorType = ARMOR_BOOTS;
		else goto cerr;
		tmp = getJSONValue(ur, "armor");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->armor = (uint8_t) tmp->data.number;
		tmp = getJSONValue(ur, "attackSpeed");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->attackSpeed = (float) tmp->data.number;
		ensure_collection(item_infos, id + 1);
		item_infos->data[id] = ii;
		if (item_infos->size < id) item_infos->size = id;
		item_infos->count++;
		continue;
		cerr: ;
		printf("[WARNING] Error Loading Item \"%s\"! Skipped.\n", ur->name);
	}
	freeJSON(&json);
	xfree(jsf);
//
	for (item i = ITM_SHOVELIRON; i <= ITM_HATCHETIRON; i++) {
		getItemInfo(i)->onItemBreakBlock = &onItemBreakBlock_tool;
		getItemInfo(i)->onItemAttacked = &onItemAttacked_tool;
	}
	getItemInfo(ITM_FLINTANDSTEEL)->onItemInteract = &onItemInteract_flintandsteel;
	for (item i = ITM_SWORDIRON; i <= ITM_HATCHETDIAMOND; i++) {
		getItemInfo(i)->onItemBreakBlock = &onItemBreakBlock_tool;
		getItemInfo(i)->onItemAttacked = &onItemAttacked_tool;
	}
	for (item i = ITM_SWORDGOLD; i <= ITM_HATCHETGOLD; i++) {
		getItemInfo(i)->onItemBreakBlock = &onItemBreakBlock_tool;
		getItemInfo(i)->onItemAttacked = &onItemAttacked_tool;
	}
	getItemInfo(ITM_SHEARS)->onItemBreakBlock = &onItemBreakBlock_tool;
	for (item i = ITM_HOEWOOD; i <= ITM_HOEGOLD; i++) {
		getItemInfo(i)->onItemInteract = &onItemInteract_hoe;
	}
	for (item i = ITM_HELMETCLOTH; i <= ITM_BOOTSGOLD; i++) {
		getItemInfo(i)->onEntityHitWhileWearing = &onEntityHitWhileWearing_armor;
		getItemInfo(i)->onItemUse = &onItemUse_armor;
	}
	getItemInfo(ITM_SEEDS)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_SEEDS_PUMPKIN)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_SEEDS_MELON)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_CARROTS)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_POTATO)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_NETHERSTALKSEEDS)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_BEETROOT_SEEDS)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_DYEPOWDER_BLACK)->onItemInteract = &onItemInteract_seeds;
	getItemInfo(ITM_SHOVELWOOD)->onItemInteract = &onItemInteract_shovel;
	getItemInfo(ITM_SHOVELGOLD)->onItemInteract = &onItemInteract_shovel;
	getItemInfo(ITM_SHOVELSTONE)->onItemInteract = &onItemInteract_shovel;
	getItemInfo(ITM_SHOVELIRON)->onItemInteract = &onItemInteract_shovel;
	getItemInfo(ITM_SHOVELDIAMOND)->onItemInteract = &onItemInteract_shovel;
	getItemInfo(ITM_REEDS)->onItemInteract = &onItemInteract_reeds;
	getItemInfo(ITM_BUCKET)->onItemInteract = &onItemInteract_bucket;
	getItemInfo(ITM_BUCKETWATER)->onItemInteract = &onItemInteract_bucket;
	getItemInfo(ITM_BUCKETLAVA)->onItemInteract = &onItemInteract_bucket;

}

void add_item(item id, struct item_info* info) {
	ensure_collection(item_infos, id + 1);
	if (item_infos->data[id] != NULL) {
		xfree(item_infos->data[id]);
	}
	item_infos->data[id] = info;
	if (item_infos->size < id) item_infos->size = id;
	item_infos->count++;
}
