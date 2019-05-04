#include <avuna/string.h>
#include <avuna/hash.h>
#include <avuna/pmem.h>
#include <avuna/util.h>
#include <time.h>
#include <stdio.h>

#define ENABLE_PROFILER
// #define ENABLE_PROFILER_MT

struct mempool* profiler_pool;
struct hashmap* profiler_sections;

struct profiler_section {
		char* name;
		double ms_spent;
		double sectionLastStart;
		double creation;
		double maxTime;
};

void beginProfilerSection(char* name) {
#ifdef ENABLE_PROFILER
	if (profiler_sections == NULL) {
		profiler_pool = mempool_new();
#ifdef ENABLE_PROFILER_MT
		profiler_sections = hashmap_new_thread(32, profiler_pool);
#else
		profiler_sections = hashmap_new(32, profiler_pool);
#endif
	}
	struct profiler_section* section = hashmap_get(profiler_sections, name);
	if (section != NULL) {
		if (section->sectionLastStart > 0.) return;
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		section->sectionLastStart = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
	} else {
		section = pcalloc(profiler_pool, sizeof(struct profiler_section));
		section->name = name;
		hashmap_put(profiler_sections, name, section);
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		section->sectionLastStart = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
		section->creation = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
	}
#endif
}

void endProfilerSection(char* name) {
#ifdef ENABLE_PROFILER
	if (profiler_sections == NULL) return;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	double now = ((double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.);
	struct profiler_section* section = hashmap_get(profiler_sections, name);
	if (section == NULL) return;
	if (section->sectionLastStart < 0.) return;
	double msa = now - section->sectionLastStart;
	section->ms_spent += msa;
	section->sectionLastStart = -1.;
	if (section->maxTime < msa) {
		section->maxTime = msa;
	}
#endif
}

void printProfiler() {
#ifdef ENABLE_PROFILER
	if (profiler_sections == NULL) return;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	double now = (double) ts.tv_nsec / 1000000. + (double) ts.tv_sec * 1000.;
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_rdlock(&profiler_sections->rwlock);
#endif
	ITER_MAP (profiler_sections) {
		struct profiler_section* ps = value;
		if (ps->sectionLastStart > 0.) {
			ps->ms_spent += now - ps->sectionLastStart;
			ps->sectionLastStart = -1.;
		}
		printf("%s: %f ms - %f ms/tick - %f maximum\n", ps->name, ps->ms_spent, ps->ms_spent / (now - ps->creation) * 50., ps->maxTime);
		ITER_MAP_END()
	}
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_unlock(&profiler_sections->rwlock);
#endif

#endif
}

void clearProfiler() {
#ifdef ENABLE_PROFILER
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_rdlock(&profiler_sections->rwlock);
#endif
	ITER_MAP (profiler_sections) {
		struct profiler_section* ps = value;
		ps->ms_spent = 0.;
		ps->maxTime = 0.;
		ITER_MAP_END()
	}
#ifdef ENABLE_PROFILER_MT
	pthread_rwlock_unlock(&profiler_sections->rwlock);
#endif
#endif
}
