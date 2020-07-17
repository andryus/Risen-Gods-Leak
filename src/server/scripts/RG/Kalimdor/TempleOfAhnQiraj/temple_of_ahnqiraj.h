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

#ifndef DEF_TEMPLE_OF_AHNQIRAJ_H
#define DEF_TEMPLE_OF_AHNQIRAJ_H

#define DataHeader "AQT"

enum EncounterDefs
{
    BOSS_SKERAM         = 1,
    BOSS_BUG_KRI        = 2,
    BOSS_BUG_YAUJ       = 3,
    BOSS_BUG_VEM        = 4,
    BOSS_SARTURA        = 5,
    BOSS_FANKRISS       = 6,
    BOSS_VISCIDUS       = 7,
    BOSS_HUHURAN        = 8,
    BOSS_TWIN_VEKLOR    = 9,
    BOSS_TWIN_VEKLINASH = 10,
    BOSS_OURO           = 11,
    BOSS_CTHUN_EYE      = 12,
    BOSS_CTHUN          = 13
};

enum DataGuid
{
    DATA_SKERAM              = 1,
    DATA_BUG_KRI             = 2,
    DATA_BUG_YAUJ            = 3,
    DATA_BUG_VEM             = 4,
    DATA_SARTURA             = 5,
    DATA_FANKRISS            = 6,
    DATA_VISCIDUS            = 7,
    DATA_HUHURAN             = 8,
    DATA_TWIN_VEKLOR         = 9,
    DATA_TWIN_VEKLINASH      = 10,
    DATA_OURO                = 11,
    DATA_CTHUN_EYE           = 12,
    DATA_CTHUN               = 13,


    DATA_VEKLORISDEAD,
    DATA_VEKLOR_DEATH,
    DATA_VEMISDEAD,
    DATA_VEM_DEATH,
    DATA_VEKNILASHISDEAD,
    DATA_VEKNILASH_DEATH,
    DATA_BUG_TRIO_DEATH,
    DATA_CTHUN_PHASE,
};

enum InstanceCreatures
{
    NPC_SKERAM          = 15263,
    NPC_BUG_KRI         = 15511,
    NPC_BUG_YAUJ        = 15543,
    NPC_BUG_VEM         = 15544,
    NPC_SARTURA         = 15516,
    NPC_SARTURA_ADD     = 15984,
    NPC_FANKRISS        = 15510,
    NPC_VISCIDUS        = 15299,
    NPC_HUHURAN         = 15509,
    NPC_TWIN_VEKLOR     = 15276,
    NPC_TWIN_VEKLINASH  = 15275,
    NPC_OURO            = 15517,
    NPC_CTHUN_EYE       = 15589,
    NPC_CTHUN           = 15727,

    NPC_CTHUN_PORTAL        = 15896,
    NPC_CLAW_TENTACLE       = 15725,
    NPC_EYE_TENTACLE        = 15726,
    NPC_SMALL_PORTAL        = 15904,
    NPC_BODY_OF_CTHUN       = 15809,
    NPC_GIANT_CLAW_TENTACLE = 15728,
    NPC_GIANT_EYE_TENTACLE  = 15334,
    NPC_FLESH_TENTACLE      = 15802,
    NPC_GIANT_PORTAL        = 15910,
};

enum InstanceGameobjects
{
    GO_SKERAM_GATE = 180636,
    GO_TWINS_GATE  = 180635
};

#endif

