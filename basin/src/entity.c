/*
 * entity.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "basin.h"
#include "network.h"
#include "packet.h"
#include "util.h"
#include "xstring.h"
#include "queue.h"
#include "entity.h"
#include <unistd.h>
#include <pthread.h>

void getEntityCollision(struct entity* ent, struct boundingbox* bb) {
	float width = .3;
	float height = 1.8;
	int32_t id = ent->type;
	if (id == ENT_BOAT) {
		width = 1.375;
		height = .5625;
	} else if (id == ENT_ITEMSTACK) {
		width = .25;
		height = .25;
	} else if (id == ENT_AREAEFFECT) {
		width = 2.;
		height = .5;
	} else if (id == ENT_MINECART) {
		width = .98;
		height = .7;
	} else if (id == ENT_TNT) {
		width = .98;
		height = .98;
	} else if (id == ENT_ENDERCRYSTAL) {
		width = 2.;
		height = 2.;
	} else if (id == ENT_ARROW) {
		width = .5;
		height = .5;
	} else if (id == ENT_SNOWBALL) {
		width = .25;
		height = .25;
	} else if (id == ENT_EGG) {
		width = .25;
		height = .25;
	} else if (id == ENT_FIREBALL) {
		width = 1.;
		height = 1.;
	} else if (id == ENT_FIRECHARGE) {
		width = .3125;
		height = .3125;
	} else if (id == ENT_ENDERPEARL) {
		width = .25;
		height = .25;
	} else if (id == ENT_WITHERSKULL) {
		width = .3125;
		height = .3125;
	} else if (id == ENT_SHULKERBULLET) {
		width = .3125;
		height = .3125;
	} else if (id == ENT_LLAMASPIT) {
		width = .25;
		height = .25;
	} else if (id == ENT_FALLINGBLOCK) {
		width = .98;
		height = .98;
	} else if (id == ENT_ITEMFRAME) {
		width = .5;
		height = .5;
	} else if (id == ENT_EYEENDER) {
		width = 1.;
		height = 1.;
	} else if (id == ENT_THROWNPOTION) {
		width = .25;
		height = .25;
	} else if (id == ENT_EXPBOTTLE) {
		width = .25;
		height = .25;
	} else if (id == ENT_FIREWORK) {
		width = .25;
		height = .25;
	} else if (id == ENT_LEASHKNOT) {
		width = .5;
		height = .5;
	} else if (id == ENT_ARMORSTAND) {
		width = .5;
		height = 1.975;
	} else if (id == ENT_EVOCATIONFANGS) {
		width = .5;
		height = .8;
	} else if (id == ENT_FISHINGFLOAT) {
		width = .25;
		height = .25;
	} else if (id == ENT_DRAGONFIREBALL) {
		width = 1.;
		height = 1.;
	} else if (id == ENT_ELDERGUARDIAN) {
		width = .85 * 2.35;
		height = .85 * 2.35;
	} else if (id == ENT_SKELETON) {
		width = .6;
		height = 1.99;
	} else if (id == ENT_STRAY) {
		width = .6;
		height = 1.99;
	} else if (id == ENT_HUSK) {
		width = .6;
		height = 1.95;
	} else if (id == ENT_ZOMBIE) {
		width = .6;
		height = 1.95;
	} else if (id == ENT_HORSE) {
		width = 1.3964833;
		height = 1.6;
	} else if (id == ENT_EVOCATIONILLAGER) {
		width = .6;
		height = 1.95;
	} else if (id == ENT_VEX) {
		width = .4;
		height = .8;
	} else if (id == ENT_VINDICATIONILLAGER) {
		width = .6;
		height = 1.95;
	} else if (id == ENT_CREEPER) {
		width = .6;
		height = 1.7;
	} else if (id == ENT_SKELETON) {
		width = .6;
		height = 1.99;
	} else if (id == ENT_SPIDER) {
		width = 1.4;
		height = .9;
	} else if (id == ENT_GIANT) {
		width = .6 * 6.;
		height = 1.95 * 6.;
	} else if (id == ENT_SLIME) {
		width = .51 * ent->data.slime.size;
		height = .51 * ent->data.slime.size;
	} else if (id == ENT_GHAST) {
		width = 4.;
		height = 4.;
	} else if (id == ENT_ZPIGMAN) {
		width = .6;
		height = 1.95;
	} else if (id == ENT_ENDERMAN) {
		width = .6;
		height = 2.9;
	} else if (id == ENT_CAVESPIDER) {
		width = .7;
		height = .5;
	} else if (id == ENT_SILVERFISH) {
		width = .4;
		height = .3;
	} else if (id == ENT_BLAZE) {
		width = 0.;
		height = 0.;
	} else if (id == ENT_MAGMACUBE) {
		width = .51 * ent->data.slime.size;
		height = .51 * ent->data.slime.size;
	} else if (id == ENT_ENDERDRAGON) {
		width = 16.;
		height = 18.;
	} else if (id == ENT_WITHER) {
		width = .9;
		height = 3.5;
	} else if (id == ENT_BAT) {
		width = .5;
		height = .9;
	} else if (id == ENT_WITCH) {
		width = .6;
		height = 1.95;
	} else if (id == ENT_ENDERMITE) {
		width = .4;
		height = .3;
	} else if (id == ENT_GUARDIAN) {
		width = .85;
		height = .85;
	} else if (id == ENT_SHULKER) {
		width = 1.;
		height = 1.;
	} else if (id == ENT_PIG) {
		width = .9;
		height = .9;
	} else if (id == ENT_SHEEP) {
		width = .9;
		height = 1.3;
	} else if (id == ENT_COW) {
		width = .9;
		height = 1.4;
	} else if (id == ENT_CHICKEN) {
		width = .4;
		height = .7;
	} else if (id == ENT_SQUID) {
		width = .8;
		height = .8;
	} else if (id == ENT_WOLF) {
		width = .6;
		height = .85;
	} else if (id == ENT_MOOSHROOM) {
		width = .9;
		height = 1.4;
	} else if (id == ENT_SNOWMAN) {
		width = .7;
		height = 1.9;
	} else if (id == ENT_OCELOT) {
		width = .6;
		height = .7;
	} else if (id == ENT_IRONGOLEM) {
		width = 1.4;
		height = 2.7;
	} else if (id == ENT_RABBIT) {
		width = .4;
		height = .5;
	} else if (id == ENT_POLARBEAR) {
		width = 1.3;
		height = 1.4;
	} else if (id == ENT_LLAMA) {
		width = .6;
		height = 1.87;
	} else if (id == ENT_VILLAGER) {
		width = .6;
		height = 1.95;
	} else if (id == ENT_PAINTING) {
		width = 0.;
		height = 0.;
	} else if (id == ENT_EXPERIENCEORB) {
		width = .5;
		height = .5;
	} else if (id == ENT_PLAYER) {
		width = .6;
		height = 1.8;
	}
	bb->minX = ent->x - width / 2;
	bb->maxX = ent->x + width / 2;
	bb->minZ = ent->z - width / 2;
	bb->maxZ = ent->z + width / 2;
	bb->minY = ent->y;
	bb->maxY = ent->y + height;
}

int entNetworkConvert(int type, int id) {
	if (type == 0) {
		if (id == 1) return ENT_BOAT;
		else if (id == 2) return ENT_ITEMSTACK;
		else if (id == 3) return ENT_AREAEFFECT;
		else if (id == 10) return ENT_MINECART;
		else if (id == 50) return ENT_TNT;
		else if (id == 51) return ENT_ENDERCRYSTAL;
		else if (id == 60) return ENT_ARROW;
		else if (id == 61) return ENT_SNOWBALL;
		else if (id == 62) return ENT_EGG;
		else if (id == 63) return ENT_FIREBALL;
		else if (id == 64) return ENT_FIRECHARGE;
		else if (id == 65) return ENT_ENDERPEARL;
		else if (id == 66) return ENT_WITHERSKULL;
		else if (id == 67) return ENT_SHULKERBULLET;
		else if (id == 68) return ENT_LLAMASPIT;
		else if (id == 70) return ENT_FALLINGBLOCK;
		else if (id == 71) return ENT_ITEMFRAME;
		else if (id == 72) return ENT_EYEENDER;
		else if (id == 73) return ENT_THROWNPOTION;
		else if (id == 75) return ENT_EXPBOTTLE;
		else if (id == 76) return ENT_FIREWORK;
		else if (id == 77) return ENT_LEASHKNOT;
		else if (id == 78) return ENT_ARMORSTAND;
		else if (id == 79) return ENT_EVOCATIONFANGS;
		else if (id == 90) return ENT_FISHINGFLOAT;
		else if (id == 91) return ENT_ARROW;
		else if (id == 93) return ENT_DRAGONFIREBALL;
	} else if (type == 1) {
		if (id == 4) return ENT_ELDERGUARDIAN;
		else if (id == 5) return ENT_SKELETON;
		else if (id == 6) return ENT_STRAY;
		else if (id == 23) return ENT_HUSK;
		else if (id == 27) return ENT_ZOMBIE;
		else if (id == 28) return ENT_HORSE;
		else if (id == 29) return ENT_HORSE;
		else if (id == 31) return ENT_HORSE;
		else if (id == 32) return ENT_HORSE;
		else if (id == 34) return ENT_EVOCATIONILLAGER;
		else if (id == 35) return ENT_VEX;
		else if (id == 36) return ENT_VINDICATIONILLAGER;
		else if (id == 50) return ENT_CREEPER;
		else if (id == 51) return ENT_SKELETON;
		else if (id == 52) return ENT_SPIDER;
		else if (id == 53) return ENT_GIANT;
		else if (id == 54) return ENT_ZOMBIE;
		else if (id == 55) return ENT_SLIME;
		else if (id == 56) return ENT_GHAST;
		else if (id == 57) return ENT_ZPIGMAN;
		else if (id == 58) return ENT_ENDERMAN;
		else if (id == 59) return ENT_CAVESPIDER;
		else if (id == 60) return ENT_SILVERFISH;
		else if (id == 61) return ENT_BLAZE;
		else if (id == 62) return ENT_MAGMACUBE;
		else if (id == 63) return ENT_ENDERDRAGON;
		else if (id == 64) return ENT_WITHER;
		else if (id == 65) return ENT_BAT;
		else if (id == 66) return ENT_WITCH;
		else if (id == 67) return ENT_ENDERMITE;
		else if (id == 68) return ENT_GUARDIAN;
		else if (id == 69) return ENT_SHULKER;
		else if (id == 90) return ENT_PIG;
		else if (id == 91) return ENT_SHEEP;
		else if (id == 92) return ENT_COW;
		else if (id == 93) return ENT_CHICKEN;
		else if (id == 94) return ENT_SQUID;
		else if (id == 95) return ENT_WOLF;
		else if (id == 96) return ENT_MOOSHROOM;
		else if (id == 97) return ENT_SNOWMAN;
		else if (id == 98) return ENT_OCELOT;
		else if (id == 99) return ENT_IRONGOLEM;
		else if (id == 100) return ENT_HORSE;
		else if (id == 101) return ENT_RABBIT;
		else if (id == 102) return ENT_POLARBEAR;
		else if (id == 103) return ENT_LLAMA;
		else if (id == 120) return ENT_VILLAGER;
	}
	return -1;
}

int networkEntConvert(int type, int id) {
	if (type == 0) {
		if (id == ENT_BOAT) return 1;
		else if (id == ENT_ITEMSTACK) return 2;
		else if (id == ENT_AREAEFFECT) return 3;
		else if (id == ENT_MINECART) return 10;
		else if (id == ENT_TNT) return 50;
		else if (id == ENT_ENDERCRYSTAL) return 51;
		else if (id == ENT_ARROW) return 60;
		else if (id == ENT_SNOWBALL) return 61;
		else if (id == ENT_EGG) return 62;
		else if (id == ENT_FIREBALL) return 63;
		else if (id == ENT_FIRECHARGE) return 64;
		else if (id == ENT_ENDERPEARL) return 65;
		else if (id == ENT_WITHERSKULL) return 66;
		else if (id == ENT_SHULKERBULLET) return 67;
		else if (id == ENT_LLAMASPIT) return 68;
		else if (id == ENT_FALLINGBLOCK) return 70;
		else if (id == ENT_ITEMFRAME) return 71;
		else if (id == ENT_EYEENDER) return 72;
		else if (id == ENT_THROWNPOTION) return 73;
		else if (id == ENT_EXPBOTTLE) return 75;
		else if (id == ENT_FIREWORK) return 76;
		else if (id == ENT_LEASHKNOT) return 77;
		else if (id == ENT_ARMORSTAND) return 78;
		else if (id == ENT_EVOCATIONFANGS) return 79;
		else if (id == ENT_FISHINGFLOAT) return 90;
		else if (id == ENT_ARROW) return 91;
		else if (id == ENT_DRAGONFIREBALL) return 93;
	} else if (type == 1) {
		if (id == ENT_ELDERGUARDIAN) return 4;
		else if (id == ENT_SKELETON) return 5;
		else if (id == ENT_STRAY) return 6;
		else if (id == ENT_HUSK) return 23;
		else if (id == ENT_ZOMBIE) return 27;
		else if (id == ENT_HORSE) return 28;
		else if (id == ENT_HORSE) return 29;
		else if (id == ENT_HORSE) return 31;
		else if (id == ENT_HORSE) return 32;
		else if (id == ENT_EVOCATIONILLAGER) return 34;
		else if (id == ENT_VEX) return 35;
		else if (id == ENT_VINDICATIONILLAGER) return 36;
		else if (id == ENT_CREEPER) return 50;
		else if (id == ENT_SKELETON) return 51;
		else if (id == ENT_SPIDER) return 52;
		else if (id == ENT_GIANT) return 53;
		else if (id == ENT_ZOMBIE) return 54;
		else if (id == ENT_SLIME) return 55;
		else if (id == ENT_GHAST) return 56;
		else if (id == ENT_ZPIGMAN) return 57;
		else if (id == ENT_ENDERMAN) return 58;
		else if (id == ENT_CAVESPIDER) return 59;
		else if (id == ENT_SILVERFISH) return 60;
		else if (id == ENT_BLAZE) return 61;
		else if (id == ENT_MAGMACUBE) return 62;
		else if (id == ENT_ENDERDRAGON) return 63;
		else if (id == ENT_WITHER) return 64;
		else if (id == ENT_BAT) return 65;
		else if (id == ENT_WITCH) return 66;
		else if (id == ENT_ENDERMITE) return 67;
		else if (id == ENT_GUARDIAN) return 68;
		else if (id == ENT_SHULKER) return 69;
		else if (id == ENT_PIG) return 90;
		else if (id == ENT_SHEEP) return 91;
		else if (id == ENT_COW) return 92;
		else if (id == ENT_CHICKEN) return 93;
		else if (id == ENT_SQUID) return 94;
		else if (id == ENT_WOLF) return 95;
		else if (id == ENT_MOOSHROOM) return 96;
		else if (id == ENT_SNOWMAN) return 97;
		else if (id == ENT_OCELOT) return 98;
		else if (id == ENT_IRONGOLEM) return 99;
		else if (id == ENT_HORSE) return 100;
		else if (id == ENT_RABBIT) return 101;
		else if (id == ENT_POLARBEAR) return 102;
		else if (id == ENT_LLAMA) return 103;
		else if (id == ENT_VILLAGER) return 120;
	}
	return -1;
}

int shouldSendAsObject(int id) {
	return (id == ENT_BOAT || id == ENT_ITEMSTACK || id == ENT_AREAEFFECT || id == ENT_MINECART || id == ENT_TNT || id == ENT_ENDERCRYSTAL || id == ENT_ARROW || id == ENT_SNOWBALL || id == ENT_EGG || id == ENT_FIREBALL || id == ENT_FIRECHARGE || id == ENT_ENDERPEARL || id == ENT_WITHERSKULL || id == ENT_SHULKERBULLET || id == ENT_LLAMASPIT || id == ENT_FALLINGBLOCK || id == ENT_ITEMFRAME || id == ENT_EYEENDER || id == ENT_THROWNPOTION || id == ENT_EXPBOTTLE || id == ENT_FIREWORK || id == ENT_LEASHKNOT || id == ENT_ARMORSTAND || id == ENT_EVOCATIONFANGS || id == ENT_FISHINGFLOAT || id == ENT_ARROW || id == ENT_DRAGONFIREBALL);
}

struct entity* newEntity(int32_t id, float x, float y, float z, uint8_t type, float yaw, float pitch) {
	struct entity* e = malloc(sizeof(struct entity));
	e->id = id;
	e->age = 0;
	e->x = x;
	e->y = y;
	e->z = z;
	e->lx = x;
	e->ly = y;
	e->lz = z;
	e->type = type;
	e->yaw = yaw;
	e->pitch = pitch;
	e->lyaw = yaw;
	e->lpitch = pitch;
	e->headpitch = 0.;
	e->onGround = 1;
	e->motX = 0.;
	e->motY = 0.;
	e->motZ = 0.;
	e->objectData = 0;
	e->markedKill = 0;
	e->effects = NULL;
	e->effect_count = 0;
	e->collidedVertically = 0;
	e->collidedHorizontally = 0;
	e->sneaking = 0;
	e->sprinting = 0;
	e->usingItem = 0;
	e->portalCooldown = 0;
	e->ticksExisted = 0;
	e->subtype = 0;
	e->fallDistance = 0.;
	e->maxHealth = 20;
	e->health = e->maxHealth;
	e->world = NULL;
	e->inWater = 0;
	e->inLava = 0;
	e->invincibilityTicks = 0;
	e->loadingPlayers = new_hashmap(1, 1);
	memset(&e->data, 0, sizeof(union entity_data));
	return e;
}

double entity_dist(struct entity* ent1, struct entity* ent2) {
	return ent1->world == ent2->world ? sqrt((ent1->x - ent2->x) * (ent1->x - ent2->x) + (ent1->y - ent2->y) * (ent1->y - ent2->y) + (ent1->z - ent2->z) * (ent1->z - ent2->z)) : 999.;
}

double entity_distsq(struct entity* ent1, struct entity* ent2) {
	return (ent1->x - ent2->x) * (ent1->x - ent2->x) + (ent1->y - ent2->y) * (ent1->y - ent2->y) + (ent1->z - ent2->z) * (ent1->z - ent2->z);
}

double entity_distsq_block(struct entity* ent1, double x, double y, double z) {
	return (ent1->x - x) * (ent1->x - x) + (ent1->y - y) * (ent1->y - y) + (ent1->z - z) * (ent1->z - z);
}

double entity_dist_block(struct entity* ent1, double x, double y, double z) {
	return sqrt((ent1->x - x) * (ent1->x - x) + (ent1->y - y) * (ent1->y - y) + (ent1->z - z) * (ent1->z - z));
}

struct packet* getEntityPropertiesPacket(struct entity* entity) {
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ENTITYPROPERTIES;
	pkt->data.play_client.entityproperties.entity_id = entity->id;
	pkt->data.play_client.entityproperties.number_of_properties = 0;
	if (entity->type == ENT_PLAYER) {

	}
	return pkt;
}

void handleMetaByte(struct entity* ent, int index, signed char b) {
	if (index == 0) {
		ent->sneaking = b & 0x02 ? 1 : 0;
		ent->sprinting = b & 0x08 ? 1 : 0;
		ent->usingItem = b & 0x10 ? 1 : 0;
	}
}

void handleMetaVarInt(struct entity* ent, int index, int32_t i) {
	if ((ent->type == ENT_SKELETON || ent->type == ENT_SLIME || ent->type == ENT_MAGMACUBE || ent->type == ENT_GUARDIAN) && index == 11) {
		ent->subtype = i;
	}
}

void handleMetaFloat(struct entity* ent, int index, float f) {

}

void handleMetaString(struct entity* ent, int index, char* str) {

	xfree(str);
}

void handleMetaSlot(struct entity* ent, int index, struct slot* slot) {
	if (ent->type == ENT_ITEMSTACK && index == 6) {
		ent->data.itemstack.slot = slot;
	}
}

void handleMetaVector(struct entity* ent, int index, float f1, float f2, float f3) {

}

void handleMetaPosition(struct entity* ent, int index, struct encpos* pos) {

}

void handleMetaUUID(struct entity* ent, int index, struct uuid* pos) {

}

int isLiving(int type) {
	return type == ENT_PLAYER || type == ENT_SKELETON || type == ENT_SPIDER || type == ENT_GIANT || type == ENT_ZOMBIE || type == ENT_SLIME || type == ENT_GHAST || type == ENT_ZPIGMAN || type == ENT_ENDERMAN || type == ENT_CAVESPIDER || type == ENT_SILVERFISH || type == ENT_BLAZE || type == ENT_MAGMACUBE || type == ENT_ENDERDRAGON || type == ENT_WITHER || type == ENT_BAT || type == ENT_WITCH || type == ENT_ENDERMITE || type == ENT_GUARDIAN || type == ENT_SHULKER || type == ENT_PIG || type == ENT_SHEEP || type == ENT_COW || type == ENT_CHICKEN || type == ENT_SQUID || type == ENT_WOLF || type == ENT_MOOSHROOM || type == ENT_SNOWMAN || type == ENT_OCELOT || type == ENT_IRONGOLEM || type == ENT_HORSE || type == ENT_RABBIT || type == ENT_VILLAGER || type == ENT_BOAT;
}

int entity_inFluid(struct entity* entity, uint16_t blk, float ydown, int meta_check) {
	struct boundingbox pbb;
	getEntityCollision(entity, &pbb);
	if (pbb.minX == pbb.maxX || pbb.minZ == pbb.maxZ || pbb.minY == pbb.maxY) return 0;
	pbb.minX += .001;
	pbb.minY += .001;
	pbb.minY -= ydown;
	pbb.minZ += .001;
	pbb.maxX -= .001;
	pbb.maxY -= .001;
	pbb.maxZ -= .001;
	for (int32_t x = floor(pbb.minX); x < floor(pbb.maxX + 1.); x++) {
		for (int32_t z = floor(pbb.minZ); z < floor(pbb.maxZ + 1.); z++) {
			for (int32_t y = floor(pbb.minY); y < floor(pbb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				if (meta_check ? (b != blk) : ((b >> 4) != (blk >> 4))) continue;
				struct block_info* bi = getBlockInfo(b);
				if (bi == NULL) continue;
				struct boundingbox bb2;
				bb2.minX = 0. + (double) x;
				bb2.maxX = 1. + (double) x;
				bb2.minY = 0. + (double) y;
				bb2.maxY = 1. + (double) y;
				bb2.minZ = 0. + (double) z;
				bb2.maxZ = 1. + (double) z;
				if (boundingbox_intersects(&bb2, &pbb)) {
					return 1;
				}
			}
		}
	}
	return 0;
}

void outputMetaByte(struct entity* ent, unsigned char** loc, size_t* size) {
	*loc = xrealloc(*loc, (*size) + 3);
	(*loc)[(*size)++] = 0;
	(*loc)[(*size)++] = 0;
	(*loc)[*size] = ent->sneaking ? 0x02 : 0;
	(*loc)[*size] |= ent->sprinting ? 0x08 : 0;
	(*loc)[(*size)++] |= ent->usingItem ? 0x08 : 0;
}

void outputMetaVarInt(struct entity* ent, unsigned char** loc, size_t* size) {
	if ((ent->type == ENT_SKELETON || ent->type == ENT_SLIME || ent->type == ENT_MAGMACUBE || ent->type == ENT_GUARDIAN)) {
		*loc = xrealloc(*loc, *size + 2 + getVarIntSize(ent->subtype));
		(*loc)[(*size)++] = 11;
		(*loc)[(*size)++] = 1;
		*size += writeVarInt(ent->subtype, *loc + *size);
	}
}

void outputMetaFloat(struct entity* ent, unsigned char** loc, size_t* size) {
	if (isLiving(ent->type)) {
		*loc = xrealloc(*loc, *size + 6);
		(*loc)[(*size)++] = 7;
		(*loc)[(*size)++] = 2;
		memcpy(*loc + *size, &ent->health, 4);
		swapEndian(*loc + *size, 4);
		*size += 4;
	}
}

void outputMetaString(struct entity* ent, unsigned char** loc, size_t* size) {

}

void outputMetaSlot(struct entity* ent, unsigned char** loc, size_t* size) {
	if (ent->type == ENT_ITEMSTACK) {
		*loc = xrealloc(*loc, *size + 1024);
		(*loc)[(*size)++] = 6;
		(*loc)[(*size)++] = 5;
		*size += writeSlot(ent->data.itemstack.slot, *loc + *size, *size + 1022);
	}
}

void outputMetaVector(struct entity* ent, unsigned char** loc, size_t* size) {

}

void outputMetaPosition(struct entity* ent, unsigned char** loc, size_t* size) {

}

void outputMetaUUID(struct entity* ent, unsigned char** loc, size_t* size) {

}

void readMetadata(struct entity* ent, unsigned char* meta, size_t size) {
	while (size > 0) {
		unsigned char index = meta[0];
		meta++;
		size--;
		if (index == 0xff || size == 0) break;
		unsigned char type = meta[0];
		meta++;
		size--;
		if (type == 0 || type == 6) {
			if (size == 0) break;
			signed char b = meta[0];
			meta++;
			size--;
			handleMetaByte(ent, index, b);
		} else if (type == 1 || type == 10 || type == 12) {
			if ((type == 10 || type == 12) && size == 0) break;
			int32_t vi = 0;
			int r = readVarInt(&vi, meta, size);
			meta += r;
			size -= r;
			handleMetaVarInt(ent, index, vi);
		} else if (type == 2) {
			if (size < 4) break;
			float f = 0.;
			memcpy(&f, meta, 4);
			meta += 4;
			size -= 4;
			swapEndian(&f, 4);
			handleMetaFloat(ent, index, f);
		} else if (type == 3 || type == 4) {
			char* str = NULL;
			int r = readString(&str, meta, size);
			meta += r;
			size -= r;
			handleMetaString(ent, index, str);
		} else if (type == 5) {
			struct slot slot;
			int r = readSlot(&slot, meta, size);
			meta += r;
			size -= r;
			handleMetaSlot(ent, index, &slot);
		} else if (type == 7) {
			if (size < 12) break;
			float f1 = 0.;
			float f2 = 0.;
			float f3 = 0.;
			memcpy(&f1, meta, 4);
			meta += 4;
			size -= 4;
			memcpy(&f2, meta, 4);
			meta += 4;
			size -= 4;
			memcpy(&f3, meta, 4);
			meta += 4;
			size -= 4;
			swapEndian(&f1, 4);
			swapEndian(&f2, 4);
			swapEndian(&f3, 4);
			handleMetaVector(ent, index, f1, f2, f3);
		} else if (type == 8) {
			struct encpos pos;
			if (size < sizeof(struct encpos)) break;
			memcpy(&pos, meta, sizeof(struct encpos));
			meta += sizeof(struct encpos);
			size -= sizeof(struct encpos);
			handleMetaPosition(ent, index, &pos);
		} else if (type == 9) {
			if (size == 0) break;
			signed char b = meta[0];
			meta++;
			size--;
			if (b) {
				struct encpos pos;
				if (size < sizeof(struct encpos)) break;
				memcpy(&pos, meta, sizeof(struct encpos));
				meta += sizeof(struct encpos);
				size -= sizeof(struct encpos);
				handleMetaPosition(ent, index, &pos);
			}
		} else if (type == 11) {
			if (size == 0) break;
			signed char b = meta[0];
			meta++;
			size--;
			if (b) {
				struct uuid uuid;
				if (size < sizeof(struct uuid)) break;
				memcpy(&uuid, meta, sizeof(struct uuid));
				meta += sizeof(struct uuid);
				size -= sizeof(struct uuid);
				handleMetaUUID(ent, index, &uuid);
			}
		}
	}
}

void writeMetadata(struct entity* ent, unsigned char** data, size_t* size) {
	*data = NULL;
	*size = 0;
	outputMetaByte(ent, data, size);
	outputMetaVarInt(ent, data, size);
	outputMetaFloat(ent, data, size);
	outputMetaString(ent, data, size);
	outputMetaSlot(ent, data, size);
	outputMetaVector(ent, data, size);
	outputMetaPosition(ent, data, size);
	outputMetaUUID(ent, data, size);
	*data = xrealloc(*data, *size + 1);
	(*data)[(*size)++] = 0xFF;
}

int getSwingTime(struct entity* ent) {
	for (size_t i = 0; i < ent->effect_count; i++) {
		if (ent->effects[i].effectID == 3) {
			return 6 - (1 + ent->effects[i].amplifier);
		} else if (ent->effects[i].effectID == 4) {
			return 6 + (1 + ent->effects[i].amplifier) * 2;
		}
	}
	return 6;
}

int moveEntity(struct entity* entity, double mx, double my, double mz, float shrink) {
	struct boundingbox obb;
	getEntityCollision(entity, &obb);
	if (obb.minX == obb.maxX || obb.minZ == obb.maxZ || obb.minY == obb.maxY) {
		entity->x += mx;
		entity->y += my;
		entity->z += mz;
		return 0;
	}
	obb.minX += shrink;
	obb.maxX -= shrink;
	obb.minY += shrink;
	obb.maxY -= shrink;
	obb.minZ += shrink;
	obb.maxZ -= shrink;
	if (mx < 0.) {
		obb.minX += mx;
	} else {
		obb.maxX += mx;
	}
	if (my < 0.) {
		obb.minY += my;
	} else {
		obb.maxY += my;
	}
	if (mz < 0.) {
		obb.minZ += mz;
	} else {
		obb.maxZ += mz;
	}
	struct boundingbox pbb;
	getEntityCollision(entity, &pbb);
	pbb.minX += shrink;
	pbb.maxX -= shrink;
	pbb.minY += shrink;
	pbb.maxY -= shrink;
	pbb.minZ += shrink;
	pbb.maxZ -= shrink;
	double ny = my;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				if (b == 0) continue;
				struct block_info* bi = getBlockInfo(b);
				if (bi == NULL) continue;
				for (size_t i = 0; i < bi->boundingBox_count; i++) {
					struct boundingbox* bb = &bi->boundingBoxes[i];
//					if (!bi->fullCube) {
//						for (double *d = (double *) &bi->boundingBoxes[0], idx = 0; idx < 6; idx++, d++)
//							printf("%f ", *d);
//						printf("\n");
//					}
					if (bb != NULL && bb->minX != bb->maxX && bb->minY != bb->maxY && bb->minZ != bb->maxZ) {
						if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
							if (pbb.maxX > bb->minX + x && pbb.minX < bb->maxX + x && pbb.maxZ > bb->minZ + z && pbb.minZ < bb->maxZ + z) {
								double t;
								if (ny > 0. && pbb.maxY <= bb->minY + y) {
									t = bb->minY + y - pbb.maxY;
									if (t < ny) {
										ny = t;
									}
								} else if (ny < 0. && pbb.minY >= bb->maxY + y) {
									t = bb->maxY + y - pbb.minY;
									if (t > ny) {
										ny = t;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	entity->y += ny;
	pbb.minY += ny;
	pbb.maxY += ny;
	double nx = mx;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				if (b == 0) continue;
				struct block_info* bi = getBlockInfo(b);
				if (bi == NULL) continue;
				for (size_t i = 0; i < bi->boundingBox_count; i++) {
					struct boundingbox* bb = &bi->boundingBoxes[i];
					if (bb != NULL && bb->minX != bb->maxX && bb->minY != bb->maxY && bb->minZ != bb->maxZ) {
						if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
							if (pbb.maxY > bb->minY + y && pbb.minY < bb->maxY + y && pbb.maxZ > bb->minZ + z && pbb.minZ < bb->maxZ + z) {
								double t;
								if (nx > 0. && pbb.maxX <= bb->minX + x) {
									t = bb->minX + x - pbb.maxX;
									if (t < nx) {
										nx = t;
									}
								} else if (nx < 0. && pbb.minX >= bb->maxX + x) {
									t = bb->maxX + x - pbb.minX;
									if (t > nx) {
										nx = t;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	entity->x += nx;
	pbb.minX += nx;
	pbb.maxX += nx;
	double nz = mz;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				if (b == 0) continue;
				struct block_info* bi = getBlockInfo(b);
				if (bi == NULL) continue;
				for (size_t i = 0; i < bi->boundingBox_count; i++) {
					struct boundingbox* bb = &bi->boundingBoxes[i];
					if (bb != NULL && bb->minX != bb->maxX && bb->minY != bb->maxY && bb->minZ != bb->maxZ) {
						if (bb->maxX + x > obb.minX && bb->minX + x < obb.maxX ? (bb->maxY + y > obb.minY && bb->minY + y < obb.maxY ? bb->maxZ + z > obb.minZ && bb->minZ + z < obb.maxZ : 0) : 0) {
							if (pbb.maxX > bb->minX + x && pbb.minX < bb->maxX + x && pbb.maxY > bb->minY + y && pbb.minY < bb->maxY + y) {
								double t;
								if (nz > 0. && pbb.maxZ <= bb->minZ + z) {
									t = bb->minZ + z - pbb.maxZ;
									if (t < nz) {
										nz = t;
									}
								} else if (nz < 0. && pbb.minZ >= bb->maxZ + z) {
									t = bb->maxZ + z - pbb.minZ;
									if (t > nz) {
										nz = t;
									}
								}
							}
						}
					}
				}
			}
		}
	}
//TODO step
	entity->z += nz;
	pbb.minZ += nz;
	pbb.maxZ += nz;
	entity->collidedHorizontally = mx != nx || mz != nz;
	entity->collidedVertically = my != ny;
	entity->onGround = entity->collidedVertically && my < 0.;
	int32_t bx = floor(entity->x);
	int32_t by = floor(entity->y - .20000000298023224);
	int32_t bz = floor(entity->z);
	block lb = getBlockWorld(entity->world, bx, by, bz);
	if (lb == BLK_AIR) {
		block lbb = getBlockWorld(entity->world, bx, by - 1, bz);
		uint16_t lbi = lbb >> 4;
		if ((lbi >= (BLK_FENCE >> 4) && lbi <= (BLK_ACACIAFENCE >> 4)) || (lbi >= (BLK_FENCEGATE >> 4) && lbi <= (BLK_ACACIAFENCEGATE >> 4)) || lbi == BLK_COBBLEWALL_NORMAL >> 4) {
			lb = lbb;
			by--;
		}
	}
	if (mx != nx) mx = 0.;
	if (mz != nz) mz = 0.;
	if (my != ny) {
		if (lb != BLK_SLIME || entity->sneaking) {
			my = 0.;
		} else {
			my = -my;
		}
	}
	return ny != my || nx != mx || nz != mz;
}

void applyKnockback(struct entity* entity, float yaw, float strength) {
	float kb_resistance = 0.;
	if (randFloat() > kb_resistance) {
		float xr = sinf(yaw / 360. * 2 * M_PI);
		float zr = -cosf(yaw / 360. * 2 * M_PI);
		float m = sqrtf(xr * xr + zr * zr);
		entity->motX /= 2.;
		entity->motZ /= 2.;
		entity->motX -= xr / m * strength;
		entity->motZ -= zr / m * strength;
		if (entity->onGround) {
			entity->motY /= 2.;
			entity->motY += strength;
			if (entity->motY > .4) entity->motY = .4;
		}
		BEGIN_BROADCAST_DIST(entity, 128.)
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_ENTITYVELOCITY;
		pkt->data.play_client.entityvelocity.entity_id = entity->id;
		pkt->data.play_client.entityvelocity.velocity_x = (int16_t)(entity->motX * 8000.);
		pkt->data.play_client.entityvelocity.velocity_y = (int16_t)(entity->motY * 8000.);
		pkt->data.play_client.entityvelocity.velocity_z = (int16_t)(entity->motZ * 8000.);
		add_queue(bc_player->outgoingPacket, pkt);
		END_BROADCAST(entity->world->players)
	}
}

void damageEntityWithItem(struct entity* attacked, struct entity* attacker, uint8_t slot_index, struct slot* item) {
	if (attacked == NULL || attacked->invincibilityTicks > 0 || !isLiving(attacked->type) || attacked->health <= 0.) return;
	if (attacked->type == ENT_PLAYER && attacked->data.player.player->gamemode == 1) return;
	float damage = 1.;
	if (item != NULL) {
		struct item_info* ii = getItemInfo(item->item);
		if (ii != NULL) {
			damage = ii->damage;
			if (attacker->type == ENT_PLAYER) {
				if (ii->onItemAttacked != NULL) {
					damage = (*ii->onItemAttacked)(attacker->world, attacker->data.player.player, attacker->data.player.player->currentItem + 36, item, attacked);
				}
			}
		}
	}
	float knockback_strength = 1.;
	if (attacker->sprinting) knockback_strength++;
	if (attacker->type == ENT_PLAYER) {
		struct player* player = attacker->data.player.player;
		float as = player_getAttackStrength(player, .5);
		damage *= .2 + as * as * .8;
		knockback_strength *= .2 + as * as * .8;
	}
	if (attacker->y > attacked->y && (attacker->ly - attacker->y) > 0 && !attacker->onGround && !attacker->sprinting) { // todo water/ladder
		damage *= 1.5;
	}
	if (damage == 0.) return;
	damageEntity(attacked, damage, 1);
//TODO: enchantment
	knockback_strength *= .2;
	applyKnockback(attacked, attacker->yaw, knockback_strength);
}

void damageEntity(struct entity* attacked, float damage, int armorable) {
	if (attacked == NULL || damage <= 0. || attacked->invincibilityTicks > 0 || !isLiving(attacked->type) || attacked->health <= 0.) return;
	float armor = 0;
	if (attacked->type == ENT_PLAYER) {
		struct player* player = attacked->data.player.player;
		if (player->gamemode == 1) return;
		for (int i = 5; i <= 8; i++) {
			struct slot* sl = getSlot(player, player->inventory, i);
			if (sl != NULL) {
				struct item_info* ii = getItemInfo(sl->item);
				if (ii != NULL) {
					if ((i == 5 && ii->armorType == ARMOR_HELMET) || (i == 6 && ii->armorType == ARMOR_CHESTPLATE) || (i == 7 && ii->armorType == ARMOR_LEGGINGS) || (i == 8 && ii->armorType == ARMOR_BOOTS)) armor += (float) ii->armor;
				}
			}
		}
	}
	float f = armor - damage / 2.;
	if (f < armor * .2) f = armor * .2;
	if (f > 20.) f = 20.;
	damage *= 1. - (f) / 25.;
	if (attacked->type == ENT_PLAYER) {
		struct player* player = attacked->data.player.player;
		if (player->gamemode == 1) return;
		for (int i = 5; i <= 8; i++) {
			struct slot* sl = getSlot(player, player->inventory, i);
			if (sl != NULL) {
				struct item_info* ii = getItemInfo(sl->item);
				if (ii != NULL && ii->onEntityHitWhileWearing != NULL) {
					damage = (*ii->onEntityHitWhileWearing)(player->world, player, i, sl, damage);
				}
			}
		}
	}
	attacked->health -= damage;
	attacked->invincibilityTicks = 10;
	if (attacked->type == ENT_PLAYER) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_UPDATEHEALTH;
		pkt->data.play_client.updatehealth.health = attacked->health;
		pkt->data.play_client.updatehealth.food = attacked->data.player.player->food;
		pkt->data.play_client.updatehealth.food_saturation = attacked->data.player.player->saturation;
		add_queue(attacked->data.player.player->outgoingPacket, pkt);
	}
	if (attacked->health <= 0.) {
		if (attacked->type == ENT_PLAYER) {
			struct player* player = attacked->data.player.player;
			broadcastf("default", "%s died", player->name);
			for (size_t i = 0; i < player->inventory->slot_count; i++) {
				struct slot* slot = getSlot(player, player->inventory, i);
				if (slot != NULL) dropPlayerItem_explode(player, slot);
				setSlot(player, player->inventory, i, 0, 0, 1);
			}
			if (player->inHand != NULL) {
				dropPlayerItem_explode(player, player->inHand);
				freeSlot(player->inHand);
				xfree(player->inHand);
				player->inHand = NULL;
			}
			if (player->openInv != NULL) player_closeWindow(player, player->openInv->windowID);
		}
	}
	BEGIN_BROADCAST_DIST(attacked, 128.)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_ANIMATION;
	pkt->data.play_client.animation.entity_id = attacked->id;
	pkt->data.play_client.animation.animation = 1;
	add_queue(bc_player->outgoingPacket, pkt);
	if (attacked->health <= 0.) {
		pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
		pkt->data.play_client.entitymetadata.entity_id = attacked->id;
		writeMetadata(attacked, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
		add_queue(bc_player->outgoingPacket, pkt);
	}
	END_BROADCAST(attacked->world->players)
	if (attacked->type == ENT_PLAYER) playSound(attacked->world, 316, 8, attacked->x, attacked->y, attacked->z, 1., 1.);
}

void healEntity(struct entity* healed, float amount) {
	if (healed == NULL || amount <= 0. || healed->health <= 0. || !isLiving(healed->type)) return;
	float oh = healed->health;
	healed->health += amount;
	if (healed->health > healed->maxHealth) healed->health = 20.;
	if (healed->type == ENT_PLAYER && oh != healed->health) {
		struct packet* pkt = xmalloc(sizeof(struct packet));
		pkt->id = PKT_PLAY_CLIENT_UPDATEHEALTH;
		pkt->data.play_client.updatehealth.health = healed->health;
		pkt->data.play_client.updatehealth.food = healed->data.player.player->food;
		pkt->data.play_client.updatehealth.food_saturation = healed->data.player.player->saturation;
		add_queue(healed->data.player.player->outgoingPacket, pkt);
	}
}

int tick_itemstack(struct world* world, struct entity* entity) {
	if (entity->data.itemstack.delayBeforeCanPickup > 0) {
		entity->data.itemstack.delayBeforeCanPickup--;
		return 0;
	}
	if (entity->age >= 6000) {
		pthread_rwlock_unlock(&world->entities->data_mutex);
		despawnEntity(world, entity);
		pthread_rwlock_rdlock(&world->entities->data_mutex);
		freeEntity(entity);
		return 1;
	}
	if (tick_counter % 10 != 0) return 0;
	struct boundingbox cebb;
	getEntityCollision(entity, &cebb);
	cebb.minX -= .625;
	cebb.maxX += .625;
	cebb.maxY += .75;
	cebb.minZ -= .625;
	cebb.maxZ += .625;
	struct boundingbox oebb;
	//int32_t cx = ((int32_t) entity->x) >> 4;
	//int32_t cz = ((int32_t) entity->z) >> 4;
	//for (int32_t icx = cx - 1; icx <= cx + 1; icx++)
	//for (int32_t icz = cz - 1; icz <= cz + 1; icz++) {
	//struct chunk* ch = getChunk(entity->world, icx, icz);
	//if (ch != NULL) {
	BEGIN_HASHMAP_ITERATION(entity->world->entities)
	struct entity* oe = (struct entity*) value;
	if (oe == entity || entity_distsq(entity, oe) > 16. * 16.) continue;
	if (oe->type == ENT_PLAYER && oe->health > 0.) {
		getEntityCollision(oe, &oebb);
		//printf("%f, %f, %f vs %f, %f, %f\n", entity->x, entity->y, entity->z, oe->x, oe->y, oe->z);
		if (boundingbox_intersects(&oebb, &cebb)) {
			int os = entity->data.itemstack.slot->itemCount;
			pthread_rwlock_unlock(&world->entities->data_mutex);
			pthread_rwlock_unlock(&world->entities->data_mutex);
			pthread_mutex_lock(&oe->data.player.player->inventory->mut);
			int r = addInventoryItem_PI(oe->data.player.player, oe->data.player.player->inventory, entity->data.itemstack.slot, 1);
			pthread_mutex_unlock(&oe->data.player.player->inventory->mut);
			pthread_rwlock_rdlock(&world->entities->data_mutex);
			pthread_rwlock_rdlock(&world->entities->data_mutex);
			if (r <= 0) {
				BEGIN_BROADCAST_DIST(entity, 32.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_COLLECTITEM;
				pkt->data.play_client.collectitem.collected_entity_id = entity->id;
				pkt->data.play_client.collectitem.collector_entity_id = oe->id;
				pkt->data.play_client.collectitem.pickup_item_count = os - r;
				add_queue(bc_player->outgoingPacket, pkt);
				END_BROADCAST(entity->world->players)
				pthread_rwlock_unlock(&world->entities->data_mutex);
				pthread_rwlock_unlock(&world->entities->data_mutex);
				despawnEntity(world, entity);
				pthread_rwlock_rdlock(&world->entities->data_mutex);
				freeEntity(entity);
				return 1;
			} else {
				BEGIN_BROADCAST_DIST(entity, 128.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
				pkt->data.play_client.entitymetadata.entity_id = entity->id;
				writeMetadata(entity, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
				add_queue(bc_player->outgoingPacket, pkt);
				END_BROADCAST(entity->world->players)
			}
			break;
		}
	} else if (oe->type == ENT_ITEMSTACK) {
		if (oe->data.itemstack.slot->item == entity->data.itemstack.slot->item && oe->data.itemstack.slot->damage == entity->data.itemstack.slot->damage && oe->data.itemstack.slot->itemCount + entity->data.itemstack.slot->itemCount <= maxStackSize(entity->data.itemstack.slot)) {
			getEntityCollision(oe, &oebb);
			oebb.minX -= .625;
			oebb.maxX += .625;
			cebb.maxY += .75;
			oebb.minZ -= .625;
			oebb.maxZ += .625;
			if (boundingbox_intersects(&oebb, &cebb)) {
				pthread_rwlock_unlock(&world->entities->data_mutex);
				pthread_rwlock_unlock(&world->entities->data_mutex);
				despawnEntity(world, entity);
				pthread_rwlock_rdlock(&world->entities->data_mutex);
				oe->data.itemstack.slot->itemCount += entity->data.itemstack.slot->itemCount;
				freeEntity(entity);
				BEGIN_BROADCAST_DIST(oe, 128.)
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_ENTITYMETADATA;
				pkt->data.play_client.entitymetadata.entity_id = oe->id;
				writeMetadata(oe, &pkt->data.play_client.entitymetadata.metadata.metadata, &pkt->data.play_client.entitymetadata.metadata.metadata_size);
				add_queue(bc_player->outgoingPacket, pkt);
				END_BROADCAST(oe->world->players)
				return 1;
			}
		}
	}
	END_HASHMAP_ITERATION(entity->world->entities)
	return 0;
}

int entity_inBlock(struct entity* ent, block blk) { // blk = 0 for any block
	struct boundingbox obb;
	getEntityCollision(ent, &obb);
	if (obb.minX == obb.maxX || obb.minY == obb.maxY || obb.minZ == obb.maxZ) return 0;
	obb.minX += .01;
	obb.minY += .01;
	obb.minZ += .01;
	obb.maxX -= .01;
	obb.maxY -= .01;
	obb.maxZ -= .01;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(ent->world, x, y, z);
				if (b == 0 || (blk != 0 && blk != b)) continue;
				struct block_info* bi = getBlockInfo(b);
				if (bi == NULL) continue;
				for (size_t i = 0; i < bi->boundingBox_count; i++) {
					struct boundingbox* bb = &bi->boundingBoxes[i];
					struct boundingbox nbb;
					nbb.minX = bb->minX + (double) x;
					nbb.maxX = bb->maxX + (double) x;
					nbb.minY = bb->minY + (double) y;
					nbb.maxY = bb->maxY + (double) y;
					nbb.minZ = bb->minZ + (double) z;
					nbb.maxZ = bb->maxZ + (double) z;
					if (boundingbox_intersects(&nbb, &obb)) {
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

void pushOutOfBlocks(struct entity* ent) {
	if (!entity_inBlock(ent, 0)) return;
	struct boundingbox ebb;
	getEntityCollision(ent, &ebb);
	double x = ent->x;
	double y = (ebb.maxY + ebb.minY) / 2.;
	double z = ent->z;
	int32_t bx = (int32_t) x;
	int32_t by = (int32_t) y;
	int32_t bz = (int32_t) z;
	double dx = x - (double) bx;
	double dy = y - (double) by;
	double dz = z - (double) bz;
	double ld = 100.;
	float fbx = 0.;
	float fby = 0.;
	float fbz = 0.;
	if (dx < ld && !isNormalCube(getBlockInfo(getBlockWorld(ent->world, bx - 1, by, bz)))) {
		ld = dx;
		fbx = -1.;
		fby = 0.;
		fbz = 0.;
	}
	if ((1. - dx) < ld && !isNormalCube(getBlockInfo(getBlockWorld(ent->world, bx + 1, by, bz)))) {
		ld = 1. - dx;
		fbx = 1.;
		fby = 0.;
		fbz = 0.;
	}
	if (dz < ld && !isNormalCube(getBlockInfo(getBlockWorld(ent->world, bx, by, bz - 1)))) {
		ld = dx;
		fbx = 0.;
		fby = 0.;
		fbz = -1.;
	}
	if ((1. - dz) < ld && !isNormalCube(getBlockInfo(getBlockWorld(ent->world, bx, by, bz + 1)))) {
		ld = 1. - dz;
		fbx = 0.;
		fby = 0.;
		fbz = 1.;
	}
	if ((1. - dy) < ld && !isNormalCube(getBlockInfo(getBlockWorld(ent->world, bx, by + 1, bz)))) {
		ld = 1. - dy;
		fbx = 0.;
		fby = 1.;
		fbz = 0.;
	}
	float mag = randFloat() * .2 + .1;
	if (fbx == 0. && fbz == 0.) {
		ent->motX *= .75;
		ent->motY = mag * fby;
		ent->motZ *= .75;
	} else if (fbx == 0. && fby == 0.) {
		ent->motX *= .75;
		ent->motY *= .75;
		ent->motZ = mag * fbz;
	} else if (fbz == 0. && fby == 0.) {
		ent->motX = mag * fbx;
		ent->motY *= .75;
		ent->motZ *= .75;
	}
}

void tick_entity(struct world* world, struct entity* entity) {
	if (entity->type != ENT_PLAYER) {
		entity->lx = entity->x;
		entity->ly = entity->y;
		entity->lz = entity->z;
		entity->lyaw = entity->yaw;
		entity->lpitch = entity->pitch;
	}
	entity->age++;
	if (entity->invincibilityTicks > 0) entity->invincibilityTicks--;
	if (entity->type > ENT_PLAYER) {
		if (entity->type == ENT_ITEMSTACK) entity->motY -= 0.04;
		if (entity->motX != 0. || entity->motY != 0. || entity->motZ != 0.) moveEntity(entity, entity->motX, entity->motY, entity->motZ, 0.);
		double friction = .98;
		if (entity->onGround) {
			struct block_info* bi = getBlockInfo(getBlockWorld(entity->world, (int32_t) floor(entity->x), (int32_t) floor(entity->y) - 1, (int32_t) floor(entity->z)));
			if (bi != NULL) friction = bi->slipperiness * .98;
		}
		entity->motX *= friction;
		entity->motY *= .98;
		entity->motZ *= friction;
		if (fabs(entity->motX) < .0001) entity->motX = 0.;
		if (fabs(entity->motY) < .0001) entity->motY = 0.;
		if (fabs(entity->motZ) < .0001) entity->motZ = 0.;
		if (entity->onGround && entity->motX == 0. && entity->motY == 0. && entity->motZ == 0.) entity->motY *= -.5;
	}
	if (entity->type == ENT_ITEMSTACK) {
		if (tick_itemstack(world, entity)) return;
	}
	if (entity->type == ENT_ITEMSTACK || entity->type == ENT_EXPERIENCEORB) pushOutOfBlocks(entity);
	if (isLiving(entity->type)) {
		entity->inWater = entity_inFluid(entity, BLK_WATER, .4, 0) || entity_inFluid(entity, BLK_WATER_1, .4, 0);
		entity->inLava = entity_inFluid(entity, BLK_LAVA, .4, 0) || entity_inFluid(entity, BLK_LAVA_1, .4, 0);
		if ((entity->inWater || entity->inLava) && entity->type == ENT_PLAYER) entity->data.player.player->llTick = tick_counter;
		//TODO: ladders
		if (entity->type == ENT_PLAYER && entity->data.player.player->gamemode == 1) goto bfd;
		if (entity->inLava) {
			damageEntity(entity, 4., 1);
//TODO: fire
		}
		if (!entity->onGround) {
			float dy = entity->ly - entity->y;
			if (dy > 0.) {
				entity->fallDistance += dy;
			}
		} else if (entity->fallDistance > 0.) {
			if (entity->fallDistance > 3. && !entity->inWater && !entity->inLava) {
				damageEntity(entity, entity->fallDistance - 3., 0);
				playSound(entity->world, 312, 8, entity->x, entity->y, entity->z, 1., 1.);
			}
			entity->fallDistance = 0.;
		}
		bfd: ;
	}
	/*int32_t cx = ((int32_t) entity->x) >> 4;
	 int32_t cz = ((int32_t) entity->z) >> 4;
	 int32_t lcx = ((int32_t) entity->lx) >> 4;
	 int32_t lcz = ((int32_t) entity->lz) >> 4;
	 if (cx != lcx || cz != lcz) {
	 struct chunk* lch = getChunk(entity->world, lcx, lcz);
	 struct chunk* cch = getChunk(entity->world, cx, cz);
	 put_hashmap(lch->entities, entity->id, NULL);
	 put_hashmap(cch->entities, entity->id, entity);
	 }*/
}

void freeEntity(struct entity* entity) {
	del_hashmap(entity->loadingPlayers);
	if (entity->type == ENT_ITEMSTACK) {
		if (entity->data.itemstack.slot != NULL) {
			freeSlot(entity->data.itemstack.slot);
			xfree(entity->data.itemstack.slot);
		}
	}
	if (entity->type == ENT_PAINTING) {
		if (entity->data.painting.title != NULL) xfree(entity->data.painting.title);
	}
	xfree(entity);
}
