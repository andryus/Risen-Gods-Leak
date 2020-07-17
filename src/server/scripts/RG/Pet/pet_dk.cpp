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
 * Scriptnames of files in this file should be prefixed with "npc_pet_dk_".
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Pet.h"
#include "CombatAI.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

enum DeathKnightSpells
{
    SPELL_DK_SUMMON_GARGOYLE_1  = 49206,
    SPELL_DK_SUMMON_GARGOYLE_2  = 50514,
    SPELL_DK_DISMISS_GARGOYLE   = 50515,
    SPELL_DK_SANCTUARY          = 54661,
    SPELL_DK_GARGOYLE_STRIKE    = 51963
};

enum DeathKnightEvents
{
    EVENT_INTERRUPT_OVER = 1
};

class npc_pet_dk_charmed_ghul : public CreatureScript
{
    public:
        npc_pet_dk_charmed_ghul() : CreatureScript("npc_pet_dk_charmed_ghul") { }
    
        struct npc_pet_dk_charmed_ghulAI : public ScriptedAI
        {
            npc_pet_dk_charmed_ghulAI(Creature* creature) : ScriptedAI(creature)
            {
                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->DespawnOrUnsummon(5 * IN_MILLISECONDS * MINUTE);
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_dk_charmed_ghulAI(creature);
        }
};

class npc_pet_dk_ebon_gargoyle : public CreatureScript
{
    public:
        npc_pet_dk_ebon_gargoyle() : CreatureScript("npc_pet_dk_ebon_gargoyle") { }

        struct npc_pet_dk_ebon_gargoyleAI : ScriptedAI
        {
            npc_pet_dk_ebon_gargoyleAI(Creature* creature) : ScriptedAI(creature) { }

            void InitializeAI() override
            {
                targetFound     = false;
                interrupted     = false;
                EnableSpellCast = false;
                despawnTimer    = 0   * IN_MILLISECONDS;
                spellCastTime   = urand(2, 3) * IN_MILLISECONDS;  
                initStatTimer   = 0.1 * IN_MILLISECONDS;                
            }

            void JustDied(Unit* /*killer*/) override
            {
                // Stop Feeding Gargoyle when it dies
                if (Unit* owner = me->GetOwner())
                    owner->RemoveAurasDueToSpell(SPELL_DK_SUMMON_GARGOYLE_2);
            }

            void IsInteruptSpell(SpellInfo const* spell)
            {
                if (spell->HasEffect(SPELL_EFFECT_INTERRUPT_CAST))
                {
                    interrupted = true;
                    events.ScheduleEvent(EVENT_INTERRUPT_OVER, spell->GetDuration());
                }
            }

            bool CheckCast()
            {
                if (me->IsWithinMeleeRange(me->GetVictim()))
                    return roll_chance_i(40);
                else
                    return false;
            }

            // Fly away when dismissed
            void SpellHit(Unit* source, SpellInfo const* spell) override
            {
                IsInteruptSpell(spell);

                if (spell->Id != SPELL_DK_DISMISS_GARGOYLE || !me->IsAlive())
                    return;

                Unit* owner = me->GetOwner();

                if (!owner || owner != source)
                    return;

                // Stop Fighting
                me->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, true);
                // Sanctuary
                DoCastSelf(SPELL_DK_SANCTUARY, true);
                me->SetReactState(REACT_PASSIVE);

                //! HACK: Creature's can't have MOVEMENTFLAG_FLYING
                // Fly Away
                me->AddUnitMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_ASCENDING | MOVEMENTFLAG_FLYING);
                me->SetCanFly(true);
                me->SetSpeedRate(MOVE_FLIGHT, 0.75f);
                me->SetSpeedRate(MOVE_RUN, 0.75f);
                float x = me->GetPositionX() + 20 * std::cos(me->GetOrientation());
                float y = me->GetPositionY() + 20 * std::sin(me->GetOrientation());
                float z = me->GetPositionZ() + 40;
                me->GetMotionMaster()->Clear(false);
                me->GetMotionMaster()->MovePoint(0, x, y, z);

                despawnTimer = 4 * IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                if (despawnTimer > 0)
                {
                    if (despawnTimer > diff)
                        despawnTimer -= diff;
                    else
                        me->DespawnOrUnsummon();

                    return;
                }

                if (spellCastTime <= diff)
                {
                    if (!EnableSpellCast)
                        EnableSpellCast = true;
                }
                else 
                    spellCastTime -= diff;

