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
 * Scripts for spells with SPELLFAMILY_GENERIC spells used for quests.
 * Ordered alphabetically using questId and scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_q#questID_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "Player.h"
#include "CreatureAI.h"

// 9712 - Thaumaturgy Channel
enum ThaumaturgyChannel
{
    SPELL_THAUMATURGY_CHANNEL = 21029
};

class spell_q2203_thaumaturgy_channel : public SpellScriptLoader
{
    public:
        spell_q2203_thaumaturgy_channel() : SpellScriptLoader("spell_q2203_thaumaturgy_channel") { }

        class spell_q2203_thaumaturgy_channel_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_q2203_thaumaturgy_channel_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_THAUMATURGY_CHANNEL))
                    return false;
                return true;
            }

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                PreventDefaultAction();
                if (Unit* caster = GetCaster())
                    caster->CastSpell(caster, SPELL_THAUMATURGY_CHANNEL, false);
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_q2203_thaumaturgy_channel_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_q2203_thaumaturgy_channel_AuraScript();
        }
};

/*########################
# Quest It Rolls Downhill
#########################*/
enum ItRollsDownhill
{
    GAMEOBJECT_CRYSTALLIZED_BLIGHT_1    = 190716,
    GAMEOBJECT_CRYSTALLIZED_BLIGHT_2    = 190939,
    GAMEOBJECT_CRYSTALLIZED_BLIGHT_3    = 190940,
    NPC_BLIGHT_CRYSTAL_KILL_KREDIT      = 28740,
};

class spell_q12673_harvest_blight_crystal : public SpellScriptLoader
{
    public:
        spell_q12673_harvest_blight_crystal() : SpellScriptLoader("spell_q12673_harvest_blight_crystal") { }
    
        class spell_q12673_harvest_blight_crystal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12673_harvest_blight_crystal_SpellScript)
    
            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Creature* caster = GetCaster()->ToCreature();
                if (!caster)
                    return;
    
                Unit* owner = caster->GetCharmerOrOwner();
                if (!owner)
                    return;
    
                Player* plr = owner->ToPlayer();
                if (!plr)
                    return;
    
                GameObject* crystal;
                if ((crystal = caster->FindNearestGameObject(GAMEOBJECT_CRYSTALLIZED_BLIGHT_1, 15.0f)) ||
                    (crystal = caster->FindNearestGameObject(GAMEOBJECT_CRYSTALLIZED_BLIGHT_2, 15.0f)) ||
                    (crystal = caster->FindNearestGameObject(GAMEOBJECT_CRYSTALLIZED_BLIGHT_3, 15.0f)))
                {
                    caster->GetMotionMaster()->MovePoint(0, crystal->GetPositionX(), crystal->GetPositionY(), crystal->GetPositionZ());
                    crystal->DespawnOrUnsummon();
                    caster->DespawnOrUnsummon(1000);
                    plr->KilledMonsterCredit(NPC_BLIGHT_CRYSTAL_KILL_KREDIT);
                }
            }
    
            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_q12673_harvest_blight_crystal_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_q12673_harvest_blight_crystal_SpellScript();
        }
};

class spell_q12661_q12669_q12676_q12677_q12713_summon_stefan : public SpellScriptLoader
{
    public:
        spell_q12661_q12669_q12676_q12677_q12713_summon_stefan() : SpellScriptLoader("spell_q12661_q12669_q12676_q12677_q12713_summon_stefan") { }

        class spell_q12661_q12669_q12676_q12677_q12713_summon_stefan_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12661_q12669_q12676_q12677_q12713_summon_stefan_SpellScript);

            void ChangeSummonPos(SpellEffIndex /*effIndex*/)
            {
                // Adjust effect summon position
                WorldLocation summonPos = *GetExplTargetDest();
                Position offset = { 0.0f, 0.0f, 20.0f, 0.0f };
                summonPos.RelocateOffset(offset);
                SetExplTargetDest(summonPos);
                GetHitDest()->RelocateOffset(offset);
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_q12661_q12669_q12676_q12677_q12713_summon_stefan_SpellScript::ChangeSummonPos, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q12661_q12669_q12676_q12677_q12713_summon_stefan_SpellScript();
        }
};

enum QuenchingMist
{
    SPELL_FLICKERING_FLAMES = 53504
};

