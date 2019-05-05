//
// Created by p on 4/24/19.
//

#include <basin/player_packet.h>
#include <basin/player.h>
#include <basin/world.h>
#include <basin/packet.h>
#include <basin/crafting.h>
#include <basin/smelting.h>
#include <basin/game.h>
#include <basin/command.h>
#include <basin/anticheat.h>
#include <basin/globals.h>
#include <basin/plugin.h>
#include <avuna/pmem.h>
#include <avuna/queue.h>
#include <avuna/string.h>

void player_packet_handle_teleportconfirm(struct player* player, struct mempool* pool, struct pkt_play_server_teleportconfirm* packet) {
    if (packet->teleport_id == player->last_teleport_id) {
        player->last_teleport_id = 0;
    }
}

void player_packet_handle_tabcomplete(struct player* player, struct mempool* pool, struct pkt_play_server_tabcomplete* packet) {
    
}

void player_packet_handle_chatmessage(struct player* player, struct mempool* pool, struct pkt_play_server_chatmessage* packet) {
    char* msg = packet->message;
    if (ac_chat(player, msg)) return;
    if (str_prefixes(msg, "/")) {
        callCommand(player, pool, msg + 1);
    } else {
        size_t max_size = strlen(msg) + 128;
        char* broadcast_str = pmalloc(pool, max_size);
        snprintf(broadcast_str, max_size, "{\"text\": \"<%s>: %s\"}", player->name, str_replace(str_replace(msg, "\\", "\\\\", pool), "\"", "\\\"", pool));
        printf("<CHAT> <%s>: %s\n", player->name, msg);
        BEGIN_BROADCAST(player->server->players_by_entity_id) {
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_CHATMESSAGE);
            pkt->data.play_client.chatmessage.position = 0;
            pkt->data.play_client.chatmessage.json_data = str_dup(broadcast_str, 0, pkt->pool);
            queue_push(bc_player->outgoing_packets, pkt);
            END_BROADCAST(player->server->players_by_entity_id)
        }
    }
}

void player_packet_handle_clientstatus(struct player* player, struct mempool* pool, struct pkt_play_server_clientstatus* packet) {
    if (packet->action_id == 0 && player->entity->health <= 0.) {
        world_despawn_entity(player->world, player->entity);
        world_spawn_entity(player->world, player->entity);
        player->entity->health = 20.f;
        player->food = 20;
        player->entity->fallDistance = 0.f;
        player->saturation = 0.f; // TODO
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_UPDATEHEALTH);
        pkt->data.play_client.updatehealth.health = player->entity->health;
        pkt->data.play_client.updatehealth.food = player->food;
        pkt->data.play_client.updatehealth.food_saturation = player->saturation;
        queue_push(player->outgoing_packets, pkt);
        pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_RESPAWN);
        pkt->data.play_client.respawn.dimension = player->world->dimension;
        pkt->data.play_client.respawn.difficulty = (uint8_t) player->server->difficulty;
        pkt->data.play_client.respawn.gamemode = player->gamemode;
        pkt->data.play_client.respawn.level_type = player->world->level_type;
        queue_push(player->outgoing_packets, pkt);
        player_teleport(player, (double) player->world->spawnpos.x + .5, (double) player->world->spawnpos.y, (double) player->world->spawnpos.z + .5); // TODO: make overworld!!!
        player_set_gamemode(player, -1);
        ITER_MAP(plugins) {
            struct plugin* plugin = value;
            if (plugin->onPlayerSpawn != NULL) (*plugin->onPlayerSpawn)(player->world, player);
            ITER_MAP_END()
        }
        player->entity->invincibilityTicks = 5;
    }
}

void player_packet_handle_clientsettings(struct player* player, struct mempool* pool, struct pkt_play_server_clientsettings* packet) {
    
}

void player_packet_handle_confirmtransaction(struct player* player, struct mempool* pool, struct pkt_play_server_confirmtransaction* packet) {
    
}

void player_packet_handle_enchantitem(struct player* player, struct mempool* pool, struct pkt_play_server_enchantitem* packet) {
    
}

