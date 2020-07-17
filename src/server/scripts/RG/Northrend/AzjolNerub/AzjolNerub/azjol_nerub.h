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

#ifndef DEF_AZJOL_NERUB_H
#define DEF_AZJOL_NERUB_H

#define AzjolNerubScriptName "instance_azjol_nerub"
#define DataHeader           "AN"

enum EncounterDefs
{
    BOSS_KRIKTHIR_THE_GATEWATCHER       = 0,
    BOSS_HADRONOX                       = 1,
    BOSS_ANUBARAK                       = 2,
    MAX_ENCOUNTER,
};

enum DataGuid
{
    DATA_KRIKTHIR_THE_GATEWATCHER,
    DATA_HADRONOX,
    DATA_ANUBARAK,
    DATA_WATCHER_GASHRA,
    DATA_WATCHER_SILTHIK,
    DATA_WATCHER_NARJIL
};

enum Data
{
    DATA_ENGAGED,
    DATA_DENIED
};

enum Npcs
{
    NPC_KRIKTHIR            = 28684,
    NPC_HADRONOX            = 28921,
    NPC_ANUBARAK            = 29120,
    NPC_WATCHER_NARJIL      = 28729,
    NPC_WATCHER_GASHRA      = 28730,
    NPC_WATCHER_SILTHIK     = 28731,
    NPC_WEB_WRAP            = 28619
};

#endif
