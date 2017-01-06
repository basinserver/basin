/*
 * world.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#include "hashmap.h"
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
#include "profile.h"
#include <errno.h>
#include "server.h"

int __boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2, int inv) {
	return (((bb1->minX >= bb2->minX && bb1->minX <= bb2->maxX) || (bb1->maxX >= bb2->minX && bb1->maxX <= bb2->maxX)) && ((bb1->minY >= bb2->minY && bb1->minY <= bb2->maxY) || (bb1->maxY >= bb2->minY && bb1->maxY <= bb2->maxY)) && ((bb1->minZ >= bb2->minZ && bb1->minZ <= bb2->maxZ) || (bb1->maxZ >= bb2->minZ && bb1->maxZ <= bb2->maxZ))) || (inv ? 0 : __boundingbox_intersects(bb2, bb1, 1)); // the second intersects handles if bb2 is contained within bb1
}

int boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2) {
	return __boundingbox_intersects(bb1, bb2, 0);
}

uint64_t getChunkKey(struct chunk* ch) {
	return (uint64_t)(((int64_t) ch->x) << 32) | (((int64_t) ch->z) & 0xFFFFFFFF);
}

uint64_t getChunkKey2(int32_t cx, int32_t cz) {
	return (uint64_t)(((int64_t) cx) << 32) | (((int64_t) cz) & 0xFFFFFFFF);
}

struct chunk* getChunk_guess(struct world* world, struct chunk* ch, int32_t x, int32_t z) {
	if (ch == NULL) return getChunk(world, x, z);
	int32_t rx = x >> 4;
	int32_t rz = z >> 4;
	if (ch->x == rx && ch->z == rz) return ch;
	if (abs(x - ch->x) > 3 || abs(z - ch->z) > 3) return getChunk(world, x, z);
	struct chunk* cch = ch;
	while (cch != NULL) {
		if (cch->x > rx) cch = cch->xn;
		else if (cch->x < rx) cch = cch->xp;
		if (cch != NULL) {
			if (cch->z > rz) cch = cch->zn;
			else if (cch->z < rz) cch = cch->zp;
		}
		if (cch != NULL && cch->x == rx && rz) return cch;
	}
	return getChunk(world, x, z);
}

struct chunk* loadRegionChunk(struct region* region, int8_t lchx, int8_t lchz, size_t chri) {
	if (region->fd[chri] < 0) {
		region->fd[chri] = open(region->file, O_RDWR);
		if (region->fd[chri] < 0) {
			printf("Error opening region: %s\n", strerror(errno));
			return NULL;
		}
		region->mapping[chri] = mmap(NULL, 67108864, PROT_READ | PROT_WRITE, MAP_SHARED, region->fd[chri], 0); // 64 MB is the theoretical limit of an uncompressed region file
		if (region->mapping[chri] == NULL || region->mapping[chri] == (void*) -1) {
			printf("Error mapping region: %s\n", strerror(errno));
		}
	}
	uint32_t* hf = region->mapping[chri] + 4 * ((lchx & 31) + (lchz & 31) * 32);
	uint32_t rhf = *hf;
	swapEndian(&rhf, 4);
	uint32_t off = ((rhf & 0xFFFFFF00) >> 8) * 4096;
	uint32_t size = ((rhf & 0x000000FF)) * 4096;
	if (off == 0 || size == 0) return NULL;
	void* chk = region->mapping[chri] + (off);
	uint32_t rsize = ((uint32_t*) chk)[0];
	swapEndian(&rsize, 4);
	uint8_t comptype = ((uint8_t*) chk)[4];
	chk += 5;
	void* rtbuf = xmalloc(65536);
	size_t rtc = 65536;
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
		if (rtc - strm.total_out < 32768) {
			rtc += 65536;
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
	for (int z = 0; z < 16; z++) {
		for (int x = 0; x < 16; x++) {
			if (rch->heightMap[z][x] <= 0) {
				for (int y = 255; y >= 0; y--) {
					block b = getBlockChunk(rch, x, y, z);
					struct block_info* bi = getBlockInfo(b);
					if (bi == NULL || bi->lightOpacity <= 0) continue;
					rch->heightMap[z][x] = y + 1;
					break;
				}
			}
		}
	}
//TODO: entities and tileticks.
	freeNBT(nbt);
	return rch;
	rerx: ;
	freeNBT(nbt);
	return NULL;
}

void generateChunk(struct world* world, struct chunk* chunk) {
	memset(chunk->sections, 0, sizeof(struct chunk_section*) * 16);
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			for (int y = 0; y < 64; y++)
				setBlockChunk(chunk, y < 5 ? BLK_BEDROCK : (y == 63 ? BLK_GRASS : (y >= 60 ? BLK_DIRT : BLK_STONE)), x, y, z, world->dimension == OVERWORLD);
		}
	}
}

struct chunk* getChunkWithLoad(struct world* world, int32_t x, int32_t z, size_t chri) {
	struct chunk* ch = get_hashmap(world->chunks, getChunkKey2(x, z));
	if (ch != NULL) return ch;
	int16_t rx = x >> 5;
	int16_t rz = z >> 5;
	uint64_t ri = (((uint64_t)(rx) & 0xFFFF) << 16) | (((uint64_t) rz) & 0xFFFF);
	struct region* ar = get_hashmap(world->regions, ri);
	if (ar == NULL) {
		char lp[PATH_MAX];
		snprintf(lp, PATH_MAX, "%s/region/r.%i.%i.mca", world->lpa, rx, rz);
		ar = newRegion(lp, rx, rz, world->chl_count);
		put_hashmap(world->regions, ri, ar);
		//TODO: populate region
	}
	struct chunk* chk = NULL;
	chk = loadRegionChunk(ar, x & 0x1F, z & 0x1F, chri);
	if (chk != NULL) {
		chk->xp = getChunk(world, x + 1, z);
		if (chk->xp != NULL) chk->xp->xn = chk;
		chk->xn = getChunk(world, x - 1, z);
		if (chk->xn != NULL) chk->xn->xp = chk;
		chk->zp = getChunk(world, x, z + 1);
		if (chk->zp != NULL) chk->zp->zn = chk;
		chk->zn = getChunk(world, x, z - 1);
		if (chk->zn != NULL) chk->zn->zp = chk;
		put_hashmap(world->chunks, getChunkKey(chk), chk);
	} else goto gen;
	return chk;
	gen: ;
	struct chunk* gc = newChunk(x, z);
	generateChunk(world, gc);
	if (gc != NULL) put_hashmap(world->chunks, getChunkKey(gc), gc);
	if (!ar) {
		char lp[PATH_MAX];
		snprintf(lp, PATH_MAX, "%s/region/r.%i.%i.mca", world->lpa, rx, rz);
		ar = newRegion(lp, rx, rz, world->chl_count);
		put_hashmap(world->regions, ri, ar);
	}
	return gc;
}

void chunkloadthr(size_t b) {
	size_t rem = 1;
	while (1) {
		if (--rem <= 0) {
			nm: ;
			pthread_mutex_lock (&chunk_wake_mut);
			pthread_cond_wait(&chunk_wake, &chunk_wake_mut);
			pthread_mutex_unlock(&chunk_wake_mut);
			rem = chunk_input->size;
			if (rem == 0) goto nm;
		}
		pthread_mutex_lock(&chunk_input->data_mutex);
		struct chunk_req* chr = pop_nowait_queue(chunk_input);
		if (chr == NULL) {
			pthread_mutex_unlock(&chunk_input->data_mutex);
			continue;
		}
		if (chr->pl->defunct) {
			xfree(chr);
			pthread_mutex_unlock(&chunk_input->data_mutex);
			continue;
		}
		if (chr->pl->chunksSent >= 3 || (chr->pl->conn != NULL && chr->pl->conn->writeBuffer_size > 1024 * 1024 * 128)) {
			add_queue(chunk_input, chr);
			pthread_mutex_unlock(&chunk_input->data_mutex);
			continue;
		}
		pthread_mutex_unlock(&chunk_input->data_mutex);
		if (chr->load) chr->pl->chunksSent++;
		if (chr->load) {
			if (contains_hashmap(chr->pl->loadedChunks, getChunkKey2(chr->cx, chr->cz))) {
				xfree(chr);
				continue;
			}
			beginProfilerSection("chunkLoading_getChunk");
			struct chunk* ch = getChunkWithLoad(chr->pl->world, chr->cx, chr->cz, b);
			if (chr->pl->loadedChunks == NULL) {
				xfree(chr);
				continue;
			}
			endProfilerSection("chunkLoading_getChunk");
			if (ch != NULL) {
				ch->playersLoaded++;
				//beginProfilerSection("chunkLoading_sendChunk_add");
				put_hashmap(chr->pl->loadedChunks, getChunkKey(ch), ch);
				//endProfilerSection("chunkLoading_sendChunk_add");
				//beginProfilerSection("chunkLoading_sendChunk_malloc");
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_CHUNKDATA;
				pkt->data.play_client.chunkdata.data = ch;
				pkt->data.play_client.chunkdata.cx = ch->x;
				pkt->data.play_client.chunkdata.cz = ch->z;
				pkt->data.play_client.chunkdata.ground_up_continuous = 1;
				pkt->data.play_client.chunkdata.number_of_block_entities = ch->tileEntities->count;
				//endProfilerSection("chunkLoading_sendChunk_malloc");
				//beginProfilerSection("chunkLoading_sendChunk_tileEntities");
				pkt->data.play_client.chunkdata.block_entities = ch->tileEntities->count == 0 ? NULL : xmalloc(sizeof(struct nbt_tag*) * ch->tileEntities->count);
				size_t ri = 0;
				for (size_t i = 0; i < ch->tileEntities->size; i++) {
					if (ch->tileEntities->data[i] == NULL) continue;
					struct tile_entity* te = ch->tileEntities->data[i];
					pkt->data.play_client.chunkdata.block_entities[ri++] = serializeTileEntity(te, 1);
				}
				//endProfilerSection("chunkLoading_sendChunk_tileEntities");
				//beginProfilerSection("chunkLoading_sendChunk_dispatch");
				add_queue(chr->pl->outgoingPacket, pkt);
				flush_outgoing(chr->pl);
				//endProfilerSection("chunkLoading_sendChunk_dispatch");
			}
		} else {
			//beginProfilerSection("unchunkLoading");
			struct packet* pkt = xmalloc(sizeof(struct packet));
			pkt->id = PKT_PLAY_CLIENT_UNLOADCHUNK;
			pkt->data.play_client.unloadchunk.chunk_x = chr->cx;
			pkt->data.play_client.unloadchunk.chunk_z = chr->cz;
			pkt->data.play_client.unloadchunk.ch = NULL;
			struct chunk* ch = getChunk(chr->pl->world, chr->cx, chr->cz);
			put_hashmap(chr->pl->loadedChunks, getChunkKey2(chr->cx, chr->cz), NULL);
			if (ch != NULL && !ch->defunct) {
				if (--ch->playersLoaded <= 0) {
					unloadChunk(chr->pl->world, ch);
				}
			}
			add_queue(chr->pl->outgoingPacket, pkt);
			flush_outgoing(chr->pl);
			//endProfilerSection("unchunkLoading");
		}
		xfree(chr);
	}
}

struct chunk* getChunk(struct world* world, int32_t x, int32_t z) {
	struct chunk* ch = get_hashmap(world->chunks, getChunkKey2(x, z));
	return ch;
}

int isChunkLoaded(struct world* world, int32_t x, int32_t z) {
	return contains_hashmap(world->chunks, getChunkKey2(x, z));
}

void unloadChunk(struct world* world, struct chunk* chunk) {
//TODO: save chunk
//pthread_rwlock_wrlock(&world->chl);
	if (chunk->xp != NULL) chunk->xp->xn = NULL;
	if (chunk->xn != NULL) chunk->xn->xp = NULL;
	if (chunk->zp != NULL) chunk->zp->zn = NULL;
	if (chunk->zn != NULL) chunk->zn->zp = NULL;
	put_hashmap(world->chunks, getChunkKey(chunk), NULL);
	add_collection(defunctChunks, chunk);
//pthread_rwlock_unlock(&world->chl);
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

uint8_t getLightChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z, uint8_t subt) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
	struct chunk_section* cs = chunk->sections[y >> 4];
	if (cs == NULL) return 0;
	uint32_t i = ((y & 0x0f) << 8) | (z << 4) | x;
	uint32_t bi = 4 * i;
	int8_t skl = 0;
	if (cs->skyLight != NULL) {
		uint8_t tskl = cs->skyLight[bi / 8];
		if (i % 2 == 1) {
			tskl &= 0xf0;
			tskl >>= 4;
		} else tskl &= 0x0f;
		skl = tskl;
	}
	skl -= subt;
	uint8_t bl = cs->blockLight[bi / 8];
	if (i % 2 == 1) {
		bl &= 0xf0;
		bl >>= 4;
	} else bl &= 0x0f;
	if (bl > skl) skl = bl;
	if (skl < 0) skl = 0;
	return skl;
}

uint8_t getRawLightChunk(struct chunk* chunk, uint8_t x, uint8_t y, uint8_t z, uint8_t blocklight) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return 0;
	struct chunk_section* cs = chunk->sections[y >> 4];
	if (cs == NULL) return 0;
	uint32_t i = ((y & 0x0f) << 8) | (z << 4) | x;
	uint32_t bi = 4 * i;
	uint8_t* target = blocklight ? cs->blockLight : cs->skyLight;
	uint8_t bl = target[bi / 8];
	if (i % 2 == 1) {
		bl &= 0xf0;
		bl >>= 4;
	} else bl &= 0x0f;
	return bl;
}

void setLightChunk(struct chunk* chunk, uint8_t light, uint8_t x, uint8_t y, uint8_t z, uint8_t blocklight, uint8_t skylight) { // skylight is only for making new chunk sections, not related to set!
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
	struct chunk_section* cs = chunk->sections[y >> 4];
	if (cs == NULL) {
		cs = newChunkSection(chunk, y >> 4, skylight);
	}
	uint32_t i = ((y & 0x0f) << 8) | (z << 4) | x;
	uint32_t bi = 4 * i;
	uint8_t* target = blocklight ? cs->blockLight : cs->skyLight;
	uint8_t bl = target[bi / 8];
	if (i % 2 == 1) {
		bl &= 0x0f;
		bl |= (light & 0x0f) << 4;
	} else {
		bl &= 0xf0;
		bl |= light & 0x0f;
	}
	target[bi / 8] = bl;
}

uint8_t getLightWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return 0;
	ch = getChunk_guess(world, ch, x, z);
	if (ch != NULL) return getLightChunk(ch, x & 0x0f, y, z & 0x0f, world->skylightSubtracted);
	else return getLightWorld(world, x, y, z, 0);
}

uint8_t getRawLightWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
	if (y < 0 || y > 255) return 0;
	ch = getChunk_guess(world, ch, x, z);
	if (ch != NULL) return getRawLightChunk(ch, x & 0x0f, y, z & 0x0f, blocklight);
	else return getRawLightWorld(world, x, y, z, blocklight);
}

uint16_t getHeightMapWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t z) {
	if (world->dimension != OVERWORLD) return 0;
	ch = getChunk_guess(world, ch, x >> 4, z >> 4);
	if (ch == NULL) return 0;
	return ch->heightMap[z & 0x0f][x & 0x0f];
}

void setLightWorld_guess(struct world* world, struct chunk* ch, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
	if (y < 0 || y > 255) return;
	ch = getChunk_guess(world, ch, x >> 4, z >> 4);
	if (ch != NULL) return setLightChunk(ch, light & 0x0f, x & 0x0f, y, z & 0x0f, blocklight, world->dimension == 0);
	else return setLightWorld(world, light & 0x0f, x, y, z, blocklight);
}

void setLightWorld(struct world* world, uint8_t light, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
	if (y < 0 || y > 255) return;
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return;
	setLightChunk(chunk, light & 0x0f, x & 0x0f, y > 255 ? 255 : y, z & 0x0f, blocklight, world->dimension == 0);
}

uint8_t getLightWorld(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t checkNeighbors) {
	if (y < 0 || y > 255) return 0;
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 15;
	if (checkNeighbors) {
		uint8_t yp = getLightChunk(chunk, x & 0x0f, y, z & 0x0f, world->skylightSubtracted);
		uint8_t xp = getLightWorld_guess(world, chunk, x + 1, y, z);
		uint8_t xn = getLightWorld_guess(world, chunk, x - 1, y, z);
		uint8_t zp = getLightWorld_guess(world, chunk, x, y, z + 1);
		uint8_t zn = getLightWorld_guess(world, chunk, x, y, z - 1);
		if (xp > yp) yp = xp;
		if (xn > yp) yp = xn;
		if (zp > yp) yp = zp;
		if (zn > yp) yp = zn;
		return yp;
	} else if (y < 0) return 0;
	else {
		return getLightChunk(chunk, x, y > 255 ? 255 : y, z, world->skylightSubtracted);
	}
}

uint8_t getRawLightWorld(struct world* world, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
	if (y < 0 || y > 255) return 0;
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 15;
	return getRawLightChunk(chunk, x & 0x0f, y > 255 ? 255 : y, z & 0x0f, blocklight);
}

block getBlockWorld(struct world* world, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return 0;
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 0;
	return getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
}

block getBlockWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return 0;
	ch = getChunk_guess(world, ch, x >> 4, z >> 4);
	if (ch != NULL) return getBlockChunk(ch, x & 0x0f, y, z & 0x0f);
	else return getBlockWorld(world, x, y, z);
}

struct tile_entity* getTileEntityChunk(struct chunk* chunk, int32_t x, int32_t y, int32_t z) { // TODO: optimize
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

struct tile_entity* getTileEntityWorld(struct world* world, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return NULL;
	struct chunk* chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return NULL;
	return getTileEntityChunk(chunk, x, y, z);
}

void setTileEntityWorld(struct world* world, int32_t x, int32_t y, int32_t z, struct tile_entity* te) {
	if (y < 0 || y > 255) return;
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
	chunk->defunct = 0;
	chunk->xp = NULL;
	chunk->xn = NULL;
	chunk->zp = NULL;
	chunk->zn = NULL;
//chunk->entities = new_hashmap(1, 0);
	return chunk;
}

void freeChunk(struct chunk* chunk) {
	for (int i = 0; i < 16; i++) {
		struct chunk_section* cs = chunk->sections[i];
		if (cs == NULL) continue;
		if (cs->blocks != NULL) xfree(cs->blocks);
		if (cs->palette != NULL) xfree(cs->palette);
		if (cs->skyLight != NULL) xfree(cs->skyLight);
		xfree(cs);
	}
	for (size_t i = 0; i < chunk->tileEntities->size; i++) {
		if (chunk->tileEntities->data[i] != NULL) {
			freeTileEntity(chunk->tileEntities->data[i]);
		}
	}
	del_collection(chunk->tileEntities);
	del_collection(chunk->tileEntitiesTickable);
//BEGIN_HASHMAP_ITERATION(chunk->entities)
//freeEntity (value);
//END_HASHMAP_ITERATION(chunk->entities)
//del_hashmap(chunk->entities);
	xfree(chunk);
}

void scheduleBlockTick(struct world* world, int32_t x, int32_t y, int32_t z, int32_t ticksFromNow) {
	if (y < 0 || y > 255) return;
	struct scheduled_tick* st = xmalloc(sizeof(struct scheduled_tick));
	st->x = x;
	st->y = y;
	st->z = z;
	st->ticksLeft = ticksFromNow;
	put_hashmap(world->scheduledTicks, (uint64_t) st, st);
}

struct region* newRegion(char* path, int16_t x, int16_t z, size_t chr_count) {
	struct region* reg = xcalloc(sizeof(struct region));
	reg->x = x;
	reg->z = z;
	reg->file = xstrdup(path, 0);
	reg->fd = xmalloc(chr_count * sizeof(int));
	for (int i = 0; i < chr_count; i++)
		reg->fd[i] = -1;
	reg->mapping = xcalloc(chr_count * sizeof(void*));
	return reg;
}

void freeRegion(struct region* region) {
	xfree(region->fd);
	xfree(region->mapping);
	xfree(region->file);
	xfree(region);
}

struct chunk_section* newChunkSection(struct chunk* chunk, int ymj, int skylight) {
	chunk->sections[ymj] = xcalloc(sizeof(struct chunk_section));
	struct chunk_section* cs = chunk->sections[ymj];
	cs->bpb = 4;
	cs->block_size = 512 * 4;
	cs->blocks = xcalloc(512 * 4 + 4);
	cs->palette_count = 1;
	cs->palette = xmalloc(sizeof(block));
	cs->palette[0] = BLK_AIR;
	cs->mvs = 0xf;
	memset(cs->blockLight, 0, 2048);
	if (skylight) {
		cs->skyLight = xmalloc(2048);
		memset(cs->skyLight, 0xFF, 2048);
	}
	return cs;
}

void setBlockChunk(struct chunk* chunk, block blk, uint8_t x, uint8_t y, uint8_t z, int skylight) {
	if (x > 15 || z > 15 || y > 255 || x < 0 || z < 0 || y < 0) return;
	struct chunk_section* cs = chunk->sections[y >> 4];
	if (cs == NULL && blk != 0) {
		cs = newChunkSection(chunk, y >> 4, skylight);
	} else if (cs == NULL) return;
	if (skylight) {
		struct block_info* bii = getBlockInfo(blk);
		if (bii != NULL && bii->lightOpacity > 1) {
			if (chunk->heightMap[z][x] <= y) chunk->heightMap[z][x] = y + 1;
		}
	}
	block ts = blk;
	if (cs->bpb < 9) {
		for (int i = 0; i < cs->palette_count; i++) {
			if (cs->palette[i] == blk) {
				ts = i;
				goto pp;
			}
		}
		uint32_t room = pow(2, cs->bpb) - cs->palette_count;
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
	if (y < 0 || y > 255) return;
	struct chunk* ch = getChunk(world, x >> 4, z >> 4);
	if (ch == NULL) return;
	setBlockWorld_guess(world, ch, blk, x, y, z);
}

void setBlockWorld_noupdate(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return;
	struct chunk* ch = getChunk(world, x >> 4, z >> 4);
	if (ch == NULL) return;
	setBlockWorld_guess_noupdate(world, ch, blk, x, y, z);
}

void world_doLightProc(struct world* world, struct chunk* chunk, int32_t x, int32_t y, int32_t z, uint8_t light) {
	uint8_t cl = getRawLightWorld_guess(world, chunk, x, y, z, 1);
	if (cl < light) setLightWorld_guess(world, chunk, light & 0x0f, x, y, z, 1);
}

struct world_lightpos {
		int32_t x;
		int32_t y;
		int32_t z;
};

int light_floodfill(struct world* world, struct chunk* chunk, struct world_lightpos* lp, int skylight, int subtract, struct hashmap* subt_upd) {
	if (lp->y < 0 || lp->y > 255) return 0;
	struct block_info* bi = getBlockInfo(getBlockWorld_guess(world, chunk, lp->x, lp->y, lp->z));
	int lo = bi == NULL ? 1 : bi->lightOpacity;
	if (lo < 1) lo = 1;
	int le = bi == NULL ? 0 : bi->lightEmission;
	int16_t maxl = 0;
	uint8_t xpl = getRawLightWorld_guess(world, chunk, lp->x + 1, lp->y, lp->z, !skylight);
	uint8_t xnl = getRawLightWorld_guess(world, chunk, lp->x - 1, lp->y, lp->z, !skylight);
	uint8_t ypl = skylight ? 0 : getRawLightWorld_guess(world, chunk, lp->x, lp->y + 1, lp->z, !skylight);
	uint8_t ynl = getRawLightWorld_guess(world, chunk, lp->x, lp->y - 1, lp->z, !skylight);
	uint8_t zpl = getRawLightWorld_guess(world, chunk, lp->x, lp->y, lp->z + 1, !skylight);
	uint8_t znl = getRawLightWorld_guess(world, chunk, lp->x, lp->y, lp->z - 1, !skylight);
	if (subtract) {
		maxl = 15 - subtract;
	} else {
		maxl = xpl;
		if (xnl > maxl) maxl = xnl;
		if (ypl > maxl) maxl = ypl;
		if (ynl > maxl) maxl = ynl;
		if (zpl > maxl) maxl = zpl;
		if (znl > maxl) maxl = znl;
	}
	if (!subtract) maxl -= lo;
	else maxl += lo;
	//printf("%s %i at %i, %i, %i\n", subtract ? "subtract" : "add", maxl, lp->x, lp->y, lp->z);
	if (maxl < 15 && subtract) subtract = 15 - maxl;
	int sslf = 0;
	if (skylight) {
		int hm = getHeightMapWorld_guess(world, chunk, lp->x, lp->z);
		if (lp->y >= hm) {
			maxl = 15;
			if (subtract) sslf = 1;
		}
		//else maxl = -1;
	}
	if (maxl < le && !skylight) maxl = le;
	if (maxl < 0) maxl = 0;
	if (maxl > 15) maxl = 15;
	uint8_t pl = getRawLightWorld_guess(world, chunk, lp->x, lp->y, lp->z, !skylight);
	if (pl == maxl) return sslf;
	setLightWorld_guess(world, chunk, maxl, lp->x, lp->y, lp->z, !skylight);
	if (subtract ? (maxl < 15) : (maxl > 0)) {

		if (subtract ? xpl > maxl : xpl < maxl) {
			lp->x++;
			if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
				lp->x--;
				void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
				put_hashmap(subt_upd, (uint64_t) n, n);
				//light_floodfill(world, chunk, lp, skylight, 0, 0);
			} else lp->x--;
		}
		if (subtract ? xnl > maxl : xnl < maxl) {
			lp->x--;
			if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
				lp->x++;
				void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
				put_hashmap(subt_upd, (uint64_t) n, n);
				//light_floodfill(world, chunk, lp, skylight, 0, 0);
			} else lp->x++;
		}
		if (!skylight && (subtract ? ypl > maxl : ypl < maxl)) {
			lp->y++;
			if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
				lp->y--;
				//light_floodfill(world, chunk, lp, skylight, 0, 0);
			} else lp->y--;
		}
		if (subtract ? ynl > maxl : ynl < maxl) {
			lp->y--;
			if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
				lp->y++;
				void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
				put_hashmap(subt_upd, (uint64_t) n, n);
				//light_floodfill(world, chunk, lp, skylight, 0, 0);
			} else lp->y++;
		}
		if (subtract ? zpl > maxl : zpl < maxl) {
			lp->z++;
			if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
				lp->z--;
				void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
				put_hashmap(subt_upd, (uint64_t) n, n);
				//light_floodfill(world, chunk, lp, skylight, 0, 0);
			} else lp->z--;
		}
		if (subtract ? znl > maxl : znl < maxl) {
			lp->z--;
			if (light_floodfill(world, chunk, lp, skylight, subtract, subt_upd) && subtract) {
				lp->z++;
				void* n = xcopy(lp, sizeof(struct world_lightpos), 0);
				put_hashmap(subt_upd, (uint64_t) n, n);
				//light_floodfill(world, chunk, lp, skylight, 0, 0);
			} else lp->z++;
		}
	}
	return sslf;
}

void setBlockWorld_guess(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return;
	chunk = getChunk_guess(world, chunk, x >> 4, z >> 4);
	if (chunk == NULL) chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return;
	block ob = getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
	uint16_t ohm = world->dimension == OVERWORLD ? chunk->heightMap[z & 0x0f][x & 0x0f] : 0;
	setBlockChunk(chunk, blk, x & 0x0f, y, z & 0x0f, world->dimension == 0);
	uint16_t nhm = world->dimension == OVERWORLD ? chunk->heightMap[z & 0x0f][x & 0x0f] : 0;
	BEGIN_BROADCAST_DISTXYZ((double) x + .5, (double) y + .5, (double) z + .5, world->players, CHUNK_VIEW_DISTANCE * 16.)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_BLOCKCHANGE;
	pkt->data.play_client.blockchange.location.x = x;
	pkt->data.play_client.blockchange.location.y = y;
	pkt->data.play_client.blockchange.location.z = z;
	pkt->data.play_client.blockchange.block_id = blk;
	add_queue(bc_player->outgoingPacket, pkt);
	END_BROADCAST(world->players)
	struct block_info* bi = getBlockInfo(ob);
	if (bi != NULL && bi->onBlockDestroyed != NULL) (*bi->onBlockDestroyed)(world, ob, x, y, z);
	beginProfilerSection("block_update");
	updateBlockWorld_guess(world, chunk, x, y, z);
	updateBlockWorld_guess(world, chunk, x + 1, y, z);
	updateBlockWorld_guess(world, chunk, x - 1, y, z);
	updateBlockWorld_guess(world, chunk, x, y, z + 1);
	updateBlockWorld_guess(world, chunk, x, y, z - 1);
	updateBlockWorld_guess(world, chunk, x, y + 1, z);
	updateBlockWorld_guess(world, chunk, x, y - 1, z);
	endProfilerSection("block_update");
	beginProfilerSection("skylight_update");
	struct block_info* nbi = getBlockInfo(blk);
	if (nbi == NULL || bi == NULL) return;
	if (world->dimension == OVERWORLD) {
		if (nhm != ohm || (bi->lightOpacity != nbi->lightOpacity && abs(y - ohm) <= 16)) {
			/*setLightWorld_guess(world, chunk, 15, x, nhm, z, 0);
			 struct world_lightpos lp;
			 if (ohm < nhm) {
			 for (int y = ohm; y < nhm; y++) {
			 setLightWorld_guess(world, chunk, 0, x, y, z, 0);
			 }
			 for (int y = ohm; y < nhm; y++) {
			 lp.x = x;
			 lp.y = y;
			 lp.z = z;
			 light_floodfill(world, chunk, &lp, 1);
			 }
			 } else {
			 for (int y = nhm; y < ohm; y++) {
			 setLightWorld_guess(world, chunk, 15, x, y, z, 0);
			 }
			 for (int y = nhm; y < ohm; y++) {
			 lp.x = x;
			 lp.y = y;
			 lp.z = z;
			 light_floodfill(world, chunk, &lp, 1);
			 }
			 }
			 lp.x = x;
			 lp.y = nhm;
			 lp.z = z;
			 light_floodfill(world, chunk, &lp, 1);*/

			beginProfilerSection("skylight_rst");
			/*for (int32_t lx = x - 16; lx <= x + 16; lx++) {
			 for (int32_t ly = ohm - 16; ly <= ohm + 16; ly++) {
			 for (int32_t lz = z - 16; lz <= z + 16; lz++) {
			 setLightWorld_guess(world, chunk, 0, lx, ly, lz, 0);
			 }
			 }
			 }*/
			/*for (int32_t lx = x - 16; lx <= x + 16; lx++) {
			 for (int32_t lz = z - 16; lz <= z + 16; lz++) {
			 int16_t hm = getHeightMapWorld_guess(world, chunk, lx, lz);
			 int16_t lb = ohm - 16;
			 int16_t ub = ohm + 16;
			 if (hm < ub && hm > lb) ub = hm;
			 for (int32_t ly = lb; ly <= ub; ly++) {
			 if (getRawLightWorld_guess(world, chunk, lx, ly, lz, 0) != 0) setLightWorld_guess(world, chunk, 0, lx, ly, lz, 0);
			 }
			 }
			 }*/
			//setLightWorld_guess(world, chunk, 0, x, ohm, z, 0);
			struct hashmap* nup = new_hashmap(1, 0);
			struct world_lightpos lp;
			lp.x = x;
			lp.y = ohm;
			lp.z = z;
			light_floodfill(world, chunk, &lp, 1, 15, nup); // todo remove nup duplicates
			BEGIN_HASHMAP_ITERATION (nup)
			struct world_lightpos* nlp = value;
			light_floodfill(world, chunk, nlp, 1, 0, 0);
			xfree (value);
			END_HASHMAP_ITERATION (nup)
			del_hashmap(nup);
			//light_floodfill(world, chunk, &lp, 1, 0, 0);
			setLightWorld_guess(world, chunk, 15, x, nhm, z, 0);
			lp.x = x;
			lp.y = nhm;
			lp.z = z;
			light_floodfill(world, chunk, &lp, 1, 0, NULL);
			endProfilerSection("skylight_rst");
			//TODO: pillar lighting?
			/*beginProfilerSection("skylight_set");
			 for (int32_t lz = z - 16; lz <= z + 16; lz++) {
			 for (int32_t lx = x - 16; lx <= x + 16; lx++) {
			 uint16_t hm = getHeightMapWorld_guess(world, chunk, lx, lz);
			 if (hm > 255) continue;
			 setLightWorld_guess(world, chunk, 15, lx, hm, lz, 0);
			 }
			 }
			 endProfilerSection("skylight_set");*/
			/*beginProfilerSection("skylight_fill");
			 for (int32_t lz = z - 16; lz <= z + 16; lz++) {
			 for (int32_t lx = x - 16; lx <= x + 16; lx++) {
			 uint16_t hm = getHeightMapWorld_guess(world, chunk, lx, lz);
			 if (hm > 255) continue;
			 lp.x = lx;
			 lp.y = hm;
			 lp.z = lz;
			 light_floodfill(world, chunk, &lp, 1, 0, 0);
			 }
			 }
			 endProfilerSection("skylight_fill");*/
		}
	}
	endProfilerSection("skylight_update");
	beginProfilerSection("blocklight_update");
	if (bi->lightEmission != nbi->lightEmission || bi->lightOpacity != nbi->lightOpacity) {
		if (bi->lightEmission == nbi->lightEmission) {
			beginProfilerSection("blocklight_update_equals");
			uint8_t xpl = getRawLightWorld_guess(world, chunk, x + 1, y, z, 1);
			uint8_t xnl = getRawLightWorld_guess(world, chunk, x - 1, y, z, 1);
			uint8_t ypl = getRawLightWorld_guess(world, chunk, x, y + 1, z, 1);
			uint8_t ynl = getRawLightWorld_guess(world, chunk, x, y - 1, z, 1);
			uint8_t zpl = getRawLightWorld_guess(world, chunk, x, y, z + 1, 1);
			uint8_t znl = getRawLightWorld_guess(world, chunk, x, y, z - 1, 1);
			int16_t maxl = xpl;
			if (xnl > maxl) maxl = xnl;
			if (ypl > maxl) maxl = ypl;
			if (ynl > maxl) maxl = ynl;
			if (zpl > maxl) maxl = zpl;
			if (znl > maxl) maxl = znl;
			maxl -= nbi->lightOpacity;
			if (maxl < nbi->lightEmission) maxl = nbi->lightEmission;
			if (maxl < 0) maxl = 0;
			if (maxl > 15) maxl = 15;
			setLightWorld_guess(world, chunk, maxl, x, y, z, 0);
			endProfilerSection("blocklight_update_equals");
		} else if (nbi->lightEmission > bi->lightEmission) {
			beginProfilerSection("blocklight_update_newlight");
			struct world_lightpos lp;
			lp.x = x;
			lp.y = y;
			lp.z = z;
			light_floodfill(world, chunk, &lp, 0, 0, NULL);
			endProfilerSection("blocklight_update_newlight");
		} else {
			beginProfilerSection("blocklight_update_remlight");
			for (int32_t lx = x - 16; lx <= x + 16; lx++) {
				for (int32_t ly = y - 16; ly <= y + 16; ly++) {
					for (int32_t lz = z - 16; lz <= z + 16; lz++) {
						setLightWorld_guess(world, chunk, 0, lx, ly, lz, 1);
					}
				}
			}
			for (int32_t lx = x - 16; lx <= x + 16; lx++) {
				for (int32_t ly = y - 16; ly <= y + 16; ly++) {
					struct world_lightpos lp;
					lp.x = lx;
					lp.y = ly;
					lp.z = z - 16;
					light_floodfill(world, chunk, &lp, 0, 0, NULL);
					lp.z = z + 16;
					light_floodfill(world, chunk, &lp, 0, 0, NULL);
				}
			}
			for (int32_t lz = z - 16; lz <= z + 16; lz++) {
				for (int32_t ly = y - 16; ly <= y + 16; ly++) {
					struct world_lightpos lp;
					lp.x = x - 16;
					lp.y = ly;
					lp.z = lz;
					light_floodfill(world, chunk, &lp, 0, 0, NULL);
					lp.x = x + 16;
					light_floodfill(world, chunk, &lp, 0, 0, NULL);
				}
			}
			for (int32_t lz = z - 16; lz <= z + 16; lz++) {
				for (int32_t lx = x - 16; lx <= x + 16; lx++) {
					struct world_lightpos lp;
					lp.x = lx;
					lp.y = y - 16;
					lp.z = lz;
					light_floodfill(world, chunk, &lp, 0, 0, NULL);
					lp.y = y + 16;
					light_floodfill(world, chunk, &lp, 0, 0, NULL);
				}
			}
			endProfilerSection("blocklight_update_remlight");
		}
	}
	endProfilerSection("blocklight_update");
}

