//
// Created by p on 4/17/19.
//

#include <basin/chunk.h>


uint64_t chunk_get_key(struct chunk* ch) {
    return (uint64_t)(((int64_t) ch->x) << 32) | (((int64_t) ch->z) & 0xFFFFFFFF);
}

uint64_t chunk_get_key_direct(int32_t cx, int32_t cz) {
    return (uint64_t)(((int64_t) cx) << 32) | (((int64_t) cz) & 0xFFFFFFFF);
}