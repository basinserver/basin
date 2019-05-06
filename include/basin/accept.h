#ifndef BASIN_ACCEPT_H_
#define BASIN_ACCEPT_H_

#include <basin/server.h>

void run_accept(struct server* param);

struct timeval {
    int32_t tv_sec;
    int32_t tv_usec;
};

#endif /* BASIN_ACCEPT_H_ */
