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
#include "blackrock_spire.h"

enum Spells
{
    SPELL_WHIRLWIND                 = 15589,
    SPELL_WHIRLWIND2                = 13736,
    SPELL_CLEAVE                    = 15284,
    SPELL_FRENZY                    = 8269,
    SPELL_MORTAL_STRIKE             = 15708,
};

enum NPCs
{
    GYTH                            = 10339,
};

enum Events
{
    EVENT_WHIRLWIND                 = 1,
    EVENT_WHIRLWIND2                = 2,
    EVENT_CLEAVE                    = 3,
    EVENT_FRENZY                    = 4,
    EVENT_MORTAL_STRIKE             = 5,
};

class boss_rend_blackhand : public CreatureScript
{
public:
    boss_rend_blackhand() : CreatureScript("boss_rend_blackhand") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return GetInstanceAI<boss_rend_blackhandAI>(creature);
    }

    struct boss_rend_blackhandAI : public BossAI
    {
        boss_rend_blackhandAI(Creature* creature) : BossAI(creature, DATA_WARCHIEF_REND_BLACKHAND) {}

        void Reset()
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_NPC);
            _Reset();
        }

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_WHIRLWIND, 15*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_WHIRLWIND2, 30*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_CLEAVE, 45*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_FRENZY, 0.1*IN_MILLISECONDS);
            events.ScheduleEvent(EVENT_MORTAL_STRIKE, 60*IN_MILLISECONDS);
        }

        void JustDied(Unit* /*killer*/)
        {
            _JustDied();
            if (GameObject* gate = me->GetMap()->GetGameObject(instance->GetData64(GO_GYTH_ENTRY_DOOR)))
                gate->SetGoState(GO_STATE_ACTIVE);
            if (instance)
            {
                instance->SetBossState(DATA_WARCHIEF_REND_BLACKHAND, DONE);
            }
        }

        void UpdateAI(uint32 diff)
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
                    case EVENT_WHIRLWIND:
                        DoCast(me->GetVictim(), SPELL_WHIRLWIND);
                        events.ScheduleEvent(EVENT_WHIRLWIND, 15*IN_MILLISECONDS);
                        break;
                    case EVENT_WHIRLWIND2:
                        DoCast(me->GetVictim(), SPELL_WHIRLWIND2);
                        events.ScheduleEvent(EVENT_WHIRLWIND2, 15*IN_MILLISECONDS);
                        break;
                    case EVENT_CLEAVE:
                        DoCast(me->GetVictim(), SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, 16*IN_MILLISECONDS);
                        break;
                    case EVENT_FRENZY:
                        DoCast(me->GetVictim(), SPELL_FRENZY);
                        events.ScheduleEvent(SPELL_FRENZY, 120*IN_MILLISECONDS); //Only 2 Min because aura
                        break;
                    case EVENT_MORTAL_STRIKE:
                        DoCast(me->GetVictim(), SPELL_MORTAL_STRIKE);
                        events.ScheduleEvent(SPELL_MORTAL_STRIKE, 16*IN_MILLISECONDS);
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_rend_blackhand()
{
    new boss_rend_blackhand();
}
