/*
 * entity.h
 *
 *  Created on: Feb 22, 2016
 *      Author: root
 */

#ifndef BASIN_ENTITY_H_
#define BASIN_ENTITY_H_

#include <basin/world.h>
#include <basin/item.h>
#include <stdint.h>
#include <stdlib.h>


#define ENT_PLAYER 0
#define ENT_ITEM 1
#define ENT_XPORB 2
#define ENT_AREAEFFECTCLOUD 3
#define ENT_ELDERGUARDIAN 4
#define ENT_WITHERSKELETON 5
#define ENT_STRAY 6
#define ENT_THROWNEGG 7
#define ENT_LEASHKNOT 8
#define ENT_PAINTING 9
#define ENT_ARROW 10
#define ENT_SNOWBALL 11
#define ENT_FIREBALL 12
#define ENT_SMALLFIREBALL 13
#define ENT_THROWNENDERPEARL 14
#define ENT_EYEOFENDERSIGNAL 15
#define ENT_THROWNPOTION 16
#define ENT_THROWNEXPBOTTLE 17
#define ENT_ITEMFRAME 18
#define ENT_WITHERSKULL 19
#define ENT_PRIMEDTNT 20
#define ENT_FALLINGBLOCK 21
#define ENT_FIREWORKSROCKETENTITY 22
#define ENT_HUSK 23
#define ENT_SPECTRALARROW 24
#define ENT_SHULKERBULLET 25
#define ENT_DRAGONFIREBALL 26
#define ENT_ZOMBIEVILLAGER 27
#define ENT_SKELETONHORSE 28
#define ENT_ZOMBIEHORSE 29
#define ENT_ARMORSTAND 30
#define ENT_DONKEY 31
#define ENT_MULE 32
#define ENT_EVOCATIONFANGS 33
#define ENT_EVOCATIONILLAGER 34
#define ENT_VEX 35
#define ENT_VINDICATIONILLAGER 36
#define ENT_MINECARTCOMMANDBLOCK 40
#define ENT_BOAT 41
#define ENT_MINECARTRIDEABLE 42
#define ENT_MINECARTCHEST 43
#define ENT_MINECARTFURNACE 44
#define ENT_MINECARTTNT 45
#define ENT_MINECARTHOPPER 46
#define ENT_MINECARTSPAWNER 47
#define ENT_CREEPER 50
#define ENT_SKELETON 51
#define ENT_SPIDER 52
#define ENT_GIANT 53
#define ENT_ZOMBIE 54
#define ENT_SLIME 55
#define ENT_GHAST 56
#define ENT_PIGZOMBIE 57
#define ENT_ENDERMAN 58
#define ENT_CAVESPIDER 59
#define ENT_SILVERFISH 60
#define ENT_BLAZE 61
#define ENT_LAVASLIME 62
#define ENT_ENDERDRAGON 63
#define ENT_WITHERBOSS 64
#define ENT_BAT 65
#define ENT_WITCH 66
#define ENT_ENDERMITE 67
#define ENT_GUARDIAN 68
#define ENT_SHULKER 69
#define ENT_PIG 90
#define ENT_SHEEP 91
#define ENT_COW 92
#define ENT_CHICKEN 93
#define ENT_SQUID 94
#define ENT_WOLF 95
#define ENT_MUSHROOMCOW 96
#define ENT_SNOWMAN 97
#define ENT_OZELOT 98
#define ENT_VILLAGERGOLEM 99
#define ENT_HORSE 100
#define ENT_RABBIT 101
#define ENT_POLARBEAR 102
#define ENT_LLAMA 103
#define ENT_LLAMASPIT 104
#define ENT_VILLAGER 120
#define ENT_ENDERCRYSTAL 200

#define POT_SPEED 1
#define POT_SLOWNESS 2
#define POT_HASTE 3
#define POT_MINING_FATIGUE 4
#define POT_STRENGTH 5
#define POT_INSTANT_HEALTH 6
#define POT_INSTANT_DAMAGE 7
#define POT_JUMP_BOOST 8
#define POT_NAUSEA 9
#define POT_REGENERATION 10
#define POT_RESISTANCE 11
#define POT_FIRE_RESISTANCE 12
#define POT_WATER_BREATHING 13
#define POT_INVISIBILITY 14
#define POT_BLINDNESS 15
#define POT_NIGHT_VISION 16
#define POT_HUNGER 17
#define POT_WEAKNESS 18
#define POT_POISON 19
#define POT_WITHER 20
#define POT_HEALTH_BOOST 21
#define POT_ABSORPTION 22
#define POT_SATURATION 23
#define POT_GLOWING 24
#define POT_LEVITATION 25
#define POT_LUCK 26
#define POT_UNLUCK 27

