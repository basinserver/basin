/*
 * block.c
 *
 *  Created on: Dec 24, 2016
 *      Author: root
 */

#include "util.h"
#include "inventory.h"
#include "globals.h"
#include "network.h"
#include "packet.h"
#include "world.h"
#include "game.h"
#include "xstring.h"
#include "queue.h"
#include "smelting.h"
#include "tileentity.h"
#include "smelting.h"
#include <fcntl.h>
#include <errno.h>
#include "json.h"
#include "item.h"
#include "block.h"
#include <unistd.h>

struct collection* block_infos;
struct collection* block_materials;

void onBlockInteract_workbench(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
	struct inventory* wb = xmalloc(sizeof(struct inventory));
	newInventory(wb, INVTYPE_WORKBENCH, 1, 10);
	wb->title = xstrdup("{\"text\": \"Crafting Table\"}", 0);
	player_openInventory(player, wb);
}

block onBlockPlaced_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	int16_t meta = blk & 0x0f;
	if (meta < 2 || meta > 5) meta = 2;
	struct tile_entity* te = newTileEntity("minecraft:chest", x, y, z);
	te->data.chest.lock = NULL;
	te->data.chest.inv = xmalloc(sizeof(struct inventory));
	newInventory(te->data.chest.inv, INVTYPE_CHEST, 2, 27);
	te->data.chest.inv->te = te;
	te->data.chest.inv->title = xstrdup("{\"text\": \"Chest\"}", 0);
	setTileEntityWorld(world, x, y, z, te);
	return (blk & ~0x0f) | meta;
}

block onBlockPlaced_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	int16_t meta = blk & 0x0f;
	if (meta < 2 || meta > 5) meta = 2;
	struct tile_entity* te = newTileEntity("minecraft:furnace", x, y, z);
	te->data.furnace.lock = NULL;
	te->data.furnace.inv = xmalloc(sizeof(struct inventory));
	newInventory(te->data.furnace.inv, INVTYPE_FURNACE, 3, 3);
	te->data.furnace.inv->te = te;
	te->data.furnace.inv->title = xstrdup("{\"text\": \"Furnace\"}", 0);
	setTileEntityWorld(world, x, y, z, te);
	return (blk & ~0x0f) | meta;
}

void onBlockDestroyed_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	struct tile_entity* te = getTileEntityWorld(world, x, y, z);
	if (te == NULL) return;
	for (size_t i = 0; i < te->data.chest.inv->slot_count; i++) {
		struct slot* sl = getSlot(NULL, te->data.chest.inv, i);
		dropBlockDrop(world, sl, x, y, z);
	}
	setTileEntityWorld(world, x, y, z, NULL);
}

void onBlockInteract_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
	struct tile_entity* te = getTileEntityWorld(world, x, y, z);
	if (te == NULL || !streq_nocase(te->id, "minecraft:chest")) {
		onBlockPlaced_chest(world, blk, x, y, z);
		te = getTileEntityWorld(world, x, y, z);
	}
	//TODO: impl locks, loot
	player_openInventory(player, te->data.chest.inv);
	BEGIN_BROADCAST_DIST(player->entity, 128.)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_BLOCKACTION;
	pkt->data.play_client.blockaction.location.x = x;
	pkt->data.play_client.blockaction.location.y = y;
	pkt->data.play_client.blockaction.location.z = z;
	pkt->data.play_client.blockaction.action_id = 1;
	pkt->data.play_client.blockaction.action_param = te->data.chest.inv->players->count;
	pkt->data.play_client.blockaction.block_type = blk >> 4;
	add_queue(bc_player->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST
}

void onBlockDestroyed_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	struct tile_entity* te = getTileEntityWorld(world, x, y, z);
	if (te == NULL) return;
	for (size_t i = 0; i < te->data.furnace.inv->slot_count; i++) {
		struct slot* sl = getSlot(NULL, te->data.furnace.inv, i);
		dropBlockDrop(world, sl, x, y, z);
	}
	setTileEntityWorld(world, x, y, z, NULL);
}

