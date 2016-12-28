/*
 * craft.c
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#include "crafting.h"

#include "network.h"
#include "block.h"
#include "item.h"
#include <fcntl.h>
#include "xstring.h"
#include <errno.h>
#include "util.h"
#include "json.h"
#include "player.h"

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
	parseJSON(&json, jsf);
	for (size_t i = 0; i < json.child_count; i++) {
		struct json_object* ur = json.children[i];
		struct crafting_recipe* cr = xcalloc(sizeof(struct crafting_recipe));
		struct json_object* tmp = getJSONValue(ur, "shapeless");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		cr->shapeless = (uint8_t) tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "width");
		if ((tmp == NULL || tmp->type != JSON_NUMBER) && !cr->shapeless) goto cerr;
		cr->width = cr->shapeless ? 0 : (uint8_t) tmp->data.number;
		for (size_t x = 0; x < ur->child_count; x++) {
			struct json_object* ir = ur->children[x];
			if (streq_nocase(ir->name, "output")) {
				tmp = getJSONValue(ir, "item_id");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->output.item = (int16_t) tmp->data.number;
				tmp = getJSONValue(ir, "item_damage");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->output.damage = (int16_t) tmp->data.number;
				tmp = getJSONValue(ir, "item_count");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->output.itemCount = (uint8_t) tmp->data.number;
				cr->output.nbt = NULL;
			} else if (startsWith_nocase(ir->name, "slot_")) {
				uint8_t id = atoi(ir->name + 5);
				if (id > 8) goto cerr;
				cr->slot[id] = xmalloc(sizeof(struct slot));
				tmp = getJSONValue(ir, "item_id");
				if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
				cr->slot[id]->item = (int16_t) tmp->data.number;
				tmp = getJSONValue(ir, "item_damage");
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

void add_crafting(struct crafting_recipe* recipe) {
	add_collection(crafting_recipies, recipe);
}
