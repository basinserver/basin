/*
 * nbt.c
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#include "nbt.h"
#include <stdlib.h>
#include <string.h>

void freeNBT(struct nbt_tag* nbt) {
	if (nbt->name != NULL) free(nbt->name);
	if (nbt->children != NULL) {
		for (size_t i = 0; i < nbt->children_count; i++) {
			freeNBT(nbt->children[i]);
			free(nbt->children[i]);
		}
		free(nbt->children);
	}
	if (nbt->id == NBT_TAG_BYTEARRAY) {
		free(nbt->data.nbt_bytearray.data);
	} else if (nbt->id == NBT_TAG_STRING) {
		free(nbt->data.nbt_string);
	} else if (nbt->id == NBT_TAG_INTARRAY) {
		free(nbt->data.nbt_intarray.ints);
	}
}

struct nbt_tag* cloneNBT(struct nbt_tag* nbt) {
	struct nbt_tag* nt = malloc(sizeof(struct nbt_tag));
	nt->name = nbt->name == NULL ? NULL : strdup(nbt->name);
	nt->id = nbt->id;
	nt->children_count = nbt->children_count;
	nt->children = nt->children_count == 0 ? NULL : malloc(sizeof(struct nbt_tag*) * nt->children_count);
	memcpy(&nt->data, &nbt->data, sizeof(union nbt_data));
	if (nbt->id == NBT_TAG_BYTEARRAY) {
		nt->data.nbt_bytearray.data = malloc(nt->data.nbt_bytearray.len);
		memcpy(nt->data.nbt_bytearray.data, nbt->data.nbt_bytearray.data, nt->data.nbt_bytearray.len);
	} else if (nbt->id == NBT_TAG_STRING) {
		nt->data.nbt_string = strdup(nbt->data.nbt_string);
	} else if (nbt->id == NBT_TAG_INTARRAY) {
		nt->data.nbt_intarray.ints = malloc(nt->data.nbt_intarray.count * 4);
		memcpy(nt->data.nbt_intarray.ints, nbt->data.nbt_intarray.ints, nt->data.nbt_intarray.count * 4);
	}
	for (size_t i = 0; i < nt->children_count; i++) {
		nt->children[i] = cloneNBT(nbt->children[i]);
	}
	return nt;
}

int __recurReadNBT(struct nbt_tag** root, unsigned char* buffer, size_t buflen, int list) {
	struct nbt_tag* cur = malloc(sizeof(struct nbt_tag));
	cur->name = NULL;
	cur->children = NULL;
	cur->children_count = 0;
	size_t r = 0;
	if (list) {
		cur->id = list;
	} else {
		if (buflen < 1) {
			free(cur);
			return 0;
		}
		cur->id = buffer[0];
		buffer++;
		r++;
		buflen--;
		if (cur->id > 0) {
			uint16_t sl;
			if (buflen < 2) {
				free(cur);
				return 0;
			}
			memcpy(&sl, buffer, 2);
			swapEndian(&sl, 2);
			buffer += 2;
			r += 2;
			buflen -= 2;
			if (buflen < sl) {
				free(cur);
				return 0;
			}
			cur->name = malloc(sl + 1);
			cur->name[sl] = 0;
			memcpy(cur->name, buffer, sl);
			buffer += sl;
			r += sl;
			buflen -= sl;
		}
	}
	if (cur->id == NBT_TAG_BYTE) {
		if (buflen < 1) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_byte, buffer, 1);
		buffer++;
		buflen--;
		r++;
	} else if (cur->id == NBT_TAG_SHORT) {
		if (buflen < 2) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_short, buffer, 2);
		swapEndian(&cur->data.nbt_short, 2);
		buffer += 2;
		buflen -= 2;
		r += 2;
	} else if (cur->id == NBT_TAG_INT) {
		if (buflen < 4) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_int, buffer, 4);
		swapEndian(&cur->data.nbt_int, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
	} else if (cur->id == NBT_TAG_LONG) {
		if (buflen < 8) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_long, buffer, 8);
		swapEndian(&cur->data.nbt_long, 8);
		buffer += 8;
		buflen -= 8;
		r += 8;
	} else if (cur->id == NBT_TAG_FLOAT) {
		if (buflen < 4) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_float, buffer, 4);
		swapEndian(&cur->data.nbt_float, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
	} else if (cur->id == NBT_TAG_DOUBLE) {
		if (buflen < 8) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_double, buffer, 8);
		swapEndian(&cur->data.nbt_double, 8);
		buffer += 8;
		buflen -= 8;
		r += 8;
	} else if (cur->id == NBT_TAG_BYTEARRAY) {
		if (buflen < 4) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_bytearray.len, buffer, 4);
		swapEndian(&cur->data.nbt_bytearray.len, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
		if (buflen < cur->data.nbt_bytearray.len || cur->data.nbt_bytearray.len <= 0) {
			free(cur->name);
			free(cur);
			return 0;
		}
		cur->data.nbt_bytearray.data = malloc(cur->data.nbt_bytearray.len);
		memcpy(cur->data.nbt_bytearray.data, buffer, cur->data.nbt_bytearray.len);
		buffer += cur->data.nbt_bytearray.len;
		buflen -= cur->data.nbt_bytearray.len;
		r += cur->data.nbt_bytearray.len;
	} else if (cur->id == NBT_TAG_STRING) {
		uint16_t sl;
		if (buflen < 2) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&sl, buffer, 2);
		swapEndian(&sl, 2);
		buffer += 2;
		r += 2;
		buflen -= 2;
		if (buflen < sl) {
			free(cur->name);
			free(cur);
			return 0;
		}
		cur->data.nbt_string = malloc(sl + 1);
		cur->data.nbt_string[sl] = 0;
		memcpy(cur->data.nbt_string, buffer, sl);
		buffer += sl;
		r += sl;
		buflen -= sl;
	} else if (cur->id == NBT_TAG_LIST) {
		if (buflen < 5) {
			free(cur->name);
			free(cur);
			return 0;
		}
		unsigned char lt = 0;
		int32_t count;
		memcpy(&lt, buffer, 1);
		memcpy(&count, buffer + 1, 4);
		swapEndian(&count, 4);
		buffer += 5;
		buflen -= 5;
		r += 5;
		struct nbt_tag *st = NULL;
		for (size_t i = 0; i < count; i++) {
			st = NULL;
			int nr = __recurReadNBT(&st, buffer, buflen, lt);
			if (nr == 0) continue;
			buffer += nr;
			buflen -= nr;
			r += nr;
			if (!(st != NULL && st->id != NBT_TAG_END && buflen > 0)) continue;
			if (cur->children == NULL) {
				cur->children = malloc(sizeof(struct nbt_tag*));
				cur->children_count = 0;
			} else {
				cur->children = realloc(cur->children, sizeof(struct nbt_tag*) * (cur->children_count + 1));
			}
			cur->children[cur->children_count++] = st;
		}
	} else if (cur->id == NBT_TAG_COMPOUND) {
		struct nbt_tag *st = NULL;
		do {
			st = NULL;
			int nr = __recurReadNBT(&st, buffer, buflen, 0);
			buffer += nr;
			buflen -= nr;
			r += nr;
			if (!(st != NULL && st->id != NBT_TAG_END && buflen > 0)) continue;
			if (cur->children == NULL) {
				cur->children = malloc(sizeof(struct nbt_tag*));
				cur->children_count = 0;
			} else {
				cur->children = realloc(cur->children, sizeof(struct nbt_tag*) * (cur->children_count + 1));
			}
			cur->children[cur->children_count++] = st;
		} while (st != NULL && st->id != NBT_TAG_END && buflen > 0);
	} else if (cur->id == NBT_TAG_INTARRAY) {
		if (buflen < 4) {
			free(cur->name);
			free(cur);
			return 0;
		}
		memcpy(&cur->data.nbt_intarray.count, buffer, 4);
		swapEndian(&cur->data.nbt_intarray.count, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
		if (buflen < (cur->data.nbt_intarray.count * 4) || cur->data.nbt_intarray.count <= 0) {
			free(cur->name);
			free(cur);
			return 0;
		}
		cur->data.nbt_intarray.ints = malloc(cur->data.nbt_intarray.count * 4);
		memcpy(cur->data.nbt_intarray.ints, buffer, cur->data.nbt_intarray.count * 4);
		buffer += cur->data.nbt_intarray.count * 4;
		buflen -= cur->data.nbt_intarray.count * 4;
		r += cur->data.nbt_intarray.count * 4;
	}
	*root = cur;
	return r;
}

int readNBT(struct nbt_tag** root, unsigned char* buffer, size_t buflen) {
	if (buflen == 0) return 0;
	return __recurReadNBT(root, buffer, buflen, 0);
}

int __recurWriteNBT(struct nbt_tag* root, unsigned char* buffer, size_t buflen, int list) {
	size_t r = 0;
	if (list) {

	} else {
		if (buflen < 1) return 0;
		buffer[0] = root->id;
		r++;
		buffer++;
		buflen--;
		if (root->id > 0) {
			int16_t sl = strlen(root->name);
			memcpy(buffer, &sl, 2);
			r += 2;
			buffer += 2;
			buflen -= 2;
			if (buflen < sl) return 0;
			memcpy(buffer, root->name, sl);
			r += sl;
			buffer += sl;
			buflen -= sl;
		}
	}
	if (root->id == NBT_TAG_BYTE) {
		if (buflen < 1) return 0;
		buffer[0] = root->data.nbt_byte;
		r++;
		buffer++;
		buflen--;
	} else if (root->id == NBT_TAG_SHORT) {
		if (buflen < 2) return 0;
		memcpy(buffer, &root->data.nbt_short, 2);
		r += 2;
		buffer += 2;
		buflen -= 2;
	} else if (root->id == NBT_TAG_INT) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_int, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
	} else if (root->id == NBT_TAG_LONG) {
		if (buflen < 8) return 0;
		memcpy(buffer, &root->data.nbt_short, 8);
		r += 8;
		buffer += 8;
		buflen -= 8;
	} else if (root->id == NBT_TAG_FLOAT) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_float, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
	} else if (root->id == NBT_TAG_DOUBLE) {
		if (buflen < 8) return 0;
		memcpy(buffer, &root->data.nbt_double, 8);
		r += 8;
		buffer += 8;
		buflen -= 8;
	} else if (root->id == NBT_TAG_BYTEARRAY) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_bytearray.len, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
		if (buflen < root->data.nbt_bytearray.len) return 0;
		memcpy(buffer, root->data.nbt_bytearray.data, root->data.nbt_bytearray.len);
		r += root->data.nbt_bytearray.len;
		buffer += root->data.nbt_bytearray.len;
		buflen -= root->data.nbt_bytearray.len;
	} else if (root->id == NBT_TAG_STRING) {
		if (buflen < 2) return 0;
		uint16_t sl = strlen(root->data.nbt_string);
		memcpy(buffer, &sl, 2);
		r += 2;
		buffer += 2;
		buflen -= 2;
		if (buflen < sl) return 0;
		memcpy(buffer, root->data.nbt_string, sl);
		r += sl;
		buffer += sl;
		buflen -= sl;
	} else if (root->id == NBT_TAG_LIST) {
		if (buflen < 5) return 0;
		buffer[0] = root->data.nbt_list.type;
		memcpy(buffer, &root->data.nbt_list.count, 4);
		r += 5;
		buffer += 5;
		buflen -= 5;
		for (size_t i = 0; i < root->children_count; i++) {
			int x = __recurWriteNBT(root->children[i], buffer, buflen, 1);
			r += x;
			buffer += x;
			buflen -= x;
			if (buflen <= 0) break;
		}
	} else if (root->id == NBT_TAG_COMPOUND) {
		for (size_t i = 0; i < root->children_count; i++) {
			int x = __recurWriteNBT(root->children[i], buffer, buflen, 1);
			r += x;
			buffer += x;
			buflen -= x;
			if (buflen <= 0 || root->children[i]->id == NBT_TAG_END) break;
		}
	} else if (root->id == NBT_TAG_INTARRAY) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_intarray.count, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
		if (buflen < root->data.nbt_intarray.count * 4) return 0;
		memcpy(buffer, root->data.nbt_intarray.ints, root->data.nbt_intarray.count * 4);
		r += root->data.nbt_intarray.count * 4;
		buffer += root->data.nbt_intarray.count * 4;
		buflen -= root->data.nbt_intarray.count * 4;
	}
	return r;
}

int writeNBT(struct nbt_tag* root, unsigned char* buffer, size_t buflen) {
	if (buflen == 0) return 0;
	return __recurWriteNBT(root, buffer, buflen, 0);
}
