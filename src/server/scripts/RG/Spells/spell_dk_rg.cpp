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

/*
 * Scripts for spells with SPELLFAMILY_DEATHKNIGHT and SPELLFAMILY_GENERIC spells used by deathknight players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_dk_".
 */

#include "Pet.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Player.h"

enum DeathKnightSpells
{
    SPELL_DK_DEATH_GRIP         = 49560,
    SPELL_OVERPOWERING_SICKNESS = 36809,
    SPELL_RAISE_ALLY_DUMMY      = 61999,
    SPELL_RAISE_ALLY            = 46619,
    SPELL_DK_BLOOD_PLAGUE       = 55078,
    SPELL_DK_FROST_FEVER        = 55095,
    SPELL_DK_GLYPH_OF_DISEASE   = 63334,
};

// 51963 - Gargoyle Strike
class spell_dk_gargoyle_strike : public SpellScriptLoader
{
    public:
        spell_dk_gargoyle_strike() : SpellScriptLoader("spell_dk_gargoyle_strike") { }

        class spell_dk_gargoyle_strike_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dk_gargoyle_strike_SpellScript);

            bool Load() override
            {
                damage = 0;

                if (GetCaster() && GetCaster()->GetEntry() == NPC_EBON_GARGOYLE)
                    return true;
                return false;
            }

            void HandleDummyHitTarget(SpellEffIndex /*effIndex*/)
            {
                if (SpellInfo const* info = GetSpellInfo())
                {
                    damage += info->Effects[0].BasePoints;
                    damage += urand(0, info->Effects[0].DieSides);
                    damage += (GetCaster()->getLevel() - 60) * 4 + 60;
                }

                Unit* target = GetHitUnit();
                Unit* caster = GetCaster();
                if (!target || !caster)
                    return;
                Unit* owner = caster->GetOwner();
                if (!owner)
                    return;

                damage += (owner->GetTotalAttackPowerValue(BASE_ATTACK) * 0.4f);
                damage *= target->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, GetSpellInfo()->GetSchoolMask());

                SetHitDamage(damage);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_dk_gargoyle_strike_SpellScript::HandleDummyHitTarget, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }

        private:
            int32 damage;

        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_dk_gargoyle_strike_SpellScript();
        }
};

// 43263 Taunt from Army of the dead
class spell_dk_ghul_taunt : public SpellScriptLoader
{
    public:
        spell_dk_ghul_taunt() : SpellScriptLoader("spell_dk_ghul_taunt") { }

        class spell_dk_ghul_taunt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dk_ghul_taunt_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targetList)
            {
                targetList.remove_if([](WorldObject* object)
                {
                    return (object->ToCreature() && object->ToCreature()->isWorldBoss()) || object->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                });
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dk_ghul_taunt_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dk_ghul_taunt_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_dk_ghul_taunt_SpellScript();
        }
};

class spell_dk_raise_ally_dummy : public SpellScriptLoader
{
    public:
        spell_dk_raise_ally_dummy() : SpellScriptLoader("spell_dk_raise_ally_dummy") { }

