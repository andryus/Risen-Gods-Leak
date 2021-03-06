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

/* ScriptData
SDName: Boss_Wushoolay
SD%Complete: 100
SDComment:
SDCategory: Zul'Gurub
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "zulgurub.h"

enum Spells
{
    SPELL_LIGHTNINGCLOUD        = 24683,
    SPELL_LIGHTNINGWAVE         = 24819,
    SPELL_FORKED_LIGHTNING      = 24682
};

enum Events
{
    EVENT_LIGHTNINGCLOUD        = 1,
    EVENT_LIGHTNINGWAVE         = 2,
    EVENT_FORKED_LIGHTNING      = 3
};

class boss_wushoolay : public CreatureScript
{
    public:
        boss_wushoolay() : CreatureScript("boss_wushoolay") { }

        struct boss_wushoolayAI : public BossAI
        {
            boss_wushoolayAI(Creature* creature) : BossAI(creature, DATA_EDGE_OF_MADNESS) { }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_LIGHTNINGCLOUD,   urand(5, 10)*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_LIGHTNINGWAVE,    urand(8, 16)*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FORKED_LIGHTNING, urand(7, 14)*IN_MILLISECONDS);
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
                        case EVENT_LIGHTNINGCLOUD:
                            DoCastVictim(SPELL_LIGHTNINGCLOUD, true);
                            events.ScheduleEvent(EVENT_LIGHTNINGCLOUD, urand(15, 20)*IN_MILLISECONDS);
                            break;
                        case EVENT_LIGHTNINGWAVE:
                            DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true), SPELL_LIGHTNINGWAVE);
                            events.ScheduleEvent(EVENT_LIGHTNINGWAVE, urand(12, 16)*IN_MILLISECONDS);
                            break;
                        case EVENT_FORKED_LIGHTNING:
                            DoCastVictim(SPELL_FORKED_LIGHTNING, true);
                            events.ScheduleEvent(EVENT_FORKED_LIGHTNING, urand(7, 14)*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }

                    if (me->HasUnitState(UNIT_STATE_CASTING))
                        return;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_wushoolayAI(creature);
        }
};

void AddSC_boss_wushoolay()
{
    new boss_wushoolay();
}

