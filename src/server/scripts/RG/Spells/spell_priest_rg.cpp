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
 * Scripts for spells with SPELLFAMILY_PRIEST and SPELLFAMILY_GENERIC spells used by priest players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pri_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "Player.h"

enum PriestSpells
{
    PRIEST_IMPROVED_SHADOWFORM_R1               = 47569,
    PRIEST_IMPROVED_SHADOWFORM_R2               = 47570,
    SPELL_CYCLONE_DRUID                         = 33786,
    SPELL_GLYPH_PAIN_SUPPRESSION                = 63248,
    SPELL_SEDUCTION                             = 6358,
    PRIEST_SPIRIT_OF_REDEMPTION                 = 62371
};

class spell_pri_fade : public SpellScriptLoader
{
    public:
        spell_pri_fade() : SpellScriptLoader("spell_pri_fade") { }
    
        class spell_pri_fade_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_fade_SpellScript);
    
            bool Validate(SpellInfo const* /*spellEntry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_IMPROVED_SHADOWFORM_R1) || !sSpellMgr->GetSpellInfo(PRIEST_IMPROVED_SHADOWFORM_R2))
                    return false;
                return true;
            }
    
            bool Load() override
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(PRIEST_IMPROVED_SHADOWFORM_R1))
                        chance = 50;
                    else if (caster->HasAura(PRIEST_IMPROVED_SHADOWFORM_R2))
                        chance = 100;
                    else
                        chance = 0;
                }
                return true;
            }
    
    
            void HandleOnCast()
            {
                if (roll_chance_i(chance))
                    if (Unit* caster = GetCaster())
                        caster->RemoveAurasWithMechanic((1 << MECHANIC_ROOT) | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_SNARE));
            }
    
            void Register() override
            {
                OnCast += SpellCastFn(spell_pri_fade_SpellScript::HandleOnCast);
            }
    
        private:
            int8 chance;
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_pri_fade_SpellScript();
        }
};

// 1706 - levitate
class spell_pri_levitate : public SpellScriptLoader
{
    public:
        spell_pri_levitate() : SpellScriptLoader("spell_pri_levitate") { }
    
        class spell_pri_levitate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_levitate_SpellScript);
    
                SpellCastResult CheckRequirement()
                {
                    if (Unit* target = GetExplTargetUnit())
                        if (target->IsMounted())
                            return SPELL_FAILED_NOT_ON_MOUNTED;
                    return SPELL_CAST_OK;
                }
    
            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_pri_levitate_SpellScript::CheckRequirement);
            }
    
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_pri_levitate_SpellScript();
        }
};

// 33206 - Pain Suppression
class spell_pri_pain_suppression : public SpellScriptLoader
{
    public:
        spell_pri_pain_suppression() : SpellScriptLoader("spell_pri_pain_suppression") { }

        class spell_pri_pain_suppression_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_pain_suppression_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                Unit* target = GetExplTargetUnit();

                // Misc Exceptions for special Spells
                // Wyvern Sting
                if (caster->HasAuraWithMechanic(1 << MECHANIC_SLEEP))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                // Repentance + Gouge
                if (caster->HasAuraWithMechanic(1 << MECHANIC_KNOCKOUT))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                // Sap
                if (caster->HasAuraWithMechanic(1 << MECHANIC_SAPPED))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                // Seduce
                if (caster->HasAura(SPELL_SEDUCTION))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                // Nothing to do if we are in Cyclone
                if (target->HasAura(SPELL_CYCLONE_DRUID))
                    caster->GetSpellHistory()->AddCooldown(GetSpellInfo()->Id, 0, std::chrono::milliseconds(GetSpellInfo()->GetDuration()));

                if (caster->HasAura(SPELL_CYCLONE_DRUID))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                if (caster->HasAuraType(SPELL_AURA_MOD_STUN) && !caster->HasAura(SPELL_GLYPH_PAIN_SUPPRESSION, caster->GetGUID()))
                    return SPELL_FAILED_STUNNED;

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_pri_pain_suppression_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_pri_pain_suppression_SpellScript();
        }
};

// - 27827 Spirit of Redemption
class spell_pri_spirit_of_redemption : public SpellScriptLoader
{
    public:
        spell_pri_spirit_of_redemption() : SpellScriptLoader("spell_pri_spirit_of_redemption") { }

        class spell_pri_spirit_of_redemption_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_spirit_of_redemption_AuraScript);

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    target->RemoveAura(PRIEST_SPIRIT_OF_REDEMPTION);
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_pri_spirit_of_redemption_AuraScript::HandleOnEffectRemove, EFFECT_2, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL);
            }
        };

        class spell_pri_spirit_of_redemption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_spirit_of_redemption_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    caster->SetHealth(caster->GetMaxHealth());
                    caster->AddAura(PRIEST_SPIRIT_OF_REDEMPTION, caster);
                    caster->CastStop();
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_pri_spirit_of_redemption_SpellScript::HandleAfterCast);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_pri_spirit_of_redemption_AuraScript();
        }

        SpellScript* GetSpellScript() const override
        {
            return new spell_pri_spirit_of_redemption_SpellScript();
        }
};

// - 34433 - Shadowfiend
class spell_pri_shadowfiend : public SpellScriptLoader
{
    public:
        spell_pri_shadowfiend() : SpellScriptLoader("spell_pri_shadowfiend") { }

        class spell_pri_shadowfiend_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_shadowfiend_SpellScript);

            void HandleBeforeCast()
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetPetGUID())
                        if (caster->GetGuardianPet())
                            caster->GetGuardianPet()->DespawnOrUnsummon();
            }

            void Register() override
            {
                BeforeCast += SpellCastFn(spell_pri_shadowfiend_SpellScript::HandleBeforeCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_pri_shadowfiend_SpellScript();
        }
};

void AddSC_priest_spell_scripts_rg()
{
    new spell_pri_fade();
    new spell_pri_levitate();
    new spell_pri_pain_suppression();
    new spell_pri_spirit_of_redemption();
    new spell_pri_shadowfiend();
}
