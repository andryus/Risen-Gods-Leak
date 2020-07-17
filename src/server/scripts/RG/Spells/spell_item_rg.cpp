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
 * Scripts for spells with SPELLFAMILY_GENERIC spells used by items.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_item_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "Player.h"

// 71169 - Shadow's Fate (Shadowmourne questline)
enum ShadowsFate
{
    SPELL_SOUL_FEAST        = 71203,
    QUEST_A_FEAST_OF_SOULS  = 24547,

    // Music
    SOUND_SHADOWMOURNE_01   = 17235,
    SOUND_SHADOWMOURNE_02   = 17236,
    SOUND_SHADOWMOURNE_03   = 17237,
    SOUND_SHADOWMOURNE_04   = 17238,
    SOUND_SHADOWMOURNE_05   = 17239,
    SOUND_SHADOWMOURNE_06   = 17240,
    SOUND_SHADOWMOURNE_07   = 17241,
    SOUND_SHADOWMOURNE_08   = 17242,
    SOUND_SHADOWMOURNE_09   = 17243,
    SOUND_SHADOWMOURNE_10   = 17244,
    SOUND_SHADOWMOURNE_11   = 17245,
    SOUND_SHADOWMOURNE_12   = 17246,
    SOUND_SHADOWMOURNE_13   = 17247,
    SOUND_SHADOWMOURNE_14   = 17248,
    SOUND_SHADOWMOURNE_15   = 17249,
    SOUND_SHADOWMOURNE_16   = 17250
};

class spell_item_shadows_fate : public SpellScriptLoader
{
    public:
        spell_item_shadows_fate() : SpellScriptLoader("spell_item_shadows_fate") { }

        class spell_item_shadows_fate_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_shadows_fate_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SOUL_FEAST))
                    return false;
                if (!sObjectMgr->GetQuestTemplate(QUEST_A_FEAST_OF_SOULS))
                    return false;
                return true;
            }

            bool Load() override
            {
                _procTarget = NULL;
                return true;
            }

            bool CheckProc(ProcEventInfo& /*eventInfo*/)
            {
                _procTarget = GetCaster();
                return _procTarget && _procTarget->GetTypeId() == TYPEID_PLAYER && _procTarget->ToPlayer()->GetQuestStatus(QUEST_A_FEAST_OF_SOULS) == QUEST_STATUS_INCOMPLETE;
            }

            void HandleProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();
                GetTarget()->CastSpell(_procTarget, SPELL_SOUL_FEAST, true);

                // Music
                Player* player = GetCaster()->ToPlayer();
                uint32 sound_id = RAND(SOUND_SHADOWMOURNE_01, SOUND_SHADOWMOURNE_02, SOUND_SHADOWMOURNE_03, SOUND_SHADOWMOURNE_04, SOUND_SHADOWMOURNE_05, SOUND_SHADOWMOURNE_06, SOUND_SHADOWMOURNE_07,
                    SOUND_SHADOWMOURNE_08, SOUND_SHADOWMOURNE_09, SOUND_SHADOWMOURNE_10, SOUND_SHADOWMOURNE_11, SOUND_SHADOWMOURNE_12, SOUND_SHADOWMOURNE_13, SOUND_SHADOWMOURNE_14,
                    SOUND_SHADOWMOURNE_15, SOUND_SHADOWMOURNE_16);

                // Shadowmourne soundeffect
                if (rand32() % 100 < 5)
                    player->PlayDirectSound(sound_id, player);
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_item_shadows_fate_AuraScript::CheckProc);
                OnEffectProc += AuraEffectProcFn(spell_item_shadows_fate_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            }

        private:
            Unit* _procTarget;
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_shadows_fate_AuraScript();
        }
};

enum Foamsword
{
    ITEM_FOAMSWORD_0    = 45061,
    ITEM_FOAMSWORD_1    = 45176,
    ITEM_FOAMSWORD_2    = 45177,
    ITEM_FOAMSWORD_3    = 45178,
    ITEM_FOAMSWORD_4    = 45179,
};

