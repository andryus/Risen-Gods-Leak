/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEF_ULDUAR_H
#define DEF_ULDUAR_H
#define DataHeader "UU"

#include "Creature.h"

enum UlduarEncounters
{
    BOSS_LEVIATHAN              = 0,
    BOSS_IGNIS                  = 1,
    BOSS_RAZORSCALE             = 2,
    BOSS_XT002                  = 3,
    BOSS_ASSEMBLY               = 4,
    BOSS_KOLOGARN               = 5,
    BOSS_AURIAYA                = 6,
    BOSS_HODIR                  = 7,
    BOSS_THORIM                 = 8,
    BOSS_FREYA                  = 9,
    BOSS_MIMIRON                = 10,
    BOSS_VEZAX                  = 11,
    BOSS_YOGGSARON              = 12,
    BOSS_ALGALON                = 13,
    MAX_ENCOUNTER,
};

enum UlduarDatas
{
    // Flame Leviathan
    DATA_COLOSSUS               = 20,
    DATA_LEVIATHAN_HARDMODE,

    // Ignis

    // Razorscale
    DATA_EXP_COMMANDER,
    DATA_RAZORSCALE_CONTROL,

    // Assembly of Iorn
    DATA_STEELBREAKER,
    DATA_MOLGEIM,
    DATA_BRUNDIR,
    DATA_ASSEMBLY_DEAD,

    // Auriaya
    DATA_SANCTUM_SENTRY_0,
    DATA_SANCTUM_SENTRY_1,
    DATA_SANCTUM_SENTRY_2,
    DATA_SANCTUM_SENTRY_3,

    // Hodir
    DATA_HODIR_RARE_CACHE_HERO,
    DATA_HODIR_RARE_CACHE,
    DATA_HODIR_CHEST_HERO,
    DATA_HODIR_CHEST, 

    // Thorim
    DATA_RUNIC_COLOSSUS,
    DATA_RUNE_GIANT,
    DATA_RUNIC_DOOR,
    DATA_STONE_DOOR,
    DATA_SIF_BLIZZARD,
    DATA_THORIM_LEVER,

    // Freya
    BOSS_ELDER_BRIGHTLEAF,
    BOSS_ELDER_STONEBARK,
    BOSS_ELDER_IRONBRANCH,

    // Mimiron
    DATA_LEVIATHAN_MK_II,
    DATA_VX_001,
    DATA_AERIAL_UNIT,
    DATA_MIMIRON_ELEVATOR,
    DATA_MAGNETIC_CORE,
    DATA_CALL_TRAM,
    DATA_FLAME_COUNT,
    DATA_MIMIRON_TELEPORTER,

    // YoggSaron
    DATA_SARA,
    DATA_BRAIN_DOOR_1,
    DATA_BRAIN_DOOR_2,
    DATA_BRAIN_DOOR_3,
    DATA_ADD_HELP_FLAG,
    DATA_KEEPER_SUPPORT_YOGG,
    DATA_COUNT_KEEPER,
    DATA_PORTAL_PHASE,

    // Algalon the Observer
    DATA_ALGALON_SUMMONED,
    DATA_SIGILDOOR_01,
    DATA_SIGILDOOR_02,
    DATA_SIGILDOOR_03,
    DATA_UNIVERSE_FLOOR_01,
    DATA_UNIVERSE_FLOOR_02,
    DATA_UNIVERSE_GLOBE,
    DATA_ALGALON_TRAPDOOR,
    DATA_BRANN_BRONZEBEARD_ALG,

    // Misc
    DATA_RAID_IS_ALLIANZ,
    DATA_PLAYER_DEATH_MASK,
    DATA_DWARF_COUNT,
};

enum UlduarNpcs
{
    // Flame Leviathan
    NPC_LEVIATHAN                           = 33113,
    NPC_COLOSSUS                            = 33237,
    NPC_VEHICLE_SIEGE                       = 33060,
    NPC_VEHICLE_CHOPPER                     = 33062,
    NPC_VEHICLE_DEMOLISHER                  = 33109,

        // Gauntlet
    NPC_STEELFORGED_DEFFENDER               = 33236,
        // Kill Event
    NPC_MAGE                                = 33672,

    // Ignis
    NPC_IGNIS                               = 33118,

