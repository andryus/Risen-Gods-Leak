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
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "GameEventMgr.h"
#include "onyxias_lair.h"

enum Yells
{
    // Say
    SAY_AGGRO                   = 0,
    SAY_KILL                    = 1,
    SAY_PHASE_2_TRANS           = 2,
    SAY_PHASE_3_TRANS           = 3,

    // Emote
    EMOTE_BREATH                = 4
};

enum Spells
{
    // Phase 1 spells
    SPELL_WING_BUFFET           = 18500,
    SPELL_FLAME_BREATH          = 18435,
    SPELL_FLAME_BREATH_25       = 68970,
    SPELL_CLEAVE                = 68868,
    SPELL_TAIL_SWEEP            = 68867,

    // Phase 2 spells
    SPELL_FIREBALL              = 18392,

    // One Spell triggers another one, spell targets are defined in db (spell_target_position)
    SPELL_BREATH_NORTH_TO_SOUTH = 17086,                    // 20x in "array"
    SPELL_BREATH_SOUTH_TO_NORTH = 18351,                    // 11x in "array"

    SPELL_BREATH_EAST_TO_WEST   = 18576,                    // 7x in "array"
    SPELL_BREATH_WEST_TO_EAST   = 18609,                    // 7x in "array"

    SPELL_BREATH_SE_TO_NW       = 18564,                    // 12x in "array"
    SPELL_BREATH_NW_TO_SE       = 18584,                    // 12x in "array"
    SPELL_BREATH_SW_TO_NE       = 18596,                    // 12x in "array"
    SPELL_BREATH_NE_TO_SW       = 18617,                    // 12x in "array"

    // Phase 3 spells
    SPELL_BELLOWING_ROAR         = 18431
};

struct OnyxMove
{
    uint8 LocId;
    uint8 LocIdEnd;
    uint32 SpellId;
    float fX, fY, fZ;
};

static const OnyxMove MoveData[8]=
{
    {0, 1, SPELL_BREATH_WEST_TO_EAST,   -33.5561f, -182.682f, -56.9457f}, //west
    {1, 0, SPELL_BREATH_EAST_TO_WEST,   -31.4963f, -250.123f, -55.1278f}, //east
    {2, 4, SPELL_BREATH_NW_TO_SE,         6.8951f, -180.246f, -55.896f},  //north-west
    {3, 5, SPELL_BREATH_NE_TO_SW,        10.2191f, -247.912f, -55.896f},  //north-east
    {4, 2, SPELL_BREATH_SE_TO_NW,       -63.5156f, -240.096f, -55.477f},  //south-east
    {5, 3, SPELL_BREATH_SW_TO_NE,       -58.2509f, -189.020f, -55.790f},  //south-west
    {6, 7, SPELL_BREATH_SOUTH_TO_NORTH, -65.8444f, -213.809f, -55.2985f}, //south
    {7, 6, SPELL_BREATH_NORTH_TO_SOUTH,  22.8763f, -217.152f, -55.0548f}, //north
};

Position const MiddleRoomLocation = {-23.6155f, -215.357f, -55.7344f, 0.0f};

Position const Phase2Location = {-80.924f, -214.299f, -82.942f, 0.0f};

Position const SpawnLocations[3]=
{
    //Whelps
    {-30.127f, -254.463f, -89.440f, 0.0f},
    {-30.817f, -177.106f, -89.258f, 0.0f},
    //Lair Guard
    {-145.950f, -212.831f, -68.659f, 0.0f}
};

enum WayPoints
{
    WP_MOVE_DEEP_BREATH_CASTED  = 8,
    WP_MOVE_PHASE_2_END         = 9,
    WP_MOVE_PHASE_2_START       = 10,
    WP_MOVE_PHASE_2             = 11,
};

enum Events
{
    EVENT_NONE, // this is just to prevent a real event from getting the number 0, which is never executed

    EVENT_FLAME_BREATH,
    EVENT_CLEAVE,
    EVENT_TAIL_SWEEP,
    EVENT_WING_BUFFET,

    EVENT_FIREBALL,
    EVENT_SPAWN_WHELP,
    EVENT_SPAWN_LAIR_GUARD,
    EVENT_DEEP_BREATH,

    EVENT_BELLOWING_ROAR,
    EVENT_SPAWN_WHELP_PHASE_3,
};

enum Phases
{
    PHASE_START     = 1,
    PHASE_AIR       = 2,
    PHASE_END       = 3
};

