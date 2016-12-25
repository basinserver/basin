/*
 * world.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */
#include "world.h"
#include "entity.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "collection.h"
#include <pthread.h>
#include "player.h"
#include "nbt.h"
#include <linux/limits.h>
#include <fcntl.h>
#include <stdio.h>
#include "util.h"
#include <unistd.h>
#include <sys/mman.h>
#include <zlib.h>
#include <dirent.h>
#include "worldmanager.h"
#include "network.h"
#include "game.h"

struct chunk* loadRegionChunk(struct region* region, int8_t lchx, int8_t lchz) {
	if (region->fd < 0) {
		region->fd = open(region->file, O_RDWR);
		if (region->fd < 0) return NULL;
		region->mapping = mmap(NULL, 67108864, PROT_READ | PROT_WRITE, MAP_SHARED, region->fd, 0); // 64 MB is the theoretical limit of an uncompressed region file
	}
	uint32_t* hf = region->mapping + 4 * ((lchx & 31) + (lchz & 31) * 32);
	uint32_t rhf = *hf;
	swapEndian(&rhf, 4);
	uint32_t off = ((rhf & 0xFFFFFF00) >> 8) * 4096;
	uint32_t size = ((rhf & 0x000000FF)) * 4096;
	if (off == 0 || size == 0) return NULL;
	void* chk = region->mapping + (off);
	uint32_t rsize = ((uint32_t*) chk)[0];
	swapEndian(&rsize, 4);
	uint8_t comptype = ((uint8_t*) chk)[4];
	chk += 5;
	void* rtbuf = xmalloc(4096);
	size_t rtc = 4096;
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	int dr = 0;
	if ((dr = inflateInit2(&strm, (32 + MAX_WBITS))) != Z_OK) { //
		printf("Compression initialization error!\n");
		xfree(rtbuf);
		return NULL;
	}
	strm.avail_in = rsize;
	strm.next_in = chk;
	strm.avail_out = rtc;
	strm.next_out = rtbuf;
	do {
		if (rtc - strm.total_out < 2048) {
			rtc += 4096;
			rtbuf = xrealloc(rtbuf, rtc);
		}
		strm.avail_out = rtc - strm.total_out;
		strm.next_out = rtbuf + strm.total_out;
		dr = inflate(&strm, Z_FINISH);
		if (dr == Z_STREAM_ERROR) {
			printf("Compression Read Error!\n");
			inflateEnd(&strm);
			xfree(rtbuf);
			return NULL;
		}
	} while (strm.avail_out == 0);
	inflateEnd(&strm);
	size_t rts = strm.total_out;
	struct nbt_tag* nbt = NULL;
	if (readNBT(&nbt, rtbuf, rts) < 0) {
		xfree(rtbuf);
		return NULL;
	}
	xfree(rtbuf);
	struct nbt_tag* level = getNBTChild(nbt, "Level");
	if (level == NULL || level->id != NBT_TAG_COMPOUND) goto rerx;
	struct nbt_tag* tmp = getNBTChild(level, "xPos");
	if (tmp == NULL || tmp->id != NBT_TAG_INT) goto rerx;
	int32_t xPos = tmp->data.nbt_int;
	tmp = getNBTChild(level, "zPos");
	if (tmp == NULL || tmp->id != NBT_TAG_INT) goto rerx;
	int32_t zPos = tmp->data.nbt_int;
	struct chunk* rch = newChunk(xPos, zPos);
	tmp = getNBTChild(level, "LightPopulated");
	if (tmp == NULL || tmp->id != NBT_TAG_BYTE) goto rerx;
	rch->lightpopulated = tmp->data.nbt_byte;
	tmp = getNBTChild(level, "TerrainPopulated");
	if (tmp == NULL || tmp->id != NBT_TAG_BYTE) goto rerx;
	rch->terrainpopulated = tmp->data.nbt_byte;
	tmp = getNBTChild(level, "InhabitedTime");
	if (tmp == NULL || tmp->id != NBT_TAG_LONG) goto rerx;
	rch->inhabitedticks = tmp->data.nbt_long;
	tmp = getNBTChild(level, "Biomes");
	if (tmp != NULL && tmp->id != NBT_TAG_BYTEARRAY) goto rerx;
	if (tmp->data.nbt_bytearray.len == 256) memcpy(rch->biomes, tmp->data.nbt_bytearray.data, 256);
	tmp = getNBTChild(level, "HeightMap");
	if (tmp != NULL && tmp->id != NBT_TAG_INTARRAY) goto rerx;
	if (tmp->data.nbt_intarray.count == 256) for (int i = 0; i < 256; i++)
		rch->heightMap[i >> 4][i & 0x0F] = (uint16_t) tmp->data.nbt_intarray.ints[i];
	struct nbt_tag* sections = getNBTChild(level, "Sections");
	for (size_t i = 0; i < sections->children_count; i++) {
		struct nbt_tag* section = sections->children[i];
		tmp = getNBTChild(section, "Y");
		if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
		uint8_t y = tmp->data.nbt_byte;
		tmp = getNBTChild(section, "Blocks");
		if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 4096) continue;
		int hna = 0;
		for (int i = 0; i < 4096; i++) {
			rch->blocks[(i >> 8) + (y * 16)][(i & 0xf0) >> 4][i & 0x0f] = ((block) tmp->data.nbt_bytearray.data[i]) << 4;
			if (((block) tmp->data.nbt_bytearray.data[i]) != 0) hna = 1;
		}
		if (hna) rch->empty[y] = 0;
		tmp = getNBTChild(section, "Add");
		if (tmp != NULL) {
			if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
			for (int i = 0; i < 4096; i++) {
				block sx = tmp->data.nbt_bytearray.data[i / 2];
				if (i % 2 == 0) {
					sx &= 0xf0;
					sx >>= 4;
				} else sx &= 0x0f;
				sx <<= 8;
				rch->blocks[(i >> 8) + (y * 16)][(i & 0xf0) >> 4][i & 0x0f] |= sx;
			}
		}
		tmp = getNBTChild(section, "Data");
		if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
		for (int i = 0; i < 4096; i++) {
			block sx = tmp->data.nbt_bytearray.data[i / 2];
			if (i % 2 == 1) {
				sx &= 0xf0;
				sx >>= 4;
			} else sx &= 0x0f;
			rch->blocks[(i >> 8) + (y * 16)][(i & 0xf0) >> 4][i & 0x0f] |= sx;
		}
		tmp = getNBTChild(section, "BlockLight");
		if (tmp != NULL) {
			if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
			memcpy(rch->blockLight, tmp->data.nbt_bytearray.data, 2048);
		}
		tmp = getNBTChild(section, "SkyLight");
		if (tmp != NULL) {
			if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
			rch->skyLight = xmalloc(2048);
			memcpy(rch->skyLight, tmp->data.nbt_bytearray.data, 2048);
		}
	}
	//TODO: entities, tileentities, and tileticks.
	region->chunks[lchz][lchx] = rch;
	region->loaded[lchz][lchx] = 1;
	freeNBT(nbt);
	return rch;
	rerx: ;
	freeNBT(nbt);
	return NULL;
}

