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

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include "Player.h"
#include "Pet.h"

class spell_gen_pet_calculate : public SpellScriptLoader
{
    public:
        spell_gen_pet_calculate() : SpellScriptLoader("spell_gen_pet_calculate") { }

        class spell_gen_pet_calculate_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_pet_calculate_AuraScript);

            bool Load() override
            {
                if (!GetCaster() || !GetCaster()->GetOwner() || GetCaster()->GetOwner()->GetTypeId() != TYPEID_PLAYER)
                    return false;
                return true;
            }

            void CalculateAmountCritSpell(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float CritSpell = 0.0f;
                    // Crit from Intellect
                    CritSpell += owner->GetSpellCritFromIntellect();
                    // Increase crit from SPELL_AURA_MOD_SPELL_CRIT_CHANCE
                    CritSpell += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_CRIT_CHANCE);
                    // Increase crit from SPELL_AURA_MOD_CRIT_PCT
                    CritSpell += owner->GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PCT);
                    // Increase crit spell from spell crit ratings
                    CritSpell += owner->GetRatingBonusValue(CR_CRIT_SPELL);

                    amount += int32(CritSpell);
                }
            }

            void CalculateAmountCritMelee(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float CritMelee = 0.0f;
                    // Crit from Agility
                    CritMelee += owner->GetMeleeCritFromAgility();
                    // Increase crit from SPELL_AURA_MOD_WEAPON_CRIT_PERCENT
                    CritMelee += owner->GetTotalAuraModifier(SPELL_AURA_MOD_WEAPON_CRIT_PERCENT);
                    // Increase crit from SPELL_AURA_MOD_CRIT_PCT
                    CritMelee += owner->GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PCT);
                    // Increase crit melee from melee crit ratings
                    CritMelee += owner->GetRatingBonusValue(CR_CRIT_MELEE);

                    amount += int32(CritMelee);
                }
            }

            void CalculateAmountMeleeHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitMelee = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_HIT_CHANCE
                    HitMelee += owner->GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
                    // Increase hit melee from meele hit ratings
                    HitMelee += owner->GetRatingBonusValue(CR_HIT_MELEE);

                    amount += int32(HitMelee);
                }
            }

            void CalculateAmountSpellHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitSpell = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    HitSpell += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase hit spell from spell hit ratings
                    HitSpell += owner->GetRatingBonusValue(CR_HIT_SPELL);

                    amount += int32(HitSpell);
                }
            }

            void CalculateAmountExpertise(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float Expertise = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_EXPERTISE
                    Expertise += owner->GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE);
                    // Increase Expertise from Expertise ratings
                    Expertise += owner->GetRatingBonusValue(CR_EXPERTISE);

                    amount += int32(Expertise);
                }
            }

            void Register() override
            {
                switch (m_scriptSpellId)
                {
                    //case SPELL_TAMED_PET_PASSIVE_06:
                    //    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountCritMelee, EFFECT_0, SPELL_AURA_MOD_WEAPON_CRIT_PERCENT);
                    //    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountCritSpell, EFFECT_1, SPELL_AURA_MOD_SPELL_CRIT_CHANCE);
                    //    break;
                    //case SPELL_PET_PASSIVE_CRIT:
                    //    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountCritSpell, EFFECT_0, SPELL_AURA_MOD_SPELL_CRIT_CHANCE);
                    //    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountCritMelee, EFFECT_1, SPELL_AURA_MOD_WEAPON_CRIT_PERCENT);
                    //    break;
                    //case SPELL_WARLOCK_PET_SCALING_05:
                    //case SPELL_HUNTER_PET_SCALING_04:
                    //    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountMeleeHit, EFFECT_0, SPELL_AURA_MOD_HIT_CHANCE);
                    //    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountSpellHit, EFFECT_1, SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    //    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountExpertise, EFFECT_2, SPELL_AURA_MOD_EXPERTISE);
                    //    break;
                    case SPELL_TAMED_PET_PASSIVE_08_HIT:
                        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountMeleeHit, EFFECT_0, SPELL_AURA_MOD_HIT_CHANCE);
                        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_pet_calculate_AuraScript::CalculateAmountSpellHit, EFFECT_1, SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                        break;
                    default:
                        break;
                }
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_pet_calculate_AuraScript();
        }
};

