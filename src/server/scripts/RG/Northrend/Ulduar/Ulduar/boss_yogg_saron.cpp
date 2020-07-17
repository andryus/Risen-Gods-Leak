/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptMgr.h"
#include "ulduar.h"
#include "AchievementMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "DBCStores.h"

enum SaysSara
{
    SAY_SARA_PREFIGHT_1                         = 0, //-1603310, // unused
    SAY_SARA_PREFIGHT_2                         = 0, //-1603311, // unused
    SAY_SARA_AGGRO_1                            = 0, //-1603312, // unused
    SAY_SARA_AGGRO_2                            = 0, //-1603313, // unused
    SAY_SARA_AGGRO_3                            = 0, //-1603314, // unused
    SAY_SARA_SLAY_1                             = 0, //-1603315, // unused
    SAY_SARA_SLAY_2                             = 0, //-1603316, // unused
    WHISP_SARA_INSANITY                         = 0, //-1603317, // unused
    SAY_SARA_PHASE2_1                           = 0, //-1603318, // unused
    SAY_SARA_PHASE2_2                           = 0, //-1603319, // unused
};

enum SaysKeepers
{
    SAY_FREYA_HELP                              = 0, //-1603189, // unused
    SAY_MIMIRON_HELP                            = 0, //-1603259, // unused
    SAY_THORIM_HELP                             = 0,
    SAY_HODIR_HELP                              = 9,
};

enum SaysYogg
{
    SAY_BRAIN_INTRO_1                           = 4,
    SAY_BRAIN_INTRO_2                           = 5,
    SAY_BRAIN_INTRO_3                           = 6,
    SAY_BRAIN_INTRO_4                           = 7,
    SAY_BRAIN_INTRO_5                           = 0,
    SAY_PHASE3                                  = 1,
    SAY_VISION                                  = 2,
    SAY_LUNCATIC_GLAZE                          = 3,
    SAY_DEAFENING_ROAR                          = 4,
    WHISP_INSANITY_1                            = 5, //-1603339,
    WHISP_INSANITY_2                            = 0, //-1603340,
    SAY_DEATH                                   = 9,
};

enum SaysVision
{
    // King Llane
    SAY_GARONA_1_VISION_1                       = 8,
    SAY_GARONA_2_VISION_1                       = 7,
    SAY_GARONA_3_VISION_1                       = 6,
    SAY_YOGGSARON_4_VISION_1                    = 5,
    SAY_YOGGSARON_5_VISION_1                    = 4,
    SAY_LLANE_6_VISION_1                        = 0,
    SAY_GARONA_7_VISION_1                       = 0,
    SAY_YOGGSARON_8_VISION_1                    = 3,
    // Lichking
    SAY_LICHKING_1_VISION_2                     = 0,
    SAY_CHAMP_2_VISION_2                        = 0,
    SAY_CHAMP_3_VISION_2                        = 1,
    SAY_LICHKING_4_VISION_2                     = 1,
    SAY_YOGGSARON_5_VISION_2                    = 6,
    SAY_YOGGSARON_6_VISION_2                    = 7,
    // Dragon Soul
    SAY_NELTHARION_1_VISION_3                   = 0,
    SAY_YSERA_2_VISION_3                        = 0,
    SAY_NELTHARION_3_VISION_3                   = 1,
    SAY_MALYGOS_4_VISION_3                      = 0,
    SAY_YOGGSARON_5_VISION_3                    = 8,
};

enum Events
{
    ACHIEV_TIMED_START_EVENT                    = 21001, // Dont Need this
    // PHASE SARA
    EVENT_SUMMON_GUARDIAN                       = 1,
    EVENT_SARA_HELP,
    // PHASE BRAIN INTRO
    EVENT_BRAIN_INTRO_1,
    EVENT_BRAIN_INTRO_2,
    EVENT_BRAIN_INTRO_3,
    EVENT_BRAIN_INTRO_4,
    EVENT_BRAIN_TRANSFORMATION,
    // PHASE BRAIN
    EVENT_PSYCHOSIS,
    EVENT_MALADY,
    EVENT_BRAIN_LINK,
    EVENT_DEATH_RAYS,
    EVENT_SPAWN_CRUSHER,
    EVENT_SPAWN_CONSTRICTOR,
    EVENT_SPAWN_CORRUPTOR,
    // PHASE YOGG
    EVENT_SUMMON_IMMORTAL_GUARDIAN,
    EVENT_LUNATIC_GAZE,
    EVENT_DEAFENING_ROAR,
    EVENT_SHADOW_BEACON,
    EVENT_ACHIEVEMENT_TIMER_RAN_OUT
};

enum Data
{
//  DATA_PORTAL_PHASE                           = 1, in ulduar.h
    DATA_SARA_PHASE                             = 2,
};

enum Achievments
{
    // Sara
    ACHIEVMENT_KISS_AND_MAKE_UP_10              = 3009,
    ACHIEVMENT_KISS_AND_MAKE_UP_25              = 3011,
    ACHIEVMENT_DRIVE_ME_CRAZY_10                = 3008,
    ACHIEVMENT_DRIVE_ME_CRAZY_25                = 3010,
    ACHIEVMENT_HE_S_NOT_GETTING_ANY_OLDER_10    = 3012,
    ACHIEVMENT_HE_S_NOT_GETTING_ANY_OLDER_25    = 3013,
    ACHIEVMENT_COMING_OUT_OF_THE_WALLS_10       = 3014,
    ACHIEVMENT_COMING_OUT_OF_THE_WALLS_25       = 3017,

    // Heroic Mode
    ACHIEVMENT_THREE_LIGHTS_IN_THE_DARKNESS_10  = 3157,
    ACHIEVMENT_TWO_LIGHTS_IN_THE_DARKNESS_10    = 3141,
    ACHIEVMENT_ONE_LIGHTS_IN_THE_DARKNESS_10    = 3158,
    ACHIEVMENT_ALONE_IN_THE_DARKNESS_10         = 3159,
    ACHIEVMENT_THREE_LIGHTS_IN_THE_DARKNESS_25  = 3161,
    ACHIEVMENT_TWO_LIGHTS_IN_THE_DARKNESS_25    = 3162,
    ACHIEVMENT_ONE_LIGHTS_IN_THE_DARKNESS_25    = 3163,
    ACHIEVMENT_ALONE_IN_THE_DARKNESS_25         = 3164,
};

enum NPCs
{
    // All Phaes
    ENTRY_SANITY_WELL                           = 33991,
    // Phase 1
    ENTRY_SARA                                  = 33134,
    ENTRY_OMINOUS_CLOUD                         = 33292,
    ENTRY_GUARDIAN_OF_YOGG_SARON                = 33136,
    // Phase 2
    ENTRY_YOGG_SARON                            = 33288,
    ENTRY_BRAIN_OF_YOGG_SARON                   = 33890,
    ENTRY_DEATH_RAY                             = 33881,
    ENTRY_DEATH_ORB                             = 33882,
    //  Tentacles
    ENTRY_CRUSHER_TENTACLE                      = 33966,
    ENTRY_CORRUPTOR_TENTACLE                    = 33985,
    ENTRY_CONSTRICTOR_TENTACLE                  = 33983,
    //  Brain
    //  All Vision
    ENTRY_BRAIN_PORTAL                          = 34072,
    ENTRY_INFULENCE_TENTACLE                    = 33943,
    ENTRY_LAUGHING_SKULL                        = 33990,
    //  Vision Dragon Soul
    ENTRY_RUBY_CONSORT                          = 33716, // 2 on Roomexit (l & r)
    ENTRY_OBSIDIAN_CONSORT                      = 33720, // 2 on End of the Room (l & r)
    ENTRY_EMERAL_CONSORT                        = 33719, // 2 on North West of the room
    ENTRY_AZURE_CONSORT                         = 33717, // 2 on North East of the room
    ENTRY_NELTHARION_VISION                     = 33523,
    ENTRY_YSERA_VISION                          = 33495,
    ENTRY_MALYGOS_VISION                        = 33535,
    //  Vision Llane
    ENTRY_SUIT_OF_ARMOR                         = 33433, // around the Room
    ENTRY_GARONA_VISION                         = 33436,
    ENTRY_KING_LLANE                            = 33437,
    //  Vision Lich King
    ENTRY_DEATHSWORN_ZEALOT                     = 33567, // 3 Groups of 3 ... left middle right
    ENTRY_LICHKING_VISION                       = 33441,
    ENTRY_IMMOLATED_CHAMPION_VISION             = 33442,
    ENTRY_TURNED_CHAMPION_VISION                = 33962,
    // Phase 3
    ENTRY_IMMORTAL_GUARDIAN                     = 33988,
    ENTRY_IMMORTAL_GURDIAN_MARKED               = 36064,

    OBJECT_THE_DRAGON_SOUL                      = 194462,
    OBJECT_FLEE_TO_SURFACE                      = 194625,
};

enum Constants
{
    MAX_BRAIN_ROOM_10                  = 4,
    MAX_BRAIN_ROOM_25                  = 10,
    MAX_NOVA_HITS                      = 8,
    MAX_LLIANE_TENTACLE_SPAWNS         = 8,
    MAX_LICHKING_TENTACLE_SPAWNS       = 9,
    MAX_DRAGONSOUL_TENTACLE_SPAWNS     = 8,
    COUNT_SPAWN_POSTIONS               = 46,
    SEC                                = 1000,
    MIN                                = 60*SEC,
    ENRAGE                             = 15*MIN,
};

enum Actions
{
    ACTION_NOVA_HIT                         = 0,
    ACTION_ACTIVATE_CLOUDS,
    ACTION_DEACTIVATE_CLOUDS,
    ACTION_PORTAL_TO_MADNESS_STORMWIND,
    ACTION_PORTAL_TO_MADNESS_DRAGON,
    ACTION_PORTAL_TO_MADNESS_LICHKING,
    ACTION_BRAIN_UNDER_30_PERCENT,
    ACTION_YOGGSARON_KILLED,
    ACTION_DEATH_RAY_MOVE,
    ACTION_USED_MINDCONTROL,
    ACTION_MODIFY_SANITY,
    ACTION_SPAWN_GUARDIAN_OF_YOGG_SARON,
    ACTION_ACTIVATE_KEEPER,
    ACTION_DEACTIVATE_KEEPER,
    ACTION_HIT_BY_VALANYR,
    ACTION_GUARDIAN_KILLED_ACHIEVMENT_PROGRESS,
};

enum Spells
{
    SPELL_IN_THE_MAWS_OF_THE_OLD_GOD            = 64184,
    //All Phases
    // Keeper Freya
    SPELL_RESILIENCE_OF_NATURE                  = 62670,
    SPELL_SANITY_WELL                           = 64170, // Send Script Effect ... Scripted in Keeper Script (Summon)
    SPELL_SANITY_WELL_DEBUFF                    = 64169, // Damage Debuff if you stand in the well
    // Keeper Thorim
    SPELL_FURY_OF_THE_STORM                     = 62702,
    SPELL_TITANIC_STORM                         = 64171, // Triggers 64172
    SPELL_TITANIC_STORM_DUMMY                   = 64172, // Dummy Spell ... kills weakend guardians
    // Keeper Mimiron
    SPELL_SPEED_OF_INVENTION                    = 62671,
    SPELL_DESTABILIZATION_MATRIX                = 65210, // No AoE ... Target every Tentakle or Guardian Random
    // Keeper Hodir
    SPELL_FORTITUDE_OF_FROST                    = 62650,
    SPELL_HODIRS_PROTECTIVE_GAZE                = 64174,
    SPELL_FLASH_FREEZE                          = 64175,
    SPELL_FLASH_FREEZE_COOLDOWN                 = 64176,
    // Sanity
    SPELL_SANITY                                = 63050,
    SPELL_INSANE                                = 63120, // MindControl
    SPELL_INSANE_2                              = 64464, // Let all player looks like FacelessOnes
    //Phase 1:
    SPELL_SUMMON_GUARDIAN                       = 63031,
    SPELL_OMINOUS_CLOUD                         = 60977,
    SPELL_OMINOUS_CLOUD_TRIGGER                 = 60978,
    SPELL_OMINOUS_CLOUD_DAMAGE                  = 60984,
    SPELL_OMINOUS_CLOUD_EFFECT                  = 63084,
    //  Sara
    SPELL_SARAS_FERVOR                          = 63138, // On Player
    SPELL_SARAS_BLESSING                        = 63134, // On Player
    SPELL_SARAS_ANGER                           = 63147, // On "Enemy"
    //  Guardians of Yogg-Saron
    SPELL_DARK_VOLLEY                           = 63038, // Needs Sanity Scripting
    SPELL_SHADOW_NOVA                           = 62714, // On Death
    SPELL_SHADOW_NOVA_H                         = 65719, // On Death
    //SPELL_DOMINATE_MIND                         = 63042, // Removed by blizz, Needs Sanity Scripting
    //SPELL_DOMINATE_MIND_PROTECTION              = 63759, // Dummy aura for target of SPELL_DOMINATE_MIND excluding
    //Phase 2:
    SPELL_SARA_TRANSFORMATION                   = 65157,
    //  Sara - She spams Psychosis without pause on random raid members unless she casts something else. The other three abilities each have a 30 second cooldown, and are used randomly when available.
    SPELL_PSYCHOSIS_10                          = 63795, // Needs Sanity Scripting
    SPELL_PSYCHOSIS_25                          = 65301, // Needs Sanity Scripting

    SPELL_BRAIN_LINK                            = 63802, // Only Apply Aura
    SPELL_BRAIN_LINK_DAMAGE                     = 63803, // Needs Sanity Scripting
    SPELL_BRAIN_LINK_DUMMY                      = 63804, // Dummy Effekt
    SPELL_MALADY_OF_MIND                        = 63830, // Trigger 63881 on remove
    SPELL_MALADY_OF_MIND_TRIGGER                = 63881,
    SPELL_DEATH_RAY_SUMMON                      = 63891, // Summond 1 33882 (controll deathray)
    SPELL_DEATH_RAY_DAMAGE_VISUAL               = 63886, // Damage visual Effekt of 33881 (triggered with periodic)
    SPELL_DEATH_RAY_PERIODIC                    = 63883, // Trigger 63884
    SPELL_DEATH_RAY_ORIGIN_VISUAL               = 63893, // Visual Effekt of 33882
    SPELL_DEATH_RAY_WARNING_VISUAL              = 63882,
    SPELL_SARA_SHADOWY_BARRIER                  = 64775,
    // Tentacle
    SPELL_ERUPT                                 = 64144,
    SPELL_TENTACLE_VOID_ZONE                    = 64017,
    //  Crusher Tentacle
    SPELL_DIMISH_POWER                          = 64145, // Aura around Caster
    SPELL_FOCUS_ANGER                           = 57688, // Trigger 57689
    SPELL_CRUSH                                 = 64146,
    //  Corruptor Tentacle
    SPELL_DRAINING_POISON                       = 64152,
    SPELL_BLACK_PLAGUE                          = 64153, // Trigger 64155
    SPELL_APATHY                                = 64156,
    SPELL_CURSE_OF_DOOM                         = 64157,
    //  Constrictor Tentacles
    SPELL_LUNGE                                 = 64123, // Need Vehicle
    SPELL_SQUEEZE                               = 64125, // Until killed (64126 with spelldiff)
    //  Influence Tentacle
    SPELL_GRIM_REPRISAL                         = 63305, // Dummy aura
    SPELL_GRIM_REPRISAL_DAMAGE                  = 64039, // Damage 1
    //  Yogg-Saron
    SPELL_EXTINGUISH_ALL_LIFE                   = 64166, // After 15 Minutes
    SPELL_SHADOWY_BARRIER                       = 63894,
    SPELL_SUMMON_CRUSHER_TENTACLE               = 64139,
    SPELL_SUMMON_CORRUPTOR_TENTACLE             = 64143,
    SPELL_SUMMON_CONSTRICTOR_TENTACLES          = 64133,
    //  Brain of Yogg-Sauron
    SPELL_INDUCE_MADNESS                        = 64059,
    SPELL_SHATTERED_ILLUSIONS                   = 64173,
    SPELL_BRAIN_HURT_VISUAL                     = 64361,
    //  Mind Portal
    SPELL_TELEPORT                              = 64027, // Not Used
    SPELL_ILLUSION_ROOM                         = 63988, // Must be removed if not in Room
    //  Lauthing Skull
    SPELL_LS_LUNATIC_GAZE_EFFECT                = 64168,
    //Phase 3:
    SPELL_YOGG_SARON_TRANSFORMATION             = 63895,
    //  Yogg-Saron
    SPELL_DEAFENING_ROAR                        = 64189, // Cast only on 25plr Mode and only with 0-3 Keepers active
    SPELL_SUMMON_IMMORTAL_GUARDIAN              = 64158,

    SPELL_SHADOW_BEACON                         = 64465, // Casted on Immortal Guardian - trigger 64466
    SPELL_EMPOWERING_SHADOWS_SCRP_1             = 64466, // Script Effekt ... as index 64467
    SPELL_EMPOWERING_SHADOWS_SCRP_2             = 64467, // I dont need this
    SPELL_EMPOWERING_SHADOWS_HEAL_10            = 64468, // 10plr Heal
    SPELL_EMPOWERING_SHADOWS_HEAL_25            = 64486, // 20plr Heal

    SPELL_LUNATIC_GAZE                          = 64163, // Triggers 4 Times 64164
    SPELL_LUNATIC_GAZE_EFFECT                   = 64164, // Needs Sanity Scripting

    //  Immortal Guardian - under 1% no damage
    SPELL_DRAIN_LIFE_10                                    = 64159,
    SPELL_DRAIN_LIFE_25                                    = 64160,
    SPELL_SUMMON_GUARDIAN_VISUAL                           = 51347,

    SPELL_WEAKENED                                         = 64162, // Dummy on low health for Titan Storm and Shadow Beacon
    SPELL_EMPOWERED                                        = 65294, // stacks 9 times ... on 100% hp it have 9 stacks .. but with <10% it havent any

    // Misc
    SPELL_SPIRIT_OF_REDEMPTION                             = 27827
};

