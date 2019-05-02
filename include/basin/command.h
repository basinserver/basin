/*
 * command.h
 *
 *  Created on: Dec 29, 2016
 *      Author: root
 */

#ifndef BASIN_COMMAND_H_
#define BASIN_COMMAND_H_

#include <basin/player.h>
#include <stdlib.h>

void init_base_commands();

void registerCommand(char* command, void (*callback)(struct player* player, char** args, size_t args_count));

void callCommand(struct player* player, struct mempool* pool, char* command);

#endif /* BASIN_COMMAND_H_ */
