#include <basin/wake_thread.h>
#include <basin/connection.h>
#include <avuna/netmgr.h>
#include <avuna/queue.h>
#include <avuna/log.h>
#include <avuna/llist.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

void wake_thread(struct wake_thread_arg* arg) {
    size_t counter = 0;
    while (1) {
        struct connection* conn = queue_pop(arg->server->prepared_connections);
        struct netmgr_thread* worker = arg->work_params->data[counter];
        counter = (counter + 1) % arg->work_params->count;
        if (netmgr_add_connection(worker, conn->managed_conn)) {
            errlog(arg->server->logger, "Failed to add connection to worker! %s", strerror(errno));
            continue;
        }
    }
}

#pragma clang diagnostic pop