class UnitHasFoarSwordCheck
{
    public:
        bool operator() (WorldObject* object)
        {
            if (Player* player = object->ToPlayer())
            {
                if (Item* mainhand = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
                    switch (mainhand->GetEntry())
                    {
                        case ITEM_FOAMSWORD_0:
                        case ITEM_FOAMSWORD_1:
                        case ITEM_FOAMSWORD_2:
                        case ITEM_FOAMSWORD_3:
                        case ITEM_FOAMSWORD_4:
                            return false;
                        default:
                            break;
                    }
            }
            return true;
        }
};

class spell_item_foamsword : public SpellScriptLoader
{
    public:
        spell_item_foamsword() : SpellScriptLoader("spell_item_foamsword") {}

        class spell_item_foamsword_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_foamsword_SpellScript)

            void FilterTargets(std::list<WorldObject*>& targetList)
            {
                targetList.remove_if(UnitHasFoarSwordCheck());
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_item_foamsword_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_CONE_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_foamsword_SpellScript();
        }
};

class spell_item_carrot_on_a_stick : public SpellScriptLoader
{
    public:
        spell_item_carrot_on_a_stick() : SpellScriptLoader("spell_item_carrot_on_a_stick") { }

        class spell_item_carrot_on_a_stick_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_carrot_on_a_stick_SpellScript);

            bool Load() override
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            SpellCastResult CheckCast()
            {
                // Pointer check should not be neccessary since we checked in the Load() if it is a Player
                if (GetCaster()->ToPlayer()->getLevel() >= 70)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_item_carrot_on_a_stick_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_carrot_on_a_stick_SpellScript();
        }
};

/*######
# For quest 11876, Help Those That Cannot Help Themselves
######*/
enum HelpThemselves
{
    QUEST_CANNOT_HELP_THEMSELVES = 11876,
    NPC_TRAPPED_MAMMOTH_CALF = 25850,
    GO_MAMMOTH_TRAP_1 = 188022,
    GO_MAMMOTH_TRAP_2 = 188024,
    GO_MAMMOTH_TRAP_3 = 188025,
    GO_MAMMOTH_TRAP_4 = 188026,
    GO_MAMMOTH_TRAP_5 = 188027,
    GO_MAMMOTH_TRAP_6 = 188028,
    GO_MAMMOTH_TRAP_7 = 188029,
    GO_MAMMOTH_TRAP_8 = 188030,
    GO_MAMMOTH_TRAP_9 = 188031,
    GO_MAMMOTH_TRAP_10 = 188032,
    GO_MAMMOTH_TRAP_11 = 188033,
    GO_MAMMOTH_TRAP_12 = 188034,
    GO_MAMMOTH_TRAP_13 = 188035,
    GO_MAMMOTH_TRAP_14 = 188036,
    GO_MAMMOTH_TRAP_15 = 188037,
    GO_MAMMOTH_TRAP_16 = 188038,
    GO_MAMMOTH_TRAP_17 = 188039,
    GO_MAMMOTH_TRAP_18 = 188040,
    GO_MAMMOTH_TRAP_19 = 188041,
    GO_MAMMOTH_TRAP_20 = 188042,
    GO_MAMMOTH_TRAP_21 = 188043,
    GO_MAMMOTH_TRAP_22 = 188044,
};

#define MammothTrapsNum 22
const uint32 MammothTraps[MammothTrapsNum] =
{
    GO_MAMMOTH_TRAP_1, GO_MAMMOTH_TRAP_2, GO_MAMMOTH_TRAP_3, GO_MAMMOTH_TRAP_4, GO_MAMMOTH_TRAP_5,
    GO_MAMMOTH_TRAP_6, GO_MAMMOTH_TRAP_7, GO_MAMMOTH_TRAP_8, GO_MAMMOTH_TRAP_9, GO_MAMMOTH_TRAP_10,
    GO_MAMMOTH_TRAP_11, GO_MAMMOTH_TRAP_12, GO_MAMMOTH_TRAP_13, GO_MAMMOTH_TRAP_14, GO_MAMMOTH_TRAP_15,
    GO_MAMMOTH_TRAP_16, GO_MAMMOTH_TRAP_17, GO_MAMMOTH_TRAP_18, GO_MAMMOTH_TRAP_19, GO_MAMMOTH_TRAP_20,
    GO_MAMMOTH_TRAP_21, GO_MAMMOTH_TRAP_22
};

class spell_item_dehta_trap_smasher : public SpellScriptLoader
{
    public:
        spell_item_dehta_trap_smasher() : SpellScriptLoader("spell_item_dehta_trap_smasher") { }

