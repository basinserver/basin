#ifndef BASIN_PLAYER_PACKET_H
#define BASIN_PLAYER_PACKET_H

#include <basin/packet.h>
#include <basin/player.h>

typedef void (*packet_handler)(struct player* player, struct mempool* pool, void* packet);

void player_packet_handle_teleportconfirm(struct player* player, struct mempool* pool, struct pkt_play_server_teleportconfirm* packet);
void player_packet_handle_tabcomplete(struct player* player, struct mempool* pool, struct pkt_play_server_tabcomplete* packet);
void player_packet_handle_chatmessage(struct player* player, struct mempool* pool, struct pkt_play_server_chatmessage* packet);
void player_packet_handle_clientstatus(struct player* player, struct mempool* pool, struct pkt_play_server_clientstatus* packet);
void player_packet_handle_clientsettings(struct player* player, struct mempool* pool, struct pkt_play_server_clientsettings* packet);
void player_packet_handle_confirmtransaction(struct player* player, struct mempool* pool, struct pkt_play_server_confirmtransaction* packet);
void player_packet_handle_enchantitem(struct player* player, struct mempool* pool, struct pkt_play_server_enchantitem* packet);
void player_packet_handle_clickwindow(struct player* player, struct mempool* pool, struct pkt_play_server_clickwindow* packet);
void player_packet_handle_closewindow(struct player* player, struct mempool* pool, struct pkt_play_server_closewindow* packet);
void player_packet_handle_pluginmessage(struct player* player, struct mempool* pool, struct pkt_play_server_pluginmessage* packet);
void player_packet_handle_useentity(struct player* player, struct mempool* pool, struct pkt_play_server_useentity* packet);
void player_packet_handle_keepalive(struct player* player, struct mempool* pool, struct pkt_play_server_keepalive* packet);
void player_packet_handle_playerposition(struct player* player, struct mempool* pool, struct pkt_play_server_playerposition* packet);
void player_packet_handle_playerpositionandlook(struct player* player, struct mempool* pool, struct pkt_play_server_playerpositionandlook* packet);
void player_packet_handle_playerlook(struct player* player, struct mempool* pool, struct pkt_play_server_playerlook* packet);
void player_packet_handle_player(struct player* player, struct mempool* pool, struct pkt_play_server_player* packet);
void player_packet_handle_vehiclemove(struct player* player, struct mempool* pool, struct pkt_play_server_vehiclemove* packet);
void player_packet_handle_steerboat(struct player* player, struct mempool* pool, struct pkt_play_server_steerboat* packet);
void player_packet_handle_playerabilities(struct player* player, struct mempool* pool, struct pkt_play_server_playerabilities* packet);
void player_packet_handle_playerdigging(struct player* player, struct mempool* pool, struct pkt_play_server_playerdigging* packet);
void player_packet_handle_entityaction(struct player* player, struct mempool* pool, struct pkt_play_server_entityaction* packet);
void player_packet_handle_steervehicle(struct player* player, struct mempool* pool, struct pkt_play_server_steervehicle* packet);
void player_packet_handle_resourcepackstatus(struct player* player, struct mempool* pool, struct pkt_play_server_resourcepackstatus* packet);
void player_packet_handle_helditemchange(struct player* player, struct mempool* pool, struct pkt_play_server_helditemchange* packet);
void player_packet_handle_creativeinventoryaction(struct player* player, struct mempool* pool, struct pkt_play_server_creativeinventoryaction* packet);
void player_packet_handle_updatesign(struct player* player, struct mempool* pool, struct pkt_play_server_updatesign* packet);
void player_packet_handle_animation(struct player* player, struct mempool* pool, struct pkt_play_server_animation* packet);
void player_packet_handle_spectate(struct player* player, struct mempool* pool, struct pkt_play_server_spectate* packet);
void player_packet_handle_playerblockplacement(struct player* player, struct mempool* pool, struct pkt_play_server_playerblockplacement* packet);
void player_packet_handle_useitem(struct player* player, struct mempool* pool, struct pkt_play_server_useitem* packet);

packet_handler player_packet_handlers[256] = {
    (packet_handler) player_packet_handle_teleportconfirm,
    (packet_handler) player_packet_handle_tabcomplete,
    (packet_handler) player_packet_handle_chatmessage,
    (packet_handler) player_packet_handle_clientstatus,
    (packet_handler) player_packet_handle_clientsettings,
    (packet_handler) player_packet_handle_confirmtransaction,
    (packet_handler) player_packet_handle_enchantitem,
    (packet_handler) player_packet_handle_clickwindow,
    (packet_handler) player_packet_handle_closewindow,
    (packet_handler) player_packet_handle_pluginmessage,
    (packet_handler) player_packet_handle_useentity,
    (packet_handler) player_packet_handle_keepalive,
    (packet_handler) player_packet_handle_playerposition,
    (packet_handler) player_packet_handle_playerpositionandlook,
    (packet_handler) player_packet_handle_playerlook,
    (packet_handler) player_packet_handle_player,
    (packet_handler) player_packet_handle_vehiclemove,
    (packet_handler) player_packet_handle_steerboat,
    (packet_handler) player_packet_handle_playerabilities,
    (packet_handler) player_packet_handle_playerdigging,
    (packet_handler) player_packet_handle_entityaction,
    (packet_handler) player_packet_handle_steervehicle,
    (packet_handler) player_packet_handle_resourcepackstatus,
    (packet_handler) player_packet_handle_helditemchange,
    (packet_handler) player_packet_handle_creativeinventoryaction,
    (packet_handler) player_packet_handle_updatesign,
    (packet_handler) player_packet_handle_animation,
    (packet_handler) player_packet_handle_spectate,
    (packet_handler) player_packet_handle_playerblockplacement,
    (packet_handler) player_packet_handle_useitem,
};

#endif /* BASIN_PLAYER_PACKET_H */
