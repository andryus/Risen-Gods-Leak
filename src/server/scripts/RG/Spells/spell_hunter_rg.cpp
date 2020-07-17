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
 * Scripts for spells with SPELLFAMILY_HUNTER, SPELLFAMILY_PET and SPELLFAMILY_GENERIC spells used by hunter players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_hun_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Pet.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellHistory.h"

enum HunterSpells
{
    IMPROVED_MEND_PET_DISPEL                        = 24406,
    IMPROVED_MEND_PET_R1                            = 19572,
    IMPROVED_MEND_PET_R2                            = 19573,
    SPELL_HUNTER_FEIGN_DEATH                        = 5384,
    SPELL_READINESS_MARK_FOR_FEIGN_DEATH            = 100008,
    SPELL_THE_BEAST_WITHIN                          = 70029,
    SPELL_HUNTER_PIERCING_SHOTS                     = 63468,
    SPELL_HUNTER_COBRA_STRIKES                      = 53257
};

enum HunterSpellIcons
{
    SPELL_ICON_FROST_AURA                           = 976,
    SPELL_ICON_BACKLASH                             = 2281,
    SPELL_ICON_SAVAGES_REND                         = 245
};

// 64860 - Item - Hunter T8 4P Bonus
class spell_hun_item_t8_4p_bonus : public SpellScriptLoader
{
    public:
        spell_hun_item_t8_4p_bonus() : SpellScriptLoader("spell_hun_item_t8_4p_bonus") { }

        class spell_hun_item_t8_4p_bonus_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_item_t8_4p_bonus_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                //! only proc on steady shot
                return eventInfo.GetDamageInfo()->GetSpellInfo()->SpellFamilyName == SPELLFAMILY_HUNTER && eventInfo.GetDamageInfo()->GetSpellInfo()->SpellIconID == 2228;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_hun_item_t8_4p_bonus_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_item_t8_4p_bonus_AuraScript();
        }
};

class spell_hun_masters_call_aura : public SpellScriptLoader
{
    public:
        spell_hun_masters_call_aura() : SpellScriptLoader("spell_hun_masters_call_aura") { }
    
        class spell_hun_masters_call_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_masters_call_aura_AuraScript);
    
            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->RemoveAurasByType(SPELL_AURA_MOD_ROOT);
                target->RemoveAurasWithMechanic((1 << MECHANIC_ROOT) | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_SNARE));
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
            }
    
            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, false);
            }
    
            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_hun_masters_call_aura_AuraScript::HandleOnEffectApply, EFFECT_1, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_hun_masters_call_aura_AuraScript::HandleOnEffectRemove, EFFECT_1, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_masters_call_aura_AuraScript();
        }
};

class spell_hun_pet_heal : public SpellScriptLoader
{
    public:
        spell_hun_pet_heal() : SpellScriptLoader("spell_hun_pet_heal") { }
    
