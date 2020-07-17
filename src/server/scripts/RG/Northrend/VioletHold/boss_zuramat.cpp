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
#include "violet_hold.h"

enum Spells
{
    SPELL_SHROUD_OF_DARKNESS                    = 54524,
    H_SPELL_SHROUD_OF_DARKNESS                  = 59745,
    SPELL_SUMMON_VOID_SENTRY                    = 54369,
    SPELL_VOID_SHIFT                            = 54361,
    H_SPELL_VOID_SHIFT                          = 59743,

    SPELL_ZURAMAT_ADD_2                         = 54342,
    H_SPELL_ZURAMAT_ADD_2                       = 59747,
    SPELL_SUMMON_VOID_SENTRY_BALL               = 58650
};

enum ZuramatEvents
{
    EVENT_SHROUD_OF_DARKNESS                    = 0,
    EVENT_SUMMON_VOID_SENTRY                    = 1,
    EVENT_VOID_SHIFT                            = 2
};

enum ZuramatCreatures
{
    CREATURE_VOID_SENTRY                        = 29364,
    CREATURE_VOID_SENTRY_VEHICLE                = 29365
};

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SLAY                                    = 1,
    SAY_DEATH                                   = 2,
    SAY_SPAWN                                   = 3,
    SAY_SHIELD                                  = 4,
    SAY_WHISPER                                 = 5
};

enum Misc
{
    ACTION_DESPAWN_VOID_SENTRY_BALL             = 1,
    DATA_VOID_DANCE                             = 2153
};

class boss_zuramat : public CreatureScript
{
public:
    boss_zuramat() : CreatureScript("boss_zuramat") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_zuramatAI (creature);
    }

    struct boss_zuramatAI : public ScriptedAI
    {
        boss_zuramatAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
       
        void Reset()
        {
            if (instance)
            {
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                    instance->SetData(DATA_1ST_BOSS_EVENT, NOT_STARTED);
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                    instance->SetData(DATA_2ND_BOSS_EVENT, NOT_STARTED);
            }

            _events.Reset();
            _events.ScheduleEvent(EVENT_SHROUD_OF_DARKNESS, 22 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_VOID_SHIFT, 15 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_SUMMON_VOID_SENTRY, 12 * IN_MILLISECONDS);
            voidDance = true;
            std::list<Creature*> AddList;
            me->GetCreatureListWithEntryInGrid(AddList, CREATURE_VOID_SENTRY, 300.0f);
            me->GetCreatureListWithEntryInGrid(AddList, CREATURE_VOID_SENTRY_VEHICLE, 300.0f);
            if (!AddList.empty())
                for (std::list<Creature*>::iterator itr = AddList.begin(); itr != AddList.end(); itr++)
                    (*itr)->DespawnOrUnsummon();
        }

        void AttackStart(Unit* who)
        {
            if (me->IsImmuneToPC() || me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
            if (instance)
            {
                if (GameObject* pDoor = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(DATA_ZURAMAT_CELL))))
                    if (pDoor->GetGoState() == GO_STATE_READY)
                    {
                        EnterEvadeMode();
                        return;
                    }
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                    instance->SetData(DATA_1ST_BOSS_EVENT, IN_PROGRESS);
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                    instance->SetData(DATA_2ND_BOSS_EVENT, IN_PROGRESS);
            }
        }

        void MoveInLineOfSight(Unit* /*who*/) {}

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHROUD_OF_DARKNESS:
                        DoCastVictim(SPELL_SHROUD_OF_DARKNESS);
                        _events.ScheduleEvent(EVENT_SHROUD_OF_DARKNESS, 22 * IN_MILLISECONDS);
                        break;
                    case EVENT_SUMMON_VOID_SENTRY:
                        DoCastVictim(SPELL_SUMMON_VOID_SENTRY, false);
                        _events.ScheduleEvent(EVENT_SUMMON_VOID_SENTRY, 20 * IN_MILLISECONDS);
                        break;
                    case EVENT_VOID_SHIFT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(target, SPELL_VOID_SHIFT);
                        _events.ScheduleEvent(EVENT_VOID_SHIFT, 20 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }
            DoMeleeAttackIfReady();
        }

        void SummonedCreatureDies(Creature* summoned, Unit* /*who*/)
        {
            if (summoned->GetEntry() == CREATURE_VOID_SENTRY)
                voidDance = false;
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            if (summon->GetEntry() == CREATURE_VOID_SENTRY)
                summon->AI()->DoAction(ACTION_DESPAWN_VOID_SENTRY_BALL);
            ScriptedAI::SummonedCreatureDespawn(summon);
        }

        uint32 GetData(uint32 type) const
        {
            if (type == DATA_VOID_DANCE)
                return voidDance ? 1 : 0;

            return 0;
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);
            std::list<Creature*> AddList;
            me->GetCreatureListWithEntryInGrid(AddList, CREATURE_VOID_SENTRY, 300.0f);
            me->GetCreatureListWithEntryInGrid(AddList, CREATURE_VOID_SENTRY_VEHICLE, 300.0f);
            if (!AddList.empty())
                for (std::list<Creature*>::iterator itr = AddList.begin(); itr != AddList.end(); itr++)
                    (*itr)->DespawnOrUnsummon();

            if (instance)
            {
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                {
                    instance->SetData(DATA_1ST_BOSS_EVENT, DONE);
                    instance->SetData(DATA_WAVE_COUNT, 7);
                }
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                {
                    instance->SetData(DATA_2ND_BOSS_EVENT, DONE);
                    instance->SetData(DATA_WAVE_COUNT, 13);
                }
            }
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(SAY_SLAY);
        }
        private:
            InstanceScript* instance;
            EventMap _events;
            bool voidDance;
    };

};

class npc_void_sentry : public CreatureScript
{
    public:
        npc_void_sentry() : CreatureScript("npc_void_sentry") { }

        struct npc_void_sentryAI : public ScriptedAI
        {
            npc_void_sentryAI(Creature* creature) : ScriptedAI(creature), _summons(creature)
            {
                me->SetReactState(REACT_PASSIVE);
            }

            void IsSummonedBy(Unit* /*summoner*/) override
            {
                DoCastSelf(SPELL_SUMMON_VOID_SENTRY_BALL, true);
            }

            void JustSummoned(Creature* summon) override
            {
                _summons.Summon(summon);
                summon->SetReactState(REACT_PASSIVE);
            }

            void SummonedCreatureDespawn(Creature* summon) override
            {
                _summons.Despawn(summon);
            }

            void DoAction(int32 actionId) override
            {
                if (actionId == ACTION_DESPAWN_VOID_SENTRY_BALL)
                    _summons.DespawnAll();
            }

            void JustDied(Unit* /*killer*/) override
            {
                DoAction(ACTION_DESPAWN_VOID_SENTRY_BALL);
            }

        private:
            SummonList _summons;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_void_sentryAI(creature);
        }
};

class achievement_void_dance : public AchievementCriteriaScript
{
    public:
        achievement_void_dance() : AchievementCriteriaScript("achievement_void_dance")
        {
        }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Zuramat = target->ToCreature())
                if (Zuramat->AI()->GetData(DATA_VOID_DANCE))
                    return true;

            return false;
        }
};

void AddSC_boss_zuramat()
{
    new boss_zuramat();
    new npc_void_sentry();
    new achievement_void_dance();
}
