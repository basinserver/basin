/*
 * network.h
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#ifndef NETWORK_H_
#define NETWORK_H_

#include "accept.h"
#include "inventory.h"
#include <openssl/rsa.h>

struct packet;

struct __attribute__((__packed__)) encpos {
		int32_t z :26;
		int32_t y :12;
		int32_t x :26;
};

struct __attribute__((__packed__)) uuid {
		uint64_t uuid1;
		uint64_t uuid2;
};

struct entity_metadata {
		uint8_t* metadata;
		size_t metadata_size;
};

RSA* public_rsa;
uint8_t* public_rsa_publickey;
SSL_CTX* mojang_ctx;

void init_encryption();

void swapEndian(void* dou, size_t ss);

int getVarIntSize(int32_t input);

int getVarLongSize(int64_t input);

int writeVarInt(int32_t input, unsigned char* buffer);

int writeVarLong(int64_t input, unsigned char* buffer);

int readVarInt(int32_t* output, unsigned char* buffer, size_t buflen);

int readVarLong(int64_t* output, unsigned char* buffer, size_t buflen);

int writeString(char* input, unsigned char* buffer, size_t buflen);

int readString(char** output, unsigned char* buffer, size_t buflen);

int readSlot(struct slot* slot, unsigned char* buffer, size_t buflen);

int writeSlot(struct slot* slot, unsigned char* buffer, size_t buflen);

void duplicateSlot(struct slot* slot, struct slot* dup);

void duplicateNBT(struct nbt_tag* nbt, struct nbt_tag* dup);

ssize_t readPacket(struct conn* conn, unsigned char* buf, size_t buflen, struct packet* packet);

ssize_t writePacket(struct conn* conn, struct packet* packet);

int writeVarInt_stream(int32_t input,
#ifdef __MINGW32__
		SOCKET
#else
		int
#endif
		fd);

int readVarInt_stream(int32_t* output,
#ifdef __MINGW32__
		SOCKET
#else
		int
#endif
		fd);
#endif /* NETWORK_H_ */