class boss_onyxia : public CreatureScript
{
    public:
        boss_onyxia() : CreatureScript("boss_onyxia") { }
    
        struct boss_onyxiaAI : public BossAI
        {
            boss_onyxiaAI(Creature* creature) : BossAI(creature, BOSS_ONYXIA) { }
    
            void Reset() override
            {
                BossAI::Reset();
                summons.DespawnAll();
                SetCombatMovement(true);
                Phase = PHASE_START;
                SummonWhelpCount = 0;
                IsMoving = false;
    
                me->SetReactState(REACT_AGGRESSIVE);
            }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
    
                Talk(SAY_AGGRO);
                SetPhase(PHASE_START);
            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);
            }
    
            void JustSummoned(Creature* summon) override
            {
                summons.Summon(summon);
                summon->SetInCombatWithZone();
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 400.0f))
                    summon->AI()->AttackStart(target);
    
                switch (summon->GetEntry())
                {
                    case NPC_WHELP:
                        SummonWhelpCount++;
                        break;
                    case NPC_LAIRGUARD:
                        summon->setActive(true);
                        break;
                }
            }
    
            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() != TYPEID_PLAYER)
                    return;
    
                Talk(SAY_KILL);
            }
    
            void SpellHitTarget(Unit* target, const SpellInfo* Spell) override
            {
                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;
    
                if ((Spell->SpellIconID == 11) && (Spell->Id != (uint32)RAID_MODE(SPELL_FLAME_BREATH, SPELL_FLAME_BREATH_25)) && (Spell->Id != SPELL_BELLOWING_ROAR)) // make sure to exclude Flame Breath and Bellowing Roar from this check
                {
                    if (instance)
                        instance->SetData(DATA_SHE_DEEP_BREATH_MORE, FAIL);
                }
            }
            
            void SpellHit(Unit* /*caster*/, const SpellInfo* Spell) override
            {
                if (Spell->SpellIconID == 11)
                {
                    PointDataId = GetMoveData();
                    if (PointDataId != -1)
                        CurrentMovePoint = MoveData[PointDataId].LocIdEnd;
    
                    me->SetSpeedRate(MOVE_FLIGHT, 1.5f);
                    me->GetMotionMaster()->MovePoint(WP_MOVE_DEEP_BREATH_CASTED, MiddleRoomLocation);
                }
            }
    
            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == POINT_MOTION_TYPE)
                {
                    switch (id)
                    {
                        case WP_MOVE_DEEP_BREATH_CASTED:
                            PointDataId = GetMoveData();
                            if (PointDataId != -1)
                            {
                                me->SetSpeedRate(MOVE_FLIGHT, 1.0f);
                                me->GetMotionMaster()->MovePoint(MoveData[PointDataId].LocId, MoveData[PointDataId].fX, MoveData[PointDataId].fY, MoveData[PointDataId].fZ);
                            }
                            break;
                        case WP_MOVE_PHASE_2_END:
                            me->GetMotionMaster()->MoveChase(me->GetVictim());
                            break;
                        case WP_MOVE_PHASE_2_START:
                            // start flying
                            me->SetCanFly(true);
                            me->SetDisableGravity(true);
                            IsMoving = false;
                            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
                            me->GetMotionMaster()->MovePoint(WP_MOVE_PHASE_2, Phase2Location.GetPositionX(), Phase2Location.GetPositionY(), Phase2Location.GetPositionZ()+25);
                            me->SetSpeedRate(MOVE_FLIGHT, 1.0f);
                            Talk(SAY_PHASE_2_TRANS);
                            break;
                        case WP_MOVE_PHASE_2:
                            SetNextRandomPoint();
                            PointDataId = GetMoveData();
    
                            if (PointDataId == -1)
                                return;
    
                            me->GetMotionMaster()->MovePoint(MoveData[PointDataId].LocId, MoveData[PointDataId].fX, MoveData[PointDataId].fY, MoveData[PointDataId].fZ);
                            IsMoving = true;
                            break;
                        default:
                            me->SetOrientation(me->GetAngle(&MiddleRoomLocation));
                            IsMoving = false;
                            break;
                    }
                }
            }
    
            int8 GetMoveData()
            {
                uint8 MaxCount = 8;
    
                for (uint8 i = 0; i < MaxCount; ++i)
                {
                    if (MoveData[i].LocId == CurrentMovePoint)
                        return i;
                }
                return -1;
            }
    
            void SetNextRandomPoint()
            {
                uint8 MaxCount = 8;
    
                uint8 temp = urand(0, MaxCount-1);
    
                if (temp == CurrentMovePoint)
                    ++temp;
    
                temp = temp % MaxCount;
    
                CurrentMovePoint = temp;
            }
    
            void DamageTaken(Unit* /*who*/, uint32& /*damage*/) override
            {
                if (Phase != PHASE_END)
                {
                    if (HealthBelowPct(65) && Phase == PHASE_START)
                        SetPhase(PHASE_AIR);
                    else if (HealthBelowPct(40) && Phase == PHASE_AIR)
                        SetPhase(PHASE_END);
                }
            }
    
            void SetPhase(uint8 _phase)
            {
                events.Reset();
                
                switch (_phase)
                {
                    case PHASE_AIR:
                    {
                        events.RescheduleEvent(EVENT_FIREBALL,         urand(2, 6) * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_SPAWN_WHELP,               10 * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_SPAWN_LAIR_GUARD,          25 * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_DEEP_BREATH,               85 * IN_MILLISECONDS);
                        SetCombatMovement(false);
                        IsMoving = true;
                        me->GetMotionMaster()->MovePoint(WP_MOVE_PHASE_2_START, Phase2Location);
                        if (instance)
                            instance->SetData(DATA_ONYXIA_PHASE_2, 0);
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        Phase = PHASE_AIR;
                        MovementTimer = 1 * IN_MILLISECONDS;
                        break;
                    }
                    case PHASE_END:
                    {
                        events.CancelEvent(EVENT_SPAWN_WHELP);
                        events.RescheduleEvent(EVENT_SPAWN_WHELP_PHASE_3, 2 * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_BELLOWING_ROAR,     15 * IN_MILLISECONDS);
                        Talk(SAY_PHASE_3_TRANS);
                        SetCombatMovement(true);
                        // stop flying
                        me->SetCanFly(false);
                        me->SetDisableGravity(false);
                        me->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);
    
                        me->SetReactState(REACT_AGGRESSIVE);
                        IsMoving = false;
                        me->GetMotionMaster()->MovePoint(WP_MOVE_PHASE_2_END, me->GetHomePosition());
                        Phase = PHASE_END;
                    }
                    case PHASE_START:
                    {
                        events.RescheduleEvent(EVENT_FLAME_BREATH,              20 * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_CLEAVE,           urand(2, 5) * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_TAIL_SWEEP,     urand(15, 20) * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_WING_BUFFET,    urand(10, 20) * IN_MILLISECONDS);
                        break;
                    }
                }
                //if (instance)
                //    instance->SetData(DATA_ONYXIA_PHASE_2, Phase);
            }
    
            bool CheckCombatState()
            {
                if (!me->IsInCombat())
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                    {
                        if (player.IsAlive() && !player.IsGameMaster() && player.IsWithinDist2d(me, 35.0f))
                        {
                            // Player entered Triggerarea
                            me->SetInCombatWith(&player);
                            AddThreat(&player, 1.0f);
                            player.SetInCombatWith(me);
                            me->SetInCombatWithZone();
                            AttackStart(&player);
                            return true;
                        }
                    }
                }
                else
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                        if (player.IsAlive() && !player.IsGameMaster())
                            return true; // Return when at least one player is alive
    
                    EnterEvadeMode();
                }
                return false;
            }
    
            void UpdateAI(uint32 diff) override
            {
                //! UpdateVictim() replacement
                if (!CheckCombatState())
                    return;
    
                if (me->HasUnitState(UNIT_STATE_CASTING) && Phase != PHASE_AIR)
                    return;
    
                events.Update(diff);
    
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_FLAME_BREATH:
                        {
                            DoCastVictim(SPELL_FLAME_BREATH);
                            events.RescheduleEvent(EVENT_FLAME_BREATH, 20 * IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_CLEAVE:
                        {
                            DoCastVictim(SPELL_CLEAVE);
                            events.RescheduleEvent(EVENT_CLEAVE, 7 * IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_TAIL_SWEEP:
                        {
                            DoCastAOE(SPELL_TAIL_SWEEP);
                            events.RescheduleEvent(EVENT_TAIL_SWEEP, urand(15, 20) * IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_WING_BUFFET:
                        {
                            DoCastVictim(SPELL_WING_BUFFET);
                            events.RescheduleEvent(EVENT_WING_BUFFET, urand(15, 30) * IN_MILLISECONDS);
                            break;
                        }
                        // Phase 2
                        case EVENT_SPAWN_WHELP:
                        {
                            DoSummon(NPC_WHELP, SpawnLocations[0]);
                            DoSummon(NPC_WHELP, SpawnLocations[1]);
                            if (SummonWhelpCount >= 40)
                            {
                                SummonWhelpCount = 0;
                                events.RescheduleEvent(EVENT_SPAWN_WHELP, 90 * IN_MILLISECONDS);
                            }
                            else
                                events.RescheduleEvent(EVENT_SPAWN_WHELP, IN_MILLISECONDS/2);
                            break;
                        }
                        case EVENT_SPAWN_LAIR_GUARD:
                        {
                            DoSummon(NPC_LAIRGUARD, SpawnLocations[2]);
                            events.RescheduleEvent(EVENT_SPAWN_LAIR_GUARD, 30 * IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_DEEP_BREATH:
                        {
                            if (!IsMoving && PointDataId != -1)
                            {
                                Talk(EMOTE_BREATH);
                                me->InterruptNonMeleeSpells(true);
                                float angle = me->GetAngle(&MiddleRoomLocation);
                                me->SetFacingTo(angle);
                                DoCastSelf(MoveData[PointDataId].SpellId);
                                events.RescheduleEvent(EVENT_DEEP_BREATH, 70 * IN_MILLISECONDS);
                            }
                            else
                                events.RescheduleEvent(EVENT_DEEP_BREATH, IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_FIREBALL:
                        {
                            if (!IsMoving && PointDataId != -1 && me->GetCurrentSpellCastTime(MoveData[PointDataId].SpellId) == 0)
                            {
                                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                    DoCast(target, SPELL_FIREBALL);
                                events.RescheduleEvent(EVENT_FIREBALL, 2 * IN_MILLISECONDS); 
                            }
                            else
                                events.RescheduleEvent(EVENT_FIREBALL, IN_MILLISECONDS); 
                            break;
                        }
    
                            // Phase 3
                        case EVENT_BELLOWING_ROAR:
                        {
                            DoCastVictim(SPELL_BELLOWING_ROAR);
                            // Eruption
                            Trinity::GameObjectInRangeCheck check(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 15);
                            GameObject* floor = me->VisitSingleNearbyObject<GameObject>(30, check);
                            if (instance && floor)
                                instance->SetGuidData(DATA_FLOOR_ERUPTION, floor->GetGUID());
                            events.RescheduleEvent(EVENT_BELLOWING_ROAR, 15 * IN_MILLISECONDS);
                            break;
                        }
                        case EVENT_SPAWN_WHELP_PHASE_3:
                        {
                            uint32 spawnCount = urand(1,2);
                            for (uint32 i = 0; i < spawnCount; ++i)
                                DoSummon(NPC_WHELP, SpawnLocations[0]);
                            
                            spawnCount = urand(1,2);
                            for (uint32 i = 0; i < spawnCount; ++i)
                                DoSummon(NPC_WHELP, SpawnLocations[1]);
                            events.RescheduleEvent(EVENT_SPAWN_WHELP_PHASE_3, 30 * IN_MILLISECONDS);
                            break;
                        }
                    }
                }
                if (Phase != PHASE_AIR)
                    DoMeleeAttackIfReady();
    
                if (MovementTimer <= diff)
                {
                    if (!IsMoving && Phase == PHASE_AIR)
                    {
                        SetNextRandomPoint();
                        PointDataId = GetMoveData();
    
                        if (PointDataId == -1)
                            return;
    
                        me->GetMotionMaster()->MovePoint(MoveData[PointDataId].LocId, MoveData[PointDataId].fX, MoveData[PointDataId].fY, MoveData[PointDataId].fZ);
                        IsMoving = true;
                        MovementTimer = 25 * IN_MILLISECONDS;
                    }
                }
                else
                    MovementTimer -= diff;
            }
    
        private:
            uint32 Phase;
            uint8 SummonWhelpCount;
            uint8 CurrentMovePoint;
            uint32 MovementTimer;
            int8 PointDataId;
            bool IsMoving;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_onyxiaAI (creature);
        }
};

void AddSC_boss_onyxia()
{
    new boss_onyxia();
}
