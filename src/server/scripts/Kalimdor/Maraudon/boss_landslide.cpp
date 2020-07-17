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
SDName: Boss_Landslide
SD%Complete: 100
SDComment:
SDCategory: Maraudon
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Spells
{
    SPELL_KNOCKAWAY         = 18670,
    SPELL_TRAMPLE           = 5568,
    SPELL_LANDSLIDE         = 21808
};

enum LandslideEvents
{
    EVENT_KNOCK_AWAY        = 1,
    EVENT_TRAMPLE           = 2
};

class boss_landslide : public CreatureScript
{
public:
    boss_landslide() : CreatureScript("boss_landslide") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_landslideAI(creature);
    }

    struct boss_landslideAI : public ScriptedAI
    {
        boss_landslideAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_KNOCK_AWAY, 8 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_TRAMPLE,    2 * IN_MILLISECONDS);
            _healthAbove50Pct = true;
        }

        void EnterCombat(Unit* /*who*/) override
        {
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
                    case EVENT_KNOCK_AWAY:
                        DoCastVictim(SPELL_KNOCKAWAY);
                        _events.ScheduleEvent(EVENT_KNOCK_AWAY, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_TRAMPLE:
                        DoCastSelf(SPELL_TRAMPLE);
                        _events.ScheduleEvent(EVENT_TRAMPLE, 8 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }

            }

            if (_healthAbove50Pct && HealthBelowPct(50))
            {
                _healthAbove50Pct = false;
                me->InterruptNonMeleeSpells(false);
                DoCastSelf(SPELL_LANDSLIDE);
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        bool _healthAbove50Pct;
    };
};

void AddSC_boss_landslide()
{
    new boss_landslide();
}
