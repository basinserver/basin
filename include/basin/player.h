/*
 * player.h
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include <basin/network.h>
#include <basin/block.h>

typedef struct _acstate {
} acstate_t;

struct player {
	struct mempool* pool;
		struct entity* entity;
		struct world* world;
		char* name;
		struct uuid uuid;
		struct conn* conn;
		uint16_t currentItem;
		uint8_t gamemode;
		uint8_t ping;
		uint8_t stage;
		uint8_t invulnerable;
		float walkSpeed;
		float flySpeed;
		uint8_t flying;
		int32_t xpseed;
		int32_t xptotal;
		int32_t xplevel;
		int32_t score;
		float saturation;
		int8_t sleeping;
		int16_t fire;
		uint32_t itemUseDuration;
		uint8_t itemUseHand;
		//TODO: enderitems inv
		int32_t food;
		int32_t foodTick;
		int32_t nextKeepAlive;
		struct encpos digging_position;
		float digging;
		float digspeed;
		struct inventory* inventory;
		struct inventory* openInv;
		struct hashmap* loadedChunks;
		struct hashmap* loadedEntities;
		struct queue* outgoingPacket;
		struct queue* incomingPacket;
		uint8_t defunct;
		struct slot* inHand;
		size_t lastSwing;
		uint8_t foodTimer;
		uint8_t spawnedIn;
		size_t llTick;
		uint8_t triggerRechunk;
		uint16_t chunksSent;
		float reachDistance;
		acstate_t acstate;
		struct queue* chunkRequests;
		float foodExhaustion;
		size_t lastTeleportID;
};

void player_hungerUpdate(struct player* player);

void player_send_entity_move(struct player* player, struct entity* ent);

struct player* player_new(struct entity* entity, char* name, struct uuid uuid, struct conn* conn, uint8_t gamemode);

void player_receive_packet(struct player* player, struct packet* inp);

void player_tick(struct world* world, struct player* player);

void player_kick(struct player* player, char* message);

int player_onGround(struct player* player);

void player_closeWindow(struct player* player, uint16_t windowID);

float player_getAttackStrength(struct player* player, float adjust);

void player_teleport(struct player* player, double x, double y, double z);

struct player* player_get_by_name(char* name);

void player_set_gamemode(struct player* player, int gamemode);

void player_free(struct player* player);

block player_can_place_block(struct player* player, uint16_t blk, int32_t x, int32_t y, int32_t z, uint8_t face);

#endif /* PLAYER_H_ */
