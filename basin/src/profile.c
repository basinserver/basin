/*
 * profile.c
 *
 *  Created on: Dec 30, 2016
 *      Author: root
 */

#include "xstring.h"
#include <time.h>
#include "util.h"
#include <stdio.h>
#include "hashmap.h"

#define ENABLE_PROFILER
//#define ENABLE_PROFILER_MT

struct hashmap* psec;

struct profiler_section {
		char* name;
		double ms_spent;
		double sectionLastStart;
		double creation;
};

void beginProfilerSection(char* name) {
#ifdef ENABLE_PROFILER
	if (psec == NULL) {
#ifdef ENABLE_PROFILER_MT
		psec = new_hashmap(1, 1);
#else
		psec = new_hashmap(1, 0);
#endif
	}
	struct profiler_section* ps = get_hashmap(psec, (uint64_t) name);
	if (ps != NULL) {
		if (ps->sectionLastStart > 0.) return;
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ps->sectionLastStart = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
	} else {
		struct profiler_section* ps = xmalloc(sizeof(struct profiler_section));
		ps->name = name;
		ps->ms_spent = 0.;
		put_hashmap(psec, (uint64_t) name, ps);
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
	struct profiler_section* ps = get_hashmap(psec, (uint64_t) name);
	if (ps == NULL) return;
	if (ps->sectionLastStart < 0.) return;
	ps->ms_spent += now - ps->sectionLastStart;
	ps->sectionLastStart = -1.;
#endif
}

void printProfiler() {
#ifdef ENABLE_PROFILER
	if (psec == NULL) return;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	double now = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
	BEGIN_HASHMAP_ITERATION (psec)
	struct profiler_section* ps = value;
	if (ps->sectionLastStart > 0.) {
		ps->ms_spent += now - ps->sectionLastStart;
		ps->sectionLastStart = -1.;
	}
	printf("%s: %f ms - %f ms/tick\n", ps->name, ps->ms_spent, ps->ms_spent / (now - ps->creation) * 50.);
	END_HASHMAP_ITERATION (psec)
#endif
}

void clearProfiler() {
#ifdef ENABLE_PROFILER
	BEGIN_HASHMAP_ITERATION (psec)
	struct profiler_section* ps = value;
	ps->ms_spent = 0.;
	END_HASHMAP_ITERATION (psec)
#endif
}
