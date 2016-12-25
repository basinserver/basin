/*
 * game.h
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#ifndef GAME_H_
#define GAME_H_

#include "world.h"
#include "entity.h"

void flush_outgoing(struct player* player);

void loadPlayer(struct player* to, struct player* from);

void onInventoryUpdate(struct inventory* inventory, int slot);

void tick_world(struct world* world);

void broadcast(char* text);

#define BEGIN_BROADCAST_DIST(distfrom, dist) for (size_t i = 0; i < distfrom->world->players->size; i++) {struct player* bc_player = (struct player*) distfrom->world->players->data[i];if (bc_player != NULL && entity_distsq(bc_player->entity, distfrom) < dist * dist) {
#define BEGIN_BROADCAST_DISTXYZ(x, y, z, players, dist) for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL && entity_distsq_block(bc_player->entity, x, y, z) < dist * dist) {
#define BEGIN_BROADCAST_EXCEPT_DIST(except, distfrom, dist) for (size_t i = 0; i < distfrom->world->players->size; i++) {struct player* bc_player = (struct player*) distfrom->world->players->data[i];if (bc_player != NULL && bc_player != except && entity_distsq(bc_player->entity, distfrom) < dist * dist) {
#define BEGIN_BROADCAST(players) for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL) {
#define BEGIN_BROADCAST_EXCEPT(players, except) for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL && bc_player != except) {
#define END_BROADCAST }}

#endif /* GAME_H_ */