class spell_warl_pet_scaling_05 : public SpellScriptLoader
{
    public:
        spell_warl_pet_scaling_05() : SpellScriptLoader("spell_warl_pet_scaling_05") { }
    
        class spell_warl_pet_scaling_05_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_pet_scaling_05_AuraScript);
    
            bool Load() override
            {
                if (!GetCaster() || !GetCaster()->GetOwner() || GetCaster()->GetOwner()->GetTypeId() != TYPEID_PLAYER)
                    return false;
                return true;
            }
    
            void CalculateAmountMeleeHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitMelee = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    HitMelee += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase hit spell from spell hit ratings
                    HitMelee += owner->GetRatingBonusValue(CR_HIT_SPELL);
    
                    amount += int32(HitMelee);
                }
            }
    
            void CalculateAmountSpellHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitSpell = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    HitSpell += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase hit spell from spell hit ratings
                    HitSpell += owner->GetRatingBonusValue(CR_HIT_SPELL);
    
                    amount += int32(HitSpell);
                }
            }
    
            void CalculateAmountExpertise(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float Expertise = 0.0f;
                    // Increase Expertise from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    Expertise += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase Expertise from SPELL_AURA_MOD_HIT_CHANCE
                    Expertise += owner->GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
                    // Increase Expertise from Hit ratings
                    Expertise += owner->GetRatingBonusValue(CR_HIT_SPELL);
                    // float Expertise is hit in % warlock has ~14% hit and pet needs 26 expertise
                    Expertise *= 1.85f;
    
                    amount += int32(Expertise);
                }
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_pet_scaling_05_AuraScript::CalculateAmountMeleeHit, EFFECT_0, SPELL_AURA_MOD_HIT_CHANCE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_pet_scaling_05_AuraScript::CalculateAmountSpellHit, EFFECT_1, SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_pet_scaling_05_AuraScript::CalculateAmountExpertise, EFFECT_2, SPELL_AURA_MOD_EXPERTISE);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_warl_pet_scaling_05_AuraScript();
        }
};

class spell_sha_pet_scaling_04 : public SpellScriptLoader
{
    public:
        spell_sha_pet_scaling_04() : SpellScriptLoader("spell_sha_pet_scaling_04") { }
    
        class spell_sha_pet_scaling_04_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_pet_scaling_04_AuraScript);
    
            bool Load() override
            {
                if (!GetCaster() || !GetCaster()->GetOwner() || GetCaster()->GetOwner()->GetTypeId() != TYPEID_PLAYER)
                    return false;
                return true;
            }
    
            void CalculateAmountMeleeHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitMelee = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_HIT_CHANCE
                    HitMelee += owner->GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
                    // Increase hit melee from meele hit ratings
                    HitMelee += owner->GetRatingBonusValue(CR_HIT_MELEE);
    
                    amount += int32(HitMelee);
                }
            }
    
            void CalculateAmountSpellHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitSpell = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    HitSpell += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase hit spell from spell hit ratings
                    HitSpell += owner->GetRatingBonusValue(CR_HIT_SPELL);
    
                    amount += int32(HitSpell);
                }
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_pet_scaling_04_AuraScript::CalculateAmountMeleeHit, EFFECT_0, SPELL_AURA_MOD_HIT_CHANCE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_pet_scaling_04_AuraScript::CalculateAmountSpellHit, EFFECT_1, SPELL_AURA_MOD_SPELL_HIT_CHANCE);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_pet_scaling_04_AuraScript();
        }
};

class spell_sha_fire_elemental_scaling : public SpellScriptLoader
{
    public:
        spell_sha_fire_elemental_scaling() : SpellScriptLoader("spell_sha_fire_elemental_scaling") { }
    
