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
#include "blackwing_lair.h"

enum Say
{
    SAY_AGGRO               = 0,
    SAY_LEASH               = 1,
};

enum Spells
{
    SPELL_CLEAVE            = 26350,
    SPELL_BLASTWAVE         = 23331,
    SPELL_MORTALSTRIKE      = 24573,
    SPELL_KNOCKBACK         = 25778
};

enum Events
{
    EVENT_CLEAVE            = 1,
    EVENT_BLASTWAVE         = 2,
    EVENT_MORTALSTRIKE      = 3,
    EVENT_KNOCKBACK         = 4
};

class boss_broodlord : public CreatureScript
{
    public:
        boss_broodlord() : CreatureScript("boss_broodlord") { }
    
        struct boss_broodlordAI : public BossAI
        {
            boss_broodlordAI(Creature* creature) : BossAI(creature, BOSS_BROODLORD) { }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_CLEAVE,        8 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BLASTWAVE,    12 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MORTALSTRIKE, 20 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_KNOCKBACK,    30 * IN_MILLISECONDS);
            }
    
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;
    
                events.Update(diff);
    
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_CLEAVE:
                            DoCastVictim(SPELL_CLEAVE);
                            events.ScheduleEvent(EVENT_CLEAVE, 7 * IN_MILLISECONDS);
                            break;
                        case EVENT_BLASTWAVE:
                            DoCastVictim(SPELL_BLASTWAVE);
                            events.ScheduleEvent(EVENT_BLASTWAVE, 20 * IN_MILLISECONDS);
                            break;
                        case EVENT_MORTALSTRIKE:
                            DoCastVictim(SPELL_MORTALSTRIKE);
                            events.ScheduleEvent(EVENT_MORTALSTRIKE, 11 * IN_MILLISECONDS);
                            break;
                        case EVENT_KNOCKBACK:
                            DoCastVictim(SPELL_KNOCKBACK);
                            if (GetThreat(me->GetVictim()))
                                ModifyThreatByPercent(me->GetVictim(), -50);
                            events.ScheduleEvent(EVENT_KNOCKBACK, urand(15, 30) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
    
                DoMeleeAttackIfReady();
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_broodlordAI>(creature);
        }
};

void AddSC_boss_broodlord()
{
    new boss_broodlord();
}
