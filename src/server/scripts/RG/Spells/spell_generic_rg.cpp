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
 * Scripts for spells with SPELLFAMILY_GENERIC which cannot be included in AI script file
 * of creature using it or can't be bound to any player class.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_gen_"
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Group.h"
#include "Player.h"
#include "CreatureAI.h"
#include "InstanceScript.h"
#include "CreatureAIImpl.h"
#include "DBCStores.h"


class ToyTrainSetCheck
{
    public:
        explicit ToyTrainSetCheck() { }

        bool operator()(WorldObject* object)
        {
            Unit* unit = object->ToUnit();
            AuraEffect * aurEff = unit->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 3646, 1);
            return (unit->GetTypeId() != TYPEID_PLAYER) || (aurEff && aurEff->GetBase()->GetDuration() > 4000);
        }
};

class spell_instant_statue : public SpellScriptLoader
{
    public:
        spell_instant_statue() : SpellScriptLoader("spell_instant_statue") { }

        class spell_instant_statue_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_instant_statue_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                    if(Unit* instantStatue = caster->GetVehicleBase())
                    {
                        caster->ExitVehicle();

                        // Prevents Crashes if the Player logouts
                        if(instantStatue->ToCreature())
                            instantStatue->ToCreature()->RemoveFromWorld();
                    }
            }

            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_instant_statue_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_instant_statue_AuraScript();
        }
};

/*############################
# TCG: Summon Imp in a bottle
############################*/
#define IMP_NPC      23224
#define NR_OF_TEXTS  61

class spell_tcg_imp_in_a_bottle : public SpellScriptLoader
{
    public:
        spell_tcg_imp_in_a_bottle() : SpellScriptLoader("spell_tcg_imp_in_a_bottle") {}
    
        class spell_tcg_imp_in_a_bottle_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_tcg_imp_in_a_bottle_SpellScript)
    
            void HandleOnCast() {
    
                if (!GetCaster() || !GetCaster()->ToPlayer())
                    return;
    
                Player* caster = GetCaster()->ToPlayer();
                uint32 textID = urand(0, NR_OF_TEXTS-1);
    
                // Summon TriggerNPC because GOs can't whisper
                if (TempSummon* summon = caster->SummonCreature(IMP_NPC, caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ())) {
                    if (CreatureAI* summonAI = summon->AI()) {
    
                        summon->SetVisible(false);
    
                        // Player in Group => Whisper to all players in the group with the same message
                        // (NPCs can't write in the group chat)
                        if (Group* g = caster->GetGroup()) { // Case in Group
                            Group::MemberSlotList const& members = g->GetMemberSlots();
                            Group::MemberSlotList::const_iterator itr;
    
                            if (members.empty())
                                return;
    
                            for (itr = members.begin(); itr != members.end(); ++itr)
                                summonAI->Talk(textID, ObjectAccessor::GetPlayer(*summon, itr->guid));
                        } else { // Player not in group => Whisper to the player
                            summonAI->Talk(textID, caster);
                        }
                        summon->DespawnOrUnsummon();
                    }
                }
            }
    
            void Register() override
            {
                OnCast += SpellCastFn(spell_tcg_imp_in_a_bottle_SpellScript::HandleOnCast);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_tcg_imp_in_a_bottle_SpellScript();
        }
};

/*############################
# TCG: Goblin Gumbo Periodic
############################*/
class spell_tcg_goblin_gumbo : public SpellScriptLoader
{
    public:
        spell_tcg_goblin_gumbo() : SpellScriptLoader("spell_tcg_goblin_gumbo") { }

        class spell_tcg_goblin_gumbo_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_tcg_goblin_gumbo_AuraScript);

            // Spell should trigger in irregular intervals
            // => trigger it every 6sec and "kill" ~87% of all procs via this script
            void OnPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (urand(0,7))
                    PreventDefaultAction();
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_tcg_goblin_gumbo_AuraScript::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_tcg_goblin_gumbo_AuraScript();
        }
};

class spell_shadow_sickle : public SpellScriptLoader
{
    public:
        spell_shadow_sickle() : SpellScriptLoader("spell_shadow_sickle") { }

        class spell_shadow_sickle_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_shadow_sickle_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(56701) || !sSpellMgr->GetSpellInfo(59104))
                    return false;
                return true;
            }

            bool Load() override
            {
                triggerId = GetSpellInfo()->Id == 59103 ? 59104 : 56701;
                return (bool) triggerId;
            }

            void HandleSpellTrigger(AuraEffect const* /*aurEff*/)
            {
                if(Unit* target = GetTarget())
                {
                    if (UnitAI* ai = target->GetAI())
                    {
                        if (Unit* victim = ai->SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f))
                            target->CastSpell(victim, triggerId, true);
                    }
                }
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_shadow_sickle_AuraScript::HandleSpellTrigger, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
            
        private:
            uint32 triggerId;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_shadow_sickle_AuraScript();
        }
};

class spell_triple_slash : public SpellScriptLoader
{
    public:
        spell_triple_slash() : SpellScriptLoader("spell_triple_slash") { }

