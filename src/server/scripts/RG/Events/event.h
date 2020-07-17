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

#ifndef DEF_EVENT_H
#define DEF_EVENT_H

void AddSC_event_brewfest();
void AddSC_event_hallows_end();
void AddSC_event_pilgrims_bounty();
void AddSC_event_lunar_festival();
void AddSC_event_childrens_week();
void AddSC_event_love_is_in_the_air();
void AddSC_midsummer_fire_festival();

//Firework
enum fireworkNPC
{
    NPC_CLUSTER_BLUE_SMALL      = 1000045,
    NPC_CLUSTER_GREEN_SMALL     = 1000046,
    NPC_CLUSTER_RED_SMALL       = 1000047,
    NPC_CLUSTER_WHITE_SMALL     = 1000048,
    NPC_CLUSTER_BLUE_BIG        = 1000049,
    NPC_CLUSTER_GREEN_BIG       = 1000050,
    NPC_CLUSTER_PURPLE_BIG      = 1000051,
    NPC_CLUSTER_RED_BIG         = 1000052,
    NPC_CLUSTER_WHITE_BIG       = 1000053,
    NPC_CLUSTER_YELLOW_BIG      = 1000054,
    NPC_RED_YELLOW              = 1000055,
    NPC_BLUE_SMALL              = 1000056,
    NPC_BLUE                    = 1000057,
    NPC_RED_SMALL               = 1000058,
    NPC_RED                     = 1000059,
    NPC_RED_VERY_SMALL          = 1000060,
    NPC_GREEN                   = 1000061,
    NPC_PURPLE_BIG              = 1000062,
    NPC_FIREWORK_RANDOM         = 1000063,
    NPC_FIRWORK_BLUE_FLAMES     = 1000064,
    NPC_FIREWORK_RED_FLAMES     = 1000065,
    NPC_EXPLOSION               = 1000066,
    NPC_EXPLOSION_LARGE         = 1000067,
    NPC_EXPLOSION_MEDIUM        = 1000068,
    NPC_EXPLOSION_SMALL         = 1000069,
    NPC_RED_STREAKS             = 1000070,
    NPC_RED_BLUE_WHITE          = 1000071,
    NPC_YELLOW_ROSE             = 1000072,
    NPC_DALARAN_FIRE            = 1000073,
    NPC_FIREWORK                = 1000074,
};

enum fireworkSpells
{
    //Cluster
    SPELL_CLUSTER_BLUE_SMALL    = 26304,
    SPELL_CLUSTER_GREEN_SMALL   = 26325,
    SPELL_CLUSTER_RED_SMALL     = 26327,
    SPELL_CLUSTER_WHITE_SMALL   = 26328,

    SPELL_CLUSTER_BLUE_BIG      = 26488,
    SPELL_CLUSTER_GREEN_BIG     = 26490,
    SPELL_CLUSTER_PURPLE_BIG    = 26516,
    SPELL_CLUSTER_RED_BIG       = 26517,
    SPELL_CLUSTER_WHITE_BIG     = 26518,
    SPELL_CLUSTER_YELLOW_BIG    = 26519,

    //Normal
    SPELL_RED_YELLOW            = 64885,
    SPELL_BLUE_SMALL            = 66402,
    SPELL_BLUE                  = 11540,
    SPELL_RED_SMALL             = 66400,
    SPELL_RED                   = 6668,
    SPELL_RED_VERY_SMALL        = 11352, //Hunter Trap
    SPELL_GREEN                 = 11541,
    SPELL_PURPLE_BIG            = 26353,

    //Animations
    SPELL_FIREWORK_RANDOM       = 46847, //Trigger in der Luft
    SPELL_FIRWORK_BLUE_FLAMES   = 46835, //gleicher Effekt wie SPELL_FIREWORK_RANDOM, nur mit blauen Flammen drumrum
    SPELL_FIREWORK_RED_FLAMES   = 46830,
    SPELL_EXPLOSION             = 62077, //Explosion + SPELL_FIREWORK_RANDOM
    SPELL_EXPLOSION_LARGE       = 62077,
    SPELL_EXPLOSION_MEDIUM      = 62075,
    SPELL_EXPLOSION_SMALL       = 62074,
    SPELL_RED_STREAKS           = 11542,
    SPELL_RED_BLUE_WHITE        = 11543,
    SPELL_YELLOW_ROSE           = 11544,
    SPELL_DALARAN_FIRE          = 55420,
    SPELL_FIREWORK              = 25465, //muss nen Trigger in sich casten
};

#endif
