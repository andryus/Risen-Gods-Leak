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
 * Scripts for spells with SPELLFAMILY_WARRIOR and SPELLFAMILY_GENERIC spells used by warrior players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warr_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Group.h"
#include "Player.h"

enum WarriorSpells
{
    SPELL_WARRIOR_ENRAGED_REGENERATION              = 55694,
    SPELL_WARRIOR_SHIELD_WALL                       = 871,
    SPELL_WARRIOR_RECKLESSNESS                      = 1719,
    SPELL_WARRIOR_RETALIATION                       = 20230,
    SPELL_WARRIOR_SPELL_REFLECTION                  = 59725,
    SPELL_WARRIOR_SPELL_REFLECTION_EFFECT           = 23920,
    SPELL_WARRIOR_INTERVENE                         = 59667,
    SPELL_WARRIOR_BERSERKER_STANCE                  = 2458
};

// - 18499 - Berserker Rage
// - 2687  - Bloodrage
// - 12292 - Death Wish
class spell_warr_general_deny_rage_effects : public SpellScriptLoader
{
    public:
        spell_warr_general_deny_rage_effects() : SpellScriptLoader("spell_warr_general_deny_rage_effects") {}
    
        class spell_warr_general_deny_rage_effects_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_general_deny_rage_effects_SpellScript);
    
            SpellCastResult CheckCast()
            {
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(SPELL_WARRIOR_ENRAGED_REGENERATION))
                        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                return SPELL_CAST_OK;
            }
    
            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_warr_general_deny_rage_effects_SpellScript::CheckCast);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_general_deny_rage_effects_SpellScript();
        }
};

// - 871 Shield Wall
class spell_warr_shield_wall : public SpellScriptLoader
{
    public:
        spell_warr_shield_wall() : SpellScriptLoader("spell_warr_shield_wall") {}

        class spell_warr_shield_wall_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_shield_wall_SpellScript);

            SpellCastResult CheckCast()
            {
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(SPELL_WARRIOR_RECKLESSNESS) || caster->HasAura(SPELL_WARRIOR_RETALIATION))
                        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_warr_shield_wall_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_shield_wall_SpellScript();
        }
};

// - 1719 - Recklessness
class spell_warr_recklessness : public SpellScriptLoader
{
    public:
        spell_warr_recklessness() : SpellScriptLoader("spell_warr_recklessness") {}

        class spell_warr_recklessness_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_recklessness_SpellScript);

            SpellCastResult CheckCast()
            {
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(SPELL_WARRIOR_SHIELD_WALL) || caster->HasAura(SPELL_WARRIOR_RETALIATION))
                        return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_warr_recklessness_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_recklessness_SpellScript();
        }
};

// 3411 - Intervene
class spell_warr_intervene : public SpellScriptLoader
{
    public:
        spell_warr_intervene() : SpellScriptLoader("spell_warr_intervene") { }

        class spell_warr_intervene_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_intervene_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    target->CastSpell(target, SPELL_WARRIOR_INTERVENE, true);
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_warr_intervene_AuraScript::OnApply, EFFECT_1, SPELL_AURA_ADD_CASTER_HIT_TRIGGER, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_intervene_AuraScript();
        }
};

// 59725 - Improved Spell Reflection
class spell_warr_improved_spell_reflection_rg : public SpellScriptLoader
{
    public:
        spell_warr_improved_spell_reflection_rg() : SpellScriptLoader("spell_warr_improved_spell_reflection_rg") { }

        class spell_warr_improved_spell_reflection_rg_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_improved_spell_reflection_rg_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* player = caster->ToPlayer())
                    {
                        if (Group* group = player->GetGroup())
                        {
                            Group::MemberSlotList const& members = group->GetMemberSlots();

                            for (Group::member_citerator itr = members.begin(); itr != members.end(); ++itr)
                            {
                                Player* groupMember = ObjectAccessor::GetPlayer(*player, itr->guid);
                                if (!groupMember)
                                    continue;

                                groupMember->RemoveAura(SPELL_WARRIOR_SPELL_REFLECTION_EFFECT);
                                groupMember->RemoveAura(SPELL_WARRIOR_SPELL_REFLECTION);
                            }
                        }
                    }
                }
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_warr_improved_spell_reflection_rg_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_REFLECT_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        class spell_warr_improved_spell_reflection_rg_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_improved_spell_reflection_rg_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                if (GetCaster())
                    unitList.remove(GetCaster());
            }           

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warr_improved_spell_reflection_rg_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_PARTY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_improved_spell_reflection_rg_SpellScript();
        }

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_improved_spell_reflection_rg_AuraScript();
        }
};

// 23920 - Spell Reflection
class spell_warr_spell_reflection : public SpellScriptLoader
{
    public:
        spell_warr_spell_reflection() : SpellScriptLoader("spell_warr_spell_reflection") { }

        class spell_warr_spell_reflection_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_spell_reflection_AuraScript);

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (!IsExpired())
                {
                    // aura remove - remove auras from all party members
                    std::list<Unit*> PartyMembers;
                    GetUnitOwner()->GetPartyMembers(PartyMembers);
                    for (std::list<Unit*>::iterator itr = PartyMembers.begin(); itr != PartyMembers.end(); ++itr)
                    {
                        if ((*itr)->GetGUID() != GetOwner()->GetGUID())
                            if (Aura* aur = (*itr)->GetAura(SPELL_WARRIOR_SPELL_REFLECTION, GetCasterGUID()))
                            {
                                aur->SetDuration(0);
                                aur->Remove();
                            }
                    }
                }
            }

            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_warr_spell_reflection_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_REFLECT_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_spell_reflection_AuraScript();
        }
};

class spell_warr_berserker_stance_passive : public SpellScriptLoader
{
    public:
        spell_warr_berserker_stance_passive() : SpellScriptLoader("spell_warr_berserker_stance_passive") { }

        class spell_warr_berserker_stance_passive_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warr_berserker_stance_passive_AuraScript);

            void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (!GetTarget()->HasAura(SPELL_WARRIOR_BERSERKER_STANCE))
                    Remove();
            }

            void Register() override
            {
                AfterEffectApply += AuraEffectApplyFn(spell_warr_berserker_stance_passive_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_WEAPON_CRIT_PERCENT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_warr_berserker_stance_passive_AuraScript();
        }
};

// -6572 - Revenge
class spell_warr_revenge : public SpellScriptLoader
{
    public:
        spell_warr_revenge() : SpellScriptLoader("spell_warr_revenge") { }

        class spell_warr_revenge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warr_revenge_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                // Stealth Nerf 2010-03-30: One Revenge per proc
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAuraState(AURA_STATE_DEFENSE))
                        caster->ModifyAuraState(AURA_STATE_DEFENSE, false);
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_warr_revenge_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_warr_revenge_SpellScript();
        }
};

void AddSC_warrior_spell_scripts_rg()
{
    new spell_warr_general_deny_rage_effects();
    new spell_warr_shield_wall();
    new spell_warr_recklessness();
    new spell_warr_improved_spell_reflection_rg();
    new spell_warr_spell_reflection();
    new spell_warr_intervene();
    new spell_warr_berserker_stance_passive();
    new spell_warr_revenge();
}