        class spell_triple_slash_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_triple_slash_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/) override
            {
                return (bool) sSpellMgr->GetSpellInfo(56645);
            }

            bool Load() override
            {
                triggerId = 56645;
                return true;
            }

            void HandleSpellTrigger(AuraEffect const* /*aurEff*/)
            {
                if(Unit* target = GetTarget())
                {
                    if (UnitAI* ai = target->GetAI())
                    {
                        if (Unit* victim = ai->SelectTarget(SELECT_TARGET_MAXTHREAT, 0, 40.0f))
                            target->CastSpell(victim, triggerId, true);
                    }
                }
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_triple_slash_AuraScript::HandleSpellTrigger, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
            
        private:
            uint32 triggerId;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_triple_slash_AuraScript();
        }
};

class OrientationCheck
{
    public:
        explicit OrientationCheck(Unit* _caster) : caster(_caster) { }
        bool operator()(WorldObject* object)
        {
            return !object->isInFront(caster, 2.5f) || !object->IsWithinDist(caster, 40.0f);
        }

    private:
        Unit* caster;
};

class spell_cleave : public SpellScriptLoader
{
    public:
        spell_cleave() : SpellScriptLoader("spell_cleave") { }
        
        class spell_cleave_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_cleave_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(OrientationCheck(GetCaster()));
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_cleave_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_cleave_SpellScript();
        }
};

class spell_arena_shaddow_sight : public SpellScriptLoader
{
    public:
        spell_arena_shaddow_sight() : SpellScriptLoader("spell_arena_shaddow_sight") { }

        class spell_arena_shaddow_sight_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_arena_shaddow_sight_AuraScript);

            void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                {
                    if (target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                        target->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
                }
            }

            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_arena_shaddow_sight_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_DETECT_STEALTH, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_arena_shaddow_sight_AuraScript();
        }
};

class spell_gen_aura_army_of_the_dead : public SpellScriptLoader
{
    public:
        spell_gen_aura_army_of_the_dead() : SpellScriptLoader("spell_gen_aura_army_of_the_dead") { }

        class spell_gen_aura_army_of_the_dead_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_aura_army_of_the_dead_AuraScript);

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* target = GetTarget())
                    if (target->FindNearestCreature(27604, 10.0f, true)) // skeleton
                        PreventDefaultAction();
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_aura_army_of_the_dead_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_aura_army_of_the_dead_AuraScript();
        }
};

enum SteamTonkData
{
    ENTRY_STEAM_TONK    = 19405,
    SPELL_CANNON        = 24933,
    SPELL_MINE          = 25099,
};

class OnlySteamTonkCheck
{
    public:
        bool operator()(WorldObject* object) const
        {
            return object->GetEntry() != ENTRY_STEAM_TONK;
        }
};

class spell_steam_tonk_abilities : public SpellScriptLoader
{
    public:
        spell_steam_tonk_abilities() : SpellScriptLoader("spell_steam_tonk_abilities") { }

        class spell_steam_tonk_abilities_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_steam_tonk_abilities_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(OnlySteamTonkCheck());
            }

            void Register() override
            {
                switch (m_scriptSpellId)
                {
                    case SPELL_CANNON:
                        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_steam_tonk_abilities_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENTRY);
                        break;
                    case SPELL_MINE:
                        //OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_steam_tonk_abilities_SpellScript::FilterTargets, EFFECT_0, TARGET_DEST_CASTER);
                        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_steam_tonk_abilities_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
                        break;
                    default:
                        break;
                }                
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_steam_tonk_abilities_SpellScript();
        }
};

enum SpectatorCheerTrigger
{
    EMOTE_ONE_SHOT_CHEER        = 4,
    EMOTE_ONE_SHOT_EXCLAMATION  = 5,
    EMOTE_ONE_SHOT_APPLAUD      = 21
};

uint8 const EmoteArray[3] = { EMOTE_ONE_SHOT_CHEER, EMOTE_ONE_SHOT_EXCLAMATION, EMOTE_ONE_SHOT_APPLAUD };

class spell_gen_spectator_cheer_trigger : public SpellScriptLoader
{
    public:
        spell_gen_spectator_cheer_trigger() : SpellScriptLoader("spell_gen_spectator_cheer_trigger") { }

        class spell_gen_spectator_cheer_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_spectator_cheer_trigger_SpellScript)

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                GetCaster()->HandleEmoteCommand(EmoteArray[urand(0, 2)]);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_spectator_cheer_trigger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_spectator_cheer_trigger_SpellScript();
        }
};

enum VendorBarkTrigger

{
    NPC_AMPHITHEATER_VENDOR     = 30098,
    SAY_AMPHITHEATER_VENDOR     = 0
};

class spell_gen_vendor_bark_trigger : public SpellScriptLoader
{
    public:
        spell_gen_vendor_bark_trigger() : SpellScriptLoader("spell_gen_vendor_bark_trigger") { }

        class spell_gen_vendor_bark_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_vendor_bark_trigger_SpellScript)

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                if (Creature* vendor = GetCaster()->ToCreature())
                    if (vendor->GetEntry() == NPC_AMPHITHEATER_VENDOR)
                        vendor->AI()->Talk(SAY_AMPHITHEATER_VENDOR);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_vendor_bark_trigger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_vendor_bark_trigger_SpellScript();
        }
};

