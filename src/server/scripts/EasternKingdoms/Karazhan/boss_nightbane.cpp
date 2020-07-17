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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "karazhan.h"

enum Spells
{
    // Phase 1
    SPELL_CLEAVE                = 30131, // Cleave all 7-10s
    SPELL_TAIL_SWEEP            = 25653, // Behind all 10-15s
    SPELL_SMOLDERING_BREATH     = 30210, // All 30s
    SPELL_BELLOWING_ROAR        = 39427, // AoE Fear all 45-60s
    SPELL_CHARRED_EARTH         = 30129, // Flames all 20-50s
    SPELL_DISTRACTING_ASH       = 30130, // Debuff all 15-30s
    // Phase 2
    SPELL_RAIN_OF_BONES         = 37098, // AoE once per Flyphase
    SPELL_SUMMON_SKELETON       = 30170, // Together with Rain of Bones
    SPELL_SMOKING_BLAST         = 37057, // All every 1s after Rain of Bones
    SPELL_SEARING_CINDERS       = 30127, // Triggered by Smoking Blast
    SPELL_FIREBALL_BARRAGE      = 30282  // On Players more than 40m away
};

enum Says
{
    EMOTE_SUMMON                = 0,
    YELL_AGGRO                  = 1,
    YELL_FLY_PHASE              = 2,
    YELL_LAND_PHASE             = 3,
    EMOTE_BREATH                = 4
};

enum Actions
{
    ACTION_START_INTRO          = 1
};

Position const IntroWay[7] =
{
    {-11023.14f, -1799.73f, 162.92f}, // Fly over Mountain, old: -11045.05f, -1798.93f, 163.71f
    {-11159.98f, -1867.89f, 101.11f}, // Above Terrasse
    {-11286.52f, -1802.45f, 92.76f},  // Fly to the village
    {-11254.28f, -1764.34f, 93.45f},  // Hover over the village
    {-11166.84f, -1882.80f, 110.93f}, // Fly Back
    {-11153.31f, -1895.11f, 95.27f},  // Prepare to land
    {-11153.31f, -1895.11f, 91.47f}   // Land
};

Position const FlyPhaseWay[3] =
{
    {-11166.84f, -1882.80f, 110.93f}, // Take off
    {-11153.31f, -1895.11f, 95.27f},  // Prepare to land
    {-11153.31f, -1895.11f, 91.47f}   // Land
};

enum Events
{
    // Phase 1
    EVENT_CLEAVE                = 1,
    EVENT_TAIL_SWEEP,
    EVENT_SMOLDERING_BREATH,
    EVENT_BELLOWING_ROAR,
    EVENT_CHARRED_EARTH,
    EVENT_DISTRACTING_ASH,
    // Phase 2
    EVENT_RAIN_OF_BONES,
    EVENT_SUMMON_SKELETON,
    EVENT_SMOKING_BLAST,
    EVENT_FIREBALL_BARRAGE,
    EVENT_STOP_FLYING,
};

