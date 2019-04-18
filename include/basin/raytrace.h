//
// Created by p on 4/17/19.
//

#ifndef BASIN_RAYTRACE_H
#define BASIN_RAYTRACE_H

#include <basin/boundingbox.h>
#include <basin/world.h>
#include <stdint.h>

int raytrace_block(struct boundingbox* bb, int32_t x, int32_t y, int32_t z, double px, double py, double pz, double ex, double ey, double ez, double* qx, double* qy, double* qz);

int raytrace(struct world* world, double x, double y, double z, double ex, double ey, double ez, int stopOnLiquid, int ignoreNonCollidable, int returnLast, double* rx, double* ry, double* rz);

#endif //BASIN_RAYTRACE_H
