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
