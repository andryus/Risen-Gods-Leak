/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#ifndef DEF_SUNKEN_TEMPLE_H
#define DEF_SUNKEN_TEMPLE_H

#define DataHeader "ST"

#define TROLLBOSS_DEATH 2
#define JAMMALAN_DEATH 7
#define MORPHAZ_DEATH 8
#define HAZZAS_DEATH 9
#define ERANIKUS_DEATH 10
#define ATALALARION_DEATH 11 //optional

#define EVENT_STATE 1

enum CreaturesSunkenTemple
{
    NPC_JAMMAL_AN        = 5710,
    NPC_SHADE_OF_EANKIUS = 5709,
    NPC_ZULLOR           = 5716,
    NPC_ATAL             = 8580,
    NPC_SHADE_OF_HAKKAR  = 8440,
};

enum DataSunkenTemple
{
    DATA_JAMMAL_AN       = 12,
    DATA_SHADE           = 13,
    DATA_TROLL_DONE      = 14,
};

enum GameobjectSunkenTemple
{
    GO_ALTAR             = 148838
};
#endif

