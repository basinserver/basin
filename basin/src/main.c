/*
 * main.c
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#include <unistd.h>
#include <stdio.h>
#include "config.h"
#include <errno.h>
#include "xstring.h"
#include "version.h"
#include "util.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "streams.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "accept.h"
#include "globals.h"
#include "collection.h"
#include "work.h"
#include <sys/types.h>
#include <time.h>
#include "world.h"
#include "worldmanager.h"
#include "server.h"
#include "game.h"
#include "block.h"
#include "crafting.h"
#include "tools.h"
#include "smelting.h"
#include "command.h"
#include "queue.h"
#include "profile.h"
#include "plugin.h"
#include <openssl/rand.h>

void main_tick() {
	pthread_cond_broadcast (&glob_tick_cond);
	pthread_rwlock_wrlock(&playersToLoad->data_mutex);
	for (size_t i = 0; i < playersToLoad->size; i++) {
		struct player* player = (struct player*) playersToLoad->data[i];
		if (player == NULL) continue;
		spawnPlayer(overworld, player);
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
		//printf("defr %i\n", defunctChunks->count);
		pthread_rwlock_wrlock(&defunctPlayers->data_mutex);
		for (size_t i = 0; i < defunctPlayers->size; i++) {
			struct player* def = defunctPlayers->data[i];
			if (def == NULL) continue;
			if (def->defunct++ >= 21) {
				freeEntity(def->entity);
				freePlayer(def);
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
				freeChunk(def);
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
}

int main(int argc, char* argv[]) {
	signal(SIGPIPE, SIG_IGN);
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
	cfg = loadConfig("main.cfg");
	if (cfg == NULL) {
		printf("Error loading Config<%s>: %s\n", "main.cfg", errno == EINVAL ? "File doesn't exist!" : strerror(errno));
		return 1;
	}
	struct cnode* dm = getUniqueByCat(cfg, CAT_DAEMON);
	if (dm == NULL) {
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
	delog = xmalloc(sizeof(struct logsess));
	delog->pi = 0;
	delog->access_fd = NULL;
	const char* el = getConfigValue(dm, "error-log");
	delog->error_fd = el == NULL ? NULL : fopen(el, "a"); // fopen will return NULL on error, which works.
//TODO: chown group to de-escalated
	int servsl;
	struct cnode** servs = getCatsByCat(cfg, CAT_SERVER, &servsl);
	int sr = 0;
	struct accept_param* aps[servsl];
	if (servsl != 1) {
		errlog(delog, "Only one server block is supported at this time.");
		return -1;
	}
	players = new_hashmap(1, 1);
	defunctPlayers = new_collection(16, 1);
	defunctChunks = new_collection(16, 1);
	playersToLoad = new_collection(16, 1);
	init_materials();
	printf("Materials Initialized\n");
	init_blocks();
	printf("Blocks Initialized\n");
	init_crafting();
	printf("Crafting Initialized\n");
	init_tools();
	printf("Tools Initialized\n");
	init_items();
	printf("Items Initialized\n");
	init_smelting();
	printf("Smelting Initialized\n");
	init_base_commands();
	printf("Commands Initialized\n");
	init_plugins();
	printf("Plugins Initialized\n");
	init_encryption();
	printf("Encryption Initialized\n");
	for (int i = 0; i < servsl; i++) {
		struct cnode* serv = servs[i];
		const char* bind_mode = getConfigValue(serv, "bind-mode");
		const char* bind_ip = NULL;
		int port = -1;
		const char* bind_file = NULL;
		int namespace = -1;
		int ba = 0;
		int ip6 = 0;
		if (streq(bind_mode, "tcp")) {
			bind_ip = getConfigValue(serv, "bind-ip");
			if (streq(bind_ip, "0.0.0.0")) {
				ba = 1;
			}
			ip6 = ba || contains(bind_ip, ":");
			const char* bind_port = getConfigValue(serv, "bind-port");
			if (!strisunum(bind_port)) {
				if (serv->id != NULL) errlog(delog, "Invalid bind-port for server: %s", serv->id);
				else errlog(delog, "Invalid bind-port for server.");
				continue;
			}
			port = atoi(bind_port);
			namespace = ip6 ? PF_INET6 : PF_INET;;
		} else if (streq(bind_mode, "unix")) {
			bind_file = getConfigValue(serv, "bind-file");
			namespace = PF_LOCAL;
		} else {
			if (serv->id != NULL) errlog(delog, "Invalid bind-mode for server: %s", serv->id);
			else errlog(delog, "Invalid bind-mode for server.");
			continue;
		}
		const char* tcc = getConfigValue(serv, "threads");
		if (!strisunum(tcc)) {
			if (serv->id != NULL) errlog(delog, "Invalid threads for server: %s", serv->id);
			else errlog(delog, "Invalid threads for server.");
			continue;
		}
		int tc = atoi(tcc);
		if (tc < 1) {
			if (serv->id != NULL) errlog(delog, "Invalid threads for server: %s, must be greater than 1.", serv->id);
			else errlog(delog, "Invalid threads for server, must be greater than 1.");
			continue;
		}
		const char* mcc = getConfigValue(serv, "max-players");
		if (!strisunum(mcc)) {
			if (serv->id != NULL) errlog(delog, "Invalid max-players for server: %s", serv->id);
			else errlog(delog, "Invalid max-players for server.");
			continue;
		}
		int mc = atoi(mcc);
		const char* mcc2 = getConfigValue(serv, "difficulty");
		if (!strisunum(mcc2)) {
			if (serv->id != NULL) errlog(delog, "Invalid difficulty for server: %s", serv->id);
			else errlog(delog, "Invalid difficulty for server.");
			continue;
		}
		int mc2 = atoi(mcc2);
		const char* dmotd = getConfigValue(serv, "motd");
		if (dmotd == NULL) {
			if (serv->id != NULL) errlog(delog, "Invalid motd for server: %s, assuming default.", serv->id);
			else errlog(delog, "Invalid motd for server, assuming default.");
			dmotd = "A Minecraft Server";
		}
		const char* onl = getConfigValue(serv, "online-mode");
		if (onl == NULL) {
			if (serv->id != NULL) errlog(delog, "Invalid online-mode for server: %s, assuming true.", serv->id);
			else errlog(delog, "Invalid online-mode for server, assuming true.");
			onl = "true";
		}
		int ionl = streq_nocase(onl, "true");
		sock: ;
		int sfd = socket(namespace, SOCK_STREAM, 0);
		if (sfd < 0) {
			if (serv->id != NULL) errlog(delog, "Error creating socket for server: %s, %s", serv->id, strerror(errno));
			else errlog(delog, "Error creating socket for server, %s", strerror(errno));
			continue;
		}
		int one = 1;
		int zero = 0;
		if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void*) &one, sizeof(one)) == -1) {
			if (serv->id != NULL) errlog(delog, "Error setting SO_REUSEADDR for server: %s, %s", serv->id, strerror(errno));
			else errlog(delog, "Error setting SO_REUSEADDR for server, %s", strerror(errno));
			close (sfd);
			continue;
		}
		if (namespace == PF_INET || namespace == PF_INET6) {
			if (ip6) {
				if (setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, (void*) &zero, sizeof(zero)) == -1) {
					if (serv->id != NULL) errlog(delog, "Error unsetting IPV6_V6ONLY for server: %s, %s", serv->id, strerror(errno));
					else errlog(delog, "Error unsetting IPV6_V6ONLY for server, %s", strerror(errno));
					close (sfd);
					continue;
				}
				struct sockaddr_in6 bip;
				bip.sin6_flowinfo = 0;
				bip.sin6_scope_id = 0;
				bip.sin6_family = AF_INET6;
				if (ba) bip.sin6_addr = in6addr_any;
				else if (!inet_pton(AF_INET6, bind_ip, &(bip.sin6_addr))) {
					close (sfd);
					if (serv->id != NULL) errlog(delog, "Error binding socket for server: %s, invalid bind-ip", serv->id);
					else errlog(delog, "Error binding socket for server, invalid bind-ip");
					continue;
				}
				bip.sin6_port = htons(port);
				if (bind(sfd, (struct sockaddr*) &bip, sizeof(bip))) {
					close (sfd);
					if (ba) {
						namespace = PF_INET;
						ip6 = 0;
						goto sock;
					}
					if (serv->id != NULL) errlog(delog, "Error binding socket for server: %s, %s", serv->id, strerror(errno));
					else errlog(delog, "Error binding socket for server, %s\n", strerror(errno));
					continue;
				}
			} else {
				struct sockaddr_in bip;
				bip.sin_family = AF_INET;
				if (!inet_aton(bind_ip, &(bip.sin_addr))) {
					close (sfd);
					if (serv->id != NULL) errlog(delog, "Error binding socket for server: %s, invalid bind-ip", serv->id);
					else errlog(delog, "Error binding socket for server, invalid bind-ip");
					continue;
				}
				bip.sin_port = htons(port);
				if (bind(sfd, (struct sockaddr*) &bip, sizeof(bip))) {
					if (serv->id != NULL) errlog(delog, "Error binding socket for server: %s, %s", serv->id, strerror(errno));
					else errlog(delog, "Error binding socket for server, %s\n", strerror(errno));
					close (sfd);
					continue;
				}
			}
		} else if (namespace == PF_LOCAL) {
			struct sockaddr_un uip;
			strncpy(uip.sun_path, bind_file, 108);
			if (bind(sfd, (struct sockaddr*) &uip, sizeof(uip))) {
				if (serv->id != NULL) errlog(delog, "Error binding socket for server: %s, %s", serv->id, strerror(errno));
				else errlog(delog, "Error binding socket for server, %s\n", strerror(errno));
				close (sfd);
				continue;
			}
		} else {
			if (serv->id != NULL) errlog(delog, "Invalid family for server: %s", serv->id);
			else errlog(delog, "Invalid family for server\n");
			close (sfd);
			continue;
		}
		if (listen(sfd, 50)) {
			if (serv->id != NULL) errlog(delog, "Error listening on socket for server: %s, %s", serv->id, strerror(errno));
			else errlog(delog, "Error listening on socket for server, %s", strerror(errno));
			close (sfd);
			continue;
		}
		if (fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL) | O_NONBLOCK) < 0) {
			if (serv->id != NULL) errlog(delog, "Error setting non-blocking for server: %s, %s", serv->id, strerror(errno));
			else errlog(delog, "Error setting non-blocking for server, %s", strerror(errno));
			close (sfd);
			continue;
		}
		struct logsess* slog = xmalloc(sizeof(struct logsess));
		slog->pi = 0;
		const char* lal = getConfigValue(serv, "access-log");
		slog->access_fd = lal == NULL ? NULL : fopen(lal, "a");
		const char* lel = getConfigValue(serv, "error-log");
		slog->error_fd = lel == NULL ? NULL : fopen(lel, "a");
		if (serv->id != NULL) acclog(slog, "Server %s listening for connections!", serv->id);
		else acclog(slog, "Server listening for connections!");
		max_players = mc;
		motd = xstrdup(dmotd, 0);
		online_mode = ionl;
		difficulty = mc2;
		const char* ovr = getConfigValue(serv, "world");
		if (ovr == NULL) {
			if (serv->id != NULL) errlog(delog, "No world defined for server: %s", serv->id);
			else errlog(delog, "No world defined for server");
			close (sfd);
			continue;
		}
		overworld = newWorld(8);
		loadWorld(overworld, ovr);
		printf("Overworld Loaded\n");
		//nether = newWorld();
		//loadWorld(nether, neth);
		//endworld = newWorld();
		//loadWorld(endworld, ed);
		worlds = new_collection(0, 1);
		add_collection(worlds, overworld);
		//add_collection(worlds, nether);
		//add_collection(worlds, endworld);
		struct accept_param* ap = xmalloc(sizeof(struct accept_param));
		ap->port = port;
		ap->server_fd = sfd;
		ap->config = serv;
		ap->works_count = tc;
		ap->works = xmalloc(sizeof(struct work_param*) * tc);
		ap->logsess = slog;
		for (int x = 0; x < tc; x++) {
			struct work_param* wp = xmalloc(sizeof(struct work_param));
			wp->conns = new_collection(mc < 1 ? 0 : mc / tc, 1);
			wp->logsess = slog;
			wp->i = x;
			wp->sport = port;
			ap->works[x] = wp;
		}
		aps[i] = ap;
		sr++;
	}
	const char* uids = getConfigValue(dm, "uid");
	const char* gids = getConfigValue(dm, "gid");
	uid_t uid = uids == NULL ? 0 : atol(uids);
	uid_t gid = gids == NULL ? 0 : atol(gids);
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
		for (int x = 0; x < aps[i]->works_count; x++) {
			int c = pthread_create(&pt, NULL, (void *) run_work, aps[i]->works[x]);
			if (c != 0) {
				if (servs[i]->id != NULL) errlog(delog, "Error creating thread: pthread errno = %i, this will cause occasional connection hanging @ %s server.", c, servs[i]->id);
				else errlog(delog, "Error creating thread: pthread errno = %i, this will cause occasional connection hanging.", c);
			}
		}
		int c = pthread_create(&pt, NULL, (void *) run_accept, aps[i]);
		if (c != 0) {
			if (servs[i]->id != NULL) errlog(delog, "Error creating thread: pthread errno = %i, server %s is shutting down.", c, servs[i]->id);
			else errlog(delog, "Error creating thread: pthread errno = %i, server is shutting down.", c);
			close(aps[i]->server_fd);
		}
	}
	pthread_t tt;
	chunk_input = new_queue(0, 1);
	pthread_cond_init(&chunk_wake, NULL);
	pthread_mutex_init(&chunk_wake_mut, NULL);
	pthread_mutex_init(&glob_tick_mut, NULL);
	pthread_cond_init(&glob_tick_cond, NULL);
	if (worlds == NULL) return -1;
	for (size_t i = 0; i < worlds->size; i++) {
		if (worlds->data[i] == NULL) continue;
		pthread_create(&tt, NULL, &tick_world, worlds->data[i]);
	}
	pthread_create(&tt, NULL, &main_tick_thread, NULL);
	for (int i = 0; i < overworld->chl_count; i++) {
		pthread_create(&tt, NULL, &chunkloadthr, (size_t) i);
	}
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
