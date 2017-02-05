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
#include <fcntl.h>
#include "json.h"
#include <errno.h>

struct entity_info* getEntityInfo(uint32_t id) {
	if (id < 0 || id > entity_infos->size) return NULL;
	return entity_infos->data[id];
}

void add_entity_info(uint32_t eid, struct entity_info* bm) {
	ensure_collection(entity_infos, eid + 1);
	entity_infos->data[eid] = bm;
	if (entity_infos->size < eid) entity_infos->size = eid;
	entity_infos->count++;
}

void init_entities() {
	entity_infos = new_collection(128, 0);
	char* jsf = xmalloc(4097);
	size_t jsfs = 4096;
	size_t jsr = 0;
	int fd = open("entities.json", O_RDONLY);
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
		printf("Error reading entity information: %s\n", strerror(errno));
	}
	close(fd);
	struct json_object json;
	parseJSON(&json, jsf);
	for (size_t i = 0; i < json.child_count; i++) {
		struct json_object* ur = json.children[i];
		struct entity_info* ei = xcalloc(sizeof(struct entity_info));
		struct json_object* tmp = getJSONValue(ur, "id");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		uint32_t id = (block) tmp->data.number;
		if (id < 0) goto cerr;
		tmp = getJSONValue(ur, "maxHealth");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ei->maxHealth = (float) tmp->data.number;
		tmp = getJSONValue(ur, "width");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ei->width = (float) tmp->data.number;
		tmp = getJSONValue(ur, "height");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ei->height = (float) tmp->data.number;
		tmp = getJSONValue(ur, "loot");
		if (tmp != NULL && tmp->type == JSON_ARRAY) {
			ei->loot_count = tmp->child_count;
			ei->loots = xcalloc(tmp->child_count * sizeof(struct entity_loot));
			for (size_t i = 0; i < tmp->child_count; i++) {
				struct json_object* fj = tmp->children[i];
				if (fj == NULL || fj->type != JSON_OBJECT) {
					xfree(ei->loots);
					goto cerr;
				}
				struct json_object* emp = getJSONValue(fj, "id");
				if (emp == NULL || emp->type != JSON_NUMBER) {
					xfree(ei->loots);
					goto cerr;
				}
				ei->loots[i].id = (item) emp->data.number;
				struct json_object* amin = getJSONValue(fj, "amountMin");
				struct json_object* amax = getJSONValue(fj, "amountMax");
				if ((amin == NULL) != (amax == NULL) || (amin != NULL && (amin->type != JSON_NUMBER || amax->type != JSON_NUMBER))) {
					xfree(ei->loots);
					goto cerr;
				}
				if (amin != NULL) {
					ei->loots[i].amountMin = (uint8_t) amin->data.number;
					ei->loots[i].amountMax = (uint8_t) amax->data.number;
				}
				amin = getJSONValue(fj, "metaMin");
				amax = getJSONValue(fj, "metaMax");
				if ((amin == NULL) != (amax == NULL) || (amin != NULL && (amin->type != JSON_NUMBER || amax->type != JSON_NUMBER))) {
					xfree(ei->loots);
					goto cerr;
				}
				if (amin != NULL) {
					ei->loots[i].metaMin = (uint8_t) amin->data.number;
					ei->loots[i].metaMax = (uint8_t) amax->data.number;
				}
			}
		}
		tmp = getJSONValue(ur, "flags");
		if (tmp == NULL || tmp->type != JSON_ARRAY) goto cerr;
		ei->flag_count = tmp->child_count;
		ei->flags = xmalloc(tmp->child_count * sizeof(char*));
		for (size_t i = 0; i < tmp->child_count; i++) {
			struct json_object* fj = tmp->children[i];
			if (fj == NULL || fj->type != JSON_STRING) goto cerr;
			ei->flags[i] = toLowerCase(xstrdup(trim(fj->data.string), 0));
		}
		tmp = getJSONValue(ur, "packetType");
		if (tmp == NULL || tmp->type != JSON_STRING) goto cerr;
		if (streq_nocase(tmp->data.string, "mob")) ei->spawn_packet = PKT_PLAY_CLIENT_SPAWNMOB;
		else if (streq_nocase(tmp->data.string, "object")) ei->spawn_packet = PKT_PLAY_CLIENT_SPAWNOBJECT;
		else if (streq_nocase(tmp->data.string, "exp")) ei->spawn_packet = PKT_PLAY_CLIENT_SPAWNEXPERIENCEORB;
		else if (streq_nocase(tmp->data.string, "painting")) ei->spawn_packet = PKT_PLAY_CLIENT_SPAWNPAINTING;
		tmp = getJSONValue(ur, "packetID");
		if (tmp == NULL || tmp->type != JSON_NUMBER) goto cerr;
		ei->spawn_packet_id = (int32_t) tmp->data.number;
		tmp = getJSONValue(ur, "dataname");
		if (tmp == NULL || tmp->type != JSON_STRING) goto cerr;
		ei->dataname = xstrdup(tmp->data.string, 0);
		add_entity_info(id, ei);
		continue;
		cerr: ;
		printf("[WARNING] Error Loading Entity \"%s\"! Skipped.\n", ur->name);
	}
	freeJSON(&json);
	xfree(jsf);
}

uint32_t getIDFromEntityDataName(const char* dataname) {
	for (size_t i = 0; i < entity_infos->size; i++) {
		struct entity_info* ei = entity_infos->data[i];
		if (ei == NULL) continue;
		if (streq_nocase(ei->dataname, dataname)) return i;
	}
	return -1;
}

