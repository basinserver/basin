/*
 * item.c
 *
 *  Created on: Dec 27, 2016
 *      Author: root
 */

#include "smelting.h"
#include "item.h"
#include "collection.h"
#include "json.h"
#include <fcntl.h>
#include "xstring.h"
#include <errno.h>
#include "util.h"

struct collection* smelting_fuels;

struct collection* smelting_recipes;

int16_t getSmeltingFuelBurnTime(struct slot* slot) {
	if (slot == NULL || slot->item < 0) return 0;
	for (size_t i = 0; i < smelting_fuels->size; i++) {
		if (smelting_fuels->data[i] == NULL) continue;
		struct smelting_fuel* sf = (struct smelting_fuel*) smelting_fuels->data[i];
		if (sf->id == slot->item && (sf->damage == -1 || sf->damage == slot->damage)) return sf->burnTime;
	}
	return 0;
}

struct slot* getSmeltingOutput(struct slot* input) {
	if (input == NULL || input->item < 0) return 0;
	for (size_t i = 0; i < smelting_recipes->size; i++) {
		if (smelting_recipes->data[i] == NULL) continue;
		struct smelting_recipe* sf = (struct smelting_recipe*) smelting_recipes->data[i];
		if (itemsStackable2(input, &sf->input)) return &sf->output;
	}
	return 0;
}

void add_smelting_fuel(struct smelting_fuel* fuel) {
	add_collection(smelting_fuels, fuel);
}

void add_smelting_recipe(struct smelting_recipe* recipe) {
	add_collection(smelting_recipes, recipe);
}

void init_smelting() {
	smelting_fuels = new_collection(128, 0);
	smelting_recipes = new_collection(128, 0);
	char* jsf = xmalloc(4097);
	size_t jsfs = 4096;
	size_t jsr = 0;
	int fd = open("smelting.json", O_RDONLY);
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
		printf("Error reading smelting information: %s\n", strerror(errno));
	}
	close(fd);
	struct json_object json;
	parseJSON(&json, jsf);
	struct json_object* fuels = getJSONValue(&json, "fuels");
	if (fuels == NULL || fuels->type != JSON_OBJECT) goto cerr2;
	struct json_object* recipes = getJSONValue(&json, "recipes");
	if (recipes == NULL || recipes->type != JSON_OBJECT) goto cerr2;
	for (size_t i = 0; i < fuels->child_count; i++) {
		struct json_object* ur = fuels->children[i];
		struct smelting_fuel* ii = xcalloc(sizeof(struct smelting_fuel));
		struct json_object* tmp = getJSONValue(ur, "id");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->id = (int16_t) tmp->data.number;
		if (ii->id <= 0) goto cerr;
		tmp = getJSONValue(ur, "damage");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->damage = (int16_t) tmp->data.number;
		tmp = getJSONValue(ur, "burnTime");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ii->burnTime = (int16_t) tmp->data.number;
		add_smelting_fuel(ii);
		continue;
		cerr: ;
		printf("[WARNING] Error Loading Smelting Fuel \"%s\"! Skipped.\n", ur->name);
	}
	for (size_t i = 0; i < recipes->child_count; i++) {
		struct json_object* ur = recipes->children[i];
		struct smelting_recipe* ii = xcalloc(sizeof(struct smelting_recipe));
		struct json_object* tmp = getJSONValue(ur, "input_item");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr3;
		ii->input.item = (int16_t) tmp->data.number;
		if (ii->input.item <= 0) goto cerr3;
		tmp = getJSONValue(ur, "input_damage");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr3;
		ii->input.damage = (int16_t) tmp->data.number;
		ii->input.itemCount = 1;
		ii->input.nbt = NULL;
		tmp = getJSONValue(ur, "output_item");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr3;
		ii->output.item = (int16_t) tmp->data.number;
		if (ii->output.item <= 0) goto cerr3;
		tmp = getJSONValue(ur, "output_damage");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr3;
		ii->output.damage = (int16_t) tmp->data.number;
		tmp = getJSONValue(ur, "output_count");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr3;
		ii->output.itemCount = (uint8_t) tmp->data.number;
		ii->output.nbt = NULL;
		add_smelting_recipe(ii);
		continue;
		cerr3: ;
		printf("[WARNING] Error Loading Smelting Recipe \"%s\"! Skipped.\n", ur->name);
	}
	freeJSON(&json);
	xfree(jsf);
	return;
	cerr2: ;
	freeJSON(&json);
	xfree(jsf);
	printf("[ERROR] Critical Failure Loading 'smelting.json', No Fuels/Recipes Object!\n");
}
