/*
 * world.c
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#include "basin/network.h"
#include "packet.h"
#include <basin/game.h>
#include <basin/block.h>
#include <basin/queue.h>
#include <basin/tileentity.h>
#include <basin/entity.h>
#include <basin/world.h>
#include <basin/globals.h>
#include <basin/profile.h>
#include <basin/server.h>
#include <basin/ai.h>
#include <basin/plugin.h>
#include <basin/perlin.h>
#include <basin/biome.h>
#include <basin/entity.h>
#include <basin/player.h>
#include <basin/nbt.h>
#include <basin/worldmanager.h>
#include <avuna/prqueue.h>
#include <avuna/string.h>
#include <avuna/pmem.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <zlib.h>
#include <dirent.h>
#include <math.h>
#include <errno.h>

int boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2) {
	return (((bb1->minX >= bb2->minX && bb1->minX <= bb2->maxX) || (bb1->maxX >= bb2->minX && bb1->maxX <= bb2->maxX) || (bb2->minX >= bb1->minX && bb2->minX <= bb1->maxX) || (bb2->maxX >= bb1->minX && bb2->maxX <= bb1->maxX)) && ((bb1->minY >= bb2->minY && bb1->minY <= bb2->maxY) || (bb1->maxY >= bb2->minY && bb1->maxY <= bb2->maxY) || (bb2->minY >= bb1->minY && bb2->minY <= bb1->maxY) || (bb2->maxY >= bb1->minY && bb2->maxY <= bb1->maxY)) && ((bb1->minZ >= bb2->minZ && bb1->minZ <= bb2->maxZ) || (bb1->maxZ >= bb2->minZ && bb1->maxZ <= bb2->maxZ) || (bb2->minZ >= bb1->minZ && bb2->minZ <= bb1->maxZ) || (bb2->maxZ >= bb1->minZ && bb2->maxZ <= bb1->maxZ)));
}

uint64_t getChunkKey(struct chunk* ch) {
	return (uint64_t)(((int64_t) ch->x) << 32) | (((int64_t) ch->z) & 0xFFFFFFFF);
}

uint64_t getChunkKey2(int32_t cx, int32_t cz) {
	return (uint64_t)(((int64_t) cx) << 32) | (((int64_t) cz) & 0xFFFFFFFF);
}

struct chunk* getChunk_guess(struct world* world, struct chunk* ch, int32_t x, int32_t z) {
	if (ch == NULL) return getChunk(world, x, z);
	if (ch->x == x && ch->z == z) return ch;
	if (abs(x - ch->x) > 3 || abs(z - ch->z) > 3) return getChunk(world, x, z);
	struct chunk* cch = ch;
	while (cch != NULL) {
		if (cch->x > x) cch = cch->xn;
		else if (cch->x < x) cch = cch->xp;
		if (cch != NULL) {
			if (cch->z > z) cch = cch->zn;
			else if (cch->z < z) cch = cch->zp;
		}
		if (cch != NULL && cch->x == x && cch->z == z) return cch;
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
	if (nbt_read(&nbt, rtbuf, rts) < 0) {
		xfree(rtbuf);
		return NULL;
	}
	xfree(rtbuf);
	struct nbt_tag* level = nbt_get(nbt, "Level");
	if (level == NULL || level->id != NBT_TAG_COMPOUND) goto rerx;
	struct nbt_tag* tmp = nbt_get(level, "xPos");
	if (tmp == NULL || tmp->id != NBT_TAG_INT) goto rerx;
	int32_t xPos = tmp->data.nbt_int;
	tmp = nbt_get(level, "zPos");
	if (tmp == NULL || tmp->id != NBT_TAG_INT) goto rerx;
	int32_t zPos = tmp->data.nbt_int;
	struct chunk* rch = newChunk(xPos, zPos);
	tmp = nbt_get(level, "LightPopulated");
	if (tmp == NULL || tmp->id != NBT_TAG_BYTE) goto rerx;
	rch->lightpopulated = tmp->data.nbt_byte;
	tmp = nbt_get(level, "TerrainPopulated");
	if (tmp == NULL || tmp->id != NBT_TAG_BYTE) goto rerx;
	rch->terrainpopulated = tmp->data.nbt_byte;
	tmp = nbt_get(level, "InhabitedTime");
	if (tmp == NULL || tmp->id != NBT_TAG_LONG) goto rerx;
	rch->inhabitedticks = tmp->data.nbt_long;
	tmp = nbt_get(level, "Biomes");
	if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY) goto rerx;
	if (tmp->data.nbt_bytearray.len == 256) memcpy(rch->biomes, tmp->data.nbt_bytearray.data, 256);
	tmp = nbt_get(level, "HeightMap");
	if (tmp == NULL || tmp->id != NBT_TAG_INTARRAY) goto rerx;
	if (tmp->data.nbt_intarray.count == 256) for (int i = 0; i < 256; i++)
		rch->heightMap[i >> 4][i & 0x0F] = (uint16_t) tmp->data.nbt_intarray.ints[i];
	struct nbt_tag* tes = nbt_get(level, "TileEntities");
	if (tes == NULL || tes->id != NBT_TAG_LIST) goto rerx;
	for (size_t i = 0; i < tes->children_count; i++) {
		struct nbt_tag* ten = tes->children[i];
		if (ten == NULL || ten->id != NBT_TAG_COMPOUND) continue;
		struct tile_entity* te = parseTileEntity(ten);
		add_collection(rch->tileEntities, te);
	}
	//TODO: tileticks
	struct nbt_tag* sections = nbt_get(level, "Sections");
	for (size_t i = 0; i < sections->children_count; i++) {
		struct nbt_tag* section = sections->children[i];
		tmp = nbt_get(section, "Y");
		if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
		uint8_t y = tmp->data.nbt_byte;
		tmp = nbt_get(section, "Blocks");
		if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 4096) continue;
		int hna = 0;
		block* rbl = xmalloc(sizeof(block) * 4096);
		for (int i = 0; i < 4096; i++) {
			rbl[i] = ((block) tmp->data.nbt_bytearray.data[i]) << 4; // [i >> 8][(i & 0xf0) >> 4][i & 0x0f]
			if (((block) tmp->data.nbt_bytearray.data[i]) != 0) hna = 1;
		}
		//if (hna) rch->empty[y] = 0;
		tmp = nbt_get(section, "Add");
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
			tmp = nbt_get(section, "Data");
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
			tmp = nbt_get(section, "BlockLight");
			if (tmp != NULL) {
				if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
				memcpy(cs->blockLight, tmp->data.nbt_bytearray.data, 2048);
			}
			tmp = nbt_get(section, "SkyLight");
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

const uint16_t generableBiomes[] = { BIOME_OCEAN, BIOME_PLAINS, BIOME_DESERT, BIOME_EXTREME_HILLS, BIOME_FOREST, BIOME_TAIGA, BIOME_SWAMPLAND };
const uint16_t generableBiomesCount = 7;

struct chunk* generateRegularChunk(struct world* world, struct chunk* chunk) {
	for (int32_t cx = 0; cx < 16; cx++) {
		for (int32_t cz = 0; cz < 16; cz++) {
			int32_t x = cx + ((int32_t) chunk->x) * 16;
			int32_t z = cz + ((int32_t) chunk->z) * 16;
			double px = ((double) x + .5);
			double py = .5 * .05;
			double pz = ((double) z + .5);
			uint16_t bi = (uint16_t)(floor((perlin_octave(&world->perlin, px, py, pz, 7., .0005, 7, .6) + 3.5)));
			if (bi < 0) bi = 0;
			if (bi >= generableBiomesCount) bi = generableBiomesCount - 1;
			uint16_t biome = generableBiomes[bi];
			chunk->biomes[cz][cx] = biome;
			double ph = 0.;
			block topSoil = BLK_GRASS;
			block subSoil = BLK_DIRT;
			block oreContainer = BLK_STONE;
			if (biome == BIOME_OCEAN) {
				topSoil = BLK_SAND;
				subSoil = BLK_SAND;
				ph = perlin_octave(&world->perlin, px, py, pz, 5., .05, 2, .25) + 2. - 32.;
			} else if (biome == BIOME_PLAINS) {
				ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
			} else if (biome == BIOME_DESERT) {
				topSoil = BLK_SAND;
				subSoil = BLK_SAND;
				ph = perlin_octave(&world->perlin, px, py, pz, 3., .03, 2, .25) + 2.;
			} else if (biome == BIOME_EXTREME_HILLS) {
				ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
				double hills = perlin_octave(&world->perlin, px, py, pz, 60., .05, 2, .25) + 20.;
				ph += hills;
			} else if (biome == BIOME_FOREST) {
				ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
			} else if (biome == BIOME_TAIGA) {
				ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
			} else if (biome == BIOME_SWAMPLAND) {
				ph = perlin_octave(&world->perlin, px, py, pz, 3., .05, 2, .25) + 2.;
			}
			int32_t terrainHeight = (int32_t)(ph) + 64;
			if (terrainHeight < 0) terrainHeight = 0;
			if (terrainHeight > 255) terrainHeight = 255;
			for (int32_t y = 0; y < terrainHeight; y++) {
				setBlockChunk(chunk, y < 5 ? BLK_BEDROCK : (y == (terrainHeight - 1) ? topSoil : (y >= (terrainHeight - 4) ? subSoil : oreContainer)), cx, y, cz, world->dimension == OVERWORLD);
			}
			if (biome == BIOME_OCEAN || biome == BIOME_EXTREME_HILLS) {
				for (int32_t y = terrainHeight; y < 64; y++) {
					setBlockChunk(chunk, BLK_WATER_1, cx, y, cz, world->dimension == OVERWORLD);
				}
			}
		}
	}
	return chunk;
}

struct chunk* generateChunk(struct world* world, struct chunk* chunk) {
	memset(chunk->sections, 0, sizeof(struct chunk_section*) * 16);
	int pluginChunked = 0;
	BEGIN_HASHMAP_ITERATION (plugins)
	struct plugin* plugin = value;
	if (plugin->generateChunk != NULL) {
		struct chunk* pchunk = (*plugin->generateChunk)(world, chunk);
		if (pchunk != NULL) {
			chunk = pchunk;
			pluginChunked = 1;
			BREAK_HASHMAP_ITERATION (plugins)
			break;
		}
	}
	END_HASHMAP_ITERATION (plugins)
	if (!pluginChunked) {
		generateRegularChunk(world, chunk);
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
	//TODO: ensure that on world change that the chunk queue is cleared for a player
	while (1) {
		nm: ;
		pthread_mutex_lock (&chunk_wake_mut);
		pthread_cond_wait(&chunk_wake, &chunk_wake_mut);
		pthread_mutex_unlock(&chunk_wake_mut);
		if (players->entry_count == 0 && globalChunkQueue->size == 0) goto nm;
		if (globalChunkQueue->size > 0) {
			struct chunk_req* cr = NULL;
			while ((cr = pop_nowait_queue(globalChunkQueue)) != NULL) {
				if (cr->load) {
					beginProfilerSection("chunkLoading_getChunk");
					struct chunk* ch = getChunkWithLoad(cr->world, cr->cx, cr->cz, b);
					if (ch != NULL) ch->playersLoaded++;
					endProfilerSection("chunkLoading_getChunk");
				} else {
					struct chunk* ch = getChunk(cr->world, cr->cx, cr->cz);
					if (ch != NULL && !ch->defunct) {
						if (--ch->playersLoaded <= 0) {
							unloadChunk(cr->world, ch);
						}
					}
				}
			}
		}
		BEGIN_HASHMAP_ITERATION (players)
		struct player* player = value;
		if (player->defunct || player->chunksSent >= 5 || player->chunkRequests->size == 0 || (player->conn != NULL && player->conn->writeBuffer_size > 1024 * 1024 * 128)) continue;
		struct chunk_req* chr = pop_nowait_queue(player->chunkRequests);
		if (chr == NULL) continue;
		if (chr->load) {
			if (chr->world != player->world) {
				xfree(chr);
				continue;
			}
			player->chunksSent++;
			if (contains_hashmap(player->loadedChunks, getChunkKey2(chr->cx, chr->cz))) {
				xfree(chr);
				continue;
			}
			beginProfilerSection("chunkLoading_getChunk");
			struct chunk* ch = getChunkWithLoad(player->world, chr->cx, chr->cz, b);
			if (player->loadedChunks == NULL) {
				xfree(chr);
				endProfilerSection("chunkLoading_getChunk");
				continue;
			}
			endProfilerSection("chunkLoading_getChunk");
			if (ch != NULL) {
				ch->playersLoaded++;
				//beginProfilerSection("chunkLoading_sendChunk_add");
				put_hashmap(player->loadedChunks, getChunkKey(ch), ch);
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
				add_queue(player->outgoingPacket, pkt);
				flush_outgoing(player);
				//endProfilerSection("chunkLoading_sendChunk_dispatch");
			}
		} else {
			//beginProfilerSection("unchunkLoading");
			struct chunk* ch = getChunk(player->world, chr->cx, chr->cz);
			uint64_t ck = getChunkKey2(chr->cx, chr->cz);
			if (get_hashmap(player->loadedChunks, ck) == NULL) {
				xfree(chr);
				continue;
			}
			put_hashmap(player->loadedChunks, ck, NULL);
			if (ch != NULL && !ch->defunct) {
				if (--ch->playersLoaded <= 0) {
					unloadChunk(player->world, ch);
				}
			}
			if (chr->world == player->world) {
				struct packet* pkt = xmalloc(sizeof(struct packet));
				pkt->id = PKT_PLAY_CLIENT_UNLOADCHUNK;
				pkt->data.play_client.unloadchunk.chunk_x = chr->cx;
				pkt->data.play_client.unloadchunk.chunk_z = chr->cz;
				pkt->data.play_client.unloadchunk.ch = NULL;
				add_queue(player->outgoingPacket, pkt);
				flush_outgoing(player);
			}
			//endProfilerSection("unchunkLoading");
		}
		xfree(chr);
		END_HASHMAP_ITERATION (players)
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
	pthread_rwlock_wrlock(&world->chunks->data_mutex);
	if (chunk->xp != NULL) chunk->xp->xn = NULL;
	if (chunk->xn != NULL) chunk->xn->xp = NULL;
	if (chunk->zp != NULL) chunk->zp->zn = NULL;
	if (chunk->zn != NULL) chunk->zn->zp = NULL;
	pthread_rwlock_unlock(&world->chunks->data_mutex);
	put_hashmap(world->chunks, getChunkKey(chunk), NULL);
	add_collection(defunctChunks, chunk);
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
	ch = getChunk_guess(world, ch, x >> 4, z >> 4);
	if (ch != NULL) return getLightChunk(ch, x & 0x0f, y, z & 0x0f, world->skylightSubtracted);
	else return getLightWorld(world, x, y, z, 0);
}

uint8_t getRawLightWorld_guess(struct world* world, struct chunk* ch, int32_t x, int32_t y, int32_t z, uint8_t blocklight) {
	if (y < 0 || y > 255) return 0;
	ch = getChunk_guess(world, ch, x >> 4, z >> 4);
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

int wrt_intermediate(double v1x, double v1y, double v1z, double v2x, double v2y, double v2z, double coord, int ct, double* rx, double* ry, double* rz) {
	double dx = v2x - v1x;
	double dy = v2y - v1y;
	double dz = v2z - v1z;
	double* mcd = NULL;
	double* mc = NULL;
	if (ct == 0) {
		mcd = &dx;
		mc = &v1x;
	} else if (ct == 1) {
		mcd = &dy;
		mc = &v1y;
	} else if (ct == 2) {
		mcd = &dz;
		mc = &v1z;
	}
	if ((*mcd) * (*mcd) < 1.0000000116860974e-7) return 1;
	else {
		double dc = (coord - (*mc)) / (*mcd);
		if (dc >= 0. && dc <= 1.) {
			*rx = v1x + dx * dc;
			*ry = v1y + dy * dc;
			*rz = v1z + dz * dc;
			return 0;
		} else return 1;
	}
}

int wrt_intersectsPlane(double rc1, double rc2, double minc1, double minc2, double maxc1, double maxc2) {
	return rc1 >= minc1 && rc1 <= maxc1 && rc2 >= minc2 && rc2 <= maxc2;
}

int world_isColliding(struct block_info* bi, int32_t x, int32_t y, int32_t z, double px, double py, double pz) {
	for (size_t i = 0; i < bi->boundingBox_count; i++) {
		struct boundingbox* bb = &bi->boundingBoxes[i];
		if (bb->minX + x < px && bb->maxX + x > px && bb->minY + y < py && bb->maxY + y > py && bb->minZ + z < pz && bb->maxZ + z > pz) return 1;
	}
	return 0;
}

int wrt_isClosest(double tx, double ty, double tz, double x1, double y1, double z1, double x2, double y2, double z2) {
	return (x1 - tx) * (x1 - tx) + (y1 - ty) * (y1 - ty) + (z1 - tz) * (z1 - tz) > (x2 - tx) * (x2 - tx) + (y2 - ty) * (y2 - ty) + (z2 - tz) * (z2 - tz);
}

int world_blockRayTrace(struct boundingbox* bb, int32_t x, int32_t y, int32_t z, double px, double py, double pz, double ex, double ey, double ez, double *qx, double* qy, double* qz) {
	double bx = ex;
	double by = ey;
	double bz = ez;
	double rx = 0.;
	double ry = 0.;
	double rz = 0.;
	int face = -1;
	if (!wrt_intermediate(px, py, pz, ex, ey, ez, bb->minX + x, 0, &rx, &ry, &rz) && wrt_intersectsPlane(ry, rz, bb->minY + y, bb->maxY + y, bb->minZ + z, bb->maxZ + z)) {
		face = XN;
		bx = rx;
		by = ry;
		bz = rz;
	}
	if (!wrt_intermediate(px, py, pz, ex, ey, ez, bb->maxX + x, 0, &rx, &ry, &rz) && wrt_intersectsPlane(ry, rz, bb->minX + x, bb->maxX + x, bb->minZ + z, bb->maxZ + z) && wrt_isClosest(px, py, pz, bx, by, bz, rx, ry, rz)) {
		face = XP;
		bx = rx;
		by = ry;
		bz = rz;
	}
	if (!wrt_intermediate(px, py, pz, ex, ey, ez, bb->minY + y, 1, &rx, &ry, &rz) && wrt_intersectsPlane(rx, rz, bb->minX + x, bb->maxX + x, bb->minZ + z, bb->maxZ + z) && wrt_isClosest(px, py, pz, bx, by, bz, rx, ry, rz)) {
		face = YN;
		bx = rx;
		by = ry;
		bz = rz;
	}
	if (!wrt_intermediate(px, py, pz, ex, ey, ez, bb->maxY + y, 1, &rx, &ry, &rz) && wrt_intersectsPlane(rx, rz, bb->minX + x, bb->maxX + x, bb->minZ + z, bb->maxZ + z) && wrt_isClosest(px, py, pz, bx, by, bz, rx, ry, rz)) {
		face = YP;
		bx = rx;
		by = ry;
		bz = rz;
	}
	if (!wrt_intermediate(px, py, pz, ex, ey, ez, bb->minZ + z, 2, &rx, &ry, &rz) && wrt_intersectsPlane(rx, ry, bb->minX + x, bb->maxX + x, bb->minY + y, bb->maxY + y) && wrt_isClosest(px, py, pz, bx, by, bz, rx, ry, rz)) {
		face = ZN;
		bx = rx;
		by = ry;
		bz = rz;
	}
	if (!wrt_intermediate(px, py, pz, ex, ey, ez, bb->maxZ + z, 2, &rx, &ry, &rz) && wrt_intersectsPlane(rx, ry, bb->minX + x, bb->maxX + x, bb->minY + y, bb->maxY + y) && wrt_isClosest(px, py, pz, bx, by, bz, rx, ry, rz)) {
		face = ZP;
		bx = rx;
		by = ry;
		bz = rz;
	}
	*qx = bx;
	*qy = by;
	*qz = bz;
	return face;
}

int world_rayTrace(struct world* world, double x, double y, double z, double ex, double ey, double ez, int stopOnLiquid, int ignoreNonCollidable, int returnLast, double* rx, double* ry, double* rz) {
	int32_t ix = (uint32_t) floor(x);
	int32_t iy = (uint32_t) floor(y);
	int32_t iz = (uint32_t) floor(z);
	int32_t iex = (uint32_t) floor(ex);
	int32_t iey = (uint32_t) floor(ey);
	int32_t iez = (uint32_t) floor(ez);
	block b = getBlockWorld(world, ix, iy, iz);
	struct block_info* bbi = getBlockInfo(b);
	if (bbi != NULL && (!ignoreNonCollidable || bbi->boundingBox_count > 0)) {
		double bx = 0.;
		double by = 0.;
		double bz = 0.;
		for (size_t i = 0; i < bbi->boundingBox_count; i++) {
			struct boundingbox* bb = &bbi->boundingBoxes[i];
			int face = world_blockRayTrace(bb, ix, iy, iz, x, y, z, ex, ey, ez, rx, ry, rz);
			if (face >= 0) return face;
		}
	}
	int k = 200;
	int cface = -1;
	while (k-- >= 0) {
		if (ix == iex && iy == iey && iz == iez) {
			return returnLast ? cface : -1;
		}
		int hx = 1;
		int hy = 1;
		int hz = 1;
		double mX = 999.;
		double mY = 999.;
		double mZ = 999.;
		if (iex > ix) mX = (double) ix + 1.;
		else if (iex < ix) mX = (double) ix;
		else hx = 0;
		if (iey > iy) mY = (double) iy + 1.;
		else if (iey < iy) mY = (double) iy;
		else hy = 0;
		if (iez > iz) mZ = (double) iz + 1.;
		else if (iez < iz) mZ = (double) iz;
		else hz = 0;
		double ax = 999.;
		double ay = 999.;
		double az = 999.;
		double dx = ex - x;
		double dy = ey - y;
		double dz = ez - z;
		if (hx) ax = (mX - x) / dx;
		if (hy) ay = (mY - y) / dy;
		if (hz) az = (mZ - z) / dz;
		if (ax == 0.) ax = -1e-4;
		if (ay == 0.) ay = -1e-4;
		if (az == 0.) az = -1e-4;
		if (ax < ay && ax < az) {
			cface = iex > ix ? XN : XP;
			x = mX;
			y += dy * ax;
			z += dz * ax;
		} else if (ay < az) {
			cface = iey > iy ? YN : YP;
			x += dx * ay;
			y = mY;
			z += dz * ay;
		} else {
			cface = iez > iz ? ZN : ZP;
			x += dx * az;
			y += dy * az;
			z = mZ;
		}
		ix = floor(x) - (cface == XP ? 1 : 0);
		iy = floor(y) - (cface == YP ? 1 : 0);
		iz = floor(z) - (cface == ZP ? 1 : 0);
		block nb = getBlockWorld(world, ix, iy, iz);
		struct block_info* bi = getBlockInfo(nb);
		if (bi != NULL && (!ignoreNonCollidable || streq_nocase(bi->material->name, "portal") || bi->boundingBox_count > 0)) {
			//todo: cancollidecheck?
			for (size_t i = 0; i < bi->boundingBox_count; i++) {
				struct boundingbox* bb = &bi->boundingBoxes[i];
				int face = world_blockRayTrace(bb, ix, iy, iz, x, y, z, ex, ey, ez, rx, ry, rz);
				if (face >= 0) return face;
			}
			//TODO: returnlast finish
		}
	}
	return returnLast ? cface : -1;
}

void explode(struct world* world, struct chunk* ch, double x, double y, double z, float strength) { // TODO: more plugin stuff?
	ch = getChunk_guess(world, ch, (int32_t) floor(x) >> 4, (int32_t) floor(z) >> 4);
	for (int32_t j = 0; j < 16; j++) {
		for (int32_t k = 0; k < 16; k++) {
			for (int32_t l = 0; l < 16; l++) {
				if (!(j == 0 || j == 15 || k == 0 || k == 15 || l == 0 || l == 15)) continue;
				double dx = (double) j / 15.0 * 2.0 - 1.0;
				double dy = (double) k / 15.0 * 2.0 - 1.0;
				double dz = (double) l / 15.0 * 2.0 - 1.0;
				double d = sqrt(dx * dx + dy * dy + dz * dz);
				dx /= d;
				dy /= d;
				dz /= d;
				float mstr = strength * (.7 + randFloat() * .6);
				double x2 = x;
				double y2 = y;
				double z2 = z;
				for (; mstr > 0.; mstr -= .225) {
					int32_t ix = (int32_t) floor(x2);
					int32_t iy = (int32_t) floor(y2);
					int32_t iz = (int32_t) floor(z2);
					block b = getBlockWorld_guess(world, ch, ix, iy, iz);
					if (b >> 4 != 0) {
						struct block_info* bi = getBlockInfo(b);
						mstr -= ((bi->resistance / 5.) + .3) * .3;
						if (mstr > 0.) { //TODO: check if entity will allow destruction
							block b = getBlockWorld_guess(world, ch, ix, iy, iz);
							setBlockWorld_guess(world, ch, 0, ix, iy, iz);
							dropBlockDrops(world, b, NULL, ix, iy, iz); //TODO: randomizE?
						}
					}
					x2 += dx * .3;
					y2 += dy * .3;
					z2 += dz * .3;
				}
			}
		}
	}
	//TODO: knockback & damage
}

void setTileEntityChunk(struct chunk* chunk, struct tile_entity* te, int32_t x, uint8_t y, int32_t z) {
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
	setTileEntityChunk(chunk, te, x, y, z);
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

void scheduleBlockTick(struct world* world, int32_t x, int32_t y, int32_t z, int32_t ticksFromNow, float priority) {
	if (y < 0 || y > 255) return;
	struct scheduled_tick* st = xmalloc(sizeof(struct scheduled_tick));
	st->x = x;
	st->y = y;
	st->z = z;
	st->ticksLeft = ticksFromNow;
	st->priority = priority;
	st->src = getBlockWorld(world, x, y, z);
	struct encpos ep;
	ep.x = x;
	ep.y = y;
	ep.z = z;
	put_hashmap(world->scheduledTicks, *((uint64_t*) &ep), st);
}

int32_t isBlockTickScheduled(struct world* world, int32_t x, int32_t y, int32_t z) {
	struct encpos ep;
	ep.x = x;
	ep.y = y;
	ep.z = z;
	struct scheduled_tick* st = get_hashmap(world->scheduledTicks, *((uint64_t*) &ep));
	if (st == NULL) {
		return 0;
	}
	return st->ticksLeft;
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
		if (bii != NULL) {
			if (bii->lightOpacity >= 1) {
				if (chunk->heightMap[z][x] <= y) chunk->heightMap[z][x] = y + 1;
				else {
					for (int ny = chunk->heightMap[z][x] - 1; ny >= 0; ny--) {
						struct block_info* nbi = getBlockInfo(getBlockChunk(chunk, x, y, z));
						if (nbi != NULL && nbi->lightOpacity >= 1) {
							chunk->heightMap[z][x] = ny + 1;
							break;
						}
					}
				}
			} else if (chunk->heightMap[z][x] > y) {
				for (int ny = chunk->heightMap[z][x] - 1; ny >= 0; ny--) {
					struct block_info* nbi = getBlockInfo(getBlockChunk(chunk, x, y, z));
					if (nbi != NULL && nbi->lightOpacity >= 1) {
						chunk->heightMap[z][x] = ny + 1;
						break;
					}
				}
			}
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

int setBlockWorld(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return 1;
	struct chunk* ch = getChunk(world, x >> 4, z >> 4);
	if (ch == NULL) return 1;
	return setBlockWorld_guess(world, ch, blk, x, y, z);
}

int setBlockWorld_noupdate(struct world* world, block blk, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return 1;
	struct chunk* ch = getChunk(world, x >> 4, z >> 4);
	if (ch == NULL) return 1;
	return setBlockWorld_guess_noupdate(world, ch, blk, x, y, z);
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
	if (skylight && lp->y <= getHeightMapWorld_guess(world, chunk, lp->x, lp->z)) {
		struct block_info* bi = getBlockInfo(getBlockWorld_guess(world, chunk, lp->x, lp->y + 1, lp->z));
		if (bi != NULL && bi->lightOpacity <= 16) {
			struct world_lightpos lp2;
			lp2.x = lp->x;
			lp2.y = lp->y + 1;
			lp2.z = lp->z;
			light_floodfill(world, chunk, &lp2, skylight, subtract, subt_upd);
		}
	}
	struct block_info* bi = getBlockInfo(getBlockWorld_guess(world, chunk, lp->x, lp->y, lp->z));
	int lo = bi == NULL ? 1 : bi->lightOpacity;
	if (lo < 1) lo = 1;
	int le = bi == NULL ? 0 : bi->lightEmission;
	int16_t maxl = 0;
	uint8_t xpl = getRawLightWorld_guess(world, chunk, lp->x + 1, lp->y, lp->z, !skylight);
	uint8_t xnl = getRawLightWorld_guess(world, chunk, lp->x - 1, lp->y, lp->z, !skylight);
	uint8_t ypl = getRawLightWorld_guess(world, chunk, lp->x, lp->y + 1, lp->z, !skylight);
	uint8_t ynl = getRawLightWorld_guess(world, chunk, lp->x, lp->y - 1, lp->z, !skylight);
	uint8_t zpl = getRawLightWorld_guess(world, chunk, lp->x, lp->y, lp->z + 1, !skylight);
	uint8_t znl = getRawLightWorld_guess(world, chunk, lp->x, lp->y, lp->z - 1, !skylight);
	if (subtract) {
		maxl = getRawLightWorld_guess(world, chunk, lp->x, lp->y, lp->z, !skylight) - subtract;
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
//if (maxl < 15) {
	int ks = 0;
	if ((subtract - lo) > 0) {
		subtract -= lo;
	} else {
		ks = 1;
	}
//}
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
	if (ks) return 1;
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

int setBlockWorld_guess(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return 1;
	chunk = getChunk_guess(world, chunk, x >> 4, z >> 4);
	if (chunk == NULL) chunk = getChunk(world, x >> 4, z >> 4);
	if (chunk == NULL) return 1;
	block ob = getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
	struct block_info* obi = getBlockInfo(ob);
	uint16_t ohm = world->dimension == OVERWORLD ? chunk->heightMap[z & 0x0f][x & 0x0f] : 0;
	struct world_lightpos lp;
	int pchm = 0;
	struct hashmap* nup = NULL;
	if (obi != NULL) {
		int ict = 0;
		if (ob != blk) {
			if (obi->onBlockDestroyed != NULL) ict = (*obi->onBlockDestroyed)(world, ob, x, y, z, blk);
			if (!ict) {
				BEGIN_HASHMAP_ITERATION (plugins)
				struct plugin* plugin = value;
				if (plugin->onBlockDestroyed != NULL && (*plugin->onBlockDestroyed)(world, ob, x, y, z, blk)) {
					ict = 1;
					BREAK_HASHMAP_ITERATION (plugins)
					break;
				}
				END_HASHMAP_ITERATION (plugins)
			}
			if (ict) return 1;
		}
		if (world->dimension == OVERWORLD && ((y >= ohm && obi->lightOpacity >= 1) || (y < ohm && obi->lightOpacity == 0))) {
			pchm = 1;
			nup = new_hashmap(1, 0);
			lp.x = x;
			lp.y = ohm;
			lp.z = z;
			light_floodfill(world, chunk, &lp, 1, 15, nup); // todo remove nup duplicates
		}
	}
	pnbi: ;
	struct block_info* nbi = getBlockInfo(blk);
	if (nbi != NULL && blk != ob) {
		int ict = 0;
		block obb = blk;
		if (nbi->onBlockPlaced != NULL) blk = (*nbi->onBlockPlaced)(world, blk, x, y, z, ob);
		if (blk == 0 && obb != 0) ict = 1;
		else if (blk != obb) goto pnbi;
		if (!ict) {
			BEGIN_HASHMAP_ITERATION (plugins)
			struct plugin* plugin = value;
			if (plugin->onBlockPlaced != NULL) {
				blk = (*plugin->onBlockPlaced)(world, blk, x, y, z, ob);
				if (blk == 0 && obb != 0) {
					ict = 1;
					BREAK_HASHMAP_ITERATION (plugins)
					break;
				} else if (blk != obb) goto pnbi;
			}
			END_HASHMAP_ITERATION (plugins)
		}
		if (ict) return 1;
	}
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
	if (nbi == NULL || obi == NULL) return 0;
	if (world->dimension == OVERWORLD) {
		if (pchm || obi->lightOpacity != nbi->lightOpacity) {
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
			if (pchm) { // todo: remove the light before block change
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
			}
			lp.x = x;
			lp.y = y;
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
	if (obi->lightEmission != nbi->lightEmission || obi->lightOpacity != nbi->lightOpacity) {
		if (obi->lightEmission <= nbi->lightEmission) {
			beginProfilerSection("blocklight_update_equals");
			struct world_lightpos lp;
			lp.x = x;
			lp.y = y;
			lp.z = z;
			light_floodfill(world, chunk, &lp, 0, 0, NULL);
			endProfilerSection("blocklight_update_equals");
		} else {
			beginProfilerSection("blocklight_update_remlight");
			nup = new_hashmap(1, 0);
			lp.x = x;
			lp.y = y;
			lp.z = z;
			light_floodfill(world, chunk, &lp, 0, obi->lightEmission, nup); // todo remove nup duplicates
			BEGIN_HASHMAP_ITERATION (nup)
			struct world_lightpos* nlp = value;
			light_floodfill(world, chunk, nlp, 0, 0, 0);
			xfree (value);
			END_HASHMAP_ITERATION (nup)
			del_hashmap(nup);
			//light_floodfill(world, chunk, &lp, 1, 0, 0);
			/*for (int32_t lx = x - 16; lx <= x + 16; lx++) {
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
			 }*/
			endProfilerSection("blocklight_update_remlight");
		}
	}
	endProfilerSection("blocklight_update");
	return 0;
}

