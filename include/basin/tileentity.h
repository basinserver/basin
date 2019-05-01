/*
 * tileentity.h
 *
 *  Created on: Dec 27, 2016
 *      Author: root
 */

#ifndef TILEENTITY_H_
#define TILEENTITY_H_

#include <basin/nbt.h>
#include <basin/inventory.h>

struct world;

struct tile_entity_banner {

};

struct tile_entity_beacon {

};

struct tile_entity_cauldron {

};

struct tile_entity_brewingstand {

};

struct tile_entity_chest {
        char* lock;
        struct inventory* inv;
        //TODO: loot table
};

struct tile_entity_comparator {

};

struct tile_entity_commandblock {

};

struct tile_entity_daylightdetector {

};

struct tile_entity_dispenser {

};

struct tile_entity_dropper {

};

struct tile_entity_enchantingtable {

};

struct tile_entity_enderchest {

};

struct tile_entity_endgateway {

};

struct tile_entity_endportal {

};

struct tile_entity_flowerpot {

};

struct tile_entity_furnace {
    char* lock;
    struct inventory* inv;
    int16_t burnTime;
    int16_t cookTime;
    int16_t cookTimeTotal;
    int16_t lastBurnMax;
};

struct tile_entity_hopper {

};

struct tile_entity_jukebox {

};

struct tile_entity_mobspawner {

};

struct tile_entity_noteblock {

};

struct tile_entity_piston {

};

struct tile_entity_sign {

};

struct tile_entity_skull {

};

struct tile_entity_structureblock {

};

union tile_entity_data {
    struct tile_entity_banner banner;
    struct tile_entity_beacon beacon;
    struct tile_entity_cauldron cauldron;
    struct tile_entity_brewingstand brewingstand;
    struct tile_entity_chest chest;
    struct tile_entity_comparator comparator;
    struct tile_entity_commandblock commandblock;
    struct tile_entity_daylightdetector daylightdetector;
    struct tile_entity_dispenser dispenser;
    struct tile_entity_dropper dropper;
    struct tile_entity_enchantingtable enchantingtable;
    struct tile_entity_enderchest enderchest;
    struct tile_entity_endgateway endgateway;
    struct tile_entity_endportal endportal;
    struct tile_entity_flowerpot flowerpot;
    struct tile_entity_furnace furnace;
    struct tile_entity_hopper hopper;
    struct tile_entity_jukebox jukebox;
    struct tile_entity_mobspawner mobspawner;
    struct tile_entity_noteblock noteblock;
    struct tile_entity_piston piston;
    struct tile_entity_sign sign;
    struct tile_entity_skull skull;
    struct tile_entity_structureblock structureblock;
};

struct tile_entity {
    char* id;
    struct mempool* pool;
    int32_t x;
    uint8_t y;
    int32_t z;
    void (*tick)(struct world* world, struct tile_entity* te);
    union tile_entity_data data;
};

void tetick_furnace(struct world* world, struct tile_entity* te);

struct tile_entity* tile_parse(struct mempool* parent, struct nbt_tag* tag);

struct tile_entity* tile_new(struct mempool* parent, char* id, int32_t x, uint8_t y, int32_t z);

struct nbt_tag* tile_serialize(struct mempool* parent, struct tile_entity* tile, int forClient);

void freeTileEntity(struct tile_entity* te);

#endif /* TILEENTITY_H_ */
