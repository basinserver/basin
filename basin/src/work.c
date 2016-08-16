/*
 * work.c
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */

#include "work.h"
#include "accept.h"
#include "xstring.h"
#include <errno.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "collection.h"
#include "util.h"
#include "streams.h"
#include <sys/ioctl.h>
#include "network.h"
#include "globals.h"
#include <openssl/md5.h>

void closeConn(struct work_param* param, struct conn* conn) {
	close(conn->fd);
	if (rem_collection(param->conns, conn)) {
		errlog(param->logsess, "Failed to delete connection properly! This is bad!");
	}
	xfree(conn->incomingPacket);
	xfree(conn->outgoingPacket);
	if (conn->host_ip != NULL) xfree(conn->host_ip);
	if (conn->readBuffer != NULL) xfree(conn->readBuffer);
	if (conn->writeBuffer != NULL) xfree(conn->writeBuffer);
	xfree(conn);
}

int handleRead(struct conn* conn, struct work_param* param, int fd) {
	if (conn->readBuffer != NULL && conn->readBuffer_size > 0) {
		int32_t length = 0;
		if (!readVarInt(&length, conn->readBuffer, conn->readBuffer_size)) return 0;
		int ls = getVarIntSize(length);
		if (conn->readBuffer_size - ls < length) return 0;
		struct packet* inp = xmalloc(sizeof(struct packet));
		ssize_t rx = readPacket(conn, conn->readBuffer + ls, length, inp);
		if (rx == -1) return -1;
		if (conn->state == STATE_HANDSHAKE && inp->id == PKT_HANDSHAKE_CLIENT_HANDSHAKE) {
			conn->host_ip = inp->data.handshake_client.handshake.ip;
			conn->host_port = inp->data.handshake_client.handshake.port;
			if (inp->data.handshake_client.handshake.protocol_version != MC_PROTOCOL_VERSION) return -2;
			if (inp->data.handshake_client.handshake.state == STATE_STATUS) {
				conn->state = STATE_STATUS;
			} else if (inp->data.handshake_client.handshake.protocol_version == STATE_LOGIN) {
				conn->state = STATE_LOGIN;
			} else return -1;
		} else if (conn->state == STATE_STATUS) {
			if (inp->id == PKT_STATUS_CLIENT_REQUEST) {
				inp->id = PKT_STATUS_SERVER_RESPONSE;
				inp->data.status_server.response.json = xmalloc(1000);
				inp->data.status_server.response.json[999] = 0;
				snprintf(inp->data.status_server.response.json, 999, "{\"version\":{\"name\":\"1.8.9\",\"protocol\":%i},\"players\":{\"max\":%i,\"online\":%i},\"description\":{\"text\":\"%s\"}}", MC_PROTOCOL_VERSION, conn->mcs->max_players, 0, conn->mcs->motd);
				if (writePacket(conn, inp) < 0) return -1;
				xfree(inp->data.status_server.response.json);
			} else if (inp->id == PKT_STATUS_CLIENT_PING) {
				inp->id = PKT_STATUS_SERVER_PONG;
				if (writePacket(conn, inp) < 0) return -1;
				conn->state = -1;
			} else return -1;
		} else if (conn->state == STATE_LOGIN) {
			if (inp->id == PKT_LOGIN_CLIENT_LOGINSTART) {
				if (conn->mcs->online_mode) {
					//TODO
					return -1;
				} else {
					inp->id = PKT_LOGIN_SERVER_LOGINSUCCESS;
					struct uuid uuid;
					unsigned char* uuidx = (unsigned char*) &uuid;
					MD5_CTX context;
					MD5_Init(&context);
					MD5_Update(&context, inp->data.login_server.loginsuccess.username, strlen(inp->data.login_server.loginsuccess.username));
					MD5_Final(uuidx, &context);
					inp->data.login_server.loginsuccess.username = inp->data.login_client.loginstart.name;
					memcpy(&inp->data.login_server.loginsuccess.UUID, &uuid, sizeof(struct uuid));
					if (writePacket(conn, inp) < 0) return -1;
					conn->state = STATE_PLAY;
					struct player* player = newPlayer(NULL, inp->data.login_server.loginsuccess.username, uuid, conn, 0);
				}
			}
		}
		memmove(conn->readBuffer, conn->readBuffer + length + ls, conn->readBuffer_size - length - ls);
		conn->readBuffer_size -= length + ls;
	}
	return 0;
}

