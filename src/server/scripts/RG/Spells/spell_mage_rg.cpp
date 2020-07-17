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
 * Scripts for spells with SPELLFAMILY_MAGE and SPELLFAMILY_GENERIC spells used by mage players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_mage_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "SpellMgr.h"

enum MageSpells
{
    SPELL_MAGE_INVISIBILITY_EFFECT               = 32612,
    SPELL_MAGE_MOLTEN_SHIELDS_R1                 = 11094,
    SPELL_MAGE_MOLTEN_SHIELDS_R2                 = 13043,
    SPELL_HOT_STREAK                             = 48108,
    SPELL_MAGE_FINGERS_OF_FROST_R1               = 44543,
    SPELL_MAGE_FINGERS_OF_FROST_R2               = 44545,
    SPELL_MAGE_FINGERS_OF_FROST_PROC             = 44544,
    SPELL_MAGE_FROSTBITE_R1                      = 11071,
    SPELL_MAGE_FROSTBITE_R2                      = 12496,
    SPELL_MAGE_FROSTBITE_R3                      = 12497,
    SPELL_MAGE_IMPOVED_BLIZZARD_R1               = 11185,
    SPELL_MAGE_IMPOVED_BLIZZARD_R2               = 12487,
    SPELL_MAGE_IMPOVED_BLIZZARD_R3               = 12488,
    SPELL_MAGE_EMPOWERED_FIREBALL_1              = 31656,
    SPELL_MAGE_EMPOWERED_FIREBALL_2              = 31657,
    SPELL_MAGE_EMPOWERED_FIREBALL_3              = 31658,
    SPELL_MAGE_CLEAR_CASTING                     = 12536,
    SPELL_MAGE_MISSILE_BARRAGE                   = 44401,
    SPELL_DK_ICEBOUND_FORTITUDE                  = 48792,
    SPELL_MAGE_T10_2P                            = 70752,
    SPELL_MAGE_T10_2P_PROC                       = 70753,

    SPELL_MAGE_ICON_BLIZZARD                     = 285
};

class spell_mage_deep_freeze : public SpellScriptLoader
{
    public:
        spell_mage_deep_freeze() : SpellScriptLoader("spell_mage_deep_freeze") { }

        enum Spells
        {
            SPELL_MAGE_DEEP_FREEZE_DMG = 71757,
        };

