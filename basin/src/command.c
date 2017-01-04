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
#include "globals.h"
#include "xstring.h"
#include "util.h"
#include "command.h"
#include "profile.h"
#include "server.h"
#include "item.h"

void command_gamemode(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count == 0 || args_count > 2) {
		sendMessageToPlayer(player, "Usage: /gamemode <gamemode> [player]", "red");
		return;
	}
	struct player* target = player;
	if (args_count == 2) target = getPlayerByName(args[1]);
	if (target == NULL) {
		sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
		return;
	}
	if (streq_nocase(args[0], "0") || streq_nocase(args[0], "survival")) {
		setPlayerGamemode(target, 0);
	} else if (streq_nocase(args[0], "1") || streq_nocase(args[0], "creative")) {
		setPlayerGamemode(target, 1);
	} else {
		sendMessageToPlayer(target, "[ERROR] No such gamemode found.", "red");
		return;
	}
	sendMessageToPlayer(NULL, "Done.", "default");
}

void command_tp(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count == 0 || args_count > 2) {
		sendMessageToPlayer(player, "Usage: /tp <to> OR /tp <from> <to>", "red");
		return;
	}
	struct player* from = args_count == 1 ? player : getPlayerByName(args[0]);
	struct player* to = args_count == 1 ? getPlayerByName(args[0]) : getPlayerByName(args[1]);
	if (from == NULL || to == NULL) {
		sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
		return;
	}
	if (from->world != to->world) {
		sendMessageToPlayer(player, "[ERROR] Players in different worlds!", "red");
		return;
	}
	teleportPlayer(from, to->entity->x, to->entity->y, to->entity->z);
}

void command_kick(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count == 0 || args_count > 2) {
		sendMessageToPlayer(player, "Usage: /kick <player> OR /kick <player> \"<reason>\"", "red");
		return;
	}
	struct player* target = getPlayerByName(args[0]);
	if (target == NULL) {
		sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
		return;
	}
	char* reason = args_count == 1 ? "You have been kicked" : args[1];
	kickPlayer(target, reason);
}

void command_say(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count != 1) {
		sendMessageToPlayer(player, "Usage: /say \"<message>\"", "red");
		return;
	}
	broadcastf("light_purple", "CONSOLE: %s", args[0]);
}

void command_motd(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	if (args_count != 1) {
		sendMessageToPlayer(NULL, "Usage: /motd \"<message>\"", "red");
		return;
	}
	sendMessageToPlayer(NULL, "Done.", "default");
	xfree (motd);
	motd = xstrdup(args[0], 0);
}

void command_list(struct player* player, char** args, size_t args_count) {
	char* plist = xmalloc(players->entry_count * 18 + 30);
	char* cptr = plist;
	BEGIN_HASHMAP_ITERATION (players)
	if (cptr > plist) {
		*cptr++ = ',';
		*cptr++ = ' ';
	}
	struct player* player = (struct player*) value;
	cptr = xstrncat(cptr, 16, player->name);
	END_HASHMAP_ITERATION (players)
	snprintf(cptr, 32, " (%lu players total)", players->entry_count);
	sendMessageToPlayer(player, plist, "gray");
	xfree(plist);
}

void command_spawn(struct player* player, char** args, size_t args_count) {
	if (!player) {
		sendMsgToPlayerf(player, "red", "Must be run as player");
		return;
	}

	if (player->entity->health < player->entity->maxHealth) {
		sendMsgToPlayerf(player, "red", "You must have full health to teleport to spawn!");
		return;
	}
	if (args_count > 0) {
		sendMessageToPlayer(player, "red", "Usage: /spawn");
		return;
	}
	teleportPlayer(player, (double) player->world->spawnpos.x + .5, (double) player->world->spawnpos.y, (double) player->world->spawnpos.z + .5);
}

void command_printprofile(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	printProfiler();
}

void command_clearprofile(struct player* player, char** args, size_t args_count) {
	if (player != NULL) return;
	clearProfiler();
}

void command_kill(struct player* player, char** args, size_t args_count) {
	if (!player) {
		if (args_count != 1) {
			sendMsgToPlayerf(player, "red", "Usage: /kill <player>");
			return;
		}
		player = getPlayerByName(args[0]);
		if (player == NULL) {
			sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
			return;
		}
	}
	damageEntity(player->entity, player->entity->maxHealth * 10., 0);
}

void init_base_commands() {
	registerCommand("gamemode", &command_gamemode);
	registerCommand("gm", &command_gamemode);
	registerCommand("tp", &command_tp);
	registerCommand("spawn", &command_spawn);
	registerCommand("kick", &command_kick);
	registerCommand("say", &command_say);
	registerCommand("printprofile", &command_printprofile);
	registerCommand("pp", &command_printprofile);
	registerCommand("clearprofile", &command_clearprofile);
	registerCommand("cp", &command_clearprofile);
	registerCommand("motd", &command_motd);
	registerCommand("list", &command_list);
	registerCommand("ls", &command_list);
	registerCommand("who", &command_list);
	registerCommand("kill", &command_kill);
}

typedef void (*command_callback)(struct player* player, char** args, size_t args_count);

struct command {
		char* command;
		command_callback callback;
};

struct collection* registered_commands;

void registerCommand(char* command, command_callback callback) {
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
	int iq = 0;
	int eq = 0;
	for (size_t i = 0; i < sl; i++) {
		int cl = 0;
		if (command[i] == ' ' && !iq) {
			command[i] = 0;
			arg_count++;
		} else if (command[i] == '\"' && !eq) {
			iq = !iq;
			eq = 0;
			cl = 1;
		} else if (command[i] == '\\') {
			eq = !eq;
			cl = eq;
		} else eq = 0;
		if (cl) {
			memmove(command + i, command + i + 1, sl - i);
			i--;
			sl--;
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
	sendMessageToPlayer(player, "[ERROR] Invalid Command!", "red");
}
