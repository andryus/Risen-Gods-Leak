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
#include "blackrock_depths.h"

enum Spells
{
    SPELL_FIREBLAST      = 15573,
    SPELL_BURNING_SPIRIT = 13489
};

enum Events
{
    EVENT_FIREBLAST      = 1,
    EVENT_SUMMON         = 2
};

class boss_ambassador_flamelash : public CreatureScript
{
    public:
        boss_ambassador_flamelash() : CreatureScript("boss_ambassador_flamelash") { }
    
        struct boss_ambassador_flamelashAI : public ScriptedAI
        {
            boss_ambassador_flamelashAI(Creature* creature) : ScriptedAI(creature) { }
    
            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_FIREBLAST, 2 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_SUMMON,    4 * IN_MILLISECONDS);
                ResetGameobjects();
            }
    
            void EnterCombat(Unit* /*who*/) override
            {
                std::list<GameObject*> RuneList;
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_A01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_B01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_C01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_D01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_E01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_F01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_G01, 300.0f);
                for (std::list<GameObject*>::const_iterator itr = RuneList.begin(); itr != RuneList.end(); ++itr)
                    if (GameObject* rune = (*itr))
                        rune->UseDoorOrButton();
            }
    
            void ActivateGameobjects() 
            {
                std::list<GameObject*> RuneList;
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_A01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_B01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_C01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_D01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_E01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_F01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_G01, 300.0f);
                for (std::list<GameObject*>::const_iterator itr = RuneList.begin(); itr != RuneList.end(); ++itr)
                    if (GameObject* rune = (*itr))
                    {
                        if (Creature* Spirit = rune->SummonCreature(NPC_BURNING_SPIRIT, rune->GetPositionX(), rune->GetPositionY(), rune->GetPositionZ(), 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 60000))
                        {
                            Spirit->AI()->AttackStart(Spirit->FindNearestPlayer(200.0f));
                            Spirit->SetInCombatWithZone();
                            Spirit->AddAura(SPELL_BURNING_SPIRIT, Spirit);
                        }                       
                    }                    
            }
    
            void ResetGameobjects() 
            {
                std::list<GameObject*> RuneList;
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_A01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_B01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_C01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_D01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_E01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_F01, 300.0f);
                me->GetGameObjectListWithEntryInGrid(RuneList, GO_DWARFRUNE_G01, 300.0f);
                for (std::list<GameObject*>::const_iterator itr = RuneList.begin(); itr != RuneList.end(); ++itr)
                    if (GameObject* rune = (*itr))
                        rune->ResetDoorOrButton();
            }
    
            void JustDied(Unit* /*killer*/) override
            {
                ResetGameobjects();
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
                        case EVENT_FIREBLAST:
                            DoCastVictim(SPELL_FIREBLAST);
                            _events.ScheduleEvent(EVENT_FIREBLAST, 7 * IN_MILLISECONDS);
                            break;
                        case EVENT_SUMMON:
                            ActivateGameobjects();
                            _events.ScheduleEvent(EVENT_FIREBLAST, 30 * IN_MILLISECONDS);
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
        
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_ambassador_flamelashAI(creature);
        }
};

void AddSC_boss_ambassador_flamelash()
{
    new boss_ambassador_flamelash();
}