        class spell_mage_deep_freeze_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_deep_freeze_SpellScript)

            /* Not the right way ...
               Should be done with passive aura
               71761 Deep Freeze Immunity State */

            bool Validate(SpellInfo const * /*spellEntry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MAGE_DEEP_FREEZE_DMG))
                    return false;
                return true;
            }

            bool Load() override
            {
                _doDamage = false;
                return true;
            }

            SpellCastResult _checkCast()
            {
                Unit * target = GetExplTargetUnit();
                if (!target || target->GetTypeId() != TYPEID_UNIT)
                    return SPELL_CAST_OK;

                if (!target->IsCharmedOwnedByPlayerOrPlayer() && \
                    target->IsImmunedToSpellEffect(GetSpellInfo(), EFFECT_0))
                        _doDamage = true;

                return SPELL_CAST_OK;
            }

            void _onEffect(SpellEffIndex effIndex)
            {
                // FIXME: doesn't work with auras, immune still in combat log
                if (_doDamage)
                    PreventHitDefaultEffect(effIndex);
            }

            void _onHit()
            {
                if (_doDamage)
                    if (Unit* target = GetHitUnit())
                        if (!target->HasAura(SPELL_DK_ICEBOUND_FORTITUDE))
                            GetCaster()->CastSpell(target, SPELL_MAGE_DEEP_FREEZE_DMG, true);
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_mage_deep_freeze_SpellScript::_checkCast);
                OnEffectHitTarget += SpellEffectFn(spell_mage_deep_freeze_SpellScript::_onEffect, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
                OnHit += SpellHitFn(spell_mage_deep_freeze_SpellScript::_onHit);
            }

        private:
            bool _doDamage;
        };

        SpellScript * GetSpellScript() const override
        {
            return new spell_mage_deep_freeze_SpellScript();
        }
};

class spell_mage_deep_freeze_immunity_state : public SpellScriptLoader
{
    public:
        spell_mage_deep_freeze_immunity_state() : SpellScriptLoader("spell_mage_deep_freeze_immunity_state") { }

        class spell_mage_deep_freeze_immunity_state_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_deep_freeze_immunity_state_AuraScript);

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();
            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_mage_deep_freeze_immunity_state_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_deep_freeze_immunity_state_AuraScript();
        }
};

class DalaranBrillianceTargetSelector
{
    public:
        bool operator() (WorldObject* object)
        {
            if (Unit* unit = object->ToUnit())
                if (unit->getLevel() >= 75)
                    return false;
    
            return true;
        }
};

class spell_dalaran_brilliance : public SpellScriptLoader
{
    public:
        spell_dalaran_brilliance() : SpellScriptLoader("spell_dalaran_brilliance") { }
    
        class spell_dalaran_brilliance_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dalaran_brilliance_SpellScript);
    
            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(DalaranBrillianceTargetSelector());
            }
    
            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_dalaran_brilliance_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_CASTER_AREA_RAID);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_dalaran_brilliance_SpellScript();
        }
};

class spell_mage_invisibility : public SpellScriptLoader
{
    public:
        spell_mage_invisibility() : SpellScriptLoader("spell_mage_invisibility") { }
    
        class spell_mage_invisibility_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_invisibility_AuraScript);
    
            void ReduceThreat(Unit* who, float pct)
            {
                const Unit::AttackerSet attackers = who->getAttackers();
                for (Unit::AttackerSet::const_iterator itr = attackers.begin(); itr != attackers.end(); ++itr)
                    (*itr)->GetThreatManager().ModifyThreatByPercent(GetUnitOwner(), pct);
            }
    
            void ReduceAggro(AuraEffect const* /*aurEff*/)
            {
                ReduceThreat(GetUnitOwner(), -50.0f);
            }
    
            void Fade(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                    return;
    
                Unit* target = GetTarget();
                target->CastSpell(target, SPELL_MAGE_INVISIBILITY_EFFECT, true, NULL, aurEff);
    
                if (Player* player = target->ToPlayer())
                    if (player->AttendsWorldBossFight())
                    {
                        player->AttackStop();
                        player->SendAttackSwingCancelAttack();
    
                        ReduceThreat(player, -100.0f);
                        return;
                    }
    
                target->CombatStop();
            }
    
            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_mage_invisibility_AuraScript::Fade, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_invisibility_AuraScript::ReduceAggro, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_invisibility_AuraScript();
        }
};

class spell_mage_iceblock : public SpellScriptLoader
{
    public:
        spell_mage_iceblock() : SpellScriptLoader("spell_mage_iceblock") { }
    
        class spell_mage_iceblock_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_iceblock_AuraScript);
    
            bool Load() override
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                        return true;
                return false;
            }
    
            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    caster->RemoveAurasDueToSpell(770);    // Faerie Fire
                    caster->RemoveAurasDueToSpell(16857);  // Faerie Fire (Feral)
                    caster->RemoveAurasDueToSpell(44457);  // Living Bomb
                    caster->RemoveAurasDueToSpell(408);    // Kidney Shot
                    caster->RemoveAurasDueToSpell(8643);   // Kidney Shot
                    caster->RemoveAurasDueToSpell(853);    // Hammer of Justice
                    caster->RemoveAurasDueToSpell(5588);   // Hammer of Justice
                    caster->RemoveAurasDueToSpell(5589);   // Hammer of Justice
                    caster->RemoveAurasDueToSpell(10308);  // Hammer of Justice
                }
            }
    
            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_mage_iceblock_AuraScript::HandleOnEffectApply, EFFECT_1, SPELL_AURA_SCHOOL_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
                OnEffectApply += AuraEffectApplyFn(spell_mage_iceblock_AuraScript::HandleOnEffectApply, EFFECT_2, SPELL_AURA_SCHOOL_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_iceblock_AuraScript();
        }
};