        class spell_dk_raise_ally_dummySpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dk_raise_ally_dummySpellScript);

            bool Load() override
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                        return true;
                return false;
            }

            bool Validate(SpellInfo const* /*spellEntry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_RAISE_ALLY))
                    return false;
                return true;
            }

            SpellCastResult CheckCast()
            {
                if (Unit* target = GetExplTargetUnit())
                {
                    if (target->IsAlive())
                        return SPELL_FAILED_TARGET_NOT_DEAD;
                    else if (target->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_TARGET_NOT_PLAYER;
                    else if (target->HasAura(SPELL_RAISE_ALLY))
                        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                }
                return SPELL_CAST_OK;
            }

            void HandleOnHitDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                    target->CastSpell(target, SPELL_RAISE_ALLY, true);
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_dk_raise_ally_dummySpellScript::CheckCast);
                OnEffectHitTarget += SpellEffectFn(spell_dk_raise_ally_dummySpellScript::HandleOnHitDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_dk_raise_ally_dummySpellScript();
        }
};

enum RaiseAllyAura
{
    ENTRY_RISEN_ALLY = 30230
};

class spell_dk_raise_ally_aura : public SpellScriptLoader
{
    public:
        spell_dk_raise_ally_aura() : SpellScriptLoader("spell_dk_raise_ally_aura") { }

        class spell_dk_raise_ally_auraAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_raise_ally_auraAuraScript);

            bool Load() override
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                        return true;
                return false;
            }

            void HandleAfterEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (Creature* ghul = target->FindNearestCreature(ENTRY_RISEN_ALLY, 2.0f))
                {
                    float BaseHealth = 7500.0f;
                    float Mod = target->getLevel() * 100.0f;
                    float BasePoints = BaseHealth + Mod;

                    ghul->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, Mod/10);
                    ghul->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, Mod/5);
                    ghul->UpdateDamagePhysical(BASE_ATTACK);

                    ghul->SetMaxHealth(BasePoints);
                    ghul->SetHealth(BasePoints);
                }
            }

            void Register() override
            {
                AfterEffectApply += AuraEffectApplyFn(spell_dk_raise_ally_auraAuraScript::HandleAfterEffectApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_raise_ally_auraAuraScript();
        }
};

class spell_dk_summon_gargoyle : public SpellScriptLoader
{
    public:
        spell_dk_summon_gargoyle() : SpellScriptLoader("spell_dk_summon_gargoyle") { }

        class spell_dk_summon_gargoyle_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dk_summon_gargoyle_SpellScript);

            bool Load() override
            {
                offset.m_positionX = 0.0f;
                offset.m_positionY = 0.0f;
                offset.m_positionZ = 0.0f;
                offset.m_orientation = 0.0f;
                return true;
            }

            void HandleHitTarget(SpellEffIndex /*effIndex*/)
            {
                WorldLocation summonPos = *GetExplTargetDest();
                summonPos.RelocateOffset(offset);
                SetExplTargetDest(summonPos);
                GetHitDest()->RelocateOffset(offset);
            }

            void SetDest(SpellDestination& dest)
            {
                // Adjust effect summon position
                if (GetCaster()->IsWithinLOS(dest._position.GetPositionX(), dest._position.GetPositionY(), dest._position.GetPositionZ() + 15.0f))
                    dest._position.m_positionZ += 15.0f;
            }

            void HandleLaunchTarget(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetExplTargetUnit())
                {
                    if (!GetCaster()->isInFront(target, 2.5f) && GetCaster()->IsWithinMeleeRange(target))
                    {
                        float o = GetCaster()->GetOrientation();
                        offset.m_positionX = (7 * cos(o)) + target->GetMeleeRange(target);
                        offset.m_positionY = (7 * sin(o)) + target->GetMeleeRange(target);
                    }
                }
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_dk_summon_gargoyle_SpellScript::HandleHitTarget, EFFECT_0, SPELL_EFFECT_SUMMON);
                OnDestinationTargetSelect += SpellDestinationTargetSelectFn(spell_dk_summon_gargoyle_SpellScript::SetDest, EFFECT_0, TARGET_DEST_CASTER_FRONT_LEFT);
                OnEffectLaunchTarget += SpellEffectFn(spell_dk_summon_gargoyle_SpellScript::HandleLaunchTarget, EFFECT_1, SPELL_EFFECT_APPLY_AURA);
            }

        private:
            Position offset;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_dk_summon_gargoyle_SpellScript();
        }
};

class spell_dk_death_and_decay_damage : public SpellScriptLoader
{
    public:
        spell_dk_death_and_decay_damage() : SpellScriptLoader("spell_dk_death_and_decay_damage") { }
    