    // Razorscale
    NPC_RAZORSCALE                          = 33186,
    NPC_EXPEDITION_COMMANDER                = 33210,
    NPC_EXPEDITION_MERCENARY                = 34144,
    NPC_EXPEDITION_ENGINEER                 = 34145,

    // XT-002
    NPC_XT002                               = 33293,
    NPC_XT_TOY_PILE                         = 33337,

    // Assembly
    NPC_STEELBREAKER                        = 32867,
    NPC_MOLGEIM                             = 32927,
    NPC_BRUNDIR                             = 32857,

    // Kologarn
    NPC_KOLOGARN                            = 32930,
    NPC_RUBBLE                              = 33768,

    // Auriaya
    NPC_AURIAYA                             = 33515,
    NPC_SANCTUM_SENTRY                      = 34014,

    // Mimiron
    NPC_MIMIRON                             = 33350,
    NPC_AERIAL_COMMAND_UNIT                 = 33670,
    NPC_MAGNETIC_CORE                       = 34068,
    NPC_LEVIATHAN_MKII                      = 33432,
    NPC_VX_001                              = 33651,

    // Hodir
    NPC_HODIR                               = 32845,

        // Helper
    NPC_TOR_GREYCLOUD                       = 32941,
    NPC_KAR_GREYCLOUD                       = 33333,
    NPC_EIVI_NIGHTFEATHER                   = 33325,
    NPC_ELLIE_NIGHTFEATHER                  = 32901,
    NPC_SPIRITWALKER_TARA                   = 33332,
    NPC_SPIRITWALKER_YONA                   = 32950,
    NPC_ELEMENTALIST_MAHFUUN                = 33328,
    NPC_ELEMENTALIST_AVUUN                  = 32900,
    NPC_AMIRA_BLAZEWEAVER                   = 33331,
    NPC_VEESHA_BLAZEWEAVER                  = 32946,
    NPC_MISSY_FLAMECUFFS                    = 32893,
    NPC_SISSY_FLAMECUFFS                    = 33327,
    NPC_BATTLE_PRIEST_ELIZA                 = 32948,
    NPC_BATTLE_PRIEST_GINA                  = 33330,
    NPC_FIELD_MEDIC_PENNY                   = 32897,
    NPC_FIELD_MEDIC_JESSI                   = 33326,

    // Thorim
    NPC_THORIM                              = 32865,
    NPC_RUNIC_COLOSSUS                      = 32872,
    NPC_RUNE_GIANT                          = 32873,
    NPC_THORIM_EVENT_BUNNY                  = 32892,

    // Freya
    NPC_FREYA                               = 32906,
    NPC_ELDER_IRONBRANCH                    = 32913,
    NPC_ELDER_STONEBARK                     = 32914,
    NPC_ELDER_BRIGHTLEAF                    = 32915,
    NPC_FREYA_ACHIEVE_TRIGGER               = 33406, 

        // Trash
    NPC_CORRUPTED_SERVITOR                  = 33354,
    NPC_MISGUIDED_NYMPH                     = 33355,
    NPC_GUARDIAN_LASHER                     = 33430,
    NPC_FOREST_SWARMER                      = 33431,
    NPC_MANGROVE_ENT                        = 33525,
    NPC_IRONROOT_LASHER                     = 33526,
    NPC_NATURES_BLADE                       = 33527,
    NPC_GUARDIAN_OF_LIFE                    = 33528,

    // Vezax
    NPC_VEZAX                               = 33271,

    // Yogg Saron
    NPC_YOGGSARON                           = 33288,
    NPC_SARA                                = 33134,

        // Keepers on Walkway
    NPC_KEEPER_WALKWAY_FREYA                = 33241,
    NPC_KEEPER_WALKWAY_MIMIRON              = 33244,
    NPC_KEEPER_WALKWAY_THORIM               = 33242,
    NPC_KEEPER_WALKWAY_HODIR                = 33213,

        // Keepers in Yoggroom
    NPC_KEEPER_YOGGROOM_FREYA               = 33410,
    NPC_KEEPER_YOGGROOM_HODIR               = 33411,
    NPC_KEEPER_YOGGROOM_MIMIRON             = 33412,
    NPC_KEEPER_YOGGROOM_THORIM              = 33413,


