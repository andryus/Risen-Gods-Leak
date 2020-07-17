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
#include "blackrock_spire.h"

enum Spells
{
    SPELL_REND                      = 16509,
    SPELL_STRIKE                    = 15580,
    SPELL_INTIMIDATING_ROAR         = 16508
};

enum Says
{
    SAY_SUMMON                      = 0,
    SAY_AGGRO                       = 1,
};

enum Events
{
    EVENT_REND                      = 1,
    EVENT_STRIKE                    = 2,
    EVENT_INTIMIDATING_ROAR         = 3
};

class boss_urok_doomhowl : public CreatureScript
{
    public:
        boss_urok_doomhowl() : CreatureScript("boss_urok_doomhowl") { }
    
        struct boss_urok_doomhowlAI : public BossAI
        {
            boss_urok_doomhowlAI(Creature* creature) : BossAI(creature, DATA_UROK_DOOMHOWL) { }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(SPELL_REND,   urand(17, 20) * IN_MILLISECONDS);
                events.ScheduleEvent(SPELL_STRIKE, urand(10, 12) * IN_MILLISECONDS);
                Talk(SAY_AGGRO);
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
                        case SPELL_REND:
                            DoCastVictim(SPELL_REND);
                            events.ScheduleEvent(SPELL_REND, urand(8, 10) * IN_MILLISECONDS);
                            break;
                        case SPELL_STRIKE:
                            DoCastVictim(SPELL_STRIKE);
                            events.ScheduleEvent(SPELL_STRIKE, urand(8, 10) * IN_MILLISECONDS);
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
            return new boss_urok_doomhowlAI(creature);
        }
};

void AddSC_boss_urok_doomhowl()
{
    new boss_urok_doomhowl();
}
