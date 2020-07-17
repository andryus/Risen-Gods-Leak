/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#ifndef DEF_RUINS_OF_AHNQIRAJ_H
#define DEF_RUINS_OF_AHNQIRAJ_H

#define DataHeader "AQR"

enum DataTypes
{
    BOSS_KURINNAXX          = 1,
    BOSS_RAJAXX             = 2,
    BOSS_MOAM               = 3,
    BOSS_BURU               = 4,
    BOSS_AYAMISS            = 5,
    BOSS_OSSIRIAN           = 6,
    MAX_ENCOUNTER,
};

enum DataGuid
{
    DATA_KURINNAXX          = 1,
    DATA_RAJAXX             = 2,
    DATA_MOAM               = 3,
    DATA_BURU               = 4,
    DATA_AYAMISS            = 5,
    DATA_OSSIRIAN           = 6,
    DATA_ANDRONOV           = 7
};

enum Creatures
{
    NPC_KURINAXX                = 15348,
    NPC_RAJAXX                  = 15341,
    NPC_MOAM                    = 15340,
    NPC_BURU                    = 15370,
    NPC_AYAMISS                 = 15369,
    NPC_OSSIRIAN                = 15339,
    NPC_HIVEZARA_HORNET         = 15934,
    NPC_HIVEZARA_SWARMER        = 15546,
    NPC_HIVEZARA_LARVA          = 15555,
    NPC_SAND_VORTEX             = 15428,
    NPC_OSSIRIAN_TRIGGER        = 15590,
    NPC_ANDRONOV                = 15471,
    NPC_KALDOREI_ELITE          = 15473,
    NPC_QEEZ                    = 15391,
    NPC_TUUBID                  = 15392,
    NPC_DRENN                   = 15389,
    NPC_XURREM                  = 15390,
    NPC_YEGGETH                 = 15386,
    NPC_PAKKON                  = 15388,
    NPC_ZERRAN                  = 15385
}; 

enum GameObjects
{
    GO_OSSIRIAN_CRYSTAL         = 180619,
};

#endif