void player_packet_handle_clickwindow(struct player* player, struct mempool* pool, struct pkt_play_server_clickwindow* packet) {
    struct inventory* inventory = NULL;
    if (packet->window_id == 0 && player->open_inventory == NULL) {
        inventory = player->inventory;
    } else if (player->open_inventory != NULL && packet->window_id == player->open_inventory->window) {
        inventory = player->open_inventory;
    }
    if (inventory == NULL) {
        return;
    }
    int8_t button = packet->button;
    int16_t action = packet->action_number;
    int32_t mode = packet->mode;
    int16_t slot = packet->slot;
    pthread_mutex_lock(&inventory->mutex);
    int craftable = inventory->type == INVTYPE_PLAYERINVENTORY || inventory->type == INVTYPE_WORKBENCH;
    // printf("click mode=%i, button=%i, slot=%i\n", mode, button, slot);
    int force_desync = 0;
    if (mode == 5 && button == 10 && player->gamemode != 1) force_desync = 1;
    if (slot >= 0 || force_desync) {
        struct slot* item = force_desync ? NULL : inventory_get(player, inventory, slot);
        if (((mode == 0 || mode == 1 || mode == 6) && ((packet->clicked_item.item < 0) != (item == NULL))) || (item != NULL && packet->clicked_item.item >= 0 && !(slot_stackable(item, &packet->clicked_item) && item->count == packet->clicked_item.count)) || force_desync) {
            // printf("desync\n");
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_CONFIRMTRANSACTION);
            pkt->data.play_client.confirmtransaction.window_id = inventory->window;
            pkt->data.play_client.confirmtransaction.action_number = action;
            pkt->data.play_client.confirmtransaction.accepted = 0;
            queue_push(player->outgoing_packets, pkt);
            if (inventory != player->inventory) {
                pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWITEMS);
                pkt->data.play_client.windowitems.window_id = inventory->window;
                pkt->data.play_client.windowitems.count = inventory->slot_count;
                pkt->data.play_client.windowitems.slot_data = pmalloc(pkt->pool, sizeof(struct slot) * inventory->slot_count);
                for (size_t i = 0; i < inventory->slot_count; i++) {
                    slot_duplicate(pkt->pool, inventory->slots[i], &pkt->data.play_client.windowitems.slot_data[i]);
                }
                queue_push(player->outgoing_packets, pkt);
            }
            pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_WINDOWITEMS);
            pkt->data.play_client.windowitems.window_id = player->inventory->window;
            pkt->data.play_client.windowitems.count = player->inventory->slot_count;
            pkt->data.play_client.windowitems.slot_data = pmalloc(pkt->pool, sizeof(struct slot) * player->inventory->slot_count);
            for (size_t i = 0; i < player->inventory->slot_count; i++) {
                slot_duplicate(pkt->pool, player->inventory->slots[i], &pkt->data.play_client.windowitems.slot_data[i]);
            }
            queue_push(player->outgoing_packets, pkt);
            pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_SETSLOT);
            pkt->data.play_client.setslot.window_id = -1;
            pkt->data.play_client.setslot.slot = -1;
            slot_duplicate(pkt->pool, player->inventory_holding, &pkt->data.play_client.setslot.slot_data);
            queue_push(player->outgoing_packets, pkt);
        } else {
            if (mode != 5 && inventory->drag_slot->size > 0) {
                pfree(inventory->drag_pool);
                inventory->drag_pool = mempool_new();
                inventory->drag_slot = llist_new(inventory->drag_pool);
            }
            int sto = 0;
            if (inventory->type == INVTYPE_FURNACE) {
                sto = slot == 2;
            }
            if (mode == 0) {
                if (craftable && slot == 0 && item != NULL) {
                    if (player->inventory_holding == NULL) {
                        player->inventory_holding = item;
                        item = NULL;
                        inventory_set_slot(player, inventory, 0, item, 0);
                        pxfer(inventory->pool, player->pool, player->inventory_holding);
                        if (player->inventory_holding->nbt != NULL) {
                            pxfer_parent(inventory->pool, player->pool, player->inventory_holding->nbt->pool);
                        }
                        crafting_once(player, inventory);
                    } else if (slot_stackable(player->inventory_holding, item) && (player->inventory_holding->count + item->count) < slot_max_size(player->inventory_holding)) {
                        player->inventory_holding->count += item->count;
                        item = NULL;
                        inventory_set_slot(player, inventory, 0, item, 0);
                        crafting_once(player, inventory);
                    }
                } else if (button == 0) {
                    if (slot_stackable(player->inventory_holding, item)) {
                        if (sto) {
                            uint8_t os = player->inventory_holding->count;
                            int mss = slot_max_size(player->inventory_holding);
                            player->inventory_holding->count += item->count;
                            if (player->inventory_holding->count > mss) player->inventory_holding->count = mss;
                            int dr = player->inventory_holding->count - os;
                            if (dr >= item->count) {
                                item = NULL;
                            } else item->count -= dr;
                            inventory_set_slot(player, inventory, slot, item, 0);
                        } else {
                            uint8_t os = item->count;
                            int mss = slot_max_size(player->inventory_holding);
                            item->count += player->inventory_holding->count;
                            if (item->count > mss) item->count = mss;
                            inventory_set_slot(player, inventory, slot, item, 0);
                            int dr = item->count - os;
                            if (dr >= player->inventory_holding->count) {
                                if (player->inventory_holding->nbt != NULL) {
                                    pfree(player->inventory_holding->nbt->pool);
                                }
                                pprefree(player->pool, player->inventory_holding);
                                player->inventory_holding = NULL;
                            } else {
                                player->inventory_holding->count -= dr;
                            }
                        }
                    } else if (player->inventory_holding == NULL || inventory_validate(inventory->type, slot, player->inventory_holding)) {
                        struct slot* t = item;
                        item = player->inventory_holding;
                        player->inventory_holding = t;
                        pxfer(inventory->pool, player->pool, player->inventory_holding);
                        if (player->inventory_holding->nbt != NULL) {
                            pxfer_parent(inventory->pool, player->pool, player->inventory_holding->nbt->pool);
                        }
                        inventory_set_slot(player, inventory, slot, item, 0);
                    }
                } else if (button == 1) {
                    if (player->inventory_holding == NULL && item != NULL) {
                        player->inventory_holding = pmalloc(player->pool, sizeof(struct slot));
                        slot_duplicate(player->pool, item, player->inventory_holding);
                        uint8_t os = item->count;
                        item->count /= 2;
                        player->inventory_holding->count = os - item->count;
                        inventory_set_slot(player, inventory, slot, item, 0);
                    } else if (player->inventory_holding != NULL && !sto && inventory_validate(inventory->type, slot, player->inventory_holding) && (item == NULL || (slot_stackable(player->inventory_holding, item) && player->inventory_holding->count + item->count < slot_max_size(player->inventory_holding)))) {
                        if (item == NULL) {
                            item = pmalloc(inventory->pool, sizeof(struct slot));
                            slot_duplicate(inventory->pool, player->inventory_holding, item);
                            item->count = 1;
                        } else {
                            item->count++;
                        }
                        if (--player->inventory_holding->count <= 0) {
                            if (player->inventory_holding->nbt != NULL) {
                                pfree(player->inventory_holding->nbt->pool);
                            }
                            pprefree(player->pool, player->inventory_holding);
                            player->inventory_holding = NULL;
                        }
                        inventory_set_slot(player, inventory, slot, item, 0);
                    }
                }
            } else if (mode == 1 && item != NULL) {
                if (inventory->type == INVTYPE_PLAYERINVENTORY) {
                    int16_t it = item->item;
                    if (slot == 0) {
                        int amt = crafting_all(player, inventory);
                        for (int i = 0; i < amt; i++)
                            inventory_add(player, inventory, item, 44, 8, 0);
                        game_update_inventory(player, inventory, 1); // 2-4 would just repeat the calculation
                    } else if (slot != 45 && it == ITM_SHIELD && inventory->slots[45] == NULL) {
                        inventory_swap(player, inventory, 45, slot, 0);
                    } else if (slot != 5 && inventory->slots[5] == NULL && (it == BLK_PUMPKIN || it == ITM_HELMETCLOTH || it == ITM_HELMETCHAIN || it == ITM_HELMETIRON || it == ITM_HELMETDIAMOND || it == ITM_HELMETGOLD)) {
                        inventory_swap(player, inventory, 5, slot, 0);
                    } else if (slot != 6 && inventory->slots[6] == NULL && (it == ITM_CHESTPLATECLOTH || it == ITM_CHESTPLATECHAIN || it == ITM_CHESTPLATEIRON || it == ITM_CHESTPLATEDIAMOND || it == ITM_CHESTPLATEGOLD)) {
                        inventory_swap(player, inventory, 6, slot, 0);
                    } else if (slot != 7 && inventory->slots[7] == NULL && (it == ITM_LEGGINGSCLOTH || it == ITM_LEGGINGSCHAIN || it == ITM_LEGGINGSIRON || it == ITM_LEGGINGSDIAMOND || it == ITM_LEGGINGSGOLD)) {
                        inventory_swap(player, inventory, 7, slot, 0);
                    } else if (slot != 8 && inventory->slots[8] == NULL && (it == ITM_BOOTSCLOTH || it == ITM_BOOTSCHAIN || it == ITM_BOOTSIRON || it == ITM_BOOTSDIAMOND || it == ITM_BOOTSGOLD)) {
                        inventory_swap(player, inventory, 8, slot, 0);
                    } else if (slot > 35 && slot < 45 && item != NULL) {
                        int r = inventory_add(player, inventory, item, 9, 36, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    } else if (slot > 8 && slot < 36 && item != NULL) {
                        int r = inventory_add(player, inventory, item, 36, 45, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    } else if ((slot == 45 || slot < 9) && item != NULL) {
                        int r = inventory_add(player, inventory, item, 9, 36, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                        else {
                            r = inventory_add(player, inventory, item, 36, 45, 0);
                            if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                        }
                    }
                } else if (inventory->type == INVTYPE_WORKBENCH) {
                    if (slot == 0) {
                        int amt = crafting_all(player, inventory);
                        for (int i = 0; i < amt; i++)
                            inventory_add(player, inventory, inventory->slots[0], 45, 9, 0);
                        game_update_inventory(player, inventory, 1); // 2-4 would just repeat the calculation
                    } else if (slot <= 9) {
                        int r = inventory_add(player, inventory, item, 10, 46, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    } else if (slot > 9 && slot < 37) {
                        int r = inventory_add(player, inventory, item, 37, 46, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    } else if (slot > 36 && slot < 46) {
                        int r = inventory_add(player, inventory, item, 10, 37, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    }
                } else if (inventory->type == INVTYPE_CHEST) {
                    if (slot < 27) {
                        int r = inventory_add(player, inventory, item, 62, 26, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    } else if (slot >= 27) {
                        int r = inventory_add(player, inventory, item, 0, 27, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    }
                } else if (inventory->type == INVTYPE_FURNACE) {
                    if (slot > 2) {
                        if (smelting_output(item) != NULL) inventory_swap(player, inventory, 0, slot, 0);
                        else if (smelting_burnTime(item) > 0) inventory_swap(player, inventory, 1, slot, 0);
                        else if (slot > 29) {
                            int r = inventory_add(player, inventory, item, 3, 30, 0);
                            if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                        } else if (slot < 30) {
                            int r = inventory_add(player, inventory, item, 30, 39, 0);
                            if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                        }
                    } else {
                        int r = inventory_add(player, inventory, item, 38, 2, 0);
                        if (r <= 0) inventory_set_slot(player, inventory, slot, NULL, 0);
                    }
                }
            } else if (mode == 2) {
                if (inventory->type == INVTYPE_PLAYERINVENTORY) {
                    if (slot == 0) {
                        struct slot* invb = inventory_get(player, inventory, 36 + button);
                        if (invb == NULL) {
                            inventory_set_slot(player, inventory, 36 + button, inventory->slots[0], 0);
                            inventory_set_slot(player, inventory, 0, NULL, 0);
                            crafting_once(player, inventory);
                        }
                    } else if (button >= 0 && button <= 8) inventory_swap(player, inventory, 36 + button, slot, 0);
                } else if (inventory->type == INVTYPE_WORKBENCH) {
                    if (slot == 0) {
                        struct slot* invb = inventory_get(player, inventory, 37 + button);
                        if (invb == NULL) {
                            inventory_set_slot(player, inventory, 37 + button, inventory->slots[0], 0);
                            inventory_set_slot(player, inventory, 0, NULL, 0);
                            crafting_once(player, inventory);
                        }
                    } else if (button >= 0 && button <= 8) inventory_swap(player, inventory, 37 + button, slot, 0);
                } else if (inventory->type == INVTYPE_CHEST) {
                    if (button >= 0 && button <= 8) inventory_swap(player, inventory, 54 + button, slot, 0);
                } else if (inventory->type == INVTYPE_FURNACE) {
                    if (button >= 0 && button <= 8) inventory_swap(player, inventory, 30 + button, slot, 0);
                }
            } else if (mode == 3) {
                if (player->gamemode == 1) {
                    if (item != NULL && player->inventory_holding == NULL) {
                        player->inventory_holding = pmalloc(player->pool, sizeof(struct slot));
                        slot_duplicate(player->pool, item, player->inventory_holding);
                        player->inventory_holding->count = slot_max_size(player->inventory_holding);
                    }
                }
                //middle click, NOP in survival?
            } else if (mode == 4 && item != NULL) {
                if (button == 0) {
                    uint8_t p = item->count;
                    item->count = 1;
                    dropPlayerItem(player, item);
                    item->count = p - 1;
                    if (item->count == 0) item = NULL;
                    inventory_set_slot(player, inventory, slot, item, 1);
                } else if (button == 1) {
                    dropPlayerItem(player, item);
                    item = NULL;
                    inventory_set_slot(player, inventory, slot, item, 1);
                }
                if (craftable && slot == 0) crafting_once(player, inventory);
            } else if (mode == 5) {
                if (button == 1 || button == 5 || button == 9) {
                    int ba = 0;
                    if (inventory->type == INVTYPE_PLAYERINVENTORY) {
                        if (slot == 0 || (slot >= 5 && slot <= 8)) {
                            ba = 1;
                        }
                    }
                    if (!ba && inventory->drag_slot->size < inventory->slot_count && inventory_validate(inventory->type, slot, player->inventory_holding) && (item == NULL || slot_stackable(item, player->inventory_holding))) {
                        llist_append(inventory->drag_slot, (void*) slot);
                    }
                }
            } else if (mode == 6 && player->inventory_holding != NULL) {
                int mss = slot_max_size(player->inventory_holding);
                if (inventory->type == INVTYPE_PLAYERINVENTORY) {
                    if (player->inventory_holding->count < mss) {
                        for (size_t i = 9; i < 36; i++) {
                            struct slot* invi = inventory_get(player, inventory, i);
                            if (slot_stackable(player->inventory_holding, invi)) {
                                uint8_t oc = player->inventory_holding->count;
                                player->inventory_holding->count += invi->count;
                                if (player->inventory_holding->count > mss) player->inventory_holding->count = mss;
                                invi->count -= player->inventory_holding->count - oc;
                                if (invi->count <= 0) invi = NULL;
                                inventory_set_slot(player, inventory, i, invi, 0);
                                if (player->inventory_holding->count >= mss) break;
                            }
                        }
                    }
                    if (player->inventory_holding->count < mss) {
                        for (size_t i = 36; i < 45; i++) {
                            struct slot* invi = inventory_get(player, inventory, i);
                            if (slot_stackable(player->inventory_holding, invi)) {
                                uint8_t oc = player->inventory_holding->count;
                                player->inventory_holding->count += invi->count;
                                if (player->inventory_holding->count > mss) player->inventory_holding->count = mss;
                                invi->count -= player->inventory_holding->count - oc;
                                if (invi->count <= 0) invi = NULL;
                                inventory_set_slot(player, inventory, i, invi, 0);
                                if (player->inventory_holding->count >= mss) break;
                            }
                        }
                    }
                } else if (inventory->type == INVTYPE_WORKBENCH) {
                    if (player->inventory_holding->count < mss) {
                        for (size_t i = 1; i < 46; i++) {
                            struct slot* invi = inventory_get(player, inventory, i);
                            if (slot_stackable(player->inventory_holding, invi)) {
                                uint8_t oc = player->inventory_holding->count;
                                player->inventory_holding->count += invi->count;
                                if (player->inventory_holding->count > mss) player->inventory_holding->count = mss;
                                invi->count -= player->inventory_holding->count - oc;
                                if (invi->count <= 0) invi = NULL;
                                inventory_set_slot(player, inventory, i, invi, 0);
                                if (player->inventory_holding->count >= mss) break;
                            }
                        }
                    }
                } else if (inventory->type == INVTYPE_CHEST) {
                    if (player->inventory_holding->count < mss) {
                        for (size_t i = 0; i < 63; i++) {
                            struct slot* invi = inventory_get(player, inventory, i);
                            if (slot_stackable(player->inventory_holding, invi)) {
                                uint8_t oc = player->inventory_holding->count;
                                player->inventory_holding->count += invi->count;
                                if (player->inventory_holding->count > mss) player->inventory_holding->count = mss;
                                invi->count -= player->inventory_holding->count - oc;
                                if (invi->count <= 0) invi = NULL;
                                inventory_set_slot(player, inventory, i, invi, 0);
                                if (player->inventory_holding->count >= mss) break;
                            }
                        }
                    }
                } else if (inventory->type == INVTYPE_FURNACE) {
                    if (player->inventory_holding->count < mss) {
                        for (size_t i = 3; i < 39; i++) {
                            struct slot* invi = inventory_get(player, inventory, i);
                            if (slot_stackable(player->inventory_holding, invi)) {
                                uint8_t oc = player->inventory_holding->count;
                                player->inventory_holding->count += invi->count;
                                if (player->inventory_holding->count > mss) player->inventory_holding->count = mss;
                                invi->count -= player->inventory_holding->count - oc;
                                if (invi->count <= 0) invi = NULL;
                                inventory_set_slot(player, inventory, i, invi, 0);
                                if (player->inventory_holding->count >= mss) break;
                            }
                        }
                    }
                }
            }
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_CONFIRMTRANSACTION);
            pkt->data.play_client.confirmtransaction.window_id = inventory->window;
            pkt->data.play_client.confirmtransaction.action_number = action;
            pkt->data.play_client.confirmtransaction.accepted = 1;
            queue_push(player->outgoing_packets, pkt);
        }
    } else if (slot == -999) {
        if (mode != 5 && inventory->drag_slot->size > 0) {
            pfree(inventory->drag_pool);
            inventory->drag_pool = mempool_new();
            inventory->drag_slot = llist_new(inventory->drag_pool);
        }
        if (mode == 0 && player->inventory_holding != NULL) {
            if (button == 1) {
                uint8_t p = player->inventory_holding->count;
                player->inventory_holding->count = 1;
                dropPlayerItem(player, player->inventory_holding);
                player->inventory_holding->count = p - 1;
                if (player->inventory_holding->count == 0) {
                    if (player->inventory_holding->nbt != NULL) {
                        pfree(player->inventory_holding->nbt->pool);
                    }
                    pprefree(player->pool, player->inventory_holding);
                    player->inventory_holding = NULL;
                }
            } else if (button == 0) {
                dropPlayerItem(player, player->inventory_holding);
                if (player->inventory_holding->nbt != NULL) {
                    pfree(player->inventory_holding->nbt->pool);
                }
                pprefree(player->pool, player->inventory_holding);
                player->inventory_holding = NULL;
            }
        } else if (mode == 5 && player->inventory_holding != NULL) {
            if (button == 0 || button == 4 || button == 8) {
                pfree(inventory->drag_pool);
                inventory->drag_pool = mempool_new();
                inventory->drag_slot = llist_new(inventory->drag_pool);
            } else if (inventory->drag_slot->size > 0) {
                if (button == 2) {
                    uint8_t total = player->inventory_holding->count;
                    uint8_t per = total / inventory->drag_slot->size;
                    if (per == 0) per = 1;
                    ITER_LLIST(inventory->drag_slot, slot_ptr) {
                        uint16_t drag_slot = (uint16_t) slot_ptr;
                        struct slot* ssl = inventory_get(player, inventory, drag_slot);
                        if (ssl == NULL) {
                            ssl = pmalloc(inventory->pool, sizeof(struct slot));
                            slot_duplicate(inventory->pool, player->inventory_holding, ssl);
                            ssl->count = per;
                            inventory_set_slot(player, inventory, drag_slot, ssl, 0);
                            if (per >= player->inventory_holding->count) {
                                if (player->inventory_holding->nbt != NULL) {
                                    pfree(player->inventory_holding->nbt->pool);
                                }
                                pprefree(player->pool, player->inventory_holding);
                                player->inventory_holding = NULL;
                                break;
                            }
                            player->inventory_holding->count -= per;
                        } else {
                            uint8_t os = ssl->count;
                            ssl->count += per;
                            int mss = slot_max_size(player->inventory_holding);
                            if (ssl->count > mss) ssl->count = mss;
                            inventory_set_slot(player, inventory, drag_slot, ssl, 0);
                            if (per >= player->inventory_holding->count) {
                                if (player->inventory_holding->nbt != NULL) {
                                    pfree(player->inventory_holding->nbt->pool);
                                }
                                pprefree(player->pool, player->inventory_holding);
                                player->inventory_holding = NULL;
                                break;
                            }
                            player->inventory_holding->count -= ssl->count - os;
                        }
                        ITER_LLIST_END();
                    }
                } else if (button == 6) {
                    uint8_t per = 1;
                    ITER_LLIST(inventory->drag_slot, slot_ptr) {
                        uint16_t drag_slot = (uint16_t) slot_ptr;
                        struct slot* ssl = inventory_get(player, inventory, drag_slot);
                        if (ssl == NULL) {
                            ssl = pmalloc(inventory->pool, sizeof(struct slot));
                            slot_duplicate(inventory->pool, player->inventory_holding, ssl);
                            ssl->count = per;
                            inventory_set_slot(player, inventory, drag_slot, ssl, 0);
                            if (per >= player->inventory_holding->count) {
                                if (player->inventory_holding->nbt != NULL) {
                                    pfree(player->inventory_holding->nbt->pool);
                                }
                                pprefree(player->pool, player->inventory_holding);
                                player->inventory_holding = NULL;
                                break;
                            }
                            player->inventory_holding->count -= per;
                        } else {
                            uint8_t os = ssl->count;
                            ssl->count += per;
                            int mss = slot_max_size(player->inventory_holding);
                            if (ssl->count > mss) ssl->count = mss;
                            inventory_set_slot(player, inventory, drag_slot, ssl, 0);
                            if (per >= player->inventory_holding->count) {
                                if (player->inventory_holding->nbt != NULL) {
                                    pfree(player->inventory_holding->nbt->pool);
                                }
                                pprefree(player->pool, player->inventory_holding);
                                player->inventory_holding = NULL;
                                break;
                            }
                            player->inventory_holding->count -= ssl->count - os;
                        }
                        ITER_LLIST_END();
                    }
                } else if (button == 10 && player->gamemode == 1) {
                    uint8_t per = slot_max_size(player->inventory_holding);
                    ITER_LLIST(inventory->drag_slot, slot_ptr) {
                        uint16_t drag_slot = (uint16_t) slot_ptr;
                        struct slot* ssl = inventory_get(player, inventory, drag_slot);
                        if (ssl == NULL) {
                            ssl = pmalloc(inventory->pool, sizeof(struct slot));
                            slot_duplicate(inventory->pool, player->inventory_holding, ssl);
                            ssl->count = per;
                            inventory_set_slot(player, inventory, drag_slot, ssl, 0);
                        } else {
                            ssl->count = per;
                            inventory_set_slot(player, inventory, drag_slot, ssl, 0);
                        }
                        ITER_LLIST_END();
                    }
                }
            }
        }
    }
    pthread_mutex_unlock(&inventory->mutex);
}

void player_packet_handle_closewindow(struct player* player, struct mempool* pool, struct pkt_play_server_closewindow* packet) {
    player_closeWindow(player, packet->window_id);
}

void player_packet_handle_pluginmessage(struct player* player, struct mempool* pool, struct pkt_play_server_pluginmessage* packet) {
    
}

void player_packet_handle_useentity(struct player* player, struct mempool* pool, struct pkt_play_server_useentity* packet) {
    struct entity* entity = world_get_entity(player->world, packet->target);
    if (packet->type == 0) {
        if (entity != NULL && entity != player->entity && entity->health > 0. && entity->world == player->world && entity_dist(entity, player->entity) < 4.) {
            struct entity_info* info = entity_get_info(entity->type);
            if (info != NULL && info->onInteract != NULL) {
                pthread_mutex_lock(&player->inventory->mutex);
                (*info->onInteract)(player->world, entity, player, inventory_get(player, player->inventory, 36 + player->currentItem), (int16_t) (36 + player->currentItem));
                pthread_mutex_unlock(&player->inventory->mutex);
            }
        }
    } else if (packet->type == 1) {
        if (entity != NULL && entity != player->entity && entity->health > 0. && entity->world == player->world && entity_dist(entity, player->entity) < 4. && (player->world->tick_counter - player->lastSwing) >= 3) {
            pthread_mutex_lock(&player->inventory->mutex);
            damageEntityWithItem(entity, player->entity, (uint8_t) (36 + player->currentItem), inventory_get(player, player->inventory, 36 + player->currentItem));
            pthread_mutex_unlock(&player->inventory->mutex);
        }
        player->lastSwing = player->world->tick_counter;
    }
}

void player_packet_handle_keepalive(struct player* player, struct mempool* pool, struct pkt_play_server_keepalive* packet) {
    if (packet->keep_alive_id == player->next_keep_alive) {
        player->next_keep_alive = 0;
    }
}

void player_packet_handle_playerposition(struct player* player, struct mempool* pool, struct pkt_play_server_playerposition* packet) {
    if (player->last_teleport_id != 0 || ac_tick(player, packet->on_ground) || ac_tickpos(player, packet->x, packet->feet_y, packet->z)) return;
    double lastX = player->entity->x;
    double lastY = player->entity->y;
    double lastZ = player->entity->z;
    player->entity->last_yaw = player->entity->yaw;
    player->entity->last_pitch = player->entity->pitch;
    double moveX = packet->x - lastX;
    double moveY = packet->feet_y - lastY;
    double moveZ = packet->z - lastZ;
    if (entity_move(player->entity, &moveX, &moveY, &moveZ, .05)) {
        player_teleport(player, lastX, lastY, lastZ);
    } else {
        player->entity->last_x = lastX;
        player->entity->last_y = lastY;
        player->entity->last_z = lastZ;
    }
    player->entity->on_ground = packet->on_ground;
    double dx = player->entity->x - player->entity->last_x;
    double dy = player->entity->y - player->entity->last_y;
    double dz = player->entity->z - player->entity->last_z;
    double d3 = dx * dx + dy * dy + dz * dz;
    if (!player->spawned_in && d3 > 0.00000001) {
        player->spawned_in = 1;
    }
    if (d3 > 5. * 5. * 5.) {
        player_teleport(player, player->entity->last_x, player->entity->last_y, player->entity->last_z);
        player_kick(player, "You attempted to move too fast!");
    } else {
        BEGIN_BROADCAST(player->entity->loadingPlayers) {
            player_send_entity_move(bc_player, player->entity);
            END_BROADCAST(player->entity->loadingPlayers)
        }
    }
}

void player_packet_handle_playerpositionandlook(struct player* player, struct mempool* pool, struct pkt_play_server_playerpositionandlook* packet) {
    if (player->last_teleport_id != 0 || ac_tick(player, packet->on_ground) || ac_tickpos(player, packet->x, packet->feet_y, packet->z) || ac_ticklook(player, packet->yaw, packet->pitch)) return;
    double lastX = player->entity->x;
    double lastY = player->entity->y;
    double lastZ = player->entity->z;
    player->entity->last_yaw = player->entity->yaw;
    player->entity->last_pitch = player->entity->pitch;
    double moveX = packet->x - lastX;
    double moveY = packet->feet_y - lastY;
    double moveZ = packet->z - lastZ;
    if (entity_move(player->entity, &moveX, &moveY, &moveZ, .05)) {
        player_teleport(player, lastX, lastY, lastZ);
    } else {
        player->entity->last_x = lastX;
        player->entity->last_y = lastY;
        player->entity->last_z = lastZ;
    }
    player->entity->on_ground = packet->on_ground;
    player->entity->yaw = packet->yaw;
    player->entity->pitch = packet->pitch;
    double dx = player->entity->x - player->entity->last_x;
    double dy = player->entity->y - player->entity->last_y;
    double dz = player->entity->z - player->entity->last_z;
    double d3 = dx * dx + dy * dy + dz * dz;
    if (!player->spawned_in && d3 > 0.00000001) {
        player->spawned_in = 1;
    }
    if (d3 > 5. * 5. * 5.) {
        player_teleport(player, player->entity->last_x, player->entity->last_y, player->entity->last_z);
        player_kick(player, "You attempted to move too fast!");
    } else {
        BEGIN_BROADCAST(player->entity->loadingPlayers) {
            player_send_entity_move(bc_player, player->entity);
            END_BROADCAST(player->entity->loadingPlayers)
        }
    }
}

void player_packet_handle_playerlook(struct player* player, struct mempool* pool, struct pkt_play_server_playerlook* packet) {
    if (player->last_teleport_id != 0 || ac_tick(player, packet->on_ground) || ac_ticklook(player, packet->yaw, packet->pitch)) return;
    player->entity->last_x = player->entity->x;
    player->entity->last_y = player->entity->y;
    player->entity->last_z = player->entity->z;
    player->entity->last_yaw = player->entity->yaw;
    player->entity->last_pitch = player->entity->pitch;
    player->entity->on_ground = packet->on_ground;
    player->entity->yaw = packet->yaw;
    player->entity->pitch = packet->pitch;
    BEGIN_BROADCAST(player->entity->loadingPlayers) {
        player_send_entity_move(bc_player, player->entity);
        END_BROADCAST(player->entity->loadingPlayers)
    }

}

void player_packet_handle_player(struct player* player, struct mempool* pool, struct pkt_play_server_player* packet) {
    if (player->last_teleport_id != 0 || ac_tick(player, packet->on_ground)) return;
    player->entity->last_x = player->entity->x;
    player->entity->last_y = player->entity->y;
    player->entity->last_z = player->entity->z;
    player->entity->last_yaw = player->entity->yaw;
    player->entity->last_pitch = player->entity->pitch;
    player->entity->on_ground = packet->on_ground;
    BEGIN_BROADCAST(player->entity->loadingPlayers) {
        player_send_entity_move(bc_player, player->entity);
        END_BROADCAST(player->entity->loadingPlayers)
    }
}

void player_packet_handle_vehiclemove(struct player* player, struct mempool* pool, struct pkt_play_server_vehiclemove* packet) {
    
}

void player_packet_handle_steerboat(struct player* player, struct mempool* pool, struct pkt_play_server_steerboat* packet) {
    
}

void player_packet_handle_playerabilities(struct player* player, struct mempool* pool, struct pkt_play_server_playerabilities* packet) {
    
}

void player_packet_handle_playerdigging(struct player* player, struct mempool* pool, struct pkt_play_server_playerdigging* packet) {
    pthread_mutex_lock(&player->inventory->mutex);
    struct slot* ci = inventory_get(player, player->inventory, player->currentItem + 36);
    struct item_info* ii = ci == NULL ? NULL : getItemInfo(ci->item);
    if (packet->status == 0) {
        if (entity_dist_block(player->entity, packet->location.x, packet->location.y, packet->location.z) > player->reachDistance) {
            pthread_mutex_unlock(&player->inventory->mutex);
            return;
        }
        struct block_info* bi = getBlockInfo(world_get_block(player->world, packet->location.x, packet->location.y, packet->location.z));
        if (bi == NULL) {
            pthread_mutex_unlock(&player->inventory->mutex);
            return;
        }
        float hard = bi->hardness;
        if (hard > 0. && player->gamemode == 0) {
            player->digging = 0.;
            memcpy(&player->digging_position, &packet->location, sizeof(struct encpos));
            BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, CHUNK_VIEW_DISTANCE * 16.){
                struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_BLOCKBREAKANIMATION);
                pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
                memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
                pkt->data.play_client.blockbreakanimation.destroy_stage = 0;
                queue_push(bc_player->outgoing_packets, pkt);
                END_BROADCAST(player->world->players)
            }
        } else if (hard == 0. || player->gamemode == 1) {
            if (player->gamemode == 1) {
                struct slot* slot = inventory_get(player, player->inventory, 36 + player->currentItem);
                if (slot != NULL && (slot->item == ITM_SWORDWOOD || slot->item == ITM_SWORDGOLD || slot->item == ITM_SWORDSTONE || slot->item == ITM_SWORDIRON || slot->item == ITM_SWORDDIAMOND)) {
                    pthread_mutex_unlock(&player->inventory->mutex);
                    return;
                }
            }
            block blk = world_get_block(player->world, packet->location.x, packet->location.y, packet->location.z);
            int nb = 0;
            if (ii != NULL && ii->onItemBreakBlock != NULL) {
                if ((*ii->onItemBreakBlock)(player->world, player, player->currentItem + 36, ci, player->digging_position.x, player->digging_position.y, player->digging_position.z)) {
                    world_set_block(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
                    nb = 1;
                }
            }
            if (!nb) {
                if (world_set_block(player->world, 0, packet->location.x, packet->location.y, packet->location.z)) {
                    world_set_block(player->world, blk, packet->location.x, packet->location.y, packet->location.z);
                } else {
                    player->foodExhaustion += 0.005;
                    if (player->gamemode != 1) dropBlockDrops(player->world, blk, player, packet->location.x, packet->location.y, packet->location.z);
                }
            }
            player->digging = -1.;
            memset(&player->digging_position, 0, sizeof(struct encpos));
        }
        pthread_mutex_unlock(&player->inventory->mutex);
    } else if (packet->status == 1 || packet->status == 2) {
        if (entity_dist_block(player->entity, packet->location.x, packet->location.y, packet->location.z) > player->reachDistance || player->digging <= 0.) {
            block blk = world_get_block(player->world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
            world_set_block(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
            pthread_mutex_unlock(&player->inventory->mutex);
            return;
        }
        if (packet->status == 2) {
            block blk = world_get_block(player->world, player->digging_position.x, player->digging_position.y, player->digging_position.z);
            if ((player->digging + player->digspeed) >= 1.) {
                int nb = 0;
                if (ii != NULL && ii->onItemBreakBlock != NULL) {
                    if ((*ii->onItemBreakBlock)(player->world, player, player->currentItem + 36, ci, player->digging_position.x, player->digging_position.y, player->digging_position.z)) {
                        world_set_block(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
                        nb = 1;
                    }
                }
                if (!nb) {
                    if (world_set_block(player->world, 0, player->digging_position.x, player->digging_position.y, player->digging_position.z)) {
                        world_set_block(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
                    } else {
                        player->foodExhaustion += 0.005;
                        if (player->gamemode != 1) dropBlockDrops(player->world, blk, player, player->digging_position.x, player->digging_position.y, player->digging_position.z);
                    }
                }
            } else {
                world_set_block(player->world, blk, player->digging_position.x, player->digging_position.y, player->digging_position.z);
            }
        }
        player->digging = -1.;
        pthread_mutex_unlock(&player->inventory->mutex);
        BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, CHUNK_VIEW_DISTANCE * 16.){
            struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_BLOCKBREAKANIMATION);
            pkt->data.play_client.blockbreakanimation.entity_id = player->entity->id;
            memcpy(&pkt->data.play_server.playerdigging.location, &player->digging_position, sizeof(struct encpos));
            memset(&player->digging_position, 0, sizeof(struct encpos));
            pkt->data.play_client.blockbreakanimation.destroy_stage = -1;
            queue_push(bc_player->outgoing_packets, pkt);
            END_BROADCAST(player->world->players)
        }
    } else if (packet->status == 3) {
        if (player->open_inventory != NULL) {
            pthread_mutex_unlock(&player->inventory->mutex);
            return;
        }
        struct slot* current_item = inventory_get(player, player->inventory, 36 + player->currentItem);
        if (current_item != NULL) {
            dropPlayerItem(player, current_item);
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, NULL, 1);
        }
        pthread_mutex_unlock(&player->inventory->mutex);
    } else if (packet->status == 4) {
        if (player->open_inventory != NULL) {
            pthread_mutex_unlock(&player->inventory->mutex);
            return;
        }
        struct slot* current_item = inventory_get(player, player->inventory, 36 + player->currentItem);
        if (current_item != NULL) {
            uint8_t ss = current_item->count;
            current_item->count = 1;
            dropPlayerItem(player, current_item);
            current_item->count = ss;
            if (--current_item->count == 0) {
                current_item = NULL;
            }
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, current_item, 1);
        }
        pthread_mutex_unlock(&player->inventory->mutex);
    } else if (packet->status == 5) {
        if (player->itemUseDuration > 0) {
            struct slot* item_in_hand = inventory_get(player, player->inventory, player->itemUseHand ? 45 : (36 + player->currentItem));
            struct item_info* item_info = item_in_hand == NULL ? NULL : getItemInfo(item_in_hand->item);
            if (item_in_hand == NULL || item_info == NULL) {
                player->itemUseDuration = 0;
                player->entity->usingItemMain = 0;
                player->entity->usingItemOff = 0;
                entity_broadcast_metadata(player->entity);
            }
            if (item_info != NULL && item_info->onItemUse != NULL) (*item_info->onItemUse)(player->world, player, player->itemUseHand ? 45 : (36 + player->currentItem), item_in_hand, player->itemUseDuration);
            player->entity->usingItemMain = 0;
            player->entity->usingItemOff = 0;
            player->itemUseDuration = 0;
            entity_broadcast_metadata(player->entity);
        }
        pthread_mutex_unlock(&player->inventory->mutex);
    } else if (packet->status == 6) {
        inventory_swap(player, player->inventory, 45, 36 + player->currentItem, 1);
        pthread_mutex_unlock(&player->inventory->mutex);
    } else pthread_mutex_unlock(&player->inventory->mutex);
}

void player_packet_handle_entityaction(struct player* player, struct mempool* pool, struct pkt_play_server_entityaction* packet) {
    if (packet->action_id == 0) player->entity->sneaking = 1;
    else if (packet->action_id == 1) player->entity->sneaking = 0;
    else if (packet->action_id == 3) player->entity->sprinting = 1;
    else if (packet->action_id == 4) player->entity->sprinting = 0;
    entity_broadcast_metadata(player->entity);
}

void player_packet_handle_steervehicle(struct player* player, struct mempool* pool, struct pkt_play_server_steervehicle* packet) {
    
}

void player_packet_handle_resourcepackstatus(struct player* player, struct mempool* pool, struct pkt_play_server_resourcepackstatus* packet) {
    
}

void player_packet_handle_helditemchange(struct player* player, struct mempool* pool, struct pkt_play_server_helditemchange* packet) {
    if (packet->slot < 0 || packet->slot >= 9) {
        return;
    }
    pthread_mutex_lock(&player->inventory->mutex);
    player->currentItem = (uint16_t) packet->slot;
    game_update_inventory(player, player->inventory, player->currentItem + 36);
    pthread_mutex_unlock(&player->inventory->mutex);
}

void player_packet_handle_creativeinventoryaction(struct player* player, struct mempool* pool, struct pkt_play_server_creativeinventoryaction* packet) {
    if (player->gamemode != 1) {
        return;
    }
    struct inventory* inventory = NULL;
    if (player->open_inventory == NULL) inventory = player->inventory;
    else if (player->open_inventory != NULL) inventory = player->open_inventory;
    if (inventory == NULL) {
        return;
    }
    pthread_mutex_lock(&inventory->mutex);
    if (packet->clicked_item.item >= 0 && packet->slot == -1) {
        dropPlayerItem(player, &packet->clicked_item);
    } else {
        inventory_set_slot(player, inventory, packet->slot, &packet->clicked_item, 1);
    }
    pthread_mutex_unlock(&inventory->mutex);
}

void player_packet_handle_updatesign(struct player* player, struct mempool* pool, struct pkt_play_server_updatesign* packet) {
    
}

void player_packet_handle_animation(struct player* player, struct mempool* pool, struct pkt_play_server_animation* packet) {
    player->lastSwing = player->world->tick_counter;
    BEGIN_BROADCAST_EXCEPT_DIST(player, player->entity, CHUNK_VIEW_DISTANCE * 16.){
        struct packet* pkt = packet_new(mempool_new(), PKT_PLAY_CLIENT_ANIMATION);
        pkt->data.play_client.animation.entity_id = player->entity->id;
        pkt->data.play_client.animation.animation = (uint8_t) (packet->hand == 0 ? 0 : 3);
        queue_push(bc_player->outgoing_packets, pkt);
        END_BROADCAST(player->entity->world->players)
    }
}

void player_packet_handle_spectate(struct player* player, struct mempool* pool, struct pkt_play_server_spectate* packet) {
    
}

void player_packet_handle_playerblockplacement(struct player* player, struct mempool* pool, struct pkt_play_server_playerblockplacement* packet) {
    if (player->open_inventory != NULL) {
        return;
    }
    if (entity_dist_block(player->entity, packet->location.x, packet->location.y, packet->location.z) > player->reachDistance) {
        return;
    }
    pthread_mutex_lock(&player->inventory->mutex);
    struct slot* current_item = inventory_get(player, player->inventory, 36 + player->currentItem);
    int32_t x = packet->location.x;
    int32_t y = packet->location.y;
    int32_t z = packet->location.z;
    uint8_t face = packet->face;
    block placed_at = world_get_block(player->world, x, y, z);
    if (current_item != NULL) {
        struct item_info* ii = getItemInfo(current_item->item);
        if (ii != NULL && ii->onItemInteract != NULL) {
            pthread_mutex_unlock(&player->inventory->mutex);
            if ((*ii->onItemInteract)(player->world, player, player->currentItem + 36, current_item, x, y, z, face, packet->cursor_position_x, packet->cursor_position_y, packet->cursor_position_z)) {
                return;
            }
            pthread_mutex_lock(&player->inventory->mutex);
            current_item = inventory_get(player, player->inventory, 36 + player->currentItem);
        }
    }
    struct block_info* bi = getBlockInfo(placed_at);
    if (!player->entity->sneaking && bi != NULL && bi->onBlockInteract != NULL) {
        pthread_mutex_unlock(&player->inventory->mutex);
        (*bi->onBlockInteract)(player->world, placed_at, x, y, z, player, face, packet->cursor_position_x, packet->cursor_position_y, packet->cursor_position_z);
        pthread_mutex_lock(&player->inventory->mutex);
        current_item = inventory_get(player, player->inventory, 36 + player->currentItem);
    } else if (current_item != NULL && current_item->item < 256) {
        if (bi == NULL || !bi->material->replacable) {
            if (face == 0) y--;
            else if (face == 1) y++;
            else if (face == 2) z--;
            else if (face == 3) z++;
            else if (face == 4) x--;
            else if (face == 5) x++;
        }
        block b2 = world_get_block(player->world, x, y, z);
        struct block_info* bi2 = getBlockInfo(b2);
        if (b2 == 0 || bi2 == NULL || bi2->material->replacable) {
            block to_place = ((uint16_t) current_item->item << 4) | ((uint8_t) current_item->damage & 0x0fu);
            struct block_info* tbi = getBlockInfo(to_place);
            if (tbi == NULL) {
                pthread_mutex_unlock(&player->inventory->mutex);
                return;
            }
            struct boundingbox entity_bounds;
            int bad = 0;
            pthread_rwlock_rdlock(&player->world->entities->rwlock);
            // TODO: localize entity information!
            ITER_MAP (player->world->entities) {
                struct entity* ent = (struct entity*) value;
                if (ent == NULL || entity_distsq_block(ent, x, y, z) > 8. * 8. || !entity_has_flag(entity_get_info(ent->type), "livingbase")) continue;
                            entity_collision_bounding_box(ent, &entity_bounds);
                entity_bounds.minX += .01;
                entity_bounds.minY += .01;
                entity_bounds.minZ += .01;
                entity_bounds.maxX -= .001;
                entity_bounds.maxY -= .001;
                entity_bounds.maxZ -= .001;
                for (size_t i = 0; i < tbi->boundingBox_count; i++) {
                    struct boundingbox* bb = &tbi->boundingBoxes[i];
                    if (bb == NULL) continue;
                    struct boundingbox block_bounds;
                    block_bounds.minX = bb->minX + x;
                    block_bounds.maxX = bb->maxX + x;
                    block_bounds.minY = bb->minY + y;
                    block_bounds.maxY = bb->maxY + y;
                    block_bounds.minZ = bb->minZ + z;
                    block_bounds.maxZ = bb->maxZ + z;
                    if (boundingbox_intersects(&block_bounds, &entity_bounds)) {
                        bad = 1;
                        goto post_iter_entities;
                    }
                }
                ITER_MAP_END()
            }
            post_iter_entities:;
            pthread_rwlock_unlock(&player->world->entities->rwlock);
            if (!bad) {
                if ((to_place = player_can_place_block(player, to_place, x, y, z, face))) {
                    if (world_set_block(player->world, to_place, x, y, z)) {
                        world_set_block(player->world, b2, x, y, z);
                        inventory_set_slot(player, player->inventory, 36 + player->currentItem, current_item, 1);
                    } else if (player->gamemode != 1) {
                        if (--current_item->count <= 0) {
                            current_item = NULL;
                        }
                        inventory_set_slot(player, player->inventory, 36 + player->currentItem, current_item, 1);
                    }
                } else {
                    world_set_block(player->world, b2, x, y, z);
                    inventory_set_slot(player, player->inventory, 36 + player->currentItem, current_item, 1);
                }
            } else {
                world_set_block(player->world, b2, x, y, z);
                inventory_set_slot(player, player->inventory, 36 + player->currentItem, current_item, 1);
            }
        } else {
            world_set_block(player->world, b2, x, y, z);
            inventory_set_slot(player, player->inventory, 36 + player->currentItem, current_item, 1);
        }
    }
    pthread_mutex_unlock(&player->inventory->mutex);
}

void player_packet_handle_useitem(struct player* player, struct mempool* pool, struct pkt_play_server_useitem* packet) {
    pthread_mutex_lock(&player->inventory->mutex);
    struct slot* cs = inventory_get(player, player->inventory, packet->hand ? 45 : (36 + player->currentItem));
    if (cs != NULL) {
        struct item_info* ii = getItemInfo(cs->item);
        if (ii != NULL && (ii->canUseItem == NULL || (*ii->canUseItem)(player->world, player, (uint8_t) (packet->hand ? 45 : (36 + player->currentItem)), cs))) {
            if (ii->maxUseDuration == 0 && ii->onItemUse != NULL) (*ii->onItemUse)(player->world, player, (uint8_t) (packet->hand ? 45 : (36 + player->currentItem)), cs, 0);
            else if (ii->maxUseDuration > 0) {
                player->itemUseDuration = 1;
                player->itemUseHand = (uint8_t) (packet->hand ? 1 : 0);
                player->entity->usingItemMain = !player->itemUseHand;
                player->entity->usingItemOff = player->itemUseHand;
                entity_broadcast_metadata(player->entity);
            }
        }
    }
    pthread_mutex_unlock(&player->inventory->mutex);
}




void player_receive_packet(struct player* player, struct packet* inp) {
    // TODO: implement health check
    INTENTIONAL SYNTAX ERROR
    if (player->entity->health <= 0. && inp->id != PKT_PLAY_SERVER_KEEPALIVE && inp->id != PKT_PLAY_SERVER_CLIENTSTATUS) goto cont;
}
