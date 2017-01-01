/*
 * profile.c
 *
 *  Created on: Dec 30, 2016
 *      Author: root
 */

#include "collection.h"
#include "xstring.h"
#include <time.h>
#include "util.h"
#include <stdio.h>

//#define ENABLE_PROFILER
//#define ENABLE_PROFILER_MT

struct collection* psec;

struct profiler_section {
		char* name;
		double ms_spent;
		double sectionLastStart;
		double creation;
};

void beginProfilerSection(char* name) {
#ifdef ENABLE_PROFILER
	int f = 0;
	if (psec == NULL) {
#ifdef ENABLE_PROFILER_MT
		psec = new_collection(16, 1);
		pthread_rwlock_rdlock(&psec->data_mutex);
#else
		psec = new_collection(16, 0);
#endif
	} else {
#ifdef ENABLE_PROFILER_MT
		pthread_rwlock_rdlock(&psec->data_mutex);
#endif
	}
	for (size_t i = 0; i < psec->size; i++) {
		struct profiler_section* ps = psec->data[i];
		if (ps == NULL) continue;
		if (streq_nocase(ps->name, name)) {
			if (ps->sectionLastStart > 0.) {
#ifdef ENABLE_PROFILER_MT
				psec = new_collection(16, 1);
				pthread_rwlock_rdlock(&psec->data_mutex);
#endif
				return;
			}
			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			ps->sectionLastStart = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
			f = 1;
			break;
		}
	}
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_unlock(&psec->data_mutex);
#endif
	if (!f) {
		struct profiler_section* ps = xmalloc(sizeof(struct profiler_section));
		ps->name = name;
		ps->ms_spent = 0.;
		add_collection(psec, ps);
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ps->sectionLastStart = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
		ps->creation = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
	}
#endif
}

void endProfilerSection(char* name) {
#ifdef ENABLE_PROFILER
	if (psec == NULL) return;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	double now = ((double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.);
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_rdlock(&psec->data_mutex);
#endif
	for (size_t i = 0; i < psec->size; i++) {
		struct profiler_section* ps = psec->data[i];
		if (ps == NULL) continue;
		if (streq_nocase(ps->name, name)) {
			if (ps->sectionLastStart < 0.) {
#ifdef ENABLE_PROFILER_MT
				pthread_rwlock_unlock(&psec->data_mutex);
#endif
				goto epsr;
			}
			ps->ms_spent += now - ps->sectionLastStart;
			ps->sectionLastStart = -1.;
#ifdef ENABLE_PROFILER_MT
			pthread_rwlock_unlock(&psec->data_mutex);
#endif
			goto epsr;
		}
	}
	epsr:;
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_unlock(&psec->data_mutex);
#endif
#endif
}

void printProfiler() {
#ifdef ENABLE_PROFILER
	if (psec == NULL) return;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	double now = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_rdlock(&psec->data_mutex);
#endif
	for (size_t i = 0; i < psec->size; i++) {
		struct profiler_section* ps = psec->data[i];
		if (ps == NULL) continue;
		if (ps->sectionLastStart > 0.) {
			ps->ms_spent += now - ps->sectionLastStart;
			ps->sectionLastStart = -1.;
		}
		printf("%s: %f ms - %f ms/tick\n", ps->name, ps->ms_spent, ps->ms_spent / (now - ps->creation) * 50.);
	}
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_unlock(&psec->data_mutex);
#endif
#endif
}

void clearProfiler() {
#ifdef ENABLE_PROFILER
	if (psec == NULL) return;
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_rdlock(&psec->data_mutex);
#endif
	for (size_t i = 0; i < psec->size; i++) {
		struct profiler_section* ps = psec->data[i];
		if (ps == NULL) continue;
		ps->ms_spent = 0.;
	}
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_unlock(&psec->data_mutex);
#endif
#endif
}
