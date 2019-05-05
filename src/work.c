#include <basin/work.h>
#include <basin/accept.h>
#include <basin/packet.h>
#include <basin/login_stage_handler.h>
#include <basin/network.h>
#include <basin/globals.h>
#include <basin/connection.h>
#include <basin/entity.h>
#include <basin/server.h>
#include <basin/worldmanager.h>
#include <basin/game.h>
#include <basin/block.h>
#include <basin/item.h>
#include <basin/version.h>
#include <basin/profile.h>
#include <avuna/json.h>
#include <avuna/streams.h>
#include <avuna/queue.h>
#include <avuna/string.h>
#include <avuna/netmgr.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/md5.h>
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <avuna/util.h>


int connection_read(struct netmgr_connection* netmgr_conn, uint8_t* read_buf, size_t read_buf_len) {
    struct connection* conn = netmgr_conn->extra;
    if (conn->disconnect) {
        return 1;
    }
    if (conn->aes_ctx_dec != NULL) {
        int decrypted_length = (int) (read_buf_len + 32); // 16 extra just in case
        void* decrypted = pmalloc(netmgr_conn->read_buffer.pool, (size_t) decrypted_length);
        if (EVP_DecryptUpdate(conn->aes_ctx_dec, decrypted, &decrypted_length, read_buf, (int) read_buf_len) != 1) {
            pprefree(netmgr_conn->read_buffer.pool, decrypted);
            return 1;
        }
        if (decrypted_length == 0) return 0;
        buffer_push(&netmgr_conn->read_buffer, decrypted, (size_t) decrypted_length);
        pprefree(netmgr_conn->read_buffer.pool, read_buf);
    } else {
        buffer_push(&netmgr_conn->read_buffer, read_buf, read_buf_len);
    }
    while (netmgr_conn->read_buffer.size > 4) {
        uint8_t peek_buf[8];
        buffer_peek(&netmgr_conn->read_buffer, 5, peek_buf);
        int32_t length = 0;
        if (!readVarInt(&length, peek_buf, 5)) {
            return 0;
        }
        int ls = getVarIntSize(length);
        if (netmgr_conn->read_buffer.size - ls < length) {
            return 0;
        }
        buffer_skip(&netmgr_conn->read_buffer, (size_t) ls);
        struct mempool* packet_pool = mempool_new();
        pchild(conn->pool, packet_pool);
        uint8_t* packet_buf = pmalloc(packet_pool, (size_t) length);
        buffer_pop(&netmgr_conn->read_buffer, (size_t) length, packet_buf);

        struct packet* packet = pmalloc(packet_pool, sizeof(struct packet));
        packet->pool = packet_pool;
        ssize_t read_packet_length = packet_read(conn, packet_buf, (size_t) length, packet);

        if (read_packet_length == -1) goto ret_error;
        if (conn->protocol_state == STATE_HANDSHAKE && packet->id == PKT_HANDSHAKE_SERVER_HANDSHAKE) {
            if (handle_packet_handshake(conn, packet)) {
                goto ret_error;
            }
        } else if (conn->protocol_state == STATE_STATUS) {
            if (handle_packet_status(conn, packet)) {
                goto ret_error;
            }
        } else if (conn->protocol_state == STATE_LOGIN) {
            if (handle_packet_login(conn, packet)) {
                goto ret_error;
            }
        } else if (conn->protocol_state == STATE_PLAY) {
            queue_push(conn->player->incoming_packets, packet);
        } else {
            goto ret_error;
        }
        goto ret;
        ret_error: ;
        pfree(packet->pool);
        return 1;
        ret: ;
        pfree(packet->pool);
    }

    return 0;
}

void connection_on_closed(struct netmgr_connection* netmgr_conn) {
    struct connection* conn = netmgr_conn->extra;
    if (conn->player != NULL) {
        broadcastf("yellow", "%s has left the server!", conn->player->name);
        conn->player->conn = NULL;
    }
    pfree(conn->pool);

}