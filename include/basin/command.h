/*
 * command.h
 *
 *  Created on: Dec 29, 2016
 *      Author: root
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include <basin/player.h>
#include <stdlib.h>

void init_base_commands();

void registerCommand(char* command, void (*callback)(struct player* player, char** args, size_t args_count));

void callCommand(struct player* player, struct mempool* pool, char* command);

#endif /* COMMAND_H_ */
