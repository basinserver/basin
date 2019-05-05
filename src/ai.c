
#include <basin/ai.h>
#include <basin/entity.h>
#include <basin/world.h>
#include <basin/game.h>
#include <avuna/hash.h>
#include <math.h>

void initai_zombie(struct world* world, struct entity* entity) {
    ai_initTask(entity, 4, &ai_swimming, &ai_shouldswimming, NULL);
    struct ai_nearestattackabletarget_data
    *and = xcalloc(sizeof(struct ai_nearestattackabletarget_data));
    and->chance = 10;
    and->type = ENT_PLAYER;
    and->checkSight = 1;
    and->unseenTicks = 0;
    and->targetSearchDelay = 0;
    and->targetDist = 16.;
    ai_initTask(entity, 1, &ai_nearestattackabletarget, &ai_shouldnearestattackabletarget, and);
    and = xcalloc(sizeof(struct ai_nearestattackabletarget_data));
    and->chance = 10;
    and->type = ENT_VILLAGER;
    and->checkSight = 0;
    and->unseenTicks = 0;
    and->targetSearchDelay = 0;
    and->targetDist = 16.;
    ai_initTask(entity, 1, &ai_nearestattackabletarget, &ai_shouldnearestattackabletarget, and);
    and = xcalloc(sizeof(struct ai_nearestattackabletarget_data));
    and->chance = 10;
    and->type = ENT_VILLAGERGOLEM;
    and->checkSight = 1;
    and->unseenTicks = 0;
    and->targetSearchDelay = 0;
    and->targetDist = 16.;
    ai_initTask(entity, 1, &ai_nearestattackabletarget, &ai_shouldnearestattackabletarget, and);
    struct ai_attackmelee_data* amd = xcalloc(sizeof(struct ai_attackmelee_data));
    amd->attackInterval = 20;
    amd->speed = 1.;
    amd->longMemory = 0;
    ai_initTask(entity, 4, &ai_zombieattack, &ai_shouldzombieattack, amd);
}

void ai_stdcancel(struct world* world, struct entity* entity, struct aitask* ai) {
    if (!ai->ai_running) return;
    entity->ai->mutex &= ~ai->mutex;
    ai->ai_running = 0;
}

struct aitask* ai_initTask(struct entity* entity, uint16_t mutex, int32_t (*ai_tick)(struct world* world, struct entity* entity, struct aitask* ai), int (*ai_should)(struct world* world, struct entity* entity, struct aitask* ai), void* data) {
    struct aitask* ai = xmalloc(sizeof(struct aitask));
    ai->ai_running = 0;
    ai->ai_waiting = 0;
    ai->mutex = mutex;
    ai->ai_should = ai_should;
    ai->ai_tick = ai_tick;
    ai->ai_cancel = &ai_stdcancel;
    ai->data = data;
    put_hashmap(entity->ai->tasks, (uint64_t) ai, ai);
    return ai;
}

int32_t ai_handletasks(struct world* world, struct entity* entity) {
    if (entity->ai == NULL) return 0;
    int32_t mw = 0;
    BEGIN_HASHMAP_ITERATION(entity->ai->tasks);
    struct aitask* ai = value;
    if (ai->ai_running) {
        if (ai->ai_waiting == -1) (*ai->ai_cancel)(world, entity, ai);
        else if (ai->ai_waiting > 0) ai->ai_waiting--;
        else if (ai->ai_tick != NULL) {
            if (!(*ai->ai_should)(world, entity, ai)) (*ai->ai_cancel)(world, entity, ai);
            else ai->ai_waiting = (*ai->ai_tick)(world, entity, ai);
        }
    } else if (ai->ai_should != NULL && !(ai->mutex & entity->ai->mutex) && (*ai->ai_should)(world, entity, ai)) {
        ai->ai_waiting = 0;
        entity->ai->mutex |= ai->mutex;
        ai->ai_waiting = (*ai->ai_tick)(world, entity, ai);
        ai->ai_running = 1;
    }
    if (ai->ai_waiting > mw) mw = ai->ai_waiting;
    END_HASHMAP_ITERATION(entity->ai->tasks);
    return mw;
}

float wrapAngle(float angle) {
    angle = fmod(angle, 360.);
    if (angle >= 180.) angle -= 360.;
    if (angle < -180.) angle += 360.;
    return angle;
}

