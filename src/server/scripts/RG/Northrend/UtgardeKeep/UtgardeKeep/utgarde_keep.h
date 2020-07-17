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

#ifndef DEF_UTGARDE_KEEP_H
#define DEF_UTGARDE_KEEP_H

enum Data {
    DATA_PRINCEKELESETH             = 1,
    DATA_SKARVALD                   = 3,
    DATA_DALRONN                    = 4,
    DATA_INGVAR                     = 6,

    DATA_PRINCEKELESETH_EVENT       = 2,
    DATA_SKARVALD_DALRONN_EVENT     = 5,
    DATA_INGVAR_EVENT               = 7,

    DATA_FORGE_1                    = 8,
    DATA_FORGE_2                    = 9,
    DATA_FORGE_3                    = 10,

    DATA_FORGE_MASTER_1             = 11,
    DATA_FORGE_MASTER_2             = 12,
    DATA_FORGE_MASTER_3             = 13,
};

enum CreatureIds
{
    // Skarvald - Dalronn
    NPC_DALRONN_GHOST               = 27389,
    NPC_SKARVALD_GHOST              = 27390,
};

#endif
