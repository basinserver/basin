/*
 * globals.h
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#define MC_PROTOCOL_VERSION_MIN 210
#define MC_PROTOCOL_VERSION_MAX 316
#define CHUNK_VIEW_DISTANCE 10

#include <stdlib.h>
#include <pthread.h>

size_t tick_counter;
struct config* cfg;
struct logsess* delog;
struct hashmap* players;
struct collection* defunctPlayers;
struct collection* defunctChunks;
struct collection* playersToLoad;
pthread_mutex_t glob_tick_mut;
pthread_cond_t glob_tick_cond;

#endif /* GLOBALS_H_ */
