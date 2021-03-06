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
    SPELL_FROSTBOLT  = 17503,
    SPELL_DRAINLIFE  = 20743,
    SPELL_DRAIN_MANA = 17243,
    SPELL_ICETOMB    = 16869
};

enum Events
{
    EVENT_FROSTBOLT  = 1,
    EVENT_DRAINLIFE  = 2,
    EVENT_ICETOMB    = 3 
};

class boss_maleki_the_pallid : public CreatureScript
{
public:
    boss_maleki_the_pallid() : CreatureScript("boss_maleki_the_pallid") { }

    struct boss_maleki_the_pallidAI : public ScriptedAI
    {
        boss_maleki_the_pallidAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = me->GetInstanceScript();
        }

        void Reset() override
        {
             _events.Reset();
            _events.ScheduleEvent(EVENT_FROSTBOLT,  1 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_ICETOMB,   16 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_DRAINLIFE, 31 * IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) override
        {
        }

        void JustDied(Unit* /*killer*/) override
        {
            instance->SetData(TYPE_PALLID, IN_PROGRESS);
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
                    case EVENT_FROSTBOLT:
                        if (rand32()%100 < 90)
                            DoCastVictim(SPELL_FROSTBOLT);
                        _events.ScheduleEvent(EVENT_FROSTBOLT, 3.5 * IN_MILLISECONDS);
                        break;
                    case EVENT_ICETOMB:
                        if (rand32()%100 < 65)
                            DoCastVictim(SPELL_ICETOMB);
                        _events.ScheduleEvent(EVENT_ICETOMB, 28 * IN_MILLISECONDS);
                        break;
                    case EVENT_DRAINLIFE:
                        if (rand32()%100 < 55)
                            DoCastVictim(SPELL_DRAINLIFE);
                        _events.ScheduleEvent(EVENT_DRAINLIFE, 31 * IN_MILLISECONDS);
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
        return GetInstanceAI<boss_maleki_the_pallidAI>(creature);
    }
};

void AddSC_boss_maleki_the_pallid()
{
    new boss_maleki_the_pallid();
}
