/*
 * craft.h
 *
 *  Created on: Dec 26, 2016
 *      Author: root
 */

#ifndef BASIN_CRAFTING_H_
#define BASIN_CRAFTING_H_

#include <basin/network.h>
#include <basin/inventory.h>
#include <avuna/list.h>

struct list* crafting_recipies;

struct crafting_recipe {
    struct slot* slot[9];
    struct slot output;
    uint8_t shapeless;
    uint8_t width;
};

void crafting_init();

void crafting_once(struct player* player, struct inventory* inv);

int crafting_all(struct player* player, struct inventory* inv);

struct slot* crafting_result(struct mempool* pool, struct slot** slots, size_t slot_count); // 012/345/678 or 01/23

#endif /* BASIN_CRAFTING_H_ */
