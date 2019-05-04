#ifndef BASIN_PLAYER_H_
#define BASIN_PLAYER_H_

#include <basin/network.h>
#include <basin/block.h>
#include <basin/connection.h>
#include <basin/world.h>
#include <avuna/pmem.h>

struct player {

    struct mempool* pool;
    struct server* server;
    struct connection* conn;
    struct world* world;
    struct entity* entity;
    uint32_t protocol_version; 

    char* name;
    struct uuid uuid;

    struct hashmap* loaded_chunks;
    struct hashmap* loaded_entities;
    struct queue* outgoing_packets;
    struct queue* incoming_packets;
    uint32_t next_keep_alive;
    uint8_t spawned_in;
    uint32_t chunks_sent;
    uint8_t trigger_rechunk;

    struct inventory* inventory;
    struct inventory* open_inventory;
    struct slot* inventory_holding;

    uint16_t currentItem;
    uint8_t gamemode;
    float reachDistance;
    uint8_t invulnerable;

    uint8_t ping;
    float walkSpeed;
    float flySpeed;
    uint8_t flying;
    int32_t xpseed;
    int32_t xptotal;
    int32_t xplevel;
    int32_t score;
    int32_t last_teleport_id;
    int8_t sleeping;
    int16_t fire;
    //TODO: enderitems inventory

    struct encpos digging_position;
    float digging;
    float digspeed;

    uint32_t itemUseDuration;
    uint8_t itemUseHand;
    size_t lastSwing;

    int32_t food;
    int32_t foodTick;
    uint8_t foodTimer;
    float foodExhaustion;
    float saturation;
};

struct player* player_new(struct mempool* parent, struct server* server, struct connection* conn, struct world* world, struct entity* entity, char* name, struct uuid uuid, uint8_t gamemode);

void player_hungerUpdate(struct player* player);

void player_send_entity_move(struct player* player, struct entity* entity);

void player_tick(struct player* player);

void player_kick(struct player* player, char* message);

int player_onGround(struct player* player);

void player_closeWindow(struct player* player, uint16_t windowID);

float player_getAttackStrength(struct player* player, float adjust);

void player_teleport(struct player* player, double x, double y, double z);

struct player* player_get_by_name(struct server* server, char* name);

void player_set_gamemode(struct player* player, int gamemode);

block player_can_place_block(struct player* player, uint16_t blk, int32_t x, int32_t y, int32_t z, uint8_t face);

#endif /* BASIN_PLAYER_H_ */
