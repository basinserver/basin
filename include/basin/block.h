#ifndef BASIN_BLOCK_H_
#define BASIN_BLOCK_H_

#include <stdint.h>

typedef uint16_t block;

#include <basin/world.h>
#include <basin/player.h>
#include <basin/item.h>

#define BLK_AIR 0u
#define BLK_STONE 16u
#define BLK_STONE_GRANITE 17u
#define BLK_STONE_GRANITESMOOTH 18u
#define BLK_STONE_DIORITE 19u
#define BLK_STONE_DIORITESMOOTH 20u
#define BLK_STONE_ANDESITE 21u
#define BLK_STONE_ANDESITESMOOTH 22u
#define BLK_GRASS 32u
#define BLK_DIRT 48u
#define BLK_DIRT_COARSE 49u
#define BLK_DIRT_PODZOL 50u
#define BLK_COBBLESTONE 64u
#define BLK_WOOD_OAK 80u
#define BLK_WOOD_SPRUCE 81u
#define BLK_WOOD_BIRCH 82u
#define BLK_WOOD_JUNGLE 83u
#define BLK_WOOD_ACACIA 84u
#define BLK_WOOD_BIG_OAK 85u
#define BLK_SAPLING_OAK 96u
#define BLK_SAPLING_SPRUCE 97u
#define BLK_SAPLING_BIRCH 98u
#define BLK_SAPLING_JUNGLE 99u
#define BLK_SAPLING_ACACIA 100u
#define BLK_SAPLING_BIG_OAK 101u
#define BLK_SAPLING_OAK_1 105u
#define BLK_SAPLING_OAK_2 106u
#define BLK_SAPLING_OAK_3 107u
#define BLK_SAPLING_OAK_4 108u
#define BLK_SAPLING_OAK_5 109u
#define BLK_BEDROCK 112u
#define BLK_WATER 128u
#define BLK_WATER_1 144u
#define BLK_LAVA 160u
#define BLK_LAVA_1 176u
#define BLK_SAND 192u
#define BLK_SAND_RED 193u
#define BLK_GRAVEL 208u
#define BLK_OREGOLD 224u
#define BLK_OREIRON 240u
#define BLK_ORECOAL 256u
#define BLK_LOG_OAK 272u
#define BLK_LOG_SPRUCE 273u
#define BLK_LOG_BIRCH 274u
#define BLK_LOG_JUNGLE 275u
#define BLK_LOG_ACACIA 276u
#define BLK_LOG_BIG_OAK 277u
#define BLK_LOG_OAK_1 278u
#define BLK_LOG_OAK_2 279u
#define BLK_LOG_OAK_3 281u
#define BLK_LOG_OAK_4 282u
#define BLK_LOG_OAK_5 283u
#define BLK_LOG_OAK_6 285u
#define BLK_LOG_OAK_7 286u
#define BLK_LOG_OAK_8 287u
#define BLK_LEAVES_OAK 288u
#define BLK_LEAVES_SPRUCE 289u
#define BLK_LEAVES_BIRCH 290u
#define BLK_LEAVES_JUNGLE 291u
#define BLK_LEAVES_OAK_2 292u
#define BLK_LEAVES_SPRUCE_1 293u
#define BLK_LEAVES_BIRCH_1 294u
#define BLK_LEAVES_JUNGLE_1 295u
#define BLK_LEAVES_SPRUCE_2 297u
#define BLK_LEAVES_BIRCH_2 298u
#define BLK_LEAVES_JUNGLE_2 299u
#define BLK_LEAVES_OAK_3 300u
#define BLK_LEAVES_SPRUCE_3 301u
#define BLK_LEAVES_BIRCH_3 302u
#define BLK_LEAVES_JUNGLE_3 303u
#define BLK_SPONGE_DRY 304u
#define BLK_SPONGE_WET 305u
#define BLK_GLASS 320u
#define BLK_ORELAPIS 336u
#define BLK_BLOCKLAPIS 352u
#define BLK_DISPENSER 368u
#define BLK_SANDSTONE 384u
#define BLK_SANDSTONE_CHISELED 385u
#define BLK_SANDSTONE_SMOOTH 386u
#define BLK_MUSICBLOCK 400u
#define BLK_BED 416u
#define BLK_BED_1 424u
#define BLK_BED_2 425u
#define BLK_BED_3 426u
#define BLK_BED_4 427u
#define BLK_BED_5 428u
#define BLK_BED_6 429u
#define BLK_BED_7 430u
#define BLK_BED_8 431u
#define BLK_GOLDENRAIL 432u
#define BLK_DETECTORRAIL 448u
#define BLK_PISTONSTICKYBASE 464u
#define BLK_WEB 480u
#define BLK_TALLGRASS_SHRUB 496u
#define BLK_TALLGRASS_GRASS 497u
#define BLK_TALLGRASS_FERN 498u
#define BLK_DEADBUSH 512u
#define BLK_PISTONBASE 528u
#define BLK_PISTONBASE_1 544u
#define BLK_CLOTH_WHITE 560u
#define BLK_CLOTH_ORANGE 561u
#define BLK_CLOTH_MAGENTA 562u
#define BLK_CLOTH_LIGHTBLUE 563u
#define BLK_CLOTH_YELLOW 564u
#define BLK_CLOTH_LIME 565u
#define BLK_CLOTH_PINK 566u
#define BLK_CLOTH_GRAY 567u
#define BLK_CLOTH_SILVER 568u
#define BLK_CLOTH_CYAN 569u
#define BLK_CLOTH_PURPLE 570u
#define BLK_CLOTH_BLUE 571u
#define BLK_CLOTH_BROWN 572u
#define BLK_CLOTH_GREEN 573u
#define BLK_CLOTH_RED 574u
#define BLK_CLOTH_BLACK 575u
#define BLK_PISTONMOVING 576u
#define BLK_FLOWER1_DANDELION 592u
#define BLK_FLOWER2_POPPY 608u
#define BLK_FLOWER2_BLUEORCHID 609u
#define BLK_FLOWER2_ALLIUM 610u
#define BLK_FLOWER2_HOUSTONIA 611u
#define BLK_FLOWER2_TULIPRED 612u
#define BLK_FLOWER2_TULIPORANGE 613u
#define BLK_FLOWER2_TULIPWHITE 614u
#define BLK_FLOWER2_TULIPPINK 615u
#define BLK_FLOWER2_OXEYEDAISY 616u
#define BLK_MUSHROOM 624u
#define BLK_MUSHROOM_1 640u
#define BLK_BLOCKGOLD 656u
#define BLK_BLOCKIRON 672u
#define BLK_STONESLAB 688u
#define BLK_STONESLAB_1 689u
#define BLK_STONESLAB_2 690u
#define BLK_STONESLAB_3 691u
#define BLK_STONESLAB_4 692u
#define BLK_STONESLAB_5 693u
#define BLK_STONESLAB_6 694u
#define BLK_STONESLAB_7 695u
#define BLK_STONESLAB_8 697u
#define BLK_STONESLAB_9 698u
#define BLK_STONESLAB_10 699u
#define BLK_STONESLAB_11 700u
#define BLK_STONESLAB_12 701u
#define BLK_STONESLAB_13 702u
#define BLK_STONESLAB_14 703u
#define BLK_STONESLAB_STONE 704u
#define BLK_STONESLAB_SAND 705u
#define BLK_STONESLAB_WOOD 706u
#define BLK_STONESLAB_COBBLE 707u
#define BLK_STONESLAB_BRICK 708u
#define BLK_STONESLAB_SMOOTHSTONEBRICK 709u
#define BLK_STONESLAB_NETHERBRICK 710u
#define BLK_STONESLAB_QUARTZ 711u
#define BLK_STONESLAB_STONE_1 713u
#define BLK_STONESLAB_STONE_2 714u
#define BLK_STONESLAB_STONE_3 715u
#define BLK_STONESLAB_STONE_4 716u
#define BLK_STONESLAB_STONE_5 717u
#define BLK_STONESLAB_STONE_6 718u
#define BLK_STONESLAB_STONE_7 719u
#define BLK_BRICK 720u
#define BLK_TNT 736u
#define BLK_BOOKSHELF 752u
#define BLK_STONEMOSS 768u
#define BLK_OBSIDIAN 784u
#define BLK_TORCH 800u
#define BLK_FIRE 816u
#define BLK_MOBSPAWNER 832u
#define BLK_STAIRSWOOD 848u
#define BLK_CHEST 864u
#define BLK_REDSTONEDUST 880u
#define BLK_OREDIAMOND 896u
#define BLK_BLOCKDIAMOND 912u
#define BLK_WORKBENCH 928u
#define BLK_CROPS 944u
#define BLK_CROPS_1 951u
#define BLK_FARMLAND 960u
#define BLK_FURNACE 976u
#define BLK_FURNACE_1 992u
#define BLK_SIGN 1008u
#define BLK_DOOROAK 1024u
#define BLK_LADDER 1040u
#define BLK_RAIL 1056u
#define BLK_STAIRSSTONE 1072u
#define BLK_SIGN_1 1088u
#define BLK_LEVER 1104u
#define BLK_PRESSUREPLATESTONE 1120u
#define BLK_DOORIRON 1136u
#define BLK_PRESSUREPLATEWOOD 1152u
#define BLK_OREREDSTONE 1168u
#define BLK_OREREDSTONE_1 1184u
#define BLK_NOTGATE 1200u
#define BLK_NOTGATE_1 1216u
#define BLK_BUTTON 1232u
#define BLK_SNOW 1248u
#define BLK_ICE 1264u
#define BLK_SNOW_1 1280u
#define BLK_CACTUS 1296u
#define BLK_CLAY 1312u
#define BLK_REEDS 1328u
#define BLK_JUKEBOX 1344u
#define BLK_FENCE 1360u
#define BLK_PUMPKIN 1376u
#define BLK_HELLROCK 1392u
#define BLK_HELLSAND 1408u
#define BLK_LIGHTGEM 1424u
#define BLK_PORTAL 1440u
#define BLK_LITPUMPKIN 1456u
#define BLK_CAKE 1472u
#define BLK_DIODE 1488u
#define BLK_DIODE_1 1504u
#define BLK_STAINEDGLASS_WHITE 1520u
#define BLK_STAINEDGLASS_ORANGE 1521u
#define BLK_STAINEDGLASS_MAGENTA 1522u
#define BLK_STAINEDGLASS_LIGHTBLUE 1523u
#define BLK_STAINEDGLASS_YELLOW 1524u
#define BLK_STAINEDGLASS_LIME 1525u
#define BLK_STAINEDGLASS_PINK 1526u
#define BLK_STAINEDGLASS_GRAY 1527u
#define BLK_STAINEDGLASS_SILVER 1528u
#define BLK_STAINEDGLASS_CYAN 1529u
#define BLK_STAINEDGLASS_PURPLE 1530u
#define BLK_STAINEDGLASS_BLUE 1531u
#define BLK_STAINEDGLASS_BROWN 1532u
#define BLK_STAINEDGLASS_GREEN 1533u
#define BLK_STAINEDGLASS_RED 1534u
#define BLK_STAINEDGLASS_BLACK 1535u
#define BLK_TRAPDOOR 1536u
#define BLK_MONSTERSTONEEGG_STONE 1552u
#define BLK_MONSTERSTONEEGG_COBBLE 1553u
#define BLK_MONSTERSTONEEGG_BRICK 1554u
#define BLK_MONSTERSTONEEGG_MOSSYBRICK 1555u
#define BLK_MONSTERSTONEEGG_CRACKEDBRICK 1556u
#define BLK_MONSTERSTONEEGG_CHISELEDBRICK 1557u
#define BLK_STONEBRICKSMOOTH 1568u
#define BLK_STONEBRICKSMOOTH_MOSSY 1569u
#define BLK_STONEBRICKSMOOTH_CRACKED 1570u
#define BLK_STONEBRICKSMOOTH_CHISELED 1571u
#define BLK_MUSHROOM_2 1584u
#define BLK_MUSHROOM_3 1600u
#define BLK_FENCEIRON 1616u
#define BLK_THINGLASS 1632u
#define BLK_MELON 1648u
#define BLK_PUMPKINSTEM 1664u
#define BLK_PUMPKINSTEM_1 1680u
#define BLK_VINE 1696u
#define BLK_FENCEGATE 1712u
#define BLK_STAIRSBRICK 1728u
#define BLK_STAIRSSTONEBRICKSMOOTH 1744u
#define BLK_MYCEL 1760u
#define BLK_WATERLILY 1776u
#define BLK_NETHERBRICK 1792u
#define BLK_NETHERFENCE 1808u
#define BLK_STAIRSNETHERBRICK 1824u
#define BLK_NETHERSTALK 1840u
#define BLK_ENCHANTMENTTABLE 1856u
#define BLK_BREWINGSTAND 1872u
#define BLK_CAULDRON 1888u
#define BLK_NULL_1 1904u
#define BLK_ENDPORTALFRAME 1920u
#define BLK_WHITESTONE 1936u
#define BLK_DRAGONEGG 1952u
#define BLK_REDSTONELIGHT 1968u
#define BLK_REDSTONELIGHT_1 1984u
#define BLK_WOODSLAB 2000u
#define BLK_WOODSLAB_1 2001u
#define BLK_WOODSLAB_2 2002u
#define BLK_WOODSLAB_3 2003u
#define BLK_WOODSLAB_4 2004u
#define BLK_WOODSLAB_5 2005u
#define BLK_WOODSLAB_6 2009u
#define BLK_WOODSLAB_7 2010u
#define BLK_WOODSLAB_8 2011u
#define BLK_WOODSLAB_9 2012u
#define BLK_WOODSLAB_10 2013u
#define BLK_WOODSLAB_OAK 2016u
#define BLK_WOODSLAB_SPRUCE 2017u
#define BLK_WOODSLAB_BIRCH 2018u
#define BLK_WOODSLAB_JUNGLE 2019u
#define BLK_WOODSLAB_ACACIA 2020u
#define BLK_WOODSLAB_BIG_OAK 2021u
#define BLK_WOODSLAB_OAK_1 2025u
#define BLK_WOODSLAB_OAK_2 2026u
#define BLK_WOODSLAB_OAK_3 2027u
#define BLK_WOODSLAB_OAK_4 2028u
#define BLK_WOODSLAB_OAK_5 2029u
#define BLK_COCOA 2032u
#define BLK_STAIRSSANDSTONE 2048u
#define BLK_OREEMERALD 2064u
#define BLK_ENDERCHEST 2080u
#define BLK_TRIPWIRESOURCE 2096u
#define BLK_TRIPWIRE 2112u
#define BLK_BLOCKEMERALD 2128u
#define BLK_STAIRSWOODSPRUCE 2144u
#define BLK_STAIRSWOODBIRCH 2160u
#define BLK_STAIRSWOODJUNGLE 2176u
#define BLK_COMMANDBLOCK 2192u
#define BLK_BEACON 2208u
#define BLK_COBBLEWALL_NORMAL 2224u
#define BLK_COBBLEWALL_MOSSY 2225u
#define BLK_FLOWERPOT 2240u
#define BLK_CARROTS 2256u
#define BLK_POTATOES 2272u
#define BLK_BUTTON_1 2288u
#define BLK_SKULL 2304u
#define BLK_ANVIL 2320u
#define BLK_CHESTTRAP 2336u
#define BLK_WEIGHTEDPLATE_LIGHT 2352u
#define BLK_WEIGHTEDPLATE_HEAVY 2368u
#define BLK_COMPARATOR 2384u
#define BLK_COMPARATOR_1 2400u
#define BLK_DAYLIGHTDETECTOR 2416u
#define BLK_BLOCKREDSTONE 2432u
#define BLK_NETHERQUARTZ 2448u
#define BLK_HOPPER 2464u
#define BLK_QUARTZBLOCK 2480u
#define BLK_QUARTZBLOCK_CHISELED 2481u
#define BLK_QUARTZBLOCK_LINES 2482u
#define BLK_QUARTZBLOCK_1 2483u
#define BLK_QUARTZBLOCK_2 2484u
#define BLK_STAIRSQUARTZ 2496u
#define BLK_ACTIVATORRAIL 2512u
#define BLK_DROPPER 2528u
#define BLK_CLAYHARDENEDSTAINED_WHITE 2544u
#define BLK_CLAYHARDENEDSTAINED_ORANGE 2545u
#define BLK_CLAYHARDENEDSTAINED_MAGENTA 2546u
#define BLK_CLAYHARDENEDSTAINED_LIGHTBLUE 2547u
#define BLK_CLAYHARDENEDSTAINED_YELLOW 2548u
#define BLK_CLAYHARDENEDSTAINED_LIME 2549u
#define BLK_CLAYHARDENEDSTAINED_PINK 2550u
#define BLK_CLAYHARDENEDSTAINED_GRAY 2551u
#define BLK_CLAYHARDENEDSTAINED_SILVER 2552u
#define BLK_CLAYHARDENEDSTAINED_CYAN 2553u
#define BLK_CLAYHARDENEDSTAINED_PURPLE 2554u
#define BLK_CLAYHARDENEDSTAINED_BLUE 2555u
#define BLK_CLAYHARDENEDSTAINED_BROWN 2556u
#define BLK_CLAYHARDENEDSTAINED_GREEN 2557u
#define BLK_CLAYHARDENEDSTAINED_RED 2558u
#define BLK_CLAYHARDENEDSTAINED_BLACK 2559u
#define BLK_THINSTAINEDGLASS_WHITE 2560u
#define BLK_THINSTAINEDGLASS_ORANGE 2561u
#define BLK_THINSTAINEDGLASS_MAGENTA 2562u
#define BLK_THINSTAINEDGLASS_LIGHTBLUE 2563u
#define BLK_THINSTAINEDGLASS_YELLOW 2564u
#define BLK_THINSTAINEDGLASS_LIME 2565u
#define BLK_THINSTAINEDGLASS_PINK 2566u
#define BLK_THINSTAINEDGLASS_GRAY 2567u
#define BLK_THINSTAINEDGLASS_SILVER 2568u
#define BLK_THINSTAINEDGLASS_CYAN 2569u
#define BLK_THINSTAINEDGLASS_PURPLE 2570u
#define BLK_THINSTAINEDGLASS_BLUE 2571u
#define BLK_THINSTAINEDGLASS_BROWN 2572u
#define BLK_THINSTAINEDGLASS_GREEN 2573u
#define BLK_THINSTAINEDGLASS_RED 2574u
#define BLK_THINSTAINEDGLASS_BLACK 2575u
#define BLK_LEAVES_ACACIA 2576u
#define BLK_LEAVES_BIG_OAK 2577u
#define BLK_LEAVES_ACACIA_1 2580u
#define BLK_LEAVES_BIG_OAK_1 2581u
#define BLK_LEAVES_ACACIA_2 2584u
#define BLK_LEAVES_BIG_OAK_2 2585u
#define BLK_LEAVES_ACACIA_3 2588u
#define BLK_LEAVES_BIG_OAK_3 2589u
#define BLK_LOG_ACACIA_1 2592u
#define BLK_LOG_BIG_OAK_1 2593u
#define BLK_LOG_OAK_9 2596u
#define BLK_LOG_OAK_10 2597u
#define BLK_LOG_OAK_11 2600u
#define BLK_LOG_OAK_12 2601u
#define BLK_LOG_OAK_13 2604u
#define BLK_LOG_OAK_14 2605u
#define BLK_STAIRSWOODACACIA 2608u
#define BLK_STAIRSWOODDARKOAK 2624u
#define BLK_SLIME 2640u
#define BLK_BARRIER 2656u
#define BLK_IRONTRAPDOOR 2672u
#define BLK_PRISMARINE_ROUGH 2688u
#define BLK_PRISMARINE_BRICKS 2689u
#define BLK_PRISMARINE_DARK 2690u
#define BLK_SEALANTERN 2704u
#define BLK_HAYBLOCK 2720u
#define BLK_WOOLCARPET_WHITE 2736u
#define BLK_WOOLCARPET_ORANGE 2737u
#define BLK_WOOLCARPET_MAGENTA 2738u
#define BLK_WOOLCARPET_LIGHTBLUE 2739u
#define BLK_WOOLCARPET_YELLOW 2740u
#define BLK_WOOLCARPET_LIME 2741u
#define BLK_WOOLCARPET_PINK 2742u
#define BLK_WOOLCARPET_GRAY 2743u
#define BLK_WOOLCARPET_SILVER 2744u
#define BLK_WOOLCARPET_CYAN 2745u
#define BLK_WOOLCARPET_PURPLE 2746u
#define BLK_WOOLCARPET_BLUE 2747u
#define BLK_WOOLCARPET_BROWN 2748u
#define BLK_WOOLCARPET_GREEN 2749u
#define BLK_WOOLCARPET_RED 2750u
#define BLK_WOOLCARPET_BLACK 2751u
#define BLK_CLAYHARDENED 2752u
#define BLK_BLOCKCOAL 2768u
#define BLK_ICEPACKED 2784u
#define BLK_DOUBLEPLANT_SUNFLOWER 2800u
#define BLK_DOUBLEPLANT_SYRINGA 2801u
#define BLK_DOUBLEPLANT_GRASS 2802u
#define BLK_DOUBLEPLANT_FERN 2803u
#define BLK_DOUBLEPLANT_ROSE 2804u
#define BLK_DOUBLEPLANT_PAEONIA 2805u
#define BLK_DOUBLEPLANT_SUNFLOWER_1 2808u
#define BLK_DOUBLEPLANT_SUNFLOWER_2 2809u
#define BLK_DOUBLEPLANT_SUNFLOWER_3 2810u
#define BLK_DOUBLEPLANT_SUNFLOWER_4 2811u
#define BLK_DOUBLEPLANT_SUNFLOWER_5 2812u
#define BLK_DOUBLEPLANT_SUNFLOWER_6 2813u
#define BLK_DOUBLEPLANT_SUNFLOWER_7 2814u
#define BLK_DOUBLEPLANT_SUNFLOWER_8 2815u
#define BLK_BANNER 2816u
#define BLK_BANNER_1 2832u
#define BLK_DAYLIGHTDETECTOR_1 2848u
#define BLK_REDSANDSTONE 2864u
#define BLK_REDSANDSTONE_CHISELED 2865u
#define BLK_REDSANDSTONE_SMOOTH 2866u
#define BLK_STAIRSREDSANDSTONE 2880u
#define BLK_STONESLAB2 2896u
#define BLK_STONESLAB2_RED_SANDSTONE 2912u
#define BLK_SPRUCEFENCEGATE 2928u
#define BLK_BIRCHFENCEGATE 2944u
#define BLK_JUNGLEFENCEGATE 2960u
#define BLK_DARKOAKFENCEGATE 2976u
#define BLK_ACACIAFENCEGATE 2992u
#define BLK_SPRUCEFENCE 3008u
#define BLK_BIRCHFENCE 3024u
#define BLK_JUNGLEFENCE 3040u
#define BLK_DARKOAKFENCE 3056u
#define BLK_ACACIAFENCE 3072u
#define BLK_DOORSPRUCE 3088u
#define BLK_DOORBIRCH 3104u
#define BLK_DOORJUNGLE 3120u
#define BLK_DOORACACIA 3136u
#define BLK_DOORDARKOAK 3152u
#define BLK_ENDROD 3168u
#define BLK_CHORUSPLANT 3184u
#define BLK_CHORUSFLOWER 3200u
#define BLK_PURPURBLOCK 3216u
#define BLK_PURPURPILLAR 3232u
#define BLK_STAIRSPURPUR 3248u
#define BLK_PURPURSLAB 3264u
#define BLK_PURPURSLAB_1 3280u
#define BLK_ENDBRICKS 3296u
#define BLK_BEETROOTS 3312u
#define BLK_BEETROOTS_1 3315u
#define BLK_GRASSPATH 3328u
#define BLK_NULL_2 3344u
#define BLK_REPEATINGCOMMANDBLOCK 3360u
#define BLK_CHAINCOMMANDBLOCK 3376u
#define BLK_FROSTEDICE 3392u
#define BLK_MAGMA 3408u
#define BLK_NETHERWARTBLOCK 3424u
#define BLK_REDNETHERBRICK 3440u
#define BLK_BONEBLOCK 3456u
#define BLK_STRUCTUREVOID 3472u
#define BLK_OBSERVER 3488u
#define BLK_SHULKERBOXWHITE 3504u
#define BLK_SHULKERBOXORANGE 3520u
#define BLK_SHULKERBOXMAGENTA 3536u
#define BLK_SHULKERBOXLIGHTBLUE 3552u
#define BLK_SHULKERBOXYELLOW 3568u
#define BLK_SHULKERBOXLIME 3584u
#define BLK_SHULKERBOXPINK 3600u
#define BLK_SHULKERBOXGRAY 3616u
#define BLK_SHULKERBOXSILVER 3632u
#define BLK_SHULKERBOXCYAN 3648u
#define BLK_SHULKERBOXPURPLE 3664u
#define BLK_SHULKERBOXBLUE 3680u
#define BLK_SHULKERBOXBROWN 3696u
#define BLK_SHULKERBOXGREEN 3712u
#define BLK_SHULKERBOXRED 3728u
#define BLK_SHULKERBOXBLACK 3744u
#define BLK_STRUCTUREBLOCK 4080u

