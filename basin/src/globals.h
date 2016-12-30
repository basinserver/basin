/*
 * globals.h
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

#define MC_PROTOCOL_VERSION_MIN 315
#define MC_PROTOCOL_VERSION_MAX 316
#define CHUNK_VIEW_DISTANCE 10

#include <stdlib.h>

size_t tick_counter;
struct config* cfg;
struct logsess* delog;
struct collection* players;

#endif /* GLOBALS_H_ */