void generateChunk(struct world* world, struct chunk* chunk) {
	if (world->dimension == OVERWORLD) {
		chunk->skyLight = xmalloc(2048);
		memset(chunk->skyLight, 0xFF, 2048);
	}
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			setBlockChunk(chunk, 1 << 4, x, 0, z);
		}
	}
}

struct chunk* getChunk(struct world* world, int16_t x, int16_t z) {
	int16_t rx = x >> 5;
	int16_t rz = z >> 5;
	pthread_rwlock_rdlock(&world->regions->data_mutex);
	struct region* ar = NULL;
	for (size_t i = 0; i < world->regions->size; i++) {
		if (world->regions->data[i] != NULL && ((struct region*) world->regions->data[i])->x == rx && ((struct region*) world->regions->data[i])->z == rz) {
			ar = world->regions->data[i];
			struct region* r = ((struct region*) world->regions->data[i]);
			struct chunk* chk = NULL;
			if (r->loaded[z & 0x1F][x & 0x1F]) {
				chk = r->chunks[z & 0x1F][x & 0x1F];
			} else {
				chk = loadRegionChunk(r, x & 0x1F, z & 0x1F);
			}
			if (chk == NULL) goto gen;
			pthread_rwlock_unlock(&world->regions->data_mutex);
			return chk;
		}
	}
	gen: ;
	pthread_rwlock_unlock(&world->regions->data_mutex);
	struct chunk* gc = newChunk(x, z);
	generateChunk(world, gc);
	if (!ar) {
		char lp[PATH_MAX];
		snprintf(lp, PATH_MAX, "%s/region/r.%i.%i.mca", world->lpa, rx, rz);
		ar = newRegion(lp, rx, rz);
		add_collection(world->regions, ar);
	}
	ar->chunks[z & 0x1F][x & 0x1F] = gc;
	ar->loaded[z & 0x1F][x & 0x1F] = 1;
	return gc;
}

