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

/*
 * Comment: Complete - BUT THE TRIGGER NEEDS DATA WHETHER THE PRISON OF TALDARAM IS OFFLINE !
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ahnkahet.h"

enum Yells
{
    TEXT_AGGRO                                  = 0,
    TEXT_CALL_SACRIFICE                         = 1,
    TEXT_SACRIFICE                              = 2,
    TEXT_SLAY                                   = 3,
    TEXT_DEATH                                  = 4,
    TEXT_PREACHING                              = 5
};

enum Spells
{
    SPELL_SPHERE_VISUAL                         = 56075,
    SPELL_GIFT_OF_THE_HERALD                    = 56219,
    SPELL_CYCLONE_STRIKE                        = 56855, // Self
    SPELL_LIGHTNING_BOLT                        = 56891, // 40Y
    SPELL_THUNDERSHOCK                          = 56926, // 30Y
};

enum Events
{
    EVENT_SACRIFICE             = 1,
    EVENT_CYCLONE_STRIKE,
    EVENT_LIGHTNING_BOLT,
    EVENT_THUNDERSHOCK
};

enum Actions
{
    ACTION_START_ENCOUNTER      = 1,
    ACTION_SACRIFICE,
    ACTION_VOLUNTEER_KILLED,
    ACTION_CALL_SACRIFICE               // On Volunteer
};

enum Phases
{
    PHASE_GROUND                = 1,
    PHASE_SACRIFICE
};

enum DataJegoda
{
    DATA_VOLUNTEER_WORK         = 1
};

const Position JedogaPosition[2] =
{
    {372.330994f, -705.278015f, -0.624178f,  5.427970f},    // Flying
    {372.330994f, -705.278015f, -16.179716f, 5.427970f}     // Ground
};

class boss_jedoga_shadowseeker : public CreatureScript
{
public:
    boss_jedoga_shadowseeker() : CreatureScript("boss_jedoga_shadowseeker") { }

    struct boss_jedoga_shadowseekerAI : public ScriptedAI
    {
        boss_jedoga_shadowseekerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            introDone = false;
        }

        InstanceScript* instance;

        void Reset()
        {
            phase = PHASE_SACRIFICE;
            volunteerWork = true;

            if (instance)
            {
                instance->SetData(DATA_JEDOGA_SHADOWSEEKER_EVENT, NOT_STARTED);
                instance->SetData(DATA_JEDOGA_RESET_INITIATES, 0);
                instance->SetData(DATA_JEDOGA_RESET_VOLUNTEERS, 0);
            }

            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveAllAuras();
            me->LoadCreaturesAddon();
        }

        void EnterCombat(Unit* who)
        {
            Talk(TEXT_AGGRO);
            instance->SetData(DATA_JEDOGA_SHADOWSEEKER_EVENT, IN_PROGRESS);

            events.Reset();
            events.RescheduleEvent(EVENT_SACRIFICE, urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS));
            events.RescheduleEvent(EVENT_CYCLONE_STRIKE, 3*IN_MILLISECONDS);
            events.RescheduleEvent(EVENT_LIGHTNING_BOLT, 7*IN_MILLISECONDS);
            events.RescheduleEvent(EVENT_THUNDERSHOCK, 12*IN_MILLISECONDS);
        }

        void KilledUnit(Unit* Victim)
        {
            if (!Victim || Victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(TEXT_SLAY);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(TEXT_DEATH);
            if (instance)
            {
                instance->SetData(DATA_JEDOGA_SHADOWSEEKER_EVENT, DONE);
                instance->SetData(DATA_JEDOGA_RESET_VOLUNTEERS, 0);
            }
        }

        void DoAction(int32 action)
        {
            switch (action)
            {
                case ACTION_START_ENCOUNTER:
                    if (instance)
                    {
                        instance->SetData(DATA_JEDOGA_SPAWN_VOLUNTEERS, 0);
                        MoveDown();
                        if (Player* target = me->FindNearestPlayer(50.0f))
                            AttackStart(target);
                    }
                    break;
                case ACTION_SACRIFICE:
                    Sacrifice();
                    MoveDown();
                    break;
                case ACTION_VOLUNTEER_KILLED:
                    volunteerWork = false;
                    MoveDown();
                    break;
            }
        }

        uint32 GetData(uint32 type) const
        {
            if (type == DATA_VOLUNTEER_WORK)
                return volunteerWork? 1: 0;

            return 0;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!introDone && who->GetTypeId() == TYPEID_PLAYER && me->GetDistance(who) < 100.0f)
            {
                Talk(TEXT_PREACHING);
                introDone = true;
            }
        }

        void MoveDown()
        {
            if (!instance)
                return;
            
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveAurasDueToSpell(SPELL_SPHERE_VISUAL);

            me->GetMotionMaster()->MovePoint(1, JedogaPosition[1]);
            
            phase = PHASE_GROUND;
        }

        void MoveUp()
        {
            if (!instance)
                return;

            me->AttackStop();
            me->InterruptNonMeleeSpells(false);
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->RemoveAllAuras();
            me->LoadCreaturesAddon();

            me->GetMotionMaster()->MovePoint(0, JedogaPosition[0]);

            phase = PHASE_SACRIFICE;
            if (instance->GetData(DATA_JEDOGA_SHADOWSEEKER_EVENT) == IN_PROGRESS)
                CallSacrifice();
        }

        void CallSacrifice()
        {
            if (!instance)
                return;

            ObjectGuid volunteerGUID = ObjectGuid(instance->GetGuidData(DATA_JEDOGA_VOLUNTEER));

            if (Creature* volunteer = ObjectAccessor::GetCreature(*me, volunteerGUID))
            {
                Talk(TEXT_CALL_SACRIFICE);
                volunteer->AI()->DoAction(ACTION_CALL_SACRIFICE);
            }
            else
                MoveDown();
        }

        void Sacrifice()
        {
            Talk(TEXT_SACRIFICE);
            DoCastSelf(SPELL_GIFT_OF_THE_HERALD, false);
            MoveDown();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (phase == PHASE_GROUND)
            {
                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SACRIFICE:
                            MoveUp();
                            break;
                        case EVENT_CYCLONE_STRIKE:
                            DoCastSelf(SPELL_CYCLONE_STRIKE, false);
                            events.RescheduleEvent(EVENT_CYCLONE_STRIKE, urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS));
                            break;
                        case EVENT_LIGHTNING_BOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                                DoCast(target, SPELL_LIGHTNING_BOLT, false);
                            events.RescheduleEvent(EVENT_LIGHTNING_BOLT, urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS));
                            break;
                        case EVENT_THUNDERSHOCK:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                                DoCast(target, SPELL_THUNDERSHOCK, false);
                            events.RescheduleEvent(EVENT_THUNDERSHOCK, urand(15*IN_MILLISECONDS, 30*IN_MILLISECONDS));
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        }

        private:
            EventMap events;
            bool introDone;
            bool volunteerWork;
            Phases phase;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_jedoga_shadowseekerAI(creature);
    }
};

class npc_jedoga_initiate : public CreatureScript
{
public:
    npc_jedoga_initiate() : CreatureScript("npc_jedoga_initiate") { }

    struct npc_jedoga_initiateAI : public ScriptedAI
    {
        npc_jedoga_initiateAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void JustDied(Unit* killer)
        {
            if (instance && instance->GetData(DATA_JEDOGA_ALL_INITIATES_DEAD))
                if (Creature* jedoga = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_JEDOGA_SHADOWSEEKER))))
                    jedoga->AI()->DoAction(ACTION_START_ENCOUNTER);
        }

        private:
            InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jedoga_initiateAI(creature);
    }
};

enum VolunteerYells
{
    SAY_CHOSEN                                  = 0,
};

class npc_jedoga_volunteer : public CreatureScript
{
public:
    npc_jedoga_volunteer() : CreatureScript("npc_jedoga_volunteer") { }

    struct npc_jedoga_volunteerAI : public ScriptedAI
    {
        npc_jedoga_volunteerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
        
        void Reset()
        {
            if (!instance)
                return;

            KillTimer = 2*IN_MILLISECONDS;
            getKilled = false;

            DoCastSelf(SPELL_SPHERE_VISUAL, false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        }

        void EnterCombat(Unit* /*who*/) { }
        void AttackStart(Unit* /*victim*/) { }
        void MoveInLineOfSight(Unit* /*who*/) { }

        void JustDied(Unit* killer)
        {
            if (!killer || !instance)
                return;

            if (killer->GetTypeId() == TYPEID_PLAYER)
                if (Creature* jedoga = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_JEDOGA_SHADOWSEEKER))))
                    jedoga->AI()->DoAction(ACTION_VOLUNTEER_KILLED);
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_CALL_SACRIFICE)
            {
                me->RemoveAurasDueToSpell(SPELL_SPHERE_VISUAL);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);

                float distance = me->GetDistance(JedogaPosition[1]);

                if (distance < 9.0f)
                    me->SetSpeedRate(MOVE_WALK, 0.25f);
                else if (distance < 15.0f)
                    me->SetSpeedRate(MOVE_WALK, 0.75f);
                else if (distance < 20.0f)
                    me->SetSpeedRate(MOVE_WALK, 1.0f);

                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MovePoint(0, JedogaPosition[1]);
                Talk(SAY_CHOSEN);
            }
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (pointId == 0)
            {
                getKilled = true;
                KillTimer = 2*IN_MILLISECONDS;
                me->SetStandState(UNIT_STAND_STATE_KNEEL);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!instance)
                return;

            if (getKilled && KillTimer <= diff)
            {
                if (Creature* jedoga = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_JEDOGA_SHADOWSEEKER))))
                {
                    jedoga->AI()->DoAction(ACTION_SACRIFICE);
                    me->KillSelf();
                    getKilled = false;
                }
            } else KillTimer -= diff;
        }

        private:
            InstanceScript* instance;
            uint32 KillTimer;
            bool getKilled;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jedoga_volunteerAI(creature);
    }
};

class achievement_volunteer_work : public AchievementCriteriaScript
{
    public:
        achievement_volunteer_work() : AchievementCriteriaScript("achievement_volunteer_work") { }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* jedoga = target->ToCreature())
                if (jedoga->AI()->GetData(DATA_VOLUNTEER_WORK))
                    return true;

            return false;
        }
};

void AddSC_boss_jedoga_shadowseeker()
{
    new boss_jedoga_shadowseeker();
    new npc_jedoga_initiate();
    new npc_jedoga_volunteer();
    new achievement_volunteer_work();
}
