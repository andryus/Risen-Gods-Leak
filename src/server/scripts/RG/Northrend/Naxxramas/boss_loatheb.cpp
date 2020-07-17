/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "naxxramas.h"

enum Spells
{
    //Loatheb
    SPELL_NECROTIC_AURA                                    = 55593,
    SPELL_SUMMON_SPORE                                     = 29234,
    SPELL_DEATHBLOOM                                       = 29865,
    H_SPELL_DEATHBLOOM                                     = 55053,
    SPELL_INEVITABLE_DOOM                                  = 29204,
    H_SPELL_INEVITABLE_DOOM                                = 55052,
    SPELL_ENRAGE                                           = 26662,
    //Spore
    SPELL_FUNGAL_CREEP                                     = 29232
};

enum Events
{
    EVENT_AURA      = 1,
    EVENT_BLOOM     = 2,
    EVENT_DOOM      = 3,
    EVENT_ENRAGE    = 4
};

enum Achievments
{
    ACHIEV_SPORE_LOSER_10                                     = 2182,
    ACHIEV_SPORE_LOSER_25                                     = 2183,
};

enum LoathebActions
{
    ACTION_SPORE_DIED,
};

class boss_loatheb : public CreatureScript
{
    public:
        boss_loatheb() : CreatureScript("boss_loatheb") { }

        struct boss_loathebAI : public BossAI
        {
            boss_loathebAI(Creature* creature) : BossAI(creature, BOSS_LOATHEB) { }

            bool SporeDead;

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);
                events.ScheduleEvent(EVENT_AURA, 10000);
                events.ScheduleEvent(EVENT_BLOOM, 5000);
                events.ScheduleEvent(EVENT_DOOM, 120000);
                events.ScheduleEvent(EVENT_ENRAGE, 7*60000);
                SporeDead = false;
            }

            void DoAction(int32 action)
            {
                if(action == ACTION_SPORE_DIED)
                    SporeDead = true;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);


                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_AURA:
                            DoCastAOE(SPELL_NECROTIC_AURA);
                            events.ScheduleEvent(EVENT_AURA, 20000);
                            break;
                        case EVENT_BLOOM:
                            // TODO : Add missing text
                            DoCastAOE(SPELL_SUMMON_SPORE, true);
                            DoCastAOE(RAID_MODE(SPELL_DEATHBLOOM, H_SPELL_DEATHBLOOM));
                            events.ScheduleEvent(EVENT_BLOOM, 30000);
                            break;
                        case EVENT_DOOM:
                            DoCastAOE(RAID_MODE(SPELL_INEVITABLE_DOOM, H_SPELL_INEVITABLE_DOOM));
                            events.ScheduleEvent(EVENT_DOOM, events.GetTimer() < 5*60000 ? 30000 : 15000);
                            break;
                        case EVENT_ENRAGE:
                            DoCastSelf(SPELL_ENRAGE);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);

                if (instance && !SporeDead)
                    instance->DoCompleteAchievement(RAID_MODE(ACHIEV_SPORE_LOSER_10,ACHIEV_SPORE_LOSER_25));
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_loathebAI (creature);
    }
};

class npc_loatheb_spore : public CreatureScript
{
public:
    npc_loatheb_spore() : CreatureScript("npc_loatheb_spore") { }

    struct npc_loatheb_sporeAI : public ScriptedAI
    {
        npc_loatheb_sporeAI(Creature *creature) : ScriptedAI(creature)
        {
            Instance = creature->GetInstanceScript();
        }

        uint32 DeathTimer;

        InstanceScript* Instance;

        void JustDied(Unit* killer)
        {
            DoCast(killer, SPELL_FUNGAL_CREEP, true);

            if (Instance)
            {
                if (Creature *Loatheb = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(DATA_LOATHEB))))
                    if (Loatheb->AI())
                        Loatheb->AI()->DoAction(ACTION_SPORE_DIED);
            }
        }

        void Reset()
        {
            DeathTimer = 120000;
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (DeathTimer <= diff)
                me->DespawnOrUnsummon();

            else DeathTimer -= diff;

            DoMeleeAttackIfReady();
        }

    };

    CreatureAI* GetAI(Creature* Creature) const
    {
        return new npc_loatheb_sporeAI (Creature);
    }
};

void AddSC_boss_loatheb()
{
    new boss_loatheb();
    new npc_loatheb_spore();
}
