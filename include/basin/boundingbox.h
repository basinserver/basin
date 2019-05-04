#ifndef BASIN_BOUNDINGBOX_H
#define BASIN_BOUNDINGBOX_H

struct boundingbox {
    double minX;
    double maxX;
    double minY;
    double maxY;
    double minZ;
    double maxZ;
};

int boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2);

#endif //BASIN_BOUNDINGBOX_H