class spell_gen_holy_strength : public SpellScriptLoader
{
    public:
        spell_gen_holy_strength() : SpellScriptLoader("spell_gen_holy_strength") { }

        class spell_gen_holy_strength_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_holy_strength_AuraScript);

            void HandleEffectCalcAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Unit* caster = GetCaster())
                {
                    int32 level = caster->getLevel();
                    if (level <= 60)
                        return;

                    AddPct(amount, -((level-60)*4));
                }
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_holy_strength_AuraScript::HandleEffectCalcAmount, EFFECT_0, SPELL_AURA_MOD_STAT);
            }
        };

        class spell_gen_holy_strength_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_holy_strength_SpellScript);

            bool Load() override
            {
                if (Unit* caster = GetCaster())
                {
                    int32 level = caster->getLevel();
                    if (level <= 60)
                        return true;

                    if (SpellValue* values = const_cast<SpellValue*>(GetSpellValue()))
                        AddPct(values->EffectBasePoints[EFFECT_1], -((level-60)*4));
                }
                return true;
            }

            void Register() override { }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_holy_strength_AuraScript();
        }

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_holy_strength_SpellScript();
        }
};

enum BladeWarding
{
    SPELL_BLADE_WARDING_DAMAGE = 64442
};

class spell_gen_blade_warding : public SpellScriptLoader
{
    public:
        spell_gen_blade_warding() : SpellScriptLoader("spell_gen_blade_warding") { }
    
        class spell_gen_blade_warding_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_blade_warding_AuraScript);
    
            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_BLADE_WARDING_DAMAGE))
                    return false;
                return true;
            }
    
            void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction(); // will prevent default effect execution
                Unit* target = GetTarget();
                int32 basepoints0 = 0;
    
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_BLADE_WARDING_DAMAGE);
                basepoints0 = int32(GetStackAmount() * spellInfo->Effects[EFFECT_0].CalcValue(GetTarget()));
    
                target->CastCustomSpell(eventInfo.GetProcTarget(), SPELL_BLADE_WARDING_DAMAGE, &basepoints0, NULL, NULL, true, NULL, aurEff);
            }
    
            void Register() override
            {
                OnEffectProc += AuraEffectProcFn(spell_gen_blade_warding_AuraScript::HandleEffectProc, EFFECT_1, SPELL_AURA_PROC_TRIGGER_SPELL);
            }
            
          };
    
          AuraScript* GetAuraScript() const override
          {
              return new spell_gen_blade_warding_AuraScript();
          }
};

class spell_gen_dream_funnel: public SpellScriptLoader
{
    public:
        spell_gen_dream_funnel() : SpellScriptLoader("spell_gen_dream_funnel") { }
    
        class spell_gen_dream_funnel_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_dream_funnel_AuraScript);
    
            void HandleEffectCalcAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
            {
                if (GetCaster())
                    amount = GetCaster()->GetMaxHealth() * 0.05f;
    
                canBeRecalculated = false;
            }
    
            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_dream_funnel_AuraScript::HandleEffectCalcAmount, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_dream_funnel_AuraScript::HandleEffectCalcAmount, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_dream_funnel_AuraScript();
        }
};

class spell_gen_touch_the_nightmare : public SpellScriptLoader
{
    public:
        spell_gen_touch_the_nightmare() : SpellScriptLoader("spell_gen_touch_the_nightmare") { }
    
        class spell_gen_touch_the_nightmare_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_touch_the_nightmare_SpellScript);
    
            void HandleDamageCalc(SpellEffIndex /*effIndex*/)
            {
                uint32 bp = GetCaster()->GetMaxHealth() * 0.3f;
                SetHitDamage(bp);
            }
    
            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_touch_the_nightmare_SpellScript::HandleDamageCalc, EFFECT_2, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_touch_the_nightmare_SpellScript();
        }
};

enum TossYouLuckMisc
{
    SAY_FLIP_SEAL = 130017,
    SAY_HEAD      = 130020,
    SAY_NUMBER    = 130021
};

class spell_gen_toss_your_luck: public SpellScriptLoader
{
    public:
        spell_gen_toss_your_luck() : SpellScriptLoader("spell_gen_toss_your_luck") { }

        class spell_gen_toss_your_luck_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_toss_your_luck_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (urand(0, 99) < 50)
                    {
                        caster->MonsterTextEmote(sObjectMgr->GetTrinityStringForDBCLocale(SAY_FLIP_SEAL), caster, false);
                        caster->MonsterTextEmote(sObjectMgr->GetTrinityStringForDBCLocale(SAY_HEAD), caster, false);
                    }
                    else
                    {
                        caster->MonsterTextEmote(sObjectMgr->GetTrinityStringForDBCLocale(SAY_FLIP_SEAL), caster, false);
                        caster->MonsterTextEmote(sObjectMgr->GetTrinityStringForDBCLocale(SAY_NUMBER), caster, false);
                    }
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_gen_toss_your_luck_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_toss_your_luck_SpellScript();
        }
};

