#include <math.h>
#include "basin.h"
#include "profile.h"
#include "anticheat.h"

int ac_lock = 0;
#define AC_BEGIN
#define AC_END(ret) return ret;

int ac_tick(struct player* player, int onground) {
	AC_BEGIN
	AC_END(0)
}

int ac_ticklook(struct player* player, float yaw, float pitch) {
	AC_BEGIN
	AC_END(0)
}

int ac_tickpos(struct player* player, double x, double y, double z) {
	AC_BEGIN
	AC_END(0);
}