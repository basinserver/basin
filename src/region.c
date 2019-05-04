#include <basin/globals.h>
#include <basin/region.h>
#include <basin/chunk.h>
#include <basin/tileentity.h>
#include <basin/nbt.h>
#include <avuna/pmem.h>
#include <avuna/string.h>
#include <avuna/log.h>
#include <avuna/llist.h>
#include <avuna/pmem_hooks.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <zlib.h>
#include <math.h>

void _region_unmap(struct region* region) {
    if (region->mapping != NULL) {
        munmap(region->mapping, 67108864);
    }
    if (region->fd >= 0) {
        close(region->fd);
    }
}


struct region* region_new(struct mempool* parent, char* path, int16_t x, int16_t z) {
    struct mempool* pool = mempool_new();
    pchild(parent, pool);
    struct region* region = pcalloc(pool, sizeof(struct region));
    region->pool = pool;
    region->x = x;
    region->z = z;
    region->file = str_dup(path, 0, pool);
    region->fd = -1;
    return region;
}

struct chunk* region_load_chunk(struct region* region, int8_t local_chunk_x, int8_t local_chunk_z) {
    if (region->fd < 0) {
        region->fd = open(region->file, O_RDWR | O_CREAT, 0664);
        if (region->fd < 0) {
            errlog(delog, "Error opening region: %s", region->file);
            return NULL;
        }
        phook(region->pool, (void (*)(void*)) _region_unmap, region);
        region->mapping = mmap(NULL, 67108864, PROT_READ | PROT_WRITE, MAP_SHARED, region->fd, 0); // 64 MB is the theoretical limit of an uncompressed region file
        if (region->mapping == NULL || region->mapping == (void*) -1) {
            errlog(delog, "Error mapping region: %s", region->file);
            return NULL;
        }
    }
    uint32_t* chunk_offset_ptr = region->mapping + 4 * ((local_chunk_x & 31) + (local_chunk_z & 31) * 32);
    uint32_t chunk_offset = *chunk_offset_ptr;
    swapEndian(&chunk_offset, 4);
    uint32_t offset = ((chunk_offset & 0xFFFFFF00) >> 8) * 4096;
    uint32_t size = ((chunk_offset & 0x000000FF)) * 4096;
    if (offset == 0 || size == 0) return NULL;
    void* chunk_ptr = region->mapping + offset;
    uint32_t chunk_size = ((uint32_t*) chunk_ptr)[0];
    swapEndian(&chunk_size, 4);
    uint8_t compression_type = ((uint8_t*) chunk_ptr)[4];
    chunk_ptr += 5;
    struct mempool* chunk_pool = mempool_new();
    pchild(region->pool, chunk_pool);
    void* decompressed_chunk = pmalloc(chunk_pool, 65536);
    size_t decompressed_chunk_capacity = 65536;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    int inflate_result = 0;
    if ((inflate_result = inflateInit2(&strm, (32 + MAX_WBITS))) != Z_OK) { //
        errlog(delog, "Compression initialization error for region: %s", region->file);
        pfree(chunk_pool);
        return NULL;
    }
    strm.avail_in = chunk_size;
    strm.next_in = chunk_ptr;
    strm.avail_out = (uInt) decompressed_chunk_capacity;
    strm.next_out = decompressed_chunk;
    do {
        if (decompressed_chunk_capacity >= strm.total_out) {
            decompressed_chunk_capacity *= 2;
            decompressed_chunk = prealloc(chunk_pool, decompressed_chunk_capacity);
        }
        strm.avail_out = (uInt) (decompressed_chunk_capacity - strm.total_out);
        strm.next_out = decompressed_chunk + strm.total_out;
        inflate_result = inflate(&strm, Z_FINISH);
        if (inflate_result == Z_STREAM_ERROR) {
            errlog(delog, "Compression Read Error for region: %s", region->file);
            inflateEnd(&strm);
            pfree(chunk_pool);
            return NULL;
        }
    } while (strm.avail_out == 0);
    inflateEnd(&strm);
    size_t rts = strm.total_out;
    struct nbt_tag* nbt = NULL;
    if (nbt_read(chunk_pool, &nbt, decompressed_chunk, rts) < 0 || nbt == NULL) {
        pfree(chunk_pool);
        return NULL;
    }
    pprefree(chunk_pool, decompressed_chunk);

