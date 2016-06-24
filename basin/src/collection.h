/*
 * queue.h
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#ifndef COLLECTION_H_
#define COLLECTION_H_

#include <pthread.h>

struct collection {
		size_t size;
		size_t count;
		size_t capacity;
		size_t rc;
		void** data;
		pthread_rwlock_t data_mutex;
};

struct collection* new_collection(size_t capacity);

int del_collection(struct collection* coll);

int add_collection(struct collection* coll, void* data);

int rem_collection(struct collection* coll, void* data);

#endif /* COLLECTION_H_ */
