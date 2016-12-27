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
	coll->capacity = capacity;
	coll->data = xmalloc((capacity == 0 ? 1 : capacity) * sizeof(void*));
	coll->rc = capacity == 0 ? 1 : 0;
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
	for (int i = 0; i < coll->size; i++) {
		if (coll->data[i] == NULL) {
			coll->count++;
			coll->data[i] = data;
			if (coll->mc) pthread_rwlock_unlock(&coll->data_mutex);
			return 0;
		}
	}
	if (coll->size == coll->rc && coll->capacity == 0) {
		coll->rc++;
		coll->data = xrealloc(coll->data, coll->rc * sizeof(void*));
	} else if (coll->capacity > 0 && coll->size == coll->capacity) {
		errno = EINVAL;
		return -1;
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

