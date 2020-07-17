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
#include "blackrock_spire.h"

enum Spells
{
    SPELL_FIRENOVA                  = 23462,
    SPELL_CLEAVE                    = 20691,
    SPELL_CONFLIGURATION            = 16805,
    SPELL_THUNDERCLAP               = 15548, //Not sure if right ID. 23931 would be a harder possibility.
};

enum Events
{
    EVENT_FIRE_NOVA                = 1,
    EVENT_CLEAVE                   = 2,
    EVENT_CONFLIGURATION           = 3,
    EVENT_THUNDERCLAP              = 4,
};

class boss_drakkisath : public CreatureScript
{
    public:
        boss_drakkisath() : CreatureScript("boss_drakkisath") { }
    
        struct boss_drakkisathAI : public BossAI
        {
            boss_drakkisathAI(Creature* creature) : BossAI(creature, DATA_GENERAL_DRAKKISATH) { }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_FIRE_NOVA,       6 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CLEAVE,          8 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CONFLIGURATION, 15 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_THUNDERCLAP,    17 * IN_MILLISECONDS);
            }
    
            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);
                if (GameObject* door1 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_DRAKKISATH_DOOR_1)))
                    door1->SetGoState(GO_STATE_ACTIVE);
                if (GameObject* door2 = me->GetMap()->GetGameObject(instance->GetGuidData(GO_DRAKKISATH_DOOR_2)))
                    door2->SetGoState(GO_STATE_ACTIVE);
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
                        case EVENT_FIRE_NOVA:
                            DoCastVictim(SPELL_FIRENOVA);
                            events.ScheduleEvent(EVENT_FIRE_NOVA, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_CLEAVE:
                            DoCastVictim(SPELL_CLEAVE);
                            events.ScheduleEvent(EVENT_CLEAVE, 8 * IN_MILLISECONDS);
                            break;
                        case EVENT_CONFLIGURATION:
                            DoCastVictim(SPELL_CONFLIGURATION);
                            events.ScheduleEvent(EVENT_CONFLIGURATION, 18 * IN_MILLISECONDS);
                            break;
                        case EVENT_THUNDERCLAP:
                            DoCastVictim(SPELL_THUNDERCLAP);
                            events.ScheduleEvent(EVENT_THUNDERCLAP, 20 * IN_MILLISECONDS);
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_drakkisathAI(creature);
        }
};

void AddSC_boss_drakkisath()
{
    new boss_drakkisath();
}
