#include <math.h>
#include "basin.h"
#include "profile.h"
#include "anticheat.h"

int ac_lock = 0;
#define AC_BEGIN if (!ac_lock++) beginProfilerSection("player_anticheat");
#define AC_END(ret) if (!--ac_lock) endProfilerSection("player_anticheat"); return ret;

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
	if (player->spawnedIn && !player->entity->inWater && !player->entity->inLava && (player->llTick + 5 < tick_counter) && player->gamemode != 1 && player->entity->health > 0.) {
		int nrog = player_onGround(player);
		float dy = player->entity->ly - player->entity->y;
		if (dy >= 0.) {
			if (!player->acstate.real_onGround) player->acstate.offGroundTime++;
			else player->acstate.offGroundTime = 0;
		} else player->acstate.offGroundTime = 0.;
		float edy = .0668715 * (float) (player->acstate.offGroundTime - .5) + .0357839;
		//printf("dy = %f ogt = %i edy = %f, %f, %f\n", dy, player->acstatez.offGroundTime, pedy, edy, nedy);
		if (dy >= 0.) {
			float edd = fabs(edy - dy);
			if (!nrog && !player->acstate.real_onGround) {
				if (edd > .1) {
					player->acstate.flightInfraction += 2;
					//printf("slowfall\n");
				}
			}
		}
		if (dy < player->acstate.ldy && !player->acstate.real_onGround) { // todo: cobwebs
			if (fabs(dy + .42) < .001 && player->acstate.real_onGround) {
				player->acstate.lastJump = tick_counter;
				//printf("jump\n");
			} else {
				if (!nrog) {
					//printf("inf\n");
					player->acstate.flightInfraction += 2;
				} // else printf("nrog\n");

			}
		}
		player->acstate.real_onGround = nrog;
		player->acstate.ldy = dy;
		if (player->acstate.real_onGround != player->entity->onGround) {
			player->acstate.flightInfraction += 2;
			//printf("nog %i %i\n", player->real_onGround, player->entity->onGround);
		}
		if (player->acstate.flightInfraction > 0) if (player->acstate.flightInfraction-- > 25) {
			kickPlayer(player, "Flying is not enabled on this server");
				AC_END(1);
		}
	}
	AC_END(0);
}