class spell_q12730_quenching_mist : public SpellScriptLoader
{
    public:
        spell_q12730_quenching_mist() : SpellScriptLoader("spell_q12730_quenching_mist") { }

        class spell_q12730_quenching_mist_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_q12730_quenching_mist_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_FLICKERING_FLAMES))
                    return false;
                return true;
            }

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                GetTarget()->RemoveAurasDueToSpell(SPELL_FLICKERING_FLAMES);
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_q12730_quenching_mist_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_q12730_quenching_mist_AuraScript();
        }
};

class spell_q12847_summon_soul_moveto_bunny : public SpellScriptLoader
{
    public:
        spell_q12847_summon_soul_moveto_bunny() : SpellScriptLoader("spell_q12847_summon_soul_moveto_bunny") { }

        class spell_q12847_summon_soul_moveto_bunny_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12847_summon_soul_moveto_bunny_SpellScript);

            void ChangeSummonPos(SpellEffIndex /*effIndex*/)
            {
                // Adjust effect summon position
                WorldLocation summonPos = *GetExplTargetDest();
                Position offset = { 0.0f, 0.0f, 2.5f, 0.0f };
                summonPos.RelocateOffset(offset);
                SetExplTargetDest(summonPos);
                GetHitDest()->RelocateOffset(offset);
            }

            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_q12847_summon_soul_moveto_bunny_SpellScript::ChangeSummonPos, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript *GetSpellScript() const override
        {
            return new spell_q12847_summon_soul_moveto_bunny_SpellScript();
        }
};

enum SpellPlantWarmaulOgreBanner
{
    // NPCs
    NPC_KILSORROW_SPELLBINDER = 17146,
    NPC_KILSORROW_CULTIST     = 17147,
    NPC_KILSORROW_DEATHWORN   = 17148,
    NPC_KILSORROW_RITUALIST   = 18658,
    NPC_GISELDA_THE_CRONE     = 18391
};

// 32307 - Plant Warmaul Ogre Banner
class spell_q9927_nagrand_plant_warmaul_ogre_banner : public SpellScriptLoader
{
    public:
        spell_q9927_nagrand_plant_warmaul_ogre_banner() : SpellScriptLoader("spell_q9927_nagrand_plant_warmaul_ogre_banner") { }

        class spell_q9927_nagrand_plant_warmaul_ogre_banner_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q9927_nagrand_plant_warmaul_ogre_banner_SpellScript);

            SpellCastResult CheckRequirement()
            {
               if (!GetExplTargetUnit())
                   return SPELL_FAILED_BAD_TARGETS;

               if (Unit* target = GetExplTargetUnit())
                   if (target->IsAlive())
                       return SPELL_FAILED_BAD_TARGETS;

               if (Unit* target = GetExplTargetUnit())
                   if (target->GetEntry() != NPC_KILSORROW_SPELLBINDER && target->GetEntry() != NPC_KILSORROW_CULTIST &&
                       target->GetEntry() != NPC_KILSORROW_DEATHWORN && target->GetEntry() != NPC_KILSORROW_RITUALIST && target->GetEntry() != NPC_GISELDA_THE_CRONE)
                       return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_q9927_nagrand_plant_warmaul_ogre_banner_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q9927_nagrand_plant_warmaul_ogre_banner_SpellScript();
        }
};

// Spell: 32314 Plant Kil'sorrow Banner
enum SpellPlantKilsorrowBanner
{
    // NPCs
    NPC_WARMAUL_REAVER = 17138,
    NPC_WARMAUL_SHAMAN = 18064
};

class spell_q9931_nagrand_plant_kil_sorrow_banner : public SpellScriptLoader
{
    public:
        spell_q9931_nagrand_plant_kil_sorrow_banner() : SpellScriptLoader("spell_q9931_nagrand_plant_kil_sorrow_banner") { }

