/*
 * server.h
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <avuna/pmem.h>
#include <avuna/log.h>
#include <basin/world.h>

struct server {
    int fd;
    struct mempool* pool;
    char* name;
    char* motd;
    size_t max_players;
    int online_mode;
    int difficulty;
    struct logsess* logger;
    struct world* nether;
    struct world* overworld;
    struct world* endworld;
    struct list* worlds;
    struct queue* prepared_connections;
    struct hashmap* players_by_entity_id;
    //  struct queue* playersToLoad;

};

//TODO: make this an option
#define RANDOM_TICK_SPEED 3

#endif /* SERVER_H_ */