void unloadChunk(struct world* world, struct chunk* chunk) {
	//TODO: save chunk
	int16_t rx = chunk->x >> 5;
	int16_t rz = chunk->z >> 5;
	pthread_rwlock_rdlock(&world->regions->data_mutex);
	for (size_t i = 0; i < world->regions->size; i++) {
		if (world->regions->data[i] != NULL && ((struct region*) world->regions->data[i])->x == rx && ((struct region*) world->regions->data[i])->z == rz) {
			struct region* r = ((struct region*) world->regions->data[i]);
			r->loaded[chunk->z & 0x1F][chunk->x & 0x1F] = 0;
			r->chunks[chunk->z & 0x1F][chunk->x & 0x1F] = 0;
			freeChunk(chunk);
			pthread_rwlock_unlock(&world->regions->data_mutex);
			return;
		}
	}
	pthread_rwlock_unlock(&world->regions->data_mutex);

}

int getBiome(struct world* world, int32_t x, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 0;
	return chunk->biomes[z & 0x0f][x & 0x0f];
}

block getBlockChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
	return chunk->blocks[y][z][x];
}

block getBlockWorld(struct world* world, int32_t x, uint8_t y, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 0;
	return getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
}

struct chunk* newChunk(int16_t x, int16_t z) {
	struct chunk* chunk = xmalloc(sizeof(struct chunk));
	memset(chunk, 0, sizeof(struct chunk));
	chunk->x = x;
	chunk->z = z;
	for (int i = 0; i < 16; i++)
		chunk->empty[i] = 1;
	chunk->playersLoaded = 0;
	return chunk;
}

void freeChunk(struct chunk* chunk) {
	if (chunk->skyLight != NULL) xfree(chunk->skyLight);
	if (chunk->tileEntities != NULL) {
		for (size_t i = 0; i < chunk->tileEntity_count; i++) {
			freeNBT(chunk->tileEntities[i]);
			xfree(chunk->tileEntities[i]);
		}
		xfree(chunk->tileEntities);
	}
	xfree(chunk);
}
struct region* newRegion(char* path, int16_t x, int16_t z) {
	struct region* reg = xmalloc(sizeof(struct region));
	memset(reg, 0, sizeof(struct region));
	reg->x = x;
	reg->z = z;
	reg->file = xstrdup(path, 0);
	reg->fd = -1;
	return reg;
}

void freeRegion(struct region* region) {
	xfree(region->file);
	xfree(region);
}

void setBlockChunk(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
	chunk->blocks[y][z][x] = blk;
	if (blk != 0) chunk->empty[y >> 4] = 0;
}

void setBlockWorld(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) {
		return;
	}
	setBlockChunk(chunk, blk, x & 0x0f, y, z & 0x0f);
	BEGIN_BROADCAST_DISTXYZ((double) x + .5, (double) y + .5, (double) z + .5, world->players, 128.)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_BLOCKCHANGE;
	pkt->data.play_client.blockchange.location.x = x;
	pkt->data.play_client.blockchange.location.y = y;
	pkt->data.play_client.blockchange.location.z = z;
	pkt->data.play_client.blockchange.block_id = blk;
	add_queue(bc_player->conn->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST

}

struct world* newWorld() {
	struct world* world = xmalloc(sizeof(struct world));
	memset(world, 0, sizeof(struct world));
	world->regions = new_collection(0);
	world->entities = new_collection(0);
	world->players = new_collection(0);
	return world;
}

