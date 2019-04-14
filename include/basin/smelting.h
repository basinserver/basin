/*
 * smelting.h
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#ifndef SMELTING_H_
#define SMELTING_H_

#include <basin/world.h>
#include <basin/entity.h>
#include <basin/inventory.h>
#include <stdint.h>

struct smelting_fuel {
		int16_t id;
		int16_t damage;
		int16_t burnTime;
};

struct smelting_recipe {
		struct slot input;
		struct slot output;
};

int16_t getSmeltingFuelBurnTime(struct slot* slot);

struct slot* getSmeltingOutput(struct slot* input);

void init_smelting();

void add_smelting_fuel(struct smelting_fuel* fuel);

void add_smelting_recipe(struct smelting_recipe* recipe);

#endif /* SMELTING_H_ */
