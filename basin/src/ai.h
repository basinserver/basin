#ifndef AI_H_
#define AI_H_

#include "hashmap.h"
#include "world.h"
#include "entity.h"
#include <stdint.h>

struct ai_attackmelee_data {
		int32_t delayCounter;
		double targetX;
		double targetY;
		double targetZ;
		int32_t attackInterval;
		double speed;
		int32_t attackTick;
		uint8_t longMemory;
};

struct ai_nearestattackabletarget_data {
		int32_t type;
		int32_t chance;
		uint8_t checkSight;
		int32_t targetSearchDelay;
		int32_t unseenTicks;
		double targetDist;
};

void initai_zombie(struct world* world, struct entity* entity);

struct aitask {
		int32_t (*ai_tick)(struct world* world, struct entity* entity, struct aitask* ai);
		int (*ai_should)(struct world* world, struct entity* entity, struct aitask* ai);
		void (*ai_cancel)(struct world* world, struct entity* entity, struct aitask* ai);
		uint8_t ai_running;
		int32_t ai_waiting;
		uint16_t mutex;
		void* data;
};

void ai_stdcancel(struct world* world, struct entity* entity, struct aitask* ai);

struct aitask* ai_initTask(struct entity* entity, uint16_t mutex, int32_t (*ai_tick)(struct world* world, struct entity* entity, struct aitask* ai), int (*ai_should)(struct world* world, struct entity* entity, struct aitask* ai), void* data);

struct aipath {

};

struct aicontext {
		struct hashmap* tasks;
		uint32_t ai_waiting;
		uint16_t mutex;
		double lookHelper_x;
		double lookHelper_y;
		double lookHelper_z;
		float lookHelper_speedYaw; // 0. to disable.
		float lookHelper_speedPitch; // 0. to disable.
};

void lookHelper_tick(struct entity* entity);

int32_t ai_handletasks(struct world* world, struct entity* entity);

int32_t ai_attackmelee(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_attackranged(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_attackrangedbow(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_avoidentity(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_beg(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_breakdoor(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_creeperswell(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_defendvillage(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_doorinteract(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_eatgrass(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_findentitynearest(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_findentitynearestplayer(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_fleesun(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_followgolem(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_followowner(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_followparent(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_harvestfarmland(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_hurtbytarget(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_leapattarget(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_llamafollowcaravan(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_lookattradeplayer(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_lookatvillager(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_lookidle(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_mate(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_moveindoors(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_movethroughvillage(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_movetoblock(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_movetowardsrestriction(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_movetowardstarget(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_nearestattackabletarget(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_ocelotattack(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_ocelotsit(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_opendoor(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_ownerhurtbytarget(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_ownerhurttarget(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_panic(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_play(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_restructopendoor(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_restrictsun(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_runaroundlikecrazy(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_sit(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_skeletonriders(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_swimming(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_target(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_targetnontamed(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_tempt(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_tradeplayer(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_villagerinteract(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_villagermate(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_wander(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_wanderavoidwater(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_watchclosest(struct world* world, struct entity* entity, struct aitask* ai);

int32_t ai_zombieattack(struct world* world, struct entity* entity, struct aitask* ai);

//shoulds

int ai_shouldattackmelee(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldattackranged(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldattackrangedbow(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldavoidentity(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldbeg(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldbreakdoor(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldcreeperswell(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shoulddefendvillage(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shoulddoorinteract(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldeatgrass(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldfindentitynearest(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldfindentitynearestplayer(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldfleesun(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldfollowgolem(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldfollowowner(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldfollowparent(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldharvestfarmland(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldhurtbytarget(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldleapattarget(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldllamafollowcaravan(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldlookattradeplayer(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldlookatvillager(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldlookidle(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldmate(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldmoveindoors(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldmovethroughvillage(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldmovetoblock(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldmovetowardsrestriction(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldmovetowardstarget(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldnearestattackabletarget(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldocelotattack(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldocelotsit(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldopendoor(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldownerhurtbytarget(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldownerhurttarget(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldpanic(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldplay(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldrestructopendoor(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldrestrictsun(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldrunaroundlikecrazy(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldsit(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldskeletonriders(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldswimming(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldtarget(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldtargetnontamed(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldtempt(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldtradeplayer(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldvillagerinteract(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldvillagermate(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldwander(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldwanderavoidwater(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldwatchclosest(struct world* world, struct entity* entity, struct aitask* ai);

int ai_shouldzombieattack(struct world* world, struct entity* entity, struct aitask* ai);

#endif /* AI_H_ */
