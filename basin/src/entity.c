/*
 * entity.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#include "entity.h"
#include <stdint.h>
#include <stdlib.h>
#include "globals.h"
#include <math.h>
#include "network.h"
#include "util.h"
#include "xstring.h"
#include "inventory.h"
#include "block.h"
#include <time.h>
#include "world.h"

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
	e->world = NULL;
	e->loadingPlayers = new_collection(0, 0);
	memset(&e->data, 0, sizeof(union entity_data));
	return e;
}

double entity_dist(struct entity* ent1, struct entity* ent2) {
	return sqrt((ent1->x - ent2->x) * (ent1->x - ent2->x) + (ent1->y - ent2->y) * (ent1->y - ent2->y) + (ent1->z - ent2->z) * (ent1->z - ent2->z));
}

double entity_distsq(struct entity* ent1, struct entity* ent2) {
	return (ent1->x - ent2->x) * (ent1->x - ent2->x) + (ent1->y - ent2->y) * (ent1->y - ent2->y) + (ent1->z - ent2->z) * (ent1->z - ent2->z);
}

double entity_distsq_block(struct entity* ent1, double x, double y, double z) {
	return (ent1->x - x) * (ent1->x - x) + (ent1->y - y) * (ent1->y - y) + (ent1->z - z) * (ent1->z - z);
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
	return type == ENT_SKELETON || type == ENT_SPIDER || type == ENT_GIANT || type == ENT_ZOMBIE || type == ENT_SLIME || type == ENT_GHAST || type == ENT_ZPIGMAN || type == ENT_ENDERMAN || type == ENT_CAVESPIDER || type == ENT_SILVERFISH || type == ENT_BLAZE || type == ENT_MAGMACUBE || type == ENT_ENDERDRAGON || type == ENT_WITHER || type == ENT_BAT || type == ENT_WITCH || type == ENT_ENDERMITE || type == ENT_GUARDIAN || type == ENT_SHULKER || type == ENT_PIG || type == ENT_SHEEP || type == ENT_COW || type == ENT_CHICKEN || type == ENT_SQUID || type == ENT_WOLF || type == ENT_MOOSHROOM || type == ENT_SNOWMAN || type == ENT_OCELOT || type == ENT_IRONGOLEM || type == ENT_HORSE || type == ENT_RABBIT || type == ENT_VILLAGER || type == ENT_BOAT;
}

void outputMetaByte(struct entity* ent, unsigned char** loc, size_t* size) {
	*loc = xrealloc(*loc, *size + 3);
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
		memcpy(*loc + *size, &ent->health, 4); // TODO: swap endian maybe?
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

void moveEntity(struct entity* entity) {
	struct boundingbox obb;
	getEntityCollision(entity, &obb);
	if (obb.minX == obb.maxX || obb.minZ == obb.maxZ || obb.minY == obb.maxY) {
		entity->x += entity->motX;
		entity->y += entity->motY;
		entity->z += entity->motZ;
		return;
	}
	if (entity->motX < 0.) {
		obb.minX += entity->motX;
	} else {
		obb.maxX += entity->motX;
	}
	if (entity->motY < 0.) {
		obb.minY += entity->motY;
	} else {
		obb.maxY += entity->motY;
	}
	if (entity->motZ < 0.) {
		obb.minZ += entity->motZ;
	} else {
		obb.maxZ += entity->motZ;
	}
	struct boundingbox pbb;
	getEntityCollision(entity, &pbb);
	double ny = entity->motY;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				struct boundingbox bb = getBlockCollision(entity->world, x, y, z, entity);
				if (b > 0 && bb.minX != bb.maxX && bb.minY != bb.maxY && bb.minZ != bb.maxZ) {
					if (bb.maxX + x > obb.minX && bb.minX + x < obb.maxX ? (bb.maxY + y > obb.minY && bb.minY + y < obb.maxY ? bb.maxZ + z > obb.minZ && bb.minZ + z < obb.maxZ : 0) : 0) {
						if (pbb.maxX > bb.minX + x && pbb.minX < bb.maxX + x && pbb.maxZ > bb.minZ + z && pbb.minZ < bb.maxZ + z) {
							double t;
							if (ny > 0. && pbb.maxY <= bb.minY + y) {
								t = bb.minY + y - pbb.maxY;
								if (t < ny) {
									ny = t;
								}
							} else if (ny < 0. && pbb.minY >= bb.maxY + y) {
								t = bb.maxY + y - pbb.minY;
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
	entity->y += ny;
	pbb.minY += ny;
	pbb.maxY += ny;
	double nx = entity->motX;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				struct boundingbox bb = getBlockCollision(entity->world, x, y, z, entity);
				if (b > 0 && bb.minX != bb.maxX && bb.minY != bb.maxY && bb.minZ != bb.maxZ) {
					if (bb.maxX + x > obb.minX && bb.minX + x < obb.maxX ? (bb.maxY + y > obb.minY && bb.minY + y < obb.maxY ? bb.maxZ + z > obb.minZ && bb.minZ + z < obb.maxZ : 0) : 0) {
						if (pbb.maxY > bb.minY + y && pbb.minY < bb.maxY + y && pbb.maxZ > bb.minZ + z && pbb.minZ < bb.maxZ + z) {
							double t;
							if (nx > 0. && pbb.maxX <= bb.minX + x) {
								t = bb.minX + x - pbb.maxX;
								if (t < nx) {
									nx = t;
								}
							} else if (nx < 0. && pbb.minX >= bb.maxX + x) {
								t = bb.maxX + x - pbb.minX;
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
	entity->x += nx;
	pbb.minX += nx;
	pbb.maxX += nx;
	double nz = entity->motZ;
	for (int32_t x = floor(obb.minX); x < floor(obb.maxX + 1.); x++) {
		for (int32_t z = floor(obb.minZ); z < floor(obb.maxZ + 1.); z++) {
			for (int32_t y = floor(obb.minY); y < floor(obb.maxY + 1.); y++) {
				block b = getBlockWorld(entity->world, x, y, z);
				struct boundingbox bb = getBlockCollision(entity->world, x, y, z, entity);
				if (b > 0 && bb.minX != bb.maxX && bb.minY != bb.maxY && bb.minZ != bb.maxZ) {
					if (bb.maxX + x > obb.minX && bb.minX + x < obb.maxX ? (bb.maxY + y > obb.minY && bb.minY + y < obb.maxY ? bb.maxZ + z > obb.minZ && bb.minZ + z < obb.maxZ : 0) : 0) {
						if (pbb.maxX > bb.minX + x && pbb.minX < bb.maxX + x && pbb.maxY > bb.minY + y && pbb.minY < bb.maxY + y) {
							double t;
							if (nz > 0. && pbb.maxZ <= bb.minZ + z) {
								t = bb.minZ + z - pbb.maxZ;
								if (t < nz) {
									nz = t;
								}
							} else if (nz < 0. && pbb.minZ >= bb.maxZ + z) {
								t = bb.maxZ + z - pbb.minZ;
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
	//TODO step
	entity->z += nz;
	pbb.minZ += nz;
	pbb.maxZ += nz;
	entity->collidedHorizontally = entity->motX != nx || entity->motZ != nz;
	entity->collidedVertically = entity->motY != ny;
	entity->onGround = entity->collidedVertically && entity->motY < 0.;
	int32_t bx = floor(entity->x);
	int32_t by = floor(entity->y - .20000000298023224);
	int32_t bz = floor(entity->z);
	block lb = getBlockWorld(entity->world, bx, by, bz);
	if (lb == BLK_AIR) {
		block lbb = getBlockWorld(entity->world, bx, by - 1, bz);
		uint16_t lbi = lbb >> 4;
		if ((lbi >= (BLK_FENCE >> 4) && lbi <= (BLK_ACACIAFENCE >> 4)) || (lbi >= (BLK_FENCEGATE >> 4) && lbi <= (BLK_ACACIAFENCEGATE_14 >> 4)) || lbi == BLK_COBBLEWALL_NORMAL >> 4) {
			lb = lbb;
			by--;
		}
	}
	if (entity->motX != nx) entity->motX = 0.;
	if (entity->motZ != nz) entity->motZ = 0.;
	if (entity->motY != ny) {
		if (lb != BLK_SLIME || entity->sneaking) {
			entity->motY = 0.;
		} else {
			entity->motY = -entity->motY;
		}
	}
}

void freeEntity(struct entity* entity) {
	del_collection(entity->loadingPlayers);
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