    // Algalon
    NPC_ALGALON                             = 32871,
    NPC_BRANN_BRONZBEARD_ALG                = 34064,
    NPC_AZEROTH                             = 34246,
    NPC_LIVING_CONSTELLATION                = 33052,
    NPC_ALGALON_STALKER                     = 33086,
    NPC_COLLAPSING_STAR                     = 32955,
    NPC_BLACK_HOLE                          = 32953,
    NPC_WORM_HOLE                           = 34099,
    NPC_ALGALON_VOID_ZONE_VISUAL_STALKER    = 34100,
    NPC_ALGALON_STALKER_ASTEROID_TARGET_01  = 33104,
    NPC_ALGALON_STALKER_ASTEROID_TARGET_02  = 33105,
    NPC_UNLEASHED_DARK_MATTER               = 34097,
};

enum UlduarGameObjects
{
    // Flame Leviathan
    GO_LEVIATHAN_DOOR_S         = 194905,
    GO_LEVIATHAN_DOOR_N         = 411507,   // Custom GO Template
    GO_LEVIATHAN_GATE           = 194630,
        // Crystalls
    GO_THORIM_STORM_GEN         = 194666,
    GO_HODIR_STORM_GEN          = 194665,
    GO_MIMIRON_STORM_GEN        = 194664,
    GO_FREYA_STORM_GEN          = 194663,
    GO_HODIR_TAR_CRYST          = 194707,
    GO_THORIM_TAR_CRYST         = 194706,
    GO_MIMIRON_TAR_CRYST        = 194705,
    GO_FREYA_TAR_CRYST          = 194704,
        // Kill Event
    GO_PORTAL_DALARAN           = 194481,

    // Razorscale
    GO_MOLE_MACHINE             = 194316,

    // Ignis

    // XT-002
    GO_XT002_DOOR               = 194631,

    // Assembly
    GO_IRON_COUNCIL_ENTRANCE    = 194554,
    GO_ARCHIVUM_DOOR            = 194556,

    // Kologarn
    GO_KOLOGARN_CHEST_HERO      = 195047,
    GO_KOLOGARN_CHEST           = 195046,
    GO_KOLOGARN_BRIDGE          = 194232,
    GO_KOLOGARN_DOOR            = 194553,

    // Auriaya

    // Hodir
    GO_HODIR_RARE_CACHE_HERO    = 194201,
    GO_HODIR_RARE_CACHE         = 194200,
    GO_HODIR_CHEST_HERO         = 194308,
    GO_HODIR_CHEST              = 194307,
    GO_HODIR_IN_DOOR_STONE      = 194442,
    GO_HODIR_OUT_DOOR_ICE       = 194441,
    GO_HODIR_OUT_DOOR_STONE     = 194634,

    // Thorim
    GO_THORIM_CHEST_HERO        = 194315,
    GO_THORIM_CHEST             = 194314,
    GO_THORIM_ENCOUNTER_DOOR    = 194559,
    GO_THORIM_STONE_DOOR        = 194558,
    GO_THORIM_RUNIC_DOOR        = 194557,
    GO_THORIM_LEVER             = 194264,

    // Freya
    GO_FREYA_CHEST_0_ELDER_10   = 194324,
    GO_FREYA_CHEST_1_ELDER_10   = 194325,
    GO_FREYA_CHEST_2_ELDER_10   = 194326,
    GO_FREYA_CHEST_3_ELDER_10   = 194327,
    GO_FREYA_CHEST_0_ELDER_25   = 194328,
    GO_FREYA_CHEST_1_ELDER_25   = 194329,
    GO_FREYA_CHEST_2_ELDER_25   = 194330,
    GO_FREYA_CHEST_3_ELDER_25   = 194331,

    // Mimiron
    GO_MIMIRON_TRAIN            = 194675,
    GO_MIMIRON_ELEVATOR         = 194749,
    GO_MIMIRON_DOOR_1           = 194776,
    GO_MIMIRON_DOOR_2           = 194774,
    GO_MIMIRON_DOOR_3           = 194775,
    GO_BIG_RED_BUTTON           = 194739,

    // Vezax
    GO_WAY_TO_YOGG              = 194255, // Before Vezax
    GO_VEZAX_DOOR               = 194750,

    // Yogg Saron
    GO_YOGGSARON_DOOR           = 194773,
    GO_YOGGBRAIN_DOOR_1         = 194635,
    GO_YOGGBRAIN_DOOR_2         = 194636,
    GO_YOGGBRAIN_DOOR_3         = 194637,

