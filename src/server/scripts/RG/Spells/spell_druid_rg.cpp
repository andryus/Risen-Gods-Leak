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
 * Scripts for spells with SPELLFAMILY_DRUID and SPELLFAMILY_GENERIC spells used by druid players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_dru_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"

enum DruidSpells
{
    SPELL_DRUID_NATURES_GRASP               = 53313,
    SPELL_DRUID_CYCLONE                     = 33786,
    SPELL_IMPROVED_INSECT_SWARM             = 57849,
    SPELL_DRUID_GLYPH_OF_BARKSKIN           = 63057,
    SPELL_DRUID_GLYPH_OF_BARKSKIN_TRIGGER   = 63058,
};

// 24905 - Moonkin Form passive passive
class spell_dru_moonkin_form_passive_passive : public SpellScriptLoader
{
    public:
        spell_dru_moonkin_form_passive_passive() : SpellScriptLoader("spell_dru_moonkin_form_passive_passive") { }

        class spell_dru_moonkin_form_passive_passive_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_moonkin_form_passive_passive_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (eventInfo.GetSpellInfo())
                    if (eventInfo.GetSpellInfo()->SpellIconID != 220 && eventInfo.GetSpellInfo()->SpellIconID != 2854 && eventInfo.GetSpellInfo()->SpellIconID != 15)
                        return true;
                return false;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_dru_moonkin_form_passive_passive_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dru_moonkin_form_passive_passive_AuraScript();
        }
};

// 53312 - Nature's Grasp
class spell_dru_natures_grasp : public SpellScriptLoader
{
    public:
        spell_dru_natures_grasp() : SpellScriptLoader("spell_dru_natures_grasp") { }

        class spell_dru_natures_grasp_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_natures_grasp_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_DRUID_NATURES_GRASP))
                    return false;
                return true;
            }

            void HandleEffectProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
            {
                if (Unit* target = eventInfo.GetProcTarget())
                    if (Unit* owner = GetUnitOwner())
                        if (target->HasAura(SPELL_DRUID_NATURES_GRASP, owner->GetGUID()))
                            PreventDefaultAction(); // will prevent default effect execution
            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_dru_natures_grasp_AuraScript::HandleEffectProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dru_natures_grasp_AuraScript();
        }
};

// 16864 - Omen of Clarity
class spell_dru_omen_of_clarity : public SpellScriptLoader
{
    public:
        spell_dru_omen_of_clarity() : SpellScriptLoader("spell_dru_omen_of_clarity") { }

        class spell_dru_omen_of_clarity_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_omen_of_clarity_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (eventInfo.GetSpellInfo())
                    if (eventInfo.GetSpellInfo()->SpellFamilyName != SPELLFAMILY_DRUID)
                        return false;

                return true;
            }

            void Register()
            {
                DoCheckProc += AuraCheckProcFn(spell_dru_omen_of_clarity_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_dru_omen_of_clarity_AuraScript();
        }
};

class spell_dru_cyclone : public SpellScriptLoader
{
    public:
        spell_dru_cyclone() : SpellScriptLoader("spell_dru_cyclone") { }

        class spell_dru_cyclone_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_cyclone_AuraScript);

            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->ApplySpellImmune(0, IMMUNITY_ID, SPELL_DRUID_CYCLONE, true);
            }

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->ApplySpellImmune(0, IMMUNITY_ID, SPELL_DRUID_CYCLONE, false);
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_dru_cyclone_AuraScript::HandleOnEffectApply, EFFECT_1, SPELL_AURA_SCHOOL_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_dru_cyclone_AuraScript::HandleOnEffectRemove, EFFECT_1, SPELL_AURA_SCHOOL_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dru_cyclone_AuraScript();
        }
};

enum Tracking
{
    SPELL_DRUID_TRACK_HUMANS                = 5225,
    SPELL_FIND_HERBES                       = 2383,
    SPELL_FIND_MINERALS                     = 2580,
    SPELL_FIND_FISH                         = 43308
};

