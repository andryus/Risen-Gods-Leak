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
 * Scriptnames of files in this file should be prefixed with "npc_pet_mag_".
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "CombatAI.h"
#include "Pet.h"

enum MirrorImageSpells
{
    SPELL_MAGE_CLONE_ME             = 45204,
    SPELL_MAGE_MASTERS_THREAT_LIST  = 58838,
    SPELL_MAGE_FROSTBOLT            = 59638,
    SPELL_MAGE_FIREBLAST            = 59637
};

class npc_pet_mage_mirror_image : public CreatureScript
{
    public:
        npc_pet_mage_mirror_image() : CreatureScript("npc_pet_mage_mirror_image") { }
    
        struct npc_pet_mage_mirror_imageAI : ScriptedAI
        {
            npc_pet_mage_mirror_imageAI(Creature* creature) : ScriptedAI(creature) { }
    
            void InitializeAI() override
            {
                Unit* owner = me->GetOwner();
                if (!owner)
                    return;
                // Inherit Master's Threat List (not yet implemented)
                // owner->CastSpell((Unit*)NULL, SPELL_MAGE_MASTERS_THREAT_LIST, true);
                // here mirror image casts on summoner spell (not present in client dbc) 49866
                // here should be auras (not present in client dbc): 35657, 35658, 35659, 35660 selfcasted by mirror images (stats related?)
                // Clone Me!
                owner->CastSpell(me, SPELL_MAGE_CLONE_ME);
                castCounter = 0;
            }
    
            void UpdateAI(uint32 diff) override
            {
                Unit* owner = me->GetOwner();
                if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                    return;
    
                if (Unit* ownerPet = owner->ToPlayer()->GetGuardianPet())
                    AttackStart(ownerPet->GetVictim() ? ownerPet->GetVictim() : NULL);
                else if (Unit* ownerPet = owner->ToPlayer()->GetPet())
                    AttackStart(ownerPet->GetVictim() ? ownerPet->GetVictim() : NULL);
                else
                {
                    if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : NULL)
                        if (Unit* target = player->GetSelectedUnit())
                            if (target->IsHostileTo(owner))
                                AttackStart(target);
                }
    
                if (!UpdateVictim())
                    return;
    
                if (me->GetVictim()->HasBreakableByDamageCrowdControlAura(me))
                {
                    me->InterruptNonMeleeSpells(false);
                    return;
                }
    
                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
    
                // Frostbolt, Frostbolt, Fireblast roation
                if ((castCounter++) % 3 < 2)
                    DoCastVictim(SPELL_MAGE_FROSTBOLT);
                else
                    DoCastVictim(SPELL_MAGE_FIREBLAST);
            }
    
            // Do not reload Creature templates on evade mode enter - prevent visual lost
            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (me->IsInEvadeMode() || !me->IsAlive())
                    return;
    
                Unit* owner = me->GetCharmerOrOwner();
    
                me->CombatStop(true);
                if (owner && !me->HasUnitState(UNIT_STATE_FOLLOW))
                {
                    me->GetMotionMaster()->Clear(false);
                    me->GetMotionMaster()->MoveFollow(owner, PET_FOLLOW_DIST, me->GetFollowAngle(), MOTION_SLOT_ACTIVE);
                }
            }
    
        private:
            uint8 castCounter;
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_mage_mirror_imageAI(creature);
        }
};


void AddSC_mage_pet_scripts()
{
    new npc_pet_mage_mirror_image();
}