        class spell_hun_pet_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_pet_heal_SpellScript);
    
            SpellCastResult CheckCast()
            {
                if (Pet* pet = GetCaster()->ToPlayer()->GetPet())
                    if (pet->isDead())
                        return SPELL_FAILED_NO_PET;
    
                return SPELL_CAST_OK;
            }
    
            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_hun_pet_heal_SpellScript::CheckCast);
            }
        };
    
        class spell_hun_pet_healAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_pet_healAuraScript);
    
            bool Validate(SpellInfo const* /*spellEntry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(IMPROVED_MEND_PET_R1) || !sSpellMgr->GetSpellInfo(IMPROVED_MEND_PET_R2))
                    return false;
                return true;
            }
    
            bool Load() override
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(IMPROVED_MEND_PET_R1))
                        chance = 25;
                    else if (caster->HasAura(IMPROVED_MEND_PET_R2))
                        chance = 50;
                    return true;
                }
                return false;
            }
    
            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* target = GetTarget())
                        if (roll_chance_i(chance))
                            caster->CastSpell(target, IMPROVED_MEND_PET_DISPEL, true);
            }
    
            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_pet_healAuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
            }
    
        private:
            int32 chance;
        };
        
        SpellScript* GetSpellScript() const override
        {
            return new spell_hun_pet_heal_SpellScript();
        }
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_pet_healAuraScript();
        }
};

// 53426 - Pet Lick Your Wounds
class spell_hun_pet_lick_your_wounds : public SpellScriptLoader
{
    public:
        spell_hun_pet_lick_your_wounds() : SpellScriptLoader("spell_hun_pet_lick_your_wounds") { }

        class spell_hun_pet_lick_your_wounds_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_pet_lick_your_wounds_SpellScript);

            bool Load() override
            {
                return GetCaster()->IsHunterPet();
            }

            void HandleBeforeCast()
            {
                Unit* pet = GetCaster();
                CharmInfo* charmInfo = pet->GetCharmInfo();

                if (!pet || !charmInfo)
                    return;

                pet->StopMoving();
                pet->GetMotionMaster()->Clear(false);
                pet->GetMotionMaster()->MoveIdle();

                charmInfo->SetIsCommandAttack(false);
                charmInfo->SetIsAtStay(true);
                charmInfo->SetIsCommandFollow(false);
                charmInfo->SetIsFollowing(false);
                charmInfo->SetIsReturning(false);
            }

            void Register() override
            {
                BeforeCast += SpellCastFn(spell_hun_pet_lick_your_wounds_SpellScript::HandleBeforeCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_hun_pet_lick_your_wounds_SpellScript();
        }
};

// 5384 - feign death
class spell_hun_feign_death : public SpellScriptLoader
{
    public:
        spell_hun_feign_death() : SpellScriptLoader("spell_hun_feign_death") { }
    
        class spell_hun_feign_death_AuraScript : public AuraScript
        {       
            PrepareAuraScript(spell_hun_feign_death_AuraScript);
    
            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* player = GetCaster())
                {
                    // if player has used readiness while feign death is active, remove its cooldown
                    if (player->HasAura(SPELL_READINESS_MARK_FOR_FEIGN_DEATH))
                    {
                        player->GetSpellHistory()->ResetCooldown(SPELL_HUNTER_FEIGN_DEATH, true);
                        player->RemoveAura(SPELL_READINESS_MARK_FOR_FEIGN_DEATH);
                    }
                }
            }
    
            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_hun_feign_death_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_FEIGN_DEATH, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_feign_death_AuraScript();
        }
};

// 34471, 38373 The beast Within
class spell_hun_the_beast_within : public SpellScriptLoader
{
    public:
        spell_hun_the_beast_within() : SpellScriptLoader("spell_hun_the_beast_within") { }

        class spell_hun_the_beast_within_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_the_beast_within_AuraScript);

            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->CastSpell(target, SPELL_THE_BEAST_WITHIN); // The Beast Within - Cosmetic
                target->RemoveAurasWithMechanic((1 << MECHANIC_STUN) | (1 << MECHANIC_FEAR) | (1 << MECHANIC_ROOT) | (1 << MECHANIC_SLEEP)| (1 << MECHANIC_FREEZE) | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_DISORIENTED) | (1 << MECHANIC_SILENCE) | (1 << MECHANIC_KNOCKOUT) | (1 << MECHANIC_BANISH));
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SLEEP, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, true); 
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true); 
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SILENCE, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_KNOCKOUT, true);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_BANISH, true); 
            }

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SLEEP, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, false); 
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SILENCE, false);  
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_KNOCKOUT, false);
                target->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_BANISH, false);
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_hun_the_beast_within_AuraScript::HandleOnEffectApply, EFFECT_2, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_hun_the_beast_within_AuraScript::HandleOnEffectRemove, EFFECT_2, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_the_beast_within_AuraScript();
        }
};

// -13159 - Aspect of the Pack
class spell_hunter_aspect_of_the_pack : public SpellScriptLoader
{
    public:
        spell_hunter_aspect_of_the_pack() : SpellScriptLoader("spell_hunter_aspect_of_the_pack") { }

        class spell_hunter_aspect_of_the_pack_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hunter_aspect_of_the_pack_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                { 
                    if (spellInfo->SpellIconID == SPELL_ICON_FROST_AURA) 
                        return false;
                    if (spellInfo->SpellIconID == SPELL_ICON_BACKLASH) 
                        return false;
                }
                return true; 
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_hunter_aspect_of_the_pack_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_hunter_aspect_of_the_pack_AuraScript();
        }
};

class spell_hunter_dust_cloud : public SpellScriptLoader
{
    public:
        spell_hunter_dust_cloud() : SpellScriptLoader("spell_hunter_dust_cloud") { }

        class spell_hunter_dust_cloud_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hunter_dust_cloud_SpellScript);

            SpellCastResult CheckCast()
            {
                if (Unit* target = GetExplTargetUnit())
                {
                    if (Creature* creature = target->ToCreature())
                        if (creature->isWorldBoss())
                            return SPELL_FAILED_BAD_TARGETS;
                }

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_hunter_dust_cloud_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_hunter_dust_cloud_SpellScript();
        }
};

// -53256 - Cobra Strikes
class spell_hun_cobra_strikes : public SpellScriptLoader
{
    public:
        spell_hun_cobra_strikes() : SpellScriptLoader("spell_hun_cobra_strikes") { }

        class spell_hun_cobra_strikes_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_cobra_strikes_AuraScript);

            void OnProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();

                for (uint8 i = 1; i <= 2; ++i)
                    if (Unit* caster = GetCaster())
                        caster->AddAura(SPELL_HUNTER_COBRA_STRIKES, caster);
            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_hun_cobra_strikes_AuraScript::OnProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_cobra_strikes_AuraScript();
        }
};

// 53508 - Wolverine bite
class spell_hun_pet_wolverine_bite : public SpellScriptLoader
{
    public:
        spell_hun_pet_wolverine_bite() : SpellScriptLoader("spell_hun_pet_wolverine_bite") { }

        class spell_hun_pet_wolverine_bite_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_pet_wolverine_bite_SpellScript);

            bool Load() override
            {
                return GetCaster()->IsPet();
            }

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                if (!caster->ToPet()->DoneSpellCrit())
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_hun_pet_wolverine_bite_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_hun_pet_wolverine_bite_SpellScript();
        }
};

// -53234  - Piercing Shots
class spell_hun_piercing_shots : public SpellScriptLoader
{
public:
    spell_hun_piercing_shots() : SpellScriptLoader("spell_hun_piercing_shots") { }

    class spell_hun_piercing_shots_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_hun_piercing_shots_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_HUNTER_PIERCING_SHOTS))
                return false;
            return true;
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (eventInfo.GetActionTarget())
                return true;
            return false;
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* caster = eventInfo.GetActor();
            Unit* target = eventInfo.GetActionTarget();

            if (DamageInfo* dmgInfo = eventInfo.GetDamageInfo())
            {
                SpellInfo const* piercingShots = sSpellMgr->GetSpellInfo(SPELL_HUNTER_PIERCING_SHOTS);
                int32 duration = piercingShots->GetMaxDuration();
                uint32 amplitude = piercingShots->Effects[EFFECT_0].Amplitude;
                uint32 dmg = dmgInfo->GetDamage() + dmgInfo->GetAbsorb();

                uint32 bp = CalculatePct(int32(dmg), aurEff->GetAmount()) / (duration / int32(amplitude));
                bp += target->GetRemainingPeriodicAmount(caster->GetGUID(), SPELL_HUNTER_PIERCING_SHOTS, SPELL_AURA_PERIODIC_DAMAGE);

                caster->CastCustomSpell(SPELL_HUNTER_PIERCING_SHOTS, SPELLVALUE_BASE_POINT0, bp, target, true, nullptr, aurEff);
            }
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_hun_piercing_shots_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_hun_piercing_shots_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_hun_piercing_shots_AuraScript();
    }
};

void AddSC_hunter_spell_scripts_rg()
{
    new spell_hun_item_t8_4p_bonus();
    new spell_hun_masters_call_aura();
    new spell_hun_pet_heal();
    new spell_hun_pet_lick_your_wounds();
    new spell_hun_feign_death();
    new spell_hun_the_beast_within();
    new spell_hunter_aspect_of_the_pack();
    new spell_hunter_dust_cloud();
    new spell_hun_cobra_strikes();
    new spell_hun_pet_wolverine_bite();
    new spell_hun_piercing_shots();
}
