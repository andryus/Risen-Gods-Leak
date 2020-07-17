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

#ifndef DEF_DRAK_THARON_H
#define DEF_DRAK_THARON_H

#define DrakTharonKeepScriptName "instance_drak_tharon_keep"
#define DataHeader               "DTK"

enum Data
{
    DATA_TROLLGORE_EVENT,
    DATA_NOVOS_EVENT,
    DATA_DRED_EVENT,
    DATA_THARON_JA_EVENT,
    DATA_KING_DRED_ACHIEV
};
enum DataGuid
{
    DATA_TROLLGORE,
    DATA_NOVOS,
    DATA_DRED,
    DATA_THARON_JA,

    DATA_NOVOS_CRYSTAL_1,
    DATA_NOVOS_CRYSTAL_2,
    DATA_NOVOS_CRYSTAL_3,
    DATA_NOVOS_CRYSTAL_4,
    DATA_NOVOS_SUMMONER_1,
    DATA_NOVOS_SUMMONER_2,
    DATA_NOVOS_SUMMONER_3,
    DATA_NOVOS_SUMMONER_4,
    DATA_NOVOS_SUMMONER_5,

    DATA_TROLLGORE_INVADER_SUMMONER_1,
    DATA_TROLLGORE_INVADER_SUMMONER_2,
    DATA_TROLLGORE_INVADER_SUMMONER_3,

    ACTION_CRYSTAL_HANDLER_DIED
};

enum CreatureIds
{
    // Trollgore
    NPC_DRAKKARI_INVADER_A = 27709,
    NPC_DRAKKARI_INVADER_B = 27753,
    NPC_DRAKKARI_INVADER_C = 27754,
    NPC_WORLD_TRIGGER      = 22515,

    // King Dred
    NPC_DRAKKARI_GUTRIPPER  = 26641,
    NPC_DRAKKARI_SCYTHECLAW = 26628,
};
#endif