int loadWorld(struct world* world, char* path) {
	char lp[PATH_MAX];
	snprintf(lp, PATH_MAX, "%s/level.dat", path); // could have a double slash, but its okay
	int fd = open(lp, O_RDONLY);
	if (fd < 0) return -1;
	unsigned char* ld = xmalloc(1024);
	size_t ldc = 1024;
	size_t ldi = 0;
	ssize_t i = 0;
	while ((i = read(fd, ld + ldi, ldc - ldi)) > 0) {
		if (ldc - ldi < 512) {
			ldc += 1024;
			ld = xrealloc(ld, ldc);
		}
		ldi += i;
	}
	close(fd);
	if (i < 0) {
		xfree(ld);
		return -1;
	}
	unsigned char* nld = NULL;
	ssize_t ds = decompressNBT(ld, ldi, (void**) &nld);
	xfree(ld);
	if (ds < 0) {
		return -1;
	}
	if (readNBT(&world->level, nld, ds) < 0) return -1;
	xfree(nld);
	struct nbt_tag* data = world->level->children[0];
	world->levelType = getNBTChild(data, "generatorName")->data.nbt_string;
	world->spawnpos.x = getNBTChild(data, "SpawnX")->data.nbt_int;
	world->spawnpos.y = getNBTChild(data, "SpawnY")->data.nbt_int;
	world->spawnpos.z = getNBTChild(data, "SpawnZ")->data.nbt_int;
	world->lpa = xstrdup(path, 0);
	printf("spawn: %i, %i, %i\n", world->spawnpos.x, world->spawnpos.y, world->spawnpos.z);
	snprintf(lp, PATH_MAX, "%s/region/", path);
	DIR* dir = opendir(lp);
	if (dir != NULL) {
		struct dirent* de = NULL;
		while ((de = readdir(dir)) != NULL) {
			if (!endsWith_nocase(de->d_name, ".mca")) continue;
			snprintf(lp, PATH_MAX, "%s/region/%s", path, de->d_name);
			char* xs = strstr(lp, "/r.") + 3;
			char* zs = strchr(xs, '.') + 1;
			if (zs == NULL) continue;
			struct region* reg = newRegion(lp, strtol(xs, NULL, 10), strtol(zs, NULL, 10));
			add_collection(world->regions, reg);
		}
		closedir(dir);
	}
	return 0;
}

int saveWorld(struct world* world, char* path) {

	return 0;
}

void freeWorld(struct world* world) { // assumes all chunks are unloaded
	pthread_rwlock_wrlock(&world->regions->data_mutex);
	for (size_t i = 0; i < world->regions->size; i++) {
		if (world->regions->data[i] != NULL) {
			freeRegion(world->regions->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->regions->data_mutex);
	del_collection(world->regions);
	pthread_rwlock_wrlock(&world->entities->data_mutex);
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL) {
			freeEntity(world->entities->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->entities->data_mutex);
	del_collection(world->entities);
	pthread_rwlock_wrlock(&world->players->data_mutex);
	for (size_t i = 0; i < world->players->size; i++) {
		if (world->players->data[i] != NULL) {
			freePlayer(world->players->data[i]);
		}
	}
	pthread_rwlock_unlock(&world->players->data_mutex);
	del_collection(world->players);
	if (world->level != NULL) {
		freeNBT(world->level);
		xfree(world->level);
	}
	if (world->lpa != NULL) xfree(world->lpa);
	xfree(world);
}

void spawnEntity(struct world* world, struct entity* entity) {
	entity->world = world;
	add_collection(world->entities, entity);
}

void spawnPlayer(struct world* world, struct player* player) {
	player->world = world;
	add_collection(world->players, player);
	spawnEntity(world, player->entity);
}

void despawnPlayer(struct world* world, struct player* player) {
	rem_collection(world->entities, player->entity);
	rem_collection(world->players, player);
}

void despawnEntity(struct world* world, struct entity* entity) {
	if (entity->type == ENT_PLAYER) despawnPlayer(world, entity->data.player.player);
	else rem_collection(world->entities, entity);
}

struct entity* getEntity(struct world* world, int32_t id) {
	pthread_rwlock_rdlock(&world->entities->data_mutex);
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL && ((struct entity*) world->entities->data[i])->id == id) {
			return world->entities->data[i];
		}
	}
	pthread_rwlock_unlock(&world->entities->data_mutex);
	return NULL;
}
