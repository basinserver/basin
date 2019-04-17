//
// Created by p on 4/17/19.
//

#include <basin/boundingbox.h>


int boundingbox_intersects(struct boundingbox* bb1, struct boundingbox* bb2) {
    return (((bb1->minX >= bb2->minX && bb1->minX <= bb2->maxX) || (bb1->maxX >= bb2->minX && bb1->maxX <= bb2->maxX) || (bb2->minX >= bb1->minX && bb2->minX <= bb1->maxX) || (bb2->maxX >= bb1->minX && bb2->maxX <= bb1->maxX)) && ((bb1->minY >= bb2->minY && bb1->minY <= bb2->maxY) || (bb1->maxY >= bb2->minY && bb1->maxY <= bb2->maxY) || (bb2->minY >= bb1->minY && bb2->minY <= bb1->maxY) || (bb2->maxY >= bb1->minY && bb2->maxY <= bb1->maxY)) && ((bb1->minZ >= bb2->minZ && bb1->minZ <= bb2->maxZ) || (bb1->maxZ >= bb2->minZ && bb1->maxZ <= bb2->maxZ) || (bb2->minZ >= bb1->minZ && bb2->minZ <= bb1->maxZ) || (bb2->maxZ >= bb1->minZ && bb2->maxZ <= bb1->maxZ)));
}