void setBlockWorld_guess_noupdate(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return;
	chunk = getChunk_guess(world, chunk, x >> 4, z >> 4);
	if (chunk == NULL) return;
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
	END_BROADCAST(world->players)
	struct block_info* bi = getBlockInfo(ob);
	if (bi != NULL && bi->onBlockDestroyed != NULL) (*bi->onBlockDestroyed)(world, ob, x, y, z);
}

struct world* newWorld(size_t chl_count) {
	struct world* world = xmalloc(sizeof(struct world));
	memset(world, 0, sizeof(struct world));
	world->regions = new_hashmap(1, 1);
	world->entities = new_hashmap(1, 1);
	world->players = new_hashmap(1, 1);
	world->chunks = new_hashmap(1, 1);
	pthread_mutex_init(&world->tick_mut, NULL);
	pthread_cond_init(&world->tick_cond, NULL);
	world->chl_count = chl_count;
	world->subworlds = new_hashmap(1, 1);
	world->skylightSubtracted = 0;
	world->scheduledTicks = new_hashmap(1, 1);
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
		if (ldc - (ldi += i) < 512) {
			ldc += 1024;
			ld = xrealloc(ld, ldc);
		}
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
	struct nbt_tag* data = getNBTChild(world->level, "Data");
	world->levelType = getNBTChild(data, "generatorName")->data.nbt_string;
	world->spawnpos.x = getNBTChild(data, "SpawnX")->data.nbt_int;
	world->spawnpos.y = getNBTChild(data, "SpawnY")->data.nbt_int;
	world->spawnpos.z = getNBTChild(data, "SpawnZ")->data.nbt_int;
	world->time = getNBTChild(data, "DayTime")->data.nbt_long;
	world->age = getNBTChild(data, "Time")->data.nbt_long;
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
			struct region* reg = newRegion(lp, strtol(xs, NULL, 10), strtol(zs, NULL, 10), world->chl_count);
			uint64_t ri = (((uint64_t)(reg->x) & 0xFFFF) << 16) | (((uint64_t) reg->z) & 0xFFFF);
			put_hashmap(world->regions, ri, reg);
		}
		closedir(dir);
	}
	return 0;
}