    // Algalon
    GO_CELESTIAL_PLANETARIUM_ACCESS_10      = 194628,
    GO_CELESTIAL_PLANETARIUM_ACCESS_25      = 194752,
    GO_DOODAD_UL_SIGILDOOR_01               = 194767,
    GO_DOODAD_UL_SIGILDOOR_02               = 194911,
    GO_DOODAD_UL_SIGILDOOR_03               = 194910,
    GO_DOODAD_UL_UNIVERSEFLOOR_01           = 194715,
    GO_DOODAD_UL_UNIVERSEFLOOR_02           = 194716,
    GO_DOODAD_UL_UNIVERSEGLOBE01            = 194148,
    GO_DOODAD_UL_ULDUAR_TRAPDOOR_03         = 194253,
    GO_GIFT_OF_THE_OBSERVER_10              = 194821,
    GO_GIFT_OF_THE_OBSERVER_25              = 194822,
};

enum UlduarSendEvents
{
    EVENT_TOWER_OF_LIFE_DESTROYED      = 21030,
    EVENT_TOWER_OF_STORM_DESTROYED     = 21031,
    EVENT_TOWER_OF_FROST_DESTROYED     = 21032,
    EVENT_TOWER_OF_FLAMES_DESTROYED    = 21033,
};

enum UlduarAchievements
{
    ACHIEVEMENT_CHAMPION_OF_ULDUAR      = 2903,
    ACHIEVEMENT_CONQUEROR_OF_ULDUAR     = 2904,
    ACHIEVEMENT_DWARFAGEDDON_10         = 3097,
    ACHIEVEMENT_DWARFAGEDDON_25         = 3098,
    ACHIEVEMENT_LUMBERJACK_10           = 2979,
    ACHIEVEMENT_LUMBERJACK_25           = 3118
};

enum UlduarAchievementCriteria
{
    CRITERIA_UNBROKEN_10                                = 10044, // Leviathan
    CRITERIA_UNBROKEN_25                                = 10045,
    CRITERIA_SHUTOUT_10                                 = 10054,
    CRITERIA_SHUTOUT_25                                 = 10055,
    CRITERIA_3_CAR_GARAGE_CHOPPER_10                    = 10046,
    CRITERIA_3_CAR_GARAGE_SIEGE_10                      = 10047,
    CRITERIA_3_CAR_GARAGE_DEMOLISHER_10                 = 10048,
    CRITERIA_3_CAR_GARAGE_CHOPPER_25                    = 10049,
    CRITERIA_3_CAR_GARAGE_SIEGE_25                      = 10050,
    CRITERIA_3_CAR_GARAGE_DEMOLISHER_25                 = 10051,
    CRITERIA_HOT_POCKET_10                              = 10430, // Ignis
    CRITERIA_HOT_POCKET_25                              = 10431,
    CRITERIA_QUICK_SHAVE_10                             = 10062, // Razorscale
    CRITERIA_QUICK_SHAVE_25                             = 10063,
    CRITERIA_THE_ASSASSINATION_OF_KING_LLANE_25         = 10321, // Yogg-Saron
    CRITERIA_THE_ASSASSINATION_OF_KING_LLANE_10         = 10324,
    CRITERIA_FORGING_OF_THE_DEMON_SOUL_25               = 10322,
    CRITERIA_FORGING_OF_THE_DEMON_SOUL_10               = 10325,
    CRITERIA_THE_TORTURED_CHAMPION_25                   = 10323,
    CRITERIA_THE_TORTURED_CHAMPION_10                   = 10326,

    CRITERIA_KILL_WITHOUT_DEATHS_FLAMELEVIATAN_10       = 10042,
    CRITERIA_KILL_WITHOUT_DEATHS_IGNIS_10               = 10342,
    CRITERIA_KILL_WITHOUT_DEATHS_RAZORSCALE_10          = 10340,
    CRITERIA_KILL_WITHOUT_DEATHS_XT002_10               = 10341,
    CRITERIA_KILL_WITHOUT_DEATHS_ASSEMBLY_10            = 10598,
    CRITERIA_KILL_WITHOUT_DEATHS_KOLOGARN_10            = 10348,
    CRITERIA_KILL_WITHOUT_DEATHS_AURIAYA_10             = 10351,
    CRITERIA_KILL_WITHOUT_DEATHS_HODIR_10               = 10439,
    CRITERIA_KILL_WITHOUT_DEATHS_THORIM_10              = 10403,
    CRITERIA_KILL_WITHOUT_DEATHS_FREYA_10               = 10582,
    CRITERIA_KILL_WITHOUT_DEATHS_MIMIRON_10             = 10347,
    CRITERIA_KILL_WITHOUT_DEATHS_VEZAX_10               = 10349,
    CRITERIA_KILL_WITHOUT_DEATHS_YOGGSARON_10           = 10350,