void lookHelper_tick(struct entity* entity) {
    if (entity->ai == NULL || entity->ai->lookHelper_speedPitch == 0. || entity->ai->lookHelper_speedYaw == 0.) return;
    double dx = entity->ai->lookHelper_x - entity->x;
    struct boundingbox bb;
    entity_collision_bounding_box(entity, &bb);
    double dy = entity->ai->lookHelper_y - entity->y - ((bb.maxY - bb.minY) * .9); // TODO: real eye height
    double dz = entity->ai->lookHelper_z - entity->z;
    double horiz_dist = sqrt(dx * dx + dz * dz);
    float dp = -(atan2(dy, horiz_dist) * 180. / M_PI);
    float dya = (atan2(dz, dx) * 180. / M_PI) - 90.;
    dp = wrapAngle(dp - entity->pitch);
    if (dp > entity->ai->lookHelper_speedPitch) dp = entity->ai->lookHelper_speedPitch;
    if (dp < -entity->ai->lookHelper_speedPitch) dp = -entity->ai->lookHelper_speedPitch;
    entity->pitch += dp;
    dya = wrapAngle(dya - entity->yaw);
    if (dya > entity->ai->lookHelper_speedYaw) dya = entity->ai->lookHelper_speedYaw;
    if (dya < -entity->ai->lookHelper_speedYaw) dya = -entity->ai->lookHelper_speedYaw;
    entity->yaw += dya;
    if (fabs(dp) < 0.5 && fabs(dya) < .5) {
        entity->ai->lookHelper_speedPitch = 0.;
        entity->ai->lookHelper_speedYaw = 0.;
    }
}

float aipath_manhattan(struct aipath_point* p1, struct aipath_point* p2) {
    return fabs(p1->x - p2->x) + fabs(p1->y - p2->y) + fabs(p1->y - p2->y);
}

void aipath_sortback(struct aipath* path, int32_t index) {
    struct aipath_point* pp = path->points[index];
    int i = 0;
    for (float ld = pp->target_dist; index > 0; index = i) {
        i = (index - 1) >> 1;
        struct aipath_point* pp2 = path->points[i];
        if (ld >= pp2->target_dist) break;
        path->points[index] = pp2;
        pp2->index = index;
    }
    path->points[index] = pp;
    pp->index = index;
}

void aipath_sortforward(struct aipath* path, int32_t index) {
    struct aipath_point* pp = path->points[index];
    float ld = pp->target_dist;
    while (1) {
        int i = 1 + (index << 1);
        int j = i + 1;
        if (i >= path->points_count) break;
        struct aipath_point* pp2 = path->points[i];
        float ld2 = pp2->target_dist;
        struct aipath_point* pp3 = NULL;
        float ld3 = 0.;
        if (j >= path->points_count) ld3 = (float) 0x7f800000; // positive infinity
        else {
            pp3 = path->points[j];
            ld3 = pp3->target_dist;
        }
        if (ld2 < ld3) {
            if (ld2 >= ld) break;
            path->points[index] = pp2;
            pp2->index = index;
            index = i;
        } else {
            if (ld3 >= ld) break;
            path->points[index] = pp3;
            pp3->index = index;
            index = j;
        }
    }
    path->points[index] = pp;
    pp->index = index;
}

struct aipath_point* aipath_addpoint(struct aipath* path, struct aipath_point* point) {
    if (path->points_count == path->points_size) {
        path->points_size *= 2;
        path->points = xrealloc(path->points, path->points_size * sizeof(struct aipath_point));
        memset(path->points + (path->points_size / 2), 0, (path->points_size / 2) * sizeof(struct aipath_point));
    }
    struct aipath_point* np = xmalloc(sizeof(struct aipath_point));
    memcpy(np, point, sizeof(struct aipath_point));
    path->points[path->points_count] = np;
    np->index = path->points_count;
    aipath_sortback(path, np->index);
    path->points_count++;
    return np;
}

struct aipath_point* aipath_dequeue(struct aipath* path) {
    if (path->points_count == 0) return NULL;
    struct aipath_point* pp = path->points[0];
    path->points[0] = path->points[--path->points_count];
    path->points[path->points_count] = NULL;
    if (path->points_count > 0) aipath_sortforward(path, 0);
    pp->index = -1;
    return pp;
}

