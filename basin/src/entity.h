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
#include "player.h"

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
#define ENT_FALLINGEGG 54
#define ENT_EXPBOTTLE 55
#define ENT_FIREWORK 56
#define ENT_LEASHKNOT 57
#define ENT_ARMORSTAND 58
#define ENT_FISHINGFLOAT 59
#define ENT_SPECTRALARROW 60
#define ENT_TIPPEDARROW 61
#define ENT_DRAGONFIREBALL 62
#define ENT_EXPERIENCEORB 63

int entNetworkConvert(int type, int id);

union entity_data {

};

struct potioneffect {
		char effectID;
		char amplifier;
		int32_t duration;
		char particles;
};

struct entity {
		int32_t id;
		double x;
		double y;
		double z;
		uint8_t type;
		float yaw;
		float pitch;
		float headpitch;
		int onGround;
		double motX;
		double motY;
		double motZ;
		union entity_data data;
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
		struct player* player;
		float fallDistance;
};

void readMetadata(struct entity* ent, unsigned char* meta, size_t size);

void load_entities();

void drawEntity(float partialTick, struct entity* ent);

struct entity* newEntity(int32_t id, float x, float y, float z, uint8_t type, float yaw, float pitch);

struct boundingbox* getEntityCollision(struct entity* ent);

void freeEntity(struct entity* entity);

#endif /* ENTITY_H_ */
