/*
 * world.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

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
#include "packet.h"
#include "game.h"
#include "block.h"
#include <math.h>
#include "queue.h"
#include "tileentity.h"
#include "xstring.h"
#include "entity.h"
#include "world.h"
#include "globals.h"

int __boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2, int inv) {
	return (((bb1->minX >= bb2->minX && bb1->minX <= bb2->maxX) || (bb1->maxX >= bb2->minX && bb1->maxX <= bb2->maxX)) && ((bb1->minY >= bb2->minY && bb1->minY <= bb2->maxY) || (bb1->maxY >= bb2->minY && bb1->maxY <= bb2->maxY)) && ((bb1->minZ >= bb2->minZ && bb1->minZ <= bb2->maxZ) || (bb1->maxZ >= bb2->minZ && bb1->maxZ <= bb2->maxZ))) || (inv ? 0 : __boundingbox_intersects(bb2, bb1, 1)); // the second intersects handles if bb2 is contained within bb1
}

int boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2) {
	return __boundingbox_intersects(bb1, bb2, 0);
}

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
	if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY) goto rerx;
	if (tmp->data.nbt_bytearray.len == 256) memcpy(rch->biomes, tmp->data.nbt_bytearray.data, 256);
	tmp = getNBTChild(level, "HeightMap");
	if (tmp == NULL || tmp->id != NBT_TAG_INTARRAY) goto rerx;
	if (tmp->data.nbt_intarray.count == 256) for (int i = 0; i < 256; i++)
		rch->heightMap[i >> 4][i & 0x0F] = (uint16_t) tmp->data.nbt_intarray.ints[i];
	struct nbt_tag* tes = getNBTChild(level, "TileEntities");
	if (tes == NULL || tes->id != NBT_TAG_LIST) goto rerx;
	for (size_t i = 0; i < tes->children_count; i++) {
		struct nbt_tag* ten = tes->children[i];
		if (ten == NULL || ten->id != NBT_TAG_COMPOUND) continue;
		struct tile_entity* te = parseTileEntity(ten);
		add_collection(rch->tileEntities, te);
	}
	//TODO: tileticks
	struct nbt_tag* sections = getNBTChild(level, "Sections");
	for (size_t i = 0; i < sections->children_count; i++) {
		struct nbt_tag* section = sections->children[i];
		tmp = getNBTChild(section, "Y");
		if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
		uint8_t y = tmp->data.nbt_byte;
		tmp = getNBTChild(section, "Blocks");
		if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 4096) continue;
		int hna = 0;
		block* rbl = xmalloc(sizeof(block) * 4096);
		for (int i = 0; i < 4096; i++) {
			rbl[i] = ((block) tmp->data.nbt_bytearray.data[i]) << 4; // [i >> 8][(i & 0xf0) >> 4][i & 0x0f]
			if (((block) tmp->data.nbt_bytearray.data[i]) != 0) hna = 1;
		}
		//if (hna) rch->empty[y] = 0;
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
				rbl[i] |= sx;
				if (rbl[i] != 0) hna = 1;
			}
		}
		if (hna) {
			tmp = getNBTChild(section, "Data");
			if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
			for (int i = 0; i < 4096; i++) {
				block sx = tmp->data.nbt_bytearray.data[i / 2];
				if (i % 2 == 1) {
					sx &= 0xf0;
					sx >>= 4;
				} else sx &= 0x0f;
				rbl[i] |= sx;
			}
			struct chunk_section* cs = xmalloc(sizeof(struct chunk_section));
			cs->palette = xmalloc(256 * sizeof(block));
			cs->palette_count = 0;
			block ipalette[getBlockSize()];
			block l = 0;
			for (int i = 0; i < 4096; i++) {
				if (rbl[i] != l) {
					for (int x = cs->palette_count - 1; x >= 0; x--) {
						if (cs->palette[x] == rbl[i]) goto cx;
					}
					ipalette[rbl[i]] = cs->palette_count;
					cs->palette[cs->palette_count++] = rbl[i]; // TODO: if only a few blocks of a certain type and on a palette boundary, use a MBC to reduce size?
					if (cs->palette_count >= 256) {
						xfree(cs->palette);
						cs->palette = NULL;
						cs->bpb = 13;
						cs->palette_count = 0;
						break;
					}
					l = rbl[i];
				}
				cx: ;
			}
			if (cs->palette != NULL) {
				cs->bpb = (uint8_t) ceil(log2(cs->palette_count));
				if (cs->bpb < 4) cs->bpb = 4;
				cs->palette = xrealloc(cs->palette, cs->palette_count * sizeof(block));
			}
			int32_t bi = 0;
			cs->blocks = xmalloc(512 * cs->bpb + 4);
			cs->block_size = 512 * cs->bpb;
			cs->mvs = 0;
			for (int i = 0; i < cs->bpb; i++)
				cs->mvs |= 1 << i;
			for (int i = 0; i < 4096; i++) { // [i >> 8][(i & 0xf0) >> 4][i & 0x0f]
				int32_t b = (int32_t)((cs->bpb == 13 ? rbl[i] : ipalette[rbl[i]]) & cs->mvs);
				int32_t cv = *((int32_t*) (&cs->blocks[bi / 8]));
				int32_t sbi = bi % 8;
				cv = (cv & ~(cs->mvs << sbi)) | (b << sbi);
				*((int32_t*) &cs->blocks[bi / 8]) = cv;
				bi += cs->bpb;
			}
			tmp = getNBTChild(section, "BlockLight");
			if (tmp != NULL) {
				if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
				memcpy(cs->blockLight, tmp->data.nbt_bytearray.data, 2048);
			}
			tmp = getNBTChild(section, "SkyLight");
			if (tmp != NULL) {
				if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
				cs->skyLight = xmalloc(2048);
				memcpy(cs->skyLight, tmp->data.nbt_bytearray.data, 2048);
			}
			rch->sections[y] = cs;
		}
		xfree(rbl);
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
	memset(chunk->sections, 0, sizeof(struct chunk_section*) * 16);
	chunk->sections[0] = xcalloc(sizeof(struct chunk_section));
	chunk->sections[0]->bpb = 13;
	chunk->sections[0]->block_size = 512 * 13;
	chunk->sections[0]->blocks = xcalloc(512 * 13);
	chunk->sections[0]->palette = NULL;
	chunk->sections[0]->palette_count = 0;
	chunk->sections[0]->mvs = 0x1FFF;
	if (world->dimension == OVERWORLD) {
		chunk->sections[0]->skyLight = xmalloc(2048);
		memset(chunk->sections[0]->skyLight, 0xFF, 2048);
	}
	memset(chunk->sections[0]->blockLight, 0xFF, 2048);
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			setBlockChunk(chunk, 1 << 4, x, 0, z, world->dimension == 0);
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
				add_collection(world->loadedChunks, chk);
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

int isChunkLoaded(struct world* world, int16_t x, int16_t z) {
	int16_t rx = x >> 5;
	int16_t rz = z >> 5;
	pthread_rwlock_rdlock(&world->regions->data_mutex);
	for (size_t i = 0; i < world->regions->size; i++) {
		if (world->regions->data[i] != NULL && ((struct region*) world->regions->data[i])->x == rx && ((struct region*) world->regions->data[i])->z == rz) {
			struct region* r = ((struct region*) world->regions->data[i]);
			int l = r->loaded[z & 0x1F][x & 0x1F];
			pthread_rwlock_unlock(&world->regions->data_mutex);
			return l;
		}
	}
	pthread_rwlock_unlock(&world->regions->data_mutex);
	return 0;
}

void unloadChunk(struct world* world, struct chunk* chunk) {
//TODO: save chunk
	pthread_rwlock_wrlock(&world->chl);
	int16_t rx = chunk->x >> 5;
	int16_t rz = chunk->z >> 5;
	rem_collection(world->loadedChunks, chunk);
	pthread_rwlock_rdlock(&world->regions->data_mutex);
	for (size_t i = 0; i < world->regions->size; i++) {
		if (world->regions->data[i] != NULL && ((struct region*) world->regions->data[i])->x == rx && ((struct region*) world->regions->data[i])->z == rz) {
			struct region* r = ((struct region*) world->regions->data[i]);
			r->loaded[chunk->z & 0x1F][chunk->x & 0x1F] = 0;
			r->chunks[chunk->z & 0x1F][chunk->x & 0x1F] = 0;
			freeChunk(chunk);
			pthread_rwlock_unlock(&world->regions->data_mutex);
			pthread_rwlock_unlock(&world->chl);
			return;
		}
	}
	pthread_rwlock_unlock(&world->regions->data_mutex);
	pthread_rwlock_unlock(&world->chl);
}

int getBiome(struct world* world, int32_t x, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 0;
	return chunk->biomes[z & 0x0f][x & 0x0f];
}

block getBlockChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
	struct chunk_section* cs = chunk->sections[y >> 4];
	if (cs == NULL) return 0;
	uint32_t i = ((y & 0x0f) << 8) | (z << 4) | x;
	uint32_t bi = cs->bpb * i;
	int32_t rcv = *((int32_t*) (&cs->blocks[bi / 8]));
	int32_t rsbi = bi % 8;
	block b = (rcv >> rsbi) & cs->mvs;
	if (cs->palette != NULL && b < cs->palette_count) b = cs->palette[b];
	return b;
}

block getBlockWorld(struct world* world, int32_t x, uint8_t y, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 0;
	return getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
}

struct tile_entity* getTileEntityChunk(struct chunk* chunk, int32_t x, uint8_t y, int32_t z) { // TODO: optimize
	if (y > 255 || y < 0) return NULL;
	for (size_t i = 0; i < chunk->tileEntities->size; i++) {
		struct tile_entity* te = (struct tile_entity*) chunk->tileEntities->data[i];
		if (te == NULL) continue;
		if (te->x == x && te->y == y && te->z == z) return te;
	}
	return NULL;
}

void setTileEntityChunk(struct chunk* chunk, struct tile_entity* te) { // TODO: optimize
	if (te == NULL) return;
	int32_t x = te->x;
	uint8_t y = te->y;
	int32_t z = te->z;
	if (y > 255 || y < 0) return;
	for (size_t i = 0; i < chunk->tileEntities->size; i++) {
		struct tile_entity* te2 = (struct tile_entity*) chunk->tileEntities->data[i];
		if (te2 == NULL) continue;
		if (te2->x == x && te2->y == y && te2->z == z) {
			if (te2->tick) rem_collection(chunk->tileEntitiesTickable, te2);
			freeTileEntity(te2);
			chunk->tileEntities->data[i] = te;
			if (te == NULL) {
				chunk->tileEntities->count--;
				if (i == chunk->tileEntities->size - 1) chunk->tileEntities->size--;
			} else if (te->tick) add_collection(chunk->tileEntitiesTickable, te);
			return;
		}
	}
	add_collection(chunk->tileEntities, te);
	if (te->tick) add_collection(chunk->tileEntitiesTickable, te);
}

void enableTileEntityTickable(struct world* world, struct tile_entity* te) {
	if (te == NULL || te->y > 255 || te->y < 0) return;
	struct chunk* chunk = getChunk(world, te->x >> 4, te->z >> 4);
	if (chunk == NULL) return;
	add_collection(chunk->tileEntitiesTickable, te);
}

void disableTileEntityTickable(struct world* world, struct tile_entity* te) {
	if (te == NULL || te->y > 255 || te->y < 0) return;
	struct chunk* chunk = getChunk(world, te->x >> 4, te->z >> 4);
	if (chunk == NULL) return;
	rem_collection(chunk->tileEntitiesTickable, te);
}

struct tile_entity* getTileEntityWorld(struct world* world, int32_t x, uint8_t y, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return NULL;
	return getTileEntityChunk(chunk, x, y, z);
}

void setTileEntityWorld(struct world* world, int32_t x, uint8_t y, int32_t z, struct tile_entity* te) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return;
	setTileEntityChunk(chunk, te);
}

struct chunk* newChunk(int16_t x, int16_t z) {
	struct chunk* chunk = xmalloc(sizeof(struct chunk));
	memset(chunk, 0, sizeof(struct chunk));
	chunk->x = x;
	chunk->z = z;
	memset(chunk->sections, 0, sizeof(struct chunk_section*) * 16);
	chunk->playersLoaded = 0;
	chunk->tileEntities = new_collection(0, 0);
	chunk->tileEntitiesTickable = new_collection(0, 0);
	return chunk;
}

void freeChunk(struct chunk* chunk) {
	for (int i = 0; i < 16; i++) {
		struct chunk_section* cs = chunk->sections[i];
		if (cs == NULL) continue;
		if (cs->blocks != NULL) xfree(cs->blocks);
		if (cs->palette != NULL) xfree(cs->palette);
		if (cs->skyLight != NULL) xfree(cs->skyLight);
	}
	for (size_t i = 0; i < chunk->tileEntities->size; i++) {
		if (chunk->tileEntities->data[i] != NULL) {
			freeTileEntity(chunk->tileEntities->data[i]);
		}
	}
	del_collection(chunk->tileEntities);
	del_collection(chunk->tileEntitiesTickable);
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

void setBlockChunk(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z, int skylight) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
	struct chunk_section* cs = chunk->sections[y >> 4];
	if (cs == NULL && blk != 0) {
		chunk->sections[y >> 4] = xcalloc(sizeof(struct chunk_section));
		cs = chunk->sections[y >> 4];
		cs->bpb = 13;
		cs->block_size = 512 * 13;
		cs->blocks = xcalloc(512 * 13 + 4);
		cs->mvs = 0x1FFF;
		memset(cs->blockLight, 0xFF, 2048);
		if (skylight) {
			cs->skyLight = xmalloc(2048);
			memset(cs->skyLight, 0xFF, 2048);
		}
	} else if (cs == NULL) return;
	block ts = blk;
	if (cs->bpb < 9) {
		for (int i = 0; i < cs->palette_count; i++) {
			if (cs->palette[i] == blk) {
				ts = i;
				goto pp;
			}
		}
		uint32_t room = pow(2, ceil(log2(cs->palette_count))) - cs->palette_count;
		if (room < 1) {
			uint8_t nbpb = cs->bpb + 1;
			if (nbpb >= 9) nbpb = 13;
			uint8_t* ndata = xcalloc(nbpb * 512 + 4);
			uint32_t bir = 0;
			uint32_t biw = 0;
			int32_t nmvs = cs->mvs | (1 << (nbpb - 1));
			if (nbpb == 13) nmvs = 0x1FFF;
			for (int i = 0; i < 4096; i++) {
				int32_t rcv = *((int32_t*) (&cs->blocks[bir / 8]));
				int32_t rsbi = bir % 8;
				int32_t b = (rcv >> rsbi) & cs->mvs;
				if (nbpb == 13) b = cs->palette[b];
				int32_t wcv = *((int32_t*) (&ndata[biw / 8]));
				int32_t wsbi = biw % 8;
				wcv = (wcv & ~(nmvs << wsbi)) | (b << wsbi);
				*((int32_t*) &ndata[biw / 8]) = wcv;
				bir += cs->bpb;
				biw += nbpb;
			}
			uint8_t* odata = cs->blocks;
			cs->blocks = ndata;
			cs->block_size = nbpb * 512;
			xfree(odata);
			cs->mvs = nmvs;
			cs->bpb = nbpb;
		}
		ts = cs->palette_count;
		cs->palette = xrealloc(cs->palette, sizeof(block) * (cs->palette_count + 1));
		cs->palette[cs->palette_count++] = blk;
		pp: ;
	}
	uint32_t i = ((y & 0x0f) << 8) | (z << 4) | x;
	uint32_t bi = cs->bpb * i;
	int32_t b = ((int32_t) ts) & cs->mvs;
	int32_t cv = *((int32_t*) (&cs->blocks[bi / 8]));
	int32_t sbi = bi % 8;
	cv = (cv & ~(cs->mvs << sbi)) | (b << sbi);
	*((int32_t*) &cs->blocks[bi / 8]) = cv;
}

void setBlockWorld(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) {
		return;
	}
	block ob = getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
	setBlockChunk(chunk, blk, x & 0x0f, y, z & 0x0f, world->dimension == 0);
	BEGIN_BROADCAST_DISTXYZ((double) x + .5, (double) y + .5, (double) z + .5, world->players, CHUNK_VIEW_DISTANCE * 16.)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_BLOCKCHANGE;
	pkt->data.play_client.blockchange.location.x = x;
	pkt->data.play_client.blockchange.location.y = y;
	pkt->data.play_client.blockchange.location.z = z;
	pkt->data.play_client.blockchange.block_id = blk;
	add_queue(bc_player->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST
	struct block_info* bi = getBlockInfo(ob);
	if (bi != NULL && bi->onBlockDestroyed != NULL) (*bi->onBlockDestroyed)(world, ob, x, y, z);
}

struct world* newWorld() {
	struct world* world = xmalloc(sizeof(struct world));
	memset(world, 0, sizeof(struct world));
	world->regions = new_collection(0, 1);
	world->entities = new_collection(0, 0);
	world->players = new_collection(0, 0);
	world->loadedChunks = new_collection(0, 0);
	pthread_rwlock_init(&world->chl, NULL);
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
	pthread_rwlock_destroy(&world->chl);
	del_collection(world->regions);
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL) {
			freeEntity(world->entities->data[i]);
		}
	}
	del_collection(world->entities);
	for (size_t i = 0; i < world->players->size; i++) {
		if (world->players->data[i] != NULL) {
			freePlayer(world->players->data[i]);
		}
	}
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
	despawnEntity(world, player->entity);
	for (size_t i = 0; i < player->loadedEntities->size; i++) {
		struct entity* pl = (struct entity*) player->loadedEntities->data[i];
		if (pl == NULL || pl == player->entity) continue;
		rem_collection(pl->loadingPlayers, player);
		player->loadedEntities->data[i] = NULL;
		player->loadedEntities->count--;
	}
	for (size_t i = 0; i < player->loadedChunks->size; i++) {
		struct chunk* pl = (struct chunk*) player->loadedChunks->data[i];
		if (pl == NULL || pl == player->entity) continue;
		pl->playersLoaded--;
		player->loadedChunks->data[i] = NULL;
		player->loadedChunks->count--;
	}
	rem_collection(world->players, player);
}

void despawnEntity(struct world* world, struct entity* entity) {
	rem_collection(world->entities, entity);
	BEGIN_BROADCAST(entity->loadingPlayers)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_DESTROYENTITIES;
	pkt->data.play_client.destroyentities.count = 1;
	pkt->data.play_client.destroyentities.entity_ids = xmalloc(sizeof(int32_t));
	pkt->data.play_client.destroyentities.entity_ids[0] = entity->id;
	add_queue(bc_player->outgoingPacket, pkt);
	flush_outgoing (bc_player);
	END_BROADCAST
	for (size_t i = 0; i < entity->loadingPlayers->size; i++) {
		struct player* pl = (struct player*) entity->loadingPlayers->data[i];
		if (pl == NULL) continue;
		rem_collection(pl->loadedEntities, entity);
		entity->loadingPlayers->data[i] = NULL;
		entity->loadingPlayers->count--;
	}
}

struct entity* getEntity(struct world* world, int32_t id) {
	for (size_t i = 0; i < world->entities->size; i++) {
		if (world->entities->data[i] != NULL && ((struct entity*) world->entities->data[i])->id == id) {
			return world->entities->data[i];
		}
	}
	return NULL;
}

void onBlockDestroyed(struct world* world, int32_t x, int32_t y, int32_t z) {

}

void onBlockPlaced(struct world* world, int32_t x, int32_t y, int32_t z) {

}

void onBlockInteract(struct world* world, int32_t x, int32_t y, int32_t z, struct player* player) {

}

void onBlockCollide(struct world* world, int32_t x, int32_t y, int32_t z, struct entity* entity) {

}

void onBlockUpdate(struct world* world, int32_t x, int32_t y, int32_t z) {

}