enum MindlessSpell
{
    BRAIN_LINK                  = 0,
    MALADY_OF_MIND              = 1,
    DEATH_RAY                   = 2,
};

enum BossPhase
{
    PHASE_NONE                  = 0,
    PHASE_SARA,
    PHASE_BRAIN_INTRO,
    PHASE_BRAIN,
    PHASE_YOGG,
};

enum BrainEventPhase
{
    PORTAL_PHASE_KING_LLIANE    = 0,
    PORTAL_PHASE_DRAGON_SOUL    = 1,
    PORTAL_PHASE_LICH_KING      = 2,
    PORTAL_PHASE_BRAIN_NONE     = 3,
};

const Position innerbrainLocation[3] =
{
    {1944.87f,  37.22f, 239.7f, (0.66f*static_cast<float>(M_PI))}, //King Lliane
    {2045.97f, -25.45f, 239.8f, 0           }, //Dragon Soul
    {1953.41f, -73.73f, 240.0f, (1.33f*static_cast<float>(M_PI))}, //Lich King
};

const Position brainPortalLocation[10] =
{
    {1970.48f,   -9.75f, 325.5f, 0},
    {1992.76f,  -10.21f, 325.5f, 0},
    {1995.53f,  -39.78f, 325.5f, 0},
    {1969.25f,  -42.00f, 325.5f, 0},
    {1960.62f,  -32.00f, 325.5f, 0},
    {1981.98f,  -5.69f,  325.5f, 0},
    {1982.78f,  -45.73f, 325.5f, 0},
    {2000.66f,  -29.68f, 325.5f, 0},
    {1999.88f,  -19.61f, 325.5f, 0},
    {1961.37f,  -19.54f, 325.5f, 0},
};

const Position kingLlaneTentacleLocation[MAX_LLIANE_TENTACLE_SPAWNS] =
{
    {1949.82f,   50.77f, 239.70f, (0.8f*static_cast<float>(M_PI))},
    {1955.24f,   72.08f, 239.70f, (1.04f*static_cast<float>(M_PI))},
    {1944.51f,   90.88f, 239.70f, (1.3f*static_cast<float>(M_PI))},
    {1923.46f,   96.71f, 239.70f, (1.53f*static_cast<float>(M_PI))},
    {1904.14f,   85.99f, 239.70f, (1.78f*static_cast<float>(M_PI))},
    {1898.78f,   64.62f, 239.70f, (0.05f*static_cast<float>(M_PI))},
    {1909.74f,   45.15f, 239.70f, (0.31f*static_cast<float>(M_PI))},
    {1931.01f,   39.85f, 239.70f, (0.55f*static_cast<float>(M_PI))},
};

const Position dragonSoulTentacleLocation[MAX_DRAGONSOUL_TENTACLE_SPAWNS] =
{
    // ENTRY_RUBY_CONSORT
    {2061.44f,  -11.83f, 239.80f, 0},
    {2059.87f,  -37.77f, 239.80f, 0},
    // ENTRY_AZURE_CONSORT
    {2105.42f,  -71.81f, 239.80f, (0.51f*static_cast<float>(M_PI))},
    {2131.29f,  -60.90f, 239.80f, (0.68f*static_cast<float>(M_PI))},
    // ENTRY_OBSIDIAN_CONSORT
    {2147.62f,  -42.06f, 239.80f, static_cast<float>(M_PI)},
    {2147.62f,   -6.91f, 239.80f, static_cast<float>(M_PI)},
    //ENTRY_EMERAL_CONSORT
    {2125.64f,   17.08f, 239.80f, (1.35f*static_cast<float>(M_PI))},
    {2109.06f,   22.69f, 239.80f, (1.42f*static_cast<float>(M_PI))},
};

const Position lichKingTentacleLocation[MAX_LICHKING_TENTACLE_SPAWNS] =
{
    {1944.20f,  -136.72f, 240.00f, (1.35f*static_cast<float>(M_PI))},
    {1950.06f,  -139.36f, 240.00f, (1.35f*static_cast<float>(M_PI))},
    {1948.23f,  -129.44f, 240.00f, (1.35f*static_cast<float>(M_PI))},

    {1920.34f,  -136.38f, 240.00f, (1.35f*static_cast<float>(M_PI))},
    {1914.44f,  -132.35f, 240.00f, (1.35f*static_cast<float>(M_PI))},
    {1919.69f,  -129.47f, 240.00f, (1.35f*static_cast<float>(M_PI))},

    {1909.49f,  -111.84f, 240.00f, (1.35f*static_cast<float>(M_PI))},
    {1902.02f,  -107.69f, 240.00f, (1.35f*static_cast<float>(M_PI))},
    {1910.28f,  -102.96f, 240.00f, (1.35f*static_cast<float>(M_PI))},
};

const Position brainLocation = {1980.01f, -25.36f, 260.00f, static_cast<float>(M_PI)};
const Position saraLocation = {1980.28f, -25.58f, 325.00f, static_cast<float>(M_PI)};

const Position innerBrainTeleportLocation[3] =
{
    {2001.40f, -59.66f, 245.07f, 0},
    {1941.61f, -25.88f, 244.98f, 0},
    {2001.18f,  9.409f, 245.24f, 0},
};

const Position freyaSanityWellLocation[5] =
{
    {1901.21f, -48.69f, 332.00f, 0},
    {1901.90f,  -2.78f, 332.00f, 0},
    {1993.58f,  45.56f, 332.00f, 0},
    {1985.87f, -91.10f, 330.20f, 0},
    {2040.12f, -39.16f, 329.00f, 0},
};

const Position kingLlaneSkullLocation[3] =
{
    {1929.41f,  45.27f, 239.70f, 0},
    {1902.31f,  72.53f, 239.70f, 0},
    {1925.10f,  91.52f, 239.70f, 0},
};

const Position lichkingSkullLocation[3] =
{
    {1955.12f, -111.96f,  240.00f, 0},
    {1919.55f, -131.94f,  240.00f, 0},
    {1921.03f,  -93.46f,  240.00f, 0},
};

const Position dragonSoulSkullLocation[4] =
{
    {2115.89f,   4.63f,  239.80f, 0},
    {2080.65f,  37.47f,  244.3f,  0},
    {2170.20f, -33.31f,  244.3f,  0},
    {2090.49f, -57.40f,  239.8f,  0},
};

const Position eventNPCLocation[9] =
{
    { 1928.23f, 66.81f, 242.40f, 5.207f },   // King Llane
    { 1929.78f, 63.60f, 242.40f, 2.124f },   // Garona - kneeling
    { 2103.87f, -13.13f, 242.65f, 4.692f },  // Ysera
    { 2118.68f, -25.56f, 242.65f, static_cast<float>(M_PI)},     // Neltharion
    { 2095.87f, -34.47f, 242.65f, 0.836f },  // Malygos
    { 2104.61f, -25.36f, 242.65f, 0.0f },    // The Dragon Soul
    { 1903.41f, -160.21f, 239.99f, 1.114f }, // Immolated Champion
    { 1909.31f, -155.88f, 239.99f, 4.222f }, // Turned Champion
    { 1907.02f, -153.92f, 239.99f, 4.187f }, // The Lich King

};

const Position TentacleAndGuardianSpawnPos[COUNT_SPAWN_POSTIONS] =
{
    {1936.79f, -18.7255f, 327.996f, 0.860526f},
    {1948.27f, -8.83564f, 326.29f, 0.707373f},
    {1964.06f, -1.32862f, 325.336f, 0.393214f},
    {1992.13f, 0.811447f, 325.831f, 5.81246f},
    {2010.76f, -17.9363f, 326.306f, 4.81501f},
    {2005.09f, -45.5907f, 326.355f, 4.1435f},
    {1976.67f, -59.7656f, 326.705f, 3.19317f},
    {1956.67f, -53.39f, 326.261f, 2.14859f},
    {1946.36f, -39.702f, 326.604f, 0.63277f},
    {1930.2f, -52.8958f, 328.963f, 5.67502f},
    {1944.04f, -63.3952f, 328.377f, 5.6711f},
    {1963.46f, -75.1063f, 328.826f, 5.80854f},
    {1977.93f, -76.9872f, 329.333f, 6.15804f},
    {1997.53f, -71.4107f, 328.803f, 0.404998f},
    {2014.2f, -56.7802f, 328.18f, 0.734865f},
    {2030.38f, -35.3695f, 328.38f, 0.935142f},
    {2026.09f, -10.8661f, 328.16f, 2.01899f},
    {2008.44f, 10.1029f, 328.144f, 2.27032f},
    {1986.64f, 25.9668f, 329.396f, 2.7337f},
    {1963.16f, 18.9856f, 328.54f, 3.44449f},
    {1939.78f, 8.50906f, 329.087f, 3.66833f},
    {1918.35f, -10.1883f, 329.453f, 3.94714f},
    {1911.74f, -27.0326f, 330.189f, 4.3477f},
    {1915.63f, -43.3295f, 329.745f, 5.2077f},
    {1966.67f, -45.9128f, 324.824f, 5.94598f},
    {1992.66f, -43.8319f, 324.89f, 0.141882f},
    {2001.68f, -30.2013f, 324.89f, 0.986185f},
    {1997.35f, -3.57648f, 325.462f, 1.86583f},
    {1974.87f, -6.89319f, 324.89f, 3.36987f},
    {1958.17f, -16.2441f, 325.252f, 3.74293f},
    {1926.08f, -28.0095f, 328.176f, 1.67734f},
    {1935.51f, -2.25165f, 327.829f, 0.73093f},
    {1977.07f, 15.7899f, 327.931f, 0.20864f},
    {2019.8f, -29.8965f, 327.335f, 4.654f},
    {1987.46f, -63.2479f, 327.574f, 3.64476f},
    {1903.32f, -47.7364f, 331.73f, 0.954776f},
    {1908.98f, -0.118233f, 331.501f, 1.03332f},
    {1958.93f, 23.3229f, 329.257f, 0.691668f},
    {1988.75f, 43.0746f, 331.62f, 6.0363f},
    {2005.46f, 18.4719f, 329.25f, 5.91064f},
    {2023.24f, 7.44369f, 329.302f, 5.30196f},
    {2045.53f, -42.0902f, 329.524f, 4.28487f},
    {2015.37f, -70.2114f, 329.494f, 4.16313f},
    {1995.58f, -88.9594f, 330.012f, 2.77691f},
    {1972.1f, -94.118f, 330.409f, 4.02569f},
    {1920.17f, -65.6125f, 330.714f, 3.04394f},
};

struct eventNPC
{
    uint32 entry;
    ObjectGuid guid;
};

struct eventSpeech
{
    uint32 npc_entry;
    int32 speech_entry;
    uint32 next_speech;
    bool isSpeaking;
};

const uint32  eventNPCEntrys[9] =
{
    ENTRY_KING_LLANE,
    ENTRY_GARONA_VISION,
    ENTRY_YSERA_VISION,
    ENTRY_NELTHARION_VISION,
    ENTRY_MALYGOS_VISION,
    OBJECT_THE_DRAGON_SOUL,
    ENTRY_IMMOLATED_CHAMPION_VISION,
    ENTRY_TURNED_CHAMPION_VISION,
    ENTRY_LICHKING_VISION,
};

const eventSpeech eventNPCSpeaching[19] =
{
    {ENTRY_GARONA_VISION,SAY_GARONA_1_VISION_1,3000, true},
    {ENTRY_GARONA_VISION,SAY_GARONA_2_VISION_1,5000, true},
    {ENTRY_GARONA_VISION,SAY_GARONA_3_VISION_1,5000, true},
    {ENTRY_YOGG_SARON,SAY_YOGGSARON_4_VISION_1,3000, true},
    {ENTRY_YOGG_SARON,SAY_YOGGSARON_5_VISION_1,3000, true},
    {ENTRY_KING_LLANE,SAY_LLANE_6_VISION_1,5000, true},
    {ENTRY_GARONA_VISION,SAY_GARONA_7_VISION_1,5000, true},
    {ENTRY_YOGG_SARON,SAY_YOGGSARON_8_VISION_1,5000, false},

    {ENTRY_LICHKING_VISION,SAY_LICHKING_1_VISION_2,5000, true},/*8*/
    {ENTRY_IMMOLATED_CHAMPION_VISION,SAY_CHAMP_2_VISION_2,5000, true},
    {ENTRY_IMMOLATED_CHAMPION_VISION,SAY_CHAMP_3_VISION_2,5000, true},
    {ENTRY_LICHKING_VISION,SAY_LICHKING_4_VISION_2,5000, true},
    {ENTRY_YOGG_SARON,SAY_YOGGSARON_5_VISION_2,5000, true},
    {ENTRY_YOGG_SARON,SAY_YOGGSARON_6_VISION_2,5000, false},

    {ENTRY_NELTHARION_VISION,SAY_NELTHARION_1_VISION_3,5000, true},/*14*/
    {ENTRY_YSERA_VISION,SAY_YSERA_2_VISION_3,5000, true},
    {ENTRY_NELTHARION_VISION,SAY_NELTHARION_3_VISION_3,5000, true},
    {ENTRY_MALYGOS_VISION,SAY_MALYGOS_4_VISION_3,5000, true},
    {ENTRY_YOGG_SARON,SAY_YOGGSARON_5_VISION_3,5000, false},
};

bool isPlayerInBrainRoom(const Player* player)
{
    return player->GetPositionZ() < 300.0f;
}

class YoggSaronTargetSelector
{
    public:
        YoggSaronTargetSelector(bool brain, bool mind, uint32 spell, Unit* excludeTargetsNearYogg, uint32 min_sanity) :
            brainroom(brain), mindcontrol(mind), spellId(spell), excludeTargetsNearYogg(excludeTargetsNearYogg), sanityLimit(min_sanity) {}

    bool operator() (Unit* unit)
    {
        if (unit->ToPlayer() && unit->ToPlayer()->IsGameMaster())
            return true;

        if (spellId && unit->HasAura(spellId))
            return true;

        if (!brainroom && unit->ToPlayer() && isPlayerInBrainRoom(unit->ToPlayer()))
            return true;

        if (!mindcontrol && (unit->HasAuraType(SPELL_AURA_AOE_CHARM) || unit->HasAuraType(SPELL_AURA_MOD_CHARM)))
            return true;

        if (excludeTargetsNearYogg && unit->GetDistance(excludeTargetsNearYogg) < 6.0f)
            return true;

        // if sanity limit is given, check if enough stacks of SPELL_SANITY. Too few stacks -> return true
        if ((sanityLimit < 100u) && unit->GetAura(SPELL_SANITY) && unit->GetAura(SPELL_SANITY)->GetStackAmount() < sanityLimit)
            return true;

        return false;
    }

    private:
        bool brainroom;
        bool mindcontrol;
        uint32 spellId;
        Unit* excludeTargetsNearYogg;
        uint32 sanityLimit;
};

Unit* SelectRandomPlayer(Unit* searcher, float range, bool allowBrain = false, bool allowMind = false, uint32 aura = 0, bool excludeTargetsNearYogg = false, uint32 min_sanity = 100u)
{
    std::list<Player*> players;
    searcher->GetPlayerListInGrid(players, range);
    Unit* yogg = NULL;
    if (excludeTargetsNearYogg)
        yogg = searcher->FindNearestCreature(ENTRY_YOGG_SARON, 200.0f);

    // Try to find macthing conditions
    players.remove_if (YoggSaronTargetSelector(allowBrain, allowMind, aura, yogg, min_sanity));

    if (players.empty() && (min_sanity < 100u))
        // we found no player fulfilling the sanity limit, so try it without the limit
        return SelectRandomPlayer(searcher, range, allowBrain, allowMind, aura, yogg, 100u);

    if (players.empty())
        return NULL;

    std::list<Player*>::iterator itr = players.begin();
    std::advance(itr, urand(0, players.size()-1));

    return (*itr);
}

