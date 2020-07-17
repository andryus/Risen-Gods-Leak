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
 * Scripts for spells with SPELLFAMILY_SHAMAN and SPELLFAMILY_GENERIC spells used by shaman players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_sha_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"

enum ShamanSpells
{
    SPELL_MAELSTROM_WEAPON                   = 53817,
    SPELL_SHAMAN_SPIRIT_HUNT                 = 58877,
    SPELL_SHAMAN_SPIRIT_HUNT_HEAL            = 58879,
    SPELL_SHAMAN_TIDAL_FORCE                 = 55166,
    SPELL_SHAMAN_BLESSING_OF_THE_ETERNALS_R1 = 51554,
};

// 8178 - Grounding Totem
class spell_sha_grounding_totem : public SpellScriptLoader
{
    public:
        spell_sha_grounding_totem() : SpellScriptLoader("spell_sha_grounding_totem") { }

        class spell_sha_grounding_totem_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_grounding_totem_AuraScript);

            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* owner = GetUnitOwner())
                    owner->setDeathState(JUST_DIED);
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_grounding_totem_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_SPELL_MAGNET, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_grounding_totem_AuraScript();
        }
};

class spell_sha_tidal_force : public SpellScriptLoader
{
    public:
        spell_sha_tidal_force() : SpellScriptLoader("spell_sha_tidal_force") { }

        class spell_sha_tidal_force_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_tidal_force_AuraScript);

            void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                if (Unit* owner = GetUnitOwner())
                    if (Aura const* tidal = owner->GetAura(SPELL_SHAMAN_TIDAL_FORCE))
                    {
                        uint32 stack = tidal->GetStackAmount();
                        owner->RemoveAura(SPELL_SHAMAN_TIDAL_FORCE);
                        for (uint8 i = 1; i < stack; ++i)
                            owner->AddAura(SPELL_SHAMAN_TIDAL_FORCE, owner);
                    }

            }

            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_sha_tidal_force_AuraScript::OnProc, EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_tidal_force_AuraScript();
        }
};

// Lightning Bolt + Chain Lightning - Triggeredspells
class spell_sha_lightning_overload_proc : public SpellScriptLoader
{
    public:
        spell_sha_lightning_overload_proc() : SpellScriptLoader("spell_sha_lightning_overload_proc") { }

        class spell_sha_lightning_overload_proc_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_lightning_overload_proc_SpellScript);

            void HandleOnHit()
            {
                if (Unit* target = GetHitUnit())
                    if (roll_chance_i(11))
                        GetCaster()->CastSpell(target, GetSpellInfo()->Id, false);
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_sha_lightning_overload_proc_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_sha_lightning_overload_proc_SpellScript();
        }
};


// 3606 - Shaman Totem Attack
class spell_shaman_attack : public SpellScriptLoader
{
    public:
        spell_shaman_attack() : SpellScriptLoader("spell_shaman_attack") { }
        
        class spell_shaman_attack_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_shaman_attack_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                Unit* owner = caster->GetOwner();
                if (Unit* target = GetExplTargetUnit())
                    if (!owner->IsFriendlyTo(target))
                        if (!owner->IsValidAttackTarget(target))
                            return SPELL_FAILED_BAD_TARGETS;
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_shaman_attack_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_shaman_attack_SpellScript();
        }
};

// 58877  - Spirit Hunt
class spell_shaman_spirit_hunt : public SpellScriptLoader
{
public:
	spell_shaman_spirit_hunt() : SpellScriptLoader("spell_shaman_spirit_hunt") { }

    class spell_shaman_spirit_hunt_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_shaman_spirit_hunt_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SHAMAN_SPIRIT_HUNT))
                return false;
            return true;
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (eventInfo.GetActor()->GetOwner())
                return true;
            return false;
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            Unit* caster = eventInfo.GetActor();
			Unit* owner = eventInfo.GetActor()->GetOwner();

            if (DamageInfo* dmgInfo = eventInfo.GetDamageInfo())
            {
                int32 triggerAmount = aurEff->GetAmount();
                int32 basepoints = CalculatePct(int32(dmgInfo->GetDamage() + dmgInfo->GetAbsorb()), triggerAmount);
                // Cast on spirit wolf
                caster->CastCustomSpell(caster, SPELL_SHAMAN_SPIRIT_HUNT_HEAL, &basepoints, NULL, NULL, true, NULL, aurEff);
				// Cast on owner
				caster->CastCustomSpell(owner, SPELL_SHAMAN_SPIRIT_HUNT_HEAL, &basepoints, NULL, NULL, true, NULL, aurEff);
            }
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_shaman_spirit_hunt_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_shaman_spirit_hunt_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_shaman_spirit_hunt_AuraScript();
    }
};

// -51940 - Earthliving Weapon (Passive)
class spell_sha_earthliving_weapon : public SpellScriptLoader
{
public:
    spell_sha_earthliving_weapon() : SpellScriptLoader("spell_sha_earthliving_weapon") { }

    class spell_sha_earthliving_weapon_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_sha_earthliving_weapon_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SHAMAN_BLESSING_OF_THE_ETERNALS_R1))
                return false;
            return true;
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            int32 chance = 20;
            Unit* caster = eventInfo.GetActor();
            if (AuraEffect const* aurEff = caster->GetAuraEffectOfRankedSpell(SPELL_SHAMAN_BLESSING_OF_THE_ETERNALS_R1, EFFECT_1, caster->GetGUID()))
                if (eventInfo.GetProcTarget()->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                    chance += aurEff->GetAmount();

            return roll_chance_i(chance);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_sha_earthliving_weapon_AuraScript::CheckProc);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_sha_earthliving_weapon_AuraScript();
    }
};

void AddSC_shaman_spell_scripts_rg()
{
    new spell_sha_grounding_totem();
    new spell_sha_tidal_force();
    new spell_sha_lightning_overload_proc();
    new spell_shaman_attack();
	new spell_shaman_spirit_hunt();
    new spell_sha_earthliving_weapon();
}