enum ApexisSpellMisc
{
    SPELL_APEXIS_VIBRATION      = 40623,
    SPELL_SWIFTNESS_VIBRATION   = 40624,
    SPELL_APEXIS_EMANATION      = 40625,
    SPELL_SWIFTNESS_EMANATION   = 40627,
    SPELL_APEXIS_ENLIGHTMENT    = 40626,
    SPELL_SWIFTNESS_ENLIGHTMENT = 40628,
    AREA_BLADE_EDGE_MOUNTAINS   = 3522,
    AREA_GRUULS_LAIR            = 3923
};

class spell_gen_apexis_buff : public SpellScriptLoader
{
    public:
        spell_gen_apexis_buff() : SpellScriptLoader("spell_gen_apexis_buff") { }
        
        class spell_gen_apexis_buff_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_apexis_buff_AuraScript);
            
            bool Validate(SpellInfo const* /*entry*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SWIFTNESS_VIBRATION) || !sSpellMgr->GetSpellInfo(SPELL_SWIFTNESS_EMANATION) || !sSpellMgr->GetSpellInfo(SPELL_SWIFTNESS_ENLIGHTMENT))
                    return false;
                return true;
            }
            
            bool Load() override
            {
                switch (GetId())
                {
                    case SPELL_APEXIS_VIBRATION:
                        _swiftness = SPELL_SWIFTNESS_VIBRATION;
                        break;
                    case SPELL_APEXIS_EMANATION:
                        _swiftness = SPELL_SWIFTNESS_EMANATION;
                        break;
                    case SPELL_APEXIS_ENLIGHTMENT:
                        _swiftness = SPELL_SWIFTNESS_ENLIGHTMENT;
                        break;
                    default:
                        return false;
                }
                return GetUnitOwner()->GetTypeId() == TYPEID_PLAYER;
            }
            
            // Add Update for Areacheck
            void CalcPeriodic(AuraEffect const* /*effect*/, bool& isPeriodic, int32& amplitude)
            {
                isPeriodic = true;
                amplitude = 5 * IN_MILLISECONDS;
            }
            
            void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->RemoveAurasDueToSpell(SPELL_SWIFTNESS_VIBRATION);
                target->RemoveAurasDueToSpell(SPELL_SWIFTNESS_EMANATION);
                target->RemoveAurasDueToSpell(SPELL_SWIFTNESS_ENLIGHTMENT);
                target->AddAura(_swiftness, target);
            }
           
            void Update(AuraEffect* /*effect*/)
            {
                if (Player* owner = GetUnitOwner()->ToPlayer())
                {
                    if ((owner->GetZoneId() == AREA_BLADE_EDGE_MOUNTAINS) || (owner->GetZoneId() == AREA_GRUULS_LAIR))
                        owner->AddAura(_swiftness, owner);
                    else
                        owner->RemoveAura(_swiftness);
                }                    
            }
           
            void Register() override
            {
                AfterEffectApply += AuraEffectApplyFn(spell_gen_apexis_buff_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_gen_apexis_buff_AuraScript::CalcPeriodic, EFFECT_0, SPELL_AURA_DUMMY);
                OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_gen_apexis_buff_AuraScript::Update, EFFECT_0, SPELL_AURA_DUMMY);
            }
            
        private:
            uint32 _swiftness;
        };
        
        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_apexis_buff_AuraScript();
        }
};

class spell_gen_shadowmeld : public SpellScriptLoader
{
    public:
        spell_gen_shadowmeld() : SpellScriptLoader("spell_gen_shadowmeld") {}

        class spell_gen_shadowmeld_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_shadowmeld_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit *caster = GetCaster();
                if (!caster)
                    return;

                caster->InterruptSpell(CURRENT_AUTOREPEAT_SPELL); // break Auto Shot and autohit
                caster->InterruptSpell(CURRENT_CHANNELED_SPELL); // break channeled spells

                bool instant_exit = true;
                if (Player *pCaster = caster->ToPlayer()) // if is a creature instant exits combat, else check if someone in party is in combat in visibility distance
                {
                    ObjectGuid myGUID = pCaster->GetGUID();
                    float visibilityRange = pCaster->GetMap()->GetVisibilityRange();
                    if (Group *pGroup = pCaster->GetGroup())
                    {
                        const Group::MemberSlotList membersList = pGroup->GetMemberSlots();
                        for (Group::member_citerator itr = membersList.begin(); itr != membersList.end() && instant_exit; ++itr)
                            if (itr->guid != myGUID)
                                if (Player *GroupMember = ObjectAccessor::GetPlayer(*pCaster, itr->guid))
                                    if (GroupMember->IsInCombat() && pCaster->GetMap() == GroupMember->GetMap() && pCaster->IsWithinDistInMap(GroupMember, visibilityRange))
                                        instant_exit = false;
                    }

                    pCaster->SendAttackSwingCancelAttack();
                }

