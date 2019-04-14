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
#include <basin/player.h>
#include <basin/game.h>
#include <avuna/string.h>
#include <avuna/json.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

void init_crafting() {
	crafting_recipies = new_collection(128, 0);
	char* jsf = xmalloc(4097);
	size_t jsfs = 4096;
	size_t jsr = 0;
	int fd = open("crafting.json", O_RDONLY);
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
		printf("Error reading crafting recipes: %s\n", strerror(errno));
	}
	close(fd);
	struct json_object json;
    json_parse(&json, jsf);
	for (size_t i = 0; i < json.child_count; i++) {
		struct json_object* ur = json.children[i];
		struct crafting_recipe* cr = xcalloc(sizeof(struct crafting_recipe));
		struct json_object* tmp = json_get(ur, "shapeless");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		cr->shapeless = (uint8_t) tmp->type == JSON_TRUE;
		tmp = json_get(ur, "width");
		if ((tmp == NULL || tmp->type != JSON_NUMBER) && !cr->shapeless) goto cerr;
		cr->width = cr->shapeless ? 0 : (uint8_t) tmp->data.number;
		for (size_t x = 0; x < ur->child_count; x++) {
			struct json_object* ir = ur->children[x];
			if (streq_nocase(ir->name, "output")) {
				tmp = json_get(ir, "item_id");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->output.item = (int16_t) tmp->data.number;
				tmp = json_get(ir, "item_damage");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->output.damage = (int16_t) tmp->data.number;
				tmp = json_get(ir, "item_count");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->output.itemCount = (uint8_t) tmp->data.number;
				cr->output.nbt = NULL;
			} else if (startsWith_nocase(ir->name, "slot_")) {
				uint8_t id = atoi(ir->name + 5);
				if (id > 8) goto cerr;
				cr->slot[id] = xmalloc(sizeof(struct slot));
				tmp = json_get(ir, "item_id");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->slot[id]->item = (int16_t) tmp->data.number;
				tmp = json_get(ir, "item_damage");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->slot[id]->damage = (int16_t) tmp->data.number;
				cr->slot[id]->itemCount = 1;
				cr->slot[id]->nbt = NULL;
			}
		}
		add_collection(crafting_recipies, cr);
		continue;
		cerr: ;
		printf("[WARNING] Error Loading Crafting Recipe \"%s\"! Skipped.\n", ur->name);
	}
	freeJSON(&json);
	xfree(jsf);
}

void craftOnce(struct player* player, struct inventory* inv) {
	int cs = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	for (int i = 1; i <= cs; i++) {
		struct slot* invi = getSlot(player, inv, i);
		if (invi == NULL) continue;
		if (--invi->itemCount <= 0) {
			freeSlot(invi);
			xfree(invi);
			invi = NULL;
		}
		inv->slots[i] = invi;
	}
	onInventoryUpdate(player, inv, 1);
}

int craftAll(struct player* player, struct inventory* inv) {
	int cs = inv->type == INVTYPE_PLAYERINVENTORY ? 4 : 9;
	uint8_t ct = 64;
	for (int i = 1; i <= cs; i++) {
		struct slot* invi = getSlot(player, inv, i);
		if (invi == NULL) continue;
		if (invi->itemCount < ct) ct = invi->itemCount;
	}
	for (int i = 1; i <= cs; i++) {
		struct slot* invi = getSlot(player, inv, i);
		if (invi == NULL) continue;
		invi->itemCount -= ct;
		if (invi->itemCount <= 0) {
			freeSlot(invi);
			xfree(invi);
			invi = NULL;
		}
		inv->slots[i] = invi;
	}
	return ct;
}

