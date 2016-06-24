/*
 * accept.h
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */

#ifndef ACCEPT_H_
#define ACCEPT_H_

#include "config.h"
#include "collection.h"
#include <sys/socket.h>
#include "work.h"
#include <netinet/ip6.h>

struct mcs {
		char* motd;
		size_t max_players;
};

struct accept_param {
		int server_fd;
		int port;
		struct cnode* config;
		int works_count;
		struct work_param** works;
		struct logsess* logsess;
		struct mcs* mcs;
};

struct conn {
		int fd;
		int state;
		struct sockaddr_in6 addr;
		socklen_t addrlen;
		unsigned char* readBuffer;
		size_t readBuffer_size;
		size_t readBuffer_checked;
		unsigned char* writeBuffer;
		size_t writeBuffer_size;
		int comp;
		struct queue* outgoingPacket;
		struct queue* incomingPacket;
		char* host_ip;
		uint16_t host_port;
		struct mcs* mcs;
};

void run_accept(struct accept_param* param);

#endif /* ACCEPT_H_ */