class boss_sara : public CreatureScript
{
public:
    boss_sara() : CreatureScript("boss_sara") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_saraAI (creature);
    }

    struct boss_saraAI : public BossAI
    {
        boss_saraAI(Creature* creature) : BossAI(creature, BOSS_YOGGSARON), summons(me)
        {
            instance = creature->GetInstanceScript();
            me->SetCanFly(true);
            randomYellTimer = urand(10*SEC, 20*SEC);
            phase = PHASE_NONE;
        }

        InstanceScript* instance;
        SummonList summons;

        BossPhase phase;
        uint32 duration;

        bool cloudsSpawned;
        uint32 cloudSpawning;

        ObjectGuid guidYogg;
        ObjectGuid guidYoggBrain;
        std::list<ObjectGuid> guidEventTentacles;
        std::list<ObjectGuid> guidEventSkulls;
        std::list<eventNPC> listeventNPCs;

        bool usedMindcontroll;


        // Phase 1
        uint32 summonedGuardiansCount;
        uint32 randomYellTimer;
        uint32 novaHitCounter;

        // Phase 2
        Actions lastBrainAction;
        uint32 madnessTimer;
        uint32 brainEventsCount;
        BrainEventPhase currentBrainEventPhase;
        bool isEventSpeaking;
        uint32 eventSpeakingPhase;
        uint32 eventSpeakingTimer;

        // Phase 3
        uint32 summonedImmortalGuardiansCount;

        //Achievment They're coming out of the walls
        bool counterStarted;
        uint32 deadGuardianCounter;

        void CloudHandling(bool remove)
        {
            std::list<Creature*> cloudList;
            me->GetCreatureListWithEntryInGrid(cloudList,ENTRY_OMINOUS_CLOUD, 500.0f);
            if (!cloudList.empty())
            {
                for (std::list<Creature*>::iterator itr = cloudList.begin(); itr != cloudList.end(); itr++)
                {
                    if (remove) (*itr)->AI()->DoAction(ACTION_DEACTIVATE_CLOUDS);
                    else (*itr)->AI()->DoAction(ACTION_ACTIVATE_CLOUDS);
                }
            }
        }

        void KeeperHandling(bool setActive)
        {

            Actions action = setActive ? ACTION_ACTIVATE_KEEPER : ACTION_DEACTIVATE_KEEPER;

            if (Creature* keeper = me->FindNearestCreature(NPC_KEEPER_YOGGROOM_MIMIRON, 500.0f))
                keeper->AI()->DoAction(action);

            if (Creature* keeper = me->FindNearestCreature(NPC_KEEPER_YOGGROOM_FREYA, 500.0f))
                keeper->AI()->DoAction(action);

            if (Creature* keeper = me->FindNearestCreature(NPC_KEEPER_YOGGROOM_THORIM, 500.0f))
                keeper->AI()->DoAction(action);

            if (Creature* keeper = me->FindNearestCreature(NPC_KEEPER_YOGGROOM_HODIR, 500.0f))
                keeper->AI()->DoAction(action);
        }


        void Reset()
        {
            //Correct achievement progress for "coming out of the walls" relies on this (phase = PHASE_NONE) being before despawning guardians
            //see npc_guardian_of_yogg_saron_AI::justDied() for use of this fact
            phase = PHASE_NONE;
            events.Reset();
            me->CombatStop();

            KeeperHandling(false);

            summons.DespawnAll();
            guidEventTentacles = std::list<ObjectGuid>();
            guidEventSkulls = std::list<ObjectGuid>();
            listeventNPCs = std::list<eventNPC>();

            me->InterruptNonMeleeSpells(false);
            // Zurueck an Home ... muss nicht sein ist aber besser so
            Position pos = me->GetHomePosition();
            me->NearTeleportTo(pos.m_positionX, pos.m_positionY, pos.m_positionZ, pos.m_orientation);
            me->RemoveAurasDueToSpell(SPELL_SHATTERED_ILLUSIONS);
            me->RemoveAurasDueToSpell(SPELL_SARA_SHADOWY_BARRIER);

            // Remove Random MoveMaster
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveIdle();

            // Reset Display
            me->setFaction(35);
            me->SetVisible(true);
            me->RemoveAurasDueToSpell(SPELL_SARA_TRANSFORMATION);
            // Reset Health
            me->SetHealth(me->GetMaxHealth());

            // Respawn Clouds
            cloudSpawning = 2*SEC;
            cloudsSpawned = false;

            SetSanityAura(true);
            // Reset HitCounter Phase 1
            novaHitCounter = MAX_NOVA_HITS;
            currentBrainEventPhase = PORTAL_PHASE_BRAIN_NONE;

            // Spawn Yoggy if not spawned
            if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                yogg->DespawnOrUnsummon();
            if (Creature* yogg = DoSummon(ENTRY_YOGG_SARON, saraLocation, 0, TEMPSUMMON_MANUAL_DESPAWN))
            {
                yogg->SetLootMode(LOOT_MODE_DEFAULT);
                yogg->SetCanFly(true);

                Position yoggPos = yogg->GetHomePosition();
                yogg->NearTeleportTo(yoggPos.GetPositionX(), yoggPos.GetPositionY(), yoggPos.GetPositionZ(), yoggPos.GetOrientation());
            }

            if (Creature* yoggbrain = ObjectAccessor::GetCreature(*me, guidYoggBrain))
                yoggbrain->DespawnOrUnsummon();
            DoSummon(ENTRY_BRAIN_OF_YOGG_SARON, brainLocation, 0, TEMPSUMMON_MANUAL_DESPAWN);

            if (instance)
                instance->SetBossState(BOSS_YOGGSARON, NOT_STARTED);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
                instance->SetBossState(BOSS_YOGGSARON, DONE);

            SetSanityAura(true);
            summons.DespawnAll();
        }

        void EnterCombat(Unit* /*who*/)
        {
            duration = 0;
            summonedGuardiansCount = 0;
            summonedImmortalGuardiansCount = 0;
            madnessTimer = 60*SEC;
            usedMindcontroll = false;
            resetAchievementProgressComingOutOfTheWalls();

            events.ScheduleEvent(EVENT_SUMMON_GUARDIAN, 10*SEC, 0, PHASE_SARA);
            events.ScheduleEvent(EVENT_SARA_HELP, urand(5*SEC, 6*SEC), 0, PHASE_SARA);

            if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                DoZoneInCombat(yogg);

            if (instance)
                instance->SetBossState(BOSS_YOGGSARON, IN_PROGRESS);

            lastBrainAction = Actions(0);
        }

        void ReceiveEmote(Player* player, uint32 emote)
        {
            if (phase >= PHASE_BRAIN)
            {
                if (emote == TEXT_EMOTE_KISS)
                {
                    if (player->HasAchieved(RAID_MODE(ACHIEVMENT_KISS_AND_MAKE_UP_10, ACHIEVMENT_KISS_AND_MAKE_UP_25)))
                        return;

                    if (me->IsWithinLOS(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ()) && me->IsWithinDistInMap(player,30.0f))
                    {
                        if (AchievementEntry const *achievKissAndMakeUp = sAchievementStore.LookupEntry(RAID_MODE(ACHIEVMENT_KISS_AND_MAKE_UP_10, ACHIEVMENT_KISS_AND_MAKE_UP_25)))
                            player->CompletedAchievement(achievKissAndMakeUp);
                    }
                }
            }
        }

        void DamageTaken(Unit* dealer, uint32 &damage)
        {
            if (dealer->GetEntry() == ENTRY_GUARDIAN_OF_YOGG_SARON)
            {
                damage = 0;
                return;
            }

            if (damage > me->GetHealth())
                damage = me->GetHealth()-1;
        }

        void SpellHitTarget(Unit* target, const SpellInfo* spell)
        {
            if (target && target->ToPlayer())
            {
                switch (spell->Id)
                {
                    case SPELL_PSYCHOSIS_10:
                        ModifySanity(target->ToPlayer(), -9);
                        break;
                    case SPELL_PSYCHOSIS_25:
                        ModifySanity(target->ToPlayer(), -12);
                        break;
                    case SPELL_MALADY_OF_MIND:
                        ModifySanity(target->ToPlayer(),-3);
                        break;
                }
            }
        }

        void MovementInform(uint32 type, uint32 i)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (i == 1)
            {
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MoveIdle();
            }
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_NOVA_HIT:
                    if (phase != PHASE_SARA)
                        return;
                    novaHitCounter--;
                    if (novaHitCounter <= 0)
                        SetPhase(PHASE_BRAIN_INTRO);
                    else me->ModifyHealth(-25000);

                    break;
                case ACTION_BRAIN_UNDER_30_PERCENT:
                    SetPhase(PHASE_YOGG);
                    break;

                case ACTION_YOGGSARON_KILLED:
                {
                    KeeperHandling(false);
                    TC_LOG_DEBUG("entities.player", "Yogg Saron killed was called!");

                    if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                        yogg->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
                    if (duration <= 7*MIN)
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEVMENT_HE_S_NOT_GETTING_ANY_OLDER_10, ACHIEVMENT_HE_S_NOT_GETTING_ANY_OLDER_25));
                    if (!usedMindcontroll)
                        instance->DoCompleteAchievement(RAID_MODE(ACHIEVMENT_DRIVE_ME_CRAZY_10, ACHIEVMENT_DRIVE_ME_CRAZY_25));

                    // Achievements
                    switch (CountKeepersActive())
                    {
                        case 0:
                            /*
                            // Grant realm first alone in the darkness kill in 25man mode if not already completed
                            if (me->GetMap() && me->GetMap()->IsRaid() && me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL)
                                DoCompleteDeathDemise();
                            */
                            instance->DoCompleteAchievement(RAID_MODE(ACHIEVMENT_ALONE_IN_THE_DARKNESS_10, ACHIEVMENT_ALONE_IN_THE_DARKNESS_25));
                        case 1:
                            instance->DoCompleteAchievement(RAID_MODE(ACHIEVMENT_ONE_LIGHTS_IN_THE_DARKNESS_10, ACHIEVMENT_ONE_LIGHTS_IN_THE_DARKNESS_25));
                        case 2:
                            instance->DoCompleteAchievement(RAID_MODE(ACHIEVMENT_TWO_LIGHTS_IN_THE_DARKNESS_10, ACHIEVMENT_TWO_LIGHTS_IN_THE_DARKNESS_25));
                        case 3:
                            instance->DoCompleteAchievement(RAID_MODE(ACHIEVMENT_THREE_LIGHTS_IN_THE_DARKNESS_10, ACHIEVMENT_THREE_LIGHTS_IN_THE_DARKNESS_25));
                            break;
                        default:
                            break;
                    }

                    me->KillSelf();
                    break;
                }
                case ACTION_USED_MINDCONTROL:
                    usedMindcontroll = true;
                    break;
                case ACTION_GUARDIAN_KILLED_ACHIEVMENT_PROGRESS:
                    if (counterStarted)
                    {
                        deadGuardianCounter++;
                        if (deadGuardianCounter >= 9)
                            instance->DoCompleteAchievement(RAID_MODE(ACHIEVMENT_COMING_OUT_OF_THE_WALLS_10, 
                                                                      ACHIEVMENT_COMING_OUT_OF_THE_WALLS_25));
                    }
                    //else: No guardian killed in the last 12 sec, start the counter
                    else
                    {
                        deadGuardianCounter = 1;
                        counterStarted = true;
                        events.ScheduleEvent(EVENT_ACHIEVEMENT_TIMER_RAN_OUT, 12 * IN_MILLISECONDS);
                    }
                    break;
            }
        }

        uint32 CountKeepersActive() {
            uint32 amountKeeperActive = 5;

            if (instance)
            {

                amountKeeperActive = 0;
                uint32 supportFlag = instance->GetData(DATA_KEEPER_SUPPORT_YOGG);

                if (supportFlag & (1 << MIMIRON))
                    amountKeeperActive++;

                if (supportFlag & (1 << FREYA))
                    amountKeeperActive++;

                if (supportFlag & (1 << THORIM))
                    amountKeeperActive++;

                if (supportFlag & (1 << HODIR))
                    amountKeeperActive++;
            }
            TC_LOG_DEBUG("entities.player", "Amount Keeper is currently at %u", amountKeeperActive);
            return amountKeeperActive;
        }


        /*
        void DoCompleteDeathDemise()
        {
            // Grant realm first achievement if not already completed by some player

            // only 25man mode
            if (!me->GetMap() || !me->GetMap()->IsRaid() || me->GetMap()->GetDifficulty() != RAID_DIFFICULTY_25MAN_NORMAL)
                return;

            // Get Achievement and Players
            AchievementEntry const* deathDemise = sAchievementStore.LookupEntry(3117);
            Map::PlayerList const& players = me->GetMap()->GetPlayers();


            if (!deathDemise || players.isEmpty())
                return;

            // check if already completed by some player
            if (sAchievementMgr->IsRealmCompleted(deathDemise))
                return;

            for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                if (Player *pPlayer = i->GetSource())
                    pPlayer->CompletedAchievement(deathDemise);
        }
        */

        uint32 GetData(uint32 data) const
        {
            switch (data)
            {
                case DATA_PORTAL_PHASE:
                    return currentBrainEventPhase;
                case DATA_SARA_PHASE:
                    return (uint32) phase;
                default:
                    break;
            }
            return 0;
        }

        void SetPhase(BossPhase newPhase)
        {
            if (phase == newPhase)
                return;

            phase = newPhase;
            events.SetPhase(newPhase);

            switch (newPhase)
            {
                case PHASE_SARA: {
                    Talk(SAY_SARA_AGGRO_1);
                    SetSanityAura();

                    // set them active
                    KeeperHandling(true);

                    // Loot
                    if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                    {
                        yogg->SetLootMode(LOOT_MODE_DEFAULT);

                        switch (CountKeepersActive())
                        {
                            case 0:
                                yogg->AddLootMode(LOOT_MODE_HARD_MODE_4); // mimirons head / tentacle
                            case 1:
                                yogg->AddLootMode(LOOT_MODE_HARD_MODE_3); // hm loot
                            case 2:
                                yogg->AddLootMode(LOOT_MODE_HARD_MODE_2); // additional orbs / recipes
                            case 3:
                                yogg->AddLootMode(LOOT_MODE_HARD_MODE_1); // additional orbs / recipes
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                }
                case PHASE_BRAIN_INTRO:
                    me->SetFullHealth();
                    events.ScheduleEvent(EVENT_BRAIN_INTRO_1, 1*SEC, 0, PHASE_BRAIN_INTRO);
                    eventSpeakingTimer = 5000;
                    isEventSpeaking = false;
                    eventSpeakingPhase = 0;
                    CloudHandling(true);
                    randomYellTimer = 35000;
                    brainEventsCount = 0;
                    break;
                case PHASE_BRAIN:
                    me->SetFullHealth();
                    events.RescheduleEvent(EVENT_PSYCHOSIS, 5*SEC, 0, PHASE_BRAIN);

                    /* Death rays temporary disabled
                    // Schedule mindspells in random order
                    if (urand(0,1))
                    {
                        events.RescheduleEvent(EVENT_MALADY, 15*SEC, 0, PHASE_BRAIN);
                        events.RescheduleEvent(EVENT_DEATH_RAYS, 25*SEC, 0, PHASE_BRAIN);
                        events.RescheduleEvent(EVENT_BRAIN_LINK, 35*SEC, 0, PHASE_BRAIN);
                    } else {
                        events.RescheduleEvent(EVENT_BRAIN_LINK, 35*SEC, 0, PHASE_BRAIN);
                        events.RescheduleEvent(EVENT_DEATH_RAYS, 15*SEC, 0, PHASE_BRAIN);
                        events.RescheduleEvent(EVENT_MALADY, 25*SEC, 0, PHASE_BRAIN);
                    }
                    */

                    // Schedule mindspells in random order

                    if (urand(0,1)) {     
                        events.RescheduleEvent(EVENT_MALADY, 15*SEC, 0, PHASE_BRAIN);
                        events.RescheduleEvent(EVENT_BRAIN_LINK, 25*SEC, 0, PHASE_BRAIN);
                     } else {
                        events.RescheduleEvent(EVENT_BRAIN_LINK, 15*SEC, 0, PHASE_BRAIN);
                        events.RescheduleEvent(EVENT_MALADY, 25*SEC, 0, PHASE_BRAIN);
                     }

                    events.RescheduleEvent(EVENT_SPAWN_CRUSHER, 1*SEC, 0, PHASE_BRAIN);
                    events.RescheduleEvent(EVENT_SPAWN_CORRUPTOR, urand(2*SEC,6*SEC), 0, PHASE_BRAIN);
                    events.RescheduleEvent(EVENT_SPAWN_CONSTRICTOR, urand(2*SEC,6*SEC), 0, PHASE_BRAIN);
                    break;

                case PHASE_YOGG:
                    events.RescheduleEvent(EVENT_SUMMON_IMMORTAL_GUARDIAN, 100, 0, PHASE_YOGG); // instant
                    events.RescheduleEvent(EVENT_DEAFENING_ROAR, 30*SEC, 0, PHASE_YOGG);
                    events.RescheduleEvent(EVENT_LUNATIC_GAZE, 12*SEC, 0, PHASE_YOGG);
                    events.RescheduleEvent(EVENT_SHADOW_BEACON, 45*SEC, 0, PHASE_YOGG);
                    me->RemoveAurasDueToSpell(SPELL_SHATTERED_ILLUSIONS);
                    me->SetVisible(false);
                    if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                    {
                        yogg->LowerPlayerDamageReq(uint32(yogg->GetMaxHealth()*0.3f));
                        yogg->SetHealth(yogg->CountPctFromMaxHealth(30));
                        yogg->RemoveAurasDueToSpell(SPELL_SHADOWY_BARRIER);
                        yogg->RemoveAurasDueToSpell(SPELL_SHATTERED_ILLUSIONS);

                        yogg->AI()->Talk(SAY_PHASE3);
                        me->AddAura(SPELL_YOGG_SARON_TRANSFORMATION, yogg);
                    }
                    for (std::list<ObjectGuid>::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                    {
                        if (Creature* temp = ObjectAccessor::GetCreature(*me, *itr))
                        {
                            switch (temp->GetEntry())
                            {
                                case ENTRY_CONSTRICTOR_TENTACLE:
                                case ENTRY_CORRUPTOR_TENTACLE:
                                case ENTRY_CRUSHER_TENTACLE:
                                    temp->RemoveAurasDueToSpell(SPELL_SHATTERED_ILLUSIONS);
                                    break;
                            }
                        }
                    }

                    summons.DespawnEntry(ENTRY_BRAIN_PORTAL);

                    if (Creature* yoggbrain = ObjectAccessor::GetCreature(*me, guidYoggBrain))
                        yoggbrain->InterruptNonMeleeSpells(false);

                    break;
                default:
                    break;
            }
        }

        void MoveInLineOfSight(Unit* target)
        {
            if (phase == PHASE_NONE)
            {
                if (target && me->GetDistance2d(target) <= 92 && target->ToPlayer() && !target->ToPlayer()->IsGameMaster())
                {
                    if (instance && instance->GetBossState(BOSS_VEZAX) == DONE)
                    {
                        SetPhase(PHASE_SARA);
                        EnterCombat(target);
                    }
                }
            }
        }

        void JustSummoned(Creature* summoned)
        {
            switch (summoned->GetEntry())
            {
                case ENTRY_GUARDIAN_OF_YOGG_SARON:
                    summoned->setFaction(14);
                    if (summoned->AI())
                        summoned->AI()->AttackStart(SelectRandomPlayer(me, 250.0f));
                    break;
                case ENTRY_YOGG_SARON:
                    guidYogg = summoned->GetGUID();
                    summoned->SetReactState(REACT_PASSIVE);
                    summoned->setFaction(14);
                    summoned->SetStandState(UNIT_STAND_STATE_SUBMERGED);
                    summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    summoned->SetFloatValue(UNIT_FIELD_COMBATREACH, 20.0f);
                    summoned->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 20.0f);
                    return;
                case ENTRY_BRAIN_OF_YOGG_SARON:
                    guidYoggBrain = summoned->GetGUID();
                    summoned->setActive(true);
                    summoned->SetFloatValue(UNIT_FIELD_COMBATREACH, 25.0f);
                    summoned->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 25.0f);
                    summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    break;
                case ENTRY_RUBY_CONSORT:
                case ENTRY_OBSIDIAN_CONSORT:
                case ENTRY_AZURE_CONSORT:
                case ENTRY_EMERAL_CONSORT:
                case ENTRY_DEATHSWORN_ZEALOT:
                case ENTRY_SUIT_OF_ARMOR:
                    summoned->SetReactState(REACT_DEFENSIVE);
                    summoned->setFaction(15);
                    break;
                case ENTRY_LAUGHING_SKULL:
                    summoned->setFaction(14);
                    summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    break;
            }

            summons.Summon(summoned);
        }

        // Search for the Immortal Guardian with minimum health, which is not already affected by Shadow Beacon
        Unit* SelectImmortalGuardianWithMinHealth()
        {
            std::list<Creature*> guardians;
            me->GetCreatureListWithEntryInGrid(guardians, ENTRY_IMMORTAL_GUARDIAN, 100.0f);

            if (guardians.empty())
                return NULL;

            Unit* minGuardian = NULL;
            uint32 minHealth = (*guardians.begin())->GetMaxHealth();

            for (std::list<Creature*>::const_iterator itr = guardians.begin(); itr != guardians.end(); ++itr)
            {
                if ((*itr) && (*itr)->IsAlive() && !(*itr)->HasAura(SPELL_SHADOW_BEACON) && ((*itr)->GetHealth() <= minHealth))
                {
                    minGuardian = (*itr);
                    minHealth = minGuardian->GetHealth();
                }
            }

            return minGuardian;
        }

        void UpdateAI(uint32 diff)
        {
            if (instance && instance->GetBossState(BOSS_VEZAX) != DONE)
                return;

            // try to avoid buggy cloudspawn in p3 and invisible clouds
            if (!cloudsSpawned)
            {
                if (cloudSpawning <= diff)
                {
                    CloudHandling(false);
                    cloudsSpawned = true;
                } else {
                    cloudSpawning -= diff;
                }
            }

            if (randomYellTimer <= diff)
            {
                switch (phase)
                {
                    case PHASE_NONE:
                        me->AI()->Talk(RAND(SAY_SARA_PREFIGHT_1, SAY_SARA_PREFIGHT_2));
                        break;
                    case PHASE_SARA:
                        me->AI()->Talk(RAND(SAY_SARA_AGGRO_2, SAY_SARA_AGGRO_3));
                        break;
                    case PHASE_BRAIN:
                        me->AI()->Talk(RAND(SAY_SARA_PHASE2_1, SAY_SARA_PHASE2_2, SAY_SARA_AGGRO_3));
                        break;
                    default:
                        break;
                }
                randomYellTimer = urand(40000, 60000);
            } else randomYellTimer -= diff;

            if (phase == PHASE_NONE && me->IsInCombat())
                Reset();

            if (phase == PHASE_NONE)
                return;

            if (!SanePlayerAlive())
            {
                Reset();
                return;
            }

            if (duration >= ENRAGE)
            {
                if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                {
                    yogg->InterruptNonMeleeSpells(false);
                    yogg->CastSpell(yogg, SPELL_EXTINGUISH_ALL_LIFE, false);
                }
                duration = ENRAGE-5000;
            }
            duration += diff;

            if (!me->HasAura(SPELL_SHATTERED_ILLUSIONS) || phase != PHASE_BRAIN)
                events.Update(diff);

            switch (phase)
            {
                case PHASE_SARA:
                    while (uint32 eventID = events.ExecuteEvent())
                    {
                        switch (eventID)
                        {
                            case EVENT_SUMMON_GUARDIAN: // Summon a guardian from a cloud
                                if (Creature* cloud = GetRandomEntryTarget(ENTRY_OMINOUS_CLOUD,500.0f))
                                    if (cloud->AI())
                                        cloud->AI()->DoAction(ACTION_SPAWN_GUARDIAN_OF_YOGG_SARON);

                                if (summonedGuardiansCount < 6)
                                    events.RescheduleEvent(EVENT_SUMMON_GUARDIAN, 10*SEC + (6 - summonedGuardiansCount)*5*SEC, 0, PHASE_SARA);
                                else
                                    events.RescheduleEvent(EVENT_SUMMON_GUARDIAN, 10*SEC, 0, PHASE_SARA);
                                summonedGuardiansCount++;
                                break;
                            case EVENT_ACHIEVEMENT_TIMER_RAN_OUT:
                                resetAchievementProgressComingOutOfTheWalls();
                                break;
                            case EVENT_SARA_HELP: // Sara "helps" the raid
                                uint32 rnd = urand(0,2);
                                switch (rnd)
                                {
                                    case 0:
                                        if (Unit* target = SelectRandomPlayer(me, 500.0f))
                                        {
                                            me->SetTarget(target->GetGUID());
                                            DoCast(target, SPELL_SARAS_FERVOR, false);
                                        }
                                        break;
                                    case 1:
                                        if (Unit* target = SelectRandomPlayer(me, 500.0f))
                                        {
                                            if (!target->HasAura(SPELL_SPIRIT_OF_REDEMPTION))
                                            {
                                                me->SetTarget(target->GetGUID());
                                                DoCast(target, SPELL_SARAS_BLESSING, false);
                                            }                                            
                                        }
                                        break;
                                    case 2:
                                        if (Unit* target = GetRandomEntryTarget(ENTRY_GUARDIAN_OF_YOGG_SARON, 500.0f))
                                        {
                                            me->SetTarget(target->GetGUID());
                                            DoCast(target, SPELL_SARAS_ANGER, false);
                                        }
                                        break;
                                }

                                // Set Life to correct value because Sara can be healed with chain heal
                                me->SetHealth((novaHitCounter * 25000) + 1);

                                events.RescheduleEvent(EVENT_SARA_HELP, urand(5000,6000), 0, PHASE_SARA);
                                break;
                        
                                    
                        } // switch eventID
                    } // while eventID
                    break;

                case PHASE_BRAIN_INTRO:
                    while (uint32 eventID = events.ExecuteEvent())
                    {
                        switch (eventID)
                        {
                            case EVENT_BRAIN_INTRO_1:
                                me->AI()->Talk(SAY_BRAIN_INTRO_1);
                                events.ScheduleEvent(EVENT_BRAIN_INTRO_2, 5*SEC, 0, PHASE_BRAIN_INTRO);
                                break;
                            case EVENT_BRAIN_INTRO_2:
                                me->AI()->Talk(SAY_BRAIN_INTRO_2);
                                events.ScheduleEvent(EVENT_BRAIN_INTRO_3, 5*SEC, 0, PHASE_BRAIN_INTRO);
                                break;
                            case EVENT_BRAIN_INTRO_3:
                                me->AI()->Talk(SAY_BRAIN_INTRO_3);
                                events.ScheduleEvent(EVENT_BRAIN_INTRO_4, 5*SEC, 0, PHASE_BRAIN_INTRO);
                                break;
                            case EVENT_BRAIN_INTRO_4:
                                me->AI()->Talk(SAY_BRAIN_INTRO_4);
                                events.ScheduleEvent(EVENT_BRAIN_TRANSFORMATION, 4*SEC, 0, PHASE_BRAIN_INTRO);
                                break;
                            case EVENT_BRAIN_TRANSFORMATION:
                                me->AddAura(SPELL_SARA_TRANSFORMATION, me);
                                DoCastSelf(SPELL_SARA_SHADOWY_BARRIER, false);
                                me->GetMotionMaster()->MovePoint(1, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()+20);
                                if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                                {
                                    Position yoggPos = yogg->GetHomePosition();
                                    yogg->NearTeleportTo(yoggPos.GetPositionX(), yoggPos.GetPositionY(), yoggPos.GetPositionZ(), yoggPos.GetOrientation());

                                    yogg->CastSpell(yogg, SPELL_SHADOWY_BARRIER, false);
                                    yogg->SetStandState(UNIT_STAND_STATE_STAND);
                                    yogg->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                    yogg->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                                    yogg->AI()->Talk(SAY_BRAIN_INTRO_5);
                                }
                                me->setFaction(14);
                                SetPhase(PHASE_BRAIN);
                                break;
                            case EVENT_ACHIEVEMENT_TIMER_RAN_OUT:
                                resetAchievementProgressComingOutOfTheWalls();
                                break;
                        } // switch eventID
                    } // while eventID
                    break;

                case PHASE_BRAIN: // Handle brain phase
                    // handle event says
                    if (isEventSpeaking)
                    {
                        if (eventSpeakingTimer <= diff)
                        {
                            eventSpeakingTimer = DoEventSpeaking(currentBrainEventPhase, eventSpeakingPhase);
                            ++eventSpeakingPhase;
                        } else eventSpeakingTimer -= diff;
                    }

                    // handle event spawning
                    if (madnessTimer <= diff)
                    {
                        if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                        {
                            yogg->AI()->Talk(SAY_VISION);
                            DoMadness();
                        }
                        madnessTimer = 80000;
                    } else madnessTimer -= diff;

                    if (!me->HasAura(SPELL_SHATTERED_ILLUSIONS)) { // NOT STUNNED
                        if (currentBrainEventPhase != PORTAL_PHASE_BRAIN_NONE)
                        {
                            if (AllSpawnsDeadOrDespawned(guidEventTentacles))
                            {
                                // all influence tentacles killed - stun tentacles & open brain room door
                                switch (currentBrainEventPhase)
                                {
                                    case PORTAL_PHASE_DRAGON_SOUL:
                                        if (instance)
                                            instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_YOGGBRAIN_DOOR_1)), true);
                                        break;
                                    case PORTAL_PHASE_LICH_KING:
                                        if (instance)
                                            instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_YOGGBRAIN_DOOR_2)), true);
                                        break;
                                    case PORTAL_PHASE_KING_LLIANE:
                                        if (instance)
                                            instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_YOGGBRAIN_DOOR_3)), true);
                                        break;
                                    default:
                                        break;
                                }
                                currentBrainEventPhase = PORTAL_PHASE_BRAIN_NONE;
                                isEventSpeaking = false;
                                if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                                    me->AddAura(SPELL_SHATTERED_ILLUSIONS, yogg);
                                me->AddAura(SPELL_SHATTERED_ILLUSIONS, me);

                                if (Creature* yoggbrain = ObjectAccessor::GetCreature(*me, guidYoggBrain))
                                    yoggbrain->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                                for (std::list<ObjectGuid>::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                                {
                                    if (Creature* temp = ObjectAccessor::GetCreature(*me, *itr))
                                    {
                                        switch (temp->GetEntry())
                                        {
                                            case ENTRY_CONSTRICTOR_TENTACLE:
                                            case ENTRY_CORRUPTOR_TENTACLE:
                                            case ENTRY_CRUSHER_TENTACLE:
                                                    me->AddAura(SPELL_SHATTERED_ILLUSIONS, temp);
                                                break;
                                        }
                                    }
                                }
                                DoKillAndDespawnGUIDs(guidEventSkulls);
                            }

                        } // handle brain
                        while (uint32 eventID = events.ExecuteEvent())
                        {
                            switch (eventID)
                            {
                                case EVENT_PSYCHOSIS:
                                    if (Unit* target = SelectRandomPlayer(me, 500.0f, false, false, 0u, false, 30u))
                                    {
                                        me->SetTarget(target->GetGUID());
                                        DoCast(target, RAID_MODE(SPELL_PSYCHOSIS_10, SPELL_PSYCHOSIS_25));
                                    }
                                    events.RescheduleEvent(EVENT_PSYCHOSIS, RAID_MODE(3000u, 1100u), 0, PHASE_BRAIN);
                                    break;

                                case EVENT_BRAIN_LINK:
                                        if (me->IsNonMeleeSpellCast(false)) // mindspells have higher priority than psychosis
                                            me->InterruptNonMeleeSpells(false);

                                        if (Unit* target1 = SelectRandomPlayer(me, 500.0f, false, false, SPELL_BRAIN_LINK))
                                        {
                                            me->AddAura(SPELL_BRAIN_LINK, target1);
                                            if (Unit* target2 = SelectRandomPlayer(me, 500.0f, false, false, SPELL_BRAIN_LINK))
                                            {
                                                me->AddAura(SPELL_BRAIN_LINK, target2);
                                            }
                                        }

                                        events.RescheduleEvent(EVENT_BRAIN_LINK, RAID_MODE(32u*SEC, 35u*SEC), 0, PHASE_BRAIN);
                                        break;
                                case EVENT_DEATH_RAYS:
                                        if (me->IsNonMeleeSpellCast(false)) // mindspells have higher priority than psychosis
                                            me->InterruptNonMeleeSpells(false);

                                        DoCast(SPELL_DEATH_RAY_SUMMON);

                                        events.RescheduleEvent(EVENT_DEATH_RAYS, RAID_MODE(27u*SEC, 32u*SEC), 0, PHASE_BRAIN);
                                        break;
                                case EVENT_MALADY:
                                        if (me->IsNonMeleeSpellCast(false)) // mindspells have higher priority than psychosis
                                            me->InterruptNonMeleeSpells(false);

                                        if (Unit* target = SelectRandomPlayer(me, 500.0f, false, false, SPELL_MALADY_OF_MIND))
                                        {
                                            me->SetTarget(target->GetGUID());
                                            DoCast(target, SPELL_MALADY_OF_MIND);
                                        }

                                        events.RescheduleEvent(EVENT_MALADY, RAID_MODE(27u*SEC, 32u*SEC), 0, PHASE_BRAIN);
                                        break;
                                case EVENT_SPAWN_CONSTRICTOR:
                                    for (uint8 i = 0; i < 25; i++)
                                    {
                                        // Try to select a target that has no squeeze debuff and is not in brainroom
                                        if (Unit* target = SelectRandomPlayer(me, 500.0f, false, false, SPELL_SQUEEZE, true))
                                        {
                                            if (Creature* tentacle = target->SummonCreature(ENTRY_CONSTRICTOR_TENTACLE, *target, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT))
                                            {
                                                tentacle->AI()->DoZoneInCombat(tentacle);
                                            }
                                            break;
                                        }
                                    }
                                    if (brainEventsCount < 4)
                                        events.RescheduleEvent (EVENT_SPAWN_CONSTRICTOR, 20*SEC, 0, PHASE_BRAIN);
                                    else
                                        events.RescheduleEvent (EVENT_SPAWN_CONSTRICTOR, 12*SEC, 0, PHASE_BRAIN);
                                    break;
                                case EVENT_SPAWN_CRUSHER: {
                                    Position pos = TentacleAndGuardianSpawnPos[urand(0, COUNT_SPAWN_POSTIONS-1)];
                                    if (Creature* tentacle = me->SummonCreature(ENTRY_CRUSHER_TENTACLE, pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000))
                                    {
                                        tentacle->SetInCombatWithZone();
                                    }

                                    if (brainEventsCount < 4)
                                        events.RescheduleEvent (EVENT_SPAWN_CRUSHER, urand(55*SEC, 60*SEC), 0, PHASE_BRAIN);
                                    else
                                        events.RescheduleEvent (EVENT_SPAWN_CRUSHER, urand(27*SEC, 35*SEC), 0, PHASE_BRAIN);
                                    }
                                    break;
                                case EVENT_SPAWN_CORRUPTOR: {
                                    Position pos = TentacleAndGuardianSpawnPos[urand(0, COUNT_SPAWN_POSTIONS-1)];
                                    if (Creature* tentacle = me->SummonCreature(ENTRY_CORRUPTOR_TENTACLE, pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000))
                                    {
                                        tentacle->SetInCombatWithZone();
                                    }

                                    if (brainEventsCount < 4)
                                        events.RescheduleEvent (EVENT_SPAWN_CORRUPTOR, urand(15*SEC, 20*SEC), 0, PHASE_BRAIN);
                                    else
                                        events.RescheduleEvent (EVENT_SPAWN_CORRUPTOR, urand(8*SEC, 12*SEC), 0, PHASE_BRAIN);
                                    }
                                    break;
                                case EVENT_ACHIEVEMENT_TIMER_RAN_OUT:
                                    resetAchievementProgressComingOutOfTheWalls();
                                    break;
                            } // switch eventId
                        } // while eventId
                    } // if NOT STUNNED
                    else // if STUNNED
                    {
                        //Achievement timer can run out even if stunned
                        while (uint32 eventID = events.ExecuteEvent())
                        {
                            switch (eventID)
                            {
                                case EVENT_ACHIEVEMENT_TIMER_RAN_OUT:
                                    resetAchievementProgressComingOutOfTheWalls();
                                    break;
                            }

                        }
                        if (Creature* yoggbrain = ObjectAccessor::GetCreature(*me, guidYoggBrain))
                        {
                            if (!yoggbrain->IsNonMeleeSpellCast(false)) // yogg brain is not casting a spell -> brain phase is over
                            {
                                if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                                    yogg->RemoveAurasDueToSpell(SPELL_SHATTERED_ILLUSIONS);
                                me->RemoveAurasDueToSpell(SPELL_SHATTERED_ILLUSIONS);

                                for (std::list<ObjectGuid>::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                                {
                                    if (Creature* temp = ObjectAccessor::GetCreature(*me, *itr))
                                    {
                                        switch (temp->GetEntry())
                                        {
                                            case ENTRY_CONSTRICTOR_TENTACLE:
                                            case ENTRY_CORRUPTOR_TENTACLE:
                                            case ENTRY_CRUSHER_TENTACLE:
                                                temp->RemoveAurasDueToSpell(SPELL_SHATTERED_ILLUSIONS);
                                            break;
                                        }
                                    }
                                }

                                // Reschedule Tentacles
                                events.RescheduleEvent(EVENT_SPAWN_CRUSHER, 1*SEC, 0, PHASE_BRAIN);
                                events.RescheduleEvent(EVENT_SPAWN_CORRUPTOR, urand(2*SEC,6*SEC), 0, PHASE_BRAIN);
                                events.RescheduleEvent(EVENT_SPAWN_CONSTRICTOR, urand(2*SEC,6*SEC), 0, PHASE_BRAIN);
                            } // if NOT CASTING SPELL
                        }
                    } // if STUNNED
                    break;

                case PHASE_YOGG:
                    if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                        if (yogg->IsNonMeleeSpellCast(false))
                            return;

                    while (uint32 eventID = events.ExecuteEvent())
                    {
                        switch (eventID)
                        {
                            case EVENT_SUMMON_IMMORTAL_GUARDIAN: {
                                Position pos = TentacleAndGuardianSpawnPos[urand(0, COUNT_SPAWN_POSTIONS-1)];
                                if (Creature* guardian = me->SummonCreature(ENTRY_IMMORTAL_GUARDIAN, pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000))
                                {
                                    guardian->SetInCombatWithZone();
                                }

                                    events.RescheduleEvent(EVENT_SUMMON_IMMORTAL_GUARDIAN, 10*SEC, 0, PHASE_YOGG);
                                summonedImmortalGuardiansCount++;
                                }
                                break;
                            case EVENT_LUNATIC_GAZE:
                                if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                                {
                                    yogg->AI()->Talk(SAY_LUNCATIC_GLAZE);
                                    yogg->CastSpell(yogg, SPELL_LUNATIC_GAZE, false);
                                }
                                events.RescheduleEvent(EVENT_LUNATIC_GAZE, 12*SEC, 0, PHASE_YOGG);
                                break;
                            case EVENT_DEAFENING_ROAR:
                                // only in 25man hardmode
                                if (RAID_MODE(1,0) || CountKeepersActive() == 4)
                                    break;

                                if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                                {
                                    yogg->AI()->Talk(SAY_DEAFENING_ROAR);
                                    yogg->CastSpell(yogg, SPELL_DEAFENING_ROAR, false);
                                }
                                events.RescheduleEvent(EVENT_DEAFENING_ROAR, 60*SEC, 0, PHASE_YOGG);
                                break;
                            case EVENT_SHADOW_BEACON: {

                                uint32 affectedGuardianCount = RAID_MODE(2u, 3u);

                                if (Creature* yogg = ObjectAccessor::GetCreature(*me, guidYogg))
                                {
                                    for (uint8 i = 0; i < affectedGuardianCount; i++)
                                    {
                                        if (Unit* target = SelectImmortalGuardianWithMinHealth())
                                        {
                                            yogg->CastSpell(target, SPELL_SHADOW_BEACON, false);
                                        }
                                    }
                                }


                                events.RescheduleEvent(EVENT_SHADOW_BEACON, 45*SEC, 0, PHASE_YOGG);

                                break;
                                } // case
                            case EVENT_ACHIEVEMENT_TIMER_RAN_OUT:
                                resetAchievementProgressComingOutOfTheWalls();
                                break;
                        } // switch eventid
                    } // while event
                default:
                    break;
            } // switch phase
        }

        void ModifySanity(Player* target, int8 amount)
        {
            if (target && target->IsAlive())
            {
                int32 newamount;
                if (Aura* aur = target->GetAura(SPELL_SANITY))
                {
                    newamount = aur->GetStackAmount();
                    if (newamount > 0)
                        newamount += amount;

                    if (newamount > 100)
                        newamount = 100;

                    if (newamount <= 0)
                        target->RemoveAurasDueToSpell(SPELL_SANITY);
                    else
                        aur->SetStackAmount(uint8(newamount));
                }
            }
        }


        // Check if we should reset (Non-GM with remaining sanity still alive)
        bool SanePlayerAlive()
        {
            if (me->GetMap() && me->GetMap()->IsDungeon())
            {
                for (Player& player : me->GetMap()->GetPlayers())
                    if (player.IsAlive() && !player.IsGameMaster() && !player.HasAura(SPELL_INSANE))
                        return true;
            }
            return false;
        }

        void SetSanityAura(bool remove = false)
        {
            if (me->GetMap() && me->GetMap()->IsDungeon())
            {
                for (Player& player : me->GetMap()->GetPlayers())
                    if (player.IsAlive())
                    {
                        if (!remove)
                        {
                            me->AddAura(SPELL_SANITY, &player);
                            me->SetAuraStack(SPELL_SANITY, &player, 100);
                        }
                        else
                            player.RemoveAurasDueToSpell(SPELL_SANITY);
                        // Hack to avoid visual tentacle bugs
                        player.RemoveAurasDueToSpell(SPELL_SQUEEZE);
                        player.RemoveAurasDueToSpell(SPELL_SQUEEZE+1);
                    }
            }
        }

        Creature* GetRandomEntryTarget(uint32 entry, float range = 100.0f)
        {
            std::list<Creature*> TargetList;
            me->GetCreatureListWithEntryInGrid(TargetList,entry, range);
            if (TargetList.empty())
                return NULL;

            std::list<Creature*>::iterator itr = TargetList.begin();
            advance(itr, urand(0, TargetList.size()-1));
            return (*itr);
        }

        bool AllSpawnsDeadOrDespawned(std::list<ObjectGuid> GuidListe)
        {
            for (std::list<ObjectGuid>::iterator itr = GuidListe.begin(); itr != GuidListe.end(); ++itr)
            {
                Creature* temp = ObjectAccessor::GetCreature(*me,*itr);
                if (temp && temp->IsAlive())
                    return false;
            }

            return true;
        }

        void DoKillAndDespawnGUIDs(std::list<ObjectGuid> GuidListe)
        {
            for (std::list<ObjectGuid>::iterator itr = GuidListe.begin(); itr != GuidListe.end(); ++itr)
            {
                Creature* temp = ObjectAccessor::GetCreature(*me,*itr);
                if (temp && temp->IsAlive())
                {
                    temp->DealDamage(temp,temp->GetHealth());
                    temp->RemoveCorpse();
                }
            }
        }

        void SummonMadnesseventNPCs()
        {
            listeventNPCs.clear();
            uint32 npcIndex = 0;
            uint32 npcAmount = 0;

            switch (currentBrainEventPhase)
            {
                case PORTAL_PHASE_KING_LLIANE:
                    npcIndex = 0;
                    npcAmount = 2;
                    break;
                case PORTAL_PHASE_DRAGON_SOUL:
                    npcIndex = 2;
                    npcAmount = 4;
                    break;
                case PORTAL_PHASE_LICH_KING:
                    npcIndex = 6;
                    npcAmount = 3;
                    break;
                default:
                    return; // something went wrong.
            }

            for (uint8 i = 0; i < npcAmount; i++)
            {
                if (eventNPCEntrys[npcIndex+i] == OBJECT_THE_DRAGON_SOUL)
                {
                    if (GameObject* obj = me->SummonGameObject(eventNPCEntrys[npcIndex+i],eventNPCLocation[npcIndex+i].GetPositionX(),eventNPCLocation[npcIndex+i].GetPositionY(),eventNPCLocation[npcIndex+i].GetPositionZ(), 0, 0, 0, 0, 0, 40000))
                    {
                        obj->setActive(true);
                        eventNPC* info = new eventNPC();
                        info->entry = obj->GetEntry();
                        info->guid = obj->GetGUID();

                        listeventNPCs.push_back(*info);
                    }
                }
                else
                {
                    if (Creature* temp = DoSummon(eventNPCEntrys[npcIndex+i],eventNPCLocation[npcIndex+i],40000,TEMPSUMMON_TIMED_DESPAWN))
                    {
                        temp->setActive(true);
                        eventNPC* info = new eventNPC();
                        info->entry = temp->GetEntry();
                        info->guid = temp->GetGUID();
                        switch (info->entry)
                        {
                            case ENTRY_GARONA_VISION:
                                temp->SetStandState(UNIT_STAND_STATE_KNEEL);
                                break;
                        }
                        listeventNPCs.push_back(*info);
                    }
                }
            }
        }

        ObjectGuid GeteventNPCGuid(uint32 entry)
        {
            for (std::list<eventNPC>::iterator itr = listeventNPCs.begin(); itr != listeventNPCs.end(); ++itr)
            {
                if (itr->entry == entry)
                    return itr->guid;
            }
            return ObjectGuid::Empty;
        }

        void DoMadness()
        {
            if (instance)
            {
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_YOGGBRAIN_DOOR_1)), false);
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_YOGGBRAIN_DOOR_2)), false);
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(GO_YOGGBRAIN_DOOR_3)), false);
            }

            Actions tempAction;
            switch (lastBrainAction)
            {
                case ACTION_PORTAL_TO_MADNESS_STORMWIND:
                    tempAction = ACTION_PORTAL_TO_MADNESS_DRAGON;
                    break;
                case ACTION_PORTAL_TO_MADNESS_DRAGON:
                    tempAction = ACTION_PORTAL_TO_MADNESS_LICHKING;
                    break;
                case ACTION_PORTAL_TO_MADNESS_LICHKING:
                    tempAction = ACTION_PORTAL_TO_MADNESS_STORMWIND;
                    break;
                default:
                    tempAction = RAND(ACTION_PORTAL_TO_MADNESS_STORMWIND,ACTION_PORTAL_TO_MADNESS_DRAGON,ACTION_PORTAL_TO_MADNESS_LICHKING);
                    break;
            }
            lastBrainAction = tempAction;

            // Spawn Portal
            int max = RAID_MODE(MAX_BRAIN_ROOM_10, MAX_BRAIN_ROOM_25);
            for (int i = 0; i < max; ++i)
            {
                if (Creature* portal = DoSummon(ENTRY_BRAIN_PORTAL,brainPortalLocation[i],40000,TEMPSUMMON_TIMED_DESPAWN))
                    portal->AI()->DoAction(tempAction);
            }

            // Spawn Event Tentacle
            switch (tempAction)
            {
                case ACTION_PORTAL_TO_MADNESS_DRAGON: {

                    currentBrainEventPhase = PORTAL_PHASE_DRAGON_SOUL;
                    guidEventTentacles.clear();
                    guidEventSkulls.clear();
                    uint32 entry = 0;
                    for (int i = 0; i < MAX_DRAGONSOUL_TENTACLE_SPAWNS; ++i)
                    {
                        switch (i)
                        {
                            case 0:
                            case 1: entry = ENTRY_RUBY_CONSORT; break;
                            case 2:
                            case 3: entry = ENTRY_AZURE_CONSORT; break;
                            case 4:
                            case 5: entry = ENTRY_OBSIDIAN_CONSORT; break;
                            case 6:
                            case 7: entry = ENTRY_EMERAL_CONSORT; break;
                        }
                        if (entry)
                            if (Creature* summon = DoSummon(entry, dragonSoulTentacleLocation[i], 60000, TEMPSUMMON_TIMED_DESPAWN))
                                guidEventTentacles.push_back(summon->GetGUID());
                    }
                    for (int i = 0; i < 4; ++i)
                    {
                        if (Creature* summon = DoSummon(ENTRY_LAUGHING_SKULL, dragonSoulSkullLocation[i], 60000, TEMPSUMMON_TIMED_DESPAWN))
                            guidEventSkulls.push_back(summon->GetGUID());
                    }

                    }
                    break;
                case ACTION_PORTAL_TO_MADNESS_LICHKING:
                    currentBrainEventPhase = PORTAL_PHASE_LICH_KING;
                    guidEventTentacles.clear();
                    guidEventSkulls.clear();
                    for (int i = 0; i < MAX_LICHKING_TENTACLE_SPAWNS; ++i)
                    {
                        if (Creature* summon = DoSummon(ENTRY_DEATHSWORN_ZEALOT, lichKingTentacleLocation[i], 60000, TEMPSUMMON_TIMED_DESPAWN))
                            guidEventTentacles.push_back(summon->GetGUID());
                    }
                    for (int i = 0; i < 3; ++i)
                    {
                        if (Creature* summon = DoSummon(ENTRY_LAUGHING_SKULL, lichkingSkullLocation[i], 60000, TEMPSUMMON_TIMED_DESPAWN))
                            guidEventSkulls.push_back(summon->GetGUID());
                    }
                    break;
                case ACTION_PORTAL_TO_MADNESS_STORMWIND:
                    currentBrainEventPhase = PORTAL_PHASE_KING_LLIANE;
                    guidEventTentacles.clear();
                    guidEventSkulls.clear();
                    for (int i = 0; i < MAX_LLIANE_TENTACLE_SPAWNS; ++i)
                    {
                        if (Creature* summon = DoSummon(ENTRY_SUIT_OF_ARMOR, kingLlaneTentacleLocation[i], 60000, TEMPSUMMON_TIMED_DESPAWN))
                            guidEventTentacles.push_back(summon->GetGUID());
                    }
                    for (int i = 0; i < 3; ++i)
                    {
                        if (Creature* summon = DoSummon(ENTRY_LAUGHING_SKULL, kingLlaneSkullLocation[i], 60000, TEMPSUMMON_TIMED_DESPAWN))
                            guidEventSkulls.push_back(summon->GetGUID());
                    }
                    break;
                default:
                    break;
            }

            if (Creature* yoggbrain = ObjectAccessor::GetCreature(*me, guidYoggBrain))
            {
                yoggbrain->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                yoggbrain->CastSpell(yoggbrain,SPELL_INDUCE_MADNESS, false);
            }

            SummonMadnesseventNPCs();
            isEventSpeaking = true;
            eventSpeakingPhase = 0;
            brainEventsCount++;
        }

        uint32 DoEventSpeaking(BrainEventPhase phase, uint32 part)
        {
            ObjectGuid npcguid = ObjectGuid::Empty;
            uint32 speachindex = 0;
            switch (phase)
            {
                case PORTAL_PHASE_KING_LLIANE:
                    speachindex = 0;
                    break;
                case PORTAL_PHASE_LICH_KING:
                    speachindex = 8;
                    break;
                case PORTAL_PHASE_DRAGON_SOUL:
                    speachindex = 14;
                    break;
                default:
                    break;
            }

            if (phase+speachindex > 18)
                return 5000;

            if (eventNPCSpeaching[speachindex+part].npc_entry != ENTRY_YOGG_SARON)
                npcguid = GeteventNPCGuid(eventNPCSpeaching[speachindex+part].npc_entry);
            else
                npcguid = guidYogg;

            if (Creature* speaker = ObjectAccessor::GetCreature(*me, npcguid))
                speaker->AI()->Talk(eventNPCSpeaching[speachindex + part].speech_entry);
            isEventSpeaking = eventNPCSpeaching[speachindex+part].isSpeaking;
            return eventNPCSpeaching[speachindex+part].next_speech;
        }

        void resetAchievementProgressComingOutOfTheWalls()
        {
            deadGuardianCounter = 0;
            counterStarted = false;
        }

    };
};

