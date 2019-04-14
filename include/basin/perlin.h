#ifndef PERLIN_H_
#define PERLIN_H_

#include <stdint.h>

struct perlin {
		uint8_t perm[256];
		uint8_t perm2[512];
};

uint64_t perlin_rand(uint64_t seed);

double perlin_octave(struct perlin* perlin, double x, double y, double z, double amp, double freq, uint32_t octaves, double persistence);

double perlin_mod(struct perlin* perlin, double x, double y, double z, double amp, double freq);

double perlin(struct perlin* perlin, double x, double y, double z);

void perlin_init(struct perlin* perlin, uint64_t seed);

#endif /* PERLIN_H_ */
