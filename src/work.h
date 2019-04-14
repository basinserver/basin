/*
 * work.h
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */

#ifndef WORK_H_
#define WORK_H_

#include <avuna/log.h>
#include <avuna/netmgr.h>

int connection_read(struct netmgr_connection* conn, uint8_t* read_buf, size_t read_buf_len);

void connection_on_closed(struct netmgr_connection* conn);

#endif /* WORK_H_ */