struct slot* getCraftResult(struct slot** slots, size_t slot_count) { // 012/345/678 or 01/23
	struct slot** os = xmalloc(sizeof(struct slot*) * slot_count);
	memcpy(os, slots, slot_count * sizeof(struct slot*));
	if (slot_count == 4) {
		if (os[0] == NULL && os[1] == NULL) {
			os[0] = os[2];
			os[1] = os[3];
			os[2] = NULL;
			os[3] = NULL;
		}
		if (os[0] == NULL && os[2] == NULL) {
			os[0] = os[1];
			os[2] = os[3];
			os[1] = NULL;
			os[3] = NULL;
		}
	} else if (slot_count == 9) {
		if (!os[3] && !os[4] && !os[5]) {
			memmove(os + 3, os + 6, 3 * sizeof(struct slot*));
			memset(os + 6, 0, 3 * sizeof(struct slot*));
		}
		if (!os[0] && !os[1] && !os[2]) {
			memmove(os, os + 3, 6 * sizeof(struct slot*));
			memset(os + 6, 0, 3 * sizeof(struct slot*));
		}
		if (!os[0] && !os[3] && !os[6]) {
			os[0] = os[1];
			os[3] = os[4];
			os[6] = os[7];
			os[1] = os[2];
			os[4] = os[5];
			os[7] = os[8];
			os[2] = NULL;
			os[5] = NULL;
			os[8] = NULL;
			if (!os[0] && !os[3] && !os[6]) {
				os[0] = os[1];
				os[3] = os[4];
				os[6] = os[7];
				os[1] = NULL;
				os[4] = NULL;
				os[7] = NULL;
			}
		}
	}
//for (int x = 0; x < slot_count; x++) {
//	printf("post %i = %i\n", x, os[x] != NULL);
//}
	for (size_t i = 0; i < crafting_recipies->size; i++) {
		struct crafting_recipe* cr = (struct crafting_recipe*) crafting_recipies->data[i];
		if (cr == NULL || (cr->width > 2 && slot_count <= 4) || (slot_count == 4 && (cr->slot[6] != NULL || cr->slot[7] != NULL || cr->slot[8] != NULL))) continue;
		if (cr->shapeless) { // TODO: optimize
			int gs = 1;
			for (int ls = 0; ls <= slot_count; ls++) {
				struct slot* iv = slots[ls];
				if (iv == NULL) continue;
				int m = 0;
				for (int ri = 0; ri < 9; ri++) {
					struct slot* cv = cr->slot[ri];
					if (itemsStackable2(cv, iv)) {
						m = 1;
						break;
					}
				}
				if (!m) {
					gs = 0;
					break;
				}
			}
			if (gs) for (int ri = 0; ri < 9; ri++) {
				struct slot* cv = cr->slot[ri];
				if (cv == NULL) continue;
				int m = 0;
				for (int ls = 1; ls <= 4; ls++) {
					struct slot* iv = slots[ls];
					if (itemsStackable2(cv, iv)) {
						m = 1;
						break;
					}
				}
				if (!m) {
					gs = 0;
					break;
				}
			}
			if (gs) {
				return &cr->output;
			}
		} else {
			int gs = 1;
			for (int x = 0; x < 3; x++) {
				for (int y = 0; y < 3; y++) {
					struct slot* cri = cr->slot[y * 3 + x];
					struct slot* oi = (slot_count == 4 && (x > 1 || y > 1)) ? NULL : os[y * (slot_count == 4 ? 2 : 3) + x];
					if (!itemsStackable2(cri, oi) && (cri != oi)) {
						gs = 0;
						goto ppbx;
					}
				}
			}
			ppbx: ;
			if (!gs) {
				gs = 1;
				for (int x = 0; x < 3; x++) {
					for (int y = 0; y < 3; y++) {
						struct slot* cri = x >= cr->width ? NULL : cr->slot[y * 3 + ((cr->width - 1) - x)];
						struct slot* oi = (slot_count == 4 && (x > 1 || y > 1)) ? NULL : os[y * (slot_count == 4 ? 2 : 3) + x];
						if (!itemsStackable2(cri, oi) && (cri != oi)) {
							gs = 0;
							goto pbx;
						}
					}
				}
			}
			pbx: ;
			if (gs) {
				return &cr->output;
			}
		}
	}
	return NULL;
}

void add_crafting(struct crafting_recipe* recipe) {
	add_collection(crafting_recipies, recipe);
}
