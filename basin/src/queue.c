/*
 * queue.c
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#include "queue.h"
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "xstring.h"
#include "util.h"

struct queue* new_queue(size_t capacity, size_t data_size) {
	struct queue* queue = xmalloc(sizeof(struct queue));
	queue->capacity = capacity;
	queue->data = xmalloc((capacity == 0 ? 1 : capacity) * data_size);
	queue->rc = capacity == 0 ? 1 : 0;
	queue->dsize = data_size;
	queue->start = 0;
	queue->end = 0;
	queue->size = 0;
	if (pthread_mutex_init(&queue->data_mutex, NULL)) {
		xfree(queue->data);
		queue->data = NULL;
		xfree(queue);
		return NULL;
	}
	if (pthread_mutex_init(&queue->out_mutex, NULL)) {
		xfree(queue->data);
		queue->data = NULL;
		xfree(queue);
		pthread_mutex_destroy(&queue->data_mutex);
		return NULL;
	}
	if (pthread_mutex_init(&queue->in_mutex, NULL)) {
		xfree(queue->data);
		queue->data = NULL;
		xfree(queue);
		pthread_mutex_destroy(&queue->data_mutex);
		pthread_mutex_destroy(&queue->out_mutex);
		return NULL;
	}
	if (pthread_cond_init(&queue->out_cond, NULL)) {
		xfree(queue->data);
		queue->data = NULL;
		xfree(queue);
		pthread_mutex_destroy(&queue->data_mutex);
		pthread_mutex_destroy(&queue->out_mutex);
		pthread_mutex_destroy(&queue->in_mutex);
		return NULL;
	}
	if (pthread_cond_init(&queue->in_cond, NULL)) {
		xfree(queue->data);
		queue->data = NULL;
		xfree(queue);
		pthread_mutex_destroy(&queue->data_mutex);
		pthread_mutex_destroy(&queue->out_mutex);
		pthread_mutex_destroy(&queue->in_mutex);
		pthread_cond_destroy(&queue->out_cond);
		return NULL;
	}
	return queue;
}

int del_queue(struct queue* queue) {
	if (queue == NULL || queue->data == NULL) return -1;
	if (pthread_mutex_destroy(&queue->data_mutex)) return -1;
	if (pthread_mutex_destroy(&queue->out_mutex)) return -1;
	if (pthread_cond_destroy(&queue->out_cond)) return -1;
	if (pthread_mutex_destroy(&queue->in_mutex)) return -1;
	if (pthread_cond_destroy(&queue->in_cond)) return -1;
	xfree(queue->data);
	queue->data = NULL;
	xfree(queue);
	return 0;
}

int add_queue(struct queue* queue, void* data) {
	pthread_mutex_lock(&queue->data_mutex);
	if (queue->size == queue->rc && queue->capacity == 0) {
		queue->rc++;
		queue->data = xrealloc(queue->data, queue->rc * queue->dsize);
		pthread_mutex_unlock(&queue->data_mutex);
	} else {
		pthread_mutex_unlock(&queue->data_mutex);
		pthread_mutex_lock(&queue->in_mutex);
		while (queue->size == queue->capacity) {
			pthread_cond_wait(&queue->in_cond, &queue->in_mutex);
		}
		pthread_mutex_unlock(&queue->in_mutex);
	}
	pthread_mutex_lock(&queue->data_mutex);
	void* nl = queue->data;
	int ix = queue->end;
	nl += ix * queue->dsize;
	memcpy(nl, data, queue->dsize);
	queue->end++;
	if (queue->end >= queue->capacity) {
		queue->end -= queue->capacity;
	}
	queue->size++;
	pthread_mutex_unlock(&queue->data_mutex);
	pthread_cond_signal(&queue->out_cond);
	return 0;
}

int pop_queue(struct queue* queue, void* data) {
	pthread_mutex_lock(&queue->out_mutex);
	while (queue->size == 0) {
		pthread_cond_wait(&queue->out_cond, &queue->out_mutex);
	}
	pthread_mutex_unlock(&queue->out_mutex);
	pthread_mutex_lock(&queue->data_mutex);
	void* nl = queue->data;
	nl += queue->start * queue->dsize;
	memcpy(data, nl, queue->dsize);
	queue->start++;
	if (queue->start >= queue->capacity) {
		queue->start -= queue->capacity;
	}
	queue->size--;
	pthread_mutex_unlock(&queue->data_mutex);
	pthread_cond_signal(&queue->in_cond);
	return 0;
}
