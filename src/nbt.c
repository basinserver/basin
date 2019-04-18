/*
 * nbt.c
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#include <basin/nbt.h>
#include <basin/network.h>
#include <basin/globals.h>
#include <avuna/string.h>
#include <avuna/llist.h>
#include <avuna/log.h>
#include <stdlib.h>
#include <zlib.h>
#include <stdio.h>

struct nbt_tag* nbt_get(struct nbt_tag* nbt, char* name) {
	return hashmap_get(nbt->children, name);
}

struct nbt_tag* nbt_clone(struct mempool* pool, struct nbt_tag* nbt) {
	struct nbt_tag* new_tag = pmalloc(pool, sizeof(struct nbt_tag));
	new_tag->pool = pool;
	new_tag->name = nbt->name == NULL ? NULL : xstrdup(nbt->name, 0);
	new_tag->id = nbt->id;
	new_tag->children = nbt->children == NULL ? NULL : hashmap_new(8, new_tag->pool);
	new_tag->children_list = nbt->children_list == NULL ? NULL : llist_new(new_tag->pool);
	memcpy(&new_tag->data, &nbt->data, sizeof(union nbt_data));
	if (nbt->id == NBT_TAG_BYTEARRAY) {
		new_tag->data.nbt_bytearray.data = pmalloc(new_tag->pool, (size_t) new_tag->data.nbt_bytearray.len);
		memcpy(new_tag->data.nbt_bytearray.data, nbt->data.nbt_bytearray.data, (size_t) new_tag->data.nbt_bytearray.len);
	} else if (nbt->id == NBT_TAG_STRING) {
		new_tag->data.nbt_string = xstrdup(nbt->data.nbt_string, 0);
	} else if (nbt->id == NBT_TAG_INTARRAY) {
		new_tag->data.nbt_intarray.ints = pmalloc(new_tag->pool, new_tag->data.nbt_intarray.count * 4);
		memcpy(new_tag->data.nbt_intarray.ints, nbt->data.nbt_intarray.ints, new_tag->data.nbt_intarray.count * 4);
	} else if (nbt->id == NBT_TAG_LONGARRAY) {
		new_tag->data.nbt_intarray.ints = pmalloc(new_tag->pool, new_tag->data.nbt_intarray.count * 4);
		memcpy(new_tag->data.nbt_intarray.ints, nbt->data.nbt_intarray.ints, new_tag->data.nbt_intarray.count * 4);
	}
	if (nbt->children != NULL) {
		ITER_LLIST(nbt->children_list, value) {
			struct nbt_tag* child = value;
			child = nbt_clone(pool, child);
			llist_append(new_tag->children_list, child);
			if (new_tag->children != NULL) {
				hashmap_put(new_tag->children, child->name, child);
			}
			ITER_LLIST_END();
		}
	}
	return new_tag;
}

ssize_t __recurReadNBT(struct mempool* pool, struct nbt_tag** root, unsigned char* buffer, size_t buflen, int list) {
	struct nbt_tag* cur = pcalloc(pool, sizeof(struct nbt_tag));
	cur->pool = pool;
	size_t r = 0;
	if (list) {
		cur->id = (unsigned char) list;
	} else {
		if (buflen < 1) {
			return 0;
		}
		cur->id = buffer[0];
		buffer++;
		r++;
		buflen--;
		if (cur->id) {
			uint16_t sl;
			if (buflen < 2) {
				return 0;
			}
			memcpy(&sl, buffer, 2);
			swapEndian(&sl, 2);
			buffer += 2;
			r += 2;
			buflen -= 2;
			if (buflen < sl) {
				return 0;
			}
			cur->name = pmalloc(pool, sl + 1);
			cur->name[sl] = 0;
			memcpy(cur->name, buffer, sl);
			buffer += sl;
			r += sl;
			buflen -= sl;
		}
	}
	if (cur->id == NBT_TAG_BYTE) {
		if (buflen < 1) {
			return 0;
		}
		memcpy(&cur->data.nbt_byte, buffer, 1);
		buffer++;
		buflen--;
		r++;
	} else if (cur->id == NBT_TAG_SHORT) {
		if (buflen < 2) {
			return 0;
		}
		memcpy(&cur->data.nbt_short, buffer, 2);
		swapEndian(&cur->data.nbt_short, 2);
		buffer += 2;
		buflen -= 2;
		r += 2;
	} else if (cur->id == NBT_TAG_INT) {
		if (buflen < 4) {
			return 0;
		}
		memcpy(&cur->data.nbt_int, buffer, 4);
		swapEndian(&cur->data.nbt_int, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
	} else if (cur->id == NBT_TAG_LONG) {
		if (buflen < 8) {
			return 0;
		}
		memcpy(&cur->data.nbt_long, buffer, 8);
		swapEndian(&cur->data.nbt_long, 8);
		buffer += 8;
		buflen -= 8;
		r += 8;
	} else if (cur->id == NBT_TAG_FLOAT) {
		if (buflen < 4) {
			return 0;
		}
		memcpy(&cur->data.nbt_float, buffer, 4);
		swapEndian(&cur->data.nbt_float, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
	} else if (cur->id == NBT_TAG_DOUBLE) {
		if (buflen < 8) {
			return 0;
		}
		memcpy(&cur->data.nbt_double, buffer, 8);
		swapEndian(&cur->data.nbt_double, 8);
		buffer += 8;
		buflen -= 8;
		r += 8;
	} else if (cur->id == NBT_TAG_BYTEARRAY) {
		if (buflen < 4) {
			return 0;
		}
		memcpy(&cur->data.nbt_bytearray.len, buffer, 4);
		swapEndian(&cur->data.nbt_bytearray.len, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
		if (buflen < cur->data.nbt_bytearray.len || cur->data.nbt_bytearray.len <= 0) {
			return 0;
		}
		cur->data.nbt_bytearray.data = pmalloc(pool, (size_t) cur->data.nbt_bytearray.len);
		memcpy(cur->data.nbt_bytearray.data, buffer, (size_t) cur->data.nbt_bytearray.len);
		buffer += cur->data.nbt_bytearray.len;
		buflen -= cur->data.nbt_bytearray.len;
		r += cur->data.nbt_bytearray.len;
	} else if (cur->id == NBT_TAG_STRING) {
		uint16_t sl;
		if (buflen < 2) {
			return 0;
		}
		memcpy(&sl, buffer, 2);
		swapEndian(&sl, 2);
		buffer += 2;
		r += 2;
		buflen -= 2;
		if (buflen < sl) {
			return 0;
		}
		cur->data.nbt_string = pmalloc(pool, sl + 1);
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
			ssize_t sub_read = __recurReadNBT(pool, &st, buffer, buflen, lt);
			if (sub_read == 0) continue;
			buffer += sub_read;
			buflen -= sub_read;
			r += sub_read;
			if (!(st != NULL && st->id != NBT_TAG_END && buflen > 0)) {
				st = NULL;
				continue;
			}
			if (cur->children_list == NULL) {
				cur->children_list = llist_new(pool);
			}
			llist_append(cur->children_list, st);
		}
	} else if (cur->id == NBT_TAG_COMPOUND) {
		struct nbt_tag *st = NULL;
		do {
			st = NULL;
			ssize_t sub_read = __recurReadNBT(pool, &st, buffer, buflen, 0);
			buffer += sub_read;
			buflen -= sub_read;
			r += sub_read;
			if (!(st != NULL && st->id != NBT_TAG_END && buflen > 0)) {
				st = NULL;
				continue;
			}
			if (cur->children == NULL) {
				cur->children = hashmap_new(8, pool);
				cur->children_list = llist_new(pool);
			}
			hashmap_put(cur->children, st->name, st);
			llist_append(cur->children_list, st);
		} while (st != NULL && st->id != NBT_TAG_END && buflen > 0);
	} else if (cur->id == NBT_TAG_INTARRAY) {
		if (buflen < 4) {
			return 0;
		}
		memcpy(&cur->data.nbt_intarray.count, buffer, 4);
		swapEndian(&cur->data.nbt_intarray.count, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
		if (buflen < (cur->data.nbt_intarray.count * 4) || cur->data.nbt_intarray.count < 0) {
			return 0;
		}
		if (cur->data.nbt_intarray.count > 0) {
			cur->data.nbt_intarray.ints = pmalloc(pool, cur->data.nbt_intarray.count * 4);
			memcpy(cur->data.nbt_intarray.ints, buffer, cur->data.nbt_intarray.count * 4);
			for (size_t i = 0; i < cur->data.nbt_intarray.count; i++) {
				swapEndian(&cur->data.nbt_intarray.ints[i], 4);
			}
			buffer += cur->data.nbt_intarray.count * 4;
			buflen -= cur->data.nbt_intarray.count * 4;
			r += cur->data.nbt_intarray.count * 4;
		}
	} else if (cur->id == NBT_TAG_LONGARRAY) {
		if (buflen < 4) {
			return 0;
		}
		memcpy(&cur->data.nbt_longarray.count, buffer, 4);
		swapEndian(&cur->data.nbt_longarray.count, 4);
		buffer += 4;
		buflen -= 4;
		r += 4;
		if (buflen < (cur->data.nbt_longarray.count * 8) || cur->data.nbt_longarray.count < 0) {
			return 0;
		}
		if (cur->data.nbt_longarray.count > 0) {
			cur->data.nbt_longarray.longs = pmalloc(pool, cur->data.nbt_longarray.count * 8);
			memcpy(cur->data.nbt_longarray.longs, buffer, cur->data.nbt_longarray.count * 4);
			for (size_t i = 0; i < cur->data.nbt_longarray.count; i++) {
				swapEndian(&cur->data.nbt_longarray.longs[i], 4);
			}
			buffer += cur->data.nbt_longarray.count * 8;
			buflen -= cur->data.nbt_longarray.count * 8;
			r += cur->data.nbt_longarray.count * 8;
		}
	}
	*root = cur;
	return r;
}

#define DECOMPRESS_BUF_SIZE 16384
ssize_t nbt_decompress(struct mempool* pool, void* data, size_t size, void** dest) {
	void* out_buf = pmalloc(pool, DECOMPRESS_BUF_SIZE);
	size_t out_buf_cap = DECOMPRESS_BUF_SIZE;
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	int inflate_result = 0;
	if ((inflate_result = inflateInit2(&strm, (32 + MAX_WBITS))) != Z_OK) {
		errlog(delog, "Compression initialization error!\n");
		return -1;
	}
	strm.avail_in = (uInt) size;
	strm.next_in = data;
	strm.avail_out = (uInt) out_buf_cap;
	strm.next_out = out_buf;
	do {
		if (out_buf_cap - strm.total_out < DECOMPRESS_BUF_SIZE / 2) {
			out_buf_cap *= 2;
			out_buf = realloc(out_buf, out_buf_cap);
		}
		strm.avail_out = out_buf_cap - strm.total_out;
		strm.next_out = out_buf + strm.total_out;
		inflate_result = inflate(&strm, Z_FINISH);
		if (inflate_result == Z_STREAM_ERROR) {
			errlog(delog, "Compression Read Error!\n");
			inflateEnd(&strm);
			return -1;
		}
	} while (strm.avail_out == 0);
	inflateEnd(&strm);
	*dest = out_buf;
	return strm.total_out;
}

ssize_t nbt_read(struct mempool* pool, struct nbt_tag** root, unsigned char* buffer, size_t buflen) {
	if (buflen == 0) return 0;
	struct mempool* sub_pool = mempool_new();
	pchild(pool, sub_pool);
	return __recurReadNBT(sub_pool, root, buffer, buflen, 0);
}

ssize_t __recurWriteNBT(struct nbt_tag* root, unsigned char* buffer, size_t buflen, int list) {
	ssize_t r = 0;
	if (list) {

	} else {
		if (buflen < 1) return 0;
		buffer[0] = root->id;
		r++;
		buffer++;
		buflen--;
		if (root->id > 0) {
			int16_t sl = (int16_t) (root->name == NULL ? 0 : strlen(root->name));
			memcpy(buffer, &sl, 2);
			swapEndian(buffer, 2);
			r += 2;
			buffer += 2;
			buflen -= 2;
			if (buflen < sl) return 0;
			if (root->name != NULL) memcpy(buffer, root->name, (size_t) sl);
			r += sl;
			buffer += sl;
			buflen -= sl;
		}
	}
	if (root->id == NBT_TAG_BYTE) {
		if (buflen < 1) return 0;
		buffer[0] = (unsigned char) root->data.nbt_byte;
		r++;
		buffer++;
		buflen--;
	} else if (root->id == NBT_TAG_SHORT) {
		if (buflen < 2) return 0;
		memcpy(buffer, &root->data.nbt_short, 2);
		swapEndian(buffer, 2);
		r += 2;
		buffer += 2;
		buflen -= 2;
	} else if (root->id == NBT_TAG_INT) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_int, 4);
		swapEndian(buffer, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
	} else if (root->id == NBT_TAG_LONG) {
		if (buflen < 8) return 0;
		memcpy(buffer, &root->data.nbt_short, 8);
		swapEndian(buffer, 8);
		r += 8;
		buffer += 8;
		buflen -= 8;
	} else if (root->id == NBT_TAG_FLOAT) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_float, 4);
		swapEndian(buffer, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
	} else if (root->id == NBT_TAG_DOUBLE) {
		if (buflen < 8) return 0;
		memcpy(buffer, &root->data.nbt_double, 8);
		swapEndian(buffer, 8);
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
		memcpy(buffer, root->data.nbt_bytearray.data, (size_t) root->data.nbt_bytearray.len);
		r += root->data.nbt_bytearray.len;
		buffer += root->data.nbt_bytearray.len;
		buflen -= root->data.nbt_bytearray.len;
	} else if (root->id == NBT_TAG_STRING) {
		if (buflen < 2) return 0;
		uint16_t sl = (uint16_t) strlen(root->data.nbt_string);
		memcpy(buffer, &sl, 2);
		swapEndian(buffer, 2);
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
		memcpy(buffer + 1, &root->data.nbt_list.count, 4);
		swapEndian(buffer + 1, 4);
		r += 5;
		buffer += 5;
		buflen -= 5;
		ITER_LLIST(root->children_list, value) {
			ssize_t sub_read = __recurWriteNBT(value, buffer, buflen, 1);
			r += sub_read;
			buffer += sub_read;
			buflen -= sub_read;
			if (buflen <= 0) goto break_iter_map;
			ITER_LLIST_END();
		}
		break_iter_map:;
	} else if (root->id == NBT_TAG_COMPOUND) {
		ITER_LLIST(root->children_list, value) {
			ssize_t sub_read = __recurWriteNBT(value, buffer, buflen, 0);
			r += sub_read;
			buffer += sub_read;
			buflen -= sub_read;
			if (buflen <= 0) goto break_iter_map2;
			ITER_LLIST_END();
		}
		break_iter_map2:;
		if (buflen <= 1) return 0;
		buffer[0] = 0;
		buffer++;
		buflen--;
		r++;
	} else if (root->id == NBT_TAG_INTARRAY) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_intarray.count, 4);
		swapEndian(buffer, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
		if (buflen < root->data.nbt_intarray.count * 4) return 0;
		memcpy(buffer, root->data.nbt_intarray.ints, root->data.nbt_intarray.count * 4);
		for (size_t i = 0; i < root->data.nbt_intarray.count; ++i) {
			swapEndian(&root->data.nbt_intarray.ints[i], 4);
		}
		r += root->data.nbt_intarray.count * 4;
		buffer += root->data.nbt_intarray.count * 4;
		buflen -= root->data.nbt_intarray.count * 4;
	} else if (root->id == NBT_TAG_LONGARRAY) {
		if (buflen < 4) return 0;
		memcpy(buffer, &root->data.nbt_longarray.count, 4);
		swapEndian(buffer, 4);
		r += 4;
		buffer += 4;
		buflen -= 4;
		if (buflen < root->data.nbt_longarray.count * 8) return 0;
		memcpy(buffer, root->data.nbt_longarray.longs, root->data.nbt_longarray.count * 8);
		for (size_t i = 0; i < root->data.nbt_longarray.count; ++i) {
			swapEndian(&root->data.nbt_longarray.longs[i], 8);
		}
		r += root->data.nbt_longarray.count * 8;
		buffer += root->data.nbt_longarray.count * 8;
		buflen -= root->data.nbt_longarray.count * 8;
	}
	return r;
}

ssize_t nbt_write(struct nbt_tag* root, unsigned char* buffer, size_t buflen) {
	if (buflen == 0) return 0;
	return __recurWriteNBT(root, buffer, buflen, 0);
}
