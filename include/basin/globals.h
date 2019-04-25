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

#include <avuna/log.h>
#include <avuna/hash.h>
#include <avuna/pmem.h>
#include <avuna/queue.h>
#include <stdlib.h>
#include <pthread.h>

struct config* cfg;
struct mempool* global_pool;
struct logsess* delog;
struct hashmap* server_map;

#endif /* GLOBALS_H_ */