        class spell_q9931_nagrand_plant_kil_sorrow_banner_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q9931_nagrand_plant_kil_sorrow_banner_SpellScript);

            void HandleAfterCast()
            {
                if (Creature* reaver = GetCaster()->FindNearestCreature(NPC_WARMAUL_REAVER, 3.0f, false))
                {
                    reaver->DespawnOrUnsummon(0);
                }
                else if (Creature* shaman = GetCaster()->FindNearestCreature(NPC_WARMAUL_SHAMAN, 3.0f, false))
                {
                    shaman->DespawnOrUnsummon(0);
                }
            }

            SpellCastResult CheckRequirement()
            {
                if (GetCaster()->FindNearestCreature(NPC_WARMAUL_REAVER, 3.0f, false)
                 || GetCaster()->FindNearestCreature(NPC_WARMAUL_SHAMAN, 3.0f, false))
                    return SPELL_CAST_OK;
                else
                    return SPELL_FAILED_NO_VALID_TARGETS;
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_q9931_nagrand_plant_kil_sorrow_banner_SpellScript::HandleAfterCast);
                OnCheckCast += SpellCheckCastFn(spell_q9931_nagrand_plant_kil_sorrow_banner_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q9931_nagrand_plant_kil_sorrow_banner_SpellScript();
        }
};

class spell_q12214_fresh_remounts : public SpellScriptLoader
{
    public:
        spell_q12214_fresh_remounts() : SpellScriptLoader("spell_q12214_fresh_remounts") { }

        class spell_q12214_fresh_remounts_SpellScript : public SpellScript
        {
           PrepareSpellScript(spell_q12214_fresh_remounts_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (caster->IsVehicle())
                   if (Unit* passenger = caster->GetCharmerOrOwner())
                    {
                         passenger->ExitVehicle();
                         caster->ToCreature()->DespawnOrUnsummon();
                    }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_q12214_fresh_remounts_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q12214_fresh_remounts_SpellScript();
        }
};

// http://de.wowhead.com/quest=11472 - The Way to His Heart...
enum TheWaytoHisHeart
{
    GO_SCHOOL_OF_TASTY_REEF_FISH = 186949 
};

class spell_q11472_anuniaqs_net: public SpellScriptLoader
{
    public:
        spell_q11472_anuniaqs_net() : SpellScriptLoader("spell_q11472_anuniaqs_net") { }

        class spell_q11472_anuniaqs_net_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q11472_anuniaqs_net_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (GameObject* go = caster->FindNearestGameObject(GO_SCHOOL_OF_TASTY_REEF_FISH, 15.0f))
                        go->DespawnOrUnsummon();
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_q11472_anuniaqs_net_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q11472_anuniaqs_net_SpellScript();
        }
};

// http://de.wowhead.com/quest=8373 - The Power of Pine
enum MiscThePowerofPine
{
    GO_FORSAKEN_STINK_BOMB       = 180449,
    GO_FORSAKEN_STINK_BOMB_CLOUD = 180450
};

class spell_q8373_clean_up_stink_bomb: public SpellScriptLoader
{
    public:
        spell_q8373_clean_up_stink_bomb() : SpellScriptLoader("spell_q8373_clean_up_stink_bomb") { }

        class spell_q8373_clean_up_stink_bomb_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q8373_clean_up_stink_bomb_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (GameObject* go = caster->FindNearestGameObject(GO_FORSAKEN_STINK_BOMB, 6.0f))
                        go->DespawnOrUnsummon();

                    if (GameObject* go = caster->FindNearestGameObject(GO_FORSAKEN_STINK_BOMB_CLOUD, 6.0f))
                        go->DespawnOrUnsummon();
                }
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_q8373_clean_up_stink_bomb_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q8373_clean_up_stink_bomb_SpellScript();
        }
};

// wowhead.com/quest=11913 - Take No Chances
enum MiscTakeNoChances
{
    GO_FARSHIRE_GRAIN      = 188112,
    GO_TEMP_FARSHIRE_GRAIN = 300184,
    NPC_BUNNY              = 26161
};

class spell_q11913_burn_grain : public SpellScriptLoader
{
    public:
        spell_q11913_burn_grain() : SpellScriptLoader("spell_q11913_burn_grain") { }
    
        class spell_q11913_burn_grain_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q11913_burn_grain_SpellScript);
    
            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (GameObject* go = caster->FindNearestGameObject(GO_FARSHIRE_GRAIN, 10.0f))
                        go->DespawnOrUnsummon(3 * IN_MILLISECONDS);
    
                    if (GameObject* go = caster->FindNearestGameObject(GO_TEMP_FARSHIRE_GRAIN, 10.0f))
                        go->DespawnOrUnsummon(3 * IN_MILLISECONDS);
    
                    if (Creature* bunny = caster->FindNearestCreature(NPC_BUNNY, 10.0f))
                        bunny->DespawnOrUnsummon(3 * IN_MILLISECONDS);
                }                
            }
    
            void Register() override
            {
                AfterCast += SpellCastFn(spell_q11913_burn_grain_SpellScript::HandleAfterCast);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_q11913_burn_grain_SpellScript();
        }
};

