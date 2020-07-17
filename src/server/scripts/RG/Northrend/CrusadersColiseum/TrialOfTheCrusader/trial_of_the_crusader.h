/* Copyright (C) 2009 - 2010 by /dev/rsa for ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

#ifndef DEF_CRUSADER_H
#define DEF_CRUSADER_H

#define DataHeader "TCR"

enum TocEncounter
{
    BOSS_BEASTS                 = 0,
    BOSS_JARAXXUS               = 1,
    BOSS_CRUSADERS              = 2,
    BOSS_VALKIRIES              = 3,
    BOSS_LICH_KING              = 4,    // not really a boss but oh well
    BOSS_ANUBARAK               = 5,
    MAX_ENCOUNTER
};

enum Data
{
    TYPE_COUNTER                = 10,
    TYPE_EVENT,
    TYPE_EVENT_TIMER,
    TYPE_EVENT_NPC,
    TYPE_TEAM_IN_INSTANCE,

    TYPE_NORTHREND_BEASTS,

    DATA_MISTRESS_OF_PAIN_COUNT,
};

enum TocMisc
{
    DESPAWN_TIME                = 1200000,

    ACTION_JARAXXUS_START_FIGHT = 100,

    DISPLAYID_DESTROYED_FLOOR   = 9060,

    SPELL_WILFRED_PORTAL        = 68424,
    SPELL_JARAXXUS_CHAINS       = 67924,
    SPELL_CORPSE_TELEPORT       = 69016,
    SPELL_DESTROY_FLOOR_KNOCKUP = 68193,
};

enum TocNorthrendBeastsEncounterStates
{
    GORMOK_IN_PROGRESS              = 1000,
    GORMOK_DONE                     = 1001,
    SNAKES_IN_PROGRESS              = 2000,
    DREADSCALE_SUBMERGED            = 2001,
    ACIDMAW_SUBMERGED               = 2002,
    SNAKES_SPECIAL                  = 2003,
    SNAKES_DONE                     = 2004,
    ICEHOWL_IN_PROGRESS             = 3000,
    ICEHOWL_DONE                    = 3001
};

enum TocWorldStates
{
    UPDATE_STATE_UI_SHOW            = 4390,
    UPDATE_STATE_UI_COUNT           = 4389
};

enum TocCreature
{
    NPC_BARRENT                 = 34816,
    NPC_TIRION                  = 34996,
    NPC_TIRION_FORDRING         = 36095,
    NPC_ARGENT_MAGE             = 36097,
    NPC_FIZZLEBANG              = 35458,
    NPC_GARROSH                 = 34995,
    NPC_VARIAN                  = 34990,
    NPC_LICH_KING               = 35877,

    NPC_THRALL                  = 34994,
    NPC_PROUDMOORE              = 34992,
    NPC_WILFRED_PORTAL          = 17965,
    NPC_TRIGGER                 = 35651,


    NPC_GORMOK                  = 34796,
    NPC_SNOBOLD_VASSAL          = 34800,
    NPC_DREADSCALE              = 34799,
    NPC_ACIDMAW                 = 35144,
    NPC_ICEHOWL                 = 34797,

    NPC_JARAXXUS                = 34780,
    NPC_MISTRESS_OF_PAIN        = 34826,
    NPC_FEL_INFERNAL            = 34815,

    NPC_CHAMPIONS_CONTROLLER    = 34781,

    NPC_ALLIANCE_DEATH_KNIGHT           = 34461,
    NPC_ALLIANCE_DRUID_BALANCE          = 34460,
    NPC_ALLIANCE_DRUID_RESTORATION      = 34469,
    NPC_ALLIANCE_HUNTER                 = 34467,
    NPC_ALLIANCE_MAGE                   = 34468,
    NPC_ALLIANCE_PALADIN_HOLY           = 34465,
    NPC_ALLIANCE_PALADIN_RETRIBUTION    = 34471,
    NPC_ALLIANCE_PRIEST_DISCIPLINE      = 34466,
    NPC_ALLIANCE_PRIEST_SHADOW          = 34473,
    NPC_ALLIANCE_ROGUE                  = 34472,
    NPC_ALLIANCE_SHAMAN_ENHANCEMENT     = 34463,
    NPC_ALLIANCE_SHAMAN_RESTORATION     = 34470,
    NPC_ALLIANCE_WARLOCK                = 34474,
    NPC_ALLIANCE_WARRIOR                = 34475,

    NPC_HORDE_DEATH_KNIGHT              = 34458,
    NPC_HORDE_DRUID_BALANCE             = 34451,
    NPC_HORDE_DRUID_RESTORATION         = 34459,
    NPC_HORDE_HUNTER                    = 34448,
    NPC_HORDE_MAGE                      = 34449,
    NPC_HORDE_PALADIN_HOLY              = 34445,
    NPC_HORDE_PALADIN_RETRIBUTION       = 34456,
    NPC_HORDE_PRIEST_DISCIPLINE         = 34447,
    NPC_HORDE_PRIEST_SHADOW             = 34441,
    NPC_HORDE_ROGUE                     = 34454,
    NPC_HORDE_SHAMAN_ENHANCEMENT        = 34455,
    NPC_HORDE_SHAMAN_RESTORATION        = 34444,
    NPC_HORDE_WARLOCK                   = 34450,
    NPC_HORDE_WARRIOR                   = 34453,

    NPC_LIGHTBANE               = 34497,
    NPC_DARKBANE                = 34496,

    NPC_DARK_ESSENCE            = 34567,
    NPC_LIGHT_ESSENCE           = 34568,

    NPC_ANUBARAK                = 34564
};

enum TocGameObject
{
    GO_CRUSADERS_CACHE_10       = 195631,
    GO_CRUSADERS_CACHE_25       = 195632,
    GO_CRUSADERS_CACHE_10_H     = 195633,
    GO_CRUSADERS_CACHE_25_H     = 195635,

    // Tribute Chest (heroic)
    // 10-man modes
    GO_TRIBUTE_CHEST_10H_25     = 195668, // 10man 01-24 attempts
    GO_TRIBUTE_CHEST_10H_45     = 195667, // 10man 25-44 attempts
    GO_TRIBUTE_CHEST_10H_50     = 195666, // 10man 45-49 attempts
    GO_TRIBUTE_CHEST_10H_99     = 195665, // 10man 50 attempts
    // 25-man modes
    GO_TRIBUTE_CHEST_25H_25     = 195672, // 25man 01-24 attempts
    GO_TRIBUTE_CHEST_25H_45     = 195671, // 25man 25-44 attempts
    GO_TRIBUTE_CHEST_25H_50     = 195670, // 25man 45-49 attempts
    GO_TRIBUTE_CHEST_25H_99     = 195669, // 25man 50 attempts

    GO_ARGENT_COLISEUM_FLOOR    = 195527, //20943
    GO_MAIN_GATE_DOOR           = 195647,
    GO_EAST_PORTCULLIS          = 195648,
    GO_WEB_DOOR                 = 195485,
    GO_PORTAL_TO_DALARAN        = 195682
};

enum TocDedicatedInsanityItems
{
    // forbidden
    ITEM_VAL_ANYR           = 46017,
    ITEM_ONY_RING           = 49486,
    ITEM_ONY_NECK           = 49485,
    ITEM_ONY_TRINKET        = 49487,

    // ilvl 258 but allowed
    ITEM_TOC_10_CLOACK_1    = 48666,
    ITEM_TOC_10_CLOACK_2    = 48667,
    ITEM_TOC_10_CLOACK_3    = 48668,
    ITEM_TOC_10_CLOACK_4    = 48669,
    ITEM_TOC_10_CLOACK_5    = 48670,
    ITEM_TOC_10_CLOACK_6    = 48671,
    ITEM_TOC_10_CLOACK_7    = 48672,
    ITEM_TOC_10_CLOACK_8    = 48673,
    ITEM_TOC_10_CLOACK_9    = 48674,
    ITEM_TOC_10_CLOACK_10   = 48675,

    MAX_ALLOWED_ITEMLEVEL   = 245,
};
static uint32 AllowedItem [] = {ITEM_TOC_10_CLOACK_1, ITEM_TOC_10_CLOACK_2, ITEM_TOC_10_CLOACK_3, ITEM_TOC_10_CLOACK_4, ITEM_TOC_10_CLOACK_5,
    ITEM_TOC_10_CLOACK_6, ITEM_TOC_10_CLOACK_7, ITEM_TOC_10_CLOACK_8, ITEM_TOC_10_CLOACK_9, ITEM_TOC_10_CLOACK_10};

static uint32 ForbiddenItems[] =
{
    ITEM_VAL_ANYR, ITEM_ONY_RING, ITEM_ONY_NECK, ITEM_ONY_TRINKET,
    // onyxia
    49501, 49500, 49499, 49498, 49496, 49495, 49494, 49493, 49492, 49491,
    49490, 49489, 49488, 49484, 49483, 49482, 49481, 49480, 49479, 49478,
    49477, 49476, 49475, 49474, 49473, 49472, 49471, 49470, 49469, 49468,
    49467, 49466, 49465, 49464,
    // icehowl
    46958, 46959, 46960, 46961, 46962, 46963, 46972, 46974, 46976, 46979,
    46985, 46988, 46990, 46992, 47251, 47252, 47253, 47254, 47255, 47256,
    47258, 47259, 47260, 47261, 47262, 47263, 47264, 47265,
    // lord jaraxxus
    46994, 46996, 46997, 46999, 47000, 47041, 47042, 47043, 47051, 47052,
    47053, 47055, 47056, 47057, 47266, 47267, 47268, 47269, 47270, 47271,
    47272, 47273, 47274, 47275, 47276, 47277, 47279, 47280,
    // champions
    47069, 47070, 47071, 47072, 47073, 47079, 47080, 47081, 47082, 47083,
    47090, 47092, 47093, 47094, 47281, 47282, 47283, 47284, 47285, 47286,
    47287, 47288, 47289, 47290, 47292, 47293, 47294, 47295,
    // twins
    47104, 47106, 47107, 47108, 47114, 47115, 47116, 47121, 47126, 47138,
    47139, 47140, 47141, 47142, 47296, 47298, 47299, 47300, 47301, 47302,
    47303, 47304, 47305, 47306, 47307, 47308, 47309, 47310,
    // anub
    47054, 47148, 47150, 47151, 47152, 47182, 47183, 47184, 47186, 47187,
    47193, 47194, 47195, 47203, 47204, 47225, 47233, 47234, 47235, 47311,
    47312, 47313, 47314, 47316, 47317, 47318, 47319, 47320, 47321, 47322,
    47323, 47324, 47325, 47326, 47327, 47328, 47329, 47330,
    // pvp
    41841, 41886, 40884, 40890, 42081, 41837, 42042, 42119, 42044, 41626,
    41904, 41833, 49179, 49183, 49181, 42045, 42118, 42077, 41899, 41910,
    42076, 41882, 41894, 42082, 42041, 40883, 46374, 42078, 42329, 42046,
    41066, 41071, 41076, 49189, 41226, 42080, 42354, 42386, 41231, 42334,
    42043, 40979, 40984, 41618, 41622, 40978, 41056, 42229, 42287, 42234,
    42250, 42521, 42483, 42504, 42498, 42272, 42210, 42244, 42262, 42324,
    42292, 42267, 42348, 44424, 42319, 42366, 42515, 42257, 42282, 42487,
    44423, 42277, 49185, 42492, 41631, 41052, 41236, 42079, 41061, 42047,
    41636, 41641, 42392
};

static uint32 ForbiddenWeapons[] =
{
    42210, 42229, 42234, 42244, 42250, 42257, 42262, 42267, 42272, 42277,
    42282, 42287, 42292, 42319, 42324, 42329, 42334, 42348, 42354, 42366,
    42386, 42392, 42487, 42492, 42498, 42504, 42515, 42521, 44423, 44424,
    46017, 46958, 46963, 46979, 46994, 46996, 47069, 47079, 47104, 47114,
    47148, 47193, 47233, 47255, 47260, 47261, 47266, 47267, 47285, 47287,
    47300, 47302, 47314, 47322, 47329, 49185, 49189, 49465, 49493, 49494,
    49495, 49496, 49498, 49499, 49500, 49501
};

enum TocAvhievments
{
    ACHIEVMENT_CALL_OF_THE_CRUSADE_10       = 3917,
    ACHIEVMENT_CALL_OF_THE_CRUSADE_25       = 3916,
};

enum TocAchievementData
{
    // Northrend Beasts
    UPPER_BACK_PAIN_10_PLAYER               = 11779,
    UPPER_BACK_PAIN_10_PLAYER_HEROIC        = 11802,
    UPPER_BACK_PAIN_25_PLAYER               = 11780,
    UPPER_BACK_PAIN_25_PLAYER_HEROIC        = 11801,
    // Lord Jaraxxus
    THREE_SIXTY_PAIN_SPIKE_10_PLAYER        = 11838,
    THREE_SIXTY_PAIN_SPIKE_10_PLAYER_HEROIC = 11861,
    THREE_SIXTY_PAIN_SPIKE_25_PLAYER        = 11839,
    THREE_SIXTY_PAIN_SPIKE_25_PLAYER_HEROIC = 11862,
    // Tribute
    A_TRIBUTE_TO_SKILL_10_PLAYER            = 12344,
    A_TRIBUTE_TO_SKILL_25_PLAYER            = 12338,
    A_TRIBUTE_TO_MAD_SKILL_10_PLAYER        = 12347,
    A_TRIBUTE_TO_MAD_SKILL_25_PLAYER        = 12341,
    A_TRIBUTE_TO_INSANITY_10_PLAYER         = 12349,
    A_TRIBUTE_TO_INSANITY_25_PLAYER         = 12343,
    A_TRIBUTE_TO_IMMORTALITY_HORDE          = 12358,
    A_TRIBUTE_TO_IMMORTALITY_ALLIANCE       = 12359,
    A_TRIBUTE_TO_DEDICATED_INSANITY         = 12360,
    REALM_FIRST_GRAND_CRUSADER              = 12350,

    // Dummy spells - not existing in dbc but we don't need that
    SPELL_WORMS_KILLED_IN_10_SECONDS        = 68523,
    SPELL_CHAMPIONS_KILLED_IN_MINUTE        = 68620,
    SPELL_DEFEAT_FACTION_CHAMPIONS          = 68184,
    SPELL_TRAITOR_KING_10                   = 68186,
    SPELL_TRAITOR_KING_25                   = 68515,

    // Timed events
    EVENT_START_TWINS_FIGHT                 = 21853
};


enum TocConstantTime
{
    SEC = 1000,
    MIN = 60000,
};

#define PLATFORM_GROUND_Z 395.11f

const Position ToCSpawnLoc[]=
{
    {563.912f, 261.625f, 394.73f, 4.70437f},  //  0 Center
    {575.451f, 261.496f, 394.73f,  4.6541f},  //  1 Left
    {549.951f,  261.55f, 394.73f, 4.74835f}   //  2 Right
};

const Position ToCCommonLoc[]=
{
    {559.257996f, 90.266197f, 395.122986f,  5.04f},       //  0 Barrent

    {563.672974f, 139.571f,    393.837006f, 0},           //  1 Center
    {563.833008f, 187.244995f, 394.5f,      0},           //  2 Backdoor
    {577.347839f, 195.338888f, 395.14f,     0},           //  3 - Right
    {550.955933f, 195.338888f, 395.14f,     0},           //  4 - Left
    {563.833008f, 176.738953f, 394.585561f, 0},           //  5 - Center
    {573.5f,      180.5f,      395.14f,     0},           //  6 Move 0 Right
    {553.5f,      180.5f,      395.14f,     0},           //  7 Move 0 Left
    {573.0f,      170.0f,      395.14f,     4.78f},       //  8 Move 1 Right
    {549.0f,      168.0f,      395.14f,     4.78f},       //  9 Move 1 Left
    {563.8f,      216.1f,      395.1f,      0},           // 10 Behind the door

    {575.042358f, 195.260727f, 395.137146f, 0}, // 5
    {552.248901f, 195.331955f, 395.132658f, 0}, // 6
    {573.342285f, 195.515823f, 395.135956f, 0}, // 7
    {554.239929f, 195.825577f, 395.137909f, 0}, // 8
    {571.042358f, 195.260727f, 395.137146f, 0}, // 9
    {556.720581f, 195.015472f, 395.132658f, 0}, // 10
    {569.534119f, 195.214478f, 395.139526f, 0}, // 11
    {569.231201f, 195.941071f, 395.139526f, 0}, // 12
    {558.811610f, 195.985779f, 394.671661f, 0}, // 13
    {567.641724f, 195.351501f, 394.659943f, 0}, // 14
    {560.633972f, 195.391708f, 395.137543f, 0}, // 15
    {565.816956f, 195.477921f, 395.136810f, 0}, // 16
    {563.744873f, 77.5388871f, 395.204834f, 0}  // 17 Barrent exit
};

const Position JaraxxusLoc[]=
{
    {508.104767f, 138.247345f, 395.128052f, 0}, // 0 - Fizzlebang start location
    {548.610596f, 139.807800f, 394.321838f, 0}, // 1 - fizzlebang end
    {581.854187f, 138.0f, 394.319f, 0},         // 2 - Portal Right
    {550.558838f, 138.0f, 394.319f, 0}          // 3 - Portal Left
};

const Position FactionChampionLoc[]=
{
    {514.231f, 105.569f, 418.234f, 0},               //  0 - Horde Initial Pos 0
    {508.334f, 115.377f, 418.234f, 0},               //  1 - Horde Initial Pos 1
    {506.454f, 126.291f, 418.234f, 0},               //  2 - Horde Initial Pos 2
    {506.243f, 106.596f, 421.592f, 0},               //  3 - Horde Initial Pos 3
    {499.885f, 117.717f, 421.557f, 0},               //  4 - Horde Initial Pos 4

    {613.127f, 100.443f, 419.74f, 0},                //  5 - Ally Initial Pos 0
    {621.126f, 128.042f, 418.231f, 0},               //  6 - Ally Initial Pos 1
    {618.829f, 113.606f, 418.232f, 0},               //  7 - Ally Initial Pos 2
    {625.845f, 112.914f, 421.575f, 0},               //  8 - Ally Initial Pos 3
    {615.566f, 109.653f, 418.234f, 0},               //  9 - Ally Initial Pos 4

    {535.469f, 113.012f, 394.66f, 0},                // 10 - Horde Final Pos 0
    {526.417f, 137.465f, 394.749f, 0},               // 11 - Horde Final Pos 1
    {528.108f, 111.057f, 395.289f, 0},               // 12 - Horde Final Pos 2
    {519.92f, 134.285f, 395.289f, 0},                // 13 - Horde Final Pos 3
    {533.648f, 119.148f, 394.646f, 0},               // 14 - Horde Final Pos 4
    {531.399f, 125.63f, 394.708f, 0},                // 15 - Horde Final Pos 5
    {528.958f, 131.47f, 394.73f, 0},                 // 16 - Horde Final Pos 6
    {526.309f, 116.667f, 394.833f, 0},               // 17 - Horde Final Pos 7
    {524.238f, 122.411f, 394.819f, 0},               // 18 - Horde Final Pos 8
    {521.901f, 128.488f, 394.832f, 0}                // 19 - Horde Final Pos 9
};

const Position TwinValkyrsLoc[]=
{
    {541.021118f, 117.262932f, 394.41f, 0}, // 0 - Light essence 1
    {586.200562f, 162.145523f, 394.41f, 0}, // 1 - Light essence 2
    {586.060242f, 117.514809f, 394.41f, 0}, // 2 - Dark essence 1
    {541.602112f, 161.879837f, 394.41f, 0}  // 3 - Dark essence 2
};

const Position LichKingLoc[]=
{
    {563.549f, 152.474f, 394.393f, 0},          // 0 - Lich king start
    {563.547f, 141.613f, 393.908f, 0}           // 1 - Lich king end
};

const Position AnubarakLoc[]=
{
    {787.932556f, 133.289780f, 142.612152f, 0},  // 0 - Anub'arak start location
    {695.240051f, 137.834824f, 142.200000f, 0},  // 1 - Anub'arak move point location
    {694.886353f, 102.484665f, 142.119614f, 0},  // 3 - Nerub Spawn
    {694.500671f, 185.363968f, 142.117905f, 0},  // 5 - Nerub Spawn
    {731.987244f, 83.3824690f, 142.119614f, 0},  // 2 - Nerub Spawn
    {740.184509f, 193.443390f, 142.117584f, 0}   // 4 - Nerub Spawn
};

const Position EndSpawnLoc[]=
{
    {648.9167f, 131.0208f, 141.6161f, 0}, // 0 - Highlord Tirion Fordring
    {649.1614f, 142.0399f, 141.3057f ,0}, // 1 - Argent Mage
    {644.6250f, 149.2743f, 140.6015f ,0}  // 2 - Portal to Dalaran
};

#endif