        class spell_sha_fire_elemental_scaling_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_fire_elemental_scaling_AuraScript);
    
            void CalculateResistanceAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // fire elemental inherits 40% of resistance from owner and 35% of armor
                if (Unit* owner = GetUnitOwner()->GetOwner())
                {
                    SpellSchoolMask schoolMask = SpellSchoolMask(aurEff->GetSpellInfo()->Effects[aurEff->GetEffIndex()].MiscValue);
                    int32 modifier = schoolMask == SPELL_SCHOOL_MASK_NORMAL ? 35 : 40;
                    amount = CalculatePct(std::max<int32>(0, owner->GetResistance(schoolMask)), modifier);
                }
            }
    
            void CalculateStatAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // fire elemental inherits 30% of intellect / stamina
                if (Unit* owner = GetUnitOwner()->GetOwner())
                {
                    Stats stat = Stats(aurEff->GetSpellInfo()->Effects[aurEff->GetEffIndex()].MiscValue);
                    amount = CalculatePct(std::max<int32>(0, owner->GetStat(stat)), 30);
                }
            }
    
            void CalculateAPAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // fire elemental inherits 300% / 150% of SP as AP
                if (Unit* owner = GetUnitOwner()->GetOwner())
                {
                    int32 fire = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE);
                    amount = CalculatePct(std::max<int32>(0, fire), (GetUnitOwner()->GetEntry() == NPC_FIRE_ELEMENTAL ? 300 : 150));
                }
            }
    
            void CalculateSPAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // fire elemental inherits 100% of SP
                if (Unit* owner = GetUnitOwner()->GetOwner())
                {
                    int32 fire = owner->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_FIRE);
                    amount = CalculatePct(std::max<int32>(0, fire), 100);
    
                    // Update appropriate player field
                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        owner->SetUInt32Value(PLAYER_PET_SPELL_POWER, (uint32)amount);
                }
            }
    
            void HandleEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                GetUnitOwner()->ApplySpellImmune(0, IMMUNITY_STATE, aurEff->GetAuraType(), true);
                if (aurEff->GetAuraType() == SPELL_AURA_MOD_ATTACK_POWER)
                    GetUnitOwner()->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_ATTACK_POWER_PCT, true);
                else if (aurEff->GetAuraType() == SPELL_AURA_MOD_STAT)
                    GetUnitOwner()->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE, true);
            }
    
            void Register() override
            {
                if (m_scriptSpellId != 35665 && m_scriptSpellId != 65225)
                    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_fire_elemental_scaling_AuraScript::CalculateResistanceAmount, EFFECT_ALL, SPELL_AURA_MOD_RESISTANCE);
    
                if (m_scriptSpellId == 35666 || m_scriptSpellId == 65226)
                    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_fire_elemental_scaling_AuraScript::CalculateStatAmount, EFFECT_ALL, SPELL_AURA_MOD_STAT);
    
                if (m_scriptSpellId == 35665 || m_scriptSpellId == 65225)
                {
                    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_fire_elemental_scaling_AuraScript::CalculateAPAmount, EFFECT_ALL, SPELL_AURA_MOD_ATTACK_POWER);
                    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_fire_elemental_scaling_AuraScript::CalculateSPAmount, EFFECT_ALL, SPELL_AURA_MOD_DAMAGE_DONE);
                }
    
                OnEffectApply += AuraEffectApplyFn(spell_sha_fire_elemental_scaling_AuraScript::HandleEffectApply, EFFECT_ALL, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_sha_fire_elemental_scaling_AuraScript();
        }
};

class spell_hun_pet_scaling_04 : public SpellScriptLoader
{
    public:
        spell_hun_pet_scaling_04() : SpellScriptLoader("spell_hun_pet_scaling_04") { }
    
