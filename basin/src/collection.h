/*
 * queue.h
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#ifndef COLLECTION_H_
#define COLLECTION_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

struct collection {
		size_t size;
		size_t count;
		size_t capacity;
		void** data;
		int8_t mc;
		pthread_rwlock_t data_mutex;
};

struct collection* new_collection(size_t capacity, uint8_t mc);

int del_collection(struct collection* coll);

int add_collection(struct collection* coll, void* data);

int rem_collection(struct collection* coll, void* data);

int contains_collection(struct collection* coll, void* data);

void ensure_collection(struct collection* coll, size_t size);

#endif /* COLLECTION_H_ */