        class spell_item_dehta_trap_smasher_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_dehta_trap_smasher_SpellScript);

            SpellCastResult CheckRequirement()
            {
                Creature* pMammoth = GetCaster()->FindNearestCreature(NPC_TRAPPED_MAMMOTH_CALF, 5.0f);
                if (!pMammoth)
                    return SPELL_FAILED_NOT_HERE;

                for (uint8 i = 0; i < MammothTrapsNum; ++i)
                {
                    GameObject* pTrap = pMammoth->FindNearestGameObject(MammothTraps[i], 1.0f);
                    if (pTrap)
                        return SPELL_CAST_OK;
                }

                return SPELL_FAILED_NOT_HERE;
            }

            void HandleAfterCast()
            {
                Creature* pMammoth = GetCaster()->FindNearestCreature(NPC_TRAPPED_MAMMOTH_CALF, 5.0f);
                if (!pMammoth)
                    return;

                for (uint8 i = 0; i < MammothTrapsNum; ++i)
                {
                    GameObject* pTrap = pMammoth->FindNearestGameObject(MammothTraps[i], 1.0f);
                    if (pTrap)
                    {
                        pMammoth->AI()->DoAction(1);
                        pTrap->SetGoState(GO_STATE_READY);
                        if (Player* player = GetCaster()->ToPlayer())
                            player->KilledMonsterCredit(NPC_TRAPPED_MAMMOTH_CALF);
                        return;
                    }
                }
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_item_dehta_trap_smasher_SpellScript::CheckRequirement);
                AfterCast += SpellCastFn(spell_item_dehta_trap_smasher_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_dehta_trap_smasher_SpellScript();
        }
};

// 67698 - Item - Coliseum 25 Normal Healer Trinket || 67752 - Item - Coliseum 25 Heroic Healer Trinket
class spell_item_coliseum_healer_trinked : public SpellScriptLoader
{
    public:
        spell_item_coliseum_healer_trinked() : SpellScriptLoader("spell_item_coliseum_healer_trinked") { }

        class spell_item_coliseum_healer_trinked_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_coliseum_healer_trinked_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->Id == 50249) // Path of Illidan
                        return false;
                return true;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_item_coliseum_healer_trinked_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_coliseum_healer_trinked_AuraScript();
        }
};

class OnlyLowLevelUnits
{
    public:
        bool operator() (WorldObject* object)
        {
            if (Unit* unit = object->ToUnit())
                if (unit->getLevel() <= 61)
                    return false;
            return true;
        }
};

class spell_item_flash_bomb : public SpellScriptLoader
{
    public:
        spell_item_flash_bomb() : SpellScriptLoader("spell_item_flash_bomb") { }

        class spell_item_flash_bomb_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_flash_bomb_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(OnlyLowLevelUnits());
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_item_flash_bomb_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_flash_bomb_SpellScript();
        }
};

enum RageofRivendare
{
    SPELL_RAGE_OF_RIVENSARE_R1 = 50117,
};

class spell_chaos_bane : public SpellScriptLoader
{
    public:
        spell_chaos_bane() : SpellScriptLoader("spell_chaos_bane") { }

        class spell_chaos_bane_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_chaos_bane_SpellScript);

            void RecalculateDamage()
            {
                int32 dmg = GetHitDamage();
                if (AuraEffect const* aurEff = GetCaster()->GetAuraEffectOfRankedSpell(SPELL_RAGE_OF_RIVENSARE_R1, EFFECT_1))
                    AddPct(dmg, aurEff->GetSpellInfo()->GetBasePoints(EFFECT_1) * 2);
                SetHitDamage(dmg);
            }

            void Register() override
            {
                OnHit += SpellHitFn(spell_chaos_bane_SpellScript::RecalculateDamage);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_chaos_bane_SpellScript();
        }
};

enum HeroicCasterWeaponProcMisc
{
    SPELL_ICON_STARFALL_AOE_R1 = 50294,
    SPELL_ICON_STARFALL_AOE_R2 = 53188,
    SPELL_ICON_STARFALL_AOE_R3 = 53189,
    SPELL_ICON_STARFALL_AOE_R4 = 53190
};

// - 71846 Item - Icecrown 25 Heroic Caster Weapon Proc
class spell_item_icecrown_caster_weapon_proc : public SpellScriptLoader
{
    public:
        spell_item_icecrown_caster_weapon_proc() : SpellScriptLoader("spell_item_icecrown_caster_weapon_proc") { }

