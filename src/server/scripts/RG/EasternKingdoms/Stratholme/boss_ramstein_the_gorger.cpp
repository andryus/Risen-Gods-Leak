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
#include "stratholme.h"

enum Spells
{
    SPELL_TRAMPLE           = 5568,
    SPELL_KNOCKOUT          = 17307
};

enum Events
{
    EVENT_TRAMPLE           = 1,
    EVENT_KNOCKOUT          = 2
};

enum CreatureId
{
    NPC_MINDLESS_UNDEAD     = 11030
};

class boss_ramstein_the_gorger : public CreatureScript
{
public:
    boss_ramstein_the_gorger() : CreatureScript("boss_ramstein_the_gorger") { }

    struct boss_ramstein_the_gorgerAI : public ScriptedAI
    {
        boss_ramstein_the_gorgerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }
        
        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_TRAMPLE,   3 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_KNOCKOUT, 12 * IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) override
        {
        }

        void JustDied(Unit* /*killer*/) override
        {
            for (uint8 i = 0; i < 30; ++i)
            {
                if (Creature* mob = me->SummonCreature(NPC_MINDLESS_UNDEAD, 3969.35f+irand(-10, 10), -3391.87f+irand(-10, 10), 119.11f, 5.91f, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 1800000))
                    mob->AI()->AttackStart(me->SelectNearestTarget(100.0f));
            }

            instance->SetData(TYPE_RAMSTEIN, DONE);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_TRAMPLE:
                        DoCastSelf(SPELL_TRAMPLE);
                        _events.ScheduleEvent(EVENT_TRAMPLE, 7 * IN_MILLISECONDS);
                        break;
                    case EVENT_KNOCKOUT:
                        DoCastVictim(SPELL_KNOCKOUT);
                        _events.ScheduleEvent(EVENT_KNOCKOUT, 10 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }
        
    private:
        EventMap _events;
        InstanceScript* instance;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_ramstein_the_gorgerAI>(creature);
    }
};

void AddSC_boss_ramstein_the_gorger()
{
    new boss_ramstein_the_gorger();
}