void onBlockInteract_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ) {
	struct tile_entity* te = getTileEntityWorld(world, x, y, z);
	if (te == NULL || !streq_nocase(te->id, "minecraft:furnace")) {
		onBlockPlaced_furnace(world, blk, x, y, z);
		te = getTileEntityWorld(world, x, y, z);
	}
	//TODO: impl locks, loot
	player_openInventory(player, te->data.furnace.inv);
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
	pkt->data.play_client.windowproperty.window_id = te->data.furnace.inv->windowID;
	pkt->data.play_client.windowproperty.property = 1;
	pkt->data.play_client.windowproperty.value = te->data.furnace.lastBurnMax;
	add_queue(player->outgoingPacket, pkt);
	flush_outgoing(player);
	pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
	pkt->data.play_client.windowproperty.window_id = te->data.furnace.inv->windowID;
	pkt->data.play_client.windowproperty.property = 3;
	pkt->data.play_client.windowproperty.value = 200;
	add_queue(player->outgoingPacket, pkt);
	flush_outgoing(player);
}

void update_furnace(struct world* world, struct tile_entity* te) {
	struct slot* si = getSlot(NULL, te->data.furnace.inv, 0);
	struct slot* fu = getSlot(NULL, te->data.furnace.inv, 1);
	struct slot* aso = getSlot(NULL, te->data.furnace.inv, 2);
	struct slot* so = getSmeltingOutput(si);
	int16_t st = getSmeltingFuelBurnTime(fu);
	int burning = 0;
	if (so != NULL && ((itemsStackable(so, aso) && (aso->itemCount + so->itemCount) <= maxStackSize(so)) || aso == NULL) && (te->data.furnace.burnTime > 0 || st > 0)) {
		burning = 1;
		if (!te->tick) {
			te->tick = &tetick_furnace;
			enableTileEntityTickable(world, te);
			setBlockWorld(world, BLK_FURNACE_LIT | (getBlockWorld(world, te->x, te->y, te->z) & 0x0f), te->x, te->y, te->z);
		}
		//printf("bt = %i\n", te->data.furnace.burnTime);
		if (te->data.furnace.burnTime <= 0 && st > 0 && fu != NULL) {
			te->data.furnace.burnTime += st + 1;
			if (--fu->itemCount == 0) fu = NULL;
			setSlot(NULL, te->data.furnace.inv, 1, fu, 1, 1);
			te->data.furnace.lastBurnMax = st;
			BEGIN_BROADCAST(te->data.furnace.inv->players)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
			pkt->data.play_client.windowproperty.window_id = te->data.furnace.inv->windowID;
			pkt->data.play_client.windowproperty.property = 1;
			pkt->data.play_client.windowproperty.value = te->data.furnace.lastBurnMax;
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			END_BROADCAST
		}
		if (te->data.furnace.cookTime <= 0) {
			//printf("start cookin\n");
			te->data.furnace.cookTime++;
		} else if (te->data.furnace.cookTime < 200) {
			//printf("cookin %i/%i\n", te->data.furnace.cookTime, 200);
			te->data.furnace.cookTime++;
		} else if (te->data.furnace.cookTime == 200) {
			te->data.furnace.cookTime = 0;
			//printf("donecookin\n");
			if (aso == NULL) setSlot(NULL, te->data.furnace.inv, 2, so, 1, 1);
			else {
				aso->itemCount++;
				setSlot(NULL, te->data.furnace.inv, 2, aso, 1, 1);
			}
			if (--si->itemCount == 0) si = NULL;
			setSlot(NULL, te->data.furnace.inv, 0, si, 1, 1);
			te->data.furnace.burnTime++; // freebie for accounting
		}
	} else te->data.furnace.cookTime = 0;
	if (te->data.furnace.burnTime > 0) {
		te->data.furnace.burnTime--;
	} else {
		if (te->tick) {
			BEGIN_BROADCAST(te->data.furnace.inv->players)
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
			pkt->data.play_client.windowproperty.window_id = te->data.furnace.inv->windowID;
			pkt->data.play_client.windowproperty.property = 0;
			pkt->data.play_client.windowproperty.value = 0;
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing (bc_player);
			pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
			pkt->data.play_client.windowproperty.window_id = te->data.furnace.inv->windowID;
			pkt->data.play_client.windowproperty.property = 2;
			pkt->data.play_client.windowproperty.value = 0;
			add_queue(bc_player->outgoingPacket, pkt);
			flush_outgoing(bc_player);
			END_BROADCAST disableTileEntityTickable(world, te);
			te->tick = NULL;
			setBlockWorld(world, BLK_FURNACE | (getBlockWorld(world, te->x, te->y, te->z) & 0x0f), te->x, te->y, te->z);
		}
		te->data.furnace.cookTime = 0;
	}
	if (burning && tick_counter % 5 == 0) {
		BEGIN_BROADCAST(te->data.furnace.inv->players)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
		pkt->data.play_client.windowproperty.window_id = te->data.furnace.inv->windowID;
		pkt->data.play_client.windowproperty.property = 0;
		pkt->data.play_client.windowproperty.value = te->data.furnace.burnTime;
		add_queue(bc_player->outgoingPacket, pkt);
		flush_outgoing (bc_player);
		pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_WINDOWPROPERTY;
		pkt->data.play_client.windowproperty.window_id = te->data.furnace.inv->windowID;
		pkt->data.play_client.windowproperty.property = 2;
		pkt->data.play_client.windowproperty.value = te->data.furnace.cookTime;
		add_queue(bc_player->outgoingPacket, pkt);
		flush_outgoing(bc_player);
		END_BROADCAST
	}
}