// 31661 - Dragons Breath
class spell_mage_dragons_breath : public SpellScriptLoader
{
    public:
        spell_mage_dragons_breath() : SpellScriptLoader("spell_mage_dragons_breath") { }

        class spell_mage_dragons_breath_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_dragons_breath_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                // Don't proc on Living Bomb explode damage
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->SpellFamilyName == SPELLFAMILY_MAGE && spellInfo->SpellFamilyFlags[1] & 0x10000)
                        return false;
                return true;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_mage_dragons_breath_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_dragons_breath_AuraScript();
        }
};

class spell_mage_molten_armor : public SpellScriptLoader
{
    public:
        spell_mage_molten_armor() : SpellScriptLoader("spell_mage_molten_armor") { }

        class spell_mage_molten_armor_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_molten_armor_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                return sSpellMgr->GetSpellInfo(SPELL_MAGE_MOLTEN_SHIELDS_R1) && sSpellMgr->GetSpellInfo(SPELL_MAGE_MOLTEN_SHIELDS_R2);
            }

            bool Load() override
            {
                chance = 0;
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(SPELL_MAGE_MOLTEN_SHIELDS_R1))
                        chance = 50;
                    else if (caster->HasAura(SPELL_MAGE_MOLTEN_SHIELDS_R2))
                        chance = 100;
                }
                return true;
            }

            bool Check(ProcEventInfo& eventInfo)
            {
                bool canProc = false;
                switch (eventInfo.GetDamageInfo()->GetAttackType())
                {
                    case RANGED_ATTACK:
                        canProc = roll_chance_i(chance);
                        break;
                    case BASE_ATTACK:
                    case OFF_ATTACK:
                        canProc = !eventInfo.GetSpellInfo();
                        break;
                    default:
                        break;
                }
                if (eventInfo.GetSpellInfo())
                    switch (eventInfo.GetSpellInfo()->DmgClass)
                    {
                        case SPELL_DAMAGE_CLASS_MELEE:
                            canProc = true;
                            break;
                        case SPELL_DAMAGE_CLASS_MAGIC:
                        case SPELL_DAMAGE_CLASS_RANGED:
                            canProc = roll_chance_i(chance);
                            break;
                        default:
                            canProc = false;
                    }

                return canProc;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_mage_molten_armor_AuraScript::Check);
            }

        private:
            int32 chance;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_molten_armor_AuraScript();
        }
};

class spell_mage_pyroblast: public SpellScriptLoader
{
    public:
        spell_mage_pyroblast() : SpellScriptLoader("spell_mage_pyroblast") { }

        class spell_mage_pyroblast_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_pyroblast_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (GetCaster()->HasAura(SPELL_MAGE_EMPOWERED_FIREBALL_3))
                    {
                        // +15.00% from sp bonus
                        float bonus = 0.15f;
                        bonus *= caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask());
                        bonus = caster->ApplyEffectModifiers(GetSpellInfo(), aurEff->GetEffIndex(), bonus);
                        amount += int32(bonus);
                    }
                    else if (GetCaster()->HasAura(SPELL_MAGE_EMPOWERED_FIREBALL_2))
                    {
                        // +10.00% from sp bonus
                        float bonus = 0.10f;
                        bonus *= caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask());
                        bonus = caster->ApplyEffectModifiers(GetSpellInfo(), aurEff->GetEffIndex(), bonus);
                        amount += int32(bonus);
                    }
                    else if (GetCaster()->HasAura(SPELL_MAGE_EMPOWERED_FIREBALL_1))
                    {
                        // +5.00% from sp bonus
                        float bonus = 0.05f;
                        bonus *= caster->SpellBaseDamageBonusDone(GetSpellInfo()->GetSchoolMask());
                        bonus = caster->ApplyEffectModifiers(GetSpellInfo(), aurEff->GetEffIndex(), bonus);
                        amount += int32(bonus);
                    }
                }
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_pyroblast_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };
        
        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_pyroblast_AuraScript();
        }
};

