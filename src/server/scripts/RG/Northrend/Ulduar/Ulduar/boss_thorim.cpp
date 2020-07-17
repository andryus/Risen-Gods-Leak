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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "ulduar.h"
#include "Player.h"
#include "SpellAuras.h"
#include "DBCStores.h"

// ###### Related Creatures/Object ######
enum Creatures
{
    NPC_SIF                         = 33196,
    NPC_POWER_SOURCE                = 34055, // bad id possible 17984
    NPC_GOLEM_BUNNY_LEFT            = 33140,
    NPC_GOLEM_BUNNY_RIGHT           = 33141,
    NPC_THUNDER_ORB                 = 33378,
    NPC_LIGHTNING_ORB               = 33138,
    NPC_THORIM_CONTROLLER           = 32879,
};

enum GameObjects
{
    GO_DOOR                         = 194560,
    GO_CACHE_OF_STORMS_10           = 194312,
    GO_CACHE_OF_STORMS_10_HM        = 194313,
    GO_CACHE_OF_STORMS_25           = 194314,
    GO_CACHE_OF_STORMS_25_HM        = 194315
};

// ###### Texts ######
enum Yells
{
    SAY_AGGRO_1                                 = 0,
    SAY_AGGRO_2                                 = 15,
    SAY_SPECIAL_1                               = 1,
    SAY_SPECIAL_2                               = 2,
    SAY_SPECIAL_3                               = 3,
    SAY_JUMPDOWN                                = 4,
    SAY_SLAY                                    = 5,
    SAY_BERSERK                                 = 6,
    SAY_WIPE                                    = 7,
    SAY_DEATH                                   = 8,
    SAY_END_NORMAL_1                            = 9,
    SAY_END_NORMAL_2                            = 10,
    SAY_END_NORMAL_3                            = 11,
    SAY_END_HARD_1                              = 12,
    SAY_END_HARD_2                              = 13,
    SAY_END_HARD_3                              = 14,
    SAY_YS_HELP                                 = 16
};

#define EMOTE_BARRIER                           "Runic Colossus surrounds itself with a crackling Runic Barrier!"
#define EMOTE_MIGHT                             "Ancient Rune Giant fortifies nearby allies with runic might!"

// ###### Datas ######
enum Achievements
{
    ACHIEVEMENT_SIFFED_10                       = 2977,
    ACHIEVEMENT_SIFFED_25                       = 2978,
    ACHIEVEMENT_LOSE_ILLUSION_10                = 3176,
    ACHIEVEMENT_LOSE_ILLUSION_25                = 3183,
    ACHIEVEMENT_WHO_NEEDS_BLOODLUST_10          = 2975,
    ACHIEVEMENT_WHO_NEEDS_BLOODLUST_25          = 2976,
    ACHIEVEMENT_DONT_STAND_IN_THE_LIGHTNING_10  = 2971,
    ACHIEVEMENT_DONT_STAND_IN_THE_LIGHTNING_25  = 2972,
};

enum Datas
{
    DATA_LIGHTNING_CHARGE              = 1,
    DATA_RUNIC_SMASH_BUNNY_TIMER       = 2,
    DATA_MINIBOSSES_DEAD               = 3,
    DATA_ACHIEVEMENT                   = 4,
    DATA_LIGHTNING                     = 4,
};

enum Actions
{
    ACTION_ARENA_DIED                  = 1,
    ACTION_GIANT_ACTIVATE              = 2,
    ACTION_COLOSSUS_START              = 3,
    ACTION_ADD_ATTACKED_BY_PLAYER      = 4,
    ACTION_SPAWN_ARENA_WAVE            = 5,
    ACTION_SPAWN_CORRIDOR_WAVE         = 6,
};

enum Misc
{
    MISC_HARD_MODE_TIMER               = 3*MINUTE*IN_MILLISECONDS,
};

enum Sides
{
    SIDE_NONE     = 0,
    SIDE_ARENA    = 1,
    SIDE_CORRIDOR = 2,
};

enum SpawnTypes
{
    SPAWN_FLAG_CHAMPION      = 0x01,
    SPAWN_FLAG_COMMONER      = 0x02,
    SPAWN_FLAG_EVOKER        = 0x04,
    SPAWN_FLAG_WARBRINGER    = 0x08,

    SPAWN_FLAG_CHAMPION_DOUBLE   = 0x11,
    SPAWN_FLAG_COMMONER_DOUBLE   = 0x22,
    SPAWN_FLAG_EVOKER_DOUBLE     = 0x44,
    SPAWN_FLAG_WARBRINGER_DOUBLE = 0x88,

    SPAWN_FLAG_ACOLYTE       = 0x1,
    SPAWN_FLAG_GUARD         = 0x2,
};

// ###### Event Controlling ######
enum Events
{
    EVENT_SAY_AGGRO             = 1,
    EVENT_STORMHAMMER           = 2,
    EVENT_CHARGE_ORB            = 3,
    EVENT_SUMMON_ADDS           = 4,
    EVENT_BERSERK               = 5,
    EVENT_UNBALANCING_STRIKE    = 6,
    EVENT_CHAIN_LIGHTNING       = 7,
    EVENT_TRANSFER_ENERGY       = 8,
    EVENT_RELEASE_ENERGY        = 9,
    EVENT_PLATFORMKILL          = 10,
    EVENT_HARDMODE_TIMEOUT      = 11,

    EVENT_COLOSSUS_RUNIC_SMASH  = 13,
    EVENT_COLOSSUS_SMASH        = 14,
    EVENT_COLOSSUS_BARRIER      = 15,
    EVENT_COLOSSUS_CHARGE       = 16,

    EVENT_GIANT_SPAWN_WAVE      = 17,
    EVENT_GIANT_STOMP           = 18,
    EVENT_GIANT_DETONATION      = 19,

    EVENT_SIF_FROSTBOLT         = 20,
    EVENT_SIF_FROST_VOLLEY      = 21,
    EVENT_SIF_BLIZZARD          = 22,
    EVENT_SIF_FROSTNOVA         = 23,
};

enum AddEvents
{
    EVENT_SPELL_0 = 1,
    EVENT_SPELL_1 = 2,
    EVENT_SPELL_2 = 3,
};

enum Phases
{
    PHASE_OPENING       = 1,
    PHASE_CORRIDOR      = 2,
    PHASE_THORIM        = 3,

    PHASE_COLOSSUS_CORRIDOR = 4,
    PHASE_COLOSSUS_FIGHT    = 5,

    PHASE_GIANT_SPAWNING = 6,
    PHASE_GIANT_FIGHT    = 7,
};

// ###### Spells ######
enum ThorimSpells
{
    SPELL_SHEAT_OF_LIGHTNING                    = 62276,
    SPELL_STORMHAMMER                           = 62042,
    SPELL_DEAFENING_THUNDER                     = 62470,
    SPELL_CHARGE_ORB                            = 62016,

    SPELL_TOUCH_OF_DOMINION                     = 62565,

    SPELL_CHAIN_LIGHTNING                       = 62131,
    SPELL_LIGHTNING_CHARGE                      = 62279,
    SPELL_LIGHTNING_RELEASE                     = 62466,
    SPELL_LIGHTNING_PILLAR                      = 62976,
    SPELL_UNBALANCING_STRIKE                    = 62130,

    SPELL_SUMMON_LIGHTNING_ORB                  = 62391,
    SPELL_BERSERK_PHASE_1                       = 62560,

    SPELL_LIGHTNING_DESTRUCTION                 = 62393,
};

enum PreAddSpells
{
    SPELL_ACID_BREATH               = 62315,
    SPELL_ACID_BREATH_H             = 62415,
    SPELL_SWEEP                     = 62316,
    SPELL_SWEEP_H                   = 62417,

    SPELL_DEVASTATE                 = 62317,
    SPELL_HEROIC_STRIKE             = 62444,
    SPELL_SUNDER_ARMOR              = 57807,

    SPELL_BARBED_SHOT               = 62318,
    SPELL_SHOOT                     = 16496,
    SPELL_WING_CLIP                 = 40652,

    SPELL_RENEW_P_EVENT             = 61967, // heals 50k HP!
    SPELL_HOLY_SMITE_P              = 62335, // used in combat
    SPELL_CIRCLE_OF_HEALING_P_EVENT = 61964, // heals 34k HP!
    SPELL_GREATER_HEAL_P_EVENT      = 61965, // heals 200k HP!
    SPELL_GREATER_HEAL_P            = 62334, // used in combat
};

enum ArenaSpells
{
    SPELL_MORTAL_STRIKE             = 35054,
    SPELL_WHIRLWIND                 = 15578,
    SPELL_CHARGE                    = 32323,

    SPELL_LOW_BLOW                  = 62326,
    SPELL_PUMMEL                    = 38313,

    SPELL_RUNIC_LIGHTNING           = 62327,
    SPELL_RUNIC_LIGHTNING_H         = 62445,
    SPELL_RUNIC_SHIELD              = 62321,
    SPELL_RUNIC_SHIELD_H            = 62529,
    SPELL_RUNIC_MENDING             = 62328,
    SPELL_RUNIC_MENDING_H           = 62446,

    SPELL_RUNIC_STRIKE              = 62322,
    SPELL_AURA_OF_CELERITY          = 62320,
    SPELL_LEAP                      = 61934,
};