void dropItems_gravel(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
	if (fortune > 3) fortune = 3;
	if (fortune == 3 || (rand() % (10 - fortune * 3)) == 0) {
		struct slot drop;
		drop.item = ITM_FLINT;
		drop.damage = 0;
		drop.itemCount = 1;
		drop.nbt = NULL;
		dropBlockDrop(world, &drop, x, y, z);
	} else {
		struct slot drop;
		drop.item = BLK_GRAVEL >> 4;
		drop.damage = BLK_GRAVEL & 0x0f;
		drop.itemCount = 1;
		drop.nbt = NULL;
		dropBlockDrop(world, &drop, x, y, z);
	}
}

void dropItems_leaves(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
	int chance = 20;
	if (fortune > 0) chance -= 2 << fortune;
	if (chance < 10) chance = 10;
	if (rand() % chance == 0) {
		struct slot drop;
		uint8_t m = blk & 0x0f;
		if (blk >> 4 == BLK_LEAVES_OAK >> 4) {
			if ((m & 0x03) == 0) {
				drop.item = BLK_SAPLING_OAK >> 4;
				drop.damage = 0;
			} else if ((m & 0x03) == 1) {
				drop.item = BLK_SAPLING_SPRUCE >> 4;
				drop.damage = 1;
			} else if ((m & 0x03) == 2) {
				drop.item = BLK_SAPLING_BIRCH >> 4;
				drop.damage = 2;
			} else if ((m & 0x03) == 3) {
				drop.item = BLK_SAPLING_JUNGLE >> 4;
				drop.damage = 3;
			}
		} else if (blk >> 4 == BLK_LEAVES_ACACIA >> 4) {
			if ((m & 0x03) == 0) {
				drop.item = BLK_SAPLING_ACACIA >> 4;
				drop.damage = 4;
			} else if ((m & 0x03) == 1) {
				drop.item = BLK_SAPLING_BIG_OAK >> 4;
				drop.damage = 5;
			} else return;
		} else return;
		drop.itemCount = 1;
		drop.nbt = NULL;
		dropBlockDrop(world, &drop, x, y, z);
	}
	if (blk == BLK_LEAVES_OAK || blk == BLK_LEAVES_BIG_OAK) {
		chance = 200;
		if (fortune > 0) {
			chance -= 10 << fortune;
			if (chance < 40) chance = 40;
		}
		if (rand() % chance == 0) {
			struct slot drop;
			drop.item = ITM_APPLE;
			drop.damage = 0;
			drop.itemCount = 1;
			drop.nbt = NULL;
			dropBlockDrop(world, &drop, x, y, z);
		}
	}
}

void dropItems_tallgrass(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
	if (rand() % 8 == 0) {
		struct slot drop;
		drop.item = ITM_SEEDS;
		drop.damage = 0;
		drop.itemCount = 1 + (rand() % (fortune * 2 + 1));
		drop.nbt = NULL;
		dropBlockDrop(world, &drop, x, y, z);
	}
}

void dropItems_hugemushroom(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune) {
	int ct = rand() % 10 - 7;
	if (ct > 0) {
		struct slot drop;
		drop.item = ((blk >> 4) == (BLK_MUSHROOM_2 >> 4) ? 39 : 40);
		drop.damage = 0;
		drop.itemCount = ct;
		drop.nbt = NULL;
		dropBlockDrop(world, &drop, x, y, z);
	}
}

