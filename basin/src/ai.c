#include "ai.h"
#include "entity.h"
#include "world.h"
#include "hashmap.h"
#include "util.h"
#include "game.h"
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
	getEntityCollision(entity, &bb);
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

int32_t ai_attackmelee(struct world* world, struct entity* entity, struct aitask* ai) {
	struct ai_attackmelee_data* amd = ai->data;
	entity->ai->lookHelper_speedYaw = 30.;
	entity->ai->lookHelper_speedPitch = 30.;
	entity->ai->lookHelper_x = entity->attacking->x;
	struct boundingbox bb;
	getEntityCollision(entity->attacking, &bb);
	entity->ai->lookHelper_y = entity->attacking->y + ((bb.maxY - bb.minY) * .9); // TODO: real eye height
	entity->ai->lookHelper_z = entity->attacking->z;
	double dist = entity_distsq(entity->attacking, entity);
	amd->delayCounter--;
//TODO: cansee
	if ((amd->longMemory || 1) && amd->delayCounter <= 0 && ((amd->targetX == 0. && amd->targetY == 0. && amd->targetZ == 0.) || entity_distsq_block(entity->attacking, amd->targetX, amd->targetY, amd->targetZ) >= 1. || randFloat() < .05)) {
		amd->targetX = entity->attacking->x;
		amd->targetY = entity->attacking->y;
		amd->targetZ = entity->attacking->z;
		if (dist > 1024.) amd->delayCounter += 10;
		else if (dist > 256.) amd->delayCounter += 5.;
		//TODO: set path
	}
	if (--amd->attackTick <= 0) amd->attackTick = 0;
	struct entity_info* ei = getEntityInfo(entity->type);
	struct entity_info* ei2 = getEntityInfo(entity->attacking->type);
	float reach = ei->width * 2. * ei->width * 2. + ei2->width;
	if (dist <= reach && amd->attackTick <= 0) {
		amd->attackTick = 20;
		swingArm(entity);
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
//if (randFloat() < .8) {
	jump(entity);
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
	struct entity_info* ei = getEntityInfo(entity->type);
	struct entity_info* ei2 = getEntityInfo(entity->attacking->type);
	if (ei == NULL || ei2 == NULL || !hasFlag(ei2, "livingbase")) return 0;
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
	if (!hasFlag(getEntityInfo(ie->type), "livingbase") || ie == entity) continue;
	double dsq = entity_distsq(entity, value);
	if (ie->type == ENT_PLAYER) {
		struct player* pl = ie->data.player.player;
		if (pl->gamemode == 1 || pl->gamemode == 3 || pl->invulnerable) continue;
		int sk = hasFlag(getEntityInfo(entity->type), "skeleton");
		int zo = hasFlag(getEntityInfo(entity->type), "zombie");
		int cr = hasFlag(getEntityInfo(entity->type), "creeper");
		if (sk || zo || cr) {
			struct slot* hs = getSlot(pl, pl->inventory, 5);
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
