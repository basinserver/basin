/*
 * log.c
 *
 *  Created on: Nov 22, 2015
 *      Author: root
 */
#include "log.h"
#include <stdio.h>
#include <sys/time.h>
#include "xstring.h"
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>

void acclog(struct logsess* logsess, char* template, ...) {
	if (!logsess->pi) {
		if (pthread_mutex_init(&logsess->lmutex, NULL) == -1) {
			printf("Failed to create logging mutex! %s\n", strerror(errno));
			logsess->pi = -1;
		} else {
			logsess->pi = 1;
		}
	}
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if (logsess->pi == 1) pthread_mutex_lock(&logsess->lmutex);
	struct tm *ctm = localtime(&tv.tv_sec);
	char ct[32]; // the above uses a static buffer, so problems could ensue, but it would be the same time being overwritten...
	strftime(ct, 31, "%Y-%m-%d %H:%M:%S", ctm);
	va_list arg;
	va_start(arg, template);
	size_t ctl = strlen(ct);
	size_t tl = strlen(template);
	char tp[ctl + 5 + tl];
	tp[0] = '[';
	memcpy(tp + 1, ct, ctl);
	tp[1 + ctl] = ']';
	tp[2 + ctl] = ' ';
	memcpy(tp + 3 + ctl, template, tl);
	tp[3 + ctl + tl] = '\n';
	tp[4 + ctl + tl] = 0;
	if (vfprintf(stdout, tp, arg) < 0) {
		errlog(logsess, "Failed writing to stdout!");
	}
	va_end(arg);
	va_start(arg, template);
	if (logsess->access_fd != NULL) {
		if (vfprintf(logsess->access_fd, tp, arg) < 0) {
			errlog(logsess, "Failed writing to accesslog!");
		}
	}
	va_end(arg);
	if (logsess->pi == 1) pthread_mutex_unlock(&logsess->lmutex);
}

void errlog(struct logsess* logsess, char* template, ...) {
	if (!logsess->pi) {
		if (pthread_mutex_init(&logsess->lmutex, NULL) == -1) {
			printf("Failed to create logging mutex! %s\n", strerror(errno));
			logsess->pi = -1;
		} else {
			logsess->pi = 1;
		}
	}
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if (logsess->pi == 1) pthread_mutex_lock(&logsess->lmutex);
	struct tm *ctm = localtime(&tv.tv_sec);
	char ct[32]; // the above uses a static buffer, so problems could ensue, but it would be the same time being overwritten...
	strftime(ct, 31, "%Y-%m-%d %H:%M:%S", ctm);
	va_list arg;
	va_start(arg, template);
	size_t ctl = strlen(ct);
	size_t
	tl = strlen(template);
	char tp[ctl + 5 + tl];
	tp[0] = '[';
	memcpy(tp + 1, ct, ctl);
	tp[1 + ctl] = ']';
	tp[2 + ctl] = ' ';
	memcpy(tp + 3 + ctl, template, tl);
	tp[3 + ctl + tl] = '\n';
	tp[4 + ctl + tl] = 0;
	if (vfprintf(stdout, tp, arg) < 0) {
		//TODO: we can't write to stdout, nothing we can do!
	}
	va_end(arg);
	va_start(arg, template);
	if (logsess->error_fd != NULL) {
		if (vfprintf(logsess->error_fd, tp, arg) < 0) {
			//its in the console
		}
	}
	va_end(arg);
	if (logsess->pi == 1) pthread_mutex_unlock(&logsess->lmutex);
}

