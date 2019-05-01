/*
 * worldmanager.c
 *
 *  Created on: Dec 23, 2016
 *      Author: root
 */

#include <basin/world.h>
#include <basin/worldmanager.h>

struct world* getWorldByID(int32_t id) {
    for (size_t i = 0; i < worlds->size; i++) {
        if (worlds->data[i] != NULL && ((struct world*) worlds->data[i])->dimension == id) {
            return ((struct world*) worlds->data[i]);
        }
    }
    return NULL;
}
