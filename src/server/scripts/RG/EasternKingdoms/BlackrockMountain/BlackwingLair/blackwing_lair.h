/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#ifndef DEF_BLACKWING_LAIR_H
#define DEF_BLACKWING_LAIR_H

#define BRLScriptName "instance_blackwing_lair"
#define DataHeader    "BWL"

enum BWLEncounter
{
    BOSS_RAZORGORE      = 0,
    BOSS_VAELASTRAZ     = 1,
    BOSS_BROODLORD      = 2,
    BOSS_FIREMAW        = 3,
    BOSS_EBONROC        = 4,
    BOSS_FLAMEGOR       = 5,
    BOSS_CHROMAGGUS     = 6,
    BOSS_NEFARIAN       = 7,
    MAX_ENCOUNTER
};

enum BWLCreatures
{
    NPC_RAZORGORE               = 12435,
    NPC_GRETHOK_THE_CONTROLLER  = 12557,
    NPC_BLACKWING_GUARDSMAN     = 14456,
    NPC_ORB_TRIGGER             = 14449,
    NPC_BLACKWING_SPELLMARKER   = 16604,

    NPC_BLACKWING_DRAGON        = 12422,
    NPC_BLACKWING_TASKMASTER    = 12458,
    NPC_BLACKWING_LEGIONAIRE    = 12416,
    NPC_BLACKWING_WARLOCK       = 12459,

    NPC_VAELASTRAZ              = 13020,
    NPC_BLACKWING_TECHNICAN     = 13996,

    NPC_BROODLORD               = 12017,

    NPC_FIRENAW                 = 11983,

    NPC_EBONROC                 = 14601,

    NPC_FLAMEGOR                = 11981,

    NPC_CHROMAGGUS              = 14020,

    NPC_VICTOR_NEFARIUS         = 10162,

    NPC_NEFARIAN                = 11583,
};

enum BWLGameObjects
{
    GO_RAZORGORE_DOOR_1         = 176964,
    GO_RAZORGORE_DOOR_2         = 176965,
    GO_VAELASTRAZ_DOOR          = 179364,
    GO_BROODLORD_DOOR           = 179365,
    GO_CHROMAGGUS_DOOR_1        = 179115,
    GO_CHROMAGGUS_DOOR_2        = 179116,
    GO_CHROMAGGUS_DOOR_3        = 179117,
    GO_NEFARIAN_DOOR            = 176966,
    
    GO_ORB_OF_DOMINATION        = 177808,
    GO_RAZORGORE_EGG            = 177807,
    GO_SUPPRESSION_DEVICE       = 179784,
};

enum BWLDataGuid
{
    DATA_RAZORGORE_THE_UNTAMED = 1,
    DATA_GRETHOK_THE_CONTROLLER,
    DATA_VAELASTRAZ_THE_CORRUPT,
    DATA_BROODLORD_LASHLAYER,
    DATA_FIRENAW,
    DATA_EBONROC,
    DATA_FLAMEGOR,
    DATA_CHROMAGGUS,
    DATA_LORD_VICTOR_NEFARIUS,
    DATA_NEFARIAN
};

enum BWLData
{
    DATA_VAELASTRAZ_PRE_EVENT,
};

enum BWLEvents
{
    EVENT_VAELASTRAZ_PRE_EVENT_1 = 1,
    EVENT_VAELASTRAZ_PRE_EVENT_2,
    EVENT_VAELASTRAZ_PRE_EVENT_3,
    EVENT_RESPAWN_NEFARIUS,
};

enum BWLMisc
{
    // Vaelastraz pre Event
    SPELL_NEFARIUS_CORRUPTION   = 23642,
    SAY_NEFARIUS_PRE_EVENT_1    = 14,
    SAY_NEFARIUS_PRE_EVENT_2    = 15,
    DATA_TIMEWALKING            = 1000,

    // SuppressionDevice
    ACTION_CASTSTOP,
    ACTION_INSTANT_REPAIR,
    NPC_SUPPRESSION_TARGET      = 1010627,
    SPELL_SUPPRESSION           = 22247
};

#endif
