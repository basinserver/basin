/*
 * accept.h
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */

#ifndef ACCEPT_H_
#define ACCEPT_H_

#include "config.h"
#include "collection.h"
#include <sys/socket.h>
#include "work.h"
#include <netinet/ip6.h>
#include <openssl/evp.h>
#include <openssl/aes.h>

struct accept_param {
		int server_fd;
		int port;
		struct cnode* config;
		int works_count;
		struct work_param** works;
		struct logsess* logsess;
};

struct conn {
		int fd;
		int state;
		struct sockaddr_in6 addr;
		socklen_t addrlen;
		unsigned char* readBuffer;
		unsigned char* readDecBuffer;
		size_t readDecBuffer_size;
		size_t readBuffer_size;
		unsigned char* writeBuffer;
		size_t writeBuffer_size;
		size_t writeBuffer_capacity;
		int comp;
		struct work_param* work;
		char* host_ip;
		uint16_t host_port;
		struct player* player;
		int disconnect;
		uint32_t protocolVersion;
		uint32_t verifyToken;
		char* onll_username;
		uint8_t sharedSecret[16];
		EVP_CIPHER_CTX* aes_ctx_enc;
		EVP_CIPHER_CTX* aes_ctx_dec;
};

void run_accept(struct accept_param* param);

#endif /* ACCEPT_H_ */