class spell_mage_frostbite : public SpellScriptLoader
{
    public:
        spell_mage_frostbite() : SpellScriptLoader("spell_mage_frostbite") {}

        class spell_mage_frostbite_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_frostbite_SpellScript)

            void HandleOnCast() 
            {
                if (!GetCaster() || !GetCaster()->ToPlayer())
                    return;

                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(SPELL_MAGE_FINGERS_OF_FROST_R1))
                        caster->AddAura(SPELL_MAGE_FINGERS_OF_FROST_PROC, caster);
                    else if (caster->HasAura(SPELL_MAGE_FINGERS_OF_FROST_R2))
                        caster->AddAura(SPELL_MAGE_FINGERS_OF_FROST_PROC, caster);
                }
            }

            void Register() override
            {
                OnCast += SpellCastFn(spell_mage_frostbite_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_mage_frostbite_SpellScript();
        }
};

class spell_mage_fingers_of_frost : public SpellScriptLoader
{
    public:
        spell_mage_fingers_of_frost() : SpellScriptLoader("spell_mage_fingers_of_frost") { }

        class spell_mage_fingers_of_frost_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_fingers_of_frost_AuraScript);

            bool Check(ProcEventInfo& /*eventInfo*/)
            {
                // Don't proc if we have Aura Frostbite
                if (Unit* target = GetTarget())
                { 
                    if (target->HasAura(SPELL_MAGE_FROSTBITE_R1))
                        return false;
                    else if (target->HasAura(SPELL_MAGE_FROSTBITE_R2))
                        return false;
                    else if (target->HasAura(SPELL_MAGE_FROSTBITE_R3))
                        return false;
                }
                return true;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_mage_fingers_of_frost_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_mage_fingers_of_frost_AuraScript();
        }
};

// -44546 - Brain Freeze
class spell_mage_brain_freeze : public SpellScriptLoader
{
    public:
        spell_mage_brain_freeze() : SpellScriptLoader("spell_mage_brain_freeze") { }

        class spell_mage_brain_freeze_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_brain_freeze_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->SpellIconID == SPELL_MAGE_ICON_BLIZZARD)
                        if (!GetTarget()->HasAura(SPELL_MAGE_IMPOVED_BLIZZARD_R1) && !GetTarget()->HasAura(SPELL_MAGE_IMPOVED_BLIZZARD_R2) && !GetTarget()->HasAura(SPELL_MAGE_IMPOVED_BLIZZARD_R3))
                            return false;          
                return true;
            }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_mage_brain_freeze_AuraScript::Check);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_brain_freeze_AuraScript();
    }
};

// 70907, 70908 - Water Elemental
class spell_summon_water_elemental : public SpellScriptLoader
{
    public:
        spell_summon_water_elemental() : SpellScriptLoader("spell_summon_water_elemental") { }

        class spell_summon_water_elemental_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_summon_water_elemental_SpellScript);

            void HandleBeforeCast()
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetPetGUID())
                        if (caster->GetGuardianPet())
                            caster->GetGuardianPet()->DespawnOrUnsummon();
            }

            void Register() override
            {
                BeforeCast += SpellCastFn(spell_summon_water_elemental_SpellScript::HandleBeforeCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_summon_water_elemental_SpellScript();
        }
};

// - 7268 - Arcane Missile (Triggered Spell)
class spell_mage_arcane_missile : public SpellScriptLoader
{
    public:
        spell_mage_arcane_missile() : SpellScriptLoader("spell_mage_arcane_missile") { }

