/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "blackrock_depths.h"

enum Spells
{
    SPELL_FIERYBURST    = 13900,
    SPELL_WARSTOMP      = 24375,
    SPELL_GOUT_OF_FLAME = 15529
};

enum Events
{
    EVENT_FIERY_BURST   = 1,
    EVENT_WARSTOMP      = 2,
    EVENT_GOUT_OF_FLAME = 3
};

enum Phases
{
    PHASE_ONE = 1,
    PHASE_TWO = 2
};

class boss_magmus : public CreatureScript
{
    public:
        boss_magmus() : CreatureScript("boss_magmus") { }

        struct boss_magmusAI : public ScriptedAI
        {
            boss_magmusAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/) override
            {
                _events.SetPhase(PHASE_ONE);
                _events.ScheduleEvent(EVENT_FIERY_BURST,              5 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_GOUT_OF_FLAME,            1 * IN_MILLISECONDS);               
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage) override
            {
                if (me->HealthBelowPctDamaged(50, damage) && _events.IsInPhase(PHASE_ONE))
                {
                    _events.SetPhase(PHASE_TWO);
                    _events.ScheduleEvent(EVENT_WARSTOMP, 0, 0, PHASE_TWO);
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
                        case EVENT_FIERY_BURST:
                            DoCastVictim(SPELL_FIERYBURST);
                            _events.ScheduleEvent(EVENT_FIERY_BURST, 6 * IN_MILLISECONDS);
                            break;
                        case EVENT_WARSTOMP:
                            DoCastVictim(SPELL_WARSTOMP);
                            _events.ScheduleEvent(EVENT_WARSTOMP, 8 * IN_MILLISECONDS, 0, PHASE_TWO);
                            break;
                        case EVENT_GOUT_OF_FLAME:
                        {
                            std::list<Creature*> HelperList;
                            me->GetCreatureListWithEntryInGrid(HelperList, NPC_IRONHAND_GUARDIAN, 400.0f);
                            if (!HelperList.empty())
                                for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                                    (*itr)->CastSpell(me, SPELL_GOUT_OF_FLAME);
                            _events.ScheduleEvent(EVENT_GOUT_OF_FLAME, urand(4, 16) * IN_MILLISECONDS);
                            break;
                        }
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* killer) override
            {
                if (InstanceScript* instance = killer->GetInstanceScript())
                    instance->HandleGameObject(instance->GetGuidData(DATA_THRONE_DOOR), true);
            }
        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_magmusAI(creature);
        }
};

void AddSC_boss_magmus()
{
    new boss_magmus();
}