struct entity_loot {
        item id;
        uint8_t amountMax;
        uint8_t amountMin;
        uint8_t metaMin;
        uint8_t metaMax;
};

struct entity_info {
    char* name;
    float maxHealth;
    float width;
    float height;
    char** flags;
    size_t flag_count;
    uint32_t spawn_packet;
    int32_t spawn_packet_id;
    struct entity_loot* loots;
    size_t loot_count;
    char* dataname;
    void (*onDeath)(struct world* world, struct entity* entity, struct entity* causer); // causer may be NULL
    void (*onAttacked)(struct world* world, struct entity* entity, struct entity* attacker); // attacker may be NULL
    uint32_t (*onAITick)(struct world* world, struct entity* entity); // returns a tick delay before next AI tick, 0 = never tick again, 1 = 1 tick
    int (*onTick)(struct world* world, struct entity* entity); // if return != 0, then the tick is cancelled (for when the entity has been despawned)
    uint32_t (*initAI)(struct world* world, struct entity* entity); // returns a tick delay before next AI tick, 0 = never tick again, 1 = 1 tick
    void (*onSpawned)(struct world* world, struct entity* entity);
    void (*onInteract)(struct world* world, struct entity* entity, struct player* interacter, struct slot* item, int16_t slot_index);
};

void swingArm(struct entity* entity);

struct list* entity_infos;

void init_entities();

uint32_t getIDFromEntityDataName(const char* dataname);

struct entity_info* getEntityInfo(uint32_t id);

void add_entity_info(uint32_t eid, struct entity_info* bm);

struct potioneffect {
    char effectID;
    char amplifier;
    int32_t duration;
    char particles;
};

int hasFlag(struct entity_info* ei, char* flag);

union entity_data {
    struct entity_player {
        struct player* player;
    } player;
    struct entity_creeper {
    
    } creeper;
    struct entity_skeleton {
    
    } skeleton;
    struct entity_spider {
    
    } spider;
    struct entity_giant {
    
    } giant;
    struct entity_zombie {
    
    } zombie;
    struct entity_slime {
        uint8_t size;
    } slime;
    struct entity_ghast {
    
    } ghast;
    struct entity_zpigman {
    
    } zpigman;
    struct entity_enderman {
    
    } enderman;
    struct entity_cavespider {
    
    } cavespider;
    struct entity_silverfish {
    
    } silverfish;
    struct entity_blaze {

    } blaze;
    struct entity_magmacube {
        uint8_t size;
    } magmacube;
    struct entity_enderdragon {

    } enderdragon;
    struct entity_wither {

    } wither;
    struct entity_bat {

    } bat;
    struct entity_witch {

    } witch;
    struct entity_endermite {

    } endermite;
    struct entity_guardian {

    } guardian;
    struct entity_shulker {

    } shulker;
    struct entity_pig {

    } pig;
    struct entity_sheep {

    } sheep;
    struct entity_cow {

    } cow;
    struct entity_chicken {

    } chicken;
    struct entity_squid {

    } squid;
    struct entity_wolf {

    } wolf;
    struct entity_mooshroom {

    } mooshroom;
    struct entity_snowman {

    } snowman;
    struct entity_ocelot {

    } ocelot;
    struct entity_irongolem {

    } irongolem;
    struct entity_horse {

    } horse;
    struct entity_rabbit {

    } rabbit;
    struct entity_villager {

    } villager;
    struct entity_boat {

    } boat;
    struct entity_itemstack {
        struct slot* slot;
        int16_t delayBeforeCanPickup;
    } itemstack;
    struct entity_areaeffect {

    } areaeffect;
    struct entity_minecart {

    } minecart;
    struct entity_tnt {
        uint16_t fuse;
    } tnt;
    struct entity_endercrystal {

    } endercrystal;
    struct entity_arrow {
        uint32_t ticksInGround;
        uint8_t isCritical;
        uint8_t pickupFlags;
        float damage;
        float knockback;
    } arrow;
    struct entity_snowball {

    } snowball;
    struct entity_egg {

    } egg;
    struct entity_fireball {

    } fireball;
    struct entity_firecharge {

    } firecharge;
    struct entity_enderpearl {

    } enderpearl;
    struct entity_witherskull {

    } witherskull;
    struct entity_shulkerbullet {

    } shulkerbullet;
    struct entity_fallingblock {
        block b;
    } fallingblock;
    struct entity_itemframe {

    } itemframe;
    struct entity_eyeender {

    } eyeender;
    struct entity_thrownpotion {

    } thrownpotion;
    struct entity_husk {

    } husk;
    struct entity_fallingegg {

    } fallingegg;
    struct entity_expbottle {

    } expbottle;
    struct entity_firework {

    } firework;
    struct entity_leashknot {

    } leashknot;
    struct entity_armorstand {

    } armorstand;
    struct entity_fishingfloat {

    } fishingfloat;
    struct entity_evocationfangs {

    } evocationfangs;
    struct entity_elderguardian {

    } elderguardian;
    struct entity_dragonfireball {

    } dragonfireball;
    struct entity_experienceorb {
        uint16_t count;
    } experienceorb;
    struct entity_polarbear {

    } polarbear;
    struct entity_llama {

    } llama;
    struct entity_llamaspit {

    } llamaspit;
    struct entity_stray {

    } stray;
    struct entity_painting {
        char* title;
        uint8_t direction;
    } painting;
    struct entity_evocationillager {

    } evocationillager;
    struct entity_vex {

    } vex;
    struct entity_vindicationillager {

    } vindicationillager;
};

