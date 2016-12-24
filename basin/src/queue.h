/*
 * queue.h
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h>

struct queue {
		size_t size;
		size_t capacity;
		size_t start;
		size_t end;
		size_t rc;
		void** data;
		pthread_mutex_t data_mutex;
		pthread_cond_t in_cond;
		pthread_cond_t out_cond;
		int mt;
};

struct queue* new_queue(size_t capacity, int mt);

int del_queue(struct queue* queue);

int add_queue(struct queue* queue, void* data);

void* pop_queue(struct queue* queue);

void* pop_nowait_queue(struct queue* queue);

void* timedpop_queue(struct queue* queue, struct timespec* abstime);

#endif /* QUEUE_H_ */
