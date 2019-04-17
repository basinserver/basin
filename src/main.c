/*
 * main.c
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#include "accept.h"
#include "work.h"
#include <basin/world.h>
#include <basin/worldmanager.h>
#include <basin/server.h>
#include <basin/game.h>
#include <basin/block.h>
#include <basin/crafting.h>
#include <basin/tools.h>
#include <basin/smelting.h>
#include <basin/command.h>
#include <basin/profile.h>
#include <basin/plugin.h>
#include <basin/version.h>
#include <basin/globals.h>
#include <avuna/config.h>
#include <avuna/string.h>
#include <avuna/streams.h>
#include <avuna/queue.h>
#include <avuna/pmem_hooks.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <openssl/rand.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void main_tick() {
	pthread_cond_broadcast (&glob_tick_cond);
	pthread_rwlock_wrlock(&playersToLoad->data_mutex);
	for (size_t i = 0; i < playersToLoad->size; i++) {
		struct player* player = (struct player*) playersToLoad->data[i];
		if (player == NULL) continue;
		world_spawn_player(overworld, player);
		playersToLoad->data[i] = NULL;
		playersToLoad->count--;
		if (i == playersToLoad->size - 1) playersToLoad->size--;
	}
	pthread_rwlock_unlock(&playersToLoad->data_mutex);
	pthread_cond_broadcast (&chunk_wake);
	BEGIN_HASHMAP_ITERATION (players)
	flush_outgoing (value);
	END_HASHMAP_ITERATION (players)
	if (tick_counter % 20 == 0) {
		pthread_rwlock_wrlock(&defunctPlayers->data_mutex);
		for (size_t i = 0; i < defunctPlayers->size; i++) {
			struct player* def = defunctPlayers->data[i];
			if (def == NULL) continue;
			if (def->defunct++ >= 21) {
				freeEntity(def->entity);
				player_free(def);
				defunctPlayers->data[i] = NULL;
				defunctPlayers->count--;
				if (i == defunctPlayers->size - 1) defunctPlayers->size--;
			}
		}
		pthread_rwlock_unlock(&defunctPlayers->data_mutex);
		pthread_rwlock_wrlock(&defunctChunks->data_mutex);
		for (size_t i = 0; i < defunctChunks->size; i++) {
			struct chunk* def = defunctChunks->data[i];
			if (def == NULL) continue;
			if (def->defunct++ >= 21) {
				//printf("frch\n");
				chunk_free(def);
				defunctChunks->data[i] = NULL;
				defunctChunks->count--;
				if (i == defunctChunks->size - 1) defunctChunks->size--;
			}
		}
		pthread_rwlock_unlock(&defunctChunks->data_mutex);
	}
	tick_counter++;
}

void main_tick_thread(void* ptr) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	double lastRun = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &ts);
		double ct = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
		while (lastRun <= ct - 50.) {
			lastRun += 50.;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			double tks = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
			main_tick();
			clock_gettime(CLOCK_MONOTONIC, &ts);
			double tkf = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
			tkf -= tks;
			if (tkf > 40.) printf("[WARNING] Tick took %f ms!\n", tkf);
		}
		usleep((lastRun - (ct - 50.)) * 1000.);
	}
#pragma clang diagnostic pop
}

struct server* server_load(struct config_node* server) {
	if (server->name == NULL) {
		errlog(delog, "Server block missing name, skipping.");
	}
	const char* bind_ip = NULL;
	uint16_t port = 25565;
	int namespace = -1;
	int bind_all = 0;
	int ip6 = 0;
	bind_ip = config_get(server, "bind-ip");
	if (str_eq(bind_ip, "0.0.0.0")) {
		bind_all = 1;
	}
	ip6 = bind_all || str_contains(bind_ip, ":");
	const char* bind_port = config_get(server, "bind-port");
	if (bind_port == NULL) {
		bind_port = "25565";
	}
	if (!str_isunum(bind_port)) {
		errlog(delog, "Invalid bind-port for server: %s", server->name);
		return NULL;
	}
	port = (uint16_t) strtoul(bind_port, NULL, 10);
	namespace = ip6 ? PF_INET6 : PF_INET;;
	const char* net_thread_count_str = config_get(server, "network-threads");
	if (!str_isunum(net_thread_count_str)) {
		errlog(delog, "Invalid threads for server: %s", server->name);
		return NULL;
	}
	size_t net_thread_count = strtoul(net_thread_count_str, NULL, 10);
	if (net_thread_count < 1) {
		errlog(delog, "Invalid threads for server: %s, must be greater than 1.", server->name);
		return NULL;
	}
	const char* max_player_str = config_get(server, "max-players_by_entity_id");
	if (max_player_str == NULL) {
		max_player_str = "20";
	}
	if (!str_isunum(max_player_str)) {
		errlog(delog, "Invalid max-players_by_entity_id for server: %s", server->name);
		return NULL;
	}
	size_t max_players = strtoul(max_player_str, NULL, 10);
	const char* difficulty_str = config_get(server, "difficulty");
	if (difficulty_str == NULL) {
		difficulty_str = "2";
	}
	if (!str_isunum(difficulty_str)) {
		errlog(delog, "Invalid difficulty for server: %s", server->name);
		return NULL;
	}
	uint8_t difficulty = (uint8_t) strtoul(difficulty_str, NULL, 10);
	const char* motd = config_get(server, "motd");
	if (motd == NULL) {
		motd = "A Minecraft Server";
	}
	const char* online_mode_str = config_get(server, "online-mode");
	if (online_mode_str == NULL) {
		online_mode_str = "true";
	}
	int is_online_mode = str_eq(online_mode_str, "true");
	sock: ;
	int server_fd = socket(namespace, SOCK_STREAM, 0);
	if (server_fd < 0) {
		errlog(delog, "Error creating socket for server: %s, %s", server->name, strerror(errno));
		return NULL;
	}
	int one = 1;
	int zero = 0;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (void*) &one, sizeof(one)) == -1) {
		errlog(delog, "Error setting SO_REUSEADDR for server: %s, %s", server->name, strerror(errno));
		close(server_fd);
		return NULL;
	}
	if (ip6) {
		if (setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY, (void*) &zero, sizeof(zero)) == -1) {
			errlog(delog, "Error unsetting IPV6_V6ONLY for server: %s, %s", server->name, strerror(errno));
			close(server_fd);
			return NULL;
		}
		struct sockaddr_in6 bind_addr;
		bind_addr.sin6_flowinfo = 0;
		bind_addr.sin6_scope_id = 0;
		bind_addr.sin6_family = AF_INET6;
		if (bind_all) bind_addr.sin6_addr = in6addr_any;
		else if (!inet_pton(AF_INET6, bind_ip, &(bind_addr.sin6_addr))) {
			close(server_fd);
			errlog(delog, "Error binding socket for server: %s, invalid bind-ip", server->name);
			return NULL;
		}
		bind_addr.sin6_port = htons(port);
		if (bind(server_fd, (struct sockaddr*) &bind_addr, sizeof(bind_addr))) {
			close (server_fd);
			if (bind_all) {
				namespace = PF_INET;
				ip6 = 0;
				goto sock;
			}
			errlog(delog, "Error binding socket for server: %s, %s", server->name, strerror(errno));
			return NULL;
		}
	} else {
		struct sockaddr_in bip;
		bip.sin_family = AF_INET;
		if (!inet_aton(bind_ip, &(bip.sin_addr))) {
			close(server_fd);
			errlog(delog, "Error binding socket for server: %s, invalid bind-ip", server->name);
			return NULL;
		}
		bip.sin_port = htons(port);
		if (bind(server_fd, (struct sockaddr*) &bip, sizeof(bip))) {
			errlog(delog, "Error binding socket for server: %s, %s", server->name, strerror(errno));
			close(server_fd);
			return NULL;
		}
	}
	if (listen(server_fd, 50)) {
		errlog(delog, "Error listening on socket for server: %s, %s", server->name, strerror(errno));
		close(server_fd);
		return NULL;
	}
	if (fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL) | O_NONBLOCK) < 0) {
		errlog(delog, "Error setting non-blocking for server: %s, %s", server->name, strerror(errno));
		close(server_fd);
		return NULL;
	}
	struct mempool* pool = mempool_new();
	struct server* serv = pcalloc(pool, sizeof(struct server));
	serv->pool = pool;
	serv->fd = server_fd;
	phook(serv->pool, close_hook, (void*) serv->fd);
	serv->name = server->name;
	serv->difficulty = difficulty;
	serv->max_players = max_players;
	serv->motd = (char*) motd;
	serv->online_mode = is_online_mode;
	serv->prepared_connections = queue_new(0, 1, serv->pool);
	struct logsess* server_log = serv->logger = pcalloc(serv->pool, sizeof(struct logsess));
	server_log->pi = 0;
	const char* access_log = config_get(server, "access-log");
	server_log->access_fd = access_log == NULL ? NULL : fopen(access_log, "a");
	const char* error_log = config_get(server, "error-log");
	server_log->error_fd = error_log == NULL ? NULL : fopen(error_log, "a");
	acclog(server_log, "Server %s listening for connections!", server->name);
	const char* ovr = config_get(server, "world");
	if (ovr == NULL) {
		errlog(delog, "No world defined for server: %s", server->name);
		pfree(serv->pool);
		return NULL;
	}
	serv->overworld = world_new(8);
	world_load(serv->overworld, (char*) ovr);
	printf("Overworld Loaded\n");
	//nether = world_new();
	//world_load(nether, neth);
	//endworld = world_new();
	//world_load(endworld, ed);
	serv->worlds = list_new(8, serv->pool);
	list_append(serv->worlds, serv->overworld);
	//add_collection(worlds, nether);
	//add_collection(worlds, endworld);
	serv-> players_by_entity_id = hashmap_thread_new(32, global_pool);
	// playersToLoad = queue_new(0, 1, global_pool);

	hashmap_put(server_map, serv->name, serv);
}

int main(int argc, char* argv[]) {
	signal(SIGPIPE, SIG_IGN);
	global_pool = mempool_new();
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	size_t us = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
	srand(us);
	RAND_seed(&us, sizeof(size_t));
	printf("Loading %s %s\n", DAEMON_NAME, VERSION);
#ifdef DEBUG
	printf("Running in Debug mode!\n");
#endif
	char ncwd[strlen(argv[0]) + 1];
	memcpy(ncwd, argv[0], strlen(argv[0]) + 1);
	char* ecwd = strrchr(ncwd, '/');
	ecwd++;
	ecwd[0] = 0;
	chdir(ncwd);
	cfg = config_load("main.cfg");
	if (cfg == NULL) {
		printf("Error loading Config<%s>: %s\n", "main.cfg", errno == EINVAL ? "File doesn't exist!" : strerror(errno));
		return 1;
	}
	struct config_node* daemon = config_get_unique_cat(cfg, "daemon");
	if (daemon == NULL) {
		printf("[daemon] block does not exist in <%s>!\n", "main.cfg");
		return 1;
	}
#ifndef DEBUG
	if (runn) {
		printf("Already running! PID = %i\n", pid);
		exit(0);
	} else {

		pid_t f = fork();
		if (f == 0) {
			printf("Now running as daemon!\n");
			exit(0);
		} else {
			printf("Daemonized! PID = %i\n", f);
			if (setsid() < 0) {
				printf("Failed to exit process tree: %s\n", strerror(errno));
				return 1;
			}
			if (freopen("/dev/null", "r", stdin) < 0) {
				printf("reopening of STDIN to /dev/null failed: %s\n", strerror(errno));
				return 1;
			}
			if (freopen("/dev/null", "w", stderr) < 0) {
				printf("reopening of STDERR to /dev/null failed: %s\n", strerror(errno));
				return 1;
			}
			if (freopen("/dev/null", "w", stdout) < 0) {
				printf("reopening of STDOUT to /dev/null failed: %s\n", strerror(errno));
				return 1;
			}
		}
	}
#else
	printf("Daemonized! PID = %i\n", getpid());
#endif
	delog = pmalloc(global_pool, sizeof(struct logsess));
	delog->pi = 0;
	delog->access_fd = NULL;
	const char* error_log = config_get(daemon, "error-log");
	delog->error_fd = error_log == NULL ? NULL : fopen(error_log, "a"); // fopen will return NULL on error, which works.
//TODO: chown group to de-escalated
	struct list* servers = hashmap_get(cfg->nodeListsByCat, "server");
	struct accept_param* accept_params[servers->count];
	if (servers->count != 1) {
		errlog(delog, "Only one server block is supported at this time.");
		return -1;
	}
	globalChunkQueue = queue_new(0, 1, global_pool);
	init_materials();
	acclog(delog, "Materials Initialized");
	init_blocks();
	acclog(delog, "Blocks Initialized");
	init_entities();
	acclog(delog, "Entities Initialized");
	init_crafting();
	acclog(delog, "Crafting Initialized");
    tools_init();
	acclog(delog, "Tools Initialized");
	init_items();
	acclog(delog, "Items Initialized");
	init_smelting();
	acclog(delog, "Smelting Initialized");
	init_base_commands();
	acclog(delog, "Commands Initialized");
	init_plugins();
	acclog(delog, "Plugins Initialized");
	init_encryption();
	acclog(delog, "Encryption Initialized");
	server_map = hashmap_thread_new(16, global_pool);
	ITER_LIST(servers) {
		struct config_node* server = item;
		server_load(server);
	}
	const char* uids = config_get(daemon, "uid");
	const char* gids = config_get(daemon, "gid");
	uid_t uid = (uid_t) (uids == NULL ? 0 : strtoul(uids, NULL, 10));
	uid_t gid = (uid_t) (gids == NULL ? 0 : strtoul(gids, NULL, 10));
	if (gid > 0) {
		if (setgid(gid) != 0) {
			errlog(delog, "Failed to setgid! %s", strerror(errno));
		}
	}
	if (uid > 0) {
		if (setuid(uid) != 0) {
			errlog(delog, "Failed to setuid! %s", strerror(errno));
		}
	}
	acclog(delog, "Running as UID = %u, GID = %u, starting workers.", getuid(), getgid());
	for (int i = 0; i < servsl; i++) {
		pthread_t pt;
		for (int x = 0; x < accept_params[i]->works_count; x++) {
			int c = pthread_create(&pt, NULL, (void *) run_work, accept_params[i]->works[x]);
			if (c != 0) {
				if (servs[i]->id != NULL) errlog(delog, "Error creating thread: pthread errno = %i, this will cause occasional connection hanging @ %s server.", c, servs[i]->id);
			}
		}
		int c = pthread_create(&pt, NULL, (void *) run_accept, accept_params[i]);
		if (c != 0) {
			if (servs[i]->id != NULL) errlog(delog, "Error creating thread: pthread errno = %i, server %s is shutting down.", c, servs[i]->id);
			close(accept_params[i]->server_fd);
		}
	}
	pthread_t tt;
	pthread_cond_init(&chunk_wake, NULL);
	pthread_mutex_init(&chunk_wake_mut, NULL);
	pthread_mutex_init(&glob_tick_mut, NULL);
	pthread_cond_init(&glob_tick_cond, NULL);
	if (worlds == NULL) return -1;
	for (size_t i = 0; i < worlds->size; i++) {
		if (worlds->data[i] == NULL) continue;
		pthread_create(&tt, NULL, &world_tick, worlds->data[i]);
	}
	pthread_create(&tt, NULL, &main_tick_thread, NULL);
	for (int i = 0; i < overworld->chl_count; i++) {
		pthread_create(&tt, NULL, &chunkloadthr, (size_t) i);
	}
	//TODO: start wake thread
	char line[4096];
	while (sr > 0) {
		if (readLine(STDIN_FILENO, line, 4096) < 0) {
			sleep(1);
			continue;
		}
		callCommand(NULL, line);
	}
	return 0;
}
