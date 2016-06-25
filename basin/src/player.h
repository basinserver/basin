/*
 * player.h
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include "network.h"
#include "accept.h"

struct player {
		struct entity* entity;
		char* name;
		struct uuid uuid;
		struct conn* conn;
		uint16_t currentItem;
		uint8_t gamemode;
		uint8_t ping;
		uint8_t stage;
};

struct player* newPlayer(struct entity* entity, char* name, struct uuid, struct conn* conn, uint8_t gamemode);

void freePlayer(struct player* player);

#endif /* PLAYER_H_ */
