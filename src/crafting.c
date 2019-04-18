/*
 * craft.c
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#include <basin/crafting.h>
#include <basin/network.h>
#include <basin/item.h>
#include <basin/block.h>
#include <basin/globals.h>
#include <basin/player.h>
#include <basin/game.h>
#include <avuna/util.h>
#include <avuna/string.h>
#include <avuna/json.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

struct mempool* crafting_pool;

int _crafting_parse_slot(struct slot* slot, struct json_object* json, int allow_count) {
	struct json_object* id_json = json_get(json, "item_id");
	if (id_json == NULL || id_json->type != JSON_NUMBER) {
		return 1;
	}
	slot->item = (int16_t) id_json->data.number;
	struct json_object* damage_json = json_get(json, "item_damage");
	if (damage_json == NULL || damage_json->type != JSON_NUMBER) {
		return 1;
	}
	slot->damage = (int16_t) damage_json->data.number;
	if (allow_count) {
		struct json_object* count_json = json_get(json, "item_count");
		if (count_json == NULL || count_json->type != JSON_NUMBER) {
			return 1;
		}
		slot->count = (uint8_t) count_json->data.number;
	} else {
		slot->count = 1;
	}
	slot->nbt = NULL;
	return 0;
}

void crafting_init() {
	crafting_pool = mempool_new();
	crafting_recipies = list_new(128, crafting_pool);
	char* json_file = (char*) read_file_fully(crafting_pool, "crafting.json", NULL);
	if (json_file == NULL) {
		errlog(delog, "Error reading crafting data: %s\n", strerror(errno));
		return;
	}
	struct json_object* json = NULL;
	json_parse(crafting_pool, &json, json_file);
	pprefree(crafting_pool, json_file);
	ITER_LLIST(json->children_list, value) {
	    struct json_object* child_json = value;
	    struct crafting_recipe* recipe = pcalloc(crafting_pool, sizeof(struct crafting_recipe));

	    struct json_object* shapeless_json = json_get(child_json, "shapeless");
	    if (shapeless_json == NULL || (shapeless_json->type != JSON_TRUE && shapeless_json->type != JSON_FALSE)) {
	        goto crafting_format_error;
	    }
	    recipe->shapeless = (uint8_t) (shapeless_json->type == JSON_TRUE);
	    struct json_object* width_json = json_get(child_json, "width");
		if ((width_json == NULL || width_json->type != JSON_NUMBER) && !recipe->shapeless) {
			goto crafting_format_error;
		}
		recipe->width = (uint8_t) (recipe->shapeless ? 0 : width_json->data.number);
		ITER_LLIST(child_json->children_list, item_value) {
			struct json_object* item_json = item_value;
			if (str_eq(item_json->name, "output")) {
				if (_crafting_parse_slot(&recipe->output, item_json, 1)) {
					goto crafting_format_error;
				}
			} else if (str_prefixes(item_json->name, "slot_")) {
				uint8_t id = (uint8_t) strtoul(item_json->name + 5, NULL, 10);
				if (id > 8) {
					goto crafting_format_error;
				}
				recipe->slot[id] = pcalloc(crafting_pool, sizeof(struct slot));
				if (_crafting_parse_slot(recipe->slot[id], item_json, 0)) {
					goto crafting_format_error;
				}
			}
			ITER_LLIST_END();
		}
        list_append(crafting_recipies, recipe);
        continue;
	    crafting_format_error:;
        printf("[WARNING] Error Loading Crafting Recipe \"%s\"! Skipped.\n", child_json->name);
        ITER_LLIST_END();
	}
	pfree(json->pool);
}

void crafting_once(struct player* player, struct inventory* inv) {
	int cap = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	for (int i = 1; i <= cap; i++) {
		struct slot* slot = inventory_get(player, inv, i);
		if (slot == NULL) continue;
		if (--slot->count <= 0) {
			if (slot->nbt != NULL) {
				pfree(slot->nbt->pool);
			}
			slot = NULL;
		}
		inv->slots[i] = slot;
	}
	onInventoryUpdate(player, inv, 1);
}

int crafting_all(struct player* player, struct inventory* inv) {
	int cap = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	uint8_t count = 64;
	for (int i = 1; i <= cap; i++) {
		struct slot* slot = inventory_get(player, inv, i);
		if (slot == NULL) continue;
		if (slot->count < count) count = slot->count;
	}
	for (int i = 1; i <= cap; i++) {
		struct slot* slot = inventory_get(player, inv, i);
		if (slot == NULL) continue;
		slot->count -= count;
		if (slot->count <= 0) {
			if (slot->nbt != NULL) {
				pfree(slot->nbt->pool);
			}
			slot = NULL;
		}
		inv->slots[i] = slot;
	}
	return count;
}

struct slot* crafting_result(struct mempool* pool, struct slot** slots, size_t slot_count) { // 012/345/678 or 01/23
	struct slot** new_slots = pmalloc(pool, sizeof(struct slot*) * slot_count);
	memcpy(new_slots, slots, slot_count * sizeof(struct slot*));
	if (slot_count == 4) {
		if (new_slots[0] == NULL && new_slots[1] == NULL) {
			new_slots[0] = new_slots[2];
			new_slots[1] = new_slots[3];
			new_slots[2] = NULL;
			new_slots[3] = NULL;
		}
		if (new_slots[0] == NULL && new_slots[2] == NULL) {
			new_slots[0] = new_slots[1];
			new_slots[2] = new_slots[3];
			new_slots[1] = NULL;
			new_slots[3] = NULL;
		}
	} else if (slot_count == 9) {
		if (!new_slots[3] && !new_slots[4] && !new_slots[5]) {
			memmove(new_slots + 3, new_slots + 6, 3 * sizeof(struct slot*));
			memset(new_slots + 6, 0, 3 * sizeof(struct slot*));
		}
		if (!new_slots[0] && !new_slots[1] && !new_slots[2]) {
			memmove(new_slots, new_slots + 3, 6 * sizeof(struct slot*));
			memset(new_slots + 6, 0, 3 * sizeof(struct slot*));
		}
		if (!new_slots[0] && !new_slots[3] && !new_slots[6]) {
			new_slots[0] = new_slots[1];
			new_slots[3] = new_slots[4];
			new_slots[6] = new_slots[7];
			new_slots[1] = new_slots[2];
			new_slots[4] = new_slots[5];
			new_slots[7] = new_slots[8];
			new_slots[2] = NULL;
			new_slots[5] = NULL;
			new_slots[8] = NULL;
			if (!new_slots[0] && !new_slots[3] && !new_slots[6]) {
				new_slots[0] = new_slots[1];
				new_slots[3] = new_slots[4];
				new_slots[6] = new_slots[7];
				new_slots[1] = NULL;
				new_slots[4] = NULL;
				new_slots[7] = NULL;
			}
		}
	}

	for (size_t i = 0; i < crafting_recipies->size; i++) {
		struct crafting_recipe* recipe = (struct crafting_recipe*) crafting_recipies->data[i];
		if (recipe == NULL || (recipe->width > 2 && slot_count <= 4) || (slot_count == 4 && (recipe->slot[6] != NULL || recipe->slot[7] != NULL || recipe->slot[8] != NULL))) continue;
		if (recipe->shapeless) { // TODO: optimize
			int matching = 1;
			for (int slot_index = 0; slot_index <= slot_count; slot_index++) {
				struct slot* slot = slots[slot_index];
				if (slot == NULL) continue;
				int local_matched = 0;
				for (int ri = 0; ri < 9; ri++) {
					struct slot* local_slot = recipe->slot[ri];
					if (slot_stackable_damage_ignore(local_slot, slot)) {
						local_matched = 1;
						break;
					}
				}
				if (!local_matched) {
					matching = 0;
					break;
				}
			}
			if (matching) {
				for (int slot_index_2 = 0; slot_index_2 < 9; slot_index_2++) {
					struct slot* slot = recipe->slot[slot_index_2];
					if (slot == NULL) continue;
					int local_matched = 0;
					for (int ls = 1; ls <= 4; ls++) {
						struct slot* local_slot = slots[ls];
						if (slot_stackable_damage_ignore(slot, local_slot)) {
							local_matched = 1;
							break;
						}
					}
					if (!local_matched) {
						matching = 0;
						break;
					}
				}
			}
			if (matching) {
				return &recipe->output;
			}
		} else {
			int matching = 1;
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					struct slot* recipe_item = recipe->slot[y * 3 + x];
					struct slot* item = (slot_count == 4 && (x > 1 || y > 1)) ? NULL : new_slots[y * (slot_count == 4 ? 2 : 3) + x];
					if (!slot_stackable_damage_ignore(recipe_item, item) && (recipe_item != item)) {
						matching = 0;
						goto post_stage_1;
					}
				}
			}
			post_stage_1: ;
			if (!matching) {
				matching = 1;
				for (int x = 0; x < 3; x++) {
					for (int y = 0; y < 3; y++) {
						struct slot* recipe_item = x >= recipe->width ? NULL : recipe->slot[y * 3 + ((recipe->width - 1) - x)];
						struct slot* item = (slot_count == 4 && (x > 1 || y > 1)) ? NULL : new_slots[y * (slot_count == 4 ? 2 : 3) + x];
						if (!slot_stackable_damage_ignore(recipe_item, item) && (recipe_item != item)) {
							matching = 0;
							goto post_stage_2;
						}
					}
				}
			}
			post_stage_2: ;
			if (matching) {
				return &recipe->output;
			}
		}
	}
	return NULL;
}
