#ifndef BASIN_LOGIN_STAGE_HANDLER_H
#define BASIN_LOGIN_STAGE_HANDLER_H

#include <basin/connection.h>
#include <basin/packet.h>

int handle_packet_handshake(struct connection* conn, struct packet* packet);

int handle_packet_login(struct connection* conn, struct packet* packet);

int handle_packet_status(struct connection* conn, struct packet* packet);

#endif //BASIN_LOGIN_STAGE_HANDLER_H
