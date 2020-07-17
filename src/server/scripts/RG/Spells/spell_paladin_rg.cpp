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
 * Scripts for spells with SPELLFAMILY_PALADIN and SPELLFAMILY_GENERIC spells used by paladin players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pal_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "Pet.h"
#include "Player.h"

enum PaladinSpells
{
    SPELL_IMPROVED_DEVOTION_AURA_RANK_1          = 20138,
    SPELL_IMPROVED_DEVOTION_AURA_RANK_2          = 20139,
    SPELL_IMPROVED_DEVOTION_AURA_RANK_3          = 20140,

    SPELL_PALADIN_AVENGING_WRATH_MARKER          = 61987,
    SPELL_PALADIN_HAND_OF_FREEDOM                = 1044,
    SPELL_PALADIN_TWO_HANDED_WEAPON              = 20111,
    SPELL_PALADIN_SEAL_OF_CORRUPTION             = 53736,
    SPELL_PALADIN_SEAL_OF_VENGEANCE              = 31801,
    SPELL_PALADIN_HOLY_VENGEANCE                 = 31803,
    SPELL_PALADIN_BLOOD_CORRUPTION               = 53742,
    SPELL_PALADIN_SEAL_OF_VENGEANCE_PROC         = 42463,
    SPELL_PALADIN_SEAL_OF_CORRUPTION_PROC        = 53739,

    // Misc
    SPELL_DRUID_CYCLONE                          = 33786,
    SPELL_TALENT_TITANS_GRIP                     = 46917,
    SPELL_TITANS_GRIP                            = 49152
};

class spell_pal_improved_devotion_aura_effect_rg : public SpellScriptLoader
{
    public:
        spell_pal_improved_devotion_aura_effect_rg() : SpellScriptLoader("spell_pal_improved_devotion_aura_effect_rg") {}
    
        class spell_pal_improved_devotion_aura_effect_rg_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_improved_devotion_aura_effect_rg_AuraScript);
    
            void HandleEffectCalcAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
            {
                int32 healBonusAmount = 0;
    
                if (Unit* owner = GetUnitOwner())
                {
                    if (owner->HasAura(SPELL_IMPROVED_DEVOTION_AURA_RANK_1, owner->GetGUID()))
                        healBonusAmount = 2;
                    else if (owner->HasAura(SPELL_IMPROVED_DEVOTION_AURA_RANK_2, owner->GetGUID()))
                        healBonusAmount = 4;
                    else if (owner->HasAura(SPELL_IMPROVED_DEVOTION_AURA_RANK_3, owner->GetGUID()))
                        healBonusAmount = 6;
                }
                amount = healBonusAmount;
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pal_improved_devotion_aura_effect_rg_AuraScript::HandleEffectCalcAmount, EFFECT_1, SPELL_AURA_MOD_HEALING_PCT);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_pal_improved_devotion_aura_effect_rg_AuraScript();
        }
};

class spell_pal_divine_shield : public SpellScriptLoader
{
    public:
        spell_pal_divine_shield() : SpellScriptLoader("spell_pal_divine_shield") { }
    
        class spell_pal_divine_shield_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_divine_shield_SpellScript);
    
            SpellCastResult CheckCast()
            {
                if (Unit* target = GetCaster())
                    if (target->HasAura(SPELL_PALADIN_AVENGING_WRATH_MARKER))
                        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
    
                return SPELL_CAST_OK;
            }
    
            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_pal_divine_shield_SpellScript::CheckCast);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_pal_divine_shield_SpellScript();
        }
};

// 1044 - Hand of Freedom
class spell_paladin_hand_of_freedom : public SpellScriptLoader
{
    public:
        spell_paladin_hand_of_freedom() : SpellScriptLoader("spell_paladin_hand_of_freedom") { }

        class spell_paladin_hand_of_freedom_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_paladin_hand_of_freedom_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                Unit* target = GetExplTargetUnit();

                // Nothing to do if we are in Cyclone
                if (target->HasAura(SPELL_DRUID_CYCLONE))
                    caster->GetSpellHistory()->AddCooldown(GetSpellInfo()->Id, 0, std::chrono::milliseconds(GetSpellInfo()->GetDuration()));

                if (caster->HasAura(SPELL_DRUID_CYCLONE))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                // Add Aura without Effect if we use it on Caster
                if (caster == target)
                    if (caster->HasAuraType(SPELL_AURA_MOD_STUN))
                        caster->AddAura(SPELL_PALADIN_HAND_OF_FREEDOM, caster);

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_paladin_hand_of_freedom_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_paladin_hand_of_freedom_SpellScript();
        }
};

// -53601 - Sacred Shield Proc
class spell_pal_sacred_shield_proc : public SpellScriptLoader
{
    public:
        spell_pal_sacred_shield_proc() : SpellScriptLoader("spell_pal_sacred_shield_proc") { }

        class spell_pal_sacred_shield_proc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_sacred_shield_proc_AuraScript);

            void Prepare(ProcEventInfo& /*eventInfo*/)
            {
                if (Pet* pet = GetTarget()->ToPet())
                    if (pet->ToPet()->DoneSpellProc())
                        PreventDefaultAction(); // will prevent charge drop and cooldown
            }

            bool CheckProc(ProcEventInfo& /*eventInfo*/)
            {
                if (Pet* pet = GetTarget()->ToPet())
                    if (pet->ToPet()->DoneSpellProc())
                        return false;

                return true;
            }

            void HandleProc(AuraEffect const* aurEff, ProcEventInfo& /*eventInfo*/)
            {
                /// @hack: due to currenct proc system implementation
                if (Pet* pet = GetTarget()->ToPet())
                    pet->SetDoneSpellProc();
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_pal_sacred_shield_proc_AuraScript::CheckProc);
                OnEffectProc += AuraEffectProcFn(spell_pal_sacred_shield_proc_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_pal_sacred_shield_proc_AuraScript();
        }
};

