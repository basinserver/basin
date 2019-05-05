/*
 * game.h
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#ifndef BASIN_GAME_H_
#define BASIN_GAME_H_

#include <basin/world.h>
#include <basin/entity.h>
#include <avuna/hash.h>
#include <pthread.h>

void player_openInventory(struct player* player, struct inventory* inv);

float game_rand_float();

void playSound(struct world* world, int32_t soundID, int32_t soundCategory, float x, float y, float z, float volume, float pitch);

void dropPlayerItem(struct player* player, struct slot* drop);

void dropEntityItem_explode(struct entity* entity, struct slot* drop);

void game_drop_block(struct world* world, struct slot* slot, int32_t x, int32_t y, int32_t z);

void dropBlockDrops(struct world* world, block blk, struct player* breaker, int32_t x, int32_t y, int32_t z);

void game_load_player(struct player* to, struct player* from);

void game_load_entity(struct player* to, struct entity* from);

void game_update_inventory(struct player* player, struct inventory* inventory, int slot);

void world_tick(struct world* world);

void sendMessageToPlayer(struct player* player, char* text, char* color);

void sendMsgToPlayerf(struct player* player, char* color, char* fmt, ...);

void broadcast(char* text, char* color);

void broadcastf(char* color, char* fmt, ...);

#define BEGIN_BROADCAST(players) pthread_rwlock_rdlock(&(players)->rwlock); ITER_MAP(players) { struct player* bc_player = (struct player*)value; {
#define BEGIN_BROADCAST_DIST(distfrom, dist) pthread_rwlock_rdlock(&(distfrom->world->players)->rwlock); ITER_MAP(distfrom->world->players) { struct player* bc_player = (struct player*)value; if(entity_distsq(bc_player->entity, distfrom) < dist * dist) {
#define BEGIN_BROADCAST_DISTXYZ(x, y, z, players, dist) pthread_rwlock_rdlock(&(players)->rwlock); ITER_MAP(players) { struct player* bc_player = (struct player*)value; if (entity_distsq_block(bc_player->entity, x, y, z) < dist * dist) {
#define BEGIN_BROADCAST_EXCEPT(players, except) pthread_rwlock_rdlock(&(players)->rwlock); ITER_MAP(players) { struct player* bc_player = (struct player*)value; if (bc_player != except) {
#define BEGIN_BROADCAST_EXCEPT_DIST(except, distfrom, dist) pthread_rwlock_rdlock(&(distfrom->world->players)->rwlock); ITER_MAP(distfrom->world->players) { struct player* bc_player = (struct player*)value; if (bc_player != except && entity_distsq(bc_player->entity, distfrom) < dist * dist) {
#define END_BROADCAST(players) } ITER_MAP_END(); pthread_rwlock_unlock(&(players)->rwlock); }

/*
 #define BEGIN_BROADCAST_DIST(distfrom, dist) for (size_t i = 0; i < distfrom->world->players->size; i++) {struct player* bc_player = (struct player*) distfrom->world->players->data[i];if (bc_player != NULL && entity_distsq(bc_player->entity, distfrom) < dist * dist) {
 #define BEGIN_BROADCAST_DISTXYZ(x, y, z, players, dist) for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL && entity_distsq_block(bc_player->entity, x, y, z) < dist * dist) {
 #define BEGIN_BROADCAST_EXCEPT_DIST(except, distfrom, dist) for (size_t i = 0; i < distfrom->world->players->size; i++) {struct player* bc_player = (struct player*) distfrom->world->players->data[i];if (bc_player != NULL && bc_player != except && entity_distsq(bc_player->entity, distfrom) < dist * dist) {
 #define BEGIN_BROADCAST(players) if(players->mc) pthread_rwlock_rdlock(&players->data_mutex); for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL) {
 #define BEGIN_BROADCAST_EXCEPT(players, except) if(players->mc) pthread_rwlock_rdlock(&players->data_mutex); for (size_t i = 0; i < players->size; i++) {struct player* bc_player = (struct player*) players->data[i];if (bc_player != NULL && bc_player != except) {
 #define END_BROADCAST_MT(players) }} if(players->mc) pthread_rwlock_unlock(&players->data_mutex);
 #define END_BROADCAST }}
 */

#endif /* BASIN_GAME_H_ */