    struct nbt_tag* level = nbt_get(nbt, "Level");
    if (level == NULL || level->id != NBT_TAG_COMPOUND) goto region_error;
    struct nbt_tag* tmp = nbt_get(level, "xPos");
    if (tmp == NULL || tmp->id != NBT_TAG_INT) goto region_error;
    int32_t xPos = tmp->data.nbt_int;
    tmp = nbt_get(level, "zPos");
    if (tmp == NULL || tmp->id != NBT_TAG_INT) goto region_error;
    int32_t zPos = tmp->data.nbt_int;
    struct chunk* chunk = chunk_new(chunk_pool, (int16_t) xPos, (int16_t) zPos);
    tmp = nbt_get(level, "LightPopulated");
    if (tmp == NULL || tmp->id != NBT_TAG_BYTE) goto region_error;
    chunk->lightpopulated = tmp->data.nbt_byte;
    tmp = nbt_get(level, "TerrainPopulated");
    if (tmp == NULL || tmp->id != NBT_TAG_BYTE) goto region_error;
    chunk->terrainpopulated = tmp->data.nbt_byte;
    tmp = nbt_get(level, "InhabitedTime");
    if (tmp == NULL || tmp->id != NBT_TAG_LONG) goto region_error;
    chunk->inhabitedticks = (uint64_t) tmp->data.nbt_long;
    tmp = nbt_get(level, "Biomes");
    if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY) goto region_error;
    if (tmp->data.nbt_bytearray.len == 256) memcpy(chunk->biomes, tmp->data.nbt_bytearray.data, 256);
    tmp = nbt_get(level, "HeightMap");
    if (tmp == NULL || tmp->id != NBT_TAG_INTARRAY) goto region_error;
    if (tmp->data.nbt_intarray.count == 256) for (int i = 0; i < 256; i++)
            chunk->heightMap[i >> 4][i & 0x0F] = (uint16_t) tmp->data.nbt_intarray.ints[i];
    tmp = nbt_get(level, "TileEntities");
    if (tmp == NULL || tmp->id != NBT_TAG_LIST) goto region_error;
    ITER_LLIST(tmp->children_list, value) {
        struct nbt_tag* child = value;
        if (child == NULL || child->id != NBT_TAG_COMPOUND) continue;
        struct tile_entity* tile = tile_parse(chunk->pool, child);
        uint64_t key = (uint64_t) (tile->x & 0x0f) << 16 | (uint64_t) tile->y << 8 | (uint64_t) (tile->z & 0x0f);
        hashmap_putint(chunk->tileEntities, key, tile);
        ITER_LLIST_END();
    }
    //TODO: tileticks
    struct nbt_tag* sections = nbt_get(level, "Sections");
    if (sections == NULL || sections->id != NBT_TAG_LIST) goto region_error;
    ITER_LLIST(sections->children_list, value) {
        struct nbt_tag* nbt_section = value;
        tmp = nbt_get(nbt_section, "Y");
        if (tmp == NULL || tmp->id != NBT_TAG_BYTE) continue;
        uint8_t y = (uint8_t) tmp->data.nbt_byte;
        tmp = nbt_get(nbt_section, "Blocks");
        if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 4096) continue;
        int hna = 0;
        block* blocks = pmalloc(chunk->pool, sizeof(block) * 4096);
        for (int i = 0; i < 4096; i++) {
            blocks[i] = ((block) tmp->data.nbt_bytearray.data[i]) << 4; // [i >> 8][(i & 0xf0) >> 4][i & 0x0f]
            if (((block) tmp->data.nbt_bytearray.data[i]) != 0) hna = 1;
        }
        //if (hna) chunk->empty[y] = 0;
        tmp = nbt_get(nbt_section, "Add");
        if (tmp != NULL) {
            if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
            for (int i = 0; i < 4096; i++) {
                block block = tmp->data.nbt_bytearray.data[i / 2];
                if (i % 2 == 0) {
                    block &= 0xf0;
                    block >>= 4;
                } else block &= 0x0f;
                block <<= 8;
                blocks[i] |= block;
                if (blocks[i] != 0) hna = 1;
            }
        }
        if (hna) {
            tmp = nbt_get(nbt_section, "Data");
            if (tmp == NULL || tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
            for (int i = 0; i < 4096; i++) {
                block block = tmp->data.nbt_bytearray.data[i / 2];
                if (i % 2 == 1) {
                    block &= 0xf0;
                    block >>= 4;
                } else block &= 0x0f;
                blocks[i] |= block;
            }
            struct chunk_section* section = pcalloc(chunk->pool, sizeof(struct chunk_section));
            section->palette = pcalloc(chunk->pool, 256 * sizeof(block));
            section->palette_count = 0;
            block inverse_palette[getBlockSize()];
            block last = 0;
            for (int i = 0; i < 4096; i++) {
                if (blocks[i] != last) {
                    for (size_t x = section->palette_count - 1; x >= 0; x--) {
                        if (section->palette[x] == blocks[i]) goto cx;
                    }
                    inverse_palette[blocks[i]] = (block) section->palette_count;
                    section->palette[section->palette_count++] = blocks[i]; // TODO: if only a few blocks of a certain type and on a palette boundary, use a MBC to reduce size?
                    if (section->palette_count >= 256) {
                        pprefree(chunk->pool, section->palette);
                        section->palette = NULL;
                        section->bits_per_block = 13;
                        section->palette_count = 0;
                        break;
                    }
                    last = blocks[i];
                }
                cx: ;
            }
            if (section->palette != NULL) {
                section->bits_per_block = (uint8_t) ceil(log2(section->palette_count));
                if (section->bits_per_block < 4) section->bits_per_block = 4;
                section->palette = prealloc(chunk->pool, section->palette, section->palette_count * sizeof(block));
            }
            int32_t bi = 0;
            section->blocks = pmalloc(chunk->pool, (size_t) (512 * section->bits_per_block + 4));
            section->block_size = (size_t) (512 * section->bits_per_block);
            section->block_mask = 0;
            for (int i = 0; i < section->bits_per_block; i++)
                section->block_mask |= 1 << i;
            for (int i = 0; i < 4096; i++) { // [i >> 8][(i & 0xf0) >> 4][i & 0x0f]
                int32_t block_bytes = (int32_t)((section->bits_per_block == 13 ? blocks[i] : inverse_palette[blocks[i]]) & section->block_mask);
                int32_t block_byte_offset = *((int32_t*) (&section->blocks[bi / 8]));
                int32_t bit_index = bi % 8;
                block_byte_offset = (block_byte_offset & ~(section->block_mask << bit_index)) | (block_bytes << bit_index);
                *((int32_t*) &section->blocks[bi / 8]) = block_byte_offset;
                bi += section->bits_per_block;
            }
            tmp = nbt_get(nbt_section, "BlockLight");
            if (tmp != NULL) {
                if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
                memcpy(section->blockLight, tmp->data.nbt_bytearray.data, 2048);
            }
            tmp = nbt_get(nbt_section, "SkyLight");
            if (tmp != NULL) {
                if (tmp->id != NBT_TAG_BYTEARRAY || tmp->data.nbt_bytearray.len != 2048) continue;
                section->skyLight = pmalloc(chunk->pool, 2048);
                memcpy(section->skyLight, tmp->data.nbt_bytearray.data, 2048);
            }
            chunk->sections[y] = section;
        }
        pprefree(chunk->pool, blocks);
        ITER_LLIST_END();
    }

    for (int z = 0; z < 16; z++) {
        for (int x = 0; x < 16; x++) {
            if (chunk->heightMap[z][x] <= 0) {
                for (int y = 255; y >= 0; y--) {
                    block b = chunk_get_block(chunk, x, y, z);
                    struct block_info* bi = getBlockInfo(b);
                    if (bi == NULL || bi->lightOpacity <= 0) continue;
                    chunk->heightMap[z][x] = (uint16_t) (y + 1);
                    break;
                }
            }
        }
    }
//TODO: entities and tileticks.
    if (nbt != NULL) {
        pfree(nbt->pool);
    }
    return chunk;
    region_error: ;
    pfree(nbt->pool);
    return NULL;
}