enum CorridorSpawnSpells
{
    SPELL_RENEW_C                   = 62333,
    SPELL_RENEW_C_H                 = 62441,
    SPELL_GREATER_HEAL_C            = 62334,
    SPELL_GREATER_HEAL_C_H          = 62442,
    SPELL_HOLY_SMITE_C              = 62335,
    SPELL_HOLY_SMITE_C_H            = 62443,

    SPELL_WHIRLING_TRIP             = 64151,
    SPELL_IMPALE                    = 62331,
    SPELL_IMPALE_H                  = 62418,

    SPELL_CLEAVE                    = 42724,
    SPELL_SHIELD_SMASH              = 62332,
    SPELL_SHIELD_SMASH_H            = 62420,
    SPELL_HAMSTRING                 = 48639,
};

// Runic Colossus (Mini Boss) Spells
enum RunicSpells
{
    SPELL_SMASH                                 = 62339,
    SPELL_RUNIC_BARRIER                         = 62338,
    SPELL_RUNIC_CHARGE                          = 62613,
    SPELL_RUNIC_SMASH                           = 62465,
    SPELL_RUNIC_SMASH_LEFT                      = 62057,
    SPELL_RUNIC_SMASH_RIGHT                     = 62058,
};

// Ancient Rune Giant (Mini Boss) Spells
enum AncientSpells
{
    SPELL_RUNIC_FORTIFICATION                   = 62942,
    SPELL_RUNE_DETONATION                       = 62526,
    SPELL_STOMP                                 = 62411
};

// Sif Spells
enum SifSpells
{
    SPELL_FROSTBOLT_VOLLEY                      = 62580,
    SPELL_FROSTNOVA                             = 62597,
    SPELL_BLIZZARD                              = 62577,
    SPELL_FROSTBOLT                             = 69274
};

enum MiscSpells
{
    SPELL_AURA_OF_CELERITY_TRIGGER              = 62398
};

#define CRITERIA_THORIM RAID_MODE<uint32>(10438, 10454)
#define CRITERIA_THORIM_I_WILL_TAKE_YOU_ALL_ON RAID_MODE<uint32>(10303, 10310)
#define CRITERIA_KILL_WITHOUT_DEATHS RAID_MODE<uint32>(CRITERIA_KILL_WITHOUT_DEATHS_THORIM_10, CRITERIA_KILL_WITHOUT_DEATHS_THORIM_25)

// ##### Additional Data #####
const Position ThunderOrbPositions[7] =
{
    {2104.99f, -233.484f, 433.576f, 5.49779f},
    {2092.64f, -262.594f, 433.576f, 6.26573f},
    {2104.76f, -292.719f, 433.576f, 0.78539f},
    {2164.97f, -293.375f, 433.576f, 2.35619f},
    {2164.58f, -233.333f, 433.576f, 3.90954f},
    {2145.81f, -222.196f, 433.576f, 4.45059f},
    {2123.91f, -222.443f, 433.576f, 4.97419f}
};

const Position PowerSourcePositions[7] =
{
    {2108.95f, -289.241f, 420.149f, 5.49779f},
    {2097.93f, -262.782f, 420.149f, 6.26573f},
    {2108.66f, -237.102f, 420.149f, 0.78539f},
    {2160.56f, -289.292f, 420.149f, 2.35619f},
    {2161.02f, -237.258f, 420.149f, 3.90954f},
    {2143.87f, -227.415f, 420.149f, 4.45059f},
    {2125.84f, -227.439f, 420.149f, 4.97419f}
};

const Position ArenaSpawnPositions[10] =
{
    {2081.51f, -285.257f, 441.059f, 0.35401f},
    {2077.07f, -262.846f, 441.059f, 6.28142f},
    {2081.16f, -241.109f, 441.059f, 5.81803f},
    {2093.38f, -221.703f, 441.059f, 5.50776f},
    {2112.42f, -208.895f, 441.059f, 5.16611f},
    {2156.97f, -209.140f, 441.059f, 4.14278f},
    {2176.08f, -221.703f, 441.059f, 3.87021f},
    {2188.42f, -240.913f, 441.059f, 3.52464f},
    {2192.94f, -262.897f, 441.059f, 3.12409f},
    {2188.17f, -285.319f, 441.059f, 2.77852f},
};

#define POS_X_ARENA  2181.19f
#define POS_Y_ARENA  -299.12f
#define POS_Z_PLATFORM 438.00f

Position const CorridorSpawnPosition = {2158.879f, -441.73f, 440.250f, 0.127f};

const Position CacheSummonPosition = {2134.58f, -286.908f, 419.495f, 1.55988f};
const Position SifSummonPosition =   {2149.27f, -260.55f, 419.69f, 2.527f};
const Position ArenaCenterPosition = {2134.79f, -263.03f, 419.84f};
#define ARENA_RADIUS   45.0f

// ##### Helper #####
enum ThorimAddIndex
{
    // pre
    BEHEMOTH                = 0,
    MERCENARY_CAPTAIN       = 1,
    MERCENARY_SOLDIER       = 2,
    DARK_RUNE_ACOLYTE_P     = 4,
    // arena
    DARK_RUNE_CHAMPION      = 5,
    DARK_RUNE_COMMONER      = 6,
    DARK_RUNE_EVOKER        = 7,
    DARK_RUNE_WARBRINGER    = 8,
    // corridor
    DARK_RUNE_ACOLYTE_C     = 9,
    IRON_RING_GUARD         = 10,
    IRON_HONOR_GUARD_C      = 11,
    RUNIC_COLOSSUS          = 12,
    RUNE_GIANT              = 13,
    // spawn
    DARK_RUNE_ACOLYTE_S     = 14,
    IRON_HONOR_GUARD_S      = 15,
};

enum ThorimAddEntries
{
    // pre
    NPC_BEMEMOTH                = 32882,
    NPC_MERCENARY_CAPTAIN_A     = 32908,
    NPC_MERCENARY_SOLDIER_A     = 32885,
    NPC_MERCENARY_CAPTAIN_H     = 32907,
    NPC_MERCENARY_SOLDIER_H     = 32883,
    NPC_DARK_RUNE_ACOLYTE_P     = 32886,
    // arena
    NPC_DARK_RUNE_CHAMPION      = 32876,
    NPC_DARK_RUNE_COMMONER      = 32904,
    NPC_DARK_RUNE_EVOKER        = 32878,
    NPC_DARK_RUNE_WARBRINGER    = 32877,
    // corridor
    NPC_DARK_RUNE_ACOLYTE_C     = 33110,
    NPC_IRON_RING_GUARD         = 32874,
    NPC_IRON_HONOR_GUARD_C      = 32875,
    // spawn
    NPC_DARK_RUNE_ACOLYTE_S     = 32957,
    NPC_IRON_HONOR_GUARD_S      = 33125,
};

struct ThorimSummonLocation
{
    uint8 id;
    float x, y, z, o;
};

struct ThorimAddSpell
{
    uint32 spell;
    uint32 timerMin;
    uint32 timerMax;
};

struct ThorimAddType
{
    uint8 id;
    uint32 entry;
    uint16 flags;
    ThorimAddSpell spells[3];
};

enum ThorimAddFlags
{
    FLAG_PRE      = 0x001,
    FLAG_ARENA    = 0x002,
    FLAG_CORRIDOR = 0x004,
    FLAG_SPAWN    = 0x008,

    FLAG_HEALER   = 0x010,
    FLAG_DD       = 0x020,
    FLAG_BOSS     = 0x040,

    FLAG_ALLIANZ  = 0x200,
    FLAG_HORDE    = 0x400,
    FLAG_BOTH     = FLAG_ALLIANZ | FLAG_HORDE,
};

enum ThorimAddCounts
{
    MAX_ADD_TYPES     = 17,
    MAX_FIXED_SPAWNS  = 16,
};