        class spell_item_icecrown_caster_weapon_proc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_icecrown_caster_weapon_proc_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                {
                    if (spellInfo->Id == SPELL_ICON_STARFALL_AOE_R1) 
                        return false;
                    if (spellInfo->Id == SPELL_ICON_STARFALL_AOE_R2) 
                        return false;
                    if (spellInfo->Id == SPELL_ICON_STARFALL_AOE_R3) 
                        return false;
                    if (spellInfo->Id == SPELL_ICON_STARFALL_AOE_R4) 
                        return false;
                }                                                                           
                return true;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_item_icecrown_caster_weapon_proc_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_icecrown_caster_weapon_proc_AuraScript();
        }
};

// - 23605 - Spell Vulnerability
class spell_item_vulnerability : public SpellScriptLoader
{
    public:
        spell_item_vulnerability() : SpellScriptLoader("spell_item_vulnerability") { }

        class spell_item_vulnerability_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_vulnerability_SpellScript);

            bool Load() override
            {
                _removed = false;
                return true;
            }

            void HandleHit()
            {
                if (Unit* target = GetHitUnit())
                {
                    if (target->getLevel() > 60)
                    {
                        PreventHitDefaultEffect(EFFECT_0);
                        _removed = true;
                    }
                }
            }

            void RemoveAura()
            {
                if (_removed)
                    PreventHitAura();
            }

            void Register() override
            {
                BeforeHit += SpellHitFn(spell_item_vulnerability_SpellScript::HandleHit);
                AfterHit += SpellHitFn(spell_item_vulnerability_SpellScript::RemoveAura);
            }

            bool _removed;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_vulnerability_SpellScript();
        }
};

enum EssenceofLifeMisc
{
    SPELL_ICON_FEL_ARMOUR = 2297
};

// - 33953 - Essence of Life
class spell_item_essence_of_life : public SpellScriptLoader
{
    public:
        spell_item_essence_of_life() : SpellScriptLoader("spell_item_essence_of_life") { }

        class spell_item_essence_of_life_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_essence_of_life_AuraScript);

            bool Check(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->SpellIconID == SPELL_ICON_FEL_ARMOUR) 
                        return false;
                return true;
            }

            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_item_essence_of_life_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_essence_of_life_AuraScript();
        }
};

enum SurgeNeedleTeleporterMisc
{
    AREA_MOONREST_GARDENS = 4157,
    AREA_SURGE_NEEDLE     = 4156,
    SPELL_SURGE_NEEDLE_1  = 47324,
    SPELL_SURGE_NEEDLE_2  = 47325
};

class spell_item_surge_needle_teleporter : public SpellScriptLoader
{
    public:
        spell_item_surge_needle_teleporter() : SpellScriptLoader("spell_item_surge_needle_teleporter") { }

        class spell_item_surge_needle_teleporter_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_surge_needle_teleporter_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Player* player = caster->ToPlayer())
                {
                    if (player->GetAreaId() == AREA_MOONREST_GARDENS)
                        player->CastSpell(player, SPELL_SURGE_NEEDLE_2);
                    else if (player->GetAreaId() == AREA_SURGE_NEEDLE)
                        player->CastSpell(player, SPELL_SURGE_NEEDLE_1);
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_item_surge_needle_teleporter_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_item_surge_needle_teleporter_SpellScript();
    }
};

enum MindControlCap
{
    SPELL_GNOMISH_MIND_CONTROL_CAP = 13181,
    SPELL_DULLARD                  = 67809,
};

class spell_item_mind_control_cap : public SpellScriptLoader
{
    public:
        spell_item_mind_control_cap() : SpellScriptLoader("spell_item_mind_control_cap") { }

        class spell_item_mind_control_cap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_mind_control_cap_SpellScript);

            bool Load() override
            {
                if (!GetCastItem())
                    return false;
                return true;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_GNOMISH_MIND_CONTROL_CAP) || !sSpellMgr->GetSpellInfo(SPELL_DULLARD))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                {
                    if (roll_chance_i(95))
                        caster->CastSpell(target, roll_chance_i(30) ? SPELL_DULLARD : SPELL_GNOMISH_MIND_CONTROL_CAP, true, GetCastItem());
                    else
                        target->CastSpell(caster, SPELL_GNOMISH_MIND_CONTROL_CAP, true); // backfire - 5% chance
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_item_mind_control_cap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_mind_control_cap_SpellScript();
        }
};

