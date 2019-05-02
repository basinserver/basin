/*
 * nbt.h
 *
 *  Created on: Feb 27, 2016
 *      Author: root
 */

#ifndef BASIN_NBT_H_
#define BASIN_NBT_H_

#include <avuna/hash.h>
#include <avuna/pmem.h>
#include <stdlib.h>
#include <stdint.h>

#define NBT_TAG_END 0
#define NBT_TAG_BYTE 1
#define NBT_TAG_SHORT 2
#define NBT_TAG_INT 3
#define NBT_TAG_LONG 4
#define NBT_TAG_FLOAT 5
#define NBT_TAG_DOUBLE 6
#define NBT_TAG_BYTEARRAY 7
#define NBT_TAG_STRING 8
#define NBT_TAG_LIST 9
#define NBT_TAG_COMPOUND 10
#define NBT_TAG_INTARRAY 11
#define NBT_TAG_LONGARRAY 12

union nbt_data {
    signed char nbt_byte;
    int16_t nbt_short;
    int32_t nbt_int;
    int64_t nbt_long;
    float nbt_float;
    double nbt_double;
    struct {
        int32_t len;
        unsigned char* data;
    } nbt_bytearray;
    char* nbt_string;
    struct {
        unsigned char type;
        int32_t count;
    } nbt_list;
    struct {
        int32_t count;
        int32_t* ints;
    } nbt_intarray;
    struct {
        int32_t count;
        int64_t* longs;
    } nbt_longarray;
};

struct nbt_tag {
    unsigned char id;
    char* name;
    struct hashmap* children;
    struct llist* children_list;
    union nbt_data data;
    struct mempool* pool;
};

ssize_t nbt_decompress(struct mempool* pool, void* data, size_t size, void** dest);

struct nbt_tag* nbt_clone(struct mempool* pool, struct nbt_tag* nbt);

struct nbt_tag* nbt_get(struct nbt_tag* nbt, char* name);

ssize_t nbt_read(struct mempool* pool, struct nbt_tag** root, unsigned char* buffer, size_t buflen);

ssize_t nbt_write(struct nbt_tag* root, unsigned char* buffer, size_t buflen);

struct nbt_tag* nbt_new(struct mempool* pool, uint8_t type);

void nbt_put(struct nbt_tag* parent, struct nbt_tag* child);

#endif /* BASIN_NBT_H_ */