const ThorimAddType ThorimAddTypes[MAX_ADD_TYPES + 1] =
{
    // Index                Entry                    Flags                                        Spell0             Timer   Spell1              Timer
    {BEHEMOTH,             NPC_BEMEMOTH,             FLAG_PRE      | FLAG_DD     | FLAG_BOTH,    {{SPELL_ACID_BREATH,   25, 35}, {SPELL_SWEEP,           15, 20}, {0,                      0,  0}}},
    {MERCENARY_CAPTAIN,    NPC_MERCENARY_CAPTAIN_A,  FLAG_PRE      | FLAG_DD     | FLAG_HORDE,   {{SPELL_DEVASTATE,     10, 15}, {SPELL_HEROIC_STRIKE,    8, 10}, {SPELL_SUNDER_ARMOR,     7, 11}}},
    {MERCENARY_SOLDIER,    NPC_MERCENARY_SOLDIER_A,  FLAG_PRE      | FLAG_DD     | FLAG_HORDE,   {{SPELL_BARBED_SHOT,   20, 25}, {SPELL_WING_CLIP,       12, 17}, {SPELL_SHOOT,            0,  0}}},
    {MERCENARY_CAPTAIN,    NPC_MERCENARY_CAPTAIN_H,  FLAG_PRE      | FLAG_DD     | FLAG_ALLIANZ, {{SPELL_DEVASTATE,     10, 15}, {SPELL_HEROIC_STRIKE,    8, 10}, {SPELL_SUNDER_ARMOR,     7, 11}}},
    {MERCENARY_SOLDIER,    NPC_MERCENARY_SOLDIER_H,  FLAG_PRE      | FLAG_DD     | FLAG_ALLIANZ, {{SPELL_BARBED_SHOT,   20, 25}, {SPELL_WING_CLIP,       12, 17}, {SPELL_SHOOT,            0,  0}}},
    {DARK_RUNE_ACOLYTE_P,  NPC_DARK_RUNE_ACOLYTE_P,  FLAG_PRE      | FLAG_HEALER | FLAG_BOTH,    {{0,                    0,  0}, {0,                      0 , 0}, {0,                      0,  0}}}, // own script
    {DARK_RUNE_CHAMPION,   NPC_DARK_RUNE_CHAMPION,   FLAG_ARENA    | FLAG_DD     | FLAG_BOTH,    {{SPELL_MORTAL_STRIKE, 15, 30}, {SPELL_WHIRLWIND,       10, 20}, {SPELL_CHARGE,          16, 18}}},
    {DARK_RUNE_COMMONER,   NPC_DARK_RUNE_COMMONER,   FLAG_ARENA    | FLAG_DD     | FLAG_BOTH,    {{SPELL_LOW_BLOW,      10, 15}, {SPELL_PUMMEL,           7, 14}, {0,                      0,  0}}},
    {DARK_RUNE_EVOKER,     NPC_DARK_RUNE_EVOKER,     FLAG_ARENA    | FLAG_HEALER | FLAG_BOTH,    {{SPELL_RUNIC_MENDING, 15, 20}, {SPELL_RUNIC_SHIELD,    25, 35}, {SPELL_RUNIC_LIGHTNING, 15, 20}}},
    {DARK_RUNE_WARBRINGER, NPC_DARK_RUNE_WARBRINGER, FLAG_ARENA    | FLAG_DD     | FLAG_BOTH,    {{SPELL_RUNIC_STRIKE,  10, 15}, {SPELL_AURA_OF_CELERITY, 0,  0}, {0,                      0,  0}}},
    {DARK_RUNE_ACOLYTE_C,  NPC_DARK_RUNE_ACOLYTE_C,  FLAG_CORRIDOR | FLAG_HEALER | FLAG_BOTH,    {{SPELL_RENEW_C,       18, 22}, {SPELL_GREATER_HEAL_C,  10, 20}, {SPELL_HOLY_SMITE_C,     0,  0}}},
    {IRON_RING_GUARD,      NPC_IRON_RING_GUARD,      FLAG_CORRIDOR | FLAG_DD     | FLAG_BOTH,    {{SPELL_WHIRLING_TRIP, 15, 25}, {SPELL_IMPALE,          15, 25}, {0,                      0,  0}}},
    {IRON_HONOR_GUARD_C,   NPC_IRON_HONOR_GUARD_C,   FLAG_CORRIDOR | FLAG_DD     | FLAG_BOTH,    {{SPELL_CLEAVE,        10, 20}, {SPELL_SHIELD_SMASH,    10, 20}, {SPELL_HAMSTRING,       20, 25}}},
    {RUNIC_COLOSSUS,       NPC_RUNIC_COLOSSUS,       FLAG_CORRIDOR | FLAG_BOSS   | FLAG_BOTH,    {{0,                    0,  0}, {0,                      0,  0}, {0,                      0,  0}}}, // own script
    {RUNE_GIANT,           NPC_RUNE_GIANT,           FLAG_CORRIDOR | FLAG_BOSS   | FLAG_BOTH,    {{0,                    0,  0}, {0,                      0,  0}, {0,                      0,  0}}}, // own script
    {DARK_RUNE_ACOLYTE_S,  NPC_DARK_RUNE_ACOLYTE_S,  FLAG_SPAWN    | FLAG_HEALER | FLAG_BOTH,    {{SPELL_RENEW_C,       18, 22}, {SPELL_GREATER_HEAL_C,  10 ,20}, {SPELL_HOLY_SMITE_C,     0,  0}}},
    {IRON_HONOR_GUARD_S,   NPC_IRON_HONOR_GUARD_S,   FLAG_SPAWN    | FLAG_DD     | FLAG_BOTH,    {{SPELL_CLEAVE,        15, 25}, {SPELL_SHIELD_SMASH,    15, 25}, {SPELL_HAMSTRING,       20, 25}}},
    {0,                    0,                        0,                                          {{0,                    0,  0}, {0,                      0,  0}, {0,                      0,  0}}},
};

const ThorimSummonLocation ThorimFixedSpawnLocations[MAX_FIXED_SPAWNS] =
{
    // pre
    {BEHEMOTH,            2149.68f, -263.477f, 419.679f, 3.120f},
    {MERCENARY_CAPTAIN,   2131.31f, -271.640f, 419.840f, 2.188f},
    {MERCENARY_SOLDIER,   2127.24f, -259.182f, 419.974f, 5.917f},
    {MERCENARY_SOLDIER,   2123.32f, -254.770f, 419.840f, 6.170f},
    {MERCENARY_SOLDIER,   2120.10f, -258.990f, 419.840f, 6.250f},
    {DARK_RUNE_ACOLYTE_P, 2129.09f, -277.142f, 419.756f, 1.222f},
    // corridor
    {IRON_RING_GUARD    , 2218.38f, -297.50f, 412.18f, 1.030f},
    {IRON_RING_GUARD    , 2235.07f, -297.98f, 412.18f, 1.613f},
    {DARK_RUNE_ACOLYTE_C, 2227.58f, -308.30f, 412.18f, 1.591f},
    {IRON_RING_GUARD    , 2235.26f, -338.34f, 412.18f, 1.589f},
    {IRON_RING_GUARD    , 2217.69f, -337.39f, 412.18f, 1.241f},
    {DARK_RUNE_ACOLYTE_C, 2227.47f, -345.37f, 412.18f, 1.566f},
    {DARK_RUNE_ACOLYTE_C, 2230.93f, -434.27f, 412.26f, 1.931f},
    {IRON_HONOR_GUARD_C,  2220.31f, -436.22f, 412.26f, 1.064f},
    {RUNE_GIANT,          2134.57f, -440.32f, 438.33f, 0.227f},
    {RUNIC_COLOSSUS,      2227.50f, -396.18f, 412.18f, 1.798f},
};

uint8 GetSide(Unit* unit)
{
    return (ArenaCenterPosition.GetExactDist2d(unit) < ARENA_RADIUS) ? SIDE_ARENA : SIDE_CORRIDOR;
}

const ThorimAddType& GetType(uint8 id, uint32 flag)
{
    for (uint8 i = 0; i < MAX_ADD_TYPES; ++i)
    {
        const ThorimAddType& type = ThorimAddTypes[i];
        if (type.id == id)
        {
            if (type.flags & flag)
                return type;
        }
    }
    return ThorimAddTypes[MAX_ADD_TYPES];
}

const ThorimAddType& GetType(uint32 entry)
{
    for (uint8 i = 0; i < MAX_ADD_TYPES; ++i)
    {
        const ThorimAddType& type = ThorimAddTypes[i];
        if (type.entry == entry)
            return type;
    }
    return ThorimAddTypes[MAX_ADD_TYPES];
}

class TrashJumpEvent : public BasicEvent
{
    public:
        TrashJumpEvent(Creature* owner) : _owner(owner), _stage(0) { }

        bool Execute(uint64 eventTime, uint32 /*updateTime*/) override
        {
            switch (_stage)
            {
                case 0:
                    _owner->CastSpell((Unit*)nullptr, SPELL_LEAP);
                    ++_stage;
                    _owner->m_Events.AddEvent(this, eventTime + 2000);
                    return false;
                case 1:
                    _owner->SetReactState(REACT_AGGRESSIVE);
                    _owner->AI()->DoZoneInCombat(_owner, 200.0f);
                    return true;
                default:
                    break;
            }

            return true;
    }

private:
    Creature* _owner;
    uint8 _stage;
};

class ThorimAddAI : public ScriptedAI
{
    public:
        ThorimAddAI(Creature* creature) : ScriptedAI(creature), events(), instance(me->GetInstanceScript()), role(GetType(me->GetEntry())) {}

        bool AutoHandleSpell(uint8 index) const
        {
            if (role.flags & FLAG_HEALER)
                return index < 2;

            if (role.id == MERCENARY_SOLDIER)
                return index < 2;

            if (role.id == DARK_RUNE_WARBRINGER)
                return index < 2;

            return true;
        }

        virtual void Reset()
        {
            events.Reset();

            for (uint8 i = 0; i < 3; ++i)
                if (AutoHandleSpell(i) && role.spells[i].spell && role.spells[i].timerMin)
                    events.ScheduleEvent(AddEvents(i + 1), urand(role.spells[i].timerMin, role.spells[i].timerMax) / 2 * IN_MILLISECONDS);

            EncounteredPlayer = false;
        }

        virtual void EnterCombat(Unit* who)
        {
            if ((role.flags & FLAG_CORRIDOR))
            {
                me->CallForHelp(15.0f);
                if (Creature* colossus = me->FindNearestCreature(NPC_RUNIC_COLOSSUS, 500.0f))
                    colossus->AI()->DoAction(ACTION_COLOSSUS_START);
            }

            if (who->GetTypeId() == TYPEID_PLAYER || who->IsPet() || who->IsTotem())
                if (Creature* controller = me->FindNearestCreature(NPC_THORIM_CONTROLLER, 500.0f))
                    controller->SetInCombatWithZone();
        }

