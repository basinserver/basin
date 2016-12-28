/*
 * queue.c
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#include "collection.h"
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "xstring.h"
#include "util.h"

struct collection* new_collection(size_t capacity, uint8_t mc) {
	struct collection* coll = xmalloc(sizeof(struct collection));
	if (capacity < 1) capacity = 1;
	coll->capacity = capacity;
	coll->data = xcalloc((capacity == 0 ? 1 : capacity) * sizeof(void*));
	coll->size = 0;
	coll->count = 0;
	coll->mc = mc;
	if (mc && pthread_rwlock_init(&coll->data_mutex, NULL)) {
		xfree(coll->data);
		coll->data = NULL;
		xfree(coll);
		return NULL;
	}

	return coll;
}

int del_collection(struct collection* coll) {
	if (coll == NULL || coll->data == NULL) return -1;
	if (coll->mc && pthread_rwlock_destroy(&coll->data_mutex)) return -1;
	xfree(coll->data);
	coll->data = NULL;
	xfree(coll);
	return 0;
}

int add_collection(struct collection* coll, void* data) {
	if (coll->mc) pthread_rwlock_wrlock(&coll->data_mutex);
	if (coll->size >= coll->capacity) {
		for (int i = 0; i < coll->capacity; i++) {
			if (coll->data[i] == NULL) {
				coll->count++;
				if (i >= coll->size) coll->size++;
				coll->data[i] = data;
				if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
				return 0;
			}
		}
		if (coll->count >= coll->capacity) {
			size_t oc = coll->capacity;
			coll->capacity += 1024 / sizeof(void*);
			coll->data = xrealloc(coll->data, coll->capacity * sizeof(void*));
			memset(coll->data + oc, 0, 1024);
		}
	}
	coll->data[coll->size++] = data;
	coll->count++;
	if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
	return 0;
}

int rem_collection(struct collection* coll, void* data) {
	if (coll->mc) pthread_rwlock_wrlock(&coll->data_mutex);
	for (int i = 0; i < coll->size; i++) {
		if (coll->data[i] == data) {
			coll->data[i] = NULL;
			coll->count--;
			if (i == coll->size - 1) coll->size--;
			if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
			return 0;
		}
	}
	if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
	return -1;
}

int contains_collection(struct collection* coll, void* data) {
	if (coll->mc) pthread_rwlock_rdlock(&coll->data_mutex);
	for (int i = 0; i < coll->size; i++) {
		if (coll->data[i] == data) {
			if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
			return 1;
		}
	}
	if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
	return 0;
}

void ensure_collection(struct collection* coll, size_t size) {
	if (coll->mc) pthread_rwlock_rdlock(&coll->data_mutex);
	if (coll->count + size >= coll->capacity) {
		size_t oc = coll->capacity;
		coll->capacity = coll->count + size;
		coll->data = xrealloc(coll->data, coll->capacity * sizeof(void*));
		memset(coll->data + oc, 0, (coll->capacity - oc) * sizeof(void*));
	}
	if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
}