class boss_nightbane : public CreatureScript
{
public:
    boss_nightbane() : CreatureScript("boss_nightbane") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_nightbaneAI>(creature);
    }

    struct boss_nightbaneAI : public ScriptedAI
    {
        boss_nightbaneAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset() override
        {
            NextFlyPhase = 75.0f;
            MovePhase = 0;
            
            Intro = false;
            Flying = false;
            WaypointReached = false;

            me->RemoveUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
            me->SetCanFly(false);
            me->SetDisableGravity(false);
            me->SetWalk(true);
            me->setActive(false);
            me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);

            HandleTerraceDoors(true);
            SetCombatMovement(true);
            me->SetStandState(UNIT_STAND_STATE_SLEEP);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetImmuneToAll(true);
            instance->SetBossState(DATA_NIGHTBANE, NOT_STARTED);

            events.Reset();
        }

        void DoAction(int32 param) override
        {
            if (param == ACTION_START_INTRO && instance && instance->GetBossState(DATA_NIGHTBANE) == NOT_STARTED) // Blackened Urn activated
            {
                Intro = true;
                Flying = false;
                WaypointReached = true;
                me->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
                me->SetCanFly(true);
                me->SetDisableGravity(true);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->setActive(true);
                me->SetWalk(false);
                HandleTerraceDoors(false);

                me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);

                instance->SetBossState(DATA_NIGHTBANE, IN_PROGRESS);
            }
        }

        void EnterCombat(Unit* /*who*/) override
        {
            instance->SetBossState(DATA_NIGHTBANE, IN_PROGRESS);

            HandleTerraceDoors(false);

            me->SetInCombatWithZone();
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
			me->SetImmuneToAll(false);

            Talk(YELL_AGGRO);
        }

        void JustDied(Unit* /*killer*/) override
        {
            instance->SetBossState(DATA_NIGHTBANE, DONE);

            HandleTerraceDoors(true);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) override
        {
            if (Flying && me->HealthBelowPct(5))
                damage = 0;
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (Intro && id >= 6)
            {
                Intro = false;
                EnterCombat(SelectTarget(SELECT_TARGET_MINDISTANCE));
                Land();
                return;
            }

            if (Flying)
            {
                switch (id)
                {
                    case 0:
                        me->AI()->AttackStart(SelectTarget(SELECT_TARGET_MAXTHREAT));
                        break;
                    case 2:
                        Land();
                        break;
                    default:
                        break;
                }
            }

            WaypointReached = true;
        }

        void JustSummoned(Creature* summoned) override
        {
            summoned->AI()->AttackStart(me->GetVictim());
        }

        void HandleTerraceDoors(bool open) 
        {
            if (instance)
            {
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_MASTERS_TERRACE_DOOR_1)), open);
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_MASTERS_TERRACE_DOOR_2)), open);
            }
        }

        void TakeOff() 
        {
            me->InterruptSpell(CURRENT_GENERIC_SPELL);

            events.CancelEvent(EVENT_CLEAVE);
            events.CancelEvent(EVENT_TAIL_SWEEP);
            events.CancelEvent(EVENT_SMOLDERING_BREATH);
            events.CancelEvent(EVENT_BELLOWING_ROAR);
            events.CancelEvent(EVENT_CHARRED_EARTH);
            events.CancelEvent(EVENT_DISTRACTING_ASH);

            Talk(YELL_FLY_PHASE);

            SkeletonCount = 0;
            Flying = true;
            MovePhase = 0;

            me->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetWalk(false);

            SetCombatMovement(false);
            me->GetMotionMaster()->MovePoint(MovePhase, FlyPhaseWay[MovePhase]);
            me->GetThreatManager().resetAllAggro();
            me->AttackStop();

            me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);

            events.ScheduleEvent(EVENT_RAIN_OF_BONES, 15 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SMOKING_BLAST, 30 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FIREBALL_BARRAGE, 25 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_STOP_FLYING, 50 * IN_MILLISECONDS);
        }

        void Land()
        {
            Flying = false;
            NextFlyPhase = floor((me->GetHealthPct() - 1.0f) / 25.0f) * 25.0f;

            me->RemoveUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);
            me->SetCanFly(false);
            me->SetDisableGravity(false);
            me->SetWalk(true);

            SetCombatMovement(true);
            me->GetMotionMaster()->Clear();
            me->GetThreatManager().resetAllAggro();
            me->GetMotionMaster()->MoveChase(me->GetVictim());
            AttackStart(me->GetVictim());

            me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);

            events.ScheduleEvent(EVENT_CLEAVE, urand(7, 10) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_TAIL_SWEEP, urand(10, 15) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_SMOLDERING_BREATH, 30 * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_BELLOWING_ROAR, urand(45, 60) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CHARRED_EARTH, urand(20, 50) * IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_DISTRACTING_ASH, urand(15, 30) * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            if (WaypointReached)
            {
                if (Intro)
                {
                    me->GetMotionMaster()->MovePoint(MovePhase, IntroWay[MovePhase]);
                    ++MovePhase;
                }
                if (Flying)
                {
                    if (MovePhase >= 1)
                    {
                        me->GetMotionMaster()->MovePoint(MovePhase, FlyPhaseWay[MovePhase]);
                        ++MovePhase;
                    }
                }

                WaypointReached = false;
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    // Phase 1 "GROUND FIGHT"
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.RescheduleEvent(EVENT_CLEAVE, urand(7, 10) * IN_MILLISECONDS);
                        break;
                    case EVENT_TAIL_SWEEP:
                        DoCastVictim(SPELL_TAIL_SWEEP);
                        events.RescheduleEvent(EVENT_TAIL_SWEEP, urand(15, 20) * IN_MILLISECONDS);
                        break;
                    case EVENT_SMOLDERING_BREATH:
                        DoCastVictim(SPELL_SMOLDERING_BREATH);
                        events.RescheduleEvent(EVENT_BELLOWING_ROAR, 30 * IN_MILLISECONDS);
                        break;
                    case EVENT_BELLOWING_ROAR:
                        DoCastVictim(SPELL_BELLOWING_ROAR);
                        events.RescheduleEvent(EVENT_BELLOWING_ROAR, urand(45, 60)*IN_MILLISECONDS);
                        break;
                    case EVENT_CHARRED_EARTH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                            DoCast(target, SPELL_CHARRED_EARTH);
                        events.RescheduleEvent(EVENT_CHARRED_EARTH, urand(20, 50) * IN_MILLISECONDS);
                        break;
                    case EVENT_DISTRACTING_ASH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                            DoCast(target, SPELL_DISTRACTING_ASH);
                        events.RescheduleEvent(EVENT_DISTRACTING_ASH, urand(15, 30) * IN_MILLISECONDS);
                        break;

                    // Phase 2 "FLYING FIGHT"
                    case EVENT_RAIN_OF_BONES:
                        Talk(EMOTE_BREATH);
                        RainTargetGUID.Clear();
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                        {
                            RainTargetGUID = target->GetGUID();
                            DoCast(target, SPELL_RAIN_OF_BONES);
                            events.ScheduleEvent(EVENT_SUMMON_SKELETON, 6 * IN_MILLISECONDS);
                        }
                        break;
                    case EVENT_SUMMON_SKELETON:
                        if (Unit* target = ObjectAccessor::GetUnit(*me, RainTargetGUID))
                            DoCast(target, SPELL_SUMMON_SKELETON, true);
                        if (++SkeletonCount < 5)
                            events.RescheduleEvent(EVENT_SUMMON_SKELETON, 2 * IN_MILLISECONDS);
                    case EVENT_SMOKING_BLAST:
                        if (Unit* target = SelectTarget(SELECT_TARGET_MAXTHREAT, 0, 0, true))
                        {
                            DoCast(target, SPELL_SMOKING_BLAST);
                            DoCast(target, SPELL_SEARING_CINDERS);
                        }
                        events.RescheduleEvent(EVENT_SMOKING_BLAST, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_FIREBALL_BARRAGE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_MAXDISTANCE, 0))
                            if(!me->IsWithinDist2d(target->GetPositionX(), target->GetPositionY(), 40))
                                DoCast(target, SPELL_FIREBALL_BARRAGE);
                        events.RescheduleEvent(EVENT_FIREBALL_BARRAGE, 1 * IN_MILLISECONDS);
                        break;
                    case EVENT_STOP_FLYING:
                        events.CancelEvent(EVENT_SMOKING_BLAST);
                        events.CancelEvent(EVENT_FIREBALL_BARRAGE);
                        Talk(YELL_LAND_PHASE);
                        ++MovePhase;
                        me->GetMotionMaster()->MovePoint(MovePhase, FlyPhaseWay[MovePhase]);
                        break;
                }
            }

            // Fly Phase all 25%
            if (!Flying && NextFlyPhase > 0 && me->GetHealthPct() < NextFlyPhase)
                TakeOff();

            if (!Flying)
                DoMeleeAttackIfReady();
        }

    private:
        // general stuff
        InstanceScript* instance;
        EventMap events;

        // fly variables
        uint32 MovePhase;
        float NextFlyPhase;
        bool Intro;
        bool Flying;
        bool WaypointReached;

        // fight variables
        uint32 SkeletonCount;
        ObjectGuid RainTargetGUID;
    };

};

class go_blackened_urn : public GameObjectScript
{
public:
    go_blackened_urn() : GameObjectScript("go_blackened_urn") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (InstanceScript* instance = go->GetInstanceScript())
            if (Creature* nightbane = ObjectAccessor::GetCreature(*go, ObjectGuid(instance->GetGuidData(DATA_NIGHTBANE))))
                nightbane->AI()->DoAction(ACTION_START_INTRO);
        go->SendUpdateToPlayer(player);
        return true;
    }
};

void AddSC_boss_nightbane()
{
    new boss_nightbane();
    new go_blackened_urn();
}