        virtual void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                if (Creature* thorim = me->FindNearestCreature(NPC_THORIM, 1000.0f))
                    thorim->AI()->SetData(DATA_ACHIEVEMENT, DATA_LIGHTNING); 
        }

        virtual void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_ADD_ATTACKED_BY_PLAYER:
                    if (!(role.flags & FLAG_PRE))
                        break;

                    me->CombatStop();
                    me->setFaction(16);
                    me->SetControlled(false, UNIT_STATE_ROOT);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetInCombatWithZone();
                    me->CallForHelp(20.0f);
                    EncounteredPlayer = true;

                    break;
            }
        }

        virtual void UpdateAI(uint32 diff)
        {
            CleanupThreatList();

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            UpdateImpl(diff);
        }

        virtual void UpdateImpl(const uint32 /*diff*/) = 0;

        void RescheduleEvent(uint8 eventId)
        {
            events.CancelEvent(eventId);
            events.ScheduleEvent(eventId, urand(role.spells[eventId - 1].timerMin, role.spells[eventId - 1].timerMax) * IN_MILLISECONDS);
        }

        virtual void EnterEvadeMode(EvadeReason /*why*/) override
        {
            if (Creature* controller = me->FindNearestCreature(NPC_THORIM, 500.0f, true))
                if (role.flags & FLAG_ARENA)
                    controller->AI()->DoAction(ACTION_ARENA_DIED);

            ScriptedAI::EnterEvadeMode();
        }

        virtual void MovementInform(uint32 type, uint32 /*id*/)
        {
            if (type == EFFECT_MOTION_TYPE)
                me->SetInCombatWithZone();
        }

        virtual void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (!isOnSameSide(attacker))
            {
                damage = 0;
                return;
            }

            if (!EncounteredPlayer && (attacker->GetTypeId() == TYPEID_PLAYER || attacker->IsPet() || attacker->IsTotem()) && (role.flags & FLAG_PRE))
            {
                if (Creature* controller = me->FindNearestCreature(NPC_THORIM_CONTROLLER, 500.0f, true))
                    controller->AI()->DoAction(ACTION_ADD_ATTACKED_BY_PLAYER);

                EncounteredPlayer = true;
            }
        }

        void CleanupThreatList()
        {
            if (!me->IsInCombat())
                return;

            std::vector<ThreatReference*> toClear;
            for (ThreatReference* ref : me->GetThreatManager().GetModifiableThreatList())
                if (!isOnSameSide(ref->GetVictim()))
                    toClear.push_back(ref);
            for (ThreatReference* ref : toClear)
                ref->ClearThreat();
        }

    protected:
        bool isOnSameSide(Unit* target)
        {
            return GetSide(target) == GetSide(me);
        }

    protected:
        const ThorimAddType& role;
        EventMap events;
        InstanceScript* instance;
        bool EncounteredPlayer;
        bool StartThreatListCheck;
};

class npc_thorim_controller : public CreatureScript
{
public:
    npc_thorim_controller() : CreatureScript("npc_thorim_controller") {}

    struct npc_thorim_controllerAI : public ScriptedAI
    {
        npc_thorim_controllerAI(Creature* creature) : ScriptedAI(creature), summons(me)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
            me->SetReactState(REACT_PASSIVE);
            instance = me->GetInstanceScript();
        }

        void Reset() override
        {
            AlivePreAdds = 0;
            AliveBossAdds = 0;

            bool isAllianz = instance->GetData(DATA_RAID_IS_ALLIANZ);

            for (uint8 i = 0; i < MAX_FIXED_SPAWNS; ++i)
            {
                const ThorimSummonLocation& spawn = ThorimFixedSpawnLocations[i];
                const ThorimAddType& type = GetType(spawn.id, isAllianz ? FLAG_ALLIANZ : FLAG_HORDE);

                TempSummonType summonType = TEMPSUMMON_CORPSE_TIMED_DESPAWN;
                if (type.flags & FLAG_BOSS)
                    summonType = TEMPSUMMON_MANUAL_DESPAWN;

                me->SummonCreature(type.entry, spawn.x, spawn.y, spawn.z, spawn.o, summonType, 5*IN_MILLISECONDS);
            }

            if (GameObject* go = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_THORIM_LEVER))))
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE);
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);

            const ThorimAddType& type = GetType(summon->GetEntry());
            if (type.flags & FLAG_PRE)
                ++AlivePreAdds;
            else if (type.flags & FLAG_BOSS)
                ++AliveBossAdds;

            if (AlivePreAdds == 6)
            {
                Creature* worm = NULL;
                Creature* captain = NULL;
                Creature* soldiers[3];
                uint8 cnt = 0;
                memset(soldiers, 0, sizeof(soldiers));

                for (std::list<ObjectGuid>::const_iterator itr = summons.begin(); itr != summons.end(); ++itr)
                {
                    if (Creature* creature = ObjectAccessor::GetCreature(*me, *itr))
                    {
                        if (creature->GetEntry() == NPC_BEMEMOTH)
                            worm = creature;
                        else if (creature->GetEntry() == NPC_MERCENARY_CAPTAIN_A || creature->GetEntry() == NPC_MERCENARY_CAPTAIN_H)
                            captain = creature;
                        else if (creature->GetEntry() == NPC_MERCENARY_SOLDIER_A || creature->GetEntry() == NPC_MERCENARY_SOLDIER_H)
                            soldiers[(cnt++) % 3] = creature;
                    }
                }

                if (!worm || !captain || !soldiers[0] || !soldiers[1] || !soldiers[2])
                    return;

                worm->SetControlled(true, UNIT_STATE_ROOT);

                captain->AI()->AttackStart(worm);
                worm->AI()->AttackStart(captain);

                for (uint8 i = 0; i < 3; ++i)
                {
                    soldiers[i]->AI()->AttackStart(worm);
                    soldiers[i]->SetControlled(true, UNIT_STATE_ROOT);
                    worm->Attack(soldiers[i], false);
                }
            }

            switch (summon->GetEntry())
            {
                case NPC_DARK_RUNE_CHAMPION:
                case NPC_DARK_RUNE_WARBRINGER:
                case NPC_DARK_RUNE_EVOKER:
                case NPC_DARK_RUNE_COMMONER:
                    summon->SetReactState(REACT_PASSIVE);
                    summon->m_Events.AddEvent(new TrashJumpEvent(summon), summon->m_Events.CalculateTime(3000));
                    break;
                default:
                    break;
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            const ThorimAddType& type = GetType(summon->GetEntry());
            if (type.flags & FLAG_PRE)
            {
                if (--AlivePreAdds <= 0)
                {
                    if (Creature* thorim = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_THORIM))))
                    {
                        thorim->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        thorim->SetInCombatWithZone();
                    }
                    if (GameObject* go = ObjectAccessor::GetGameObject(*me, ObjectGuid(instance->GetGuidData(DATA_THORIM_LEVER))))
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE);
                }
            }
            else if (type.flags & FLAG_BOSS)
                --AliveBossAdds;
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            summons.DespawnAll();
            me->DespawnOrUnsummon();
            if (Creature* thorim = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(BOSS_THORIM))))
                thorim->AI()->Reset();
        }

        void JustDied(Unit* /*killer*/) override
        {
            summons.DespawnAll();
        }

        void DoAction(int32 type) override
        {
            switch (type)
            {
                case ACTION_ADD_ATTACKED_BY_PLAYER:
                {
                    DummyEntryCheckPredicate predicate;
                    summons.DoAction(ACTION_ADD_ATTACKED_BY_PLAYER, predicate);
                    break;
                }
            }
        }

        uint32 GetData(uint32 id) const override
        {
            if (id != DATA_MINIBOSSES_DEAD)
                return 0;

            return AliveBossAdds == 0;
        }

        void SetData(uint32 id, uint32 value) override
        {
            switch (id)
            {
                case ACTION_SPAWN_ARENA_WAVE:
                    SpawnArenaWave(value);
                    break;
                case ACTION_SPAWN_CORRIDOR_WAVE:
                    SpawnCorridorWave(value);
                    break;
            }
        }

        void SpawnArenaWave(uint16 type)
        {
            std::map<uint8, uint8> counts;

            for (uint8 i = 0; i < 2; ++i)
            {
                uint8 flag = (type >> (i * 4)) & 0xF;

                if (flag & SPAWN_FLAG_CHAMPION)
                    counts[DARK_RUNE_CHAMPION]++;
                if (flag & SPAWN_FLAG_COMMONER)
                    counts[DARK_RUNE_COMMONER] = 6;
                if (flag & SPAWN_FLAG_EVOKER)
                    counts[DARK_RUNE_EVOKER]++;
                if (flag & SPAWN_FLAG_WARBRINGER)
                    counts[DARK_RUNE_WARBRINGER]++;
            }

            std::set<uint8> usedIds;

            for (std::map<uint8, uint8>::const_iterator itr = counts.begin(); itr != counts.end(); ++itr)
            {
                for (uint8 i = 0; i < itr->second; ++i)
                {
                    uint8 spawnIndex = 0;
                    do
                    {
                        spawnIndex = urand(0, 9);
                    } while (usedIds.find(spawnIndex) != usedIds.end());
                    usedIds.insert(spawnIndex);

                    const ThorimAddType& add = GetType(itr->first, FLAG_BOTH);

                    me->SummonCreature(add.entry, ArenaSpawnPositions[spawnIndex], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 8 * IN_MILLISECONDS);
                }
            }
        }

        void SpawnCorridorWave(uint8 type)
        {
            std::map<uint8, uint8> counts;

            if (type & SPAWN_FLAG_ACOLYTE)
                counts[DARK_RUNE_ACOLYTE_S]++;
            if (type & SPAWN_FLAG_GUARD)
                counts[IRON_HONOR_GUARD_S]++;

            for (std::map<uint8, uint8>::const_iterator itr = counts.begin(); itr != counts.end(); ++itr)
            {
                const ThorimAddType& add = GetType(itr->first, FLAG_BOTH);

                if (Creature* spawn = me->SummonCreature(add.entry, CorridorSpawnPosition, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 8*IN_MILLISECONDS))
                    spawn->SetInCombatWithZone();
            }
        }

    private:
        InstanceScript* instance;
        SummonList summons;
        uint8 AlivePreAdds;
        uint8 AliveBossAdds;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_thorim_controllerAI(creature);
    }
};