struct block_material {
        char* name;
        uint8_t flammable;
        uint8_t replacable;
        uint8_t requiresnotool;
        uint8_t mobility;
        uint8_t adventure_exempt;
        uint8_t liquid;
        uint8_t solid;
        uint8_t blocksLight;
        uint8_t blocksMovement;
        uint8_t opaque;
};

struct block_material* getBlockMaterial(char* name);

struct block_info* getBlockInfo(block b);

struct block_info* getBlockInfoLoose(block b);

uint8_t getFaceFromPlayer(struct player* player);

item getItemFromName(const char* name);

struct block_info {
    int (*onBlockDestroyed)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replacedBy);
    int (*onBlockDestroyedPlayer)(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z);
    block (*onBlockPlaced)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replaced);
    block (*onBlockPlacedPlayer)(struct player* player, struct world* world, block blk, int32_t x, int32_t y, int32_t z, uint8_t face);
    void (*onBlockInteract)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ);
    void (*onBlockCollide)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct entity* entity);
    void (*onBlockUpdate)(struct world* world, block blk, int32_t x, int32_t y, int32_t z);
    void (*dropItems)(struct world* world, block blk, int32_t x, int32_t y, int32_t z, int fortune);
    int (*canBePlaced)(struct world* world, block blk, int32_t x, int32_t y, int32_t z);
    void (*randomTick)(struct world* world, struct chunk* ch, block blk, int32_t x, int32_t y, int32_t z);
    int (*scheduledTick)(struct world* world, block blk, int32_t x, int32_t y, int32_t z);
    struct boundingbox* boundingBoxes;
    size_t boundingBox_count;
    float hardness;
    struct block_material* material;
    float slipperiness;
    item drop;
    int16_t drop_damage;
    uint8_t drop_min;
    uint8_t drop_max;
    uint8_t fullCube;
    uint8_t canProvidePower;
    uint8_t lightOpacity;
    uint8_t lightEmission;
    float resistance;
};

int falling_canFallThrough(block b);

int isNormalCube(struct block_info* bi);

void init_materials();

void init_blocks();

size_t getBlockSize();

void add_block_material(struct block_material* bm);

void add_block_info(block blk, struct block_info* bm);

void onBlockInteract_workbench(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ);

void onBlockInteract_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ);

void onBlockInteract_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z, struct player* player, uint8_t face, float curPosX, float curPosY, float curPosZ);

void update_furnace(struct world* world, struct tile_entity* te);

int onBlockDestroyed_chest(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replacedBy);

int onBlockDestroyed_furnace(struct world* world, block blk, int32_t x, int32_t y, int32_t z, block replacedBy);

int canBePlaced_reeds(struct world* world, block blk, int32_t x, int32_t y, int32_t z);

void randomTick_sapling(struct world* world, struct chunk* chunk, block blk, int32_t x, int32_t y, int32_t z);

#endif /* BLOCK_H_ */
