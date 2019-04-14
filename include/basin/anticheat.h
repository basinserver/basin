#ifndef BASIN_ANTICHEAT_H_
#define BASIN_ANTICHEAT_H_

#include <basin/player.h>

int ac_chat(struct player* player, char* msg);
int ac_tick(struct player* player, int onground);
int ac_ticklook(struct player* player, float yaw, float pitch);
int ac_tickpos(struct player* player, double x, double y, double z);

#endif //BASIN_ANTICHEAT_H_
