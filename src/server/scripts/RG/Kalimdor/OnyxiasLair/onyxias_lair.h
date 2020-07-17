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

#ifndef DEF_ONYXIAS_LAIR_H
#define DEF_ONYXIAS_LAIR_H

#define DataHeader "OL"

enum EncounterDefs
{
    BOSS_ONYXIA     = 0,
    MAX_ENCOUNTER,
};

enum DataGuid
{
    DATA_ONYXIA,
    DATA_FLOOR_ERUPTION,
};

enum InstanceData
{
    DATA_ONYXIA_PHASE_2,
    DATA_SHE_DEEP_BREATH_MORE
};

enum Creatures
{
    NPC_ONYXIA                  = 10184,

    NPC_WHELP                   = 11262,
    NPC_LAIRGUARD               = 36561
};

enum GameObjects
{
    GO_WHELP_SPAWNER            = 176510,
    GO_WHELP_EGG                = 176511
};

enum Achievements
{
    ACHIEVEMENT_DEEP_BREATH_10_PLAYER = 4404,
    ACHIEVEMENT_DEEP_BREATH_25_PLAYER = 4407,
    ACHIEVEMENT_MANY_WHELPS_10_PLAYER = 4403,
    ACHIEVEMENT_MANY_WHELPS_25_PLAYER = 4406
};

enum AchievementData
{
    ACHIEV_CRITERIA_MANY_WHELPS_10_PLAYER                   = 12565, // Criteria for achievement 4403: Many Whelps! Handle It! (10 player) Hatch 50 eggs in 10s
    ACHIEV_CRITERIA_MANY_WHELPS_25_PLAYER                   = 12568, // Criteria for achievement 4406: Many Whelps! Handle It! (25 player) Hatch 50 eggs in 10s
    ACHIEV_CRITERIA_DEEP_BREATH_10_PLAYER                   = 12566, // Criteria for achievement 4404: She Deep Breaths More (10 player) Everybody evade Deep Breath
    ACHIEV_CRITERIA_DEEP_BREATH_25_PLAYER                   = 12569, // Criteria for achievement 4407: She Deep Breaths More (25 player) Everybody evade Deep Breath
    ACHIEV_TIMED_MORE_DOTS                                  =  6601  // Timed event for achievement 4402, 4005: More Dots! (10, 25 player) 5 min kill
};
#endif
