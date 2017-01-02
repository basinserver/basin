/*
 * hashmap.c
 *
 *  Created on: Nov 19, 2015
 *      Author: root
 */

#include "hashmap.h"
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "xstring.h"
#include "util.h"
#include <math.h>

struct hashmap* new_hashmap(uint8_t bucket_count_bytes, uint8_t mc) {
	if (bucket_count_bytes > 2 || bucket_count_bytes < 1) return NULL;
	struct hashmap* map = xmalloc(sizeof(struct hashmap));
	map->bucket_count = (size_t) pow(2, 8 * bucket_count_bytes);
	map->buckets = xcalloc(map->bucket_count * sizeof(struct hashmap_entry*));
	map->entry_count = 0;
	map->mc = mc;
	if (mc && pthread_rwlock_init(&map->data_mutex, NULL)) {
		xfree(map->buckets);
		map->buckets = NULL;
		xfree(map);
		return NULL;
	}
	return map;
}

#ifdef MEM_LEAK_DEBUG
struct hashmap* new_hashmap_memsafe(uint8_t bucket_count_bytes, uint8_t mc) {
	if (bucket_count_bytes > 2 || bucket_count_bytes < 1) return NULL;
	struct hashmap* map = xmalloc_real(sizeof(struct hashmap), NULL, 0);
	map->bucket_count = (size_t) pow(2, 8 * bucket_count_bytes);
	map->buckets = xcalloc_real(map->bucket_count * sizeof(struct hashmap_entry*), NULL, 0);
	map->entry_count = 0;
	map->mc = mc;
	if (mc && pthread_rwlock_init(&map->data_mutex, NULL)) {
		xfree(map->buckets);
		map->buckets = NULL;
		xfree(map);
		return NULL;
	}
	return map;
}
#endif

int del_hashmap(struct hashmap* map) {
	if (map == NULL || map->buckets == NULL) return -1;
	if (map->mc && pthread_rwlock_destroy(&map->data_mutex)) return -1;
	for (size_t i = 0; i < map->bucket_count; i++) {
		struct hashmap_entry* head = map->buckets[i];
		while (1) {
			if (head == NULL) break;
			struct hashmap_entry* next = head->next;
			xfree(head);
			head = next;
		}
	}
	xfree(map->buckets);
	map->buckets = NULL;
	xfree(map);
	return 0;
}

void put_hashmap(struct hashmap* map, uint64_t key, void* value) {
	if (map->mc) pthread_rwlock_wrlock(&map->data_mutex);
	uint8_t* hashi = (uint8_t*) &key;
	for (int i = 1; i < sizeof(uint64_t); i++) {
		hashi[0] ^= hashi[i];
		if (i > 1 && map->bucket_count == sizeof(uint16_t)) hashi[1] ^= hashi[i];
	}
	if (map->bucket_count == 65536) hashi[1] ^= hashi[0];
	uint16_t fhash = hashi[0];
	//printf("%i\n", fhash);
	if (map->bucket_count == 65536) fhash |= (hashi[1] << 8);
	struct hashmap_entry* prior = map->buckets[fhash];
	struct hashmap_entry* entry = map->buckets[fhash];
	while (entry != NULL && entry->key != key) {
		prior = entry;
		entry = entry->next;
	}
	if (entry == NULL) {
		if (value != NULL) {
			map->entry_count++;
			entry = xmalloc(sizeof(struct hashmap_entry));
			entry->key = key;
			entry->value = value;
			entry->next = NULL;
			if (prior != NULL && prior != entry) {
				prior->next = entry;
			} else {
				map->buckets[fhash] = entry;
			}
		}
	} else {
		if (value == NULL) {
			if (prior != entry && prior != NULL) {
				prior->next = entry->next;
			} else {
				map->buckets[fhash] = entry->next;
			}
			xfree(entry);
			map->entry_count--;
		} else {
			entry->value = value;
		}
	}
	if (map->mc) pthread_rwlock_unlock(&map->data_mutex);
}

#ifdef MEM_LEAK_DEBUG
void put_hashmap_memsafe(struct hashmap* map, uint64_t key, void* value) {
	if (map->mc) pthread_rwlock_wrlock(&map->data_mutex);
	uint8_t* hashi = (uint8_t*) &key;
	for (int i = 1; i < sizeof(uint64_t); i++) {
		hashi[0] ^= hashi[i];
		if (i > 1 && map->bucket_count == sizeof(uint16_t)) hashi[1] ^= hashi[i];
	}
	if (map->bucket_count == 65536) hashi[1] ^= hashi[0];
	uint16_t fhash = hashi[0];
	//printf("%i\n", fhash);
	if (map->bucket_count == 65536) fhash |= (hashi[1] << 8);
	struct hashmap_entry* prior = map->buckets[fhash];
	struct hashmap_entry* entry = map->buckets[fhash];
	while (entry != NULL && entry->key != key) {
		prior = entry;
		entry = entry->next;
	}
	if (entry == NULL) {
		if (value != NULL) {
			map->entry_count++;
			entry = xmalloc_real(sizeof(struct hashmap_entry), NULL, 0);
			entry->key = key;
			entry->value = value;
			entry->next = NULL;
			if (prior != NULL && prior != entry) {
				prior->next = entry;
			} else {
				map->buckets[fhash] = entry;
			}
		}
	} else {
		if (value == NULL) {
			if (prior != entry && prior != NULL) {
				prior->next = entry->next;
			} else {
				map->buckets[fhash] = entry->next;
			}
			xfree_real(entry, NULL, 0);
			map->entry_count--;
		} else {
			entry->value = value;
		}
	}
	if (map->mc) pthread_rwlock_unlock(&map->data_mutex);
}
#endif

void* get_hashmap(struct hashmap* map, uint64_t key) {
	if (map->mc) pthread_rwlock_rdlock(&map->data_mutex);
	uint8_t* hashi = (uint8_t*) &key;
	for (int i = 1; i < sizeof(uint64_t); i++) {
		hashi[0] ^= hashi[i];
		if (i > 1 && map->bucket_count == 65536) hashi[1] ^= hashi[i];
	}
	if (map->bucket_count == 65536) hashi[1] ^= hashi[0];
	uint16_t fhash = hashi[0];
	if (map->bucket_count == 65536) fhash |= (hashi[1] << 8);
	struct hashmap_entry* entry = map->buckets[fhash];
	while (entry != NULL && entry->key != key) {
		entry = entry->next;
	}
	void* value = NULL;
	if (entry != NULL) {
		value = entry->value;
	}
	if (map->mc) pthread_rwlock_unlock(&map->data_mutex);
	return value;
}

int contains_hashmap(struct hashmap* map, uint64_t key) {
	return get_hashmap(map, key) != NULL;
}
