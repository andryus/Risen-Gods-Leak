/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2014-2016 Rising Gods <http://www.rising-gods.de/>
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

/*
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "npc_pet_pri_".
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "PassiveAI.h"
#include "PetAI.h"

enum PriestSpells
{
    SPELL_PRIEST_GLYPH_OF_SHADOWFIEND       = 58228,
    SPELL_PRIEST_GLYPH_OF_SHADOWFIEND_MANA  = 58227,
    SPELL_PRIEST_LIGHTWELL_CHARGES          = 59907
};

class npc_pet_pri_lightwell : public CreatureScript
{
    public:
        npc_pet_pri_lightwell() : CreatureScript("npc_pet_pri_lightwell") { }

        struct npc_pet_pri_lightwellAI : public PassiveAI
        {
            npc_pet_pri_lightwellAI(Creature* creature) : PassiveAI(creature)
            {
                DoCastSelf(SPELL_PRIEST_LIGHTWELL_CHARGES, false);
            }

            void Reset() override
            {
                level = true;
            }

            void UpdateAI(uint32 /*diff*/)
            {
                // Needed because the owner is sometimes not available
                // due to asynchrone threads
                if (level)
                {
                    if (Unit* owner = me->GetOwner())
                    {
                        me->SetLevel(owner->getLevel());
                        level = false;
                    }
                }
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (!me->IsAlive())
                    return;

                me->GetThreatManager().ClearAllThreat();
                me->CombatStop(true);
                me->ResetPlayerDamageReq();
            }

        private:
            bool level;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_pri_lightwellAI(creature);
        }
};

class npc_pet_pri_shadowfiend : public CreatureScript
{
    public:
        npc_pet_pri_shadowfiend() : CreatureScript("npc_pet_pri_shadowfiend") { }

        struct npc_pet_pri_shadowfiendAI : public PetAI
        {
            npc_pet_pri_shadowfiendAI(Creature* creature) : PetAI(creature) { }

            void Reset() override
            {
                PetAI::Reset();
            }

            bool CanAIAttack(Unit const* victim) const override
            {
                if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                    if (!player->IsPvP() && victim->IsPvP())
                        return false;

                return true;
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (me->IsSummon())
                    if (Unit* owner = me->ToTempSummon()->GetSummoner())
                        if (owner->HasAura(SPELL_PRIEST_GLYPH_OF_SHADOWFIEND))
                            owner->CastSpell(owner, SPELL_PRIEST_GLYPH_OF_SHADOWFIEND_MANA, true);
            }

            void UpdateAI(uint32 diff) override
            {
                if (me->HasAuraWithMechanic((1 << MECHANIC_STUN) | (1 << MECHANIC_FEAR) | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_DISORIENTED) | (1 << MECHANIC_SILENCE) | (1 << MECHANIC_KNOCKOUT) | (1 << MECHANIC_BANISH)))
                    return;

                if (!me->GetVictim())
                    if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : nullptr)
                        if (Unit* target = player->GetSelectedUnit())
                        {
                            if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH) && me->IsValidAttackTarget(target))
                                AttackStart(target);
                        }                        

                if (!UpdateVictim())
                    return;
               
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_pri_shadowfiendAI(creature);
        }
};

void AddSC_priest_pet_scripts()
{
    new npc_pet_pri_lightwell();
    new npc_pet_pri_shadowfiend();
}
