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
 * Scripts for spells with SPELLFAMILY_WARLOCK and SPELLFAMILY_GENERIC spells used by warlock players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warl_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "WorldSession.h"

enum WarlockSpells
{
    WARLOCK_GLYPH_OF_SUCCUBUS                       = 56250,
    SPELL_WARLOCK_FEL_HUNTER_SPELL_LOCK_SILENCE     = 24259,
    SPELL_PRIEST_SHADOW_WORD_DEATH                  = 32409,
    SPELL_RUNIC_POWER_BACK                          = 61257,
    SPELL_RUNIC_RETURN                              = 61258,
    SPELL_WARLOCK_SEDUCTION                         = 6358,
    SPELL_WARRIOR_BLADESTORM                        = 46924,
    SPELL_WARLOCK_PET_LESSER_INVISIBILITY           = 7870,
    SPELL_DEMONIC_PACT                              = 48090,
    SPELL_UNCHAINED_MAGIC                           = 69762,
    SPELL_INSTABILITY                               = 69766,
    SPELL_WARLOCK_LIFE_TAP_GLYPHE                   = 63320,
    SPELL_WARLOCK_LIFE_TAP_GLYPHE_EFFECT            = 63321,
};

class spell_warlock_enslave_demon : public SpellScriptLoader
{
    public:
        spell_warlock_enslave_demon() : SpellScriptLoader("spell_warlock_enslave_demon") { }
    
        class spell_warlock_enslave_demon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warlock_enslave_demon_SpellScript);
    
            SpellCastResult CheckCast()
            {
                if (Unit* target = GetExplTargetUnit())
                    if (target->ToCreature())
                        if (target->ToCreature()->isWorldBoss())
                            return SPELL_FAILED_ERROR;
                return SPELL_CAST_OK;
            }
    
            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_warlock_enslave_demon_SpellScript::CheckCast);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_warlock_enslave_demon_SpellScript();
        }
};

class spell_warlock_seduction : public SpellScriptLoader
{
    public:
        spell_warlock_seduction() : SpellScriptLoader("spell_warlock_seduction") { }
    