//

struct block_info* getBlockInfo(block b) {
	if (b >= block_infos->size) {
		b &= ~0x0f;
		if (b >= block_infos->size) return NULL;
		else return block_infos->data[b];
	}
	if (block_infos->data[b] != NULL) return block_infos->data[b];
	return block_infos->data[b & ~0x0f];
}

// index = item ID
const char* nameToIDMap[] = { "minecraft:air", "minecraft:stone", "minecraft:grass", "minecraft:dirt", "minecraft:cobblestone", "minecraft:planks", "minecraft:sapling", "minecraft:bedrock", "", "", "", "", "minecraft:sand", "minecraft:gravel", "minecraft:gold_ore", "minecraft:iron_ore", "minecraft:coal_ore", "minecraft:log", "minecraft:leaves", "minecraft:sponge", "minecraft:glass", "minecraft:lapis_ore", "minecraft:lapis_block", "minecraft:dispenser", "minecraft:sandstone", "minecraft:noteblock", "", "minecraft:golden_rail", "minecraft:detector_rail", "minecraft:sticky_piston", "minecraft:web", "minecraft:tallgrass", "minecraft:deadbush", "minecraft:piston", "", "minecraft:wool", "", "minecraft:yellow_flower", "minecraft:red_flower", "minecraft:brown_mushroom", "minecraft:red_mushroom", "minecraft:gold_block", "minecraft:iron_block", "", "minecraft:stone_slab", "minecraft:brick_block", "minecraft:tnt", "minecraft:bookshelf", "minecraft:mossy_cobblestone", "minecraft:obsidian",
		"minecraft:torch", "", "minecraft:mob_spawner", "minecraft:oak_stairs", "minecraft:chest", "", "minecraft:diamond_ore", "minecraft:diamond_block", "minecraft:crafting_table", "", "minecraft:farmland", "minecraft:furnace", "", "", "", "minecraft:ladder", "minecraft:rail", "minecraft:stone_stairs", "", "minecraft:lever", "minecraft:stone_pressure_plate", "", "minecraft:wooden_pressure_plate", "minecraft:redstone_ore", "", "", "minecraft:redstone_torch", "minecraft:stone_button", "minecraft:snow_layer", "minecraft:ice", "minecraft:snow", "minecraft:cactus", "minecraft:clay", "", "minecraft:jukebox", "minecraft:fence", "minecraft:pumpkin", "minecraft:netherrack", "minecraft:soul_sand", "minecraft:glowstone", "", "minecraft:lit_pumpkin", "", "", "", "minecraft:stained_glass", "minecraft:trapdoor", "minecraft:monster_egg", "minecraft:stonebrick", "minecraft:brown_mushroom_block", "minecraft:red_mushroom_block", "minecraft:iron_bars", "minecraft:glass_pane", "minecraft:melon_block",
		"", "", "minecraft:vine", "minecraft:fence_gate", "minecraft:brick_stairs", "minecraft:stone_brick_stairs", "minecraft:mycelium", "minecraft:waterlily", "minecraft:nether_brick", "minecraft:nether_brick_fence", "minecraft:nether_brick_stairs", "", "minecraft:enchanting_table", "", "", "", "minecraft:end_portal_frame", "minecraft:end_stone", "minecraft:dragon_egg", "minecraft:redstone_lamp", "", "", "minecraft:wooden_slab", "", "minecraft:sandstone_stairs", "minecraft:emerald_ore", "minecraft:ender_chest", "minecraft:tripwire_hook", "", "minecraft:emerald_block", "minecraft:spruce_stairs", "minecraft:birch_stairs", "minecraft:jungle_stairs", "minecraft:command_block", "minecraft:beacon", "minecraft:cobblestone_wall", "", "", "", "minecraft:wooden_button", "", "minecraft:anvil", "minecraft:trapped_chest", "minecraft:light_weighted_pressure_plate", "minecraft:heavy_weighted_pressure_plate", "", "", "minecraft:daylight_detector", "minecraft:redstone_block", "minecraft:quartz_ore",
		"minecraft:hopper", "minecraft:quartz_block", "minecraft:quartz_stairs", "minecraft:activator_rail", "minecraft:dropper", "minecraft:stained_hardened_clay", "minecraft:stained_glass_pane", "minecraft:leaves2", "minecraft:log2", "minecraft:acacia_stairs", "minecraft:dark_oak_stairs", "minecraft:slime", "minecraft:barrier", "minecraft:iron_trapdoor", "minecraft:prismarine", "minecraft:sea_lantern", "minecraft:hay_block", "minecraft:carpet", "minecraft:hardened_clay", "minecraft:coal_block", "minecraft:packed_ice", "minecraft:double_plant", "", "", "", "minecraft:red_sandstone", "minecraft:red_sandstone_stairs", "", "minecraft:stone_slab2", "minecraft:spruce_fence_gate", "minecraft:birch_fence_gate", "minecraft:jungle_fence_gate", "minecraft:dark_oak_fence_gate", "minecraft:acacia_fence_gate", "minecraft:spruce_fence", "minecraft:birch_fence", "minecraft:jungle_fence", "minecraft:dark_oak_fence", "minecraft:acacia_fence", "", "", "", "", "", "minecraft:end_rod",
		"minecraft:chorus_plant", "minecraft:chorus_flower", "minecraft:purpur_block", "minecraft:purpur_pillar", "minecraft:purpur_stairs", "", "minecraft:purpur_slab", "minecraft:end_bricks", "", "minecraft:grass_path", "", "minecraft:repeating_command_block", "minecraft:chain_command_block", "", "minecraft:magma", "minecraft:nether_wart_block", "minecraft:red_nether_brick", "minecraft:bone_block", "minecraft:structure_void", "minecraft:observer", "minecraft:white_shulker_box", "minecraft:orange_shulker_box", "minecraft:magenta_shulker_box", "minecraft:light_blue_shulker_box", "minecraft:yellow_shulker_box", "minecraft:lime_shulker_box", "minecraft:pink_shulker_box", "minecraft:gray_shulker_box", "minecraft:silver_shulker_box", "minecraft:cyan_shulker_box", "minecraft:purple_shulker_box", "minecraft:blue_shulker_box", "minecraft:brown_shulker_box", "minecraft:green_shulker_box", "minecraft:red_shulker_box", "minecraft:black_shulker_box", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "minecraft:structure_block", "minecraft:iron_shovel", "minecraft:iron_pickaxe", "minecraft:iron_axe", "minecraft:flint_and_steel", "minecraft:apple", "minecraft:bow", "minecraft:arrow", "minecraft:coal", "minecraft:diamond", "minecraft:iron_ingot", "minecraft:gold_ingot", "minecraft:iron_sword", "minecraft:wooden_sword", "minecraft:wooden_shovel", "minecraft:wooden_pickaxe", "minecraft:wooden_axe", "minecraft:stone_sword", "minecraft:stone_shovel", "minecraft:stone_pickaxe", "minecraft:stone_axe", "minecraft:diamond_sword", "minecraft:diamond_shovel", "minecraft:diamond_pickaxe", "minecraft:diamond_axe", "minecraft:stick", "minecraft:bowl", "minecraft:mushroom_stew", "minecraft:golden_sword", "minecraft:golden_shovel", "minecraft:golden_pickaxe", "minecraft:golden_axe", "minecraft:string", "minecraft:feather", "minecraft:gunpowder", "minecraft:wooden_hoe", "minecraft:stone_hoe", "minecraft:iron_hoe", "minecraft:diamond_hoe",
		"minecraft:golden_hoe", "minecraft:wheat_seeds", "minecraft:wheat", "minecraft:bread", "minecraft:leather_helmet", "minecraft:leather_chestplate", "minecraft:leather_leggings", "minecraft:leather_boots", "minecraft:chainmail_helmet", "minecraft:chainmail_chestplate", "minecraft:chainmail_leggings", "minecraft:chainmail_boots", "minecraft:iron_helmet", "minecraft:iron_chestplate", "minecraft:iron_leggings", "minecraft:iron_boots", "minecraft:diamond_helmet", "minecraft:diamond_chestplate", "minecraft:diamond_leggings", "minecraft:diamond_boots", "minecraft:golden_helmet", "minecraft:golden_chestplate", "minecraft:golden_leggings", "minecraft:golden_boots", "minecraft:flint", "minecraft:porkchop", "minecraft:cooked_porkchop", "minecraft:painting", "minecraft:golden_apple", "minecraft:sign", "minecraft:wooden_door", "minecraft:bucket", "minecraft:water_bucket", "minecraft:lava_bucket", "minecraft:minecart", "minecraft:saddle", "minecraft:iron_door", "minecraft:redstone",
		"minecraft:snowball", "minecraft:boat", "minecraft:leather", "minecraft:milk_bucket", "minecraft:brick", "minecraft:clay_ball", "minecraft:reeds", "minecraft:paper", "minecraft:book", "minecraft:slime_ball", "minecraft:chest_minecart", "minecraft:furnace_minecart", "minecraft:egg", "minecraft:compass", "minecraft:fishing_rod", "minecraft:clock", "minecraft:glowstone_dust", "minecraft:fish", "minecraft:cooked_fish", "minecraft:dye", "minecraft:bone", "minecraft:sugar", "minecraft:cake", "minecraft:bed", "minecraft:repeater", "minecraft:cookie", "minecraft:filled_map", "minecraft:shears", "minecraft:melon", "minecraft:pumpkin_seeds", "minecraft:melon_seeds", "minecraft:beef", "minecraft:cooked_beef", "minecraft:chicken", "minecraft:cooked_chicken", "minecraft:rotten_flesh", "minecraft:ender_pearl", "minecraft:blaze_rod", "minecraft:ghast_tear", "minecraft:gold_nugget", "minecraft:nether_wart", "minecraft:potion", "minecraft:glass_bottle", "minecraft:spider_eye",
		"minecraft:fermented_spider_eye", "minecraft:blaze_powder", "minecraft:magma_cream", "minecraft:brewing_stand", "minecraft:cauldron", "minecraft:ender_eye", "minecraft:speckled_melon", "minecraft:spawn_egg", "minecraft:experience_bottle", "minecraft:fire_charge", "minecraft:writable_book", "minecraft:written_book", "minecraft:emerald", "minecraft:item_frame", "minecraft:flower_pot", "minecraft:carrot", "minecraft:potato", "minecraft:baked_potato", "minecraft:poisonous_potato", "minecraft:map", "minecraft:golden_carrot", "minecraft:skull", "minecraft:carrot_on_a_stick", "minecraft:nether_star", "minecraft:pumpkin_pie", "minecraft:fireworks", "minecraft:firework_charge", "minecraft:enchanted_book", "minecraft:comparator", "minecraft:netherbrick", "minecraft:quartz", "minecraft:tnt_minecart", "minecraft:hopper_minecart", "minecraft:prismarine_shard", "minecraft:prismarine_crystals", "minecraft:rabbit", "minecraft:cooked_rabbit", "minecraft:rabbit_stew", "minecraft:rabbit_foot",
		"minecraft:rabbit_hide", "minecraft:armor_stand", "minecraft:iron_horse_armor", "minecraft:golden_horse_armor", "minecraft:diamond_horse_armor", "minecraft:lead", "minecraft:name_tag", "minecraft:command_block_minecart", "minecraft:mutton", "minecraft:cooked_mutton", "", "minecraft:end_crystal", "minecraft:spruce_door", "minecraft:birch_door", "minecraft:jungle_door", "minecraft:acacia_door", "minecraft:dark_oak_door", "minecraft:chorus_fruit", "minecraft:chorus_fruit_popped", "minecraft:beetroot", "minecraft:beetroot_seeds", "minecraft:beetroot_soup", "minecraft:dragon_breath", "minecraft:splash_potion", "minecraft:spectral_arrow", "minecraft:tipped_arrow", "minecraft:lingering_potion", "minecraft:shield", "minecraft:elytra", "minecraft:spruce_boat", "minecraft:birch_boat", "minecraft:jungle_boat", "minecraft:acacia_boat", "minecraft:dark_oak_boat", "minecraft:totem", "minecraft:shulker_shell", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "minecraft:record_13", "minecraft:record_cat", "minecraft:record_blocks", "minecraft:record_chirp", "minecraft:record_far", "minecraft:record_mall", "minecraft:record_mellohi", "minecraft:record_stal", "minecraft:record_strad", "minecraft:record_ward", "minecraft:record_11", "minecraft:record_wait" };

