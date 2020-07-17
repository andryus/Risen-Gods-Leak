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

#ifndef DEF_OBSIDIAN_SANCTUM_H
#define DEF_OBSIDIAN_SANCTUM_H

#define OSScriptName "instance_obsidian_sanctum"
#define DataHeader "OS"

enum EncounterDefs
{
    BOSS_SARTHARION             = 0,
    MAX_ENCOUNTER
};

enum Data
{
    DATA_TENEBRON_EVENT         = 1,
    DATA_SHADRON_EVENT          = 2,
    DATA_VESPERON_EVENT         = 3,

    DATA_TENEBRON_PREFIGHT      = 7,
    DATA_SHADRON_PREFIGHT       = 8,
    DATA_VESPERON_PREFIGHT      = 9,

    DATA_DRAGONS_STILL_ALIVE    = 10,
};

enum DataGuid
{
    DATA_SARTHARION             = 1,
    DATA_TENEBRON               = 2,
    DATA_SHADRON                = 3,
    DATA_VESPERON               = 4
};

enum ObsiSpells
{
    SPELL_TWILIGHT_REVENGE      = 60639,
};

enum ObsiCreatures
{
    NPC_SARTHARION              = 28860,
    NPC_TENEBRON                = 30452,
    NPC_SHADRON                 = 30451,
    NPC_VESPERON                = 30449,

    NPC_ACOLYTE_OF_VESPERON     = 31219,

    NPC_FIRE_CYCLONE            = 30648,
    NPC_LAVA_BLAZE              = 30643,

    // trash
    NPC_ONYX_SANCTUM_GUARDIEN   = 30453,
    NPC_ONYX_BROOD_GENERAL      = 30680,
    NPC_ONYX_BLAZE_MISTRESS     = 30681,
    ONYX_FLY_CAPTAIN            = 30682
};

enum ObsiGameObjects
{
    GO_TWILIGHT_PORTAL          = 193988,
    GO_NORMAL_PORTAL            = 193989
};

#endif
