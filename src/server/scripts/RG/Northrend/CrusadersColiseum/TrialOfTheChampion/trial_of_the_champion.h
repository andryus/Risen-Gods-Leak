/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012-2015 Rising-Gods <http://www.rising-gods.de/>
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

#ifndef DEF_TOC_H
#define DEF_TOC_H

#define DataHeader "TC"

enum EncounterDefs
{
    BOSS_GRAND_CHAMPIONS,
    BOSS_ARGENT_CHALLENGE,
    BOSS_BLACK_KNIGHT,
    MAX_ENCOUNTER
};

enum Data
{
    // first encounter
    DATA_LESSER_CHAMPIONS_DEFEATED,
    DATA_PHASE_TWO_MOVEMENT,
    DATA_CANCEL_FIRST_ENCOUNTER,

    // second encounter
    DATA_ARGENT_CHAMPION,
    DATA_ARGENT_SOLDIER_DEFEATED,

    // third encounter
    DATA_BLACK_KNIGHT_SPAWNED,

    // instance
    DATA_EVENT_NPC,
    DATA_EVENT_TIMER,
    DATA_EVENT_STAGE,
    DATA_TEAM_IN_INSTANCE,
};

enum DataGuid
{
    DATA_ANNOUNCER,

    DATA_GRAND_CHAMPION_1,
    DATA_GRAND_CHAMPION_2,
    DATA_GRAND_CHAMPION_3,
};

enum EventStage
{
    EVENT_STAGE_NULL,

    EVENT_STAGE_CHAMPIONS_000, // announcer
    EVENT_STAGE_CHAMPIONS_010, // tirion
    EVENT_STAGE_CHAMPIONS_015, // thrall
    EVENT_STAGE_CHAMPIONS_020, // garrosh
    EVENT_STAGE_CHAMPIONS_030, // varian
    EVENT_STAGE_CHAMPIONS_040, // tirion
    EVENT_STAGE_CHAMPIONS_WAVE_1,
    EVENT_STAGE_CHAMPIONS_WAVE_2,
    EVENT_STAGE_CHAMPIONS_WAVE_3,
    EVENT_STAGE_CHAMPIONS_050,
    EVENT_STAGE_CHAMPIONS_WAVE_1_START,
    EVENT_STAGE_CHAMPIONS_WAVE_2_START,
    EVENT_STAGE_CHAMPIONS_WAVE_3_START,
    EVENT_STAGE_CHAMPIONS_WAVE_BOSS,
    EVENT_STAGE_CHAMPIONS_WAVE_BOSS_P2,

    EVENT_STAGE_ARGENT_CHALLENGE_000,
    EVENT_STAGE_ARGENT_CHALLENGE_010,
    EVENT_STAGE_ARGENT_CHALLENGE_020,
    EVENT_STAGE_ARGENT_CHALLENGE_030,
    EVENT_STAGE_ARGENT_CHALLENGE_040,
    EVENT_STAGE_ARGENT_CHALLENGE_050,

    EVENT_STAGE_BLACK_KNIGHT_000, // tirion
    EVENT_STAGE_BLACK_KNIGHT_010,
    EVENT_STAGE_BLACK_KNIGHT_020, // announcer
    EVENT_STAGE_BLACK_KNIGHT_030, // tirion
    EVENT_STAGE_BLACK_KNIGHT_035, // garrosh
    EVENT_STAGE_BLACK_KNIGHT_036, // varian
    EVENT_STAGE_BLACK_KNIGHT_040, // tirion
    EVENT_STAGE_BLACK_KNIGHT_050,
    EVENT_STAGE_BLACK_KNIGHT_060, // thrall
    EVENT_STAGE_BLACK_KNIGHT_070  // varian
};

enum ToC5Actions
{
    ACTION_GRAND_CHAMPIONS_MOVE_1    = 1,
    ACTION_GRAND_CHAMPIONS_MOVE_2,
    ACTION_GRAND_CHAMPIONS_MOVE_3,
};

enum ToC5Events
{
    EVENT_CHEER_AT_RANDOM_PLAYER    = 1,
    EVENT_FIREWORK,
};

