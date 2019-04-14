/*
 * craft.h
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#ifndef CRAFTING_H_
#define CRAFTING_H_

#include <basin/network.h>

struct collection* crafting_recipies;

struct crafting_recipe {
		struct slot* slot[9];
		struct slot output;
		uint8_t shapeless;
		uint8_t width;
};

void init_crafting();

void craftOnce(struct player* player, struct inventory* inv);

int craftAll(struct player* player, struct inventory* inv);

struct slot* getCraftResult(struct slot** slots, size_t slot_count);

void add_crafting(struct crafting_recipe* recipe);

#endif /* CRAFTING_H_ */