void thr_player_tick(struct subworld* sw) {
	while (1) {
		pthread_mutex_lock(&sw->world->tick_mut);
		pthread_cond_wait(&sw->world->tick_cond, &sw->world->tick_mut);
		pthread_mutex_unlock(&sw->world->tick_mut);
		if (sw->defunct) {
			//assume all players are gone
			put_hashmap(sw->world->subworlds, (uint64_t) sw, NULL);
			del_hashmap(sw->players);
			xfree(sw);
			return;
		}
		beginProfilerSection("player_receive_packet");
		BEGIN_HASHMAP_ITERATION(sw->players)
		struct player* player = (struct player*) value;
		if (player->incomingPacket->size == 0) continue;
		pthread_mutex_lock(&player->incomingPacket->data_mutex);
		struct packet* wp = pop_nowait_queue(player->incomingPacket);
		while (wp != NULL) {
			player_receive_packet(player, wp);
			freePacket(STATE_PLAY, 0, wp);
			xfree(wp);
			wp = pop_nowait_queue(player->incomingPacket);
		}
		pthread_mutex_unlock(&player->incomingPacket->data_mutex);
		END_HASHMAP_ITERATION(sw->players)
		endProfilerSection("player_receive_packet");
		beginProfilerSection("tick_player");
		BEGIN_HASHMAP_ITERATION(sw->players)
		struct player* player = (struct player*) value;
		tick_player(sw->world, player);
		tick_entity(sw->world, player->entity); // might need to be moved into separate loop later
		END_HASHMAP_ITERATION(sw->players)
		endProfilerSection("tick_player");
	}
}

