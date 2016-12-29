/*
 * player.h
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include "inventory.h"
#include "network.h"
#include "accept.h"
#include "collection.h"

struct player {
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
		uint8_t mayfly;
		uint8_t instabuild;
		float walkSpeed;
		float flySpeed;
		uint8_t maybuild;
		uint8_t flying;
		int32_t xpseed;
		int32_t xptotal;
		int32_t xplevel;
		int32_t score;
		float saturation;
		int8_t sleeping;
		int16_t fire;
		//TODO: enderitems & inventory
		int32_t food;
		float foodexhaustion;
		int32_t foodTick;
		int32_t nextKeepAlive;
		struct encpos digging_position;
		float digging;
		float digspeed;
		struct inventory* inventory;
		struct inventory* openInv;
		struct collection* loadedChunks;
		struct collection* loadedEntities;
		struct queue* outgoingPacket;
		struct queue* incomingPacket;
		uint8_t defunct;
		struct slot* inHand;
		size_t lastSwing;
};

struct player* newPlayer(struct entity* entity, char* name, struct uuid, struct conn* conn, uint8_t gamemode);

float player_getAttackStrength(struct player* player, float adjust);

void teleportPlayer(struct player* player, double x, double y, double z);

void freePlayer(struct player* player);

#endif /* PLAYER_H_ */
