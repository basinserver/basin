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

//#define MEM_LEAK_DEBUG

#ifdef MEM_LEAK_DEBUG

void* xmalloc_real(size_t size, char* file, int line);

void xfree_real(void* ptr, char* file, int line);

void* xcalloc_real(size_t size, char* file, int line);

void* xrealloc_real(void* ptr, size_t size, char* file, int line);

#define xmalloc(size) xmalloc_real(size, __FILE__ , __LINE__)

#define xfree(ptr) xfree_real(ptr, __FILE__ , __LINE__)

#define xcalloc(size) xcalloc_real(size, __FILE__ , __LINE__)

#define xrealloc(ptr, size) xrealloc_real(ptr, size, __FILE__ , __LINE__)

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

#endif /* UTIL_H_ */
