/*
 * util.c
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "xstring.h"
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include "util.h"
#include "hashmap.h"

#ifdef MEM_LEAK_DEBUG

struct alloc {
	size_t size;
	char* file;
	int line;
};

struct hashmap* mem_alloced;

uint64_t mem_key(char* file, uint16_t line) {
	uint64_t dkey = 0;
	size_t fl = strlen(file);
	memcpy(&dkey, file, fl < 6 ? fl : 6);
	memcpy(((uint8_t*)&dkey) + 6, &line, 2);
	return dkey;
}

void printAlloced() {
	struct hashmap* proc = new_hashmap_memsafe(1,0);
	BEGIN_HASHMAP_ITERATION(mem_alloced)
	struct alloc* al = (struct alloc*)value;
	uint64_t dkey = mem_key(al->file, al->line);
	struct alloc* nal = get_hashmap(proc, dkey);
	if(nal == NULL) {
		nal = xmalloc_real(sizeof(struct alloc), NULL, 0);
		nal->size = al->size;
		nal->line = al->line;
		nal->file = al->file;
		put_hashmap_memsafe(proc, dkey, nal);
	} else {
		nal->size += al->size;
	}
	END_HASHMAP_ITERATION(mem_alloced)
	BEGIN_HASHMAP_ITERATION(proc)
	struct alloc* al = (struct alloc*)value;
	if(al->size >1024) printf("%s:%i using %lu kB\n", al->file, al->line, al->size / 1024);
	xfree_real(al, NULL, 0);
	END_HASHMAP_ITERATION(proc)
	del_hashmap(proc);
}

void* xmalloc_real(size_t size, char* file, int line) {
#else
void* xmalloc(size_t size) {
#endif
	if (size > 10485760) {
		//printf("Big malloc %u!\n", size);
	}
	void* m = malloc(size);
	if (m == NULL) {
		printf("Out of Memory! @ malloc size %u\n", size);
		exit(1);
	}
#ifdef MEM_LEAK_DEBUG
	if (file != NULL) {
		if (mem_alloced == NULL) mem_alloced = new_hashmap_memsafe(1, 1);
		struct alloc* al = xmalloc_real(sizeof(struct alloc), NULL, 0);
		al->file = file;
		al->line = line;
		al->size = size;
		put_hashmap_memsafe(mem_alloced, (uint64_t) m, al);
	}
#endif
	return m;
}

#ifdef MEM_LEAK_DEBUG
void xfree_real(void* ptr, char* file, int line) {
#else
void xfree(void* ptr) {
#endif
#ifdef MEM_LEAK_DEBUG
	if (file != NULL) {
		if (mem_alloced == NULL) mem_alloced = new_hashmap_memsafe(1, 1);
		struct alloc* al = (struct alloc*) get_hashmap(mem_alloced, (uint64_t) ptr);
		if (al != NULL) {
			put_hashmap_memsafe(mem_alloced, (uint64_t) ptr, NULL);
			xfree_real(al, NULL, 0);
		}
	}
#endif
	free(ptr);
}

#ifdef MEM_LEAK_DEBUG
void* xcalloc_real(size_t size, char* file, int line) {
#else
void* xcalloc(size_t size) {
#endif
	if (size > 10485760) {
		//printf("Big calloc %u!\n", size);
	}
	void* m = calloc(1, size);
	if (m == NULL) {
		printf("Out of Memory! @ calloc size %u\n", size);
		exit(1);
	}
#ifdef MEM_LEAK_DEBUG
	if (file != NULL) {
		if (mem_alloced == NULL) mem_alloced = new_hashmap_memsafe(1, 1);
		struct alloc* al = xmalloc_real(sizeof(struct alloc), NULL, 0);
		al->file = file;
		al->line = line;
		al->size = size;
		put_hashmap_memsafe(mem_alloced, (uint64_t) m, al);
	}
#endif
	return m;
}

#ifdef MEM_LEAK_DEBUG
void* xrealloc_real(void* ptr, size_t size, char* file, int line) {
#else
void* xrealloc(void* ptr, size_t size) {
#endif
	if (size == 0) {
#ifdef MEM_LEAK_DEBUG
		xfree_real(ptr, file, line);
#else
		xfree(ptr);
#endif
		return NULL;
	}
	if (ptr == NULL) {
#ifdef MEM_LEAK_DEBUG
		return xmalloc_real(size, file, line);
#else
		return xmalloc(ptr);
#endif
	}
	if (size > 10485760) {
		//printf("Big realloc %u!\n", size);
	}
	void* m = realloc(ptr, size);
	if (m == NULL) {
		printf("Out of Memory! @ realloc size %u\n", size);
		exit(1);
	}
#ifdef MEM_LEAK_DEBUG
	if (file != NULL) {
		if (mem_alloced == NULL) mem_alloced = new_hashmap(1, 1);
		struct alloc* al = (struct alloc*) get_hashmap(mem_alloced, (uint64_t) ptr);
		if (al == NULL) {
			al = xmalloc_real(sizeof(struct alloc), NULL, 0);
			al->file = file;
			al->line = line;
			al->size = size;
			put_hashmap_memsafe(mem_alloced, (uint64_t) m, al);
			printf("nall\n");
		} else {
			al->size = size;
			if (ptr != m) {
				put_hashmap_memsafe(mem_alloced, (uint64_t) ptr, NULL);
				put_hashmap_memsafe(mem_alloced, (uint64_t) m, al);
			}
		}
	}
#endif
	return m;
}

void* xcopy(const void* ptr, size_t size, size_t expand) {
	void* alloc = xmalloc(size + expand);
	memcpy(alloc, ptr, size);
	return alloc;
}

char* xstrdup(const char* str, size_t expand) {
	return str == NULL ? NULL : xcopy(str, strlen(str) + 1, expand);
}

// copies up the maxsize bytes from src to dst, until a null byte is reached.
// does not copy the null byte. returns pointer to byte after the last byte of the dst after copying
char* xstrncat(char* dst, size_t maxsize, char* src) {
	const char *maxcdst = dst + maxsize;
	char *cdst = dst;
	for (char *csrc = src; *csrc && cdst < maxcdst; ++cdst, ++csrc)
		*cdst = *csrc;
	return cdst;
}

int recur_mkdir(const char* path, mode_t mode) {
	char rp[PATH_MAX];
	realpath(path, rp);
	size_t pl = strlen(rp);
	char* pp[16];
	int ppi = 0;
	for (int i = 0; i < pl; i++) {
		if (rp[i] == '/') {
			if (ppi == 16) break;
			pp[ppi++] = &rp[i] + 1;
			rp[i] = 0;
		}
	}
	if (strlen(pp[ppi - 1]) == 0) ppi--;
	char vp[pl + 1];
	vp[pl] = 0;
	vp[0] = 0;
	for (int i = 0; i < ppi; i++) {
		strcat(vp, "/");
		strcat(vp, pp[i]);
		int r = mkdir(vp, mode);
		if (r == -1 && errno != EEXIST) {
			return -1;
		}
	}
	return 0;
}

int memeq(const unsigned char* mem1, size_t mem1_size, const unsigned char* mem2, size_t mem2_size) {
	if (mem1 == NULL || mem2 == NULL) return 0;
	if (mem1 == mem2 && mem1_size == mem2_size) return 1;
	if (mem1_size != mem2_size) return 0;
	for (int i = 0; i < mem1_size; i++) {
		if (mem1[i] != mem2[i]) {
			return 0;
		}
	}
	return 1;
}

int memseq(const unsigned char* mem, size_t mem_size, const unsigned char c) {
	if (mem == NULL) return 0;
	for (int i = 0; i < mem_size; i++) {
		if (mem[i] != c) {
			return 0;
		}
	}
	return 1;
}
