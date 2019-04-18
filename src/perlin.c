#include <basin/perlin.h>
#include <stdint.h>
#include <math.h>

uint64_t perlin_rand(uint64_t seed) {
	if (seed == 0) {
		seed = 4567890;
	}
	seed ^= seed >> 12;
	seed ^= seed >> 25;
	seed ^= seed >> 27;
	return seed * (uint64_t) 0x2545F4914F6CDD1D;
}

double _perlin_lerp(double d1, double d2, double t) {
	return d1 + (d2 - d1) * t;
}

double _perlin_grad(int32_t hash, double x, double y, double z) {
	int32_t h = hash & 0x0f;
	double u = h < 8 ? x : y;
	double v = 0.;
	if (h < 4) v = y;
	else if (h == 12 || h == 14) v = x;
	else v = z;
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double _perlin_fade(double t) {
	return t * t * t * (t * (t * 6. - 15.) + 10.);
}

double perlin_octave(struct perlin* perlin, double x, double y, double z, double amp, double freq, uint32_t octaves, double persistence) {
	double total = 0.;
	double originalAmp = amp;
	for (uint32_t i = 0; i < octaves; i++) {
		total += perlin_mod(perlin, x, y, z, amp, freq);
		amp *= persistence;
		freq *= 2.;
	}
	return total;
}

double perlin_mod(struct perlin* perl, double x, double y, double z, double amp, double freq) {
	return perlin(perl, x * freq, y, z * freq) * amp;
}

double perlin(struct perlin* perlin, double x, double y, double z) { // returns -1->1
	int32_t xi = (int32_t) x & 0xFF;
	int32_t yi = (int32_t) y & 0xFF;
	int32_t zi = (int32_t) z & 0xFF;
	double xf = x - floor(x);
	double yf = y - floor(y);
	double zf = z - floor(z);
	double u = _perlin_fade(xf);
	double v = _perlin_fade(yf);
	double w = _perlin_fade(zf);
	int32_t a = (int32_t) perlin->perm2[xi] + yi;
	int32_t aa = (int32_t) perlin->perm2[a] + zi;
	int32_t ab = (int32_t) perlin->perm2[a + 1] + zi;
	int32_t b = (int32_t) perlin->perm2[xi + 1] + yi;
	int32_t ba = (int32_t) perlin->perm2[b] + zi;
	int32_t bb = (int32_t) perlin->perm2[b + 1] + zi;
	double x1 = _perlin_lerp(_perlin_grad((int32_t) perlin->perm2[aa], xf, yf, zf), _perlin_grad((int32_t) perlin->perm2[ba], xf - 1, yf, zf), u);
	double x2 = _perlin_lerp(_perlin_grad((int32_t) perlin->perm2[ab], xf, yf - 1, zf), _perlin_grad((int32_t) perlin->perm2[bb], xf - 1, yf - 1, zf), u);
	double y1 = _perlin_lerp(x1, x2, v);
	x1 = _perlin_lerp(_perlin_grad((int32_t) perlin->perm2[aa + 1], xf, yf, zf - 1), _perlin_grad((int32_t) perlin->perm2[ba + 1], xf - 1, yf, zf - 1), u);
	x2 = _perlin_lerp(_perlin_grad((int32_t) perlin->perm2[ab + 1], xf, yf - 1, zf - 1), _perlin_grad((int32_t) perlin->perm2[bb + 1], xf - 1, yf - 1, zf - 1), u);
	double y2 = _perlin_lerp(x1, x2, v);
	return _perlin_lerp(y1, y2, w);
}

void perlin_init(struct perlin* perlin, uint64_t seed) {
	uint16_t nums[256];
	for (uint16_t x = 0; x < 256; x++) {
		nums[x] = x;
	}
	for (uint16_t x = 0; x < 256; x++) {
		seed = perlin_rand(seed);
		uint16_t n = 0;
		while ((n = nums[seed % 256]) >= 256) {
			seed = perlin_rand(seed);
		}
		perlin->perm[x] = (uint8_t) n;
		nums[seed % 256] = 256;
	}
	for (uint16_t x = 0; x < 512; x++) {
		perlin->perm2[x] = perlin->perm[x % 256];
	}
}