    CRITERIA_KILL_WITHOUT_DEATHS_FLAMELEVIATAN_25       = 10352,
    CRITERIA_KILL_WITHOUT_DEATHS_IGNIS_25               = 10355,
    CRITERIA_KILL_WITHOUT_DEATHS_RAZORSCALE_25          = 10353,
    CRITERIA_KILL_WITHOUT_DEATHS_XT002_25               = 10354,
    CRITERIA_KILL_WITHOUT_DEATHS_ASSEMBLY_25            = 10599,
    CRITERIA_KILL_WITHOUT_DEATHS_KOLOGARN_25            = 10357,
    CRITERIA_KILL_WITHOUT_DEATHS_AURIAYA_25             = 10363,
    CRITERIA_KILL_WITHOUT_DEATHS_HODIR_25               = 10719,
    CRITERIA_KILL_WITHOUT_DEATHS_THORIM_25              = 10404,
    CRITERIA_KILL_WITHOUT_DEATHS_FREYA_25               = 10583,
    CRITERIA_KILL_WITHOUT_DEATHS_MIMIRON_25             = 10361,
    CRITERIA_KILL_WITHOUT_DEATHS_VEZAX_25               = 10362,
    CRITERIA_KILL_WITHOUT_DEATHS_YOGGSARON_25           = 10364,

    CRITERIA_KILL_WITHOUT_DEATHS_ALGALON_10             = 10568,
    CRITERIA_KILL_WITHOUT_DEATHS_ALGALON_25             = 10570,
    CRITERIA_HERALD_OF_TITANS                           = 10678,
    CRITERIA_LUMBERJACKED                               = 21686,
};

//! ToDo: it's what?
enum UlduarAchievementCriteriaIds
{
    CRITERIA_CON_SPEED_ATORY                     = 21597,
    CRITERIA_DISARMED                            = 21687,
};

enum UlduarKeepers
{
    FREYA   = 0,
    THORIM  = 1,
    HODIR   = 2,
    MIMIRON = 3,
};

enum UlduarKeeperTowers
{
    THORIM_TOWER                      = 1 << THORIM,
    HODIR_TOWER                       = 1 << HODIR,
    FREYA_TOWER                       = 1 << FREYA,
    MIMIRON_TOWER                     = 1 << MIMIRON,
};

enum UlduarAchievementData
{
    MAX_HERALD_ARMOR_ITEMLEVEL                   = 226,
    MAX_HERALD_WEAPON_ITEMLEVEL                  = 232,
};

enum UlduarEvents
{
    EVENT_UPDATE_ALGALON_TIMER    = 1,
    EVENT_LEVIATHAN_DRIVE_OUT     = 2,
    EVENT_DWARFAGEDDON_TIMER      = 3,
    EVENT_LEVIATHAN_VEHICLE_CHECK = 4,
    EVENT_DESPAWN_LEVIATHAN_VEHICLES = 5
};

enum UlduarActions
{
    ACTION_ALGALON_SPAWN_INTRO      = 100001,
    ACTION_ASSEMBLY_PLAYER_DIED     = 100002,
    ACTION_ALGALON_SEND_REPLYCODE   = 100003,
    ACTION_DRIVE_TO_CENTER          = 100004,
};

enum UlduarWorldStates
{
    WORLD_STATE_ALGALON_DESPAWN_TIMER   = 4131,
    WORLD_STATE_ALGALON_TIMER_ENABLED   = 4132,
};

Position const SiegePositions[5] =
{
    { -814.59f, -64.54f, 429.92f, 5.969f },
    { -784.37f, -33.31f, 429.92f, 5.096f },
    { -808.99f, -52.10f, 429.92f, 5.668f },
    { -798.59f, -44.00f, 429.92f, 5.663f },
    { -812.83f, -77.71f, 429.92f, 0.046f },
};