// 768 - Cat Form
// We will use cat form to store the old tracking. Druids can detect humans in cat form but not in other forms.
// After switching to another form the druid should activate the old tracking which was active before he activates shapeshift.
class spell_dru_cat_form : public SpellScriptLoader
{
    public:
        spell_dru_cat_form() : SpellScriptLoader("spell_dru_cat_form") { }

        class spell_dru_cat_form_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_cat_form_AuraScript);

            bool Load() override
            {
                oldSpellId = 0;
                return true;
            }

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_DRUID_TRACK_HUMANS) || !sSpellMgr->GetSpellInfo(SPELL_FIND_HERBES) ||
                    !sSpellMgr->GetSpellInfo(SPELL_FIND_MINERALS) || !sSpellMgr->GetSpellInfo(SPELL_FIND_FISH))
                    return false;
                return true;
            }

            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* owner = GetUnitOwner())
                {
                    if (owner->HasAura(SPELL_FIND_HERBES))
                        oldSpellId = SPELL_FIND_HERBES;
                    else if (owner->HasAura(SPELL_FIND_MINERALS))
                        oldSpellId = SPELL_FIND_MINERALS;
                    else if (owner->HasAura(SPELL_FIND_FISH))
                        oldSpellId = SPELL_FIND_FISH;
                }
            }

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* owner = GetUnitOwner())
                    if (owner->HasAura(SPELL_DRUID_TRACK_HUMANS))
                        owner->CastSpell(owner, oldSpellId, true);
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_dru_cat_form_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_dru_cat_form_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL);
            }

        private:
            uint32 oldSpellId;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dru_cat_form_AuraScript();
        }
};

class spell_dru_wrath : public SpellScriptLoader
{
    public:
        spell_dru_wrath() : SpellScriptLoader("spell_dru_wrath") { }

        class spell_dru_wrath_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dru_wrath_SpellScript);

            void RecalculateDamage() 
            {
                Unit* target = GetExplTargetUnit();

                if (AuraEffect const* aura = GetCaster()->GetAuraEffectOfRankedSpell(SPELL_IMPROVED_INSECT_SWARM, EFFECT_0))
                    if (target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_DRUID, 0x00200000, 0, 0))
                    {
                        int32 dmg = GetHitDamage();
                        AddPct(dmg, aura->GetSpellInfo()->GetBasePoints(EFFECT_0));
                        SetHitDamage(dmg);
                    }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_dru_wrath_SpellScript::RecalculateDamage);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_dru_wrath_SpellScript();
        }
};

class spell_dru_barkskin : public SpellScriptLoader
{
    public:
        spell_dru_barkskin() : SpellScriptLoader("spell_dru_barkskin") { }

        class spell_dru_barkskin_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dru_barkskin_AuraScript);

            void AfterApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (GetUnitOwner()->HasAura(SPELL_DRUID_GLYPH_OF_BARKSKIN, GetUnitOwner()->GetGUID()))
                    GetUnitOwner()->CastSpell(GetUnitOwner(), SPELL_DRUID_GLYPH_OF_BARKSKIN_TRIGGER, true);
            }

            void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetUnitOwner()->RemoveAurasDueToSpell(SPELL_DRUID_GLYPH_OF_BARKSKIN_TRIGGER, GetUnitOwner()->GetGUID());
            }

            void Register() override
            {
                AfterEffectApply += AuraEffectApplyFn(spell_dru_barkskin_AuraScript::AfterApply, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_dru_barkskin_AuraScript::AfterRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_dru_barkskin_AuraScript();
        }
};

void AddSC_druid_spell_scripts_rg()
{
    new spell_dru_cat_form();
    new spell_dru_cyclone();
    new spell_dru_moonkin_form_passive_passive();
    new spell_dru_natures_grasp();
    new spell_dru_omen_of_clarity();
    new spell_dru_wrath();
    new spell_dru_barkskin();
}
