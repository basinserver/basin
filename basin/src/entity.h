/*
 * entity.h
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#ifndef ENTITY_H_
#define ENTITY_H_

#include <stdint.h>
#include "world.h"
#include <stdlib.h>
#include "world.h"

/*

 #define ENT_PLAYER 0
 #define ENT_ITEM 1
 #define ENT_XP_ORB 2
 #define ENT_AREA_EFFECT_CLOUD 3
 #define ENT_ELDER_GUARDIAN 4
 #define ENT_WITHER_SKELETON 5
 #define ENT_STRAY 6
 #define ENT_EGG 7
 #define ENT_LEASH_KNOT 8
 #define ENT_PAINTING 9
 #define ENT_ARROW 10
 #define ENT_SNOWBALL 11
 #define ENT_FIREBALL 12
 #define ENT_SMALL_FIREBALL 13
 #define ENT_ENDER_PEARL 14
 #define ENT_EYE_OF_ENDER_SIGNAL 15
 #define ENT_POTION 16
 #define ENT_XP_BOTTLE 17
 #define ENT_ITEM_FRAME 18
 #define ENT_WITHER_SKULL 19
 #define ENT_TNT 20
 #define ENT_FALLING_BLOCK 21
 #define ENT_FIREWORKS_ROCKET 22
 #define ENT_HUSK 23
 #define ENT_SPECTRAL_ARROW 24
 #define ENT_SHULKER_BULLET 25
 #define ENT_DRAGON_FIREBALL 26
 #define ENT_ZOMBIE_VILLAGER 27
 #define ENT_SKELETON_HORSE 28
 #define ENT_ZOMBIE_HORSE 29
 #define ENT_ARMOR_STAND 30
 #define ENT_DONKEY 31
 #define ENT_MULE 32
 #define ENT_EVOCATION_FANGS 33
 #define ENT_EVOCATION_ILLAGER 34
 #define ENT_VEX 35
 #define ENT_VINDICATION_ILLAGER 36
 #define ENT_COMMANDBLOCK_MINECART 40
 #define ENT_BOAT 41
 #define ENT_MINECART 42
 #define ENT_CHEST_MINECART 43
 #define ENT_FURNACE_MINECART 44
 #define ENT_TNT_MINECART 45
 #define ENT_HOPPER_MINECART 46
 #define ENT_SPAWNER_MINECART 47
 #define ENT_CREEPER 50
 #define ENT_SKELETON 51
 #define ENT_SPIDER 52
 #define ENT_GIANT 53
 #define ENT_ZOMBIE 54
 #define ENT_SLIME 55
 #define ENT_GHAST 56
 #define ENT_ZOMBIE_PIGMAN 57
 #define ENT_ENDERMAN 58
 #define ENT_CAVE_SPIDER 59
 #define ENT_SILVERFISH 60
 #define ENT_BLAZE 61
 #define ENT_MAGMA_CUBE 62
 #define ENT_ENDER_DRAGON 63
 #define ENT_WITHER 64
 #define ENT_BAT 65
 #define ENT_WITCH 66
 #define ENT_ENDERMITE 67
 #define ENT_GUARDIAN 68
 #define ENT_SHULKER 69
 #define ENT_PIG 90
 #define ENT_SHEEP 91
 #define ENT_COW 92
 #define ENT_CHICKEN 93
 #define ENT_SQUID 94
 #define ENT_WOLF 95
 #define ENT_MOOSHROOM 96
 #define ENT_SNOWMAN 97
 #define ENT_OCELOT 98
 #define ENT_VILLAGER_GOLEM 99
 #define ENT_HORSE 100
 #define ENT_RABBIT 101
 #define ENT_POLAR_BEAR 102
 #define ENT_LLAMA 103
 #define ENT_LLAMA_SPIT 104
 #define ENT_VILLAGER 120
 #define ENT_ENDER_CRYSTAL 200


 */