Position const ChopperPositions[5] =
{
    { -717.83f, -106.56f, 430.02f, 0.122f },
    { -717.83f, -114.23f, 430.44f, 0.122f },
    { -717.83f, -109.70f, 430.22f, 0.122f },
    { -718.45f, -118.24f, 430.26f, 0.052f },
    { -718.45f, -123.58f, 430.41f, 0.085f },
};

Position const DemolisherPositions[5] =
{
    { -724.12f, -176.64f, 430.03f, 2.543f },
    { -766.70f, -225.03f, 430.50f, 1.710f },
    { -729.54f, -186.26f, 430.12f, 1.902f },
    { -756.01f, -219.23f, 430.50f, 2.369f },
    { -798.01f, -227.24f, 429.84f, 1.446f },
};

Position const DemolisherRespawnPositions[5] =
{
    { 125.75f, 58.05f, 409.80f, 0 },
    { 125.75f, 47.78f, 409.80f, 0 },
    { 109.09f, 57.64f, 409.80f, 0 },
    { 108.86f, 48.15f, 409.80f, 0 },
    { 117.55f, 36.39f, 409.80f, 0 },
};

Position const SiegeRespawnPositions[5] =
{
    { 128.51f, -20.26f, 409.80f, 0 },
    { 128.51f, -45.31f, 409.80f, 0 },
    { 108.59f, -31.62f, 409.80f, 0 },
    { 108.59f, -19.92f, 409.36f, 0 },
    { 108.59f, -44.63f, 409.81f, 0 },
};

Position const ChopperRespawnPositions[5] =
{
    { 126.87f, 27.78f, 409.95f, 0 },
    { 126.87f, 21.33f, 409.95f, 0 },
    { 117.71f, 27.50f, 410.40f, 0 },
    { 117.60f, 21.48f, 410.11f, 0 },
    { 121.80f, 16.57f, 409.80f, 0 },
};

Position const LeviathanFinishPositions[14] =
{
    { 214.305f, - 97.7288f, 409.902f,  4.69557f },
    { 214.345f, - 93.4653f, 409.902f,  4.70543f },
    { 214.357f, - 88.4769f, 409.902f,  4.72817f },
    { 244.563f, - 98.3617f, 409.819f,  4.68692f },
    { 244.701f, - 94.1342f, 409.819f,  4.80294f },
    { 244.495f, - 89.2472f, 409.819f,  4.60713f },
    { 256.192f, -100.648f,  409.817f,  4.53754f },
    { 256.93f,  - 96.4637f, 409.819f,  4.75397f },
    { 257.797f, - 91.5382f, 409.819f,  4.6667f  },
    { 224.66f,  - 98.4238f, 409.902f,  4.68688f },
    { 224.788f, - 93.4255f, 409.902f,  4.92305f },
    { 224.916f, - 88.4271f, 409.902f,  4.6683f  },
    { 232.422f, -110.840f,  409.803f, -4.69557f },
    { 239.007f, -110.462f,  409.803f, -4.69557f },
};


const Position walkwayKeeperSpawnLocation[4] =
{
    {2028.129f, 17.763f, 411.357f, 3.80698f},  // Mimiron
    {1946.400f, 32.388f, 421.532f, 5.38784f},  // Freya
    {2027.135f, -66.178f, 411.353f, 2.42861f}, // Thorim
    {1945.071f, -79.098f, 411.357f, 1.05809f}, // Hodir
};

const Position yoggroomKeeperSpawnLocation[4] =
{
    {1939.15f,  42.47f, 338.46f, 1.7f*static_cast<float>(M_PI)}, // Mimiron
    {2037.09f,  25.43f, 338.46f, 1.3f*static_cast<float>(M_PI)}, // Freya
    {2036.88f, -73.66f, 338.46f, 0.7f*static_cast<float>(M_PI)}, // Thorim
    {1939.94f, -90.49f, 338.46f, 0.3f*static_cast<float>(M_PI)}, // Hodir
};

const Position algalonSpawnLocation = { 1632.36f, -310.093f, 417.321f, 0.5f*static_cast<float>(M_PI)};

class PlayerOrPetCheck
{
    public:
        bool operator()(WorldObject* object) const
        {
            if (object->GetTypeId() == TYPEID_PLAYER)
                return false;
    
            if (Creature* creature = object->ToCreature())
                return !creature->IsPet();
    
            return true;
        }
};

#endif
