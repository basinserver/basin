/*
 * item.h
 *
 *  Created on: Mar 25, 2016
 *      Author: root
 */

#ifndef ITEM_H_
#define ITEM_H_

#include <stdint.h>

typedef int16_t item;

#include "player.h"
#include "world.h"
#include "entity.h"
#include "inventory.h"
#include "tools.h"

#define ITM_SHOVELIRON 256
#define ITM_PICKAXEIRON 257
#define ITM_HATCHETIRON 258
#define ITM_FLINTANDSTEEL 259
#define ITM_APPLE 260
#define ITM_BOW 261
#define ITM_ARROW 262
#define ITM_COAL 263
#define ITM_DIAMOND 264
#define ITM_INGOTIRON 265
#define ITM_INGOTGOLD 266
#define ITM_SWORDIRON 267
#define ITM_SWORDWOOD 268
#define ITM_SHOVELWOOD 269
#define ITM_PICKAXEWOOD 270
#define ITM_HATCHETWOOD 271
#define ITM_SWORDSTONE 272
#define ITM_SHOVELSTONE 273
#define ITM_PICKAXESTONE 274
#define ITM_HATCHETSTONE 275
#define ITM_SWORDDIAMOND 276
#define ITM_SHOVELDIAMOND 277
#define ITM_PICKAXEDIAMOND 278
#define ITM_HATCHETDIAMOND 279
#define ITM_STICK 280
#define ITM_BOWL 281
#define ITM_MUSHROOMSTEW 282
#define ITM_SWORDGOLD 283
#define ITM_SHOVELGOLD 284
#define ITM_PICKAXEGOLD 285
#define ITM_HATCHETGOLD 286
#define ITM_STRING 287
#define ITM_FEATHER 288
#define ITM_SULPHUR 289
#define ITM_HOEWOOD 290
#define ITM_HOESTONE 291
#define ITM_HOEIRON 292
#define ITM_HOEDIAMOND 293
#define ITM_HOEGOLD 294
#define ITM_SEEDS 295
#define ITM_WHEAT 296
#define ITM_BREAD 297
#define ITM_HELMETCLOTH 298
#define ITM_CHESTPLATECLOTH 299
#define ITM_LEGGINGSCLOTH 300
#define ITM_BOOTSCLOTH 301
#define ITM_HELMETCHAIN 302
#define ITM_CHESTPLATECHAIN 303
#define ITM_LEGGINGSCHAIN 304
#define ITM_BOOTSCHAIN 305
#define ITM_HELMETIRON 306
#define ITM_CHESTPLATEIRON 307
#define ITM_LEGGINGSIRON 308
#define ITM_BOOTSIRON 309
#define ITM_HELMETDIAMOND 310
#define ITM_CHESTPLATEDIAMOND 311
#define ITM_LEGGINGSDIAMOND 312
#define ITM_BOOTSDIAMOND 313
#define ITM_HELMETGOLD 314
#define ITM_CHESTPLATEGOLD 315
#define ITM_LEGGINGSGOLD 316
#define ITM_BOOTSGOLD 317
#define ITM_FLINT 318
#define ITM_PORKCHOPRAW 319
#define ITM_PORKCHOPCOOKED 320
#define ITM_PAINTING 321
#define ITM_APPLEGOLD 322
#define ITM_SIGN 323
#define ITM_DOOROAK 324
#define ITM_BUCKET 325
#define ITM_BUCKETWATER 326
#define ITM_BUCKETLAVA 327
#define ITM_MINECART 328
#define ITM_SADDLE 329
#define ITM_DOORIRON 330
#define ITM_REDSTONE 331
#define ITM_SNOWBALL 332
#define ITM_BOAT_OAK 333
#define ITM_LEATHER 334
#define ITM_MILK 335
#define ITM_BRICK 336
#define ITM_CLAY 337
#define ITM_REEDS 338
#define ITM_PAPER 339
#define ITM_BOOK 340
#define ITM_SLIMEBALL 341
#define ITM_MINECARTCHEST 342
#define ITM_MINECARTFURNACE 343
#define ITM_EGG 344
#define ITM_COMPASS 345
#define ITM_FISHINGROD 346
#define ITM_CLOCK 347
#define ITM_YELLOWDUST 348
#define ITM_FISH_COD_RAW 349
#define ITM_FISH_COD_COOKED 350
#define ITM_DYEPOWDER_BLACK 351
#define ITM_BONE 352
#define ITM_SUGAR 353
#define ITM_CAKE 354
#define ITM_BED 355
#define ITM_DIODE 356
#define ITM_COOKIE 357
#define ITM_MAP 358
#define ITM_SHEARS 359
#define ITM_MELON 360
#define ITM_SEEDS_PUMPKIN 361
#define ITM_SEEDS_MELON 362
#define ITM_BEEFRAW 363
#define ITM_BEEFCOOKED 364
#define ITM_CHICKENRAW 365
#define ITM_CHICKENCOOKED 366
#define ITM_ROTTENFLESH 367
#define ITM_ENDERPEARL 368
#define ITM_BLAZEROD 369
#define ITM_GHASTTEAR 370
#define ITM_GOLDNUGGET 371
#define ITM_NETHERSTALKSEEDS 372
#define ITM_POTION 373
#define ITM_GLASSBOTTLE 374
#define ITM_SPIDEREYE 375
#define ITM_FERMENTEDSPIDEREYE 376
#define ITM_BLAZEPOWDER 377
#define ITM_MAGMACREAM 378
#define ITM_BREWINGSTAND 379
#define ITM_CAULDRON 380
#define ITM_EYEOFENDER 381
#define ITM_SPECKLEDMELON 382
#define ITM_MONSTERPLACER 383
#define ITM_EXPBOTTLE 384
#define ITM_FIREBALL 385
#define ITM_WRITINGBOOK 386
#define ITM_WRITTENBOOK 387
#define ITM_EMERALD 388
#define ITM_FRAME 389
#define ITM_FLOWERPOT 390
#define ITM_CARROTS 391
#define ITM_POTATO 392
#define ITM_POTATOBAKED 393
#define ITM_POTATOPOISONOUS 394
#define ITM_EMPTYMAP 395
#define ITM_CARROTGOLDEN 396
#define ITM_SKULL_SKELETON 397
#define ITM_CARROTONASTICK 398
#define ITM_NETHERSTAR 399
#define ITM_PUMPKINPIE 400
#define ITM_FIREWORKS 401
#define ITM_FIREWORKSCHARGE 402
#define ITM_ENCHANTEDBOOK 403
#define ITM_COMPARATOR 404
#define ITM_NETHERBRICK 405
#define ITM_NETHERQUARTZ 406
#define ITM_MINECARTTNT 407
#define ITM_MINECARTHOPPER 408
#define ITM_PRISMARINESHARD 409
#define ITM_PRISMARINECRYSTALS 410
#define ITM_RABBITRAW 411
#define ITM_RABBITCOOKED 412
#define ITM_RABBITSTEW 413
#define ITM_RABBITFOOT 414
#define ITM_RABBITHIDE 415
#define ITM_ARMORSTAND 416
#define ITM_HORSEARMORMETAL 417
#define ITM_HORSEARMORGOLD 418
#define ITM_HORSEARMORDIAMOND 419
#define ITM_LEASH 420
#define ITM_NAMETAG 421
#define ITM_MINECARTCOMMANDBLOCK 422
#define ITM_MUTTONRAW 423
#define ITM_MUTTONCOOKED 424
#define ITM_END_CRYSTAL 426
#define ITM_DOORSPRUCE 427
#define ITM_DOORBIRCH 428
#define ITM_DOORJUNGLE 429
#define ITM_DOORACACIA 430
#define ITM_DOORDARKOAK 431
#define ITM_CHORUSFRUIT 432
#define ITM_CHORUSFRUITPOPPED 433
#define ITM_BEETROOT 434
#define ITM_BEETROOT_SEEDS 435
#define ITM_BEETROOT_SOUP 436
#define ITM_DRAGON_BREATH 437
#define ITM_SPLASH_POTION 438
#define ITM_SPECTRAL_ARROW 439
#define ITM_TIPPED_ARROW 440
#define ITM_LINGERING_POTION 441
#define ITM_SHIELD 442
#define ITM_ELYTRA 443
#define ITM_BOAT_SPRUCE 444
#define ITM_BOAT_BIRCH 445
#define ITM_BOAT_JUNGLE 446
#define ITM_BOAT_ACACIA 447
#define ITM_BOAT_DARK_OAK 448
#define ITM_TOTEM 449
#define ITM_SHULKERSHELL 450
#define ITM_RECORD_1 2256
#define ITM_RECORD_2 2257
#define ITM_RECORD_3 2258
#define ITM_RECORD_4 2259
#define ITM_RECORD_5 2260
#define ITM_RECORD_6 2261
#define ITM_RECORD_7 2262
#define ITM_RECORD_8 2263
#define ITM_RECORD_9 2264
#define ITM_RECORD_10 2265
#define ITM_RECORD_11 2266
#define ITM_RECORD_12 2267

#define ARMOR_NONE 0
#define ARMOR_HELMET 1
#define ARMOR_CHESTPLATE 2
#define ARMOR_LEGGINGS 3
#define ARMOR_BOOTS 4

struct item_info {
		struct tool_info* toolType;
		uint8_t maxStackSize;
		int16_t maxDamage;
		uint8_t armor;
		uint8_t armorType;
		float damage;
		float attackSpeed;
		float toolProficiency;
		uint8_t harvestLevel;
		void (*onItemUse)(struct world* world, struct player* player, struct slot* slot, uint16_t ticks); // not in-world usage
		void (*onItemInteract)(struct world* world, struct player* player, struct slot* slot, int32_t x, uint8_t y, int32_t z, uint8_t face); // in-world usage
		void (*onItemBreakBlock)(struct world* world, struct player* player, struct slot* slot, int32_t x, uint8_t y, int32_t z); // in-world usage
		void (*onItemAttacked)(struct world* world, struct player* player, struct slot* slot, struct entity* entity); // entity may be NULL
};

struct item_info* getItemInfo(item id);

void init_items();

void add_item(item id, struct item_info* info);

#endif /* ITEM_H_ */