class boss_thorim : public CreatureScript
{
public:
    boss_thorim() : CreatureScript("boss_thorim") { }

    struct boss_thorimAI : public BossAI
    {
        boss_thorimAI(Creature* creature) : BossAI(creature, BOSS_THORIM)
        {
            ASSERT(instance);
        }

        void InitializeAI() override
        {
            Reset();
            UpdateCriteria = false;
        }

        void Reset() override
        {
            if (!(events.IsInPhase(PHASE_OPENING)))
                Talk(SAY_WIPE);

            if (Creature* controller = me->FindNearestCreature(NPC_THORIM_CONTROLLER, 500.0f))
            {
                controller->AI()->JustDied(me);
                controller->DespawnOrUnsummon();
            }


            BossAI::Reset();

            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL | UNIT_FLAG_NON_ATTACKABLE);

            events.SetPhase(PHASE_OPENING);

            OrbSummoned = false;
            HardMode = true;
            InflictedByLightningCharge = 0;
            lastSpawn = 0;

            if (Creature* controller = me->SummonCreature(NPC_THORIM_CONTROLLER, *me))
                ControllerGUID = controller->GetGUID();
        }

        bool CanAIAttack(Unit const* target) const override
        {
            if (target->GetPositionY() > -159.527f && me->GetPositionY() > -159.527f)
                return false;

            return BossAI::CanAIAttack(target);
        }

        void KilledUnit(Unit* victim) override
        {
            if (!(rand32()%5))
                Talk(SAY_SLAY);

            if (victim->GetTypeId() == TYPEID_PLAYER)
                UpdateCriteria = true;
        }