struct aipath* findPath(int32_t sx, int32_t sy, int32_t sz, int32_t ex, int32_t ey, int32_t ez) {
    struct aipath_point end;
    memset(&end, 0, sizeof(struct aipath_point));
    end.x = ex;
    end.y = ey;
    end.z = ez;
    struct aipath_point start;
    memset(&start, 0, sizeof(struct aipath_point));
    start.x = sx;
    start.y = sy;
    start.z = sz;
    struct aipath* path = xmalloc(sizeof(struct aipath));
    path->points_count = 0;
    path->points_size = 128;
    path->points = xcalloc(sizeof(struct aipath_point) * path->points_size);
    start.x = sx;
    start.y = sy;
    start.z = sz;
    start.next_dist = aipath_manhattan(path->points[0], &end);
    start.target_dist = start.next_dist;
    struct aipath_point* cp = aipath_addpoint(path, &start);
    int i = 0;
    while (path->points_count > 0) {
        if (++i >= 200) break;
        struct aipath_point* pp2 = aipath_dequeue(path);
        if (pp2->x == end.x && pp2->y == end.y && pp2->z == end.z) {
            cp = pp2;
            break;
        }
        if (aipath_manhattan(pp2, &end) < aipath_manhattan(cp, &end)) cp = pp2;
        pp2->visited = 1;

    }
    return path;
}

void freePath(struct aipath* path) {

}

int32_t ai_attackmelee(struct world* world, struct entity* entity, struct aitask* ai) {
    struct ai_attackmelee_data* amd = ai->data;
    entity->ai->lookHelper_speedYaw = 30.;
    entity->ai->lookHelper_speedPitch = 30.;
    entity->ai->lookHelper_x = entity->attacking->x;
    struct boundingbox bb;
    entity_collision_bounding_box(entity->attacking, &bb);
    entity->ai->lookHelper_y = entity->attacking->y + ((bb.maxY - bb.minY) * .9); // TODO: real eye height
    entity->ai->lookHelper_z = entity->attacking->z;
    double dist = entity_distsq(entity->attacking, entity);
    amd->delayCounter--;
//TODO: cansee
    if ((amd->longMemory || 1) && amd->delayCounter <= 0 && ((amd->targetX == 0. && amd->targetY == 0. && amd->targetZ == 0.) || entity_distsq_block(entity->attacking, amd->targetX, amd->targetY, amd->targetZ) >= 1. || game_rand_float() < .05)) {
        amd->targetX = entity->attacking->x;
        amd->targetY = entity->attacking->y;
        amd->targetZ = entity->attacking->z;
        if (dist > 1024.) amd->delayCounter += 10;
        else if (dist > 256.) amd->delayCounter += 5.;
        //TODO: set path
    }
    if (--amd->attackTick <= 0) amd->attackTick = 0;
    struct entity_info* ei = entity_get_info(entity->type);
    struct entity_info* ei2 = entity_get_info(entity->attacking->type);
    float reach = ei->width * 2. * ei->width * 2. + ei2->width;
    if (dist <= reach && amd->attackTick <= 0) {
        amd->attackTick = 20;
        entity_animation(entity);
        damageEntityWithItem(entity->attacking, entity, -1, NULL);
    }
    return 0;
}

