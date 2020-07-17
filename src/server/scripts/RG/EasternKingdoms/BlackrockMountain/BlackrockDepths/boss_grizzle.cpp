/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

enum Grizzle
{
    SPELL_GROUNDTREMOR      = 6524,
    SPELL_FRENZY            = 28371,
    EMOTE_FRENZY_KILL       = 0
};

enum Events
{
    EVENT_GROUNDTREMOR      = 1,
    EVENT_FRENZY            = 2
};

enum Phases
{
    PHASE_ONE               = 1,
    PHASE_TWO               = 2
};

class boss_grizzle : public CreatureScript
{
    public:
        boss_grizzle() : CreatureScript("boss_grizzle") { }

        struct boss_grizzleAI : public ScriptedAI
        {
            boss_grizzleAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/) override 
            {
                _events.SetPhase(PHASE_ONE);
                _events.ScheduleEvent(EVENT_GROUNDTREMOR, 12 * IN_MILLISECONDS);
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage) override
            {
                if (me->HealthBelowPctDamaged(50, damage) && _events.IsInPhase(PHASE_ONE))
                {
                    _events.SetPhase(PHASE_TWO);
                    _events.ScheduleEvent(EVENT_FRENZY, 0, 0, PHASE_TWO);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_GROUNDTREMOR:
                            DoCastVictim(SPELL_GROUNDTREMOR);
                            _events.ScheduleEvent(EVENT_GROUNDTREMOR, 8 * IN_MILLISECONDS);
                            break;
                        case EVENT_FRENZY:
                           DoCastSelf(SPELL_FRENZY);
                           Talk(EMOTE_FRENZY_KILL);
                            _events.ScheduleEvent(EVENT_FRENZY, 15 * IN_MILLISECONDS, 0, PHASE_TWO);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_grizzleAI(creature);
        }
};

void AddSC_boss_grizzle()
{
    new boss_grizzle();
}