// http://www.wowhead.com/quest=11520 - Discovering Your Roots
enum RaorthornRavagerMisc
{
    SPELL_EXPOSE_RAZORTHORN_ROOT = 44935,
    SPELL_SUMMON_RAZORTHORN_ROOT = 44941,
    GO_RAZORTHORN_DIRT_MOUND     = 187073,
    RAZORTHORN_ROOT              = 187072
};

class spell_q11520_expose_razorthorn_root : public SpellScriptLoader
{
    public:
        spell_q11520_expose_razorthorn_root() : SpellScriptLoader("spell_q11520_expose_razorthorn_root") { }
    
        class spell_q11520_expose_razorthorn_root_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q11520_expose_razorthorn_root_SpellScript);
    
            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                    if (GameObject* root = caster->FindNearestGameObject(GO_RAZORTHORN_DIRT_MOUND, 30.0f))
                    {
                       caster->GetMotionMaster()->MovePoint(1, root->GetPositionX(), root->GetPositionY(), root->GetPositionZ());
                       root->SummonGameObject(RAZORTHORN_ROOT, root->GetPositionX(), root->GetPositionY(), root->GetPositionZ(), root->GetOrientation(), 0, 0, 0, 0, 300);
                       root->SetPhaseMask(2, true); // see GO-SAI - Phase is set to 1 every 120 sec.
                    }
            }
    
            void Register() override
            {
                AfterCast += SpellCastFn(spell_q11520_expose_razorthorn_root_SpellScript::HandleAfterCast);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_q11520_expose_razorthorn_root_SpellScript();
        }
};

// wowhead.com/quest=12094 - Latent Power
class spell_q12094_draw_power : public SpellScriptLoader
{
    public:
        spell_q12094_draw_power() : SpellScriptLoader("spell_q12094_draw_power") { }
    
        class spell_q12094_draw_power_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_q12094_draw_power_AuraScript);
    
            void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                    if (Unit* caster = GetCaster())
                        if (Unit* target = GetUnitOwner())
                            if (Player* player = caster->ToPlayer())
                                player->KilledMonsterCredit(target->GetEntry());
            }
    
            void Register() override
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_q12094_draw_power_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_q12094_draw_power_AuraScript();
        }
};

// http://www.wowhead.com/quest=10712 - Test Flight: Ruuan Weald
enum TestFlightRuuanWeald
{
    QUEST_TEST_FLIGHT_RUAN_WALD = 10712
};

class spell_q10712_spin_nether_weather_vane : public SpellScriptLoader
{
    public:
        spell_q10712_spin_nether_weather_vane() : SpellScriptLoader("spell_q10712_spin_nether_weather_vane") { }
    
        class spell_q10712_spin_nether_weather_vane_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q10712_spin_nether_weather_vane_SpellScript);
    
            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                    caster->ToPlayer()->CompleteQuest(QUEST_TEST_FLIGHT_RUAN_WALD);
            }
    
            void Register() override
            {
                AfterCast += SpellCastFn(spell_q10712_spin_nether_weather_vane_SpellScript::HandleAfterCast);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_q10712_spin_nether_weather_vane_SpellScript();
        }
};

// http://www.wowhead.com/quest=10565 - The Stones of Vekh'nir
enum TheStonesofVekhnir
{
    QUEST_THE_STONE_OF_VEKHNIR = 10565
};

class spell_q10565_imbue_crystal : public SpellScriptLoader
{
    public:
        spell_q10565_imbue_crystal() : SpellScriptLoader("spell_q10565_imbue_crystal") { }

        class spell_q10565_imbue_crystal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q10565_imbue_crystal_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                    caster->ToPlayer()->CompleteQuest(QUEST_THE_STONE_OF_VEKHNIR);                  
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_q10565_imbue_crystal_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q10565_imbue_crystal_SpellScript();
        }
};

// http://www.wowhead.com/quest=13129 - Head Games
enum HeadGames
{
    ITEM_KURZELS_BLUE_SCRAP = 43214
};

class spell_q13129_stain_cloth : public SpellScriptLoader
{
    public:
        spell_q13129_stain_cloth() : SpellScriptLoader("spell_q13129_stain_cloth") { }