        void JustDied(Unit* killer) override
        {
            BossAI::JustDied(killer);
            Talk(SAY_DEATH);

            instance->DoCompleteCriteria(CRITERIA_THORIM);
            instance->DoCompleteCriteria(CRITERIA_THORIM_I_WILL_TAKE_YOU_ALL_ON);

            if (!UpdateCriteria)
                instance->DoCompleteCriteria(CRITERIA_KILL_WITHOUT_DEATHS);

            me->setFaction(35);

            if (HardMode)
            {
                instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_LOSE_ILLUSION_10, ACHIEVEMENT_LOSE_ILLUSION_25));
                me->SummonGameObject(RAID_MODE(GO_CACHE_OF_STORMS_10_HM, GO_CACHE_OF_STORMS_25_HM),
                    CacheSummonPosition.GetPositionX(), CacheSummonPosition.GetPositionY(), CacheSummonPosition.GetPositionZ(), CacheSummonPosition.GetOrientation(),
                    0, 0, 1, 1, 7*DAY);
            }
            else
                me->SummonGameObject(RAID_MODE(GO_CACHE_OF_STORMS_10, GO_CACHE_OF_STORMS_25),
                    CacheSummonPosition.GetPositionX(), CacheSummonPosition.GetPositionY(), CacheSummonPosition.GetPositionZ(), CacheSummonPosition.GetOrientation(),
                    0, 0, 1, 1, 7*DAY);
            
            if (!InflictedByLightningCharge)
                instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_DONT_STAND_IN_THE_LIGHTNING_10, ACHIEVEMENT_DONT_STAND_IN_THE_LIGHTNING_25));
            

            if (Creature* controller = ObjectAccessor::GetCreature(*me, ControllerGUID))
                controller->DespawnOrUnsummon();
        }

        void EnterCombat(Unit* who) override
        {
            BossAI::EnterCombat(who);

            Talk(SAY_AGGRO_1);

            // Spawn Thunder Orbs
            for(uint8 n = 0; n < 7; n++)
                me->SummonCreature(NPC_THUNDER_ORB, ThunderOrbPositions[n], TEMPSUMMON_CORPSE_DESPAWN);

            InflictedByLightningCharge = false;

            events.SetPhase(PHASE_CORRIDOR);

            DoCastSelf(SPELL_SHEAT_OF_LIGHTNING);
            events.ScheduleEvent(EVENT_STORMHAMMER,        40*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
            events.ScheduleEvent(EVENT_CHARGE_ORB,         30*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
            events.ScheduleEvent(EVENT_SUMMON_ADDS,        20*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
            events.ScheduleEvent(EVENT_BERSERK,           300*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
            events.ScheduleEvent(EVENT_SAY_AGGRO,          10*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
            events.ScheduleEvent(EVENT_HARDMODE_TIMEOUT, MISC_HARD_MODE_TIMER, 0, PHASE_CORRIDOR);
        }

        void JustSummoned(Creature* summon) override
        {
            summons.Summon(summon);
            if (summon->GetEntry() == NPC_LIGHTNING_ORB)
                summon->GetMotionMaster()->MovePath(NPC_LIGHTNING_ORB * 10, true);
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell) override
        {
            if (spell->Id == SPELL_LIGHTNING_RELEASE && target->ToPlayer())
            {
                InflictedByLightningCharge = true;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SAY_AGGRO:
                        Talk(SAY_AGGRO_2);
                        break;
                    case EVENT_STORMHAMMER:
                        DoCast(SPELL_STORMHAMMER);
                        events.ScheduleEvent(EVENT_STORMHAMMER, urand(15, 20)*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
                        break;
                    case EVENT_CHARGE_ORB:
                        DoCastAOE(SPELL_CHARGE_ORB);
                        events.ScheduleEvent(EVENT_CHARGE_ORB, urand(15, 20)*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
                        break;
                    case EVENT_SUMMON_ADDS:
                        SpawnAdd();
                        events.ScheduleEvent(EVENT_SUMMON_ADDS, 10*IN_MILLISECONDS, 0, PHASE_CORRIDOR);
                        break;
                    case EVENT_PLATFORMKILL:
                    {
                        std::list<HostileReference*> threatList = me->GetThreatManager().getThreatList();
                        for (std::list<HostileReference*>::iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
                        {
                            Unit* player = (*itr)->getTarget();
                            if (!player || player->GetTypeId() != TYPEID_PLAYER)
                                continue;

                            if (GetSide(player) == SIDE_CORRIDOR)
                                me->Kill(player, true);
                        }
                        break;
                    }
                    case EVENT_UNBALANCING_STRIKE:
                        
                        // Add normalmode debuff if it somehow got lost
                        // Check this here and not in every update tick to improve effeciency
                        if (!HardMode && !me->HasAura(SPELL_TOUCH_OF_DOMINION))
                            me->AddAura(SPELL_TOUCH_OF_DOMINION, me);

                        DoCastVictim(SPELL_UNBALANCING_STRIKE);
                        events.ScheduleEvent(EVENT_UNBALANCING_STRIKE, urand(15, 20)*IN_MILLISECONDS, 0, PHASE_THORIM);
                        break;
                    case EVENT_CHAIN_LIGHTNING:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me, true)))
                            DoCast(target, SPELL_CHAIN_LIGHTNING);
                        events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, urand(7, 15)*IN_MILLISECONDS, 0, PHASE_THORIM);
                        break;
                    case EVENT_TRANSFER_ENERGY:
                        if (Creature* source = me->SummonCreature(NPC_POWER_SOURCE, PowerSourcePositions[urand(0, 6)], TEMPSUMMON_TIMED_DESPAWN, 9000))
                            me->AddAura(SPELL_LIGHTNING_PILLAR, source);
                        events.ScheduleEvent(EVENT_RELEASE_ENERGY, 8*IN_MILLISECONDS, 0, PHASE_THORIM);
                        break;
                    case EVENT_RELEASE_ENERGY:
                        if (Creature* source = me->FindNearestCreature(NPC_POWER_SOURCE, 100.0f))
                            me->CastSpell(source, SPELL_LIGHTNING_RELEASE);
                        DoCastSelf(SPELL_LIGHTNING_CHARGE, true);
                        events.ScheduleEvent(EVENT_TRANSFER_ENERGY, 8*IN_MILLISECONDS, 0, PHASE_THORIM);
                        break;
                    case EVENT_BERSERK:
                        if (events.IsInPhase(PHASE_CORRIDOR))
                        {
                            DoCastSelf(SPELL_BERSERK_PHASE_1, true);
                            DoCastSelf(SPELL_SUMMON_LIGHTNING_ORB, true);
                            instance->SetData(DATA_RUNIC_DOOR, GO_STATE_ACTIVE); // Open doors so Orb can pass
                            instance->SetData(DATA_STONE_DOOR, GO_STATE_ACTIVE);
                        }
                        Talk(SAY_BERSERK);
                        break;
                    case EVENT_HARDMODE_TIMEOUT:
                        HardMode = false;
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void SpawnAdd()
        {
            uint8 one = 0;
            uint8 two = 0;

            bool disChamp = (lastSpawn & SPAWN_FLAG_CHAMPION_DOUBLE) == SPAWN_FLAG_CHAMPION_DOUBLE;
            if (!disChamp && (lastSpawn & SPAWN_FLAG_CHAMPION_DOUBLE) && urand(0, 100) < 50)
                disChamp = true;

            if (urand(0, 100) < 10 && !disChamp)
            {
                one = SPAWN_FLAG_CHAMPION;
                two = SPAWN_FLAG_CHAMPION;
            }
            else if (urand(0, 100) < 40)
            {
                if (urand(0, 100) < 20 && !disChamp)
                {
                    one = urand(0, 1) ? SPAWN_FLAG_EVOKER : SPAWN_FLAG_WARBRINGER;
                    two = SPAWN_FLAG_CHAMPION;
                }
                else
                {
                    one = SPAWN_FLAG_EVOKER;
                    two = SPAWN_FLAG_WARBRINGER;
                }
            }
            else if (urand(0, 100) < 65)
            {
                if (urand(0, 100) < 30 && !disChamp)
                    one = SPAWN_FLAG_CHAMPION;
                else
                    one = urand(0, 1) ? SPAWN_FLAG_EVOKER : SPAWN_FLAG_WARBRINGER;

            }
            else
                one = SPAWN_FLAG_COMMONER;

            uint16 spawn = one | (two << 4);
            if (Creature* controller = ObjectAccessor::GetCreature(*me, ControllerGUID))
                controller->AI()->SetData(ACTION_SPAWN_ARENA_WAVE, spawn);
            lastSpawn = spawn;
        }

        void DamageTaken(Unit* attacker, uint32 &damage) override
        {
            if (damage >= me->GetHealth())
            {
                Map::PlayerList const &PlayerList = instance->instance->GetPlayers();
                AchievementEntry const* achievement = sAchievementStore.LookupEntry(RAID_MODE(ACHIEVEMENT_WHO_NEEDS_BLOODLUST_10, ACHIEVEMENT_WHO_NEEDS_BLOODLUST_25));

                if (achievement)
                    for (Player& player : me->GetMap()->GetPlayers())
                        if (player.HasAura(SPELL_AURA_OF_CELERITY) || player.HasAura(SPELL_AURA_OF_CELERITY_TRIGGER))
                            player.CompletedAchievement(achievement);
            }

            if (events.IsInPhase(PHASE_CORRIDOR))
            {
                if (Creature* controller = ObjectAccessor::GetCreature(*me, ControllerGUID))
                    if (controller->AI()->GetData(DATA_MINIBOSSES_DEAD))
                        if (attacker->GetTypeId() == TYPEID_PLAYER && me->IsWithinDistInMap(attacker, 10.0f))
                        {
                            Talk(SAY_JUMPDOWN);

                            events.SetPhase(PHASE_THORIM);

                            me->RemoveAurasDueToSpell(SPELL_SHEAT_OF_LIGHTNING);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);

                            me->GetMotionMaster()->MoveJump(ArenaCenterPosition, 10.0f, 20.0f);

                            events.ScheduleEvent(EVENT_UNBALANCING_STRIKE,  15*IN_MILLISECONDS, 0, PHASE_THORIM);
                            events.ScheduleEvent(EVENT_CHAIN_LIGHTNING,     20*IN_MILLISECONDS, 0, PHASE_THORIM);
                            events.ScheduleEvent(EVENT_TRANSFER_ENERGY,     20*IN_MILLISECONDS, 0, PHASE_THORIM);
                            events.CancelEvent(EVENT_BERSERK);
                            events.ScheduleEvent(EVENT_PLATFORMKILL,        10*IN_MILLISECONDS, 0, PHASE_THORIM);
                            // Hard Mode
                            if (HardMode)
                            {
                                me->SummonCreature(NPC_SIF, SifSummonPosition, TEMPSUMMON_CORPSE_DESPAWN);
                                instance->DoCompleteAchievement(RAID_MODE(ACHIEVEMENT_SIFFED_10, ACHIEVEMENT_SIFFED_25));
                            }
                            else
                            {
                                me->AddAura(SPELL_TOUCH_OF_DOMINION, me);
                            }
                        }
            }
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case ACTION_ARENA_DIED:
                    if (!OrbSummoned)
                    {
                        events.RescheduleEvent(EVENT_BERSERK, 1*IN_MILLISECONDS);
                        OrbSummoned = true;
                    }
                    break;
            }
        }

        void SetData(uint32 id, uint32 data) override
        {
            switch (id)
            {
                case DATA_LIGHTNING_CHARGE:
                    InflictedByLightningCharge = data ? true : false;
                    break;
                case DATA_ACHIEVEMENT: 
                    if (data == DATA_LIGHTNING)
                        UpdateCriteria = true;
            }
        }

        private:
            ObjectGuid ControllerGUID;
            bool InflictedByLightningCharge;
            bool HardMode;
            bool OrbSummoned;
            uint16 lastSpawn;
            bool UpdateCriteria;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_thorimAI(creature);
    }
};

class npc_runic_colossus : public CreatureScript
{
    public:
        npc_runic_colossus() : CreatureScript("npc_runic_colossus") { }

        struct npc_runic_colossusAI : public ScriptedAI
        {
            npc_runic_colossusAI(Creature* creature) : ScriptedAI(creature), events(), summons(me), instance(me->GetInstanceScript())
            {
                ASSERT(instance);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            }

            void Reset() override
            {
                summons.DespawnAll();
                events.Reset();

                // Runed Door closed
                instance->SetData(DATA_RUNIC_DOOR, GO_STATE_READY);

                std::list<Creature*> HelperList;
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_IRON_HONOR_GUARD_C,  42.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_DARK_RUNE_ACOLYTE_C, 42.0f);
                if (!HelperList.empty())
                    for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                        (*itr)->setFaction(FACTION_FRIENDLY_TO_ALL);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                if (target->GetPositionY() > -159.527f && me->GetPositionY() > -159.527f)
                    return false;

                return true;
            }

            void EnterCombat(Unit* /*who*/) override
            {
                events.SetPhase(PHASE_COLOSSUS_FIGHT);
                events.CancelEvent(EVENT_COLOSSUS_RUNIC_SMASH);
                events.ScheduleEvent(EVENT_COLOSSUS_SMASH, urand(15, 18)*IN_MILLISECONDS, 0, PHASE_COLOSSUS_FIGHT);
                events.ScheduleEvent(EVENT_COLOSSUS_BARRIER, urand(12, 15)*IN_MILLISECONDS, 0, PHASE_COLOSSUS_FIGHT);
                events.ScheduleEvent(EVENT_COLOSSUS_CHARGE, urand(20, 24)*IN_MILLISECONDS, 0, PHASE_COLOSSUS_FIGHT);

                me->InterruptNonMeleeSpells(true);
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    if (Creature* thorim = me->FindNearestCreature(NPC_THORIM, 1000.0f))
                        thorim->AI()->SetData(DATA_ACHIEVEMENT, DATA_LIGHTNING);
            }

            void JustDied(Unit* /*victim*/) override
            {
                summons.DespawnAll();
                events.Reset();

                instance->SetData(DATA_RUNIC_DOOR, GO_STATE_ACTIVE);

                if (Creature* source = me->FindNearestCreature(NPC_RUNE_GIANT, 250.0f ,true))
                    source->AI()->DoAction(ACTION_GIANT_ACTIVATE);

                std::list<Creature*> HelperList;
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_IRON_HONOR_GUARD_C,  200.0f);
                me->GetCreatureListWithEntryInGrid(HelperList, NPC_DARK_RUNE_ACOLYTE_C, 200.0f);
                if (!HelperList.empty())
                    for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                        (*itr)->setFaction(FACTION_HOSTILE);
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_COLOSSUS_START:
                        if (events.IsInPhase(PHASE_COLOSSUS_CORRIDOR) || !me->IsAlive())
                            break;
                        events.SetPhase(PHASE_COLOSSUS_CORRIDOR);
                        events.RescheduleEvent(EVENT_COLOSSUS_RUNIC_SMASH, 1*IN_MILLISECONDS, 0, PHASE_COLOSSUS_CORRIDOR);
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (events.ExecuteEvent() == EVENT_COLOSSUS_RUNIC_SMASH)
                {
                    DoCast(urand(0, 1) ? SPELL_RUNIC_SMASH_LEFT : SPELL_RUNIC_SMASH_RIGHT);
                    events.Repeat(5*IN_MILLISECONDS);
                }

                if (!UpdateVictim())
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_COLOSSUS_BARRIER:
                            me->MonsterTextEmote(EMOTE_BARRIER, 0, true);
                            DoCastSelf(SPELL_RUNIC_BARRIER);
                            events.ScheduleEvent(EVENT_COLOSSUS_BARRIER, urand(35, 45)*IN_MILLISECONDS, 0, PHASE_COLOSSUS_FIGHT);
                            break;
                        case EVENT_COLOSSUS_SMASH:
                            DoCastVictim(SPELL_SMASH);
                            events.ScheduleEvent(EVENT_COLOSSUS_SMASH, urand(15, 18)*IN_MILLISECONDS, 0, PHASE_COLOSSUS_FIGHT);
                            break;
                        case EVENT_COLOSSUS_CHARGE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, NonTankTargetSelector(me, true)))
                                DoCast(target, SPELL_RUNIC_CHARGE);
                            events.ScheduleEvent(EVENT_COLOSSUS_CHARGE, 20*IN_MILLISECONDS, 0, PHASE_COLOSSUS_FIGHT);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* instance;
            EventMap events;
            SummonList summons;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_runic_colossusAI(creature);
        }
};