void world_pretick(struct world* world) {
	if (++world->time >= 24000) world->time = 0;
	world->age++;
	float pday = ((float) world->time / 24000.) - .25;
	if (pday < 0.) pday++;
	if (pday > 1.) pday--;
	float cel_angle = 1. - ((cosf(pday * M_PI) + 1.) / 2.);
	cel_angle = pday + (cel_angle - pday) / 3.;
	float psubs = 1. - (cosf(cel_angle * M_PI * 2.) * 2. + .5);
	if (psubs < 0.) psubs = 0.;
	if (psubs > 1.) psubs = 1.;
//TODO: rain, thunder
	world->skylightSubtracted = (uint8_t)(psubs * 11.);
}

void tick_world(struct world* world) {
	int32_t lcg = rand();
	while (1) {
		pthread_mutex_lock (&glob_tick_mut);
		pthread_cond_wait(&glob_tick_cond, &glob_tick_mut);
		pthread_mutex_unlock(&glob_tick_mut);
		world_pretick(world);
		pthread_cond_broadcast(&world->tick_cond); // we use a different condition for subtick threads for the pretick
		beginProfilerSection("tick_entity");
		BEGIN_HASHMAP_ITERATION(world->entities)
		struct entity* entity = (struct entity*) value;
		if (entity->type != ENT_PLAYER) tick_entity(world, entity);
		END_HASHMAP_ITERATION(world->entities)
		endProfilerSection("tick_entity");
		beginProfilerSection("tick_chunks");
		BEGIN_HASHMAP_ITERATION(world->chunks)
		struct chunk* chunk = (struct chunk*) value;
		beginProfilerSection("tick_chunk_tileentity");
		for (size_t x = 0; x < chunk->tileEntitiesTickable->size; x++) {
			struct tile_entity* te = (struct tile_entity*) chunk->tileEntitiesTickable->data[x];
			if (te == NULL) continue;
			(*te->tick)(world, te);
		}
		endProfilerSection("tick_chunk_tileentity");
		beginProfilerSection("tick_chunk_randomticks");
		if (RANDOM_TICK_SPEED > 0) for (int t = 0; t < 16; t++) {
			struct chunk_section* cs = chunk->sections[t];
			if (cs != NULL) {
				for (int z = 0; z < RANDOM_TICK_SPEED; z++) {
					lcg = lcg * 3 + 1013904223;
					int32_t ctotal = lcg >> 2;
					uint8_t x = ctotal & 0x0f;
					uint8_t z = (ctotal >> 8) & 0x0f;
					uint8_t y = (ctotal >> 16) & 0x0f;
					block b = getBlockChunk(chunk, x, y + (t << 4), z);
					struct block_info* bi = getBlockInfo(b);
					if (bi != NULL && bi->randomTick != NULL) (*bi->randomTick)(world, chunk, b, x + (chunk->x << 4), y + (t << 4), z + (chunk->z << 4));
				}
			}
		}
		endProfilerSection("tick_chunk_randomticks");
		END_HASHMAP_ITERATION(world->chunks)
		beginProfilerSection("tick_chunk_scheduledticks");
		BEGIN_HASHMAP_ITERATION(world->scheduledTicks)
		struct scheduled_tick* st = value;
		if (--st->ticksLeft <= 0) {
			block b = getBlockWorld(world, st->x, st->y, st->z);
			struct block_info* bi = getBlockInfo(b);
			int k = 0;
			pthread_rwlock_unlock(&world->scheduledTicks->data_mutex);
			if (bi->scheduledTick != NULL) k = (*bi->scheduledTick)(world, b, st->x, st->y, st->z);
			if (k > 0) {
				st->ticksLeft = k;
			} else {
				put_hashmap(world->scheduledTicks, (uint64_t) st, NULL);
				xfree(st);
			}
			pthread_rwlock_rdlock(&world->scheduledTicks->data_mutex);
		}
		END_HASHMAP_ITERATION(world->scheduledTicks)
		endProfilerSection("tick_chunk_scheduledticks");
		endProfilerSection("tick_chunks");
	}
}

