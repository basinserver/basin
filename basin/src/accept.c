/*
 * accept.c
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */
#include "accept.h"
#include "util.h"
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include "xstring.h"
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include "work.h"
#include <unistd.h>
#include "queue.h"
#include <pthread.h>

void run_accept(struct accept_param* param) {
	static int one = 1;
	static unsigned char onec = 1;
	struct timeval timeout;
	timeout.tv_sec = 60;
	timeout.tv_usec = 0;
	struct pollfd spfd;
	spfd.events = POLLIN;
	spfd.revents = 0;
	spfd.fd = param->server_fd;
	while (1) {
		struct conn * c = xmalloc(sizeof(struct conn));
		memset(&c->addr, 0, sizeof(struct sockaddr_in6));
		c->addrlen = sizeof(struct sockaddr_in6);
		c->readBuffer = NULL;
		c->readBuffer_size = 0;
		c->writeBuffer = NULL;
		c->writeBuffer_size = 0;
		c->writeBuffer_capacity = 0;
		c->comp = -1;
		c->state = 0;
		c->onll_username = NULL;
		c->disconnect = 0;
		c->host_ip = NULL;
		c->player = NULL;
		c->host_port = 0;
		c->protocolVersion = 0;
		c->verifyToken = 0;
		c->aes_ctx_enc = NULL;
		c->aes_ctx_dec = NULL;
		c->readDecBuffer = NULL;
		c->readDecBuffer_size = 0;
		memset(c->sharedSecret, 0, 16);
		if (poll(&spfd, 1, -1) < 0) {
			printf("Error while polling server: %s\n", strerror(errno));
			xfree(c);
			continue;
		}
		if ((spfd.revents ^ POLLIN) != 0) {
			printf("Error after polling server: %i (poll revents), closing server!\n", spfd.revents);
			xfree(c);
			close(param->server_fd);
			break;
		}
		spfd.revents = 0;
		int cfd = accept(param->server_fd, (struct sockaddr*) &c->addr, &c->addrlen);
		if (cfd < 0) {
			if (errno == EAGAIN) continue;
			printf("Error while accepting client: %s\n", strerror(errno));
			xfree(c);
			continue;
		}
		c->fd = cfd;
		if (setsockopt(cfd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout))) printf("Setting recv timeout failed! %s\n", strerror(errno));
		if (setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout))) printf("Setting send timeout failed! %s\n", strerror(errno));
		if (setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (void *) &one, sizeof(one))) printf("Setting TCP_NODELAY failed! %s\n", strerror(errno));
		if (fcntl(cfd, F_SETFL, fcntl(cfd, F_GETFL) | O_NONBLOCK) < 0) {
			printf("Setting O_NONBLOCK failed! %s, this error cannot be recovered, closing client.\n", strerror(errno));
			close(cfd);
			continue;
		}
		struct work_param* work = param->works[rand() % param->works_count];
		c->work = work;
		if (add_collection(work->conns, c)) { // TODO: send to lowest load, not random
			if (errno == EINVAL) {
				printf("Too many open connections! Closing client.\n");
			} else {
				printf("Collection failure! Closing client. %s\n", strerror(errno));
			}
			close(cfd);
			continue;
		}
		if (write(work->pipes[1], &onec, 1) < 1) {
			printf("Failed to write to wakeup pipe! Things may slow down. %s\n", strerror(errno));
		}
	}
	pthread_cancel (pthread_self());}