item getItemFromName(const char* name) {
	for (size_t i = 0; i < (sizeof(nameToIDMap) / sizeof(char*)); i++) {
		if (streq_nocase(nameToIDMap[i], name)) return i;
	}
	return -1;
}

struct block_material* getBlockMaterial(char* name) {
	for (size_t i = 0; i < block_materials->size; i++) {
		struct block_material* bm = (struct block_material*) block_materials->data[i];
		if (bm == NULL) continue;
		if (streq_nocase(bm->name, name)) return bm;
	}
	return NULL;
}

size_t getBlockSize() {
	return block_infos->size;
}

void add_block_material(struct block_material* bm) {
	add_collection(block_materials, bm);
}

void add_block_info(block blk, struct block_info* bm) {
	ensure_collection(block_infos, blk + 1);
	block_infos->data[blk] = bm;
	if (block_infos->size < blk) block_infos->size = blk;
	block_infos->count++;
}

void init_materials() {
	block_materials = new_collection(64, 0);
	char* jsf = xmalloc(4097);
	size_t jsfs = 4096;
	size_t jsr = 0;
	int fd = open("materials.json", O_RDONLY);
	ssize_t r = 0;
	while ((r = read(fd, jsf + jsr, jsfs - jsr)) > 0) {
		jsr += r;
		if (jsfs - jsr < 512) {
			jsfs += 4096;
			jsf = xrealloc(jsf, jsfs + 1);
		}
	}
	jsf[jsr] = 0;
	if (r < 0) {
		printf("Error reading material information: %s\n", strerror(errno));
	}
	close(fd);
	struct json_object json;
	parseJSON(&json, jsf);
	for (size_t i = 0; i < json.child_count; i++) {
		struct json_object* ur = json.children[i];
		struct block_material* bm = xcalloc(sizeof(struct block_material));
		bm->name = xstrdup(ur->name, 0);
		struct json_object* tmp = getJSONValue(ur, "flammable");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->flammable = tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "replaceable");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->replacable = tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "requiresnotool");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->requiresnotool = tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "mobility");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		bm->mobility = (uint8_t) tmp->data.number;
		tmp = getJSONValue(ur, "adventure_exempt");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->adventure_exempt = tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "liquid");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->liquid = tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "solid");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->solid = tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "blocksLight");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->blocksLight = tmp->type == JSON_TRUE;
		tmp = getJSONValue(ur, "blocksMovement");
		if (tmp == NULL || (tmp->type != JSON_TRUE && tmp->type != JSON_FALSE)) goto cerr;
		bm->blocksMovement = tmp->type == JSON_TRUE;
		add_block_material(bm);
		continue;
		cerr: ;
		printf("[WARNING] Error Loading Material \"%s\"! Skipped.\n", ur->name);
	}
	freeJSON(&json);
	xfree(jsf);
}

