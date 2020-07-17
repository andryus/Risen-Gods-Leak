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

enum AnastariSpells
{
    // Spells
    SPELL_BANSHEEWAIL     = 16565,
    SPELL_BANSHEECURSE    = 16867,
    SPELL_SILENCE         = 18327,
    SPELL_POSSESS         = 17244, 
    SPELL_POSSESSED       = 17246,
    SPELL_POSSESS_INV     = 17250         // baroness becomes invisible while possessing a target
};

enum AnastariEvents
{
    // Events
    EVENT_BANSHEEWAIL     = 1,
    EVENT_BANSHEECURSE    = 2,
    EVENT_SILENCE         = 3,
    EVENT_POSSESS         = 4,
    EVENT_POSSESS_END     = 5
};

class boss_baroness_anastari : public CreatureScript
{
    public:
        boss_baroness_anastari() : CreatureScript("boss_baroness_anastari") { }

        struct boss_baroness_anastariAI : public ScriptedAI
        {
            boss_baroness_anastariAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = me->GetInstanceScript();
            }
            
            void Reset() override
            {
                _events.Reset();
                uiPlayer.Clear();
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                // If it's invisible don't evade
                if (m_uiPossessEndTimer)
                    return;
                
                ScriptedAI::EnterEvadeMode();
            }

            void EnterCombat(Unit* /*who*/) override
            {
                _events.ScheduleEvent(EVENT_BANSHEEWAIL,   1 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_BANSHEECURSE, 11 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_SILENCE,      13 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_POSSESS,      60 * IN_MILLISECONDS);
            }

            void JustDied(Unit* /*killer*/) override
            {
                instance->SetData(TYPE_BARONESS, IN_PROGRESS);
            }

            void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
            {
                uiPlayer = guid;
            }

            void UpdateAI(uint32 diff) override
            {
                if (m_uiPossessEndTimer)
                {
                    // Check if the possessed player has been damaged
                    if (m_uiPossessEndTimer <= diff)
                    {
                        // If aura has expired, return to fight
                        if (!me->HasAura(SPELL_POSSESS_INV))
                        {
                            m_uiPossessEndTimer = 0;
                            return;
                        }

                        // Check for possessed player
                        if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayer))
                        {
                            if (!player || !player->IsAlive())
                            {
                                me->RemoveAurasDueToSpell(SPELL_POSSESS_INV);
                                m_uiPossessEndTimer = 0;
                                return;
                            }

                            // If possessed player has less than 50% health
                            if (player->GetHealth() <= player->GetMaxHealth() * .5f)
                            {
                                me->RemoveAurasDueToSpell(SPELL_POSSESS_INV);
                                player->RemoveAurasDueToSpell(SPELL_POSSESSED);
                                player->RemoveAurasDueToSpell(SPELL_POSSESS);
                                m_uiPossessEndTimer = 0;
                                return;
                            }

                            m_uiPossessEndTimer = 1 * IN_MILLISECONDS;
                        }                        
                    }
                    else
                        m_uiPossessEndTimer -= diff;
                }

                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BANSHEEWAIL:
                            if (rand32()%100 < 95)
                                DoCastVictim(SPELL_BANSHEEWAIL);
                            _events.ScheduleEvent(EVENT_BANSHEEWAIL, 4 * IN_MILLISECONDS);
                            break;
                        case EVENT_BANSHEECURSE:
                            if (rand32()%100 < 75)
                                DoCastVictim(SPELL_BANSHEECURSE);
                            _events.ScheduleEvent(EVENT_BANSHEECURSE, 18 * IN_MILLISECONDS);
                            break;
                        case EVENT_SILENCE:
                            if (rand32()%100 < 80)
                                DoCastVictim(SPELL_SILENCE);
                            _events.ScheduleEvent(EVENT_SILENCE, 13 * IN_MILLISECONDS);
                            break;
                        case EVENT_POSSESS:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 200.0f, true))
                            {
                                DoCast(target, SPELL_POSSESS);
                                DoCast(target, SPELL_POSSESSED, true);
                                DoCastSelf(SPELL_POSSESS_INV, true);
                                me->AI()->SetGuidData(target->GetGUID());
                            }
                            m_uiPossessEndTimer = 1 * IN_MILLISECONDS;
                            _events.ScheduleEvent(EVENT_POSSESS, 30*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
            
        private:
            EventMap _events;
            uint32 m_uiPossessEndTimer;
            InstanceScript* instance;
            ObjectGuid uiPlayer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_baroness_anastariAI>(creature);
        }
};

void AddSC_boss_baroness_anastari()
{
    new boss_baroness_anastari();
}
