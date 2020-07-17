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

/* ScriptData
SDName: Boss_Firemaw
SD%Complete: 100
SDComment:
SDCategory: Blackwing Lair
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackwing_lair.h"
#include "SpellInfo.h"

enum Spells
{
    SPELL_SHADOWFLAME       = 22539,
    SPELL_WINGBUFFET        = 23339,
    SPELL_FLAMEBUFFET       = 23341,
    SPELL_TRASH             = 12787,
};

enum Events
{
    EVENT_SHADOWFLAME       = 1,
    EVENT_WINGBUFFET,
    EVENT_FLAMEBUFFET,
};

class boss_firemaw : public CreatureScript
{
    public:
        boss_firemaw() : CreatureScript("boss_firemaw") { }
    
        struct boss_firemawAI : public BossAI
        {
            boss_firemawAI(Creature* creature) : BossAI(creature, BOSS_FIREMAW) { }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);

                DoCastSelf(SPELL_TRASH);
    
                events.ScheduleEvent(EVENT_SHADOWFLAME, urand(10, 20) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WINGBUFFET,             30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FLAMEBUFFET,             2 * IN_MILLISECONDS);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spellInfo)
            {
                if (spellInfo->Id == SPELL_WINGBUFFET)
                    if (GetThreat(target))
                        ModifyThreatByPercent(target, -75);
            }
    
            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);
    
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                if (!UpdateVictim())
                    return;
    
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_SHADOWFLAME:
                            DoCastVictim(SPELL_SHADOWFLAME);
                            events.ScheduleEvent(EVENT_SHADOWFLAME, urand(10, 20) * IN_MILLISECONDS);
                            break;
                        case EVENT_WINGBUFFET:
                            DoCastVictim(SPELL_WINGBUFFET);                                
                            events.ScheduleEvent(EVENT_WINGBUFFET, 30 * IN_MILLISECONDS);
                            break;
                        case EVENT_FLAMEBUFFET:
                            DoCastVictim(SPELL_FLAMEBUFFET);
                            events.ScheduleEvent(EVENT_FLAMEBUFFET, 2 * IN_MILLISECONDS);
                            break;
                    }
                }
    
                DoMeleeAttackIfReady();
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_firemawAI>(creature);
        }
};

void AddSC_boss_firemaw()
{
    new boss_firemaw();
}