int32_t ai_attackranged(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_attackrangedbow(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_avoidentity(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_beg(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_breakdoor(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_creeperswell(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_defendvillage(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_doorinteract(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_eatgrass(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_findentitynearest(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_findentitynearestplayer(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_fleesun(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_followgolem(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_followowner(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_followparent(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_harvestfarmland(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_hurtbytarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_leapattarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_llamafollowcaravan(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_lookattradeplayer(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_lookatvillager(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_lookidle(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_mate(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_moveindoors(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_movethroughvillage(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_movetoblock(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_movetowardsrestriction(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_movetowardstarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_nearestattackabletarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_ocelotattack(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_ocelotsit(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_opendoor(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_ownerhurtbytarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_ownerhurttarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_panic(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_play(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_restructopendoor(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_restrictsun(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_runaroundlikecrazy(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_sit(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_skeletonriders(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_swimming(struct world* world, struct entity* entity, struct aitask* ai) {
//if (game_rand_float() < .8) {
    entity_jump(entity);
//}
    return 0;
}

int32_t ai_target(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_targetnontamed(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_tempt(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_tradeplayer(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_villagerinteract(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_villagermate(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_wander(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_wanderaint32_twater(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_watchclosest(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int32_t ai_zombieattack(struct world* world, struct entity* entity, struct aitask* ai) {
    return ai_attackmelee(world, entity, ai);
}

//shoulds

int ai_shouldattackmelee(struct world* world, struct entity* entity, struct aitask* ai) {
    if (entity->attacking == NULL || entity->attacking->health <= 0. || (entity->type == ENT_PLAYER && (entity->data.player.player->gamemode == 1 || entity->data.player.player->gamemode == 3))) return 0;
    struct entity_info* ei = entity_get_info(entity->type);
    struct entity_info* ei2 = entity_get_info(entity->attacking->type);
    if (ei == NULL || ei2 == NULL || !entity_has_flag(ei2, "livingbase")) return 0;
    return entity->attacking != NULL;
    //float reach = ei->width * 2. * ei->width * 2. + ei2->width;
    //return reach >= entity_distsq(entity->attacking, entity);
}

int ai_shouldattackranged(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldattackrangedbow(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldavoidentity(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldbeg(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldbreakdoor(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldcreeperswell(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shoulddefendvillage(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shoulddoorinteract(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldeatgrass(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldfindentitynearest(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldfindentitynearestplayer(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldfleesun(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldfollowgolem(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldfollowowner(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldfollowparent(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldharvestfarmland(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldhurtbytarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldleapattarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldllamafollowcaravan(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldlookattradeplayer(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldlookatvillager(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldlookidle(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldmate(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldmoveindoors(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldmovethroughvillage(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldmovetoblock(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldmovetowardsrestriction(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldmovetowardstarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldnearestattackabletarget(struct world* world, struct entity* entity, struct aitask* ai) {
    struct ai_nearestattackabletarget_data* data = ai->data;
    if (data->chance > 0 && rand() % data->chance != 0) return 0;
    double cd = data->targetDist * data->targetDist;
    struct entity* ce = NULL;
    BEGIN_HASHMAP_ITERATION(world->entities)
    struct entity* ie = value;
    if (!entity_has_flag(entity_get_info(ie->type), "livingbase") || ie == entity) continue;
    double dsq = entity_distsq(entity, value);
    if (ie->type == ENT_PLAYER) {
        struct player* pl = ie->data.player.player;
        if (pl->gamemode == 1 || pl->gamemode == 3 || pl->invulnerable) continue;
        int sk = entity_has_flag(entity_get_info(entity->type), "skeleton");
        int zo = entity_has_flag(entity_get_info(entity->type), "zombie");
        int cr = entity_has_flag(entity_get_info(entity->type), "creeper");
        if (sk || zo || cr) {
            struct slot* hs = inventory_get(pl, pl->inventory, 5);
            if (hs != NULL) {
                if (sk && hs->damage == 0) dsq *= 2.;
                else if (zo && hs->damage == 2) dsq *= 2.;
                else if (cr && hs->damage == 4) dsq *= 2.;
            }
        }
    }
    if (dsq < cd) {
        //TODO check teams, sight
        cd = dsq;
        ce = value;
    }
    END_HASHMAP_ITERATION(world->entities)
    if (entity->attacking != NULL && ce != entity->attacking) put_hashmap(entity->attacking->attackers, entity->id, NULL);
    if (ce != NULL && entity->attacking != ce) {
        put_hashmap(ce->attackers, entity->id, entity);
    }
    entity->attacking = ce;
    return 0;
}

int ai_shouldocelotattack(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldocelotsit(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldopendoor(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldownerhurtbytarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldownerhurttarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldpanic(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldplay(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldrestructopendoor(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldrestrictsun(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldrunaroundlikecrazy(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldsit(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldskeletonriders(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldswimming(struct world* world, struct entity* entity, struct aitask* ai) {
    return entity->inWater || entity->inLava;
}

int ai_shouldtarget(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldtargetnontamed(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldtempt(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldtradeplayer(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldvillagerinteract(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldvillagermate(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldwander(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldwanderaintwater(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldwatchclosest(struct world* world, struct entity* entity, struct aitask* ai) {
    return 0;
}

int ai_shouldzombieattack(struct world* world, struct entity* entity, struct aitask* ai) {
    return ai_shouldattackmelee(world, entity, ai);
}
