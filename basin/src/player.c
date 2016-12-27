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
#include "network.h"
#include "inventory.h"
#include "xstring.h"
#include "queue.h"

struct player* newPlayer(struct entity* entity, char* name, struct uuid uuid, struct conn* conn, uint8_t gamemode) {
	struct player* player = xmalloc(sizeof(struct player));
	entity->data.player.player = player;
	player->entity = entity;
	player->conn = conn;
	player->name = name;
	player->uuid = uuid;
	player->gamemode = gamemode;
	player->currentItem = 0;
	player->ping = 0;
	player->stage = 0;
	player->invulnerable = 0;
	player->mayfly = 0;
	player->instabuild = 0;
	player->walkSpeed = 0;
	player->flySpeed = 0;
	player->maybuild = 0;
	player->flying = 0;
	player->xpseed = 0;
	player->xptotal = 0;
	player->xplevel = 0;
	player->score = 0;
	player->saturation = 0.;
	player->sleeping = 0;
	player->fire = 0;
	//TODO: enderitems & inventory
	player->food = 0;
	player->foodexhaustion = 0.;
	player->foodTick = 0;
	player->nextKeepAlive = 0;
	memset(&player->digging_position, 0, sizeof(struct encpos));
	player->digging = -1.;
	player->digspeed = 0.;
	newInventory(&player->inventory, INVTYPE_PLAYERINVENTORY, 0, 46);
	player->openInv = NULL;
	add_collection(player->inventory.players, player);
	player->loadedChunks = new_collection(0, 0);
	player->loadedEntities = new_collection(0, 0);
	player->incomingPacket = new_queue(0, 1);
	player->outgoingPacket = new_queue(0, 1);
	return player;
}

void freePlayer(struct player* player) {
	del_queue(player->incomingPacket);
	del_queue(player->outgoingPacket);
	del_collection(player->loadedChunks);
	del_collection(player->loadedEntities);
	freeInventory(&player->inventory);
	xfree(player->name);
	xfree(player);
}