                if (!caster->GetInstanceScript() || !caster->GetInstanceScript()->IsEncounterInProgress()) //Don't leave combat if you are in combat with a boss
                {
                    if (!instant_exit)
                        caster->getHostileRefManager().deleteReferences(); // exit combat after 6 seconds
                    else caster->CombatStop(); // isn't necessary to call AttackStop because is just called in CombatStop
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_shadowmeld_SpellScript::HandleDummy, EFFECT_1, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_shadowmeld_SpellScript();
        }
};

enum Spells_Triggered_Paralytic_Poison
{
    SPELL_PARALYZE = 35202
};

class spell_gen_paralytic_poison : public SpellScriptLoader
{
    public:
        spell_gen_paralytic_poison() : SpellScriptLoader("spell_gen_paralytic_poison") { }
    
        class spell_gen_paralytic_poison_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_paralytic_poison_AuraScript);
    
            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
            {
                if (GetDuration() == 0)
                    if (Unit* target = GetTarget())
                        target->CastSpell(target, SPELL_PARALYZE, true);
            }
    
            void Register() override
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_gen_paralytic_poison_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_paralytic_poison_AuraScript();
        }
};

enum ApprenticeRidingMisc
{
    ACHIEVEMENT_GIDDY_UP = 891
};

class spell_gen_apprentice_riding : public SpellScriptLoader
{
    public:
        spell_gen_apprentice_riding() : SpellScriptLoader("spell_gen_apprentice_riding") { }

        class spell_gen_apprentice_riding_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_apprentice_riding_SpellScript);

            void HandleDummyHitTarget(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                    target->ToPlayer()->CompletedAchievement(sAchievementStore.LookupEntry(ACHIEVEMENT_GIDDY_UP));
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_apprentice_riding_SpellScript::HandleDummyHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_apprentice_riding_SpellScript();
        }
};

enum CurseoftheEyeMisc
{
    SPELL_CURSE_FEMALE = 10653,
    SPELL_CURSE_MALE   = 10651
};

class spell_curse_of_the_eye : public SpellScriptLoader
{
    public:
        spell_curse_of_the_eye() : SpellScriptLoader("spell_curse_of_the_eye") { }

        class spell_curse_of_the_eye_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_curse_of_the_eye_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                GetCaster()->CastSpell(GetHitUnit(), GetHitUnit()->getGender() == GENDER_FEMALE ? SPELL_CURSE_FEMALE : SPELL_CURSE_MALE, true);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_curse_of_the_eye_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_curse_of_the_eye_SpellScript();

        }
};

class spell_gen_reduced_above_60 : public SpellScriptLoader
{
    public:
        spell_gen_reduced_above_60() : SpellScriptLoader("spell_gen_reduced_above_60") { }

        class spell_gen_reduced_above_60_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_reduced_above_60_SpellScript);

            void RecalculateDamage()
            {
                if (Unit* target = GetHitUnit())
                    if (target->getLevel() > 60)
                    {
                        int32 damage = GetHitDamage();
                        AddPct(damage, -4 * int8(std::min(target->getLevel(), uint8(85)) - 60)); // prevents reduce by more than 100%
                        SetHitDamage(damage);
                    }
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_gen_reduced_above_60_SpellScript::RecalculateDamage);
            }
        };

        class spell_gen_reduced_above_60_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_reduced_above_60_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & canBeRecalculated)
            {
                if (Unit* owner = GetUnitOwner())
                    if (owner->getLevel() > 60)
                        AddPct(amount, -4 * int8(std::min(owner->getLevel(), uint8(85)) - 60)); // prevents reduce by more than 100%
            }

            void Register() override
            {
                if (m_scriptSpellId != 20004) // Lifestealing enchange - no aura effect
                    DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_reduced_above_60_AuraScript::CalculateAmount, EFFECT_ALL, SPELL_AURA_ANY);
            }
        };
        
        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_reduced_above_60_SpellScript();
        }

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_reduced_above_60_AuraScript();
        }
};

class spell_gen_grow_flower_patch : public SpellScriptLoader
{
    public:
        spell_gen_grow_flower_patch() : SpellScriptLoader("spell_gen_grow_flower_patch") { }

        class spell_gen_grow_flower_patch_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_grow_flower_patch_SpellScript)

            SpellCastResult CheckCast()
            {
                if (GetCaster()->HasAuraType(SPELL_AURA_MOD_STEALTH) || GetCaster()->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
                    return SPELL_FAILED_DONT_REPORT;

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_gen_grow_flower_patch_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_grow_flower_patch_SpellScript();
        }
};

class spell_gen_disabled_above_63 : public SpellScriptLoader
{
    public:
        spell_gen_disabled_above_63() : SpellScriptLoader("spell_gen_disabled_above_63") { }

        class spell_gen_disabled_above_63_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_disabled_above_63_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & canBeRecalculated)
            {
                Unit* target = GetUnitOwner();
                if (target->getLevel() <= 63)
                    amount = amount * target->getLevel() / 60;
                else
                    SetDuration(1);
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_gen_disabled_above_63_AuraScript::CalculateAmount, EFFECT_ALL, SPELL_AURA_ANY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_disabled_above_63_AuraScript();
        }
};

