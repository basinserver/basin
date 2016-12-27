/*
 * craft.h
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#ifndef CRAFT_H_
#define CRAFT_H_

#include "collection.h"
#include "network.h"

struct collection* crafting_recipies;

struct crafting_recipe {
		struct slot* slot[9];
		struct slot output;
		uint8_t shapeless;
		uint8_t width;
};

void load_crafting_recipies();

#endif /* CRAFT_H_ */