        class spell_hun_pet_scaling_04_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_pet_scaling_04_AuraScript);
    
            bool Load() override
            {
                if (!GetCaster() || !GetCaster()->GetOwner() || GetCaster()->GetOwner()->GetTypeId() != TYPEID_PLAYER)
                    return false;
                return true;
            }
    
            void CalculateAmountMeleeHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (!GetCaster() || !GetCaster()->GetOwner())
                    return;
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitMelee = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_HIT_CHANCE
                    HitMelee += owner->GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
                    // Increase hit melee from meele hit ratings
                    HitMelee += owner->GetRatingBonusValue(CR_HIT_MELEE);
    
                    amount += int32(HitMelee);
                }
            }
    
            void CalculateAmountSpellHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (!GetCaster() || !GetCaster()->GetOwner())
                    return;
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitSpell = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    HitSpell += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase hit spell from spell hit ratings
                    HitSpell += owner->GetRatingBonusValue(CR_HIT_SPELL);
    
                    amount += int32(HitSpell);
                }
            }
    
            void CalculateAmountExpertise(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (!GetCaster() || !GetCaster()->GetOwner())
                    return;
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float Expertise = 0.0f;
                    // Increase Expertise from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    Expertise += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase Expertise from SPELL_AURA_MOD_HIT_CHANCE
                    Expertise += owner->GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
                    // Increase Expertise from Hit ratings
                    Expertise += owner->GetRatingBonusValue(CR_HIT_SPELL);
                    // float Expertise is hit in % hunter has ~8% hit and pet needs 26 expertise
                    Expertise *= 3.25f;
    
                    amount += int32(Expertise);
                }
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hun_pet_scaling_04_AuraScript::CalculateAmountMeleeHit, EFFECT_0, SPELL_AURA_MOD_HIT_CHANCE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hun_pet_scaling_04_AuraScript::CalculateAmountSpellHit, EFFECT_1, SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hun_pet_scaling_04_AuraScript::CalculateAmountExpertise, EFFECT_2, SPELL_AURA_MOD_EXPERTISE);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_hun_pet_scaling_04_AuraScript();
        }
};

class spell_dk_pet_scaling_02 : public SpellScriptLoader
{
    public:
        spell_dk_pet_scaling_02() : SpellScriptLoader("spell_dk_pet_scaling_02") { }
    
        class spell_dk_pet_scaling_02_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_pet_scaling_02_AuraScript);
    
            bool Load() override
            {
                if (!GetCaster() || !GetCaster()->ToPet() || !GetCaster()->ToPet()->IsPetGhoul() || !GetCaster()->GetOwner() || GetCaster()->GetOwner()->GetTypeId() != TYPEID_PLAYER)
                    return false;
                return true;
            }
    
            void CalculateAmountMeleeHaste(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (!GetCaster() || !GetCaster()->GetOwner())
                    return;
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HasteMelee = 0.0f;
                    // Increase speed from BASE_ATTACK

                    HasteMelee = owner->GetRatingBonusValue(CR_HASTE_MELEE);

                    // hackfix for SPELL_DK_UNHOLY_PRESENCE
                    // I suspect 49772 should be applied on pet in case of unholy presence
                    // its currently used as triggered spell, so need to research
                    if (owner->HasAura(48265)) 
                        HasteMelee += 15.0f;

                    amount += int32(HasteMelee);
                }
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_pet_scaling_02_AuraScript::CalculateAmountMeleeHaste, EFFECT_1, SPELL_AURA_MELEE_SLOW);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_pet_scaling_02_AuraScript();
        }
};

class spell_dk_pet_scaling_03 : public SpellScriptLoader
{
    public:
        spell_dk_pet_scaling_03() : SpellScriptLoader("spell_dk_pet_scaling_03") { }
    
