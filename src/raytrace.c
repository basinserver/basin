//
// Created by p on 4/17/19.
//

#include <basin/raytrace.h>
#include <basin/world.h>
#include <avuna/string.h>
#include <math.h>

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

int raytrace_block(struct boundingbox* bb, int32_t x, int32_t y, int32_t z, double px, double py, double pz, double ex, double ey, double ez, double* qx, double* qy, double* qz) {
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

int raytrace(struct world* world, double x, double y, double z, double ex, double ey, double ez, int stopOnLiquid, int ignoreNonCollidable, int returnLast, double* rx, double* ry, double* rz) {
    int32_t ix = (uint32_t) floor(x);
    int32_t iy = (uint32_t) floor(y);
    int32_t iz = (uint32_t) floor(z);
    int32_t iex = (uint32_t) floor(ex);
    int32_t iey = (uint32_t) floor(ey);
    int32_t iez = (uint32_t) floor(ez);
    block b = world_get_block(world, ix, iy, iz);
    struct block_info* bbi = getBlockInfo(b);
    if (bbi != NULL && (!ignoreNonCollidable || bbi->boundingBox_count > 0)) {
        for (size_t i = 0; i < bbi->boundingBox_count; i++) {
            struct boundingbox* bb = &bbi->boundingBoxes[i];
            int face = raytrace_block(bb, ix, iy, iz, x, y, z, ex, ey, ez, rx, ry, rz);
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
        ix = (int32_t) (floor(x) - (cface == XP ? 1 : 0));
        iy = (int32_t) (floor(y) - (cface == YP ? 1 : 0));
        iz = (int32_t) (floor(z) - (cface == ZP ? 1 : 0));
        block nb = world_get_block(world, ix, iy, iz);
        struct block_info* bi = getBlockInfo(nb);
        if (bi != NULL && (!ignoreNonCollidable || str_eq(bi->material->name, "portal") || bi->boundingBox_count > 0)) {
            //todo: cancollidecheck?
            for (size_t i = 0; i < bi->boundingBox_count; i++) {
                struct boundingbox* bb = &bi->boundingBoxes[i];
                int face = raytrace_block(bb, ix, iy, iz, x, y, z, ex, ey, ez, rx, ry, rz);
                if (face >= 0) return face;
            }
            //TODO: returnlast finish
        }
    }
    return returnLast ? cface : -1;
}
