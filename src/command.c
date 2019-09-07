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
        player_send_message(player, "red", "Usage: /gamemode <gamemode> [player]");
        return;
    }
    struct player* target = player;
    if (args_count == 2) target = player_get_by_name(player->server, args[1]);
    if (target == NULL) {
        player_send_message(player, "red", "[ERROR] No such player found.");
        return;
    }
    if (str_eq(args[0], "0") || str_eq(args[0], "survival")) {
        player_set_gamemode(target, 0);
    } else if (str_eq(args[0], "1") || str_eq(args[0], "creative")) {
        player_set_gamemode(target, 1);
    } else {
        player_send_message(target, "red", "[ERROR] No such gamemode found.");
        return;
    }
    player_send_message(NULL, "default", "Done.");
}

void command_tp(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count == 0 || args_count > 4) {
        player_send_message(player, "red", "Usage: /tp <to> OR /tp <from> <to> or <x> <y> <z> or <player> <x> <y> <z>");
        return;
    }
    if (args_count >= 3) {
        struct player* from = args_count == 3 ? player : player_get_by_name(server, args[0]);
        if (from == NULL) {
            player_send_message(player, "red", "[ERROR] No such player found.");
            return;
        }
        int32_t ai = args_count == 4 ? 1 : 0;
        player_teleport(from, strtol(args[ai++], NULL, 10), strtol(args[ai++], NULL, 10), strtol(args[ai++], NULL, 10));
    } else {
        struct player* from = args_count == 1 ? player : player_get_by_name(server, args[0]);
        struct player* to = args_count == 1 ? player_get_by_name(server, args[0]) : player_get_by_name(server, args[1]);
        if (from == NULL || to == NULL) {
            player_send_message(player, "red", "[ERROR] No such player found.");
            return;
        }
        if (from->world != to->world) {
            player_send_message(player, "red", "[ERROR] Players in different worlds!");
            return;
        }
        player_teleport(from, to->entity->x, to->entity->y, to->entity->z);
    }
}

void command_kick(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count == 0 || args_count > 2) {
        player_send_message(player, "red", "Usage: /kick <player> OR /kick <player> \"<reason>\"");
        return;
    }
    struct player* target = player_get_by_name(server, args[0]);
    if (target == NULL) {
        player_send_message(player, "red", "[ERROR] No such player found.");
        return;
    }
    char* reason = args_count == 1 ? "You have been kicked" : args[1];
    player_kick(target, reason);
}

void command_say(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count != 1) {
        player_send_message(player, "red", "Usage: /say \"<message>\"");
        return;
    }
    player_broadcast("light_purple", "CONSOLE: %s", args[0]);
}

void command_motd(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    if (args_count != 1) {
        player_send_message(NULL, "red", "Usage: /motd \"<message>\"");
        return;
    }
    player_send_message(NULL, "default", "Done.");
    server->motd = prealloc(global_pool, server->motd, strlen(args[0]) + 1);
    server->motd[strlen(args[0])] = 0;
    memcpy(server->motd, args[0], strlen(args[0]) + 1);
}

void command_list(struct server* server, struct player* player, char** args, size_t args_count) {
    char message[4096];
    message[0] = 0;
    pthread_rwlock_rdlock(&server->players_by_entity_id->rwlock);
    ITER_MAP(server->players_by_entity_id) {
        if (message[0] != 0) {
            strncat(message, ", ", 4096);
        }
        struct player* iter_player = (struct player*) value;
        strncat(message, iter_player->name, 4096);
        ITER_MAP_END();
    }
    snprintf(message + strlen(message), 4096 - strlen(message), " (%lu players total)", server->players_by_entity_id->entry_count);
    pthread_rwlock_unlock(&server->players_by_entity_id->rwlock);
    player_send_message(player, "gray", "%s", message);
}

