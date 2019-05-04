#ifndef BASIN_SERVER_H_
#define BASIN_SERVER_H_

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
    uint32_t next_entity_id;
    size_t tick_counter;
    //  struct queue* playersToLoad;

};

//TODO: make this an option
#define RANDOM_TICK_SPEED 3

#endif /* BASIN_SERVER_H_ */
