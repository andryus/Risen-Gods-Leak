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
 * Scripts for spells with SPELLFAMILY_ROGUE and SPELLFAMILY_GENERIC spells used by rogue players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_rog_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"
#include "DBCStores.h"

enum RogueSpells
{
    SPELL_ROGUE_KILLING_SPREE_TELEPORT           = 57840,
    SPELL_ROGUE_KILLING_SPREE_ATTACK             = 57841,

    SPELL_IMPROVED_POISONS_RANK_1                = 14113,
};

// 2094 - blind
class spell_rog_blind : public SpellScriptLoader
{
    public:
        spell_rog_blind() : SpellScriptLoader("spell_rog_blind") { }

        class spell_rog_blind_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_blind_SpellScript);

            void HandleOnHit()
            {
                if (Unit* target = GetHitUnit())
                {
                    if (target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                        target->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_rog_blind_SpellScript::HandleOnHit);
            }

        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rog_blind_SpellScript();
        }
};

// 1766 - kick
// 2098 - eviscerate
// 1833 - cheap shot
// 408  - kidney shot
// 8647 - expose armor
class spell_rog_proc_poison_general : public SpellScriptLoader
{
    public:
        spell_rog_proc_poison_general() : SpellScriptLoader("spell_rog_proc_poison_general") { }

        class spell_rog_proc_poison_general_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_proc_poison_general_SpellScript);

            void HandleOnHit()
            {
                Player* player = GetCaster()->ToPlayer();
                Unit* target = GetHitUnit();
                Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

                if (!player || !target || !item)
                    return;

                // item combat enchantments
                for (uint8 slot = 0; slot < MAX_ENCHANTMENT_SLOT; ++slot)
                {
                    SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(item->GetEnchantmentId(EnchantmentSlot(slot)));
                    if (!enchant)
                        continue;

                    for (uint8 s = 0; s < 3; ++s)
                    {
                        if (enchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                            continue;

                        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(enchant->spellid[s]);
                        if (!spellInfo)
                        {
                            TC_LOG_ERROR("spells", "Player::CastItemCombatSpell Enchant %i, player (Name: %s, GUID: %u) cast unknown spell %i", enchant->ID, player->GetName().c_str(), player->GetGUID().GetCounter(), enchant->spellid[s]);
                            continue;
                        }

                        // Proc only rogue poisons
                        if (spellInfo->SpellFamilyName != SPELLFAMILY_ROGUE || spellInfo->Dispel != DISPEL_POISON)
                            continue;

                        int8 procChance = 0;
                        bool scaleWithImprovedPoisons = false;
                        switch (spellInfo->SpellIconID)
                        {
                            case 110:
                            case 264:
                            case 1496:
                                procChance = 50;
                                break;
                            case 513:
                                procChance = 30;
                                scaleWithImprovedPoisons = true;
                                break;
                            case 247:
                                procChance = 20;
                                scaleWithImprovedPoisons = true;
                                break;
                        }
                        if (scaleWithImprovedPoisons)
                            if (Aura* aura = player->GetAuraOfRankedSpell(SPELL_IMPROVED_POISONS_RANK_1, player->GetGUID()))
                                procChance += aura->GetEffect(spellInfo->SpellIconID == 513 ? EFFECT_0 : EFFECT_1)->GetAmount();

                        if (roll_chance_i(procChance))
                        {
                            if (spellInfo->IsPositive())
                                player->CastSpell(player, enchant->spellid[s], true, item);
                            else
                                player->CastSpell(target, enchant->spellid[s], true, item);
                        }
                    }
                }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_rog_proc_poison_general_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rog_proc_poison_general_SpellScript();
        }
};

class spell_rog_sap : public SpellScriptLoader
{
    public:
        spell_rog_sap() : SpellScriptLoader("spell_rog_sap") { }

        class spell_rog_sap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_sap_SpellScript);

            void HandleOnHit()
            {
                if (Unit* target = GetHitUnit())
                    if (target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                        target->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_rog_sap_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rog_sap_SpellScript();
        }
};

class spell_rog_vanish : public SpellScriptLoader
{
    public:
        spell_rog_vanish() : SpellScriptLoader("spell_rog_vanish") { }

        class spell_rog_vanish_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_vanish_SpellScript);

            void HandleOnCast()
            {
                if (Unit* caster = GetCaster())
                    caster->ClearInCombat();
            }

            void Register() override
            {
                OnCast += SpellCastFn(spell_rog_vanish_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_rog_vanish_SpellScript();
        }
};

class spell_rog_savage_combat : public SpellScriptLoader
{
    public:
        spell_rog_savage_combat() : SpellScriptLoader("spell_rog_savage_combat") { }

        class spell_rog_savage_combat_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rog_savage_combat_AuraScript);

            void CalcPeriodic(AuraEffect const* /*effect*/, bool& isPeriodic, int32& amplitude)
            {
                isPeriodic = true;
                amplitude = 1000;
            }

            void Update(AuraEffect* auraEffect)
            {
                Unit::AuraApplicationMap const& auras = GetUnitOwner()->GetAppliedAuras();
                for (Unit::AuraApplicationMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    if (itr->second->GetBase()->GetCasterGUID() == this->GetCasterGUID() && itr->second->GetBase()->GetSpellInfo()->Dispel == DISPEL_POISON)
                        return;

                SetDuration(0);
            }

            void Register() override
            {
                DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_rog_savage_combat_AuraScript::CalcPeriodic, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);
                OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_rog_savage_combat_AuraScript::Update, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN);

            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_rog_savage_combat_AuraScript();
        }
};

void AddSC_rogue_spell_scripts_rg()
{
    new spell_rog_blind();
    new spell_rog_proc_poison_general();
    new spell_rog_sap();
    new spell_rog_vanish();
    new spell_rog_savage_combat();
}