void init_blocks() {
	block_infos = new_collection(128, 0);
	char* jsf = xmalloc(4097);
	size_t jsfs = 4096;
	size_t jsr = 0;
	int fd = open("blocks.json", O_RDONLY);
	ssize_t r = 0;
	while ((r = read(fd, jsf + jsr, jsfs - jsr)) > 0) {
		jsr += r;
		if (jsfs - jsr < 512) {
			jsfs += 4096;
			jsf = xrealloc(jsf, jsfs + 1);
		}
	}
	jsf[jsr] = 0;
	if (r < 0) {
		printf("Error reading block information: %s\n", strerror(errno));
	}
	close(fd);
	struct json_object json;
	parseJSON(&json, jsf);
	for (size_t i = 0; i < json.child_count; i++) {
		struct json_object* ur = json.children[i];
		struct block_info* bi = xcalloc(sizeof(struct block_info));
		struct json_object* tmp = getJSONValue(ur, "id");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		block id = (block) tmp->data.number;
		if (id < 0) goto cerr;
		struct json_object* colls = getJSONValue(ur, "collision");
		if (colls == NULL || colls->type != JSON_OBJECT) goto cerr;
		bi->boundingBox_count = colls->child_count;
		bi->boundingBoxes = colls->child_count == 0 ? NULL : xmalloc(sizeof(struct boundingbox) * colls->child_count);
		for (size_t x = 0; x < colls->child_count; x++) {
			struct json_object* coll = colls->children[x];
			struct boundingbox* bb = &bi->boundingBoxes[x];
			tmp = getJSONValue(coll, "minX");
			if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
			bb->minX = (double) tmp->data.number;
			tmp = getJSONValue(coll, "maxX");
			if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
			bb->maxX = (double) tmp->data.number;
			tmp = getJSONValue(coll, "minY");
			if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
			bb->minY = (double) tmp->data.number;
			tmp = getJSONValue(coll, "maxY");
			if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
			bb->maxY = (double) tmp->data.number;
			tmp = getJSONValue(coll, "minZ");
			if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
			bb->minZ = (double) tmp->data.number;
			tmp = getJSONValue(coll, "maxZ");
			if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
			bb->maxZ = (double) tmp->data.number;
		}
		tmp = getJSONValue(ur, "dropItem");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		bi->drop = (item) tmp->data.number;
		tmp = getJSONValue(ur, "dropDamage");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		bi->drop_damage = (int16_t) tmp->data.number;
		tmp = getJSONValue(ur, "dropAmountMin");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		bi->drop_min = (uint8_t) tmp->data.number;
		tmp = getJSONValue(ur, "dropAmountMax");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		bi->drop_max = (uint8_t) tmp->data.number;
		tmp = getJSONValue(ur, "hardness");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		bi->hardness = (float) tmp->data.number;
		tmp = getJSONValue(ur, "material");
		if (tmp == NULL || tmp->type != JSON_STRING) goto cerr;
		bi->material = getBlockMaterial(tmp->data.string);
		tmp = getJSONValue(ur, "slipperiness");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		bi->slipperiness = (float) tmp->data.number;
		add_block_info(id, bi);
		continue;
		cerr: ;
		printf("[WARNING] Error Loading Block \"%s\"! Skipped.\n", ur->name);
	}
	freeJSON(&json);
	xfree(jsf);
	struct block_info* tmp = getBlockInfo(BLK_CHEST);
	tmp->onBlockInteract = &onBlockInteract_chest;
	tmp->onBlockDestroyed = &onBlockDestroyed_chest;
	tmp->onBlockPlaced = &onBlockPlaced_chest;
	tmp = getBlockInfo(BLK_WORKBENCH);
	tmp->onBlockInteract = &onBlockInteract_workbench;
	tmp = getBlockInfo(BLK_FURNACE);
	tmp->onBlockInteract = &onBlockInteract_furnace;
	tmp->onBlockDestroyed = &onBlockDestroyed_furnace;
	tmp->onBlockPlaced = &onBlockPlaced_furnace;
	tmp = getBlockInfo(BLK_FURNACE_LIT);
	tmp->onBlockInteract = &onBlockInteract_furnace;
	tmp->onBlockDestroyed = &onBlockDestroyed_furnace;
	tmp->onBlockPlaced = &onBlockPlaced_furnace;
	tmp = getBlockInfo(BLK_CHESTTRAP);
	tmp->onBlockInteract = &onBlockInteract_chest;
	tmp->onBlockDestroyed = &onBlockDestroyed_chest;
	tmp->onBlockPlaced = &onBlockPlaced_chest;
	tmp = getBlockInfo(BLK_GRAVEL);
	tmp->dropItems = &dropItems_gravel;
	tmp = getBlockInfo(BLK_LEAVES_OAK);
	tmp->dropItems = &dropItems_leaves;
	tmp = getBlockInfo(BLK_LEAVES_SPRUCE);
	tmp->dropItems = &dropItems_leaves;
	tmp = getBlockInfo(BLK_LEAVES_BIRCH);
	tmp->dropItems = &dropItems_leaves;
	tmp = getBlockInfo(BLK_LEAVES_JUNGLE);
	tmp->dropItems = &dropItems_leaves;
	tmp = getBlockInfo(BLK_LEAVES_ACACIA);
	tmp->dropItems = &dropItems_leaves;
	tmp = getBlockInfo(BLK_LEAVES_BIG_OAK);
	tmp->dropItems = &dropItems_leaves;
	tmp = getBlockInfo(BLK_TALLGRASS_GRASS);
	tmp->dropItems = &dropItems_tallgrass;
	tmp = getBlockInfo(BLK_MUSHROOM_2);
	tmp->dropItems = &dropItems_hugemushroom;
	tmp = getBlockInfo(BLK_MUSHROOM_3);
	tmp->dropItems = &dropItems_hugemushroom;
}