        class spell_mage_arcane_missile_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_arcane_missile_SpellScript);

            SpellCastResult CheckCast()
            {
                if (Unit* caster = GetCaster())
                    if (Unit* target = GetExplTargetUnit())
                        if (target == caster)
                            return SPELL_FAILED_BAD_TARGETS;
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_mage_arcane_missile_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_mage_arcane_missile_SpellScript();
        }
};

// -5143 - Arcane Missiles (Channeled Spell)
class spell_mage_arcane_missile_channel : public SpellScriptLoader
{
public:
    spell_mage_arcane_missile_channel() : SpellScriptLoader("spell_mage_arcane_missile_channel") { }

    class spell_mage_arcane_missile_channel_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_mage_arcane_missile_channel_SpellScript);

        // Remove only missile barrage if missile barrage and clearcasting are active
        void RestoreClearCastIfMissileBarrageIsPresent()
        {
            Spell* arcaneMissles = GetSpell();
            std::set<Aura*> const appliedMods = arcaneMissles->GetUsedModAuras();
            Aura* clearCasting = NULL;
            Aura* missileBarrage = NULL;
            for (std::set<Aura*>::const_iterator modIter = appliedMods.begin(); modIter != appliedMods.end(); modIter++)
            {
                uint32 id = (*modIter)->GetSpellInfo()->Id;
                if (id == SPELL_MAGE_CLEAR_CASTING) 
                    clearCasting = (*modIter);
                if (id == SPELL_MAGE_MISSILE_BARRAGE)
                    missileBarrage = (*modIter);
            }
            if (clearCasting && missileBarrage)
                arcaneMissles->RestoreUsedModAuras(SPELL_MAGE_CLEAR_CASTING, clearCasting);
        }

        void Register() override
        {
            BeforeCast += SpellCastFn(spell_mage_arcane_missile_channel_SpellScript::RestoreClearCastIfMissileBarrageIsPresent);
        }

    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_mage_arcane_missile_channel_SpellScript();
    }

    class spell_mage_arcane_missile_channel_AuraScript : public AuraScript
    {
    public:
        PrepareAuraScript(spell_mage_arcane_missile_channel_AuraScript);

        void RemoveArcaneMissile(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE || GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_CANCEL)
            {
                Unit* caster = GetCaster();
                if (caster && missileBarrageApplied && caster->HasAura(SPELL_MAGE_T10_2P))
                    caster->CastSpell(caster, SPELL_MAGE_T10_2P_PROC, true);
            }
        }

        void ApplyArcaneMissile(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (Spell* arcaneMissles = caster->FindCurrentSpellBySpellId(aurEff->GetSpellInfo()->Id))
                {
                    std::set<Aura*> const appliedMods = arcaneMissles->GetUsedModAuras();
                    for (std::set<Aura*>::const_iterator modIter = appliedMods.begin(); modIter != appliedMods.end(); modIter++)
                    {
                        if ((*modIter)->GetSpellInfo()->Id == SPELL_MAGE_MISSILE_BARRAGE)
                            missileBarrageApplied = true;
                    }
                }
            }
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(spell_mage_arcane_missile_channel_AuraScript::ApplyArcaneMissile, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_mage_arcane_missile_channel_AuraScript::RemoveArcaneMissile, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }

        bool missileBarrageApplied = false;
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_arcane_missile_channel_AuraScript();
    }
};

void AddSC_mage_spell_scripts_rg()
{
    new spell_mage_deep_freeze;
    new spell_mage_deep_freeze_immunity_state();
    new spell_dalaran_brilliance();
    new spell_mage_invisibility();
    new spell_mage_iceblock();
    new spell_mage_dragons_breath();
    new spell_mage_molten_armor();
    new spell_mage_pyroblast();
    new spell_mage_frostbite();
    new spell_mage_fingers_of_frost();
    new spell_mage_brain_freeze();
    new spell_summon_water_elemental();
    new spell_mage_arcane_missile();
    new spell_mage_arcane_missile_channel();
}