int saveWorld(struct world* world, char* path) {

	return 0;
}

void freeWorld(struct world* world) { // assumes all chunks are unloaded
	BEGIN_HASHMAP_ITERATION(world->regions)
	freeRegion (value);
	END_HASHMAP_ITERATION(world->regions)
//pthread_rwlock_destroy(&world->chl);
	del_hashmap(world->regions);
	del_hashmap(world->entities);
	del_hashmap(world->chunks);
	BEGIN_HASHMAP_ITERATION(world->players)
	freePlayer(value);
	END_HASHMAP_ITERATION(world->players)
	del_hashmap(world->players);
	BEGIN_HASHMAP_ITERATION(world->scheduledTicks)
	xfree(value);
	END_HASHMAP_ITERATION(world->scheduledTicks)
	del_hashmap(world->scheduledTicks);
	if (world->level != NULL) {
		freeNBT(world->level);
		xfree(world->level);
	}
	if (world->lpa != NULL) xfree(world->lpa);
	xfree(world);
}

struct chunk* getEntityChunk(struct entity* entity) {
	return getChunk(entity->world, ((int32_t) entity->x) >> 4, ((int32_t) entity->z) >> 4);
}

void spawnEntity(struct world* world, struct entity* entity) {
	entity->world = world;
	if (entity->loadingPlayers == NULL) entity->loadingPlayers = new_hashmap(1, 1);
	put_hashmap(world->entities, entity->id, entity);
//struct chunk* ch = getEntityChunk(entity);
//if (ch != NULL) {
//	put_hashmap(ch->entities, entity->id, entity);
//}
}

