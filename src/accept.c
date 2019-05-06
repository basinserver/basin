#include <basin/accept.h>
#include <basin/work.h>
#include <basin/connection.h>
#include <avuna/queue.h>
#include <avuna/string.h>
#include <avuna/netmgr.h>
#include <avuna/pmem.h>
#include <pthread.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#include <netinet/tcp.h>
#include <netinet/in.h>


void run_accept(struct server* server) {
    static int one = 1;
    struct timeval timeout;
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    struct pollfd spfd;
    spfd.events = POLLIN;
    spfd.revents = 0;
    spfd.fd = server->fd;
    while (1) {
        struct mempool* pool = mempool_new();
        struct connection* conn = pcalloc(pool, sizeof(struct connection));
        conn->pool = pool;
        conn->addrlen = sizeof(struct sockaddr_in6);
        conn->managed_conn = pcalloc(conn->pool, sizeof(struct netmgr_connection));
        conn->managed_conn->pool = conn->pool;
        conn->managed_conn->extra = conn;
        conn->compression_state = -1;
        conn->server = server;
        buffer_init(&conn->managed_conn->read_buffer, conn->pool);
        buffer_init(&conn->managed_conn->write_buffer, conn->pool);
        if (poll(&spfd, 1, -1) < 0) {
            printf("Error while polling server: %s\n", strerror(errno));
            pfree(pool);
            continue;
        }
        if ((spfd.revents ^ POLLIN) != 0) {
            printf("Error after polling server: %i (poll revents)!\n", spfd.revents);
            pfree(pool);
        }
        spfd.revents = 0;
        int fd = accept(server->fd, (struct sockaddr*) &conn->addr, &conn->addrlen);
        if (fd < 0) {
            if (errno == EAGAIN) continue;
            printf("Error while accepting client: %s\n", strerror(errno));
            pfree(pool);
            continue;
        }
        conn->managed_conn->fd = fd;
        conn->managed_conn->read = connection_read;
        conn->managed_conn->on_closed = connection_on_closed;
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout))) printf("Setting recv timeout failed! %s\n", strerror(errno));
        if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout))) printf("Setting send timeout failed! %s\n", strerror(errno));
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *) &one, sizeof(one))) printf("Setting TCP_NODELAY failed! %s\n", strerror(errno));
        if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) < 0) {
            printf("Setting O_NONBLOCK failed! %s, this error cannot be recovered, closing client.\n", strerror(errno));
            close(fd);
            continue;
        }
        queue_push(server->prepared_connections, conn);
    }
    pthread_cancel (pthread_self());
}
