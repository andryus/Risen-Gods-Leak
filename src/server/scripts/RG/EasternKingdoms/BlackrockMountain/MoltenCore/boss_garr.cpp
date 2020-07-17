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
SDName: Boss_Garr
SD%Complete: 80
SDComment: Adds NYI
SDCategory: Molten Core
EndScriptData */

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "molten_core.h"

enum Spells
{
    // Garr
    SPELL_ANTIMAGIC_PULSE   = 19492,
    SPELL_MAGMA_SHACKLES    = 19496,
    SPELL_ENRAGE            = 19516,

    // Adds
    SPELL_ERUPTION          = 19497,
    SPELL_IMMOLATE          = 20294,
};

enum Events
{
    EVENT_ANTIMAGIC_PULSE    = 1,
    EVENT_MAGMA_SHACKLES     = 2,
};

class boss_garr : public CreatureScript
{
    public:
        boss_garr() : CreatureScript("boss_garr") { }

        struct boss_garrAI : public BossAI
        {
            boss_garrAI(Creature* creature) : BossAI(creature, BOSS_GARR) { }

            void Reset()
            {
                me->RemoveAurasDueToSpell(SPELL_ENRAGE);
            }

            void EnterCombat(Unit* victim) override
            {
                BossAI::EnterCombat(victim);
                events.ScheduleEvent(EVENT_ANTIMAGIC_PULSE, 25 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MAGMA_SHACKLES,  15 * IN_MILLISECONDS);
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
                        case EVENT_ANTIMAGIC_PULSE:
                            DoCastSelf(SPELL_ANTIMAGIC_PULSE);
                            events.ScheduleEvent(EVENT_ANTIMAGIC_PULSE, urand(10, 15)* IN_MILLISECONDS);
                            break;
                        case EVENT_MAGMA_SHACKLES:
                            DoCastSelf(SPELL_MAGMA_SHACKLES);
                            events.ScheduleEvent(EVENT_MAGMA_SHACKLES, urand(8, 12) * IN_MILLISECONDS);
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
            return new boss_garrAI(creature);
        }
};

class npc_firesworn : public CreatureScript
{
    public:
        npc_firesworn() : CreatureScript("npc_firesworn") { }

        struct npc_fireswornAI : public ScriptedAI
        {
            npc_fireswornAI(Creature* creature) : ScriptedAI(creature) { }
            
            void Reset() override
            {
                immolateTimer = 4 * IN_MILLISECONDS;      //These times are probably wrong
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage) override
            {
                uint32 const health10pct = me->CountPctFromMaxHealth(10);
                uint32 health = me->GetHealth();
                if (int32(health) - int32(damage) < int32(health10pct))
                {
                    damage = 0;
                    if (Creature* garr = me->FindNearestCreature(NPC_GARR, 300.0f))
                        garr->CastSpell(garr, SPELL_ENRAGE, true);
                    DoCastVictim(SPELL_ERUPTION);
                    me->DespawnOrUnsummon();
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (immolateTimer <= diff)
                {
                     if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_IMMOLATE);
                     immolateTimer = urand(5, 10) * IN_MILLISECONDS;
                }
                else
                    immolateTimer -= diff;

                DoMeleeAttackIfReady();
            }
            
        private:
            uint32 immolateTimer;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_fireswornAI(creature);
        }
};

void AddSC_boss_garr()
{
    new boss_garr();
    new npc_firesworn();
}