class npc_ominous_cloud : public CreatureScript
{
public:
    npc_ominous_cloud() : CreatureScript("npc_ominous_cloud") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ominous_cloudAI (creature);
    }

    struct npc_ominous_cloudAI : public ScriptedAI
    {
        npc_ominous_cloudAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            active = false;
        }

        InstanceScript* instance;
        bool active;

        void MoveInLineOfSight(Unit* target)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) == IN_PROGRESS)
            {
                if (target && me->GetDistance2d(target) <= 5 && target->ToPlayer() && !target->ToPlayer()->IsGameMaster())
                {
                    if (!me->HasAura(SPELL_SUMMON_GUARDIAN))
                    {
                        DoAction(ACTION_SPAWN_GUARDIAN_OF_YOGG_SARON);
                    }
                 }
            }
        }

        void AttackStart(Unit* /*attacker*/) {}

        void DamageTaken(Unit* /*dealer*/, uint32 &damage)
        {
            damage = 0;
        }


        void JustSummoned(Creature* summoned)
        {
            switch (summoned->GetEntry())
            {
                case ENTRY_GUARDIAN_OF_YOGG_SARON:
                    summoned->setFaction(14);
                    if (summoned->AI())
                        summoned->AI()->AttackStart(SelectRandomPlayer(me, 250.0f));

                    // Prevent buggy cloud spawning in later phases
                    if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                    {
                        if (sara->AI()->GetData(DATA_SARA_PHASE) != PHASE_SARA)
                        {
                            summoned->DespawnOrUnsummon();
                            me->AI()->DoAction(ACTION_DEACTIVATE_CLOUDS);
                        }
                    }
                    if (!active)
                    {
                        summoned->DespawnOrUnsummon();
                        me->AI()->DoAction(ACTION_DEACTIVATE_CLOUDS);
                    }
            }
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_DEACTIVATE_CLOUDS:
                    active = false;

                    me->RemoveAurasDueToSpell(SPELL_SUMMON_GUARDIAN);
                    me->RemoveAurasDueToSpell(SPELL_OMINOUS_CLOUD_EFFECT);
                    me->SetReactState(REACT_PASSIVE);
                    break;

                case ACTION_ACTIVATE_CLOUDS:
                    active = true;

                    Reset();
                    me->SetReactState(REACT_AGGRESSIVE);
                    break;

                case ACTION_SPAWN_GUARDIAN_OF_YOGG_SARON: // summon guardian if bossfight in progress, correct phase and active
                    // check if already casting
                    if (me->HasAura(SPELL_SUMMON_GUARDIAN))
                    {
                        return;
                    }

                    // check if bossfight in progress, else stop
                    if (!instance || instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
                    {
                        me->AI()->DoAction(ACTION_DEACTIVATE_CLOUDS);
                        return;
                    }

                    // check if sarah in correct phase, else stop
                    if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                    {
                        if ((sara->AI()->GetData(DATA_SARA_PHASE) != PHASE_SARA))
                        {
                            me->AI()->DoAction(ACTION_DEACTIVATE_CLOUDS);
                            return;
                        }
                    }

                    // check if active, else stop
                    if (!active)
                    {
                        me->AI()->DoAction(ACTION_DEACTIVATE_CLOUDS);
                        return;
                    }

                    DoCastSelf(SPELL_SUMMON_GUARDIAN, true);
            }
        }

        void Reset()
        {
            if (!active)
                return;

            DoCastSelf(SPELL_OMINOUS_CLOUD_EFFECT, true); // must be here, aura must be added after bossreset
            me->RemoveAurasDueToSpell(SPELL_SUMMON_GUARDIAN);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->GetMotionMaster()->MoveRandom(25.0f);
        }

        void UpdateAI(uint32 /*diff*/) {}
    };
};