        class spell_warlock_seduction_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warlock_seduction_SpellScript);
    
            bool Validate(SpellInfo const* /*spellEntry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_GLYPH_OF_SUCCUBUS))
                    return false;
                return true;
            }
    
            SpellCastResult CheckCast()
            {
                if (Unit* target = GetExplTargetUnit())
                    if (target->HasAura(SPELL_WARRIOR_BLADESTORM))
                        return SPELL_FAILED_IMMUNE;
                return SPELL_CAST_OK;
            }
    
            void HandleOnHit()
            {
                Unit* caster = GetCaster();
                Unit* victim = GetHitUnit();
    
                if (caster == victim)
                    GetCaster()->RemoveAura(SPELL_WARLOCK_SEDUCTION);
                
                GetCaster()->RemoveAura(SPELL_WARLOCK_PET_LESSER_INVISIBILITY);
    
                if (Unit* caster = GetCaster())
                    if (Unit* owner = caster->GetOwner())
                        if (owner->HasAura(WARLOCK_GLYPH_OF_SUCCUBUS, owner->GetGUID()))
                            if (Unit* target = GetHitUnit())
                                if (caster != victim)
                                { 
                                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE, ObjectGuid::Empty, target->GetAura(SPELL_PRIEST_SHADOW_WORD_DEATH)); // SW:D shall not be removed.
                                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_LEECH);   
                                    target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE_PERCENT);
                                }
            }
    
            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_warlock_seduction_SpellScript::CheckCast);
                OnHit += SpellHitFn(spell_warlock_seduction_SpellScript::HandleOnHit);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_warlock_seduction_SpellScript();
        }
};

// 19244, 19647 Spell Lock (Fel Hunter)
class spell_warlock_spell_lock : public SpellScriptLoader
{
    public:
        spell_warlock_spell_lock() : SpellScriptLoader("spell_warlock_spell_lock") { }
    
        class spell_warlock_spell_lock_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warlock_spell_lock_SpellScript);
    
            bool Validate(SpellInfo const* /*spellEntry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_WARLOCK_FEL_HUNTER_SPELL_LOCK_SILENCE))
                    return false;
                return true;
            }
    
            void OnLaunch(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
            }
    
            void Silence()
            {
                // Silence is casted via triggered spell, which is executed on launch and causes also interruption of current casted spells
                // because they are already interrupted SPELL_EFFECT_INTERRUPT_CAST does actually nothing
                // this prevents locking the spell school
                GetCaster()->CastSpell(GetHitUnit(), SPELL_WARLOCK_FEL_HUNTER_SPELL_LOCK_SILENCE, true);
            }
    
            void Register() override
            {
                OnEffectLaunchTarget += SpellEffectFn(spell_warlock_spell_lock_SpellScript::OnLaunch, EFFECT_1, SPELL_EFFECT_TRIGGER_SPELL);
                AfterHit += SpellHitFn(spell_warlock_spell_lock_SpellScript::Silence);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_warlock_spell_lock_SpellScript();
        }
};

class spell_warlock_consume_shadows_effect : public SpellScriptLoader
{
    public:
        spell_warlock_consume_shadows_effect() : SpellScriptLoader("spell_warlock_consume_shadows_effect") { }
   
        class spell_warlock_consume_shadows_effect_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warlock_consume_shadows_effect_AuraScript);

        public:
            spell_warlock_consume_shadows_effect_AuraScript() : oldCommand(COMMAND_STAY) { }

        private:
            bool Load() override
            {
                return GetCaster()->IsPet();
            }

            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* pet = GetCaster();
                if (!pet)
                    return;

                CharmInfo* charmInfo = pet->GetCharmInfo();
                if (!charmInfo)
                    return;

                if (GetTarget() == pet)
                {
                    if (!pet->GetVictim())
                        oldCommand = charmInfo->GetCommandState();
                    else
                    {
                        oldCommand = COMMAND_ATTACK;
                        targetGuid = pet->GetVictim()->GetGUID();
                    }

                    if (Player* owner = GetCaster()->GetCharmerOrOwnerPlayerOrPlayerItself())
                        owner->GetSession()->HandlePetActionHelper(GetCaster(), GetCasterGUID(), COMMAND_STAY, ACT_COMMAND, ObjectGuid::Empty);
                }
            }

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetTarget() || !GetCaster() || oldCommand == COMMAND_STAY || GetTarget() != GetCaster())
                    return;
                
                if (Player* owner = GetCaster()->GetCharmerOrOwnerPlayerOrPlayerItself())
                    owner->GetSession()->HandlePetActionHelper(GetCaster(), GetCasterGUID(), oldCommand, ACT_COMMAND, targetGuid);
            }
            
            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_warlock_consume_shadows_effect_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_MOD_STEALTH_DETECT, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_warlock_consume_shadows_effect_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_MOD_STEALTH_DETECT, AURA_EFFECT_HANDLE_REAL);
            }

            CommandStates oldCommand;
            ObjectGuid targetGuid;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warlock_consume_shadows_effect_AuraScript();
        }
};

// 61257 - Runic Power Back on Snare/Root
class spell_warlock_runic_power_back : public SpellScriptLoader
{
    public:
        spell_warlock_runic_power_back() : SpellScriptLoader("spell_warlock_runic_power_back") { }

        class spell_warlock_runic_power_back_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warlock_runic_power_back_AuraScript);
            
            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    if (target->HasAura(SPELL_RUNIC_POWER_BACK))
                        target->AddAura(SPELL_RUNIC_RETURN, target);
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_warlock_runic_power_back_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_DECREASE_SPEED, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warlock_runic_power_back_AuraScript();
        }
};

class spell_warlock_drain_soul : public SpellScriptLoader
{
    public:
        spell_warlock_drain_soul() : SpellScriptLoader("spell_warlock_drain_soul") { }

        class spell_warlock_drain_soul_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warlock_drain_soul_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* victim = GetHitUnit())
                    {
                        caster->EngageWithTarget(victim);
                    }                        
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_warlock_drain_soul_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
            }
        };

        class spell_warlock_drain_soul_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warlock_drain_soul_AuraScript);

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* victim = GetTarget())
                    {
                        caster->EngageWithTarget(victim);
                    }
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warlock_drain_soul_AuraScript::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warlock_drain_soul_SpellScript();
        }

        AuraScript* GetAuraScript() const override
        {
            return new spell_warlock_drain_soul_AuraScript();
        }
};

class spell_warlock_demonic_pact : public SpellScriptLoader
{
    public:
        spell_warlock_demonic_pact() : SpellScriptLoader("spell_warlock_demonic_pact") { }

        class spell_warlock_demonic_pact_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warlock_demonic_pact_AuraScript);

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (eventInfo.GetActor()->HasAura(SPELL_DEMONIC_PACT))
                    if (eventInfo.GetActor()->GetAura(SPELL_DEMONIC_PACT)->GetDuration() > 25 * IN_MILLISECONDS)
                        return false;

                return true;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_warlock_demonic_pact_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warlock_demonic_pact_AuraScript();
        }
};

// - 18220 - Dark Pact
class spell_warlock_dark_pact : public SpellScriptLoader
{
    public:
        spell_warlock_dark_pact() : SpellScriptLoader("spell_warlock_dark_pact") { }

        class spell_warlock_dark_pact_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warlock_dark_pact_SpellScript);

            void HandleEffectHit(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster())
                    return;
                
                if (GetCaster()->HasAura(SPELL_WARLOCK_LIFE_TAP_GLYPHE))
                    GetCaster()->CastSpell(GetCaster(), SPELL_WARLOCK_LIFE_TAP_GLYPHE_EFFECT, true);
            }

            void HandleAfterCast()
            {
                if (GetCaster()->HasAura(SPELL_UNCHAINED_MAGIC))
                    GetCaster()->CastSpell(GetCaster(), SPELL_INSTABILITY);
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_warlock_dark_pact_SpellScript::HandleEffectHit, EFFECT_0, SPELL_EFFECT_POWER_DRAIN);
                AfterCast += SpellCastFn(spell_warlock_dark_pact_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warlock_dark_pact_SpellScript();
        }
};

// 126 - Eye of Kilrogg
class spell_warl_eye_of_kilrogg : public SpellScriptLoader
{
    public:
        spell_warl_eye_of_kilrogg() : SpellScriptLoader("spell_warl_eye_of_kilrogg") { }
    
        class spell_warl_eye_of_kilrogg_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_eye_of_kilrogg_AuraScript);
    
            void HandleAuraApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                PreventDefaultAction();
                if (Player* player = GetTarget()->ToPlayer())
                {
                    player->UnsummonPetTemporaryIfAny();
    
                    if (player->HasAura(58081)) // Glyph of Kilrogg
                        if (Unit* charm = player->GetCharm())
                        {
                            charm->SetSpeedRate(MOVE_RUN, 2.14f);
                            if (charm->GetMapId() == 530 || charm->GetMapId() == 571)
                            {
                                charm->SetCanFly(true);
                                charm->SetSpeedRate(MOVE_FLIGHT, 2.14f);
                                charm->SendMovementFlagUpdate();
                            }
                        }
                }
            }
    
            void HandleAuraRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* player = GetTarget()->ToPlayer())
                {
                    if (Unit* charm = player->GetCharm())
                        charm->ToTempSummon()->UnSummon();
    
                    player->ResummonPetTemporaryUnSummonedIfAny();
                }
            }
    
            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_warl_eye_of_kilrogg_AuraScript::HandleAuraApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_warl_eye_of_kilrogg_AuraScript::HandleAuraRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_warl_eye_of_kilrogg_AuraScript();
        }
};

void AddSC_warlock_spell_scripts_rg()
{
    new spell_warlock_enslave_demon();
    new spell_warlock_seduction();
    new spell_warlock_spell_lock();
    new spell_warlock_consume_shadows_effect();
    new spell_warlock_runic_power_back();
    new spell_warlock_drain_soul();
    new spell_warlock_demonic_pact();
    new spell_warlock_dark_pact();
    new spell_warl_eye_of_kilrogg();
}