void getEntityCollision(struct entity* ent, struct boundingbox* bb) {
	struct entity_info* ei = getEntityInfo(ent->type);
	bb->minX = ent->x - ei->width / 2;
	bb->maxX = ent->x + ei->width / 2;
	bb->minZ = ent->z - ei->width / 2;
	bb->maxZ = ent->z + ei->width / 2;
	bb->minY = ent->y;
	bb->maxY = ent->y + ei->height;
}

struct entity* newEntity(int32_t id, float x, float y, float z, uint32_t type, float yaw, float pitch) {
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
	if ((ent->type == ENT_SKELETON || ent->type == ENT_SLIME || ent->type == ENT_LAVASLIME || ent->type == ENT_GUARDIAN) && index == 11) {
		ent->subtype = i;
	}
}

void handleMetaFloat(struct entity* ent, int index, float f) {

}

void handleMetaString(struct entity* ent, int index, char* str) {

	xfree(str);
}

void handleMetaSlot(struct entity* ent, int index, struct slot* slot) {
	if (ent->type == ENT_ITEM && index == 6) {
		ent->data.itemstack.slot = slot;
	}
}

void handleMetaVector(struct entity* ent, int index, float f1, float f2, float f3) {

}

void handleMetaPosition(struct entity* ent, int index, struct encpos* pos) {

}

void handleMetaUUID(struct entity* ent, int index, struct uuid* pos) {

}

int hasFlag(struct entity_info* ei, char* flag) {
	for (size_t i = 0; i < ei->flag_count; i++)
		if (streq_nocase(ei->flags[i], flag)) return 1;
	return 0;
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
	if ((ent->type == ENT_SKELETON || ent->type == ENT_SLIME || ent->type == ENT_LAVASLIME || ent->type == ENT_GUARDIAN)) {
		*loc = xrealloc(*loc, *size + 2 + getVarIntSize(ent->subtype));
		(*loc)[(*size)++] = 11;
		(*loc)[(*size)++] = 1;
		*size += writeVarInt(ent->subtype, *loc + *size);
	}
}

void outputMetaFloat(struct entity* ent, unsigned char** loc, size_t* size) {
	if (hasFlag(getEntityInfo(ent->type), "livingbase")) {
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
	if (ent->type == ENT_ITEM) {
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

int moveEntity(struct entity* entity, double* mx, double* my, double* mz, float shrink) {
	struct boundingbox obb;
	getEntityCollision(entity, &obb);
	if (obb.minX == obb.maxX || obb.minZ == obb.maxZ || obb.minY == obb.maxY) {
		entity->x += *mx;
		entity->y += *my;
		entity->z += *mz;
		return 0;
	}
	obb.minX += shrink;
	obb.maxX -= shrink;
	obb.minY += shrink;
	obb.maxY -= shrink;
	obb.minZ += shrink;
	obb.maxZ -= shrink;
	if (*mx < 0.) {
		obb.minX += *mx;
	} else {
		obb.maxX += *mx;
	}
	if (*my < 0.) {
		obb.minY += *my;
	} else {
		obb.maxY += *my;
	}
	if (*mz < 0.) {
		obb.minZ += *mz;
	} else {
		obb.maxZ += *mz;
	}
	struct boundingbox pbb;
	getEntityCollision(entity, &pbb);
	pbb.minX += shrink;
	pbb.maxX -= shrink;
	pbb.minY += shrink;
	pbb.maxY -= shrink;
	pbb.minZ += shrink;
	pbb.maxZ -= shrink;
	double ny = *my;
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
	double nx = *mx;
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
	double nz = *mz;
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
	entity->collidedHorizontally = *mx != nx || *mz != nz;
	entity->collidedVertically = *my != ny;
	entity->onGround = entity->collidedVertically && *my < 0.;
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
	if (*mx != nx) *mx = 0.;
	if (*mz != nz) *mz = 0.;
	if (*my != ny) {
		if (lb != BLK_SLIME || entity->sneaking) {
			*my = 0.;
		} else {
			*my = -(*my);
		}
	}
	return ny != *my || nx != *mx || nz != *mz;
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
	if (attacked == NULL || attacked->invincibilityTicks > 0 || !hasFlag(getEntityInfo(attacked->type), "livingbase") || attacked->health <= 0.) return;
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
	if (attacked == NULL || damage <= 0. || attacked->invincibilityTicks > 0 || !hasFlag(getEntityInfo(attacked->type), "livingbase") || attacked->health <= 0.) return;
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
	if (healed == NULL || amount <= 0. || healed->health <= 0. || !hasFlag(getEntityInfo(healed->type), "livingbase")) return;
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
	} else if (oe->type == ENT_ITEM) {
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
		if (entity->type == ENT_ITEM) entity->motY -= 0.04;
		else if (hasFlag(getEntityInfo(entity->type), "livingbase")) entity->motY -= .08;

		if (entity->motX != 0. || entity->motY != 0. || entity->motZ != 0.) moveEntity(entity, &entity->motX, &entity->motY, &entity->motZ, 0.);
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
		if (entity->type == ENT_ITEM && entity->onGround && entity->motX == 0. && entity->motY == 0. && entity->motZ == 0.) entity->motY *= -.5;
		if (entity->type == ENT_CREEPER) printf("onGround = %i, motY = %f\n", entity->onGround, entity->motY);
	}
	if (entity->type == ENT_ITEM) {
		if (tick_itemstack(world, entity)) return;
	}
	if (entity->type == ENT_ITEM || entity->type == ENT_XPORB) pushOutOfBlocks(entity);
	if (hasFlag(getEntityInfo(entity->type), "livingbase")) {
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
	if (entity->type == ENT_ITEM) {
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