class npc_guardian_of_yogg_saron : public CreatureScript
{
public:
    npc_guardian_of_yogg_saron() : CreatureScript("npc_guardian_of_yogg_saron") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_guardian_of_yogg_saronAI (creature);
    }

    struct npc_guardian_of_yogg_saronAI : public ScriptedAI
    {
        npc_guardian_of_yogg_saronAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->setFaction(14);
        }

        InstanceScript* instance;
        uint32 darkVolleyTimer;

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (target && target->ToPlayer() && spell->Id == SPELL_DARK_VOLLEY)
            {
                if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                    CAST_AI(boss_sara::boss_saraAI,sara->AI())->ModifySanity(target->ToPlayer(),-2);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA)));
           
            DoCastSelf(SPELL_SHADOW_NOVA, true);
            
            if (instance)
            {
                if (sara)
                {
                    //The achievement shouldn't progress due to despawning and dying adds during the reset
                    //The reset happens in phase = PHASE_NONE, so we only add to the count in other phases
                    if (sara->AI()->GetData(DATA_SARA_PHASE) != PHASE_NONE)
                    sara->AI()->DoAction(ACTION_GUARDIAN_KILLED_ACHIEVMENT_PROGRESS);
                    
                    if (me->GetDistance2d(sara) <= 15)
                        sara->AI()->DoAction(ACTION_NOVA_HIT);
                }
            }
            
            me->DespawnOrUnsummon(5000);
        }

        void Reset()
        {
            darkVolleyTimer = urand(15000,20000);
            if (Unit* target = SelectRandomPlayer(me, 100.0f))
                me->AI()->AttackStart(target);
        }

        void UpdateAI(uint32 diff)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
            {
                me->DealDamage(me, me->GetMaxHealth());
                me->RemoveCorpse();
            }

            if (!UpdateVictim())
                return;

            if (darkVolleyTimer <= diff)
            {
                DoCast(SPELL_DARK_VOLLEY);
                darkVolleyTimer = urand(15000,20000);
            }else darkVolleyTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