class spell_pal_seal_of_vengeance_corruption : public SpellScriptLoader
{
    public:
        spell_pal_seal_of_vengeance_corruption() : SpellScriptLoader("spell_pal_seal_of_vengeance_corruption") { }
    
        class spell_pal_seal_of_vengeance_corruption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_seal_of_vengeance_corruption_SpellScript);
    
            void RecalculateDamage()
            {
                if (Aura* aura = GetCaster()->GetAuraOfRankedSpell(SPELL_PALADIN_TWO_HANDED_WEAPON))
                {
                    int32 dmg = GetHitDamage();
                    AddPct(dmg, aura->GetSpellInfo()->GetBasePoints(EFFECT_0));
                    SetHitDamage(dmg);
                }
            }
    
            void Register() override
            {
                OnHit += SpellHitFn(spell_pal_seal_of_vengeance_corruption_SpellScript::RecalculateDamage);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_pal_seal_of_vengeance_corruption_SpellScript();
        }
};

// 68055 Judgements of the Just - Triggeredspells
class spell_pal_judgements_of_the_just : public SpellScriptLoader
{
    public:
        spell_pal_judgements_of_the_just() : SpellScriptLoader("spell_pal_judgements_of_the_just") { }
    
        class spell_pal_judgements_of_the_just_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_judgements_of_the_just_SpellScript);
    
            void HandleOnHit()
            {
                const uint32 sealDebuffSpellId = (GetCaster()->ToPlayer()->GetTeam() == ALLIANCE ? SPELL_PALADIN_HOLY_VENGEANCE : SPELL_PALADIN_BLOOD_CORRUPTION);

                if (Unit* target = GetHitUnit())
                    if (GetCaster()->HasAura(SPELL_PALADIN_SEAL_OF_CORRUPTION) || GetCaster()->HasAura(SPELL_PALADIN_SEAL_OF_VENGEANCE))
                        if (Aura* aur = target->GetAura(sealDebuffSpellId))
                        {
                            uint8 stackAmount = aur->GetStackAmount();
                            if (stackAmount >= 1)
                            {
                                uint32 sealProccSpellId = (GetCaster()->ToPlayer()->GetTeam() == ALLIANCE ? SPELL_PALADIN_SEAL_OF_VENGEANCE_PROC : SPELL_PALADIN_SEAL_OF_CORRUPTION_PROC);
                                // seal procc does x% of weapon damage, depending on the stackamount
                                const int32 percentWeaponDamage = (int32)(6.6f * stackAmount);
                                GetCaster()->CastCustomSpell(target, sealProccSpellId, &percentWeaponDamage, NULL, NULL, true);
                            }
                        }
            }
    
            void Register() override
            {
                OnHit += SpellHitFn(spell_pal_judgements_of_the_just_SpellScript::HandleOnHit);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_pal_judgements_of_the_just_SpellScript();
        }
};

// 19753 - Divine Intervention
class spell_pal_divine_intervention : public SpellScriptLoader
{
    public:
        spell_pal_divine_intervention() : SpellScriptLoader("spell_pal_divine_intervention") { }
    
        class spell_pal_divine_intervention_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_divine_intervention_AuraScript);
    
            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* target = GetTarget()->ToPlayer())
                    if (target->HasTalent(SPELL_TALENT_TITANS_GRIP, target->GetActiveSpec()))
                        target->AddAura(SPELL_TITANS_GRIP, target);
            }
    
            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_pal_divine_intervention_AuraScript::OnApply, EFFECT_1, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_pal_divine_intervention_AuraScript();
        }
};

// -1022 - Hand of Protection
class spell_pal_hand_of_protection : public SpellScriptLoader
{
public:
    spell_pal_hand_of_protection() : SpellScriptLoader("spell_pal_hand_of_protection") {}

    class spell_pal_hand_of_protection_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pal_hand_of_protection_SpellScript);

        SpellCastResult CheckCast()
        {
            for (auto&& effect : GetCaster()->GetAuraEffectsByType(SPELL_AURA_MOD_STUN))
            {
                const auto mechanic = effect->GetSpellInfo()->GetAllEffectsMechanicMask();
                if ((mechanic & (1 << MECHANIC_STUN)) != 0)
                    return SPELL_FAILED_STUNNED;
            }

            return SPELL_CAST_OK;
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_pal_hand_of_protection_SpellScript::CheckCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_pal_hand_of_protection_SpellScript();
    }
};

void AddSC_paladin_spell_scripts_rg()
{
    new spell_pal_improved_devotion_aura_effect_rg();
    new spell_pal_divine_shield();
    new spell_paladin_hand_of_freedom();
    new spell_pal_sacred_shield_proc();
    new spell_pal_seal_of_vengeance_corruption();
    new spell_pal_judgements_of_the_just();
    new spell_pal_divine_intervention();
    new spell_pal_hand_of_protection();
}
