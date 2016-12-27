/*
 * craft.c
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#include "craft.h"
#include "block.h"
#include "item.h"
#include "network.h"
#include <fcntl.h>
#include "xstring.h"
#include <errno.h>
#include "util.h"
#include "json.h"

void load_crafting_recipies() {
	crafting_recipies = new_collection(0, 0);
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
		cr->shapeless = (uint8_t) getJSONValue(ur, "shapeless")->type == JSON_TRUE;
		cr->width = cr->shapeless ? 0 : (uint8_t) getJSONValue(ur, "width")->data.number;
		for (size_t x = 0; x < ur->child_count; x++) {
			struct json_object* ir = ur->children[x];
			if (streq_nocase(ir->name, "output")) {
				cr->output.item = (int16_t) getJSONValue(ir, "item_id")->data.number;
				cr->output.damage = (int16_t) getJSONValue(ir, "item_damage")->data.number;
				cr->output.itemCount = (uint8_t) getJSONValue(ir, "item_count")->data.number;
				cr->output.nbt = NULL;
			} else if (startsWith_nocase(ir->name, "slot_")) {
				uint8_t id = atoi(ir->name + 5);
				if (id > 8) continue;
				cr->slot[id] = xmalloc(sizeof(struct slot));
				cr->slot[id]->item = (int16_t) getJSONValue(ir, "item_id")->data.number;
				cr->slot[id]->damage = (int16_t) getJSONValue(ir, "item_damage")->data.number;
				cr->slot[id]->itemCount = 1;
				cr->slot[id]->nbt = NULL;
			}
		}
		add_collection(crafting_recipies, cr);
	}
	freeJSON(&json);
	xfree(jsf);
}