#define ENT_UNDEFINED 1
#define ENT_PLAYER 2
#define ENT_CREEPER 3
#define ENT_SKELETON 4
#define ENT_SPIDER 5
#define ENT_GIANT 6
#define ENT_ZOMBIE 7
#define ENT_SLIME 8
#define ENT_GHAST 9
#define ENT_ZPIGMAN 10
#define ENT_ENDERMAN 11
#define ENT_CAVESPIDER 12
#define ENT_SILVERFISH 13
#define ENT_BLAZE 14
#define ENT_MAGMACUBE 15
#define ENT_ENDERDRAGON 16
#define ENT_WITHER 17
#define ENT_BAT 18
#define ENT_WITCH 19
#define ENT_ENDERMITE 20
#define ENT_GUARDIAN 21
#define ENT_SHULKER 22
#define ENT_PIG 23
#define ENT_SHEEP 24
#define ENT_COW 25
#define ENT_CHICKEN 26
#define ENT_SQUID 27
#define ENT_WOLF 28
#define ENT_MOOSHROOM 29
#define ENT_SNOWMAN 30
#define ENT_OCELOT 31
#define ENT_IRONGOLEM 32
#define ENT_HORSE 33
#define ENT_RABBIT 34
#define ENT_VILLAGER 35
#define ENT_BOAT 36
#define ENT_ITEMSTACK 37
#define ENT_AREAEFFECT 38
#define ENT_MINECART 39
#define ENT_TNT 40
#define ENT_ENDERCRYSTAL 41
#define ENT_ARROW 42
#define ENT_SNOWBALL 43
#define ENT_EGG 44
#define ENT_FIREBALL 45
#define ENT_FIRECHARGE 46
#define ENT_ENDERPEARL 47
#define ENT_WITHERSKULL 48
#define ENT_SHULKERBULLET 49
#define ENT_FALLINGBLOCK 50
#define ENT_ITEMFRAME 51
#define ENT_EYEENDER 52
#define ENT_THROWNPOTION 53
#define ENT_HUSK 54
#define ENT_EXPBOTTLE 55
#define ENT_FIREWORK 56
#define ENT_LEASHKNOT 57
#define ENT_ARMORSTAND 58
#define ENT_FISHINGFLOAT 59
#define ENT_EVOCATIONFANGS 60
#define ENT_ELDERGUARDIAN 61
#define ENT_DRAGONFIREBALL 62
#define ENT_EXPERIENCEORB 63
#define ENT_POLARBEAR 64
#define ENT_LLAMA 65
#define ENT_LLAMASPIT 66
#define ENT_STRAY 67
#define ENT_PAINTING 68
#define ENT_EVOCATIONILLAGER 69
#define ENT_VEX 70
#define ENT_VINDICATIONILLAGER 71

int entNetworkConvert(int type, int id);
int networkEntConvert(int type, int id);

int shouldSendAsObject(int id);

#define POT_SPEED 1
#define POT_SLOWNESS 2
#define POT_HASTE 3
#define POT_MINING_FATIGUE 4
#define POT_STRENGTH 5
#define POT_INSTANT_HEALTH 6
#define POT_INSTANT_DAMAGE 7
#define POT_JUMP_BOOST 8
#define POT_NAUSEA 9
#define POT_REGENERATION 10
#define POT_RESISTANCE 11
#define POT_FIRE_RESISTANCE 12
#define POT_WATER_BREATHING 13
#define POT_INVISIBILITY 14
#define POT_BLINDNESS 15
#define POT_NIGHT_VISION 16
#define POT_HUNGER 17
#define POT_WEAKNESS 18
#define POT_POISON 19
#define POT_WITHER 20
#define POT_HEALTH_BOOST 21
#define POT_ABSORPTION 22
#define POT_SATURATION 23
#define POT_GLOWING 24
#define POT_LEVITATION 25
#define POT_LUCK 26
#define POT_UNLUCK 27

struct potioneffect {
		char effectID;
		char amplifier;
		int32_t duration;
		char particles;
};

int isLiving(int type);