class npc_runic_smash : public CreatureScript
{
    public:
        npc_runic_smash() : CreatureScript("npc_runic_smash") { }

        struct npc_runic_smashAI : public ScriptedAI
        {
            npc_runic_smashAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
                me->SetReactState(REACT_PASSIVE);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
                ExplodeTimer = 10*IN_MILLISECONDS;
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == DATA_RUNIC_SMASH_BUNNY_TIMER)
                    ExplodeTimer = data;
            }

            bool CanAIAttack(Unit const* target) const override
            {
                if (target->GetPositionY() > -159.527f && me->GetPositionY() > -159.527f)
                    return false;

                return true;
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    if (Creature* thorim = me->FindNearestCreature(NPC_THORIM, 1000.0f))
                        thorim->AI()->SetData(DATA_ACHIEVEMENT, DATA_LIGHTNING);
            }

            void UpdateAI(uint32 diff) override
            {
                if (ExplodeTimer <= diff)
                {
                    DoCastAOE(SPELL_RUNIC_SMASH, true);
                    ExplodeTimer = 10*IN_MILLISECONDS;
                }
                else
                    ExplodeTimer -= diff;
            }

            private:
                uint32 ExplodeTimer;
        };


        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_runic_smashAI(creature);
        }
};

class npc_ancient_rune_giant : public CreatureScript
{
    public:
        npc_ancient_rune_giant() : CreatureScript("npc_ancient_rune_giant") { }

        struct npc_ancient_rune_giantAI : public ScriptedAI
        {
            npc_ancient_rune_giantAI(Creature* creature) : ScriptedAI(creature), events(), summons(me), instance(me->GetInstanceScript())
            {
                ASSERT(instance);
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            }

            void Reset() override
            {
                events.Reset();
                summons.DespawnAll();

                spawn = 2;

                // Stone Door closed
                instance->SetData(DATA_STONE_DOOR, GO_STATE_READY);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                if (target->GetPositionY() > -159.527f && me->GetPositionY() > -159.527f)
                    return false;

                return true;
            }

            void EnterCombat(Unit* /*pWho*/) override
            {
                events.SetPhase(PHASE_GIANT_FIGHT);
                events.CancelEvent(EVENT_GIANT_SPAWN_WAVE);
                events.ScheduleEvent(EVENT_GIANT_STOMP, urand(10, 12)*IN_MILLISECONDS, 0, PHASE_GIANT_FIGHT);
                events.ScheduleEvent(EVENT_GIANT_DETONATION, 25*IN_MILLISECONDS, 0, PHASE_GIANT_FIGHT);

                me->MonsterTextEmote(EMOTE_MIGHT, 0, true);
                DoCastSelf(SPELL_RUNIC_FORTIFICATION, true);
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    if (Creature* thorim = me->FindNearestCreature(NPC_THORIM, 1000.0f))
                        thorim->AI()->SetData(DATA_ACHIEVEMENT, DATA_LIGHTNING);
            }

            void JustDied(Unit* /*victim*/) override
            {
                instance->SetData(DATA_STONE_DOOR, GO_STATE_ACTIVE);
            }

            void DoAction(int32 action) override
            {
                switch (action)
                {
                    case ACTION_GIANT_ACTIVATE:
                        events.SetPhase(PHASE_GIANT_SPAWNING);
                        events.ScheduleEvent(EVENT_GIANT_SPAWN_WAVE, 3*IN_MILLISECONDS, 0, PHASE_GIANT_SPAWNING);
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                if (events.ExecuteEvent() == EVENT_GIANT_SPAWN_WAVE)
                {
                    if (Creature* controller = me->FindNearestCreature(NPC_THORIM_CONTROLLER, 500.0f))
                    {
                        if (spawn == 2)
                        {
                            controller->AI()->SetData(ACTION_SPAWN_CORRIDOR_WAVE, SPAWN_FLAG_ACOLYTE | SPAWN_FLAG_GUARD);
                            spawn = urand(0, 1);
                        }
                        else
                        {
                            controller->AI()->SetData(ACTION_SPAWN_CORRIDOR_WAVE, spawn == 0 ? SPAWN_FLAG_ACOLYTE : SPAWN_FLAG_GUARD);
                            spawn = 1 - spawn;
                        }
                    }
                    events.CancelEvent(EVENT_GIANT_SPAWN_WAVE);
                    events.ScheduleEvent(EVENT_GIANT_SPAWN_WAVE, urand(12, 15)*IN_MILLISECONDS, 0, PHASE_GIANT_SPAWNING);
                }

                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_GIANT_STOMP:
                            DoCastSelf(SPELL_STOMP);
                            events.ScheduleEvent(EVENT_GIANT_STOMP, urand(10, 12)*IN_MILLISECONDS, 0, PHASE_GIANT_FIGHT);
                            break;
                        case EVENT_GIANT_DETONATION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40, true))
                                 DoCast(target, SPELL_RUNE_DETONATION);
                            events.ScheduleEvent(EVENT_GIANT_DETONATION, urand(10, 12)*IN_MILLISECONDS, 0, PHASE_GIANT_FIGHT);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            private:
                InstanceScript* instance;
                EventMap events;
                SummonList summons;
                uint8 spawn;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ancient_rune_giantAI(creature);
        }
};

class npc_thorim_add : public CreatureScript
{
    public:
        npc_thorim_add() : CreatureScript("npc_thorim_add") { }

        struct npc_thorim_addAI : public ThorimAddAI
        {
            npc_thorim_addAI(Creature *creature) : ThorimAddAI(creature) {}

            void EnterCombat(Unit* who) override
            {
                if (role.id == DARK_RUNE_WARBRINGER)
                    DoCast(role.spells[1].spell);

                ThorimAddAI::EnterCombat(who);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                if (target->GetPositionY() > -159.527f && me->GetPositionY() > -159.527f)
                    return false;

                return true;
            }

            void UpdateImpl(const uint32 /*diff*/) override
            {
                if (role.flags & FLAG_HEALER)
                {
                    if (uint32 eventId = events.ExecuteEvent())
                    {
                        Unit* target = DoSelectLowestHpFriendly(30.0f);
                        if (!target)
                            target = me;

                        DoCast(target, role.spells[eventId - 1].spell);
                        RescheduleEvent(eventId);
                    }
                }
                else
                {
                    if (uint32 eventId = events.ExecuteEvent())
                    {
                        DoCast(role.spells[eventId - 1].spell);
                        RescheduleEvent(eventId);
                    }
                }

                if (role.spells[2].spell && ((role.flags & FLAG_HEALER) || role.id == MERCENARY_SOLDIER))
                    DoSpellAttackIfReady(role.spells[2].spell);
                else
                    DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_thorim_addAI(creature);
        }
};

class npc_thorim_pre_healer : public CreatureScript
{
    public:
        npc_thorim_pre_healer() : CreatureScript("npc_thorim_pre_healer") { }

        enum PreHealerEvents
        {
            EVENT_CIRCLE = 1,
            EVENT_RENEW  = 2,
            EVENT_HEAL   = 3,
        };

        struct npc_thorim_pre_healerAI : public ThorimAddAI
        {
            npc_thorim_pre_healerAI(Creature *creature) : ThorimAddAI(creature)
            {
                ASSERT(instance);
            }

            void Reset() override
            {
                ThorimAddAI::Reset();

                me->SetReactState(REACT_PASSIVE);

                events.ScheduleEvent(EVENT_CIRCLE, 10*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_RENEW,  15*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_HEAL,   20*IN_MILLISECONDS);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                if (target->GetPositionY() > -159.527f && me->GetPositionY() > -159.527f)
                    return false;

                return true;
            }

            void EnterCombat(Unit* /*who*/) override { }

            void DamageTaken(Unit* attacker, uint32& damage) override
            {
                ThorimAddAI::DamageTaken(attacker, damage);

                if (!EncounteredPlayer && (attacker->GetTypeId() == TYPEID_PLAYER || attacker->IsPet() || attacker->IsTotem()))
                    events.CancelEvent(EVENT_RENEW);
            }

            void UpdateAI(uint32 diff) override
            {
                if (EncounteredPlayer)
                    if (!UpdateVictim())
                        return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                if (uint32 eventId = events.ExecuteEvent())
                {
                    Unit* target = DoSelectLowestHpFriendly(40.0f);
                    if (target)
                        switch (eventId)
                        {
                            case EVENT_CIRCLE:
                                DoCast(target, SPELL_CIRCLE_OF_HEALING_P_EVENT);
                                events.RescheduleEvent(EVENT_CIRCLE, !EncounteredPlayer ? 10000 : 25000);
                                break;
                            case EVENT_HEAL:
                                if (!EncounteredPlayer)
                                    DoCast(target, SPELL_GREATER_HEAL_P_EVENT);
                                else
                                    DoCast(target, SPELL_GREATER_HEAL_P);
                                events.RescheduleEvent(EVENT_HEAL, 10000);
                                break;
                            case EVENT_RENEW:
                                DoCast(target, SPELL_RENEW_P_EVENT);
                                events.RescheduleEvent(EVENT_RENEW, 15000);
                                break;
                        }
                }

                if (me->IsInCombat() && EncounteredPlayer)
                    DoSpellAttackIfReady(SPELL_HOLY_SMITE_P);
            }