struct entity {
    struct mempool* pool;
    int32_t id;
    double x;
    double y;
    double z;
    double last_x;
    double last_y;
    double last_z;
    uint32_t type;
    float yaw;
    float pitch;
    float last_yaw;
    float last_pitch;
    float headpitch;
    int on_ground;
    int collidedVertically;
    int collidedHorizontally;
    double motX;
    double motY;
    double motZ;
    float health;
    float maxHealth;
    int32_t objectData;
    int markedKill;
    struct potioneffect* effects;
    size_t effect_count;
    int sneaking;
    int usingItemMain;
    int usingItemOff;
    int sprinting;
    int portalCooldown;
    size_t ticksExisted;
    int32_t subtype;
    float fallDistance;
    union entity_data data;
    struct world* world;
    struct hashmap* loadingPlayers;
    uint64_t age;
    uint8_t invincibilityTicks;
    uint8_t inWater;
    uint8_t inLava;
    uint8_t immovable;
    struct aicontext* ai;
    struct entity* attacking;
    struct hashmap* attackers;
};

struct entity* entity_new(struct world* world, int32_t id, double x, double y, double z, uint32_t type, float yaw, float pitch);

int damageEntityWithItem(struct entity* attacked, struct entity* attacker, uint8_t slot_index, struct slot* item);

int damageEntity(struct entity* attacked, float damage, int armorable);

void healEntity(struct entity* healed, float amount);

void readMetadata(struct entity* ent, unsigned char* meta, size_t size);

void writeMetadata(struct entity* ent, unsigned char** data, size_t* size);

void updateMetadata(struct entity* ent);

void jump(struct entity* entity);

int entity_inFluid(struct entity* entity, uint16_t blk, float ydown, int meta_check);

double entity_dist(struct entity* ent1, struct entity* ent2);

double entity_distsq(struct entity* ent1, struct entity* ent2);

double entity_distsq_block(struct entity* ent1, double x, double y, double z);

double entity_dist_block(struct entity* ent1, double x, double y, double z);

void getEntityCollision(struct entity* ent, struct boundingbox* bb);

int moveEntity(struct entity* entity, double* mx, double* my, double* mz, float shrink);

int tick_itemstack(struct world* world, struct entity* entity);

void tick_entity(struct world* world, struct entity* entity);

void freeEntity(struct entity* entity);

#endif /* BASIN_ENTITY_H_ */
