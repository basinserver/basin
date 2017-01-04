#include <math.h>
#include "basin.h"
#include "profile.h"
#include "anticheat.h"

#define AC_BEGIN beginProfilerSection("player_anticheat");
#define AC_END(ret) { endProfilerSection("player_anticheat"); if (ret) printf("[AC] Player %s infraction at %s (%d)\n", player->name, __func__, ret); return ret; }

int ac_chat(struct player* player, char* msg) {
	AC_BEGIN

	for (char* p = msg; *p; p++) {
		if (*p < 32 || *p == 127 || *p == 0xA7) {
			kickPlayer(player, "Invalid chat message");
			AC_END(1);
		}
	}
	AC_END(0)
}

int ac_tick(struct player* player, int onground) {
	AC_BEGIN
	AC_END(0)
}

int ac_ticklook(struct player* player, float yaw, float pitch) {
	AC_BEGIN
	if (!isfinite(yaw) || !isfinite(pitch))
	AC_END(1)
	AC_END(0)
}

int ac_tickpos(struct player* player, double x, double y, double z) {
	AC_BEGIN
	if (!isfinite(x) || !isfinite(y) || !isfinite(z))
	AC_END(1)
	AC_END(0);
}
