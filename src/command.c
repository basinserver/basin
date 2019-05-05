#include <basin/game.h>
#include <basin/player.h>
#include <basin/globals.h>
#include <basin/command.h>
#include <basin/profile.h>
#include <basin/server.h>
#include <basin/item.h>
#include <avuna/string.h>
#include <stdlib.h>

void command_gamemode(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count == 0 || args_count > 2) {
        sendMessageToPlayer(player, "Usage: /gamemode <gamemode> [player]", "red");
        return;
    }
    struct player* target = player;
    if (args_count == 2) target = player_get_by_name(player->server, args[1]);
    if (target == NULL) {
        sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
        return;
    }
    if (str_eq(args[0], "0") || str_eq(args[0], "survival")) {
        player_set_gamemode(target, 0);
    } else if (str_eq(args[0], "1") || str_eq(args[0], "creative")) {
        player_set_gamemode(target, 1);
    } else {
        sendMessageToPlayer(target, "[ERROR] No such gamemode found.", "red");
        return;
    }
    sendMessageToPlayer(NULL, "Done.", "default");
}

void command_tp(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count == 0 || args_count > 4) {
        sendMessageToPlayer(player, "Usage: /tp <to> OR /tp <from> <to> or <x> <y> <z> or <player> <x> <y> <z>", "red");
        return;
    }
    if (args_count >= 3) {
        struct player* from = args_count == 3 ? player : player_get_by_name(server, args[0]);
        if (from == NULL) {
            sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
            return;
        }
        int32_t ai = args_count == 4 ? 1 : 0;
        player_teleport(from, strtol(args[ai++], NULL, 10), strtol(args[ai++], NULL, 10), strtol(args[ai++], NULL, 10));
    } else {
        struct player* from = args_count == 1 ? player : player_get_by_name(server, args[0]);
        struct player* to = args_count == 1 ? player_get_by_name(server, args[0]) : player_get_by_name(server, args[1]);
        if (from == NULL || to == NULL) {
            sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
            return;
        }
        if (from->world != to->world) {
            sendMessageToPlayer(player, "[ERROR] Players in different worlds!", "red");
            return;
        }
        player_teleport(from, to->entity->x, to->entity->y, to->entity->z);
    }
}

void command_kick(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count == 0 || args_count > 2) {
        sendMessageToPlayer(player, "Usage: /kick <player> OR /kick <player> \"<reason>\"", "red");
        return;
    }
    struct player* target = player_get_by_name(server, args[0]);
    if (target == NULL) {
        sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
        return;
    }
    char* reason = args_count == 1 ? "You have been kicked" : args[1];
    player_kick(target, reason);
}

void command_say(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count != 1) {
        sendMessageToPlayer(player, "Usage: /say \"<message>\"", "red");
        return;
    }
    broadcastf("light_purple", "CONSOLE: %s", args[0]);
}

void command_motd(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count != 1) {
        sendMessageToPlayer(NULL, "Usage: /motd \"<message>\"", "red");
        return;
    }
    sendMessageToPlayer(NULL, "Done.", "default");
    server->motd = prealloc(global_pool, server->motd, strlen(args[0]) + 1);
    server->motd[strlen(args[0])] = 0;
    memcpy(server->motd, args[0], strlen(args[0]) + 1);
}

void command_list(struct server* server, struct player* player, char** args, size_t args_count) {
    char* plist = pmalloc();
    char* cptr = plist;
    BEGIN_HASHMAP_ITERATION (players);
    if (cptr > plist) {
        *cptr++ = ',';
        *cptr++ = ' ';
    }
    struct player* player = (struct player*) value;
    cptr = xstrncat(cptr, 16, player->name);
    END_HASHMAP_ITERATION (players);
    snprintf(cptr, 32, " (%lu players_by_entity_id total)", players->entry_count);
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
    player_teleport(player, (double) player->world->spawnpos.x + .5, (double) player->world->spawnpos.y,
                    (double) player->world->spawnpos.z + .5);
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
        player = player_get_by_name(args[0]);
        if (player == NULL) {
            sendMessageToPlayer(player, "[ERROR] No such player found.", "red");
            return;
        }
    }
    damageEntity(player->entity, player->entity->maxHealth * 10., 0);
}

void command_tps(struct player* player, char** args, size_t args_count) {
    broadcastf("light_purple", "%f tps", player->world->tps);
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
    registerCommand("tps", &command_tps);
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

void callCommand(struct player* player, struct mempool* pool, char* command) {
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
        if (com == NULL) {
            
        }
        if (streq_nocase(com->command, rc)) {
            (*com->callback)(player, args, arg_count);
            return;
        }
    }
    sendMessageToPlayer(player, "[ERROR] Invalid Command!", "red");
}