        class spell_dk_death_and_decay_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dk_death_and_decay_damage_SpellScript);
    
            void RecalculateDamage()
            {
                if (Aura *aura = GetCaster()->GetAura(70650)) // Death Knight T10 Tank 2P Bonus
                {
                    int32 dmg = GetHitDamage();
                    AddPct(dmg, aura->GetSpellInfo()->GetBasePoints(EFFECT_0));
                    SetHitDamage(dmg);
                }
            }
    
            void Register() override
            {
                OnHit += SpellHitFn(spell_dk_death_and_decay_damage_SpellScript::RecalculateDamage);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_dk_death_and_decay_damage_SpellScript();
        }
};

// -49222 - Bone Shield
class spell_dk_bone_shield : public SpellScriptLoader
{
    public:
        spell_dk_bone_shield() : SpellScriptLoader("spell_dk_bone_shield") { }
    
        class spell_dk_bone_shield_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_bone_shield_AuraScript);
    
            bool Check(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                { 
                    if (spellInfo->Id == 66240) // Anub'arak leeching swarm
                        return false;
                    if (spellInfo->IsTargetingArea()) // AoE Effects
                        return false;
                }
    
                return true;
            }
    
            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_dk_bone_shield_AuraScript::Check);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_bone_shield_AuraScript();
        }
};

// -53386 - Cinderglacier
class spell_dk_cinderglacier : public SpellScriptLoader
{
    public:
        spell_dk_cinderglacier() : SpellScriptLoader("spell_dk_cinderglacier") { }

        class spell_dk_cinderglacier_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_cinderglacier_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->Id == 51460) // Necrosis
                        return false;
                return true;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_dk_cinderglacier_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_cinderglacier_AuraScript();
        }
};

// 49576 - Death Grip Initial
class spell_dk_death_grip_initial : public SpellScriptLoader
{
    public:
        spell_dk_death_grip_initial() : SpellScriptLoader("spell_dk_death_grip_initial") { }

        class spell_dk_death_grip_initial_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dk_death_grip_initial_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                // Death Grip should not be castable while jumping/falling
                if (caster->HasUnitState(UNIT_STATE_JUMPING) || caster->HasUnitMovementFlag(MOVEMENTFLAG_FALLING))
                    return SPELL_FAILED_MOVING;

                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* victim = GetHitUnit();

                if (caster == victim)
                    caster = GetExplTargetUnit();

                caster->CastSpell(victim, SPELL_DK_DEATH_GRIP, true);
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_dk_death_grip_initial_SpellScript::CheckCast);
                OnEffectHitTarget += SpellEffectFn(spell_dk_death_grip_initial_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_dk_death_grip_initial_SpellScript();
        }
};

// 45524 - Chains of Ice
class spell_dk_chains_of_ice : public SpellScriptLoader
{
    public:
        spell_dk_chains_of_ice() : SpellScriptLoader("spell_dk_chains_of_ice") { }

        class spell_dk_chains_of_ice_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_chains_of_ice_AuraScript);

            void HandlePeriodic(AuraEffect* aurEff)
            {
                // Get 0 effect aura
                if (AuraEffect* slow = GetAura()->GetEffect(0))
                {
                    int32 newAmount = slow->GetAmount() + aurEff->GetAmount();
                    if (newAmount > 0)
                        newAmount = 0;
                    slow->ChangeAmount(newAmount);
                }
            }

            void Register() override
            {
                OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_dk_chains_of_ice_AuraScript::HandlePeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };
        
        class spell_dk_chains_of_ice_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dk_chains_of_ice_SpellScript);

            void RemoveChains()
            {
                if (Unit* target = GetHitUnit())
                    target->RemoveAura(GetSpellInfo()->Id);
            }

            void Register() override
            {
                BeforeHit += SpellHitFn(spell_dk_chains_of_ice_SpellScript::RemoveChains);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_chains_of_ice_AuraScript();
        }

        SpellScript* GetSpellScript() const override
        {
            return new spell_dk_chains_of_ice_SpellScript();
        }
};

