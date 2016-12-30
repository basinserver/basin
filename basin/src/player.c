/*
 * player.c
 *
 *  Created on: Jun 24, 2016
 *      Author: root
 */

#include "network.h"
#include "entity.h"
#include "accept.h"
#include "util.h"
#include "inventory.h"
#include "xstring.h"
#include "queue.h"
#include "packet.h"
#include "game.h"
#include "collection.h"
#include "player.h"
#include "globals.h"
#include "item.h"
#include "tileentity.h"

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
	player->food = 20;
	player->foodexhaustion = 0.;
	player->foodTick = 0;
	player->nextKeepAlive = 0;
	player->inHand = NULL;
	memset(&player->digging_position, 0, sizeof(struct encpos));
	player->digging = -1.;
	player->digspeed = 0.;
	player->inventory = xmalloc(sizeof(struct inventory));
	newInventory(player->inventory, INVTYPE_PLAYERINVENTORY, 0, 46);
	player->openInv = NULL;
	add_collection(player->inventory->players, player);
	player->loadedChunks = new_collection(0, 0);
	player->loadedEntities = new_collection(0, 0);
	player->incomingPacket = new_queue(0, 1);
	player->outgoingPacket = new_queue(0, 1);
	player->defunct = 0;
	player->lastSwing = tick_counter;
	player->foodTimer = 0;
	return player;
}

float player_getAttackStrength(struct player* player, float adjust) {
	float cooldownPeriod = 4.;
	struct slot* sl = getSlot(player, player->inventory, 36 + player->currentItem);
	if (sl != NULL) {
		struct item_info* ii = getItemInfo(sl->item);
		if (ii != NULL) {
			cooldownPeriod = ii->attackSpeed;
		}
	}
	float str = ((float) (tick_counter - player->lastSwing) + adjust) * cooldownPeriod / 20.;
	if (str > 1.) str = 1.;
	if (str < 0.) str = 0.;
	return str;
}

void teleportPlayer(struct player* player, double x, double y, double z) {
	player->entity->x = x;
	player->entity->y = y;
	player->entity->z = z;
	player->entity->lx = x;
	player->entity->ly = y;
	player->entity->lz = z;
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYVELOCITY;
	pkt->data.play_client.entityvelocity.entity_id = player->entity->id;
	pkt->data.play_client.entityvelocity.velocity_x = 0;
	pkt->data.play_client.entityvelocity.velocity_y = 0;
	pkt->data.play_client.entityvelocity.velocity_z = 0;
	add_queue(player->outgoingPacket, pkt);
	flush_outgoing(player);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_PLAYERPOSITIONANDLOOK;
	pkt->data.play_client.playerpositionandlook.x = player->entity->x;
	pkt->data.play_client.playerpositionandlook.y = player->entity->y;
	pkt->data.play_client.playerpositionandlook.z = player->entity->z;
	pkt->data.play_client.playerpositionandlook.yaw = player->entity->yaw;
	pkt->data.play_client.playerpositionandlook.pitch = player->entity->pitch;
	pkt->data.play_client.playerpositionandlook.flags = 0x0;
	pkt->data.play_client.playerpositionandlook.teleport_id = 0;
	add_queue(player->outgoingPacket, pkt);
	flush_outgoing(player);
}

struct player* getPlayerByName(char* name) {
	for (size_t i = 0; i < players->size; i++) {
		struct player* player = players->data[i];
		if (player != NULL && streq_nocase(name, player->name)) return player;
	}
	return NULL;
}

void player_closeWindow(struct player* player, uint16_t windowID) {
	struct inventory* inv = NULL;
	if (windowID == 0 && player->openInv == NULL) inv = player->inventory;
	else if (player->openInv != NULL && windowID == player->openInv->windowID) inv = player->openInv;
	if (inv != NULL) {
		if (player->inHand != NULL) {
			dropPlayerItem(player, player->inHand);
			freeSlot(player->inHand);
			xfree(player->inHand);
			player->inHand = NULL;
		}
		if (inv->type == INVTYPE_PLAYERINVENTORY) {
			for (int i = 1; i < 5; i++) {
				if (inv->slots[i] != NULL) {
					dropPlayerItem(player, inv->slots[i]);
					setSlot(player, inv, i, NULL, 0, 1);
				}
			}
		} else if (inv->type == INVTYPE_WORKBENCH) {
			for (int i = 1; i < 10; i++) {
				if (inv->slots[i] != NULL) {
					dropPlayerItem(player, inv->slots[i]);
					setSlot(player, inv, i, NULL, 0, 1);
				}
			}
			freeInventory(inv);
			inv = NULL;
		} else if (inv->type == INVTYPE_CHEST) {
			if (inv->te != NULL) {
				BEGIN_BROADCAST_DIST(player->entity, 128.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_BLOCKACTION;
				pkt->data.play_client.blockaction.location.x = inv->te->x;
				pkt->data.play_client.blockaction.location.y = inv->te->y;
				pkt->data.play_client.blockaction.location.z = inv->te->z;
				pkt->data.play_client.blockaction.action_id = 1;
				pkt->data.play_client.blockaction.action_param = inv->players->count - 1;
				pkt->data.play_client.blockaction.block_type = getBlockWorld(player->world, inv->te->x, inv->te->y, inv->te->z) >> 4;
				add_queue(bc_player->outgoingPacket, pkt);
				flush_outgoing (bc_player);
				END_BROADCAST
			}
		}
	}
	if (inv != NULL && inv->type != INVTYPE_PLAYERINVENTORY) rem_collection(inv->players, player);
	player->openInv = NULL; // TODO: free sometimes?

}

void setPlayerGamemode(struct player* player, int gamemode) {
	if (gamemode != -1) {
		player->gamemode = gamemode;
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_CHANGEGAMESTATE;
		pkt->data.play_client.changegamestate.reason = 3;
		pkt->data.play_client.changegamestate.value = gamemode;
		add_queue(player->outgoingPacket, pkt);
		flush_outgoing(player);
	}
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_PLAYERABILITIES;
	pkt->data.play_client.playerabilities.flags = player->gamemode == 1 ? (0x04 | 0x08) : 0x0;
	pkt->data.play_client.playerabilities.flying_speed = .05;
	pkt->data.play_client.playerabilities.field_of_view_modifier = .1;
	add_queue(player->outgoingPacket, pkt);
	flush_outgoing(player);
}

void freePlayer(struct player* player) {
	struct packet* pkt = NULL;
	while ((pkt = pop_nowait_queue(player->incomingPacket)) != NULL) {
		freePacket(STATE_PLAY, 0, pkt);
	}
	del_queue(player->incomingPacket);
	while ((pkt = pop_nowait_queue(player->outgoingPacket)) != NULL) {
		freePacket(STATE_PLAY, 1, pkt);
	}
	if (player->inHand != NULL) {
		freeSlot(player->inHand);
		xfree(player->inHand);
	}
	del_queue(player->outgoingPacket);
	del_collection(player->loadedChunks);
	del_collection(player->loadedEntities);
	freeInventory(player->inventory);
	xfree(player->inventory);
	xfree(player->name);
	xfree(player);
}