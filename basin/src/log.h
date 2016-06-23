/*
 * log.h
 *
 *  Created on: Nov 22, 2015
 *      Author: root
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

struct logsess {
		pthread_mutex_t lmutex;
		int pi;
		FILE* access_fd;
		FILE* error_fd;
};

void acclog(struct logsess* logsess, char* template, ...);

void errlog(struct logsess* logsess, char* template, ...);

#endif /* LOG_H_ */