class spell_gen_black_magic_enchant : public SpellScriptLoader
{
    public:
        spell_gen_black_magic_enchant() : SpellScriptLoader("spell_gen_black_magic_enchant") { }

        class spell_gen_black_magic_enchant_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_black_magic_enchant_AuraScript);

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                SpellInfo const* spellInfo = eventInfo.GetDamageInfo()->GetSpellInfo();
                if (!spellInfo)
                    return false;

                // Old 'code'
                if (eventInfo.GetTypeMask() & PROC_FLAG_DONE_SPELL_MAGIC_DMG_CLASS_NEG)
                    return true;

                // Implement Black Magic enchant Bug
                if (spellInfo->SpellFamilyName == SPELLFAMILY_DRUID)
                    if (spellInfo->SpellIconID == 2857 /*SPELL_INFECTED_WOUNDS*/ || spellInfo->SpellIconID == 147 /*SPELL_SHRED*/ || spellInfo->SpellFamilyFlags[1] & 0x400 /*SPELL_MANGLE_(CAT)*/ || spellInfo->SpellIconID == 261 /*SPELL_MAUL*/)
                        return true;

                return false;
            }

            void Register()
            {
                DoCheckProc += AuraCheckProcFn(spell_gen_black_magic_enchant_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_black_magic_enchant_AuraScript();
        }
};

class spell_gen_area_aura_select_players : public SpellScriptLoader
{
    public:
        spell_gen_area_aura_select_players() : SpellScriptLoader("spell_gen_area_aura_select_players") { }

        class spell_gen_area_aura_select_players_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_area_aura_select_players_AuraScript);

            bool CheckAreaTarget(Unit* target)
            {
                return target->GetTypeId() == TYPEID_PLAYER;
            }

            void Register() override
            {
                DoCheckAreaTarget += AuraCheckAreaTargetFn(spell_gen_area_aura_select_players_AuraScript::CheckAreaTarget);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_area_aura_select_players_AuraScript();
        }
};

class spell_gen_proc_from_direct_damage : public SpellScriptLoader
{
    public:
        spell_gen_proc_from_direct_damage() : SpellScriptLoader("spell_gen_proc_from_direct_damage") { }

        class spell_gen_proc_from_direct_damage_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_proc_from_direct_damage_AuraScript);

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                return !eventInfo.GetDamageInfo()->GetSpellInfo() || !eventInfo.GetDamageInfo()->GetSpellInfo()->IsTargetingArea();
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_gen_proc_from_direct_damage_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_proc_from_direct_damage_AuraScript();
        }
};

class spell_gen_no_offhand_proc : public SpellScriptLoader
{
    public:
        spell_gen_no_offhand_proc() : SpellScriptLoader("spell_gen_no_offhand_proc") { }

        class spell_gen_no_offhand_proc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_no_offhand_proc_AuraScript);

            bool CheckProc(ProcEventInfo& eventInfo)
            {
                return !(eventInfo.GetTypeMask() & PROC_FLAG_DONE_OFFHAND_ATTACK);
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_gen_no_offhand_proc_AuraScript::CheckProc);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_no_offhand_proc_AuraScript();
        }
};

class spell_gen_visual_dummy_stun : public SpellScriptLoader
{
    public:
        spell_gen_visual_dummy_stun() : SpellScriptLoader("spell_gen_visual_dummy_stun") { }
    
        class spell_gen_visual_dummy_stun_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_visual_dummy_stun_AuraScript);
    
            void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->SetControlled(true, UNIT_STATE_STUNNED);
            }
    
            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                target->SetControlled(false, UNIT_STATE_STUNNED);
            }
    
            void Register() override
            {
                OnEffectApply += AuraEffectApplyFn(spell_gen_visual_dummy_stun_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_gen_visual_dummy_stun_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
    
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_gen_visual_dummy_stun_AuraScript();
        }
};

class spell_gen_npc_charm_check : public SpellScriptLoader
{
    public:
        spell_gen_npc_charm_check() : SpellScriptLoader("spell_gen_npc_charm_check") { }

        class spell_gen_npc_charm_check_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_npc_charm_check_SpellScript)

            SpellCastResult CheckCast()
            {
                if (GetCaster()->CanHaveThreatList())
                if (auto creature = GetCaster()->ToCreature())
                {
                    uint32 count = 0;
                    auto& threatList = creature->GetThreatManager().getThreatList();
                    for (auto itr : threatList)
                    {
                        auto target = ObjectAccessor::GetUnit(*creature, itr->getUnitGuid());
                        if (target && !target->IsPet())
                            if (++count >= 2)
                                return SPELL_CAST_OK;
                    }
                    return SPELL_FAILED_DONT_REPORT;
                }

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_gen_npc_charm_check_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_gen_npc_charm_check_SpellScript();
        }
};

class spell_profession_fishing : public SpellScriptLoader
{
public:
    spell_profession_fishing() : SpellScriptLoader("spell_profession_fishing") { }