                if (initStatTimer <= diff)
                {
                    if (Unit* owner = me->GetOwner())
                    {
                        me->SetMaxHealth(uint32(owner->GetMaxHealth() * 0.8f));
                        me->SetHealth(me->GetMaxHealth());
                    }
                    initStatTimer = 120 * IN_MILLISECONDS;
                }    
                else
                    initStatTimer -= diff;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INTERRUPT_OVER:
                            interrupted = false;
                            break;
                        default:
                            break;
                    }
                }

                Unit* owner = me->GetOwner();
                if (!owner || owner->GetTypeId() != TYPEID_PLAYER)
                    return;

                if (Unit* ownerPet = owner->ToPlayer()->GetPet())
                    AttackStart(ownerPet->GetVictim() ? ownerPet->GetVictim() : NULL);
                else
                {
                    if (Player* player = me->GetOwner() ? me->GetOwner()->ToPlayer() : NULL)
                        if (Unit* target = player->GetSelectedUnit())
                            if (target->IsHostileTo(owner))
                                AttackStart(target);
                }

                if (!targetFound)
                {
                    // Find victim of Summon Gargoyle spell
                    std::list<Unit*> targets;
                    Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(me, me, 30);
     
                    me->VisitAnyNearbyObject<Unit, Trinity::ContainerAction>(30, targets, u_check);
                    for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
                        if ((*iter)->GetAura(SPELL_DK_SUMMON_GARGOYLE_1, owner->GetGUID()))
                        {
                            me->Attack((*iter), false);
                            targetFound = true;
                            break;
                        }

                    if (!targetFound)
                        return;
                }

                if (!UpdateVictim() || interrupted || !targetFound)
                    return;

                if (me->GetVictim()->HasBreakableByDamageCrowdControlAura(me))
                {
                    me->InterruptNonMeleeSpells(false);
                    return;
                }

                if (CheckCast())
                    DoMeleeAttackIfReady();
                else if (!me->HasUnitState(UNIT_STATE_CASTING) && EnableSpellCast)
                    DoCastVictim(SPELL_DK_GARGOYLE_STRIKE);
            }

        private:
            bool interrupted;
            bool targetFound;
            bool EnableSpellCast;
            uint32 despawnTimer;
            uint32 spellCastTime;
            uint32 initStatTimer;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_dk_ebon_gargoyleAI(creature);
        }
};

class npc_pet_dk_dancing_rune_weapon : public CreatureScript
{
    public:
        npc_pet_dk_dancing_rune_weapon() : CreatureScript("npc_pet_dk_dancing_rune_weapon") { }
    
        struct npc_pet_dk_dancing_rune_weaponAI : public ScriptedAI
        {
        private:
            uint32 _swingTimer;
    
        public:
            npc_pet_dk_dancing_rune_weaponAI(Creature* creature) : ScriptedAI(creature)
            {
                _swingTimer = 3500;    // Ready to swing as soon as it spawns
            }
    
            void Reset() override
            {
                if (Unit* owner = me->GetOwner())
                {
                    // Apply auras if the death knight has them
                    if (owner->HasAura(59921))  // Frost Fever (passive)
                        me->AddAura(59921, me);
                    if (owner->HasAura(59879))  // Blood Plague (passive)
                        me->AddAura(59879, me);
                    if (owner->HasAura(59327))  // Glyph of Rune Tap
                        me->AddAura(59327, me);
    
                    // Synchronize weapon's health to get proper combat log healing effects
                    me->SetMaxHealth(owner->GetMaxHealth());
                    me->SetHealth(owner->GetMaxHealth());
    
                    // All threat is redirected to the death knight.
                    // The rune weapon should never be attacked by players and NPCs alike
                    me->SetRedirectThreat(owner->GetGUID(), 100);
                }
                else
                    me->DespawnOrUnsummon();
            }

            void AttackStart(Unit* who) override
            {
                if (who)
                    me->GetMotionMaster()->MoveChase(who);
            }
    
            void UpdateAI(uint32 diff) override
            {
                // TODO: chase target
                _swingTimer += diff;
                if (_swingTimer >= 3500)
                {
                    if (Unit* owner = me->GetOwner())
                    {
                        if (me->GetVictim() == NULL)
                            if (Unit* target = ObjectAccessor::GetUnit(*owner, owner->GetTarget()))
                                me->Attack(target, false);
                        if (me->GetVictim() == NULL) return;    // Prevent crash
    
                        CalcDamageInfo damageInfo;
                        owner->CalculateMeleeDamage(me->GetVictim(), 0, &damageInfo, BASE_ATTACK);
                        damageInfo.attacker = me;
                        damageInfo.damage = damageInfo.damage / 2;
    
                        me->DealDamageMods(me->GetVictim(), damageInfo.damage, &damageInfo.absorb);
                        me->SendAttackStateUpdate(&damageInfo);
                        me->ProcDamageAndSpell(damageInfo.target, damageInfo.procAttacker, damageInfo.procVictim, damageInfo.procEx, damageInfo.damage, damageInfo.attackType);
                        me->DealMeleeDamage(&damageInfo, true);
                    }
                    _swingTimer = 0;
                }
    
                // Do not call base to prevent weapon's own autoattack that deals less than 5 damage
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_dk_dancing_rune_weaponAI(creature);
        }
};

void AddSC_deathknight_pet_scripts()
{
    new npc_pet_dk_charmed_ghul();
    new npc_pet_dk_ebon_gargoyle();
    new npc_pet_dk_dancing_rune_weapon();
}