class npc_crusher_tentacle : public CreatureScript
{
public:
    npc_crusher_tentacle() : CreatureScript("npc_crusher_tentacle") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_crusher_tentacleAI (creature);
    }

    struct npc_crusher_tentacleAI : public ScriptedAI
    {
        npc_crusher_tentacleAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            once = false;
            me->setFaction(14);
            me->SetCanFly(true);
            if (instance)
            {
                if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                    sara->AI()->JustSummoned(me);
            }

            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ERUPT, true);
        }

        InstanceScript* instance;
        bool once;
        uint32 diminishPower;

        void DamageTaken(Unit* dealer, uint32 &/*damage*/)
        {
            if (me->IsNonMeleeSpellCast(false) && dealer && dealer->IsWithinMeleeRange(me) && (dealer->IsPet() || dealer->IsHunterPet() || dealer->ToPlayer()))
            {
                me->InterruptNonMeleeSpells(false);
                diminishPower = urand(3*SEC,7*SEC);
            }
        }

        void MoveInLineOfSight(Unit* target)
        {
            if (target && me->GetDistance2d(target) <= me->GetMeleeRange(target) && !(target->ToPlayer() && target->ToPlayer()->IsGameMaster()))
                AttackStartNoMove(target);
        }

        void AttackStart(Unit* /*target*/) {}

        void JustDied(Unit* /*killer*/)
        {
            me->RemoveAurasDueToSpell(SPELL_TENTACLE_VOID_ZONE);
            me->DespawnOrUnsummon(5000);
        }

        void Reset()
        {
            if (Unit* target = SelectRandomPlayer(me, 500.0f))
                AttackStartNoMove(target);

            DoCastSelf(SPELL_TENTACLE_VOID_ZONE, true);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat();

            diminishPower = urand(5*SEC, 10*SEC);
            me->AddAura(SPELL_CRUSH, me);
            DoCast(SPELL_FOCUS_ANGER);
        }

        void UpdateAI(uint32 diff)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
            {
                me->DealDamage(me, me->GetMaxHealth());
                me->RemoveCorpse();
            }

            if (!once)
            {
                DoCast(SPELL_ERUPT);
                once = true;
            }

            if (!UpdateVictim())
                return;

            if (diminishPower <= diff)
            {
                if (!me->IsNonMeleeSpellCast(false))
                    DoCast(SPELL_DIMISH_POWER);
            } else diminishPower -= diff;

            if (me->GetVictim() && (me->GetVictim()->ToPlayer() ||me->GetVictim()->IsPet() || me->GetVictim()->IsHunterPet()))
                DoMeleeAttackIfReady();
        }
    };
};

class npc_corruptor_tentacle : public CreatureScript
{
public:
    npc_corruptor_tentacle() : CreatureScript("npc_corruptor_tentacle") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_corruptor_tentacleAI (creature);
    }

    struct npc_corruptor_tentacleAI : public ScriptedAI
    {
        npc_corruptor_tentacleAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            once = false;
            me->setFaction(14);
            me->SetCanFly(true);
            if (instance)
            {
                if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                    sara->AI()->JustSummoned(me);
            }

            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ERUPT, true);
        }

        InstanceScript* instance;
        bool once;
        uint32 spellTimer;

        void MoveInLineOfSight(Unit* target)
        {
            if (target && me->GetDistance2d(target) <= me->GetMeleeRange(target) && !(target->ToPlayer() && target->ToPlayer()->IsGameMaster()))
                AttackStartNoMove(target);
        }

        void AttackStart(Unit* /*target*/) {}

        void JustDied(Unit* /*killer*/)
        {
            me->RemoveAurasDueToSpell(SPELL_TENTACLE_VOID_ZONE);
            me->DespawnOrUnsummon(5000);
        }

        void Reset()
        {
            if (Unit* target = SelectRandomPlayer(me, 500.0f))
                AttackStartNoMove(target);

            DoCastSelf(SPELL_TENTACLE_VOID_ZONE, true);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat();
            spellTimer = urand(5000,10000);
        }

        void UpdateAI(uint32 diff)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
            {
                me->DealDamage(me, me->GetMaxHealth());
                me->RemoveCorpse();
            }

            if (!once)
            {
                DoCast(SPELL_ERUPT);
                once = true;
            }

            if (!UpdateVictim())
                return;

            if (spellTimer <= diff)
            {
                if (Unit* target = SelectRandomPlayer(me, 500.0f))
                            DoCast(target,RAND(SPELL_DRAINING_POISON,SPELL_BLACK_PLAGUE,SPELL_APATHY,SPELL_CURSE_OF_DOOM), false);
                spellTimer = urand(5000,7000);
            } else spellTimer -= diff;
        }
    };
};


class npc_constrictor_tentacle : public CreatureScript
{
public:
    npc_constrictor_tentacle() : CreatureScript("npc_constrictor_tentacle") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_constrictor_tentacleAI (creature);
    }

    struct npc_constrictor_tentacleAI : public ScriptedAI
    {
        npc_constrictor_tentacleAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            once = false;
            me->setFaction(14);
            me->SetCanFly(true);
            if (instance)
            {
                if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                    sara->AI()->JustSummoned(me);
            }
            grabbedPlayerGUID.Clear();

            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ERUPT, true);
        }

        InstanceScript* instance;
        bool once;
        ObjectGuid grabbedPlayerGUID;

        void IsSummonedBy(Unit* summoner)
        {
            summoner->CastSpell(me, SPELL_LUNGE, true);
            me->AddAura(SPELL_SQUEEZE, summoner);
            grabbedPlayerGUID = summoner->GetGUID();
        }

        void DamageTaken(Unit* dealer, uint32 &damage)
        {
            if (dealer && dealer->GetGUID() == grabbedPlayerGUID)
                damage = 0;
        }

        void MoveInLineOfSight(Unit* target)
        {
            if (target && me->GetDistance2d(target) <= me->GetMeleeRange(target) && !(target->ToPlayer() && target->ToPlayer()->IsGameMaster()))
                AttackStartNoMove(target);
        }

        void AttackStart(Unit* /*target*/) {}

        void JustDied(Unit* /*killer*/)
        {
            me->RemoveAurasDueToSpell(SPELL_TENTACLE_VOID_ZONE);

            if (Unit* player = ObjectAccessor::GetUnit(*me, grabbedPlayerGUID))
            {
                player->RemoveAurasDueToSpell(SPELL_LUNGE);
                player->RemoveAurasDueToSpell(SPELL_SQUEEZE);
                player->RemoveAurasDueToSpell(SPELL_SQUEEZE+1); // 25man
                grabbedPlayerGUID.Clear();
            }
            me->DespawnOrUnsummon(5000);
        }

        void Reset()
        {
            if (Unit* target = SelectRandomPlayer(me, 500.0f))
                AttackStartNoMove(target);

            DoCastSelf(SPELL_TENTACLE_VOID_ZONE, true);
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoZoneInCombat();
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
            {
                me->DealDamage(me, me->GetMaxHealth());
                me->RemoveCorpse();
            }

            if (!once)
            {
                DoCast(SPELL_ERUPT);
                once = true;
            }

            if (!UpdateVictim())
                return;
        }
    };
};

class npc_descend_into_madness : public CreatureScript
{
public:
    npc_descend_into_madness() : CreatureScript("npc_descend_into_madness") { }

    bool OnGossipHello(Player* player,Creature* creature)
    {
        if (!player || !creature)
            return false;

        player->PlayerTalkClass->ClearMenus();

        bool AcceptTeleport = false;
        Position posTeleportPosition;
        BrainEventPhase pTemp = PORTAL_PHASE_BRAIN_NONE;
        if (creature->AI())
            pTemp = CAST_AI(npc_descend_into_madnessAI,creature->AI())->bPhase;

        switch (pTemp)
        {
            case PORTAL_PHASE_KING_LLIANE:
            case PORTAL_PHASE_DRAGON_SOUL:
            case PORTAL_PHASE_LICH_KING:
                AcceptTeleport = true;
                posTeleportPosition = innerbrainLocation[pTemp];
                break;
            default:
                AcceptTeleport = false;
                return true;
        }

        if (AcceptTeleport)
        {
            player->RemoveAura(SPELL_DIMISH_POWER);
            player->ApplySpellImmune(0, IMMUNITY_ID, SPELL_DIMISH_POWER, true);
            player->RemoveAurasDueToSpell(SPELL_BRAIN_LINK);
            creature->AddAura(SPELL_ILLUSION_ROOM,player);
            player->NearTeleportTo(posTeleportPosition.m_positionX,posTeleportPosition.m_positionY,posTeleportPosition.m_positionZ,posTeleportPosition.m_orientation, true);
            CAST_AI(npc_descend_into_madnessAI,creature->AI())->bPhase = PORTAL_PHASE_BRAIN_NONE;
            AcceptTeleport = false;
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2,SPELL_ILLUSION_ROOM);
            creature->DespawnOrUnsummon();
        }

        return true;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_descend_into_madnessAI (creature);
    }

    struct npc_descend_into_madnessAI : public ScriptedAI
    {
        npc_descend_into_madnessAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ERUPT, true);
        }

        void DamageTaken(Unit* /*dealer*/, uint32 &damage)
        {
            damage = 0;
        }

        BrainEventPhase bPhase;

        void DoAction(int32 action)
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            switch (action)
            {
                case ACTION_PORTAL_TO_MADNESS_STORMWIND:
                    bPhase = PORTAL_PHASE_KING_LLIANE;
                    break;
                case ACTION_PORTAL_TO_MADNESS_DRAGON:
                    bPhase = PORTAL_PHASE_DRAGON_SOUL;
                    break;
                case ACTION_PORTAL_TO_MADNESS_LICHKING:
                    bPhase = PORTAL_PHASE_LICH_KING;
                    break;
            }
        }

        void Reset()
        {
            bPhase = PORTAL_PHASE_BRAIN_NONE;
        }

        void UpdateAI(uint32 /*diff*/) {}
    };
};


class npc_brain_of_yogg_saron : public CreatureScript
{
public:
    npc_brain_of_yogg_saron() : CreatureScript("npc_brain_of_yogg_saron") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_brain_of_yogg_saronAI (creature);
    }

    struct npc_brain_of_yogg_saronAI : public ScriptedAI
    {
        npc_brain_of_yogg_saronAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->setFaction(14);
            me->SetCanFly(true);
        }

        void Reset()
        {
            checkPlayersTimer = 5*SEC;
            yoggsaron = me->FindNearestCreature(ENTRY_YOGG_SARON, 500.0f);
        }

        Unit* yoggsaron;            // pointer should be vaild, if Yogg Saron despawns no brain is present or attackable
        uint32 checkPlayersTimer;

        void DamageTaken(Unit* /*dealer*/, uint32 &damage)
        {
            if (damage > me->GetHealth())
                damage = me->GetHealth()-1;

            if (yoggsaron)
            {
                yoggsaron->SetHealth((uint32) (me->GetHealthPct() * 0.01 * yoggsaron->GetMaxHealth()));
            }

            if (HealthBelowPct(31))
            {
                if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                        sara->AI()->DoAction(ACTION_BRAIN_UNDER_30_PERCENT);

                if (!me->HasAura(SPELL_BRAIN_HURT_VISUAL))
                    DoCastSelf(SPELL_BRAIN_HURT_VISUAL, true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }
        }

        void SpellHitTarget(Unit *target, SpellInfo const* spell)
        {
            if (!target->ToPlayer())
                return;

            if (spell->Id == SPELL_INDUCE_MADNESS)
            {
                if (isPlayerInBrainRoom(target->ToPlayer()))
                {
                    // player didn't leave brainroom early enough - remove sanity (yogg will make him insane)

                    target->RemoveAurasDueToSpell(SPELL_SANITY);
                }
            }
        }

        InstanceScript* instance;

        void UpdateAI(uint32 diff)
        {
            if (me->IsNonMeleeSpellCast(false)) // brain is casting -> during brain phase
            {
                if (checkPlayersTimer <= diff)
                {
                    if (me->GetMap()->GetPlayers().empty())
                        return;

                    // First iteration - Remove players without Illusion Room buff
                    for (Player& player : me->GetMap()->GetPlayers())
                        if (player.IsAlive() && isPlayerInBrainRoom(&player) && !player.IsGameMaster() && !player.HasAura(SPELL_ILLUSION_ROOM))
                        {
                            player.ApplySpellImmune(0, IMMUNITY_ID, SPELL_DIMISH_POWER, false);
                            player.NearTeleportTo(saraLocation.GetPositionX(), saraLocation.GetPositionY(), saraLocation.GetPositionZ(), static_cast<float>(M_PI), false);
                            player.JumpTo(40.0f, 15.0f, true);
                        }

                    // Second iteration - Remove superflous players
                    uint8 countPlayersInBrainRoom = 0;
                    for (Player& player : me->GetMap()->GetPlayers())
                        if (player.IsAlive() && isPlayerInBrainRoom(&player) && !player.IsGameMaster())
                        {
                            countPlayersInBrainRoom++;

                            if (countPlayersInBrainRoom > RAID_MODE(MAX_BRAIN_ROOM_10, MAX_BRAIN_ROOM_25)) // too many players -> remove superflous ones
                            {
                                player.ApplySpellImmune(0, IMMUNITY_ID, SPELL_DIMISH_POWER, false);
                                player.RemoveAurasDueToSpell(SPELL_ILLUSION_ROOM);
                                player.NearTeleportTo(saraLocation.GetPositionX(), saraLocation.GetPositionY(), saraLocation.GetPositionZ(), static_cast<float>(M_PI), false);
                                player.JumpTo(40.0f, 15.0f, true);
                            }
                        }

                    checkPlayersTimer = urand(5*SEC, 10*SEC);
                }
                else {
                    checkPlayersTimer -= diff;
                }
            } // if casting
        }
    };
};

class boss_yogg_saron : public CreatureScript
{
public:
    boss_yogg_saron() : CreatureScript("boss_yogg_saron") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_yogg_saronAI (creature);
    }

    struct boss_yogg_saronAI : public ScriptedAI
    {
        boss_yogg_saronAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetUnitMovementFlags(MOVEMENTFLAG_HOVER | MOVEMENTFLAG_SWIMMING);
            bCreateValanyr = false;
        }

        InstanceScript* instance;
        uint32 uiSanityCheckTimer;
        bool usedMindcontroll;
        bool bCreateValanyr;

        void Reset()
        {
            uiSanityCheckTimer = 1000;
            usedMindcontroll = false;
        }

        void CreateValanyr()
        {
            if (!bCreateValanyr)
                return;

            if (me->GetMap() && me->GetMap()->IsDungeon())
            {
                for (Player& player : me->GetMap()->GetPlayers())
                    if (player.IsAlive() && player.GetQuestStatus(13629) == QUEST_STATUS_INCOMPLETE)
                    {
                        if (!player.HasItemCount(45897,1, true))
                            player.AddItem(45897,1);
                    }
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
            {
                TC_LOG_DEBUG("entities.player", "Sara found, action was called");
                sara->AI()->DoAction(ACTION_YOGGSARON_KILLED);
            }

            me->AI()->Talk(SAY_DEATH);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SANITY_WELL);
            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_SANITY_WELL_DEBUFF);

            CreateValanyr();
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (!instance) return;

            if (target && target->ToPlayer())
            {
                switch (spell->Id)
                {
                case SPELL_LUNATIC_GAZE_EFFECT:
                    if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                        CAST_AI(boss_sara::boss_saraAI,sara->AI())->ModifySanity(target->ToPlayer(),-4);
                    break;
                }
            }
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_IN_THE_MAWS_OF_THE_OLD_GOD)
            {
                bCreateValanyr = true;
            }
        }

         void DoAction(int32 action)
        {
            switch (action)
            {
                // a player has thrown "Unbound Fragments of Val'anyr" on Yogg
                // that yogg is casting Defaning Roar (and thus, we are in 25man hardmode) is checked via item script
                case ACTION_HIT_BY_VALANYR:
                    bCreateValanyr = true;
                    if (!me->HasAura(SPELL_IN_THE_MAWS_OF_THE_OLD_GOD))
                        me->AddAura(SPELL_IN_THE_MAWS_OF_THE_OLD_GOD, me);
                    break;
            }
        }
        void UpdateAI(uint32 diff)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
                return;

            if (uiSanityCheckTimer <= diff)
            {
                if (me->GetMap() && me->GetMap()->IsDungeon())
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                    {
                        if (player.IsAlive() && !player.IsGameMaster())
                        {
                            if (!player.HasAura(SPELL_SANITY) && !player.HasAura(SPELL_INSANE)) // player has no sanity left -> make him insane
                            {
                                // Say Sara we used Mindcontrol - for Archievment ... we need only one mindecontrolled Player to cancel Achievment
                                if (!usedMindcontroll)
                                {
                                    if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                                        sara->AI()->DoAction(ACTION_USED_MINDCONTROL);
                                    usedMindcontroll = true;
                                }

                                if (isPlayerInBrainRoom(&player))
                                {
                                    player.RemoveAurasDueToSpell(SPELL_ILLUSION_ROOM);
                                    player.NearTeleportTo(saraLocation.GetPositionX(), saraLocation.GetPositionY(), saraLocation.GetPositionZ(), static_cast<float>(M_PI), false);
                                }

                                me->CastSpell(&player, SPELL_INSANE);
                                me->AddAura(SPELL_INSANE_2, &player);

                                Talk(RAND(WHISP_INSANITY_1, WHISP_INSANITY_2));
                            }
                        }
                    }
                }

                uiSanityCheckTimer = 1000;
            } else uiSanityCheckTimer -= diff;
        }
    };
};

