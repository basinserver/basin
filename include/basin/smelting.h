#ifndef BASIN_SMELTING_H_
#define BASIN_SMELTING_H_

#include <basin/world.h>
#include <basin/entity.h>
#include <basin/inventory.h>
#include <stdint.h>

struct smelting_fuel {
    int16_t id;
    int16_t damage;
    int16_t burn_time;
};

struct smelting_recipe {
    struct slot input;
    struct slot output;
};

int16_t smelting_burnTime(struct slot* slot);

struct slot* smelting_output(struct slot* input);

void init_smelting();

void add_smelting_fuel(struct smelting_fuel* fuel);

void add_smelting_recipe(struct smelting_recipe* recipe);

#endif /* BASIN_SMELTING_H_ */