enum UniversalRemote
{
    SPELL_CONTROL_MACHINE      = 8345,
    SPELL_MOBILITY_MALFUNCTION = 8346,
    SPELL_TARGET_LOCK       = 8347,
};

class spell_item_universal_remote : public SpellScriptLoader
{
    public:
        spell_item_universal_remote() : SpellScriptLoader("spell_item_universal_remote") { }

        class spell_item_universal_remote_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_universal_remote_SpellScript);

            bool Load() override
            {
                if (!GetCastItem())
                    return false;
                return true;
            }

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_CONTROL_MACHINE) || !sSpellMgr->GetSpellInfo(SPELL_MOBILITY_MALFUNCTION) || !sSpellMgr->GetSpellInfo(SPELL_TARGET_LOCK))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                {
                    if (roll_chance_i(85))
                        caster->CastSpell(target, roll_chance_i(30) ? SPELL_MOBILITY_MALFUNCTION : SPELL_CONTROL_MACHINE, true, GetCastItem()); // root - 25% chance
                    else
                        caster->CastSpell(target, SPELL_TARGET_LOCK, true); // threat - 15% chance
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_item_universal_remote_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_universal_remote_SpellScript();
        }
};

class spell_item_fetch_ball : public SpellScriptLoader
{
    public:
        spell_item_fetch_ball() : SpellScriptLoader("spell_item_fetch_ball") {}

        class spell_item_fetch_ball_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_fetch_ball_SpellScript);

            void SelectTarget(std::list<WorldObject*>& targets)
            {
                Creature* target = NULL;
                for (std::list<WorldObject*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    if (Creature* creature = (*itr)->ToCreature())
                    {
                        if (creature->GetOwnerGUID() == GetCaster()->GetOwnerGUID() && !creature->IsNonMeleeSpellCast(false) &&
                            creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != POINT_MOTION_TYPE)
                        {
                            target = creature;
                            break;
                        }
                    }

                targets.clear();
                if (target)
                    targets.push_back(target);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_item_fetch_ball_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_fetch_ball_SpellScript();
        }
};

class spell_item_toxic_wasteling : public SpellScriptLoader
{
    public:
        spell_item_toxic_wasteling() : SpellScriptLoader("spell_item_toxic_wasteling") {}

        class spell_item_toxic_wasteling_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_toxic_wasteling_SpellScript);

            void HandleJump(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Creature* target = GetHitCreature())
                {
                    GetCaster()->GetMotionMaster()->Clear(false);
                    GetCaster()->GetMotionMaster()->MoveIdle();
                    GetCaster()->ToCreature()->SetHomePosition(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f);
                    GetCaster()->GetMotionMaster()->MoveJump(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 12.0f, 3.0f, 1);
                    target->DespawnOrUnsummon(1500);
                }
            }

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
            }

            void Register() override
            {
                OnEffectLaunchTarget += SpellEffectFn(spell_item_toxic_wasteling_SpellScript::HandleJump, EFFECT_0, SPELL_EFFECT_JUMP);
                OnEffectHitTarget += SpellEffectFn(spell_item_toxic_wasteling_SpellScript::HandleScriptEffect, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_item_toxic_wasteling_SpellScript();
        }
};

class spell_item_sleepy_willy : public SpellScriptLoader
{
    public:
        spell_item_sleepy_willy() : SpellScriptLoader("spell_item_sleepy_willy") {}

        class spell_item_sleepy_willy_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_item_sleepy_willy_SpellScript);

            void SelectTarget(std::list<WorldObject*>& targets)
            {
                Creature* target = NULL;
                for (std::list<WorldObject*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    if (Creature* creature = (*itr)->ToCreature())
                        if (creature->GetCreatureType() == CREATURE_TYPE_CRITTER)
                        {
                            target = creature;
                            break;
                        }

                targets.clear();
                if (target)
                    targets.push_back(target);

            }

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                Creature* target = GetHitCreature();
                if (!target)
                    return;

                GetCaster()->CastSpell(target, GetEffectValue(), false);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_item_sleepy_willy_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnEffectHitTarget += SpellEffectFn(spell_item_sleepy_willy_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };
        
        SpellScript* GetSpellScript() const override
        {
            return new spell_item_sleepy_willy_SpellScript();
        }
};

enum eChicken
{
    SPELL_ROCKET_CHICKEN_EMOTE = 45255,
};

