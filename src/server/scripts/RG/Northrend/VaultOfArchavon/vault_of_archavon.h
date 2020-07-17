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

#ifndef DEF_ARCHAVON_H
#define DEF_ARCHAVON_H

#define DataHeader "VA"

enum Creatures
{
    NPC_ARCHAVON    = 31125,
    NPC_EMALON      = 33993,
    NPC_KORALON     = 35013,
    NPC_TORAVON     = 38433
};

enum EncounterDefs
{
    BOSS_ARCHAVON   = 0,
    BOSS_EMALON     = 1,
    BOSS_KORALON    = 2,
    BOSS_TORAVON    = 3,
    MAX_ENCOUNTER
};

enum AchievementCriteriaIds
{
    CRITERIA_EARTH_WIND_FIRE_10 = 12018,
    CRITERIA_EARTH_WIND_FIRE_25 = 12019
};

enum AchievementSpells
{
    SPELL_EARTH_WIND_FIRE_ACHIEVEMENT_CHECK = 68308
};

enum VoAEvents
{
    EVENT_CKECK_WG_TIMER = 1
};

enum VoASpells
{
    SPELL_STONED        = 63080,
    SPELL_CLOSE_WARNING = 65124
};

#endif