        class spell_dk_pet_scaling_03_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_pet_scaling_03_AuraScript);
    
            bool Load() override
            {
                if (!GetCaster() || !GetCaster()->GetOwner() || GetCaster()->GetOwner()->GetTypeId() != TYPEID_PLAYER)
                    return false;
                return true;
            }
    
            void CalculateAmountMeleeHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (!GetCaster() || !GetCaster()->GetOwner())
                    return;
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitMelee = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_HIT_CHANCE
                    HitMelee += owner->GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
                    // Increase hit melee from meele hit ratings
                    HitMelee += owner->GetRatingBonusValue(CR_HIT_MELEE);
    
                    amount += int32(HitMelee);
                }
            }
    
            void CalculateAmountSpellHit(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (!GetCaster() || !GetCaster()->GetOwner())
                    return;
                if (Player* owner = GetCaster()->GetOwner()->ToPlayer())
                {
                    // For others recalculate it from:
                    float HitSpell = 0.0f;
                    // Increase hit from SPELL_AURA_MOD_SPELL_HIT_CHANCE
                    HitSpell += owner->GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
                    // Increase hit spell from spell hit ratings
                    HitSpell += owner->GetRatingBonusValue(CR_HIT_SPELL);
    
                    amount += int32(HitSpell);
                }
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_pet_scaling_03_AuraScript::CalculateAmountMeleeHit, EFFECT_0, SPELL_AURA_MOD_HIT_CHANCE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_pet_scaling_03_AuraScript::CalculateAmountSpellHit, EFFECT_1, SPELL_AURA_MOD_SPELL_HIT_CHANCE);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_pet_scaling_03_AuraScript();
        }
};

class spell_dk_avoidance_passive : public SpellScriptLoader
{
    public:
        spell_dk_avoidance_passive() : SpellScriptLoader("spell_dk_avoidance_passive") { }
    
        class spell_dk_avoidance_passive_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_dk_avoidance_passive_AuraScript);
    
            bool Load() override
            {
                if (!GetCaster() || !GetCaster()->GetOwner() || GetCaster()->GetOwner()->GetTypeId() != TYPEID_PLAYER || GetCaster()->GetOwner()->getClass() != CLASS_DEATH_KNIGHT)
                    return false;
                return true;
            }
    
            void CalculateAvoidanceAmount(AuraEffect const* /* aurEff */, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Unit* pet = GetUnitOwner())
                {
                    if (Unit* owner = pet->GetOwner())
                    {
                        // Army of the dead ghoul
                        if (pet->GetEntry() == NPC_ARMY_OF_DEAD_GHOUL)
                            amount = -90;
                        // Night of the dead
                        else if (Aura* aur = owner->GetAuraOfRankedSpell(SPELL_NIGHT_OF_THE_DEAD))
                        {
                            amount = aur->GetSpellInfo()->Effects[EFFECT_2].CalcValue();
                            amount *= -1;
                        }
                    }
                }
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_avoidance_passive_AuraScript::CalculateAvoidanceAmount, EFFECT_0, SPELL_AURA_MOD_CREATURE_AOE_DAMAGE_AVOIDANCE);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_dk_avoidance_passive_AuraScript();
        }
};

enum HunterCower
{
    SPELL_IMPROVED_COWER_R1     = 53180,
    SPELL_IMPROVED_COWER_R2     = 53181
};

class spell_hun_cower_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_hun_cower_AuraScript);

    void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
    {
        if (Unit* pet = GetUnitOwner())
        {
            if (auto aurEff = pet->GetAuraEffect(SPELL_IMPROVED_COWER_R1, EFFECT_0))
                AddPct(amount, aurEff->CalculateAmount(pet));
            else if (auto aurEff = pet->GetAuraEffect(SPELL_IMPROVED_COWER_R2, EFFECT_0))
                AddPct(amount, aurEff->CalculateAmount(pet));
        }
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hun_cower_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_DECREASE_SPEED);
    }
};

void AddSC_pet_spell_scripts_rg()
{
    new spell_gen_pet_calculate();
    new spell_warl_pet_scaling_05();
    new spell_sha_pet_scaling_04();
    new spell_sha_fire_elemental_scaling();
    new spell_hun_pet_scaling_04();
    new spell_dk_pet_scaling_02();
    new spell_dk_pet_scaling_03();
    new spell_dk_avoidance_passive();
    new SpellScriptLoaderEx<spell_hun_cower_AuraScript>("spell_hun_cower");
}
