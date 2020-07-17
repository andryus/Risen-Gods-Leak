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
SDName: Boss_Flamegor
SD%Complete: 100
SDComment:
SDCategory: Blackwing Lair
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "blackwing_lair.h"
#include "SpellInfo.h"


enum Emotes
{
    EMOTE_FRENZY            = 0,
};

enum Spells
{
    SPELL_SHADOWFLAME        = 22539,
    SPELL_WINGBUFFET         = 23339,
    SPELL_FRENZY             = 23342  // This spell periodically triggers fire nova
};

enum Events
{
    EVENT_SHADOWFLAME       = 1,
    EVENT_WINGBUFFET        = 2,
    EVENT_FRENZY            = 3
};

class boss_flamegor : public CreatureScript
{
    public:
        boss_flamegor() : CreatureScript("boss_flamegor") { }
    
        struct boss_flamegorAI : public BossAI
        {
            boss_flamegorAI(Creature* creature) : BossAI(creature, BOSS_FLAMEGOR) { }
    
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
    
                events.ScheduleEvent(EVENT_SHADOWFLAME, urand(10, 20) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_WINGBUFFET,             30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_FRENZY,                 8 * IN_MILLISECONDS);
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
                        case EVENT_FRENZY:
                            Talk(EMOTE_FRENZY);
                            DoCastSelf(SPELL_FRENZY);
                            events.ScheduleEvent(EVENT_FRENZY, urand(8, 10) * IN_MILLISECONDS);
                            break;
                    }
                }
    
                DoMeleeAttackIfReady();
            }
        };
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_flamegorAI>(creature);
        }
};

void AddSC_boss_flamegor()
{
    new boss_flamegor();
}