class npc_influence_tentacle : public CreatureScript
{
public:
    npc_influence_tentacle() : CreatureScript("npc_influence_tentacle") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_influence_tentacleAI (creature);
    }

    struct npc_influence_tentacleAI : public ScriptedAI
    {
        npc_influence_tentacleAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            me->SetReactState(REACT_DEFENSIVE);
        }

        void JustDied(Unit* /*killer*/)
        {
            me->RemoveAurasDueToSpell(SPELL_TENTACLE_VOID_ZONE);
        }

        void Reset()
        {
        }

        void DamageTaken(Unit* dealer, uint32 &damage)
        {
            if (dealer->ToPlayer())
                me->CastCustomSpell(SPELL_GRIM_REPRISAL_DAMAGE, SPELLVALUE_BASE_POINT0, int32(damage *0.60), dealer, true);
        }

        void EnterCombat(Unit* /*attacker*/)
        {
            if (me->GetEntry() != ENTRY_INFULENCE_TENTACLE)
            {
                me->UpdateEntry(ENTRY_INFULENCE_TENTACLE);

                if (Is25ManRaid())
                    me->SetMaxHealth(46000); // 40k * 1.15 RG Custom Bonus

                me->setFaction(14);
                me->setRegeneratingHealth(false);
                me->SetFullHealth();

                //DoCast(SPELL_GRIM_REPRISAL);
                DoCastSelf(SPELL_TENTACLE_VOID_ZONE, true);
            }
        }

        // overwrite to avoid meleehit
        void UpdateAI(uint32 /*diff*/) {}
    };
};

class npc_immortal_guardian : public CreatureScript
{
public:
    npc_immortal_guardian() : CreatureScript("npc_immortal_guardian") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_immortal_guardianAI (creature);
    }

    struct npc_immortal_guardianAI : public ScriptedAI
    {
        npc_immortal_guardianAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();

            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;

        uint32 drainLifeTimer;
        uint32 uiAttackTimer;
        uint32 uiSpawnVisualTimer;

        void Reset()
        {
            drainLifeTimer = 10000;
            uiAttackTimer = 3500;
            uiSpawnVisualTimer = 1000;
        }

        void JustDied(Unit* /*killer*/)
        {
            me->DespawnOrUnsummon(5000);
        }

        void DamageTaken(Unit* dealer, uint32 &damage)
        {
            if (dealer->GetGUID() == me->GetGUID())
                return;

            if (me->GetHealth() <= damage)
                damage = me->GetHealth()-1;
        }

        void UpdateAI(uint32 diff)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
            {
                me->DealDamage(me, me->GetMaxHealth());
                me->RemoveCorpse();
            }

            if (me->HasAura(SPELL_SHADOW_BEACON)) // does have Shadow Beacon - should be displayed as marked guardian
            {
                if (me->GetEntry() == ENTRY_IMMORTAL_GUARDIAN)
                    me->UpdateEntry(ENTRY_IMMORTAL_GURDIAN_MARKED);

            }
            else // does not have Shadow Beacon - should be displayed normally
            {
                if (me->GetEntry() == ENTRY_IMMORTAL_GURDIAN_MARKED)
                    me->UpdateEntry(ENTRY_IMMORTAL_GUARDIAN);
            }

            {
                uint32 stacks = uint32(me->GetHealthPct() / 10);
                if (stacks > 9) stacks = 9;

                if (stacks > 0)
                {
                    me->RemoveAurasDueToSpell(SPELL_WEAKENED);
                    if (!me->HasAura(SPELL_EMPOWERED))
                        me->AddAura(SPELL_EMPOWERED, me);
                    me->SetAuraStack(SPELL_EMPOWERED, me, stacks);
                }else
                {
                    me->RemoveAurasDueToSpell(SPELL_EMPOWERED);
                    if (!me->HasAura(SPELL_WEAKENED))
                        me->AddAura(SPELL_WEAKENED, me);
                }
            }

            if (me->HasReactState(REACT_PASSIVE))
            {
                if (uiSpawnVisualTimer <= diff)
                {
                    DoCastSelf(SPELL_SUMMON_GUARDIAN_VISUAL);
                    uiSpawnVisualTimer = 250;

                } else uiSpawnVisualTimer -= diff;


                if (uiAttackTimer <= diff)
                {
                    me->SetReactState(REACT_AGGRESSIVE);

                    if (Unit* target = SelectRandomPlayer(me, 100.0f))
                        me->AI()->AttackStart(target);

                } else uiAttackTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (drainLifeTimer < diff)
            {
                DoCastVictim(RAID_MODE(SPELL_DRAIN_LIFE_10, SPELL_DRAIN_LIFE_25));
                drainLifeTimer = urand(35000, 50000);
            } else drainLifeTimer -= diff;


            DoMeleeAttackIfReady();
        }

    };
};

class AllSaronitCreaturesInRange
{
    public:
        AllSaronitCreaturesInRange(const WorldObject* object, float maxRange) : m_object(object), m_fRange(maxRange) {}
        bool operator() (Unit* unit)
        {
            if (IsSaronitEntry(unit->GetEntry()) && m_object->IsWithinDist(unit, m_fRange, false) && unit->IsAlive())
                return true;

            return false;
        }

    private:
        const WorldObject* m_object;
        float m_fRange;

        bool IsSaronitEntry(uint32 entry)
        {
            switch (entry)
            {
            case ENTRY_GUARDIAN_OF_YOGG_SARON:
            case ENTRY_CONSTRICTOR_TENTACLE:
            case ENTRY_CORRUPTOR_TENTACLE:
            case ENTRY_CRUSHER_TENTACLE:
            case ENTRY_IMMORTAL_GUARDIAN:
                return true;
            }
            return false;
        }
};

class npc_support_keeper : public CreatureScript
{
public:
    npc_support_keeper() : CreatureScript("npc_support_keeper") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_support_keeperAI (creature);
    }

    struct npc_support_keeperAI : public ScriptedAI
    {
        npc_support_keeperAI(Creature* creature) : ScriptedAI(creature) , summons(me)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            isActive = false;

            auraspell = 0;
            switch (me->GetEntry())
            {
                case NPC_KEEPER_YOGGROOM_HODIR:
                    auraspell = SPELL_FORTITUDE_OF_FROST;
                    break;
                case NPC_KEEPER_YOGGROOM_FREYA:
                    auraspell = SPELL_RESILIENCE_OF_NATURE;
                    break;
                case NPC_KEEPER_YOGGROOM_THORIM:
                    auraspell = SPELL_FURY_OF_THE_STORM;
                    break;
                case NPC_KEEPER_YOGGROOM_MIMIRON:
                    auraspell = SPELL_SPEED_OF_INVENTION;
                    break;
            }

        }

        SummonList summons;
        InstanceScript* instance;

        // general handling
        bool isActive;
        uint32 secondarySpellTimer;
        uint32 auraspell;

        // freya sanity well handling
        bool summoning;
        bool summoned;

        void AttackStart(Unit* /*attacker*/) {}

        void Reset() {}

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_DEACTIVATE_KEEPER:
                {
                    isActive = false;
                    me->RemoveAurasDueToSpell(auraspell);
                    me->InterruptNonMeleeSpells(true);

                    if (me->GetEntry() == NPC_KEEPER_YOGGROOM_HODIR)
                        me->RemoveAurasDueToSpell(SPELL_HODIRS_PROTECTIVE_GAZE);
                    if (me->GetEntry() == NPC_KEEPER_YOGGROOM_THORIM)
                        me->RemoveAurasDueToSpell(SPELL_TITANIC_STORM);

                    summons.DespawnAll();
                    DoUnsummonWells();

                    break;
                }
                case ACTION_ACTIVATE_KEEPER:
                {
                    isActive = true;
                    DoCast(auraspell);
                    summoning = false;
                    summoned = false;
                    secondarySpellTimer = 10000;
                    break;
                }
            }
        }


        void JustSummoned(Creature* summoned)
        {
            summons.Summon(summoned);
        }

        void DosummonsanityWells()
        {
            for (uint8 i = 0; i < 5 ; i++)
                DoSummon(ENTRY_SANITY_WELL, freyaSanityWellLocation[i], 0, TEMPSUMMON_MANUAL_DESPAWN);
        }

        void DoUnsummonWells()
        {
            std::list<Creature*> WellList;
            me->GetCreatureListWithEntryInGrid(WellList, ENTRY_SANITY_WELL, 500.0f);
            if (!WellList.empty())
                for (std::list<Creature*>::iterator itr = WellList.begin(); itr != WellList.end(); itr++)
                    (*itr)->DespawnOrUnsummon();
        }

        void GetAliveSaronitCreatureListInGrid(std::list<Creature*>& list, float maxSearchRange)
        {
            AllSaronitCreaturesInRange check(me, maxSearchRange);

            me->VisitAnyNearbyObject<Creature, Trinity::ContainerAction>(maxSearchRange, list, check);
        }

        void UpdateAI(uint32 diff)
        {
            if (!isActive)
                return;

            if (secondarySpellTimer <= diff)
            {
                switch (me->GetEntry())
                {
                    case NPC_KEEPER_YOGGROOM_THORIM:
                        if (!me->HasAura(SPELL_TITANIC_STORM))
                            DoCast(SPELL_TITANIC_STORM);

                        secondarySpellTimer = 10000;
                        return;
                    case NPC_KEEPER_YOGGROOM_FREYA:
                        if (!summoned)
                        {
                            if (!summoning)
                            {
                                DoCast(SPELL_SANITY_WELL);
                                secondarySpellTimer = 3000;
                                summoning = true;
                            }else
                            {
                                DosummonsanityWells();
                                summoned = true;
                            }
                        }
                        return;
                    case NPC_KEEPER_YOGGROOM_MIMIRON:
                        {
                            std::list<Creature*> creatureList;
                            GetAliveSaronitCreatureListInGrid(creatureList,5000);
                            if (!creatureList.empty())
                            {
                                std::list<Creature*>::iterator itr = creatureList.begin();
                                advance(itr, urand(0, creatureList.size()-1));
                                DoCast((*itr), SPELL_DESTABILIZATION_MATRIX, false);
                            }
                        }
                        secondarySpellTimer = 30000;
                        return;
                    case NPC_KEEPER_YOGGROOM_HODIR:
                        {
                            if (!me->HasAura(SPELL_HODIRS_PROTECTIVE_GAZE))
                                DoCastSelf(SPELL_HODIRS_PROTECTIVE_GAZE, false);
                            secondarySpellTimer = 25000;
                            return;
                        }
                }
                secondarySpellTimer = 10000;
            }
            else
            {
                if (me->GetEntry() == NPC_KEEPER_YOGGROOM_HODIR)
                {
                    if (!me->HasAura(SPELL_HODIRS_PROTECTIVE_GAZE))
                        secondarySpellTimer -= diff;
                }
                else
                    secondarySpellTimer -= diff;
            }
        }
    };
};

class npc_sanity_well : public CreatureScript
{
public:
    npc_sanity_well() : CreatureScript("npc_sanity_well") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_sanity_wellAI (creature);
    }

    struct npc_sanity_wellAI : public ScriptedAI
    {
        npc_sanity_wellAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ERUPT, true);
        }

        InstanceScript* instance;

        uint32 sanityTickTimer;

        void DamageTaken(Unit* /* dealer */, uint32 &damage)
        {
            damage = 0;
        }

        void Reset()
        {
            sanityTickTimer = 2000;
        }

        void AttackStart(Unit* /*who*/) {}

        void MoveInLineOfSight(Unit* mover)
        {
            if (mover && mover->ToPlayer() )
            {
                if (me->IsWithinDist2d(mover, 6))
                {
                    if (!mover->HasAura(SPELL_SANITY_WELL_DEBUFF))
                        me->AddAura(SPELL_SANITY_WELL_DEBUFF, mover);
                }
                else
                {
                    if (mover->HasAura(SPELL_SANITY_WELL_DEBUFF))
                        mover->RemoveAurasDueToSpell(SPELL_SANITY_WELL_DEBUFF, me->GetGUID());
                }
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (instance && instance->GetBossState(BOSS_YOGGSARON) != IN_PROGRESS)
            {
                me->DealDamage(me, me->GetMaxHealth());
                me->RemoveCorpse();
            }

            if (sanityTickTimer <= diff)
            {
                std::list<Player*> plrList;
                me->GetPlayerListInGrid(plrList, 10.f);
                for (std::list<Player*>::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                {
                    if ((*itr))
                    {
                        if ((*itr)->HasAura(SPELL_SANITY_WELL_DEBUFF))
                        {
                            if (Creature* sara = ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(DATA_SARA))))
                                CAST_AI(boss_sara::boss_saraAI, sara->AI())->ModifySanity((*itr),20);
                        }
                    }
                }
                sanityTickTimer = 2000;
            }else sanityTickTimer -= diff;
        }
    };
};

class npc_laughting_skull : public CreatureScript
{
public:
    npc_laughting_skull() : CreatureScript("npc_laughting_skull") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_laughting_skullAI (creature);
    }

    struct npc_laughting_skullAI : public ScriptedAI
    {
        npc_laughting_skullAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (!instance) return;

            if (target && target->ToPlayer())
            {
                switch (spell->Id)
                {
                    case SPELL_LS_LUNATIC_GAZE_EFFECT:
                        if (Creature* sara = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_SARA))))
                            CAST_AI(boss_sara::boss_saraAI, sara->AI())->ModifySanity(target->ToPlayer(),-2);
                        break;
                }
            }
        }

        void AttackStart(Unit* /*who*/) {}

        void UpdateAI(uint32 /*diff*/)
        {
        }
    };
};

class npc_death_orb : public CreatureScript
{
public:
    npc_death_orb() : CreatureScript("npc_death_orb") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_death_orbAI (creature);
    }

    struct npc_death_orbAI : public ScriptedAI
    {
        npc_death_orbAI(Creature* creature) : ScriptedAI(creature) , summons(me)
        {
            SetCombatMovement(false);
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ERUPT, true);
        }

        void DamageTaken(Unit* /* dealer */, uint32 &damage)
        {
            damage = 0;
        }

        SummonList summons;
        InstanceScript* instance;
        bool prepaired;
        uint32 reathRayEffektTimer;

        void Reset()
        {
            me->SetCanFly(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            prepaired = false;
            summons.DespawnAll();
            SummonDeathRays();
            reathRayEffektTimer = 5000;
            DoCastSelf(SPELL_DEATH_RAY_ORIGIN_VISUAL, true);
        }

        void SummonDeathRays()
        {
            for (uint8 i = 0; i < 4; i++)
            {
                Position pos;
                me->GetNearPoint2D(pos.m_positionX, pos.m_positionY, float(rand_norm()*10 + 30), float(2*static_cast<float>(M_PI)*rand_norm()));
                pos.m_positionZ = me->GetPositionZ();
                pos.m_positionZ = me->GetMap()->GetHeight(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), true, 500.0f);

                DoSummon(ENTRY_DEATH_RAY, pos, 20000, TEMPSUMMON_TIMED_DESPAWN);
            }
        }

        void JustSummoned(Creature* summoned)
        {
            switch (summoned->GetEntry())
            {
            case ENTRY_DEATH_RAY:
                summoned->setFaction(14);
                summoned->CastSpell(summoned, SPELL_DEATH_RAY_WARNING_VISUAL, true, 0, 0, me->GetGUID());
                summoned->SetReactState(REACT_PASSIVE);
                summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                break;
            }

            summons.Summon(summoned);
        }

        void AttackStart(Unit* /*who*/) {}

        void UpdateAI(uint32 diff)
        {
            if (reathRayEffektTimer <= diff)
            {
                for (std::list<ObjectGuid>::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                {
                    if (Creature* temp = ObjectAccessor::GetCreature(*me,*itr))
                    {
                        temp->CastSpell(temp, SPELL_DEATH_RAY_PERIODIC, true);
                        temp->CastSpell(temp, SPELL_DEATH_RAY_DAMAGE_VISUAL, true, 0, 0, me->GetGUID());

                        temp->AI()->DoAction(ACTION_DEATH_RAY_MOVE);
                    }
                }
                reathRayEffektTimer = 30000;
            }else reathRayEffektTimer -= diff;
        }
    };
};

class npc_death_ray : public CreatureScript
{
public:
    npc_death_ray() : CreatureScript("npc_death_ray") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_death_rayAI (creature);
    }

    struct npc_death_rayAI : public ScriptedAI
    {
        npc_death_rayAI(Creature* creature) : ScriptedAI(creature)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_ERUPT, true);
        }

        void DamageTaken(Unit* /* dealer */, uint32 &damage)
        {
            damage = 0;
        }

        bool moving;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            moving = true;
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_DEATH_RAY_MOVE)
                moving = false;
        }

        void DoRandomMove()
        {
            Position pos;
            me->GetNearPoint2D(pos.m_positionX, pos.m_positionY, 10, float(2*static_cast<float>(M_PI)*rand_norm()));
            pos.m_positionZ = me->GetPositionZ();
            pos.m_positionZ = me->GetMap()->GetHeight(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), true, 50.0f);
            me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);
            me->GetMotionMaster()->MovePoint(1,pos);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id != 1)
                return;

            moving = false;
        }

        void AttackStart(Unit* /*who*/) {}

        void UpdateAI(uint32 /*diff*/)
        {
            if (!moving)
            {
                DoRandomMove();
                moving = true;
            }
        }
    };
};

