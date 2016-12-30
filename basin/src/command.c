/*
 * command.c
 *
 *  Created on: Dec 29, 2016
 *      Author: root
 */

#include <stdlib.h>
#include "collection.h"
#include "game.h"
#include "player.h"
#include "xstring.h"
#include "util.h"
#include "command.h"

void command_gamemode(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count == 0 || args_count > 2) {
		sendMessageToPlayer(player, "Usage: /gamemode <gamemode> [player]");
		return;
	}
	struct player* target = player;
	if (args_count == 2) target = getPlayerByName(args[1]);
	if (target == NULL) {
		sendMessageToPlayer(player, "[ERROR] No such player found.");
		return;
	}
	if (streq_nocase(args[0], "0") || streq_nocase(args[0], "survival")) {
		setPlayerGamemode(target, 0);
	} else if (streq_nocase(args[0], "1") || streq_nocase(args[0], "creative")) {
		setPlayerGamemode(target, 1);
	} else {
		sendMessageToPlayer(target, "[ERROR] No such gamemode found.");
		return;
	}
}

void command_tp(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count == 0 || args_count > 2) {
		sendMessageToPlayer(player, "Usage: /tp <to> OR /tp <from> <to>");
		return;
	}
	struct player* from = args_count == 1 ? player : getPlayerByName(args[0]);
	struct player* to = args_count == 1 ? getPlayerByName(args[0]) : getPlayerByName(args[1]);
	if (from == NULL || to == NULL) {
		sendMessageToPlayer(player, "[ERROR] No such player found.");
		return;
	}
	if (from->world != to->world) {
		sendMessageToPlayer(player, "[ERROR] Players in different worlds!");
		return;
	}
	teleportPlayer(from, to->entity->x, to->entity->y, to->entity->z);
}

void command_kick(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count == 0 || args_count > 2) {
		sendMessageToPlayer(player, "Usage: /kick <player> OR /kick <player> <reason>");
		return;
	}
	struct player* from = getPlayerByName(args[0]);
	char* reason = args_count == 1 ? "You Have Been Kicked" : args[1];
	size_t rl = strlen(reason);
	char rreason[rl + 512];
	snprintf(rreason, rl + 512, "{\"text\": \"%s\"}", reason);
	kickPlayer(from, rreason);
}

void command_spawn(struct player* player, char** args, size_t args_count) {
	if (player->entity->health < player->entity->maxHealth) {
		sendMsgToPlayerf(player, "You must have full health to teleport to spawn!");
		return;
	}
	if (args_count > 0) {
		sendMessageToPlayer(player, "Usage: /spawn");
		return;
	}
	teleportPlayer(player, (double) player->world->spawnpos.x + .5, (double) player->world->spawnpos.y, (double) player->world->spawnpos.z + .5);
}

void init_base_commands() {
	registerCommand("gamemode", &command_gamemode);
	registerCommand("gm", &command_gamemode);
	registerCommand("tp", &command_tp);
	registerCommand("spawn", &command_spawn);
	registerCommand("kick", &command_kick);
}

struct command {
		char* command;
		void (*callback)(struct player* player, char** args, size_t args_count);
};

struct collection* registered_commands;

void registerCommand(char* command, void (*callback)(struct player* player, char** args, size_t args_count)) {
	if (registered_commands == NULL) registered_commands = new_collection(16, 0);
	struct command* com = xmalloc(sizeof(struct command));
	com->command = xstrdup(command, 0);
	com->callback = callback;
	add_collection(registered_commands, com);
}

void callCommand(struct player* player, char* command) {
	if (registered_commands == NULL) return;
	size_t sl = strlen(command);
	size_t arg_count = 0;
	for (size_t i = 0; i < sl; i++) {
		if (command[i] == ' ') {
			command[i] = 0;
			arg_count++;
		}
	}
	char* args[arg_count];
	char* rc = command;
	size_t i = strlen(command) + 1;
	command += i;
	size_t i2 = 0;
	while (i < sl) {
		args[i2] = command;
		i += strlen(command) + 1;
		command += strlen(command) + 1;
		args[i2] = trim(args[i2]);
		i2++;
	}
	for (size_t i3 = 0; i3 < registered_commands->size; i3++) {
		struct command* com = (struct command*) registered_commands->data[i3];
		if (com == NULL) continue;
		if (streq_nocase(com->command, rc)) {
			(*com->callback)(player, args, arg_count);
			return;
		}
	}
	sendMessageToPlayer(player, "[ERROR] Invalid Command!");
}