enum ToC5SPells
{
    SPELL_FIREWORK_TRIGGER      = 66409,

    SPELL_FIREWORK_RED          = 66400,
    SPELL_FIREWORK_BLUE         = 66402,
    SPELL_FIREWORK_GOLD         = 64885
};

enum ToC5Achievments
{
    ACHIEVEMENT_TOC5_HERO_A      = 4298,
    ACHIEVEMENT_TOC5_A           = 4296,

    ACHIEVEMENT_TOC5_HERO_H      = 4297,
    ACHIEVEMENT_TOC5_H           = 3778,

    ACHIEVEMENT_CRITERIA_HAD_WORSE = 11789,
};

enum Equipment
{
    ITEM_ARGENT_LANCE           = 46106,
    ITEM_HORDE_LANCE            = 46089,
    ITEM_ALLIANCE_LANCE         = 46090,

    ITEM_BLACK_KNIGHT           = 40343
};

enum Npcs
{
    NPC_TIRION                  = 1010664,
    NPC_GARROSH                 = 1010665,
    NPC_THRALL                  = 1010666,
    NPC_JAINA                   = 1010667,
    NPC_VARIAN                  = 1010668,

    // Horde grand champions
    NPC_MOKRA                   = 35572,
    NPC_ERESSEA                 = 35569,
    NPC_RUNOK                   = 35571,
    NPC_ZULTORE                 = 35570,
    NPC_VISCERI                 = 35617,

    // Alliance grand champions
    NPC_JACOB                   = 34705,
    NPC_AMBROSE                 = 34702,
    NPC_COLOSOS                 = 34701,
    NPC_JAELYNE                 = 34657,
    NPC_LANA                    = 34703,

    // Faction Champions Horde
    NPC_ORGRIMAR_CHAMPION       = 35314,
    NPC_SILVERMOON_CHAMPION     = 35326,
    NPC_THUNDER_CHAMPION        = 35325,
    NPC_TROLL_CHAMPION          = 35323,
    NPC_UNDERCITY_CHAMPION      = 35327,

    // Faction Champions Alliance
    NPC_STORMWIND_CHAMPION      = 35328,
    NPC_GNOMERAGN_CHAMPION      = 35331,
    NPC_EXODAR_CHAMPION         = 35330,
    NPC_DRNASSUS_CHAMPION       = 35332,
    NPC_IRONFORGE_CHAMPION      = 35329,

    NPC_EADRIC                  = 35119,
    NPC_PALETRESS               = 34928,

    // Crusader MOBs
    NPC_ARGENT_LIGHWIELDER      = 35309,
    NPC_ARGENT_MONK             = 35305,
    NPC_PRIESTESS               = 35307,

    // Black Knight
    NPC_BLACK_KNIGHT            = 35451,

    // Black Knight's add
    NPC_RISEN_JAEREN            = 35545,
    NPC_RISEN_ARELAS            = 35564,

    // Announcer Start Event
    NPC_JAEREN                  = 35004,
    NPC_ARELAS                  = 35005,

    // Trigger
    NPC_TRIGGER_FIREWORK        = 35066,

    NPC_FX_TRIGGER_HORDE        = 34883,
    NPC_FX_TRIGGER_ALLIANCE     = 34887,
    NPC_FX_TRIGGER_BLOODELF     = 34904,
    NPC_FX_TRIGGER_TAURE        = 34903,
    NPC_FX_TRIGGER_TROLL        = 34902,
    NPC_FX_TRIGGER_ORC          = 34901,
    NPC_FX_TRIGGER_UNDEAD       = 34905,
    NPC_FX_TRIGGER_DWARF        = 34906, 
    NPC_FX_TRIGGER_GNOME        = 34910,
    NPC_FX_TRIGGER_HUMAN        = 34900,
    NPC_FX_TRIGGER_NIGHTELF     = 34909,
    NPC_FX_TRIGGER_DRAENEI      = 34908,
};

enum GameObjects
{
    GO_MAIN_GATE                = 195647,
    GO_NORTH_PORTCULLIS         = 195650,

