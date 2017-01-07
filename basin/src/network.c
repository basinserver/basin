/*
 * network.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "nbt.h"
#include <zlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "accept.h"
#include "packet.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>
#include "network.h"
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <openssl/aes.h>
#include <openssl/evp.h>

void init_encryption() {
	public_rsa = RSA_new();
	BIGNUM* pn = BN_new();
	if (BN_set_word(pn, RSA_F4) != 1) {
		printf("Error generating RSA BIGNUM!\n");
		return;
	}
	if (RSA_generate_key_ex(public_rsa, 1024, pn, NULL) != 1) {
		printf("Error generating RSA keypair!\n");
		return;
	}
	size_t s = i2d_RSA_PUBKEY(public_rsa, &public_rsa_publickey);
	if (s != 162) {
		printf("[WARNING] RSA Public Key size was not 162! Size = %lu\n", s);
	}
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_library_init();
	EVP_add_cipher (EVP_aes_128_cfb8());;
	const SSL_METHOD* method = SSLv23_client_method();
	mojang_ctx = SSL_CTX_new(method);
}

void swapEndian(void* dou, size_t ss) {
	uint8_t* pxs = (uint8_t*) dou;
	for (int i = 0; i < ss / 2; i++) {
		uint8_t tmp = pxs[i];
		pxs[i] = pxs[ss - 1 - i];
		pxs[ss - 1 - i] = tmp;
	}
}

int getVarIntSize(int32_t input) {
	for (int32_t x = 1; x < 5; x++) {
		if ((input & -1 << x * 7) == 0) return x;
	}
	return 5;
}

int getVarLongSize(int64_t input) {
	for (unsigned char x = 1; x < 10; ++x)
		if (((input & -1) << (x * 7)) == 0) return x;
	return 10;
}

int writeVarInt(int32_t input, unsigned char* buffer) {
	int i = 0;
	while ((input & -128) != 0) {
		buffer[i++] = (input & 127) | 128;
		input >>= 7;
	}
	buffer[i++] = input;
	return i;
}

int writeVarLong(int64_t input, unsigned char* buffer) {
	int i = 0;
	while ((input & -128) != 0) {
		buffer[i++] = (input & 127) | 128;
		input >>= 7;
	}
	buffer[i++] = input;
	return i;
}

int readVarInt(int32_t* output, unsigned char* buffer, size_t buflen) {
	*output = 0;
	int v2 = 0;
	signed char v3;
	do {
		if (v2 >= buflen) return 0;
		v3 = buffer[v2];
		*output |= (v3 & 127) << (v2++ * 7);
		if (v2 > 5) return v2;
	} while ((v3 & 128) == 128);
	return v2;
}

int readVarLong(int64_t* output, unsigned char* buffer, size_t buflen) {
	*output = 0;
	int v2 = 0;
	signed char v3;
	do {
		if (v2 >= buflen) return 0;
		v3 = buffer[v2];
		*output |= (v3 & 127) << (v2++ * 7);
		if (v2 > 10) return v2;
	} while ((v3 & 128) == 128);
	return v2;
}

int writeString(char* input, unsigned char* buffer, size_t buflen) {
	if (buflen < 4) return 0;
	ssize_t sl = strlen(input);
	if (sl - 4 > (ssize_t) buflen) {
		sl = buflen - 4;
	}
	int x = writeVarInt(sl, buffer);
	buflen -= x;
	buffer += x;
	memcpy(buffer, input, sl);
	return sl + x;
}

int readString(char** output, unsigned char* buffer, size_t buflen) {
	if (buflen < 1) {
		*output = malloc(1);
		(*output)[0] = 0;
		return 0;
	}
	int32_t sl;
	int x = readVarInt(&sl, buffer, buflen);
	if (x == -1) {
		*output = malloc(1);
		(*output)[0] = 0;
		return 0;
	}
	if (sl > 32767) {
		*output = malloc(1);
		(*output)[0] = 0;
		return 0;
	}
	buflen -= x;
	buffer += x;
	if (buflen < sl) {
		*output = malloc(1);
		(*output)[0] = 0;
		return 0;
	}
	*output = malloc(sl + 1);
	memcpy(*output, buffer, sl);
	(*output)[sl] = 0;
	return x + sl; // silently ignores characters past the outlen
}

int writeVarInt_stream(int32_t input,
#ifdef __MINGW32__
		SOCKET
#else
		int
#endif
		fd) {
	int i = 0;
	unsigned char n = 0;
	while ((input & -128) != 0) {
		n = (input & 127) | 128;
#ifdef __MINGW32__
		if (send(fd, &n, 1, 0) != 1) {
#else
		if (write(fd, &n, 1) != 1) {
#endif
			return -1;
		}
		input >>= 7;
	}
#ifdef __MINGW32__
	if (send(fd, &input, 1, 0) != 1) return -1;
#else
	if (write(fd, &input, 1) != 1) return -1;
#endif
	return i;
}

int readVarInt_stream(int32_t* output,
#ifdef __MINGW32__
		SOCKET
#else
		int
#endif
		fd) {
	*output = 0;
	int v2 = 0;
	signed char v3;
	do {
#ifdef __MINGW32__
		if (recv(fd, &v3, 1, 0) != 1) {
#else
		if (read(fd, &v3, 1) != 1) {
#endif
			return v2;
		}
		*output |= (v3 & 127) << (v2++ * 7);
		if (v2 > 5) return v2;
	} while ((v3 & 128) == 128);
	return v2;
}

int readSlot(struct slot* slot, unsigned char* buffer, size_t buflen) {
	if (buflen < 2) return -1;
	memcpy(&slot->item, buffer, 2);
	swapEndian(&slot->item, 2);
	if (slot->item == -1) {
		slot->damage = 0;
		slot->itemCount = 0;
		slot->nbt = malloc(sizeof(struct nbt_tag));
		slot->nbt->id = NBT_TAG_END;
		slot->nbt->name = NULL;
		slot->nbt->children = NULL;
		slot->nbt->children_count = 0;
		return 2;
	}
	buffer += 2;
	buflen -= 2;
	if (buflen < 4) return -1;
	slot->itemCount = *buffer;
	buffer++;
	buflen--;
	memcpy(&slot->damage, buffer, 2);
	swapEndian(&slot->damage, 2);
	buffer += 2;
	buflen -= 2;
	return 5 + readNBT(&slot->nbt, buffer, buflen);
}

void duplicateSlot(struct slot* slot, struct slot* dup) {
	if (slot == NULL) {
		memset(dup, 0, sizeof(struct slot));
		dup->item = -1;
		return;
	}
	dup->item = slot->item;
	dup->damage = slot->damage;
	dup->itemCount = slot->itemCount;
	dup->nbt = xmalloc(sizeof(struct nbt_tag));
	duplicateNBT(slot->nbt, dup->nbt);
}

void duplicateNBT(struct nbt_tag* nbt, struct nbt_tag* dup) {
	if (nbt == NULL) {
		memset(dup, 0, sizeof(struct nbt_tag));
		return;
	}
	dup->name = nbt->name == NULL ? NULL : xstrdup(nbt->name, 0);
	dup->id = nbt->id;
	dup->children_count = nbt->children_count;
	if (dup->id == NBT_TAG_BYTE || dup->id == NBT_TAG_SHORT || dup->id == NBT_TAG_INT || dup->id == NBT_TAG_LONG || dup->id == NBT_TAG_FLOAT || dup->id == NBT_TAG_DOUBLE) {
		memcpy(&dup->data, &nbt->data, sizeof(union nbt_data));
	} else if (dup->id == NBT_TAG_BYTEARRAY) {
		dup->data.nbt_bytearray.len = nbt->data.nbt_bytearray.len;
		dup->data.nbt_bytearray.data = xmalloc(dup->data.nbt_bytearray.len);
		memcpy(dup->data.nbt_bytearray.data, nbt->data.nbt_bytearray.data, dup->data.nbt_bytearray.len);
	} else if (dup->id == NBT_TAG_STRING) {
		dup->data.nbt_string = xstrdup(nbt->data.nbt_string, 0);
	} else if (dup->id == NBT_TAG_LIST) {
		dup->data.nbt_list.type = nbt->data.nbt_list.type;
		dup->data.nbt_list.count = nbt->data.nbt_list.count;
	} else if (dup->id == NBT_TAG_INTARRAY) {
		dup->data.nbt_intarray.count = nbt->data.nbt_intarray.count;
		dup->data.nbt_intarray.ints = xmalloc(4 * dup->data.nbt_intarray.count);
		memcpy(dup->data.nbt_intarray.ints, nbt->data.nbt_intarray.ints, dup->data.nbt_bytearray.len);
	}
	dup->children = dup->children_count == 0 ? NULL : xmalloc(sizeof(struct nbt_tag*) * dup->children_count);
	for (size_t i = 0; i < dup->children_count; i++) {
		dup->children[i] = xmalloc(sizeof(struct nbt_tag));
		duplicateNBT(nbt->children[i], dup->children[i]);
	}
}

int writeSlot(struct slot* slot, unsigned char* buffer, size_t buflen) {
	if (buflen < 2) return -1;
	memcpy(buffer, &slot->item, 2);
	swapEndian(buffer, 2);
	buffer += 2;
	buflen -= 2;
	if (slot->item < 0) return 2;
	if (buflen < 3) return -1;
	memcpy(buffer, &slot->itemCount, 1);
	buffer++;
	buflen--;
	memcpy(buffer, &slot->damage, 2);
	swapEndian(buffer, 2);
	buffer += 2;
	buflen -= 2;
	return 5 + writeNBT(slot->nbt, buffer, buflen);
}
