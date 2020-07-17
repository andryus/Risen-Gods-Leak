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
    SPELL_CAUTERIZING_FLAMES                      = 59466, //Only in heroic
    SPELL_FIREBOLT                                = 54235,
    H_SPELL_FIREBOLT                              = 59468,
    SPELL_FLAME_BREATH                            = 54282,
    H_SPELL_FLAME_BREATH                          = 59469,
    SPELL_LAVA_BURN                               = 54249,
    H_SPELL_LAVA_BURN                             = 59594
};

class boss_lavanthor : public CreatureScript
{
public:
    boss_lavanthor() : CreatureScript("boss_lavanthor") { }

    struct boss_lavanthorAI : public ScriptedAI
    {
        boss_lavanthorAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        uint32 FireboltTimer;
        uint32 FlameBreathTimer;
        uint32 LavaBurnTimer;
        uint32 CauterizingFlamesTimer;

        InstanceScript* instance;

        void Reset()
        {
            FireboltTimer = 1000;
            FlameBreathTimer = 5000;
            LavaBurnTimer = 10000;
            CauterizingFlamesTimer = 3000;
            if (instance)
            {
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                    instance->SetData(DATA_1ST_BOSS_EVENT, NOT_STARTED);
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                    instance->SetData(DATA_2ND_BOSS_EVENT, NOT_STARTED);
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            if (instance)
            {
            if (GameObject* Door = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(DATA_LAVANTHOR_CELL))))
                    if (Door->GetGoState() == GO_STATE_READY)
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

        void MoveInLineOfSight(Unit* /*who*/) {}

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            if (FireboltTimer <= diff)
            {
                DoCastVictim(SPELL_FIREBOLT);
                FireboltTimer = urand(5000, 13000);
            } else FireboltTimer -= diff;

            if (FlameBreathTimer <= diff)
            {
                DoCastVictim(SPELL_FLAME_BREATH);
                FlameBreathTimer = urand(10000, 15000);
            } else FlameBreathTimer -= diff;

            if (LavaBurnTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_LAVA_BURN);
                LavaBurnTimer = urand(15000, 23000);
            } else LavaBurnTimer -= diff;

            if (IsHeroic())
            {
                if (CauterizingFlamesTimer <= diff)
                {
                    DoCastVictim(SPELL_CAUTERIZING_FLAMES);
                    CauterizingFlamesTimer = urand(10000, 16000);
                } else CauterizingFlamesTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
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
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_lavanthorAI (creature);
    }
};

void AddSC_boss_lavanthor()
{
    new boss_lavanthor();
}
