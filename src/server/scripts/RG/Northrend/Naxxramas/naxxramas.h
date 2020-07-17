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

#ifndef DEF_NAXXRAMAS_H
#define DEF_NAXXRAMAS_H

#define DataHeader "NAX"

enum EncounterDefs
{
    BOSS_ANUBREKHAN,
    BOSS_FAERLINA,
    BOSS_MAEXXNA,
    BOSS_NOTH,
    BOSS_HEIGAN,
    BOSS_LOATHEB,
    BOSS_PATCHWERK,
    BOSS_GROBBULUS,
    BOSS_GLUTH,
    BOSS_THADDIUS,
    BOSS_RAZUVIOUS,
    BOSS_GOTHIK,
    BOSS_HORSEMEN,
    BOSS_SAPPHIRON,
    BOSS_KELTHUZAD,
    MAX_BOSS_NUMBER
};

enum Data
{
    DATA_HEIGAN_ERUPT,
    DATA_GOTHIK_GATE,
    DATA_SAPPHIRON_BIRTH,

    DATA_HORSEMEN_CHECK_ACHIEVEMENT_CREDIT,
    DATA_HORSEMEN_BESERK,
    DATA_HORSEMEN_BESERK_0,
    DATA_HORSEMEN_BESERK_1,

    DATA_ALL_ENCOUNTERS_DEAD,
    DATA_ABOMINATION_KILLED,
    DATA_SHOCKING,
};

enum DataGuid
{
    DATA_ANUBREKHAN,
    DATA_FAERLINA,
    DATA_THANE,
    DATA_LADY,
    DATA_BARON,
    DATA_SIR,
    DATA_GLUTH,
    DATA_THADDIUS,
    DATA_HEIGAN,
    DATA_LOATHEB,
    DATA_FEUGEN,
    DATA_STALAGG,
    DATA_KELTHUZAD,
    DATA_KELTHUZAD_PORTAL01,
    DATA_KELTHUZAD_PORTAL02,
    DATA_KELTHUZAD_PORTAL03,
    DATA_KELTHUZAD_PORTAL04,
    DATA_KELTHUZAD_TRIGGER,
};


enum GlobalYells
{
    BIGGLSWORTH_DEAD_YELL                                  = -1533089,

    SAY_TAUNT1                                             = 1,
    SAY_TAUNT2                                             = 2,
    SAY_TAUNT3                                             = 3,
    SAY_TAUNT4                                             = 4,
    SAY_SAPP_DIALOG1                                       = 5,
    SAY_SAPP_DIALOG2_LICH                                  = 6,
    SAY_SAPP_DIALOG3                                       = 7,
    SAY_SAPP_DIALOG4_LICH                                  = 8,
    SAY_SAPP_DIALOG5                                       = 9,
};

enum NaxxAchievements
{
    ACHIEVEMENT_THE_IMMORTAL = 2186,
    ACHIEVEMENT_THE_UNDYING  = 2187
};

enum Actions
{
    ACTION_SAPP_DIALOG  = 1,
};

enum TriggerCreatures
{
     NPC_KELTHUZAD_PROXY     = 1010515,
     NPC_LICHKING_PROXY      = 1010692,
};

const Position Speech = {3538.588f, -5153.587f, 166.212f, 0.0f};

enum GameObjects
{
    GO_BIRTH                = 181356
};

enum InstanceEvents
{
    // Dialogue that happens after Gothik's death.
    EVENT_DIALOGUE_GOTHIK_KORTHAZZ     = 1,
    EVENT_DIALOGUE_GOTHIK_ZELIEK,
    EVENT_DIALOGUE_GOTHIK_BLAUMEUX,
    EVENT_DIALOGUE_GOTHIK_RIVENDARE,
    EVENT_DIALOGUE_GOTHIK_BLAUMEUX2,
    EVENT_DIALOGUE_GOTHIK_ZELIEK2,
    EVENT_DIALOGUE_GOTHIK_KORTHAZZ2,
    EVENT_DIALOGUE_GOTHIK_RIVENDARE2
};

enum InstanceTexts
{
    SAY_DIALOGUE_GOTHIK_HORSEMAN       = 5,
    SAY_DIALOGUE_GOTHIK_HORSEMAN2      = 6
};

#endif