    GO_CHAMPIONS_LOOT           = 195709,
    GO_CHAMPIONS_LOOT_H         = 195710,

    GO_EADRIC_LOOT              = 195374,
    GO_EADRIC_LOOT_H            = 195375,
    GO_PALETRESS_LOOT           = 195323,
    GO_PALETRESS_LOOT_H         = 195324
};

enum Vehicles
{
    //Grand Champions Alliance Vehicles
    VEHICLE_MARSHAL_JACOB_ALERIUS_MOUNT             = 35637,
    VEHICLE_AMBROSE_BOLTSPARK_MOUNT                 = 35633,
    VEHICLE_COLOSOS_MOUNT                           = 35768,
    VEHICLE_JAELYNES_MOUNT                          = 34658,
    VEHICLE_LANA_STOUTHAMMER_MOUNT                  = 35636,
    //Grand Champions Adds Alliance Vehicles
    VEHICLE_DARNASSIA_NIGHTSABER                    = 33319,
    VEHICLE_EXODAR_ELEKK                            = 33318,
    VEHICLE_STORMWIND_STEED                         = 33217,
    VEHICLE_GNOMEREGAN_MECHANOSTRIDER               = 33317,
    VEHICLE_IRONFORGE_RAM                           = 33316,

    //Grand Champions Horde Vehicles
    VEHICLE_MOKRA_SKILLCRUSHER_MOUNT                = 35638,
    VEHICLE_ERESSEA_DAWNSINGER_MOUNT                = 35635,
    VEHICLE_RUNOK_WILDMANE_MOUNT                    = 35640,
    VEHICLE_ZUL_TORE_MOUNT                          = 35641,
    VEHICLE_DEATHSTALKER_VESCERI_MOUNT              = 35634,
    //Grand Champions Adds Horde Vehicles
    VEHICLE_FORSAKE_WARHORSE                        = 33324,
    VEHICLE_THUNDER_BLUFF_KODO                      = 33322,
    VEHICLE_ORGRIMMAR_WOLF                          = 33320,
    VEHICLE_SILVERMOON_HAWKSTRIDER                  = 33323,
    VEHICLE_DARKSPEAR_RAPTOR                        = 33321,

    VEHICLE_ARGENT_WARHORSE                         = 36557,
    VEHICLE_ARGENT_BATTLEWORG                       = 36558,

    VEHICLE_BLACK_KNIGHT                            = 35491
};

enum BossIds
{
    BOSS_ID_NONE = 0,
    BOSS_ID_FACTION_1 = 1,
    BOSS_ID_FACTION_2 = 2,
    BOSS_ID_FACTION_3 = 3,
    BOSS_ID_FACTION_4 = 4,
    BOSS_ID_FACTION_5 = 5,
};

Position const ToCCommonPos[] =
{
    {746.59f, 618.49f, 411.09f, 0.0f},  // 0, Middle of the arena (loot chest position)
    {741.77f, 623.31f, 411.17f, 0.0f},  // 1, Middle of the arena (announcer position)
    {735.81f, 661.92f, 412.39f, 0.0f},  // 2, Infront of the Main gate

    {739.67f, 662.54f, 412.39f, 4.5f},  // 3, Main gate - Grand Champions
    {746.71f, 661.02f, 411.69f, 4.6f},  // 4, Main gate - Grand Champions
    {754.34f, 660.70f, 412.39f, 4.8f},  // 5, Main gate - Grand Champions
};

Position const FireworkPos[] =
{
    { 746.68f, 587.17f, 411.56f, 1.52f }, // 0
    { 715.81f, 618.24f, 411.56f, 0.03f }, // 1
    { 776.72f, 618.18f, 411.56f, 3.15f }, // 2
    { 746.38f, 618.30f, 411.09f, 5.08f }, // 3
    { 746.89f, 650.21f, 411.56f, 4.61f }  // 4
};

const Position BlackKnight[2] =
{
    {769.834f, 651.915f, 447.035f, 0.0f},   // 0, Spawn position
    {751.039f, 628.864f, 411.171f, 4.066f}, // 1, Intro 1
};

#endif
