
#include <basin/connection.h>
#include <basin/packet.h>


void connection_flush(struct player* player) {
    if (player->conn == NULL) return;
    struct packet* packet;
    while ((packet = queue_maybepop(player->outgoing_packets)) != NULL) {
        if(packet_write(player->conn, packet)) {
            // TODO: we need to standardize how we close connections
            player->conn->disconnect = 1;
            break;
        }
    }
    netmgr_trigger_write(player->conn->managed_conn);
}