#ifndef BASIN_COMMAND_H_
#define BASIN_COMMAND_H_

#include <basin/player.h>
#include <stdlib.h>

void init_base_commands();

typedef void (*command_callback)(struct server* server, struct player* player, char** args, size_t args_count);

void command_register(char* called_by, command_callback callback);

void command_call(struct server* server, struct player* player, char* command);

#endif /* BASIN_COMMAND_H_ */
