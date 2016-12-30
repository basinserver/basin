/*
 * player.h
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include "network.h"

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
		uint8_t foodTimer;
		size_t lastTPSCalculation;
		uint32_t tps;
		uint8_t real_onGround;
		float reachDistance;
		uint8_t flightInfraction;
		double ldy;
		size_t lastJump;
		uint32_t offGroundTime;
		uint8_t spawnedIn;
		size_t llTick;
		uint8_t triggerRechunk;
};

struct player* newPlayer(struct entity* entity, char* name, struct uuid, struct conn* conn, uint8_t gamemode);

void kickPlayer(struct player* player, char* message);

int player_onGround(struct player* player);

void player_closeWindow(struct player* player, uint16_t windowID);

float player_getAttackStrength(struct player* player, float adjust);

void teleportPlayer(struct player* player, double x, double y, double z);

struct player* getPlayerByName(char* name);

void setPlayerGamemode(struct player* player, int gamemode);

void freePlayer(struct player* player);

#endif /* PLAYER_H_ */