class spell_item_rocket_chicken : public SpellScriptLoader
{
    public:
        spell_item_rocket_chicken() : SpellScriptLoader("spell_item_rocket_chicken") { }

        class spell_item_rocket_chicken_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_rocket_chicken_AuraScript);

            void HandleDummyTick(AuraEffect const* /*aurEff*/)
            {
                if (roll_chance_i(5))
                {
                    GetTarget()->ToCreature()->DespawnOrUnsummon(8 * IN_MILLISECONDS);
                    GetTarget()->Kill(GetTarget(), GetTarget());
                }
                else if (roll_chance_i(50))
                    GetTarget()->CastSpell(GetTarget(), SPELL_ROCKET_CHICKEN_EMOTE, false);
            }

            void Register() override
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_item_rocket_chicken_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_rocket_chicken_AuraScript();
        }
};

enum LimitlessPowerMisc
{
    ITEM_SHIFTING_NAARU_SLIVER = 34429
};

// 45044 - Limitless Power 
class spell_item_limitless_power : public SpellScriptLoader
{
    public:
        spell_item_limitless_power() : SpellScriptLoader("spell_item_limitless_power") { }

        class spell_item_limitless_power_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_limitless_power_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (GetUnitOwner()->ToPlayer() && !GetUnitOwner()->ToPlayer()->HasItemOrGemWithIdEquipped(ITEM_SHIFTING_NAARU_SLIVER, 1))
                    amount = 0;
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_item_limitless_power_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DAMAGE_DONE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_item_limitless_power_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_HEALING_DONE);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_item_limitless_power_AuraScript();
        }
};

enum EthernalSoulTraderMisc
{
    NPC_TRADER_PET = 27914,
    SPELL_PET_AURA = 50052,
    ITEM_KC        = 38186
};

class spell_ethereal_soul_trader_onkill : public SpellScriptLoader
{
    public:
        spell_ethereal_soul_trader_onkill() : SpellScriptLoader("spell_ethereal_soul_trader_onkill") { }
    
        class spell_ethereal_soul_trader_onkill_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ethereal_soul_trader_onkill_SpellScript);
    
            void HandleDummy(SpellEffIndex effIndex)
            {
                if (Player* player = GetCaster()->ToPlayer())
                    player->AddItem(ITEM_KC, 1);
                
                if (Unit* target = GetCaster()->FindNearestCreature(NPC_TRADER_PET, 30.0f))
                    if (target->GetTypeId() == TYPEID_UNIT)
                        target->CastSpell(target, SPELL_PET_AURA, true);
            }
    
            void Register() override
            {
                OnEffectHit += SpellEffectFn(spell_ethereal_soul_trader_onkill_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const override
        {
           return new spell_ethereal_soul_trader_onkill_SpellScript();
        }
};

class spell_ethereal_soul_trader_onkill_effect : public SpellScriptLoader
{
    public:
        spell_ethereal_soul_trader_onkill_effect() : SpellScriptLoader("spell_ethereal_soul_trader_onkill_effect") { }

        class spell_ethereal_soul_trader_onkill_effect_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ethereal_soul_trader_onkill_effect_SpellScript);

            SpellCastResult CheckCast()
            {
                if (Unit* target = GetExplTargetUnit())
                    if (target->ToCreature()->GetCreatureTemplate() && target->ToCreature()->GetCreatureTemplate()->unit_flags & UNIT_FLAG_IMMUNE_TO_PC)
                        return SPELL_FAILED_BAD_TARGETS;
                
                return SPELL_CAST_OK;
            }

            void Register() override
            {
                OnCheckCast += SpellCheckCastFn(spell_ethereal_soul_trader_onkill_effect_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_ethereal_soul_trader_onkill_effect_SpellScript();
        }
};

// 60490 - Embrace of the Spider
class spell_item_embrace_of_the_spider : public SpellScriptLoader
{
    public:
        spell_item_embrace_of_the_spider() : SpellScriptLoader("spell_item_embrace_of_the_spider") { }
    
        class spell_item_embrace_of_the_spider_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_item_embrace_of_the_spider_AuraScript);
    
            bool CheckProc(ProcEventInfo& eventInfo)
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->Effects[EFFECT_0].Effect == SPELL_EFFECT_SUMMON)
                        return false;
    
                return true;
            }
    
            void Register() override
            {
                DoCheckProc += AuraCheckProcFn(spell_item_embrace_of_the_spider_AuraScript::CheckProc);
            }
        };
    
        AuraScript* GetAuraScript() const override
        {
            return new spell_item_embrace_of_the_spider_AuraScript();
        }
};