int setBlockWorld_guess_noupdate(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z) {
	if (y < 0 || y > 255) return 1;
	chunk = getChunk_guess(world, chunk, x >> 4, z >> 4);
	if (chunk == NULL) return 1;
	block ob = getBlockChunk(chunk, x & 0x0f, y, z & 0x0f);
	struct block_info* obi = getBlockInfo(ob);
	if (obi != NULL) {
		int ict = 0;
		if (ob != blk) {
			if (obi->onBlockDestroyed != NULL) ict = (*obi->onBlockDestroyed)(world, ob, x, y, z, blk);
			if (!ict) {
				BEGIN_HASHMAP_ITERATION (plugins)
				struct plugin* plugin = value;
				if (plugin->onBlockDestroyed != NULL && (*plugin->onBlockDestroyed)(world, ob, x, y, z, blk)) {
					ict = 1;
					BREAK_HASHMAP_ITERATION (plugins)
					break;
				}
				END_HASHMAP_ITERATION (plugins)
			}
			if (ict) return 1;
		}
	}
	pnbi: ;
	struct block_info* nbi = getBlockInfo(blk);
	if (nbi != NULL && blk != ob) {
		int ict = 0;
		block obb = blk;
		if (nbi->onBlockPlaced != NULL) blk = (*nbi->onBlockPlaced)(world, blk, x, y, z, ob);
		if (blk == 0 && obb != 0) ict = 1;
		else if (blk != obb) goto pnbi;
		if (!ict) {
			BEGIN_HASHMAP_ITERATION (plugins)
			struct plugin* plugin = value;
			if (plugin->onBlockPlaced != NULL) {
				blk = (*plugin->onBlockPlaced)(world, blk, x, y, z, ob);
				if (blk == 0 && obb != 0) {
					ict = 1;
					BREAK_HASHMAP_ITERATION (plugins)
					break;
				} else if (blk != obb) goto pnbi;
			}
			END_HASHMAP_ITERATION (plugins)
		}
		if (ict) return 1;
	}
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
	return 0;
}

