/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2014 Rising Gods <http://www.rising-gods.de/>
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

#ifndef DEF_DIRE_MAUL_H
#define DEF_DIRE_MAUL_H

uint32 const EncounterCount = 10;

enum GoDataTypes
{
    DATA_CRYSTAL_GENERATOR_1 = 0,
    DATA_CRYSTAL_GENERATOR_2 = 1,
    DATA_CRYSTAL_GENERATOR_3 = 2,
    DATA_CRYSTAL_GENERATOR_4 = 3,
    DATA_CRYSTAL_GENERATOR_5 = 4,
    DATA_FORCE_FIELD         = 5,
    DATA_CRYSTAL_ACTIVATED   = 6,
    DATA_TENDRIS_WARPWOOD    = 7
};

enum Events
{
    EVENT_START_GO_SEARCH    = 0,
    EVENT_GO_SEARCH          = 1
};

enum DireMaulMiscs
{
    MAX_ENCOUNTER           = 10,

    // East
    TYPE_ALZZIN             = 0, // Do not change - Handled with Acid
    TYPE_ZEVRIM             = 1,
    TYPE_IRONBARK           = 2,

    // West
    TYPE_WARPWOOD           = 3,
    TYPE_IMMOLTHAR          = 4,
    TYPE_PRINCE             = 5,

    // North
    TYPE_KING_GORDOK        = 7,
    TYPE_MOLDAR             = 8,
    TYPE_FENGUS             = 9,
    TYPE_SLIPKIK            = 10,
    TYPE_KROMCRUSH          = 11,

    // East
    GO_CRUMBLE_WALL         = 177220,
    GO_CORRUPT_VINE         = 179502,
    GO_FELVINE_SHARD        = 179559,
    GO_CONSERVATORY_DOOR    = 176907, // Opened by Ironbark the redeemed
    NPC_ZEVRIM_THORNHOOF    = 11490,
    NPC_OLD_IRONBARK        = 11491,
    NPC_IRONBARK_REDEEMED   = 14241,

    // West
    NPC_WARPWOOD_STOMPER    = 11465,
    NPC_PETRIFIED_TREANT    = 11458,
    NPC_PETRIFIED_GUARDIEN  = 14303,
    NPC_WARPWOOD_TREANT     = 11462,
    NPC_WARPWOOD_TANGLER    = 11464,
    NPC_WARPWOOD_CRUSHER    = 13021,
    NPC_WARPWOOD_GUARD      = 11459,
    NPC_STONED_GUARD        = 14303,
    NPC_TENDRIS_WARPWOOD    = 11489,
    NPC_HORSE               = 14566, 
    NPC_PRINCE_TORTHELDRIN  = 11486,
    NPC_IMMOLTHAR           = 11496,
    NPC_ARCANE_ABERRATION   = 11480,
    NPC_MANA_REMNANT        = 11483,
    NPC_HIGHBORNE_SUMMONER  = 11466,
    GO_PRINCES_CHEST        = 179545,
    GO_PRINCES_CHEST_AURA   = 179563,
    GO_CRYSTAL_GENERATOR_1  = 177259,
    GO_CRYSTAL_GENERATOR_2  = 177257,
    GO_CRYSTAL_GENERATOR_3  = 177258,
    GO_CRYSTAL_GENERATOR_4  = 179504,
    GO_CRYSTAL_GENERATOR_5  = 179505,
    GO_FORCEFIELD           = 179503,
    GO_WARPWOOD_DOOR        = 177221,
    GO_WEST_LIBRARY_DOOR    = 179550,

    // North
    NPC_GUARD_MOLDAR        = 14326,
    NPC_STOMPER_KREEG       = 14322,
    NPC_GUARD_FENGUS        = 14321,
    NPC_GUARD_SLIPKIK       = 14323,
    NPC_CAPTAIN_KROMCRUSH   = 14325,
    NPC_CHORUSH             = 14324,
    NPC_KING_GORDOK         = 11501,
    NPC_MIZZLE_THE_CRAFTY   = 14353,
    GO_KNOTS_CACHE          = 179501,
    GO_KNOTS_BALL_AND_CHAIN = 179511,
    GO_GORDOK_TRIBUTE       = 179564,
    GO_NORTH_LIBRARY_DOOR   = 179549,
    SAY_FREE_IMMOLTHAR      = 0,
    SAY_KILL_IMMOLTHAR      = 1,
    SAY_IRONBARK_REDEEM     = 2,
    SPELL_KING_OF_GORDOK    = 22799,
    NPC_GORDOK_MAULER       = 11442,
    NPC_GORDOK_CAPTAIN      = 11445,
    NPC_GORDOK_SPIRIT       = 11446,
    NPC_GORDOK_WARLOCK      = 11448,
    NPC_GORDOK_REAVER       = 11450,
    NPC_GORDOK_KROMCRUSH    = 14325,
    NPC_GORDOK_BUSHWACKER   = 14351,
    NPC_GORDOK_BRUTE        = 11441,
    NPC_GORDOK_MAGE         = 11444,
    NPC_WARLOCK_GUARD       = 14385
};

#endif
