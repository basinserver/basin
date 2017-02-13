/*
 * util.h
 *
 *  Created on: Nov 17, 2015
 *      Author: root
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

//#define MEM_LEAK_DEBUG

#ifdef MEM_LEAK_DEBUG

#define xmalloc(size) malloc(size)

#define xfree(ptr) free(ptr)

#define xcalloc(size) calloc(1, size)

#define xrealloc(ptr, size) realloc(ptr, size)

#else

void* xmalloc(size_t size);

void xfree(void* ptr);

void* xcalloc(size_t size);

void* xrealloc(void* ptr, size_t size);

#endif

void* xcopy(const void* ptr, size_t size, size_t expand);

char* xstrdup(const char* str, size_t expand);

char* xstrncat(char* dst, size_t maxsize, char* src);

int recur_mkdir(const char* path, mode_t mode);

int memeq(const unsigned char* mem1, size_t mem1_size, const unsigned char* mem2, size_t mem2_size);

int memseq(const unsigned char* mem, size_t mem_size, const unsigned char c);

pid_t gettid();

#endif /* UTIL_H_ */
