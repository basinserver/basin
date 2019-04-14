/*
 * worldmanager.h
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#ifndef WORLDMANAGER_H_
#define WORLDMANAGER_H_

#include <basin/world.h>

#define NETHER -1
#define OVERWORLD 0
#define ENDWORLD 1

struct world* nether;
struct world* overworld;
struct world* endworld;

struct collection* worlds;

struct world* getWorldByID(int32_t id);

#endif /* WORLDMANAGER_H_ */