void run_work(struct work_param* param) {
	if (pipe(param->pipes) != 0) {
		printf("Failed to create pipe! %s\n", strerror(errno));
		return;
	}
	unsigned char wb;
	unsigned char* mbuf = xmalloc(1024);
	while (1) {
		pthread_rwlock_rdlock(&param->conns->data_mutex);
		size_t cc = param->conns->count;
		struct pollfd fds[cc + 1];
		struct conn* conns[cc];
		int fdi = 0;
		for (int i = 0; i < param->conns->size; i++) {
			if (param->conns->data[i] != NULL) {
				conns[fdi] = (param->conns->data[i]);
				struct conn* conn = conns[fdi];
				fds[fdi].fd = conns[fdi]->fd;
				fds[fdi].events = POLLIN | (conn->writeBuffer_size > 0 ? POLLOUT : 0);
				fds[fdi++].revents = 0;
				if (fdi == cc) break;
			}
		}
		pthread_rwlock_unlock(&param->conns->data_mutex);
		fds[cc].fd = param->pipes[0];
		fds[cc].events = POLLIN;
		fds[cc].revents = 0;
		int cp = poll(fds, cc + 1, -1);
		if (cp < 0) {
			printf("Poll error in worker thread! %s\n", strerror(errno));
		} else if (cp == 0) continue;
		else if ((fds[cc].revents & POLLIN) == POLLIN) {
			if (read(param->pipes[0], &wb, 1) < 1) printf("Error reading from pipe, infinite loop COULD happen here.\n");
			if (cp-- == 1) continue;
		}
		for (int i = 0; i < cc; i++) {
			int re = fds[i].revents;
			struct conn* conn = conns[i];
			if ((re & POLLERR) == POLLERR) {
				//printf("POLLERR in worker poll! This is bad!\n");
				goto cont;
			}
			if ((re & POLLHUP) == POLLHUP) {
				closeConn(param, conn);
				goto cont;
			}
			if ((re & POLLNVAL) == POLLNVAL) {
				printf("Invalid FD in worker poll! This is bad!\n");
				closeConn(param, conn);
				goto cont;
			}
			if ((re & POLLIN) == POLLIN) {
				size_t tr = 0;
				ioctl(fds[i].fd, FIONREAD, &tr);
				unsigned char* loc;
				if (conn->readBuffer == NULL) {
					conn->readBuffer = xmalloc(tr); // TODO: max upload?
					conn->readBuffer_size = tr;
					loc = conn->readBuffer;
				} else {
					conn->readBuffer_size += tr;
					conn->readBuffer = xrealloc(conn->readBuffer, conn->readBuffer_size);
					loc = conn->readBuffer + conn->readBuffer_size - tr;
				}
				ssize_t r = 0;
				if (r == 0 && tr == 0) { // nothing to read, but wont block.
					ssize_t x = read(fds[i].fd, loc + r, tr - r);
					if (x <= 0) {
						closeConn(param, conn);
						conn = NULL;
						goto cont;
					}
					r += x;
				}
				while (r < tr) {
					ssize_t x = read(fds[i].fd, loc + r, tr - r);
					if (x <= 0) {
						closeConn(param, conn);
						conn = NULL;
						goto cont;
					}
					r += x;
				}
				int p = 0;
				p = handleRead(conn, param, fds[i].fd);
				if (p == 1) {
					goto cont;
				}
			}
			if ((re & POLLOUT) == POLLOUT && conn != NULL) {
				ssize_t mtr = write(fds[i].fd, conn->writeBuffer, conn->writeBuffer_size);
				if (mtr < 0 && errno != EAGAIN) {
					closeConn(param, conn);
					conn = NULL;
					goto cont;
				} else if (mtr < 0) {
					goto cont;
				} else if (mtr < conn->writeBuffer_size) {
					memmove(conn->writeBuffer, conn->writeBuffer + mtr, conn->writeBuffer_size - mtr);
					conn->writeBuffer_size -= mtr;
					conn->writeBuffer = xrealloc(conn->writeBuffer, conn->writeBuffer_size);
				} else {
					conn->writeBuffer_size = 0;
					xfree(conn->writeBuffer);
					conn->writeBuffer = NULL;
				}
			}
			if (conn->state == -1 && conn->writeBuffer_size == 0) {
				closeConn(param, conn);
				conn = NULL;
			}
			cont: ;
			if (--cp == 0) break;
		}
	}
	xfree(mbuf);
}