union entity_data {
		struct entity_player {
				struct player* player;
		} player;
		struct entity_creeper {

		} creeper;
		struct entity_skeleton {

		} skeleton;
		struct entity_spider {

		} spider;
		struct entity_giant {

		} giant;
		struct entity_zombie {

		} zombie;
		struct entity_slime {
				uint8_t size;
		} slime;
		struct entity_ghast {

		} ghast;
		struct entity_zpigman {

		} zpigman;
		struct entity_enderman {

		} enderman;
		struct entity_cavespider {

		} cavespider;
		struct entity_silverfish {

		} silverfish;
		struct entity_blaze {

		} blaze;
		struct entity_magmacube {
				uint8_t size;
		} magmacube;
		struct entity_enderdragon {

		} enderdragon;
		struct entity_wither {

		} wither;
		struct entity_bat {

		} bat;
		struct entity_witch {

		} witch;
		struct entity_endermite {

		} endermite;
		struct entity_guardian {

		} guardian;
		struct entity_shulker {

		} shulker;
		struct entity_pig {

		} pig;
		struct entity_sheep {

		} sheep;
		struct entity_cow {

		} cow;
		struct entity_chicken {

		} chicken;
		struct entity_squid {

		} squid;
		struct entity_wolf {

		} wolf;
		struct entity_mooshroom {

		} mooshroom;
		struct entity_snowman {

		} snowman;
		struct entity_ocelot {

		} ocelot;
		struct entity_irongolem {

		} irongolem;
		struct entity_horse {

		} horse;
		struct entity_rabbit {

		} rabbit;
		struct entity_villager {

		} villager;
		struct entity_boat {

		} boat;
		struct entity_itemstack {
				struct slot* slot;
				int16_t delayBeforeCanPickup;
		} itemstack;
		struct entity_areaeffect {

		} areaeffect;
		struct entity_minecart {

		} minecart;
		struct entity_tnt {

		} tnt;
		struct entity_endercrystal {

		} endercrystal;
		struct entity_arrow {

		} arrow;
		struct entity_snowball {

		} snowball;
		struct entity_egg {

		} egg;
		struct entity_fireball {

		} fireball;
		struct entity_firecharge {

		} firecharge;
		struct entity_enderpearl {

		} enderpearl;
		struct entity_witherskull {

		} witherskull;
		struct entity_shulkerbullet {

		} shulkerbullet;
		struct entity_fallingblock {

		} fallingblock;
		struct entity_itemframe {

		} itemframe;
		struct entity_eyeender {

		} eyeender;
		struct entity_thrownpotion {

		} thrownpotion;
		struct entity_husk {

		} husk;
		struct entity_fallingegg {

		} fallingegg;
		struct entity_expbottle {

		} expbottle;
		struct entity_firework {

		} firework;
		struct entity_leashknot {

		} leashknot;
		struct entity_armorstand {

		} armorstand;
		struct entity_fishingfloat {

		} fishingfloat;
		struct entity_evocationfangs {

		} evocationfangs;
		struct entity_elderguardian {

		} elderguardian;
		struct entity_dragonfireball {

		} dragonfireball;
		struct entity_experienceorb {
				uint16_t count;
		} experienceorb;
		struct entity_polarbear {

		} polarbear;
		struct entity_llama {

		} llama;
		struct entity_llamaspit {

		} llamaspit;
		struct entity_stray {

		} stray;
		struct entity_painting {
				char* title;
				uint8_t direction;
		} painting;
		struct entity_evocationillager {

		} evocationillager;
		struct entity_vex {

		} vex;
		struct entity_vindicationillager {

		} vindicationillager;
};

struct entity {
		int32_t id;
		double x;
		double y;
		double z;
		double lx;
		double ly;
		double lz;
		uint8_t type;
		float yaw;
		float pitch;
		float lyaw;
		float lpitch;
		float headpitch;
		int onGround;
		int collidedVertically;
		int collidedHorizontally;
		double motX;
		double motY;
		double motZ;
		float health;
		float maxHealth;
		int32_t objectData;
		int markedKill;
		struct potioneffect* effects;
		size_t effect_count;
		int sneaking;
		int usingItem;
		int sprinting;
		int portalCooldown;
		size_t ticksExisted;
		int32_t subtype;
		float fallDistance;
		union entity_data data;
		struct world* world;
		struct hashmap* loadingPlayers;
		uint64_t age;
		uint8_t invincibilityTicks;
		uint8_t inWater;
		uint8_t inLava;
};

void damageEntityWithItem(struct entity* attacked, struct entity* attacker, uint8_t slot_index, struct slot* item);

void damageEntity(struct entity* attacked, float damage, int armorable);

void healEntity(struct entity* healed, float amount);

void readMetadata(struct entity* ent, unsigned char* meta, size_t size);

void writeMetadata(struct entity* ent, unsigned char** data, size_t* size);

void load_entities();

int entity_inFluid(struct entity* entity, uint16_t blk, float ydown, int meta_check);

double entity_dist(struct entity* ent1, struct entity* ent2);

double entity_distsq(struct entity* ent1, struct entity* ent2);

double entity_distsq_block(struct entity* ent1, double x, double y, double z);

double entity_dist_block(struct entity* ent1, double x, double y, double z);

struct entity* newEntity(int32_t id, float x, float y, float z, uint8_t type, float yaw, float pitch);

void getEntityCollision(struct entity* ent, struct boundingbox* bb);

int moveEntity(struct entity* entity, double mx, double my, double mz, float shrink);

int tick_itemstack(struct world* world, struct entity* entity);

void tick_entity(struct world* world, struct entity* entity);

void freeEntity(struct entity* entity);

#endif /* ENTITY_H_ */