class go_flee_to_surface : public GameObjectScript
{
public:
    go_flee_to_surface() : GameObjectScript("go_flee_to_surface") { }

    bool OnGossipHello(Player *player, GameObject * /*go*/)
    {
        // player wants to leave brain room

        player->RemoveAurasDueToSpell(SPELL_ILLUSION_ROOM);
        player->ApplySpellImmune(0, IMMUNITY_ID, SPELL_DIMISH_POWER, false);
        player->NearTeleportTo(saraLocation.GetPositionX(), saraLocation.GetPositionY(), saraLocation.GetPositionZ() ,static_cast<float>(M_PI), false);
        player->JumpTo(40.0f, 15.0f, true);
        return false;
    }
};

class spell_keeper_support_aura_targeting : public SpellScriptLoader
{
    public:
        spell_keeper_support_aura_targeting() : SpellScriptLoader("spell_keeper_support_aura_targeting") { }

    class spell_keeper_support_aura_targeting_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_keeper_support_aura_targeting_AuraScript)

        void HandleEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            std::list<Unit*> targetList;
            aurEff->GetTargetList(targetList);

            for (std::list<Unit*>::iterator iter = targetList.begin(); iter != targetList.end(); ++iter)
                if (!(*iter)->ToPlayer() && (*iter)->GetGUID() != GetCasterGUID() )
                    (*iter)->RemoveAurasDueToSpell(GetSpellInfo()->Id);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_keeper_support_aura_targeting_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_keeper_support_aura_targeting_AuraScript();
    }
};

class DontLooksDirectlyInGazeCheck
{
    public:
        DontLooksDirectlyInGazeCheck(WorldObject* caster) : _caster(caster) { }

        bool operator() (WorldObject* unit)
        {
            Position pos;
            _caster->GetPosition(&pos);
            return !unit->HasInArc(static_cast<float>(static_cast<float>(M_PI)), &pos);
        }

    private:
        WorldObject* _caster;
};

class spell_lunatic_gaze_targeting : public SpellScriptLoader
{
    public:
        spell_lunatic_gaze_targeting() : SpellScriptLoader("spell_lunatic_gaze_targeting") { }

        class spell_lunatic_gaze_targeting_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_lunatic_gaze_targeting_SpellScript)

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if (DontLooksDirectlyInGazeCheck(GetCaster()));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_lunatic_gaze_targeting_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_lunatic_gaze_targeting_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        // function which creates SpellScript
        SpellScript *GetSpellScript() const
        {
            return new spell_lunatic_gaze_targeting_SpellScript();
        }
};

class spell_brain_link_periodic_dummy : public SpellScriptLoader
{
    public:
        spell_brain_link_periodic_dummy() : SpellScriptLoader("spell_brain_link_periodic_dummy") { }

    class spell_brain_link_periodic_dummy_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_brain_link_periodic_dummy_AuraScript)

        void HandlePeriodicDummy(AuraEffect const* aurEff)
        {
            PreventDefaultAction();
            if (Unit* trigger = GetTarget())
            {
                if (trigger->GetTypeId() == TYPEID_PLAYER)
                {
                    if (!trigger->GetMap()->IsDungeon())
                        return;

                    for (Player& player : trigger->GetMap()->GetPlayers())
                        if (player.HasAura(SPELL_BRAIN_LINK) && player.GetGUID() != trigger->GetGUID())
                            {
                                if (!player.HasAura(SPELL_SANITY) || isPlayerInBrainRoom(&player) || isPlayerInBrainRoom(trigger->ToPlayer()))
                                {
                                    player.RemoveAurasDueToSpell(SPELL_BRAIN_LINK);
                                    trigger->RemoveAurasDueToSpell(SPELL_BRAIN_LINK);
                                    return;
                                }

                                if (Unit* caster = GetCaster())
                                {
                                    if (!player.IsWithinDist(trigger, float(aurEff->GetAmount())))
                                    {
                                        trigger->CastSpell(&player, SPELL_BRAIN_LINK_DAMAGE, true);
                                        if (InstanceScript* pInstance = caster->GetInstanceScript())
                                            if (caster->ToCreature() && caster->GetGUID() == ObjectGuid(pInstance->GetGuidData(DATA_SARA)))
                                                CAST_AI(boss_sara::boss_saraAI, caster->ToCreature()->AI())->ModifySanity(&player, -2);
                                    }
                                    else
                                        trigger->CastSpell(&player, SPELL_BRAIN_LINK_DUMMY, true);
                                }
                            }
                }
            }
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_brain_link_periodic_dummy_AuraScript::HandlePeriodicDummy, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_brain_link_periodic_dummy_AuraScript();
    }
};

class spell_titanic_storm : public SpellScriptLoader
{
    public:
        spell_titanic_storm() : SpellScriptLoader("spell_titanic_storm") { }

        class spell_titanic_storm_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_titanic_storm_AuraScript);

            // this is an additional effect to be executed
            void Periodic(AuraEffect const* /*aurEff*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                std::list<Creature*> guardians;
                caster->GetCreatureListWithEntryInGrid(guardians, ENTRY_IMMORTAL_GUARDIAN, 500.0f);

                if (guardians.empty())
                    return;

                for (std::list<Creature*>::const_iterator itr = guardians.begin(); itr != guardians.end(); ++itr)
                {
                    if ((*itr) && (*itr)->IsAlive() && (*itr)->HasAura(SPELL_WEAKENED))
                        caster->Kill((*itr), false);
                }

            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_titanic_storm_AuraScript::Periodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_titanic_storm_AuraScript();
        }
};

class spell_insane_death_effekt : public SpellScriptLoader
{
    public:
        spell_insane_death_effekt() : SpellScriptLoader("spell_insane_death_effekt") { }

        class spell_insane_death_effekt_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_insane_death_effekt_AuraScript)

            void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit * target = GetTarget())
                    if (target->ToPlayer() && target->IsAlive())
                        target->DealDamage(target,target->GetHealth());
            }

            // function registering
            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_insane_death_effekt_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_AOE_CHARM, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript *GetAuraScript() const
        {
            return new spell_insane_death_effekt_AuraScript();
        }
};

class spell_induce_madness : public SpellScriptLoader
{
    public:
        spell_induce_madness() : SpellScriptLoader("spell_induce_madness") { }

        class spell_induce_madness_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_induce_madness_SpellScript);

            void HandleOnCast()
            {
                std::list<Player*> Player_List;
                GetCaster()->GetPlayerListInGrid(Player_List, 100.f);

                for (std::list<Player*>::iterator itr = Player_List.begin(); itr != Player_List.end(); ++itr)
                    if (isPlayerInBrainRoom(*itr))
                        (*itr)->RemoveAurasDueToSpell(SPELL_SANITY);
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_induce_madness_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_induce_madness_SpellScript();
        }
};

//class spell_summon_tentacle_position : public SpellScriptLoader
//{
//    public:
//        spell_summon_tentacle_position() : SpellScriptLoader("spell_summon_tentacle_position") { }
//
//        class spell_summon_tentacle_position_SpellScript : public SpellScript
//        {
//            PrepareSpellScript(spell_summon_tentacle_position_SpellScript);
//
//            void ChangeSummonPos()
//            {
//                WorldLocation summonPos = *GetExplTargetDest();
//                if (Unit* caster = GetCaster())
//                    summonPos.m_positionZ = caster->GetMap()->GetHeight(summonPos.GetPositionX(), summonPos.GetPositionY(), summonPos.GetPositionZ());
//
//                SetExplTargetDest(summonPos);
//            }
//
//            void Register()
//            {
//                OnCast += SpellCastFn(spell_summon_tentacle_position_SpellScript::ChangeSummonPos);
//            }
//        };
//
//        SpellScript* GetSpellScript() const
//        {
//            return new spell_summon_tentacle_position_SpellScript();
//        }
//};

class spell_empowering_shadows : public SpellScriptLoader
{
    public:
        spell_empowering_shadows() : SpellScriptLoader("spell_empowering_shadows") { }

        class spell_empowering_shadows_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_empowering_shadows_SpellScript)

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Unit * target = GetHitUnit())
                {
                    uint32 spell = 0;
                    switch(target->GetMap()->GetDifficulty())
                    {
                        case RAID_DIFFICULTY_10MAN_NORMAL:
                            spell = SPELL_EMPOWERING_SHADOWS_HEAL_10;
                            break;
                        case RAID_DIFFICULTY_25MAN_NORMAL:
                            spell = SPELL_EMPOWERING_SHADOWS_HEAL_25;
                            break;
                        default:
                            break;
                    }
                    if (spell)
                    {
                        GetCaster()->CastSpell(target, spell, true);
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_empowering_shadows_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        // function which creates SpellScript
        SpellScript *GetSpellScript() const
        {
            return new spell_empowering_shadows_SpellScript();
        }
};

// Hodir's Protective Gaze
class spell_hodir_protective_gaze : public SpellScriptLoader
{
public:
    spell_hodir_protective_gaze() : SpellScriptLoader("spell_hodir_protective_gaze") { }

    class spell_hodir_protective_gaze_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_hodir_protective_gaze_AuraScript);

        bool Validate(SpellInfo const * /*spellEntry*/)
        {
            return sSpellStore.LookupEntry(SPELL_FLASH_FREEZE_COOLDOWN);
        }

        void HandleEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            std::list<Unit*> targetList;
            aurEff->GetTargetList(targetList);

            for (std::list<Unit*>::iterator iter = targetList.begin(); iter != targetList.end(); ++iter)
                if (!(*iter)->ToPlayer() && (*iter)->GetGUID() != GetCasterGUID() )
                    (*iter)->RemoveAurasDueToSpell(GetSpellInfo()->Id);
        }

        void CalculateAmount(AuraEffect const * /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
        {
            // Set absorbtion amount to unlimited
            amount = -1;
        }

        void Absorb(AuraEffect * /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
        {
            Unit * target = GetTarget();
            if (dmgInfo.GetDamage() < target->GetHealth())
                return;

            target->CastSpell(target, SPELL_FLASH_FREEZE, true);

            // absorb hp till 1 hp
            absorbAmount = dmgInfo.GetDamage() - target->GetHealth() + 1;

            // Remove Aura from Hodir
            if (GetCaster() && GetCaster()->ToCreature())
                GetCaster()->ToCreature()->RemoveAurasDueToSpell(SPELL_HODIRS_PROTECTIVE_GAZE);
        }

        void Register()
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hodir_protective_gaze_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_hodir_protective_gaze_AuraScript::Absorb, EFFECT_0);
            OnEffectApply += AuraEffectApplyFn(spell_hodir_protective_gaze_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript *GetAuraScript() const
    {
        return new spell_hodir_protective_gaze_AuraScript();
    }
};

class spell_malady_of_the_mind : public SpellScriptLoader
{
    public:
        spell_malady_of_the_mind() : SpellScriptLoader("spell_malady_of_the_mind") { }

        class spell_malady_of_the_mindAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_malady_of_the_mindAuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                return sSpellStore.LookupEntry(SPELL_MALADY_OF_MIND_TRIGGER);
            }

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    if (Unit* caster = GetCaster())
                    {

                        // Sanity Support
                        if (target->IsAlive())
                            if (Aura* aura = target->GetAura(SPELL_SANITY))
                            {
                                uint8 amount = aura->GetStackAmount();
                                amount -= 3;

                                if (amount <= 0)
                                    target->RemoveAurasDueToSpell(SPELL_SANITY);
                                else
                                    aura->SetStackAmount(amount);
                            }

                        if (GetSpellInfo()->Id != SPELL_MALADY_OF_MIND)
                                return;

                        std::list<Player*> Player_List;
                        target->GetPlayerListInGrid(Player_List, 10.f);
                        for (std::list<Player*>::iterator itr = Player_List.begin(); itr != Player_List.end(); ++itr)
                            if (!(*itr)->HasAura(SPELL_MALADY_OF_MIND) && !(*itr)->HasAura(SPELL_MALADY_OF_MIND_TRIGGER) &&
                                (*itr) != target && (*itr)->HasAura(SPELL_SANITY))
                                { // Try to add the Malady of the Mind to another Player in Range
                                    caster->AddAura(SPELL_MALADY_OF_MIND_TRIGGER, (*itr));
                                    return;
                                }
                    }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_malady_of_the_mindAuraScript::HandleOnEffectRemove, EFFECT_1, SPELL_AURA_MOD_FEAR, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_malady_of_the_mindAuraScript();
        }
};

class spell_yogg_tentacle_squeeze : public SpellScriptLoader
{
public:
    spell_yogg_tentacle_squeeze() : SpellScriptLoader("spell_yogg_tentacle_squeeze") { }

    class spell_yogg_tentacle_squeeze_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_yogg_tentacle_squeeze_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_LUNGE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SQUEEZE))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_SQUEEZE+1))
                return false;
            return true;
        }

        void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget())
            {
                GetTarget()->RemoveAura(SPELL_LUNGE);

            }

            if (Unit* caster = GetCaster())
                if (caster->ToCreature() && (caster->ToCreature()->GetEntry() == ENTRY_CONSTRICTOR_TENTACLE || caster->ToCreature()->GetEntry() == ENTRY_CONSTRICTOR_TENTACLE+1))
                    caster->KillSelf();
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_yogg_tentacle_squeeze_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }

    };

    AuraScript* GetAuraScript() const
    {
        return new spell_yogg_tentacle_squeeze_AuraScript();
    }
};

#define GOSSIP_KEEPER_HELP                  "Lend us your aid, keeper. Together we shall defeat Yogg-Saron."

class npc_keeper_help : public CreatureScript
{
public:
    npc_keeper_help() : CreatureScript("npc_keeper_help") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (InstanceScript* instance = creature->GetInstanceScript())
        {
            uint32 supportFlag = instance->GetData(DATA_KEEPER_SUPPORT_YOGG);
            switch (creature->GetEntry())
            {
                case NPC_KEEPER_WALKWAY_FREYA:
                    if (supportFlag & (1 << FREYA))
                        return false;
                    break;
                case NPC_KEEPER_WALKWAY_MIMIRON:
                    if (supportFlag & (1 << MIMIRON))
                        return false;
                    break;
                case NPC_KEEPER_WALKWAY_THORIM:
                    if (supportFlag & (1 << THORIM))
                        return false;
                    break;
                case NPC_KEEPER_WALKWAY_HODIR:
                    if (supportFlag & (1 << HODIR))
                        return false;
                    break;
            }

            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_KEEPER_HELP, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        }
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->CLOSE_GOSSIP_MENU();

        InstanceScript* instance = creature->GetInstanceScript();

        if (!instance) return false;

        if ( action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            switch (creature->GetEntry())
            {
                case NPC_KEEPER_WALKWAY_FREYA:
                    creature->AI()->Talk(SAY_FREYA_HELP);
                    instance->SetData(DATA_ADD_HELP_FLAG, FREYA);
                    creature->GetMap()->SummonCreature(NPC_KEEPER_YOGGROOM_FREYA, yoggroomKeeperSpawnLocation[1]);
                    break;
                case NPC_KEEPER_WALKWAY_MIMIRON:
                    creature->AI()->Talk(SAY_MIMIRON_HELP);
                    instance->SetData(DATA_ADD_HELP_FLAG, MIMIRON);
                    creature->GetMap()->SummonCreature(NPC_KEEPER_YOGGROOM_MIMIRON, yoggroomKeeperSpawnLocation[0]);
                    break;
                case NPC_KEEPER_WALKWAY_THORIM:
                    creature->AI()->Talk(SAY_THORIM_HELP);
                    instance->SetData(DATA_ADD_HELP_FLAG, THORIM);
                    creature->GetMap()->SummonCreature(NPC_KEEPER_YOGGROOM_THORIM, yoggroomKeeperSpawnLocation[2]);
                    break;
                case NPC_KEEPER_WALKWAY_HODIR:
                    creature->AI()->Talk(SAY_HODIR_HELP);
                    instance->SetData(DATA_ADD_HELP_FLAG, HODIR);
                    creature->GetMap()->SummonCreature(NPC_KEEPER_YOGGROOM_HODIR, yoggroomKeeperSpawnLocation[3]);
                    break;
            }
            creature->DespawnOrUnsummon(10*SEC);
        }

        return true;
    }
};

class item_unbound_fragments_of_valanyr : public ItemScript
{
public:
    item_unbound_fragments_of_valanyr() : ItemScript("item_unbound_fragments_of_valanyr") { }

    bool OnUse(Player* player, Item* pItem, SpellCastTargets const& /*targets*/)
    {

        if (Creature* yogg = player->FindNearestCreature(ENTRY_YOGG_SARON,20))
        {
            if (yogg->FindCurrentSpellBySpellId(SPELL_DEAFENING_ROAR))
            {
                yogg->AI()->DoAction(ACTION_HIT_BY_VALANYR);
                return false;
            }
        }

        player->SendEquipError(EQUIP_ERR_CANT_DO_RIGHT_NOW, pItem, NULL);
        return true;
    }
};

void AddSC_boss_yoggsaron()
{
    new boss_sara();
    new boss_yogg_saron();

    new npc_ominous_cloud();
    new npc_guardian_of_yogg_saron();
    new npc_crusher_tentacle();
    new npc_corruptor_tentacle();
    new npc_constrictor_tentacle();
    new npc_descend_into_madness();
    new npc_influence_tentacle();
    new npc_brain_of_yogg_saron();
    new npc_immortal_guardian();
    new npc_support_keeper();
    new npc_sanity_well();
    new npc_laughting_skull();
    new npc_death_orb();
    new npc_death_ray();
    new npc_keeper_help();

    new go_flee_to_surface();

    new item_unbound_fragments_of_valanyr();

    new spell_yogg_tentacle_squeeze();
    new spell_keeper_support_aura_targeting();
    new spell_lunatic_gaze_targeting();
    new spell_brain_link_periodic_dummy();
    new spell_titanic_storm();
    new spell_insane_death_effekt();
    //new spell_summon_tentacle_position();
    new spell_empowering_shadows();
    new spell_hodir_protective_gaze();
    new spell_induce_madness();
    new spell_malady_of_the_mind();
}