void command_spawn(struct server* server, struct player* player, char** args, size_t args_count) {
    if (!player) {
        player_send_message(player, "red", "Must be run as player");
        return;
    }

    if (player->entity->health < player->entity->maxHealth) {
        player_send_message(player, "red", "You must have full health to teleport to spawn!");
        return;
    }
    if (args_count > 0) {
        player_send_message(player, "Usage: /spawn", "red");
        return;
    }
    player_teleport(player, (double) player->world->spawnpos.x + .5, (double) player->world->spawnpos.y,
                    (double) player->world->spawnpos.z + .5);
}

void command_printprofile(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    printProfiler();
}

void command_clearprofile(struct server* server, struct player* player, char** args, size_t args_count) {
    if (player != NULL) return;
    clearProfiler();
}

void command_kill(struct server* server, struct player* player, char** args, size_t args_count) {
    if (!player) {
        if (args_count != 1) {
            player_send_message(player, "red", "Usage: /kill <player>");
            return;
        }
        player = player_get_by_name(server, args[0]);
        if (player == NULL) {
            player_send_message(player, "red", "[ERROR] No such player found.");
            return;
        }
    }
    damageEntity(player->entity, player->entity->maxHealth * 10., 0);
}

void command_tps(struct server* server, struct player* player, char** args, size_t args_count) {
    player_broadcast(server->players_by_entity_id, "light_purple", "%f tps", player == NULL ? server->overworld->tps : player->world->tps);
}

void init_base_commands() {
    command_register("gamemode", &command_gamemode);
    command_register("gm", &command_gamemode);
    command_register("tp", &command_tp);
    command_register("spawn", &command_spawn);
    command_register("kick", &command_kick);
    command_register("say", &command_say);
    command_register("printprofile", &command_printprofile);
    command_register("pp", &command_printprofile);
    command_register("clearprofile", &command_clearprofile);
    command_register("cp", &command_clearprofile);
    command_register("motd", &command_motd);
    command_register("list", &command_list);
    command_register("ls", &command_list);
    command_register("who", &command_list);
    command_register("kill", &command_kill);
    command_register("tps", &command_tps);
}

struct command {
        char* command;
        command_callback callback;
};

struct hashmap* registered_commands;

void command_register(char* called_by, command_callback callback) {
    if (registered_commands == NULL) {
        registered_commands = hashmap_thread_new(16, global_pool);
    }
    struct command* command = pmalloc(registered_commands->pool, sizeof(struct command));
    command->command = str_dup(called_by, 0, registered_commands->pool);
    command->callback = callback;
    hashmap_put(registered_commands, command->command, command);
}

void command_call(struct server* server, struct player* player, char* command) {
    if (registered_commands == NULL) {
        return;
    }
    size_t command_length = strlen(command);
    size_t arg_count = 0;
    int in_quote = 0;
    int escaped = 0;
    for (size_t i = 0; i < command_length; i++) {
        int add_character = 0;
        if (command[i] == ' ' && !in_quote) {
            command[i] = 0;
            arg_count++;
        } else if (command[i] == '\"' && !escaped) {
            in_quote = !in_quote;
            escaped = 0;
            add_character = 1;
        } else if (command[i] == '\\') {
            escaped = !escaped;
            add_character = escaped;
        } else escaped = 0;
        if (add_character) {
            memmove(command + i, command + i + 1, command_length - i);
            i--;
            command_length--;
        }
    }
    char* args[arg_count];
    char* real_command = command;
    size_t i = strlen(command) + 1;
    command += i;
    size_t i2 = 0;
    while (i < command_length) {
        args[i2] = command;
        i += strlen(command) + 1;
        command += strlen(command) + 1;
        args[i2] = str_trim(args[i2]);
        i2++;
    }
    if (arg_count == 0) {
        return;
    }
    struct command* com = (struct command*) hashmap_get(registered_commands, args[0]);
    if (com == NULL) {
        player_send_message(player, "red", "[ERROR] Command \"/%s\" not found!", args[0]);
        return;
    }
    (*com->callback)(server, player, args, arg_count);
}
