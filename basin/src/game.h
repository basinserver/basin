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

void player_openInventory(struct player* player, struct inventory* inv);

void flush_outgoing(struct player* player);

float randFloat();

void playSound(struct world* world, int32_t soundID, int32_t soundCategory, float x, float y, float z, float volume, float pitch);

void dropPlayerItem(struct player* player, struct slot* drop);

void dropPlayerItem_explode(struct player* player, struct slot* drop);

void dropBlockDrop(struct world* world, struct slot* slot, int32_t x, int32_t y, int32_t z);

void dropBlockDrops(struct world* world, block blk, struct player* breaker, int32_t x, int32_t y, int32_t z);

void loadPlayer(struct player* to, struct player* from);

void loadEntity(struct player* to, struct entity* from);

void onInventoryUpdate(struct player* player, struct inventory* inv, int slot);

void tick_world(struct world* world);

void sendMessageToPlayer(struct player* player, char* text);

void sendMsgToPlayerf(struct player* player, char* fmt, ...);

void broadcast(char* text);

void broadcastf(char* fmt, ...);

#define BEGIN_BROADCAST_DIST(distfrom, dist) for (size_t i = 0; i < distfrom->world->players->size; i++) {struct player* bc_player = (struct player*) distfrom->world->players->data[i];if (bc_player != NULL && entity_distsq(bc_player->entity, distfrom) < dist * dist) {
#define BEGIN_BROADCAST_DISTXYZ(x, y, z, players, dist) for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL && entity_distsq_block(bc_player->entity, x, y, z) < dist * dist) {
#define BEGIN_BROADCAST_EXCEPT_DIST(except, distfrom, dist) for (size_t i = 0; i < distfrom->world->players->size; i++) {struct player* bc_player = (struct player*) distfrom->world->players->data[i];if (bc_player != NULL && bc_player != except && entity_distsq(bc_player->entity, distfrom) < dist * dist) {
#define BEGIN_BROADCAST(players) for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL) {
#define BEGIN_BROADCAST_EXCEPT(players, except) for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL && bc_player != except) {
#define END_BROADCAST }}

#define CHAT_ESCAPE "\u00A7"
#define CHAT_BLACK "\u00A70"
#define CHAT_DARKBLUE "\u00A71"
#define CHAT_DARKGREEN "\u00A72"
#define CHAT_DARKAQUA "\u00A73"
#define CHAT_DARKRED "\u00A74"
#define CHAT_DARKPURPLE  "\u00A75"
#define CHAT_GOLD "\u00A76"
#define CHAT_GRAY "\u00A77"
#define CHAT_DARKGRAY "\u00A78"
#define CHAT_INDIGO "\u00A79"
#define CHAT_GREEN "\u00A7A"
#define CHAT_AQUA "\u00A7B"
#define CHAT_RED "\u00A7C"
#define CHAT_PINK "\u00A7D"
#define CHAT_YELLOW "\u00A7E"
#define CHAT_WHITE "\u00A7F"
#define CHAT_STRIKETHROUGH "\u00A7M"
#define CHAT_UNDERLINED "\u00A7N"
#define CHAT_BOLD "\u00A7L"
#define CHAT_RANDOM "\u00A7K"
#define CHAT_ITALIC "\u00A7O"

#endif /* GAME_H_ */
