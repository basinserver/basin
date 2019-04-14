/*
 * accept.h
 *
 *  Created on: Nov 18, 2015
 *      Author: root
 */

#ifndef ACCEPT_H_
#define ACCEPT_H_

#include <basin/server.h>

struct accept_param {
		struct server* server;
};

void run_accept(struct accept_param* param);

#endif /* ACCEPT_H_ */