            void UpdateImpl(const uint32 /*diff*/) {}
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_thorim_pre_healerAI(creature);
        }
};

class npc_sif : public CreatureScript
{
    public:
        npc_sif() : CreatureScript("npc_sif") { }

        struct npc_sifAI : public ScriptedAI
        {
            npc_sifAI(Creature* creature) : ScriptedAI(creature), events() {}

            void Reset() override
            {
                events.Reset();

                me->SetReactState(REACT_PASSIVE);

                me->GetMotionMaster()->MoveRandom(30.0f);

                events.ScheduleEvent(EVENT_SIF_FROSTBOLT, 2*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SIF_FROST_VOLLEY, 15*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SIF_BLIZZARD, 30*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SIF_FROSTNOVA, urand(20, 25)*IN_MILLISECONDS);
            }

            bool CanAIAttack(Unit const* target) const override
            {
                if (target->GetPositionY() > -159.527f && me->GetPositionY() > -159.527f)
                    return false;

                return true;
            }

            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    if (Creature* thorim = me->FindNearestCreature(NPC_THORIM, 1000.0f))
                        thorim->AI()->SetData(DATA_ACHIEVEMENT, DATA_LIGHTNING);
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SIF_FROSTBOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                                DoCast(target, SPELL_FROSTBOLT);
                            events.ScheduleEvent(EVENT_SIF_FROSTBOLT, 4*IN_MILLISECONDS);
                            return;
                        case EVENT_SIF_FROST_VOLLEY:
                            DoCastAOE(SPELL_FROSTBOLT_VOLLEY);
                            events.ScheduleEvent(EVENT_SIF_FROST_VOLLEY, urand(15, 20)*IN_MILLISECONDS);
                            return;
                        case EVENT_SIF_BLIZZARD:
                            DoCastAOE(SPELL_BLIZZARD);
                            events.ScheduleEvent(EVENT_SIF_BLIZZARD, urand (35, 45) * IN_MILLISECONDS);
                            return;
                        case EVENT_SIF_FROSTNOVA:
                            DoCastAOE(SPELL_FROSTNOVA);
                            events.ScheduleEvent(EVENT_SIF_FROSTNOVA, urand(20, 25)*IN_MILLISECONDS);
                            return;
                    }
                }
            }

            private:
                EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_sifAI(creature);
        }
};

class spell_impale_aura : public SpellScriptLoader
{
    public:
        spell_impale_aura() : SpellScriptLoader("spell_impale_aura") { }

        class spell_impale_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_impale_aura_AuraScript);

            void OnPeriodic(AuraEffect const* /*aurEff*/)
            {
                Unit* owner = GetUnitOwner();
                if (!owner)
                    return;

                if (owner->GetHealthPct() >= GetSpellInfo()->Effects[EFFECT_1].CalcValue())
                    owner->RemoveAurasDueToSpell(GetSpellInfo()->Id);

            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_impale_aura_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_impale_aura_AuraScript();
        }
};


class NotInArenaCheck
{
    public:
        bool operator() (WorldObject* object)
        {
            if (!object->ToUnit())
                return true;

            return GetSide(object->ToUnit()) != SIDE_ARENA;
        }
};

class spell_stormhammer_targeting : public SpellScriptLoader
{
    public:
        spell_stormhammer_targeting() : SpellScriptLoader("spell_stormhammer_targeting") { }

        class spell_stormhammer_targeting_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_stormhammer_targeting_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(NotInArenaCheck());
                Trinity::Containers::RandomResize(unitList, 1);

                targets = unitList;
            }

            void SetTarget(std::list<WorldObject*>& unitList)
            {
                unitList = targets;
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_stormhammer_targeting_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_stormhammer_targeting_SpellScript::SetTarget, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_stormhammer_targeting_SpellScript::SetTarget, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
            }

            std::list<WorldObject*> targets;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_stormhammer_targeting_SpellScript();
        }
};

class spell_lightning_charge : public SpellScriptLoader
{
    public:
        spell_lightning_charge() : SpellScriptLoader("spell_lightning_charge") { }

        class spell_lightning_charge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_lightning_charge_SpellScript);

            void HandleScript(SpellEffIndex /*eff*/)
            {
                if (GetHitDamage() >= 1)
                    if (GetCaster()->GetAI())
                        GetCaster()->GetAI()->SetData(DATA_LIGHTNING_CHARGE, 1);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_lightning_charge_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_lightning_charge_SpellScript();
        }
};

class spell_runic_smash : public SpellScriptLoader
{
    public:
        spell_runic_smash() : SpellScriptLoader("spell_runic_smash") { }

        class spell_runic_smash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_runic_smash_SpellScript);

            void HandleScript(SpellEffIndex /*eff*/)
            {
                int32 bp = GetSpellInfo()->Effects[EFFECT_0].BasePoints;

                uint32 entry = bp == 0 ? NPC_GOLEM_BUNNY_LEFT : NPC_GOLEM_BUNNY_RIGHT;
                Unit* caster = GetCaster();

                for (uint8 i = 0; i < 16; ++i)
                {
                    float x = caster->GetPositionX();
                    float y = caster->GetPositionY() + i * 10;
                    float z = caster->GetPositionZ();

                    for (uint8 j = 1; j < 3; ++j)
                        if (Creature* bunny = caster->SummonCreature(entry, x + (10 * j * (bp == 0 ? -1 : 1)), y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 5000))
                            bunny->AI()->SetData(DATA_RUNIC_SMASH_BUNNY_TIMER, (i + 1) * 200);
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_runic_smash_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_TRIGGER_SPELL);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_runic_smash_SpellScript();
        }
};

// 61934 - Leap
class spell_thorim_arena_leap : public SpellScriptLoader
{
    public:
        spell_thorim_arena_leap() : SpellScriptLoader("spell_thorim_arena_leap") { }

        class spell_thorim_arena_leap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_thorim_arena_leap_SpellScript);

            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (GetHitDest())
                    GetCaster()->ToCreature()->SetHomePosition(*GetHitDest());
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_thorim_arena_leap_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_JUMP_DEST);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_thorim_arena_leap_SpellScript();
        }
};

class HeightPositionCheck
{
    public:
        HeightPositionCheck(bool ret) : _ret(ret) { }

        bool operator() (WorldObject* obj) const
        {
            return (obj->GetPositionZ() > 425.0f) == _ret;
        }

private:
    bool _ret;
};

// 62577 - Blizzard
// 62603 - Blizzard
class spell_thorim_blizzard : public SpellScriptLoader
{
    public:
        spell_thorim_blizzard() : SpellScriptLoader("spell_thorim_blizzard") { }

        class spell_thorim_blizzard_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_thorim_blizzard_SpellScript);

            void SelectTarget(std::list<WorldObject*>& targets)
            {
                targets.clear();

                if (Creature* blizzard = ObjectAccessor::GetCreature(*GetCaster(), ObjectGuid(_instance->GetGuidData(DATA_SIF_BLIZZARD))))
                    targets.push_back(blizzard);
            }

            bool Load() override
            {
                _instance = GetCaster()->GetInstanceScript();
                return _instance != nullptr;
            }

            InstanceScript* _instance;

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_thorim_blizzard_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

    SpellScript* GetSpellScript() const override
    {
        return new spell_thorim_blizzard_SpellScript();
    }
};

// 62576 - Blizzard
// 62602 - Blizzard
class spell_thorim_blizzard_effect : public SpellScriptLoader
{
    public:
        spell_thorim_blizzard_effect() : SpellScriptLoader("spell_thorim_blizzard_effect") { }

        class spell_thorim_blizzard_effect_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_thorim_blizzard_effect_AuraScript);

            bool CheckAreaTarget(Unit* target)
            {
                /// @todo: fix this for all dynobj auras
                if (target != GetOwner())
                {
                    // check if not stacking aura already on target
                    // this one prevents overriding auras periodically by 2 near area aura owners
                    Unit::AuraApplicationMap const& auraMap = target->GetAppliedAuras();
                    for (Unit::AuraApplicationMap::const_iterator iter = auraMap.begin(); iter != auraMap.end(); ++iter)
                    {
                        Aura const* aura = iter->second->GetBase();
                        if (GetId() == aura->GetId() && GetOwner() != aura->GetOwner() /*!GetAura()->CanStackWith(aura)*/)
                            return false;
                    }
                }
                return true;
            }

            void Register() override
            {
               DoCheckAreaTarget += AuraCheckAreaTargetFn(spell_thorim_blizzard_effect_AuraScript::CheckAreaTarget);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_thorim_blizzard_effect_AuraScript();
        }
};

void AddSC_boss_thorim()
{
    new boss_thorim();
    new npc_thorim_controller();
    new npc_thorim_add();
    new npc_thorim_pre_healer();
    new npc_runic_colossus();
    new npc_runic_smash();
    new npc_ancient_rune_giant();
    new npc_sif();
    new spell_impale_aura();
    new spell_stormhammer_targeting();
    new spell_lightning_charge();
    new spell_runic_smash();
    new spell_thorim_arena_leap();
    new spell_thorim_blizzard();
    new spell_thorim_blizzard_effect();
}