void spawnPlayer(struct world* world, struct player* player) {
	player->world = world;
	if (player->loadedEntities == NULL) player->loadedEntities = new_hashmap(1, 0);
	if (player->loadedChunks == NULL) player->loadedChunks = new_hashmap(1, 1);
	put_hashmap(world->players, player->entity->id, player);
	BEGIN_HASHMAP_ITERATION(world->subworlds)
	struct subworld* sw = value;
	if (sw->players->entry_count < 100 && !sw->defunct) {
		put_hashmap(sw->players, player->entity->id, player);
		player->subworld = sw;
		BREAK_HASHMAP_ITERATION(world->subworlds)
		goto se;
	}
	END_HASHMAP_ITERATION(world->subworlds)
//no subworld with < 100 players
	struct subworld* sw = xmalloc(sizeof(struct subworld));
	sw->world = world;
	sw->players = new_hashmap(1, 1);
	sw->defunct = 0;
	player->subworld = sw;
	put_hashmap(world->subworlds, (uint64_t) sw, sw);
	pthread_t swt;
	pthread_create(&swt, NULL, &thr_player_tick, sw);
	put_hashmap(sw->players, player->entity->id, player); // no ticks until next tick!
	se: ;
	spawnEntity(world, player->entity);
}

void despawnPlayer(struct world* world, struct player* player) {
	despawnEntity(world, player->entity);
	pthread_rwlock_unlock(&player->subworld->players->data_mutex);
	put_hashmap(player->subworld->players, player->entity->id, NULL);
	pthread_rwlock_rdlock(&player->subworld->players->data_mutex);
	BEGIN_HASHMAP_ITERATION(player->loadedEntities)
	if (value == NULL || value == player->entity) continue;
	struct entity* ent = (struct entity*) value;
	put_hashmap(ent->loadingPlayers, player->entity->id, NULL);
	END_HASHMAP_ITERATION(player->loadedEntities)
	del_hashmap(player->loadedEntities);
	player->loadedEntities = NULL;
	BEGIN_HASHMAP_ITERATION(player->loadedChunks)
	struct chunk* pl = (struct chunk*) value;
	if (--pl->playersLoaded <= 0) {
		unloadChunk(world, pl);
	}
	END_HASHMAP_ITERATION(player->loadedChunks)
	del_hashmap(player->loadedChunks);
	player->loadedChunks = NULL;
	put_hashmap(world->players, player->entity->id, NULL);
	if (player->subworld->players->entry_count <= 0) player->subworld->defunct = 1;
}