        class spell_q13129_stain_cloth_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q13129_stain_cloth_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                    caster->ToPlayer()->DestroyItemCount(ITEM_KURZELS_BLUE_SCRAP, 1, true);
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_q13129_stain_cloth_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q13129_stain_cloth_SpellScript();
        }
};

// http://de.wowhead.com/quest=12050 - Lumber Hack
enum GameObjectIDs
{
    GO_COLDWIND_TREE = 188539,
    GO_POSTER_KNIFE  = 190353,
};

class spell_q12050_gather_lumber : public SpellScriptLoader
{
    public:
        spell_q12050_gather_lumber() : SpellScriptLoader("spell_q12050_gather_lumber") { }

        class spell_q12050_gather_lumber_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12050_gather_lumber_SpellScript);

            void HandleAfterCast()
            {
                if (GameObject* go = GetCaster()->FindNearestGameObject(GO_COLDWIND_TREE, 10.0f))
                    go->SetPhaseMask(2, true);
                if (GameObject* go = GetCaster()->FindNearestGameObject(GO_POSTER_KNIFE, 10.0f))
                    go->SetPhaseMask(2, true);
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_q12050_gather_lumber_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q12050_gather_lumber_SpellScript();
        }
};

// http://www.wowhead.com/quest=12028 - Spiritual Insight
enum SpiritualInsightMisc
{
    NPC_TOALUU_THE_MYSTIC = 26595,
    DATA_TOALUU           = 4
};

class spell_q12028_spiritual_insight : public SpellScriptLoader
{
    public:
        spell_q12028_spiritual_insight() : SpellScriptLoader("spell_q12028_spiritual_insight") { }

        class spell_q12028_spiritual_insight_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_q12028_spiritual_insight_SpellScript);

            void HandleAfterCast()
            {
                if (Creature* toalu = GetCaster()->FindNearestCreature(NPC_TOALUU_THE_MYSTIC, 50.0f))
                    toalu->AI()->SetData(DATA_TOALUU, DATA_TOALUU);
            }

            void Register() override
            {
                AfterCast += SpellCastFn(spell_q12028_spiritual_insight_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_q12028_spiritual_insight_SpellScript();
        }
};

enum WeaknesstoLightning
{
    SPELL_WEAKNESS_TO_LIGHTNING_CREDIT = 46443,
};

class spell_weakness_to_lightning_quest_credit : public SpellScriptLoader
{
public:
    spell_weakness_to_lightning_quest_credit() : SpellScriptLoader("spell_weakness_to_lightning_quest_credit") {}

    class spell_weakness_to_lightning_quest_credit_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_weakness_to_lightning_quest_credit_SpellScript)

        void GetCorrectTarget()
        {
            if (GetHitUnit() && !GetHitUnit()->ToPlayer() && GetCaster())
                if (Unit* target = GetHitUnit()->GetCharmerOrOwner())
                {
                    PreventHitDefaultEffect(EFFECT_0);
                    GetCaster()->CastSpell(target, SPELL_WEAKNESS_TO_LIGHTNING_CREDIT);
                }
        }

        void Register() override
        {
            BeforeHit += SpellHitFn(spell_weakness_to_lightning_quest_credit_SpellScript::GetCorrectTarget);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_weakness_to_lightning_quest_credit_SpellScript();
    }
};

void AddSC_quest_spell_scripts_rg()
{
    new spell_q2203_thaumaturgy_channel();
    new spell_q12673_harvest_blight_crystal();
    new spell_q9927_nagrand_plant_warmaul_ogre_banner();
    new spell_q12730_quenching_mist();
    new spell_q9931_nagrand_plant_kil_sorrow_banner();
    new spell_q12847_summon_soul_moveto_bunny();
    new spell_q12214_fresh_remounts();
    new spell_q11472_anuniaqs_net();
    new spell_q8373_clean_up_stink_bomb();
    new spell_q11913_burn_grain();
    new spell_q11520_expose_razorthorn_root();
    new spell_q12094_draw_power();
    new spell_q10712_spin_nether_weather_vane();
    new spell_q10565_imbue_crystal();
    new spell_q13129_stain_cloth();
    new spell_q12050_gather_lumber();
    new spell_q12028_spiritual_insight();
    new spell_weakness_to_lightning_quest_credit();
}
