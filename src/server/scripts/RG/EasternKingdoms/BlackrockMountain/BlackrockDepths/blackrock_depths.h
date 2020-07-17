/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#ifndef DEF_BRD_H
#define DEF_BRD_H

#define DataHeader "BRD"

enum FactionIds
{
    FACTION_NEUTRAL_BR         = 734,
    FACTION_HOSTILE_BR         = 754,
    FACTION_FRIEND             = 35,
    FACTION_ARENA_NEUTRAL      = 674,
};

enum Creatures
{
    NPC_EMPEROR                = 9019,
    NPC_PHALANX                = 9502,
    NPC_ANGERREL               = 9035,
    NPC_DOPEREL                = 9040,
    NPC_HATEREL                = 9034,
    NPC_VILEREL                = 9036,
    NPC_SEETHREL               = 9038,
    NPC_GLOOMREL               = 9037,
    NPC_DOOMREL                = 9039,
    NPC_MAGMUS                 = 9938,
    NPC_MOIRA                  = 8929,
    NPC_HURLEY_BLACKBREATH     = 9537,
    // Arena Spectator
    NPC_ARENA_SPECTATOR        = 8916,
    NPC_SHADOWFORGE_PEASANT    = 8896,
    NPC_SHADOWFORGE_CITIZEN    = 8902,
    NPC_SHADOWFORGE_SENATOR    = 8904,
    NPC_ANVILRAGE_SOLDIER      = 8893,
    NPC_ANVILRAGE_MEDIC        = 8894,
    NPC_ANVILRAGE_OFFICER      = 8895,
    // Arena 
    NPC_GRIMSTONE              = 10096,
    NPC_THELDREN               = 16059,
    // Flamelash
    NPC_BURNING_SPIRIT         = 9178,
    // Bridgeevent
    NPC_LOREGRAIN              = 9024,
    NPC_ANVILRAGE_GUARDMAN     = 8891,
    // Magmusmisc
    NPC_IRONHAND_GUARDIAN      = 8982,
    // Golemlord
    NPC_RAGEREAVER_GOLEM       = 8906,
    NPC_HAMMER_CONSTRUCT       = 8907
};

enum Waypoints
{
    PATH_ANVILRAGE_GUARDMAN    = 889100
};

enum GameObjects
{
    GO_ARENA1                  = 161525,
    GO_ARENA2                  = 161522,
    GO_ARENA3                  = 161524,
    GO_ARENA4                  = 161523,
    GO_SHADOW_LOCK             = 161460,
    GO_SHADOW_MECHANISM        = 161461,
    GO_SHADOW_GIANT_DOOR       = 157923,
    GO_SHADOW_DUMMY            = 161516,
    GO_BAR_KEG_SHOT            = 170607,
    GO_BAR_KEG_TRAP            = 171941,
    GO_BAR_DOOR                = 170571,
    GO_TOMB_ENTER              = 170576,
    GO_TOMB_EXIT               = 170577,
    GO_LYCEUM                  = 170558,
    GO_SF_N                    = 174745, // Shadowforge Brazier North
    GO_SF_S                    = 174744, // Shadowforge Brazier South
    GO_GOLEM_ROOM_N            = 170573, // Magmus door North
    GO_GOLEM_ROOM_S            = 170574, // Magmus door Soutsh
    GO_THRONE_ROOM             = 170575, // Throne door
    GO_SPECTRAL_CHALICE        = 164869,
    GO_CHEST_SEVEN             = 169243,
    GO_ARENA_SPOILS            = 181074,

    GO_DWARFRUNE_A01           = 170578,
    GO_DWARFRUNE_B01           = 170579,
    GO_DWARFRUNE_C01           = 170580,
    GO_DWARFRUNE_D01           = 170581,
    GO_DWARFRUNE_E01           = 170582,
    GO_DWARFRUNE_F01           = 170583,
    GO_DWARFRUNE_G01           = 170584,

    // Quests
    GO_BANNER_OF_PROVOCATION   = 181058,
};

enum DataTypes
{
    TYPE_RING_OF_LAW        = 1,
    TYPE_VAULT              = 2,
    TYPE_BAR                = 3,
    TYPE_TOMB_OF_SEVEN      = 4,
    TYPE_LYCEUM             = 5,
    TYPE_IRON_HALL          = 6,

    DATA_EMPEROR            = 10,
    DATA_PHALANX            = 11,

    DATA_ARENA1             = 12,
    DATA_ARENA2             = 13,
    DATA_ARENA3             = 14,
    DATA_ARENA4             = 15,

    DATA_GO_BAR_KEG         = 16,
    DATA_GO_BAR_KEG_TRAP    = 17,
    DATA_GO_BAR_DOOR        = 18,
    DATA_GO_CHALICE         = 19,

    DATA_GHOSTKILL          = 20,
    DATA_EVENSTARTER        = 21,

    DATA_GOLEM_DOOR_N       = 22,
    DATA_GOLEM_DOOR_S       = 23,

    DATA_THRONE_DOOR        = 24,

    DATA_SF_BRAZIER_N       = 25,
    DATA_SF_BRAZIER_S       = 26,
    DATA_MOIRA              = 27,

    DATA_THUNDERBREW_KEG    = 28,
    DATA_GO_ARENA_SPOILS    = 29,

    MAX_NPC_AMOUNT          = 4
};

#endif