void despawnEntity(struct world* world, struct entity* entity) {
//struct chunk* ch = getEntityChunk(entity);
	put_hashmap(world->entities, entity->id, NULL);
//put_hashmap(ch->entities, entity->id, NULL);
	BEGIN_BROADCAST(entity->loadingPlayers)
	struct packet* pkt = xmalloc(sizeof(struct packet));
	pkt->id = PKT_PLAY_CLIENT_DESTROYENTITIES;
	pkt->data.play_client.destroyentities.count = 1;
	pkt->data.play_client.destroyentities.entity_ids = xmalloc(sizeof(int32_t));
	pkt->data.play_client.destroyentities.entity_ids[0] = entity->id;
	add_queue(bc_player->outgoingPacket, pkt);
	put_hashmap(bc_player->loadedEntities, entity->id, NULL);
	END_BROADCAST(entity->loadingPlayers)
	del_hashmap(entity->loadingPlayers);
	entity->loadingPlayers = NULL;
}

struct entity* getEntity(struct world* world, int32_t id) {
	return get_hashmap(world->entities, id);
}

void updateBlockWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return;
	block b = getBlockWorld_guess(world, ch, x, y, z);
	struct block_info* bi = getBlockInfo(b);
	if (bi != NULL && bi->onBlockUpdate != NULL) bi->onBlockUpdate(world, b, x, y, z);
}

void updateBlockWorld(struct world* world, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return;
	block b = getBlockWorld(world, x, y, z);
	struct block_info* bi = getBlockInfo(b);
	if (bi != NULL && bi->onBlockUpdate != NULL) bi->onBlockUpdate(world, b, x, y, z);
}
