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
    SPELL_FRENZY                    = 8269,
    SPELL_KNOCK_AWAY                = 10101
};

enum Events
{
    EVENT_FRENZY                    = 1,
    EVENT_KNOCK_AWAY                = 2
};

enum Texts
{
    SAY_AGGRO                       = 0,
    SAY_KILL                        = 1
};

class boss_highlord_omokk : public CreatureScript
{
    public:
        boss_highlord_omokk() : CreatureScript("boss_highlord_omokk") { }
    
        struct boss_highlordomokkAI : public BossAI
        {
            boss_highlordomokkAI(Creature* creature) : BossAI(creature, DATA_HIGHLORD_OMOKK) { }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_FRENZY,      20 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_KNOCK_AWAY,  18 * IN_MILLISECONDS);
                Talk(SAY_AGGRO);
            }
    
            void KilledUnit(Unit* victim) override
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL, victim);
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
                        case EVENT_FRENZY:
                            DoCastVictim(SPELL_FRENZY);
                            events.ScheduleEvent(EVENT_FRENZY, 60 * IN_MILLISECONDS);
                            break;
                        case EVENT_KNOCK_AWAY:
                            DoCastVictim(SPELL_KNOCK_AWAY);
                            events.ScheduleEvent(EVENT_KNOCK_AWAY, 12 * IN_MILLISECONDS);
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
            return new boss_highlordomokkAI(creature);
        }
};

void AddSC_boss_highlordomokk()
{
    new boss_highlord_omokk();
}
