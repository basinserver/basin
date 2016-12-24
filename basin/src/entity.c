/*
 * entity.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#include "entity.h"
#include <stdint.h>
#include <stdlib.h>
#include <GL/gl.h>
#include "globals.h"
#include <math.h>
#include "network.h"
#include "util.h"
#include "xstring.h"

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
		else if (id == 70) return ENT_FALLINGBLOCK;
		else if (id == 71) return ENT_ITEMFRAME;
		else if (id == 72) return ENT_EYEENDER;
		else if (id == 73) return ENT_THROWNPOTION;
		else if (id == 74) return ENT_FALLINGEGG;
		else if (id == 75) return ENT_EXPBOTTLE;
		else if (id == 76) return ENT_FIREWORK;
		else if (id == 77) return ENT_LEASHKNOT;
		else if (id == 78) return ENT_ARMORSTAND;
		else if (id == 90) return ENT_FISHINGFLOAT;
		else if (id == 91) return ENT_SPECTRALARROW;
		else if (id == 92) return ENT_TIPPEDARROW;
		else if (id == 93) return ENT_DRAGONFIREBALL;
	} else if (type == 1) {
		if (id == 50) return ENT_CREEPER;
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
		else if (id == 120) return ENT_VILLAGER;
	}
	return -1;
}

struct entity* newEntity(int32_t id, float x, float y, float z, uint8_t type, float yaw, float pitch) {
	struct entity* e = malloc(sizeof(struct entity));
	e->id = id;
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
	e->sneaking = 0;
	e->sprinting = 0;
	e->usingItem = 0;
	e->portalCooldown = 0;
	e->ticksExisted = 0;
	e->subtype = 0;
	e->fallDistance = 0.;
	e->maxHealth = 20;
	e->world = NULL;
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

struct boundingbox no_bb;
struct boundingbox bb_player;
struct boundingbox bb_skeleton;

void load_entities() {
	no_bb.minX = 0.;
	no_bb.minY = 0.;
	no_bb.minZ = 0.;
	no_bb.maxX = 0.;
	no_bb.maxY = 0.;
	no_bb.maxZ = 0.;
	bb_player.minX = -.3;
	bb_player.minY = 0.;
	bb_player.minZ = -.3;
	bb_player.maxX = .3;
	bb_player.maxY = 1.8;
	bb_player.maxZ = .3;
//
	bb_skeleton.minX = -.3;
	bb_skeleton.minY = 0.;
	bb_skeleton.minZ = -.3;
	bb_skeleton.maxX = .3;
	bb_skeleton.maxY = 1.95;
	bb_skeleton.maxZ = .3;
//
}

struct boundingbox* getEntityCollision(struct entity* ent) {
	if (ent->type == ENT_PLAYER || ent->type == ENT_CREEPER || ent->type == ENT_ZOMBIE || ent->type == ENT_ZPIGMAN || ent->type == ENT_BLAZE || ent->type == ENT_WITCH || ent->type == ENT_VILLAGER) return &bb_player;
	else if (ent->type == ENT_SKELETON) return &bb_skeleton;
	return &no_bb;
}

void freeEntity(struct entity* entity) {
	free(entity);
}
