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
    SPELL_FRENZY                    = 8269,
    SPELL_SUMMON_SPECTRAL_ASSASSIN  = 27249,
    SPELL_SHADOW_BOLT_VOLLEY        = 27382,
    SPELL_SHADOW_WRATH              = 27286
};

enum Says
{
    EMOTE_FRENZY                    = 0
};

enum Events
{
    EVENT_SUMMON_SPECTRAL_ASSASSIN  = 1,
    EVENT_SHADOW_BOLT_VOLLEY        = 2,
    EVENT_SHADOW_WRATH              = 3
};

class boss_lord_valthalak : public CreatureScript
{
    public:
        boss_lord_valthalak() : CreatureScript("boss_lord_valthalak") { }
    
        struct boss_lord_valthalakAI : public BossAI
        {
            boss_lord_valthalakAI(Creature* creature) : BossAI(creature, DATA_LORD_VALTHALAK) { }
    
            void Reset() override
            {
                BossAI::Reset();
                frenzy40 = false;
                frenzy15 = false;
            }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_SUMMON_SPECTRAL_ASSASSIN,  urand(6,8) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SHADOW_WRATH,             urand(9,18) * IN_MILLISECONDS);
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                instance->SetData(DATA_LORD_VALTHALAK, DONE);
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
                        case EVENT_SUMMON_SPECTRAL_ASSASSIN:
                            DoCastSelf(SPELL_SUMMON_SPECTRAL_ASSASSIN);
                            events.ScheduleEvent(EVENT_SUMMON_SPECTRAL_ASSASSIN, urand(30,350) * IN_MILLISECONDS);
                            break;
                        case EVENT_SHADOW_BOLT_VOLLEY:
                            DoCastVictim(SPELL_SHADOW_BOLT_VOLLEY);
                            events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, urand(4,6) * IN_MILLISECONDS);
                            break;
                        case EVENT_SHADOW_WRATH:
                            DoCastVictim(SPELL_SHADOW_WRATH);
                            events.ScheduleEvent(EVENT_SHADOW_WRATH, urand(19,24) * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }
    
                if (!frenzy40)
                {
                    if (HealthBelowPct(40))
                    {
                        DoCastSelf(SPELL_FRENZY);
                        events.CancelEvent(EVENT_SUMMON_SPECTRAL_ASSASSIN);
                        frenzy40 = true;
                    }
                }
    
                if (!frenzy15)
                {
                    if (HealthBelowPct(15))
                    {
                        DoCastSelf(SPELL_FRENZY);
                        events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, urand(7,14) * IN_MILLISECONDS);
                        frenzy15 = true;
                    }
                }
    
                DoMeleeAttackIfReady();
            }
        private:
            bool frenzy40;
            bool frenzy15;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_lord_valthalakAI>(creature);
        }
};

void AddSC_boss_lord_valthalak()
{
    new boss_lord_valthalak();
}
