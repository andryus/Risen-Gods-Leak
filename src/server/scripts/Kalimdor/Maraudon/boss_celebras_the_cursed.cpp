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
SDName: Boss_Celebras_the_Cursed
SD%Complete: 100
SDComment:
SDCategory: Maraudon
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Spells
{
    SPELL_WRATH                 = 21807,
    SPELL_ENTANGLINGROOTS       = 12747,
    SPELL_CORRUPT_FORCES        = 21968
};

enum CelebrasMisc
{
    EVENT_WRATH                 = 0,
    EVENT_ROOTS                 = 1,
    EVENT_CORRUPT_FORCES        = 2,
    NPC_CELEBRAS                = 13716
};

class celebras_the_cursed : public CreatureScript
{
public:
    celebras_the_cursed() : CreatureScript("celebras_the_cursed") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new celebras_the_cursedAI(creature);
    }

    struct celebras_the_cursedAI : public ScriptedAI
    {
        celebras_the_cursedAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_WRATH, 8*IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_ROOTS, 2*IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_CORRUPT_FORCES, 30*IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) override { }

        void JustDied(Unit* /*killer*/) override
        {
            me->SummonCreature(NPC_CELEBRAS, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 600000);
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
                    case EVENT_WRATH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                            DoCast(target, SPELL_WRATH);
                        _events.ScheduleEvent(EVENT_WRATH, 8*IN_MILLISECONDS);
                        break;
                    case EVENT_ROOTS:
                        DoCastVictim(SPELL_ENTANGLINGROOTS);
                        _events.ScheduleEvent(EVENT_ROOTS, 20*IN_MILLISECONDS);
                        break;
                    case EVENT_CORRUPT_FORCES:
                        me->InterruptNonMeleeSpells(false);
                        DoCastSelf(SPELL_CORRUPT_FORCES);
                        _events.ScheduleEvent(EVENT_CORRUPT_FORCES, 20*IN_MILLISECONDS);
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

void AddSC_boss_celebras_the_cursed()
{
    new celebras_the_cursed();
}