// 63611 - Improved Blood Presence
class spell_dk_improved_blood_presence_proc : public SpellScriptLoader
{
    public:
        spell_dk_improved_blood_presence_proc() : SpellScriptLoader("spell_dk_improved_blood_presence_proc") { }

        class spell_dk_improved_blood_presence_proc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_improved_blood_presence_proc_AuraScript);

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->Id == SPELL_OVERPOWERING_SICKNESS)
                        return false;

                return eventInfo.GetDamageInfo()->GetDamage();
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_dk_improved_blood_presence_proc_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_improved_blood_presence_proc_AuraScript();
        }
};

// -49004 - Scent of Blood
class spell_dk_scent_of_blood_trigger : public SpellScriptLoader
{
    public:
        spell_dk_scent_of_blood_trigger() : SpellScriptLoader("spell_dk_scent_of_blood_trigger") { }

        class spell_dk_scent_of_blood_trigger_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_scent_of_blood_trigger_AuraScript);

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                return (eventInfo.GetHitMask() & (PROC_EX_DODGE | PROC_EX_PARRY)) || eventInfo.GetDamageInfo()->GetDamage();
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_dk_scent_of_blood_trigger_AuraScript::CheckProc);
            }
        };
        
        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_scent_of_blood_trigger_AuraScript();
        }
};

class spell_dk_pestilence_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_dk_pestilence_SpellScript);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        if (!sSpellMgr->GetSpellInfo(SPELL_DK_BLOOD_PLAGUE) ||
            !sSpellMgr->GetSpellInfo(SPELL_DK_FROST_FEVER) ||
            !sSpellMgr->GetSpellInfo(SPELL_DK_GLYPH_OF_DISEASE))
            return false;
        return true;
    }

    void OnHit(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* hitUnit = GetHitUnit();
        Unit* victim = GetExplTargetUnit();

        if (!victim)
            return;

        // Get diseases on target of spell (spread on new targets)
        if (hitUnit != victim)
        {
            // And spread them on target
            // Blood Plague
            if (victim->HasAura(SPELL_DK_BLOOD_PLAGUE, caster->GetGUID()))
                caster->CastSpell(hitUnit, SPELL_DK_BLOOD_PLAGUE, true);
            // Frost Fever
            if (victim->HasAura(SPELL_DK_FROST_FEVER, caster->GetGUID()))
                caster->CastSpell(hitUnit, SPELL_DK_FROST_FEVER, true);
        }
        // Glyph of Disease - cast on unit target too to refresh aura
        else if (caster->HasAura(SPELL_DK_GLYPH_OF_DISEASE) && hitUnit == victim)
        {
            // Blood Plague
            if (Aura* currentBloodPlague = victim->GetAura(SPELL_DK_BLOOD_PLAGUE, caster->GetGUID()))
                caster->CastSoftRefreshSpell(hitUnit, currentBloodPlague->GetSpellInfo());
            // Frost Fever
            if (Aura* currentFrostFever = victim->GetAura(SPELL_DK_FROST_FEVER, caster->GetGUID()))
                caster->CastSoftRefreshSpell(hitUnit, currentFrostFever->GetSpellInfo());
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_dk_pestilence_SpellScript::OnHit, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

void AddSC_deathknight_spell_scripts_rg()
{
    new spell_dk_bone_shield();
    new spell_dk_cinderglacier();
    new spell_dk_gargoyle_strike();
    new spell_dk_ghul_taunt();
    new spell_dk_raise_ally_dummy();
    new spell_dk_raise_ally_aura();
    new spell_dk_summon_gargoyle();
    new spell_dk_death_and_decay_damage();
    new spell_dk_death_grip_initial();
    new spell_dk_chains_of_ice();
    new spell_dk_improved_blood_presence_proc();
    new spell_dk_scent_of_blood_trigger();
    new SpellScriptLoaderEx<spell_dk_pestilence_SpellScript>("spell_dk_pestilence");
}