    class spell_profession_fishing_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_profession_fishing_SpellScript)

        SpellCastResult CheckCast()
        {
            if (GetCaster() && GetCaster()->GetMap() && GetCaster()->GetMap()->IsBattleground())
                return SPELL_FAILED_NO_FISH;

            return SPELL_CAST_OK;
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_profession_fishing_SpellScript::CheckCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_profession_fishing_SpellScript();
    }
};

class spell_baby_murloc_dance_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_baby_murloc_dance_AuraScript);

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* target = GetTarget())
            target->HandleEmoteCommand(EMOTE_STATE_DANCE);
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* target = GetTarget())
            target->HandleEmoteCommand(EMOTE_ONESHOT_NONE);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_baby_murloc_dance_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(spell_baby_murloc_dance_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

enum AuraOfMadnessSpells
{
    SPELL_SOCIOPATH         = 39511,
    SPELL_DELUSIONAL        = 40997,
    SPELL_KLEPTOMANIA       = 40998,
    SPELL_MEGALOMANIA       = 40999,
    SPELL_PARANOIA          = 41002,
    SPELL_MANIC             = 41005,
    SPELL_NARCISSISM        = 41009,
    SPELL_MARTYR_COMPLEX    = 41011,
    SPELL_DEMENTIA_INC      = 41406,
    SPELL_DEMENTIA_DEC      = 41409,
};

class spell_gen_aura_of_madnessAuraScript : public AuraScript
{
    PrepareAuraScript(spell_gen_aura_of_madnessAuraScript);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& procInfo)
    {
        if (!GetCaster())
            return;

        if (Player* player = GetCaster()->ToPlayer())
        {
            switch (player->getClass())
            {
            case CLASS_PALADIN:
            case CLASS_DRUID:
                player->CastSpell(player,RAND(SPELL_SOCIOPATH, SPELL_DELUSIONAL, SPELL_KLEPTOMANIA, SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA_DEC));
                break;
            case CLASS_ROGUE:
            case CLASS_WARRIOR:
            case CLASS_DEATH_KNIGHT:
                player->CastSpell(player, RAND(SPELL_SOCIOPATH, SPELL_DELUSIONAL, SPELL_KLEPTOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_MARTYR_COMPLEX));
                break;
            case CLASS_PRIEST:
            case CLASS_SHAMAN:
            case CLASS_MAGE:
            case CLASS_WARLOCK:
                player->CastSpell(player, RAND(SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA_INC, SPELL_DEMENTIA_DEC));
                break;
            case CLASS_HUNTER:
                player->CastSpell(player, RAND(SPELL_DELUSIONAL, SPELL_MEGALOMANIA, SPELL_PARANOIA, SPELL_MANIC, SPELL_NARCISSISM, SPELL_MARTYR_COMPLEX, SPELL_DEMENTIA_INC, SPELL_DEMENTIA_DEC));
                break;
            default:
                return;
            }
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_gen_aura_of_madnessAuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

enum UnstablePowerSpells
{
    SPELL_UNSTABLE_POWER_BUFF = 24659,
};

class spell_gen_unstable_powerAuraScript : public AuraScript
{
    PrepareAuraScript(spell_gen_unstable_powerAuraScript);

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetCaster())
            return;

        for (uint8 i = 0; i < 12; ++i)
            GetCaster()->CastSpell(GetCaster(), SPELL_UNSTABLE_POWER_BUFF, true);
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetCaster())
            return;

        GetCaster()->RemoveAurasDueToSpell(SPELL_UNSTABLE_POWER_BUFF);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_gen_unstable_powerAuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(spell_gen_unstable_powerAuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_gen_unstable_power_auraAuraScript : public AuraScript
{
    PrepareAuraScript(spell_gen_unstable_power_auraAuraScript);

    void HandleProc(ProcEventInfo& /*procInfo*/)
    {
        if (!GetCaster())
            return;

        if (Aura* aura = GetCaster()->GetAura(SPELL_UNSTABLE_POWER_BUFF))
        {
            aura->ModStackAmount(-1);
        }
    }

    void Register() override
    {
        OnProc += AuraProcFn(spell_gen_unstable_power_auraAuraScript::HandleProc);
    }
};

enum RestlessStrengthSpells
{
    SPELL_RESTLESS_STRENGTH_BUFF = 24662,
};

class spell_gen_restless_strenghtAuraScript : public AuraScript
{
    PrepareAuraScript(spell_gen_restless_strenghtAuraScript);

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetCaster())
            return;

        for (uint8 i = 0; i < 20; ++i)
            GetCaster()->CastSpell(GetCaster(), SPELL_RESTLESS_STRENGTH_BUFF, true);
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (!GetCaster())
            return;

        GetCaster()->RemoveAurasDueToSpell(SPELL_RESTLESS_STRENGTH_BUFF);
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& procInfo)
    {
        if (Aura* aura = GetCaster()->GetAura(SPELL_RESTLESS_STRENGTH_BUFF))
            aura->ModStackAmount(-1);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_gen_restless_strenghtAuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(spell_gen_restless_strenghtAuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectProc += AuraEffectProcFn(spell_gen_restless_strenghtAuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

enum SunwellExaltedCasterNeckMisc
{
    SPELL_SUNWELL_EXALTED_CASTER_NECK   = 45481,
    SPELL_SUNWELL_EXALTED_MEELE_NECK    = 45482,
    SPELL_SUNWELL_EXALTED_TANK_NECK     = 45483,
    SPELL_SUNWELL_EXALTED_HEALER_NECK   = 45484,
    SPELL_LIGHTS_STRENGTH               = 45480,
    SPELL_ARCANE_STRIKE                 = 45428,
    SPELL_ARCANE_INSIGHT                = 45431,
    SPELL_LIGHTS_WARD                   = 45432,
    SPELL_LIGHTS_SALVATION              = 45478,
    SPELL_ARCANE_SURGE                  = 45430,
    SPELL_LIGHTS_WRATH                  = 45479,
    SPELL_ACRANE_BOLT                   = 45429,
    FACTION_SCRYERS                     = 934,
    FACTION_ALDOR                       = 932,
};

class spell_gen_sunwell_exalted_neckAuraScript : public AuraScript
{
    PrepareAuraScript(spell_gen_sunwell_exalted_neckAuraScript);

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& procInfo)
    {
        if (!GetCaster())
            return;

        if (Player* player = GetCaster()->ToPlayer())
        {
            if (player->GetReputationRank(FACTION_ALDOR) == REP_EXALTED)
            {
                switch (aurEff->GetId())
                {
                    case SPELL_SUNWELL_EXALTED_CASTER_NECK:
                        player->CastSpell(player, SPELL_LIGHTS_WRATH);
                        break;
                    case SPELL_SUNWELL_EXALTED_MEELE_NECK:
                        player->CastSpell(player, SPELL_LIGHTS_STRENGTH);
                        break;
                    case SPELL_SUNWELL_EXALTED_TANK_NECK:
                        player->CastSpell(player, SPELL_ARCANE_INSIGHT);
                        break;
                    case SPELL_SUNWELL_EXALTED_HEALER_NECK:
                        player->CastSpell(player, SPELL_LIGHTS_SALVATION);
                        break;
                    default:
                        break;
                }
            }
            else if (player->GetReputationRank(FACTION_SCRYERS) == REP_EXALTED)
            {
                switch (aurEff->GetId())
                {
                    case SPELL_SUNWELL_EXALTED_CASTER_NECK:
                        player->CastSpell(player->GetVictim(), SPELL_ACRANE_BOLT);
                        break;
                    case SPELL_SUNWELL_EXALTED_MEELE_NECK:
                        player->CastSpell(player->GetVictim(), SPELL_ARCANE_STRIKE);
                        break;
                    case SPELL_SUNWELL_EXALTED_TANK_NECK:
                        player->CastSpell(player, SPELL_LIGHTS_WARD);
                        break;
                    case SPELL_SUNWELL_EXALTED_HEALER_NECK:
                        player->CastSpell(player->GetVictim(), SPELL_ARCANE_SURGE);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_gen_sunwell_exalted_neckAuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

void AddSC_generic_spell_scripts_rg()
{
    new spell_shadow_sickle();
    new spell_triple_slash();
    new spell_instant_statue();
    new spell_tcg_imp_in_a_bottle();
    new spell_tcg_goblin_gumbo();
    new spell_cleave();
    new spell_arena_shaddow_sight();
    new spell_gen_aura_army_of_the_dead();
    new spell_steam_tonk_abilities();
    new spell_gen_spectator_cheer_trigger();
    new spell_gen_vendor_bark_trigger();
    new spell_gen_holy_strength();
    new spell_gen_blade_warding();
    new spell_gen_dream_funnel();
    new spell_gen_touch_the_nightmare();
    new spell_gen_toss_your_luck();
    new spell_gen_apexis_buff();
    new spell_gen_shadowmeld();
    new spell_gen_paralytic_poison();
    new spell_gen_apprentice_riding();
    new spell_curse_of_the_eye();
    new spell_gen_reduced_above_60();
    new spell_gen_grow_flower_patch();
    new spell_gen_disabled_above_63();
    new spell_gen_black_magic_enchant();
    new spell_gen_area_aura_select_players();
    new spell_gen_proc_from_direct_damage();
    new spell_gen_no_offhand_proc();
    new spell_gen_visual_dummy_stun();
    new spell_gen_npc_charm_check();
    new spell_profession_fishing();
    new SpellScriptLoaderEx<spell_baby_murloc_dance_AuraScript>("spell_baby_murloc_dance");
    new SpellScriptLoaderEx<spell_gen_aura_of_madnessAuraScript>("spell_gen_aura_of_madness");
    new SpellScriptLoaderEx<spell_gen_unstable_powerAuraScript>("spell_gen_unstable_power");
    new SpellScriptLoaderEx<spell_gen_unstable_power_auraAuraScript>("spell_gen_unstable_power_aura");
    new SpellScriptLoaderEx<spell_gen_restless_strenghtAuraScript>("spell_gen_restless_strength");
    new SpellScriptLoaderEx<spell_gen_sunwell_exalted_neckAuraScript>("spell_gen_sunwell_exalted_neck");
}
