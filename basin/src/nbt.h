/*
 * nbt.h
 *
 *  Created on: Feb 27, 2016
 *      Author: root
 */

#ifndef NBT_H_
#define NBT_H_

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
};

struct nbt_tag {
		unsigned char id;
		char* name;
		size_t children_count;
		struct nbt_tag** children;
		union nbt_data data;
};

void freeNBT(struct nbt_tag* nbt);

struct nbt_tag* cloneNBT(struct nbt_tag* nbt);

int readNBT(struct nbt_tag** root, unsigned char* buffer, size_t buflen);

int writeNBT(struct nbt_tag* root, unsigned char* buffer, size_t buflen);

#endif /* NBT_H_ */
