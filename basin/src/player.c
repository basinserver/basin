/*
 * player.c
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#include "player.h"
#include "entity.h"
#include "accept.h"
#include "util.h"

struct player* newPlayer(struct entity* entity, char* name, struct uuid uuid, struct conn* conn, uint8_t gamemode) {
	struct player* player = xmalloc(sizeof(struct player));
	player->entity = entity;
	player->conn = conn;
	player->name = name;
	player->uuid = uuid;
	player->gamemode = gamemode;
	player->currentItem = 0;
	player->ping = 0;
	player->stage = 0;
	return player;
}

void freePlayer(struct player* player) {
	xfree(player->name);
	xfree(player);
}