struct world* newWorld(size_t chl_count) {
	struct world* world = xmalloc(sizeof(struct world));
	memset(world, 0, sizeof(struct world));
	world->regions = new_hashmap(1, 1);
	world->entities = new_hashmap(1, 0);
	world->players = new_hashmap(1, 1);
	world->chunks = new_hashmap(1, 1);
	world->chl_count = chl_count;
	world->skylightSubtracted = 0;
	world->scheduledTicks = new_hashmap(1, 0);
	world->tps = 0;
	world->ticksInSecond = 0;
	world->seed = 9876543;
	perlin_init(&world->perlin, world->seed);
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
	ssize_t ds = nbt_decompress(ld, ldi, (void**) &nld);
	xfree(ld);
	if (ds < 0) {
		return -1;
	}
	if (nbt_read(&world->level, nld, ds) < 0) return -1;
	xfree(nld);
	struct nbt_tag* data = nbt_get(world->level, "Data");
	world->levelType = nbt_get(data, "generatorName")->data.nbt_string;
	world->spawnpos.x = nbt_get(data, "SpawnX")->data.nbt_int;
	world->spawnpos.y = nbt_get(data, "SpawnY")->data.nbt_int;
	world->spawnpos.z = nbt_get(data, "SpawnZ")->data.nbt_int;
	world->time = nbt_get(data, "DayTime")->data.nbt_long;
	world->age = nbt_get(data, "Time")->data.nbt_long;
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
	BEGIN_HASHMAP_ITERATION (plugins)
	struct plugin* plugin = value;
	if (plugin->onWorldLoad != NULL) (*plugin->onWorldLoad)(world);
	END_HASHMAP_ITERATION (plugins)
	return 0;
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
		beginProfilerSection("tick_world");
		if (tick_counter % 20 == 0) {
			world->tps = world->ticksInSecond;
			world->ticksInSecond = 0;
		}
		world->ticksInSecond++;
		world_pretick(world);
		beginProfilerSection("player_receive_packet");
		BEGIN_HASHMAP_ITERATION(world->players)
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
		END_HASHMAP_ITERATION(world->players)
		endProfilerSection("player_receive_packet");
		beginProfilerSection("tick_player");
		BEGIN_HASHMAP_ITERATION(world->players)
		struct player* player = (struct player*) value;
		tick_player(world, player);
		tick_entity(world, player->entity); // might need to be moved into separate loop later
		END_HASHMAP_ITERATION(world->players)
		endProfilerSection("tick_player");
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
		struct prqueue* pq = prqueue_new(0, 0);
		BEGIN_HASHMAP_ITERATION(world->scheduledTicks)
		struct scheduled_tick* st = value;
		if (--st->ticksLeft <= 0) { //
			prqueue_add(pq, st, st->priority);
			/*block b = getBlockWorld(world, st->x, st->y, st->z);
			 struct block_info* bi = getBlockInfo(b);
			 int k = 0;
			 if (bi->scheduledTick != NULL) k = (*bi->scheduledTick)(world, b, st->x, st->y, st->z);
			 if (k > 0) {
			 st->ticksLeft = k;
			 } else {
			 struct encpos ep;
			 ep.x = st->x;
			 ep.y = st->y;
			 ep.z = st->z;
			 put_hashmap(world->scheduledTicks, *((uint64_t*) &ep), NULL);
			 xfree(st);
			 }*/
		}
		END_HASHMAP_ITERATION(world->scheduledTicks)
		struct scheduled_tick* st = NULL;
		while ((st = prqueue_pop(pq)) != NULL) {
			//printf("%i: %i, %i, %i, #%f\n", tick_counter, st->x, st->y, st->z, st->priority);
			block b = getBlockWorld(world, st->x, st->y, st->z);
			if (st->src != b) {
				struct encpos ep;
				ep.x = st->x;
				ep.y = st->y;
				ep.z = st->z;
				put_hashmap(world->scheduledTicks, *((uint64_t*) &ep), NULL);
				xfree(st);
				continue;
			}
			struct block_info* bi = getBlockInfo(b);
			int k = 0;
			if (bi->scheduledTick != NULL) k = (*bi->scheduledTick)(world, b, st->x, st->y, st->z);
			if (k > 0) {
				st->ticksLeft = k;
				st->src = getBlockWorld(world, st->x, st->y, st->z);
			} else if (k < 0) {
				xfree(st);
			} else {
				struct encpos ep;
				ep.x = st->x;
				ep.y = st->y;
				ep.z = st->z;
				put_hashmap(world->scheduledTicks, *((uint64_t*) &ep), NULL);
				xfree(st);
			}
		}
		prqueue_del(pq);
		endProfilerSection("tick_chunk_scheduledticks");
		endProfilerSection("tick_chunks");
		BEGIN_HASHMAP_ITERATION (plugins)
		struct plugin* plugin = value;
		if (plugin->tick_world != NULL) (*plugin->tick_world)(world);
		END_HASHMAP_ITERATION (plugins)
		endProfilerSection("tick_world");
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
	if (entity->loadingPlayers == NULL) {
		entity->loadingPlayers = new_hashmap(1, 1);
	}
	if (entity->attackers == NULL) entity->attackers = new_hashmap(1, 0);
	put_hashmap(world->entities, entity->id, entity);
	struct entity_info* ei = getEntityInfo(entity->type);
	if (ei != NULL) {
		if (ei->initAI != NULL) {
			entity->ai = xcalloc(sizeof(struct aicontext));
			entity->ai->tasks = new_hashmap(1, 0);
			(*ei->initAI)(world, entity);
		}
		if (ei->onSpawned != NULL) (*ei->onSpawned)(world, entity);
	}
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
	se: ;
	spawnEntity(world, player->entity);
	BEGIN_HASHMAP_ITERATION (plugins)
	struct plugin* plugin = value;
	if (plugin->onPlayerSpawn != NULL) (*plugin->onPlayerSpawn)(world, player);
	END_HASHMAP_ITERATION (plugins)
}

void despawnPlayer(struct world* world, struct player* player) {
	if (player->openInv != NULL) player_closeWindow(player, player->openInv->windowID);
	despawnEntity(world, player->entity);
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
	BEGIN_HASHMAP_ITERATION(entity->attackers)
	struct entity* attacker = value;
	if (attacker->attacking == entity) attacker->attacking = NULL;
	END_HASHMAP_ITERATION(entity->attackers)
	del_hashmap(entity->attackers);
	entity->attackers = NULL;
	entity->attacking = NULL;
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
