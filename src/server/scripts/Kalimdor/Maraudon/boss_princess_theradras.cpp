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
SDName: Boss_Princess_Theradras
SD%Complete: 100
SDComment:
SDCategory: Maraudon
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedCreature.h"

enum Spells
{
    SPELL_DUSTFIELD             = 21909,
    SPELL_BOULDER               = 21832,
    SPELL_THRASH                = 3391,
    SPELL_REPULSIVEGAZE         = 21869
};

enum TheradrasMisc
{
    EVENT_DUSTFIELD             = 1,
    EVENT_BOULDER               = 2,
    EVENT_TRASH                 = 3,
    EVENT_REPULSIVE_GAZE        = 4,
    NPC_ZAETAR                  = 12238
};

class boss_princess_theradras : public CreatureScript
{
public:
    boss_princess_theradras() : CreatureScript("boss_princess_theradras") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_ptheradrasAI(creature);
    }

    struct boss_ptheradrasAI : public ScriptedAI
    {
        boss_ptheradrasAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_DUSTFIELD, 8*IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_BOULDER, 2*IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_TRASH, 5*IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_REPULSIVE_GAZE, 23*IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) override { }

        void JustDied(Unit* /*killer*/) override
        {
            me->SummonCreature(NPC_ZAETAR, 28.067f, 61.875f, -123.405f, 4.67f, TEMPSUMMON_TIMED_DESPAWN, 600000);
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
                    case EVENT_DUSTFIELD:
                        DoCastSelf(SPELL_DUSTFIELD);
                        _events.ScheduleEvent(EVENT_DUSTFIELD, 14*IN_MILLISECONDS);
                        break;
                    case EVENT_BOULDER:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(target, SPELL_BOULDER);
                        _events.ScheduleEvent(EVENT_BOULDER, 10*IN_MILLISECONDS);
                        break;
                    case EVENT_TRASH:
                        DoCastSelf(SPELL_THRASH);
                        _events.ScheduleEvent(EVENT_TRASH, 18*IN_MILLISECONDS);
                        break;
                    case EVENT_REPULSIVE_GAZE:
                        DoCastVictim(SPELL_REPULSIVEGAZE);
                        _events.ScheduleEvent(EVENT_REPULSIVE_GAZE, 20*IN_MILLISECONDS);
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
};

void AddSC_boss_ptheradras()
{
    new boss_princess_theradras();
}