// Meteorite Crystal(46051), Meteoric Inspiration(64999)
class spell_item_meteorite_crystal : public SpellScriptLoader
{
public:
    spell_item_meteorite_crystal() : SpellScriptLoader("spell_item_meteorite_crystal") { }

    class spell_item_meteorite_crystal_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_item_meteorite_crystal_AuraScript);

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            // no further checks for channeled spells
            if (eventInfo.GetSpellInfo()->AttributesEx & SPELL_ATTR1_CHANNELED_1)
                return true;

            // only spell which cost mana can proc
            if (eventInfo.GetSpellInfo()->ManaCostPercentage == 0 &&
                // paladin - beacon of light
                eventInfo.GetSpellInfo()->Id != 53652 && eventInfo.GetSpellInfo()->Id != 53653 && eventInfo.GetSpellInfo()->Id != 53654 &&
                // paladin - judgement
                eventInfo.GetSpellInfo()->Id != 20185 && eventInfo.GetSpellInfo()->Id != 20184 && eventInfo.GetSpellInfo()->Id != 20186 &&
                // paladin - holy shock
                eventInfo.GetSpellInfo()->SpellIconID != 156)
                return false;

            uint32 spellId = eventInfo.GetSpellInfo()->Id;
            auto itr = procHistroryMap.find(spellId);

            // first proc by this spell: always allow proc
            if (itr == procHistroryMap.end())
                return HandleSpellHistory(spellId);
            
            // spell already proced this GCD: disallow proc
            if (time(0) - itr->second < SECOND)
                return false;
            else
            // spell proced last GCD: allow proc again
                return HandleSpellHistory(spellId);
        }

        bool HandleSpellHistory(uint32 spellId)
        {
            procHistroryMap[spellId] = time(0);
            return true;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_item_meteorite_crystal_AuraScript::CheckProc);
        }

    private:
        std::unordered_map<uint32 /*spellId*/, time_t /*castTime*/> procHistroryMap;
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_meteorite_crystal_AuraScript();
    }
};

enum SurgingPower
{
    SPELL_SURGING_POWER_NHC     = 71600,
    SPELL_SURGING_POWER_HC      = 71643,

    SPELL_SURGE_OF_POWER_NHC    = 71601,
    SPELL_SURGE_OF_POWER_HC     = 71644,
};

class spell_item_surging_power : public SpellScriptLoader
{
public:
    spell_item_surging_power() : SpellScriptLoader("spell_item_surging_power") { }

    class spell_item_surging_power_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_item_surging_power_AuraScript);

        void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* caster = GetUnitOwner())
            {
                if (!caster->HasAura(SPELL_SURGE_OF_POWER_NHC))
                    caster->RemoveAurasDueToSpell(SPELL_SURGING_POWER_NHC);
                else if (!caster->HasAura(SPELL_SURGE_OF_POWER_HC))
                    caster->RemoveAurasDueToSpell(SPELL_SURGING_POWER_HC);
            }
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_item_surging_power_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_CHARM, AURA_EFFECT_HANDLE_REAL);
        }

    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_item_surging_power_AuraScript();
    }
};

void AddSC_item_spell_scripts_rg()
{
    new spell_item_shadows_fate();
    new spell_item_foamsword();
    new spell_item_carrot_on_a_stick();
    new spell_item_dehta_trap_smasher();
    new spell_item_coliseum_healer_trinked();
    new spell_item_flash_bomb();
    new spell_chaos_bane();
    new spell_item_icecrown_caster_weapon_proc();
    new spell_item_vulnerability();
    new spell_item_essence_of_life();
    new spell_item_surge_needle_teleporter();
    new spell_item_mind_control_cap();
    new spell_item_universal_remote();
    new spell_item_fetch_ball();
    new spell_item_toxic_wasteling();
    new spell_item_sleepy_willy();
    new spell_item_rocket_chicken();
    new spell_item_limitless_power();
    new spell_ethereal_soul_trader_onkill();
    new spell_ethereal_soul_trader_onkill_effect();
    new spell_item_embrace_of_the_spider();
    new spell_item_meteorite_crystal();
    new spell_item_surging_power();
}
