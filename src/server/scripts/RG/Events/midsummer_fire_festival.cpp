/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "Vehicle.h"
#include "Group.h"
#include "LFGMgr.h"
#include "GridNotifiers.h"
#include "GameEventMgr.h"
#include "DBCStores.h"

/*###################################
# spell_gen_ribbon_pole_dancer_check
####################################*/

enum RibbonPoleData
{
    SPELL_HAS_FULL_MIDSUMMER_SET        = 58933,
    SPELL_BURNING_HOT_POLE_DANCE        = 58934,
    SPELL_RIBBON_DANCE                  = 29175,
    GO_RIBBON_POLE                      = 181605
};

class spell_gen_ribbon_pole_dancer_check : public SpellScriptLoader
{
    public:
        spell_gen_ribbon_pole_dancer_check() : SpellScriptLoader("spell_gen_ribbon_pole_dancer_check") { }

        class spell_gen_ribbon_pole_dancer_check_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_gen_ribbon_pole_dancer_check_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_HAS_FULL_MIDSUMMER_SET))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_BURNING_HOT_POLE_DANCE))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_RIBBON_DANCE))
                    return false;
                return true;
            }

            void PeriodicTick(AuraEffect const* /*aurEff*/)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                // check if aura needs to be removed
                if (!target->FindNearestGameObject(GO_RIBBON_POLE, 20.0f))
                {
                    target->InterruptNonMeleeSpells(false);
                    target->RemoveAurasDueToSpell(GetId());
                    return;
                }

                // set xp buff duration
                if (Aura* aur = target->GetAura(SPELL_RIBBON_DANCE))
                {
                    aur->SetMaxDuration(aur->GetMaxDuration() >= 3600000 ? 3600000 : aur->GetMaxDuration() + 180000);
                    aur->RefreshDuration();

                    // reward achievement criteria
                    if (aur->GetMaxDuration() >= 2160000 && target->HasAura(SPELL_HAS_FULL_MIDSUMMER_SET))
                        target->CastSpell(target, SPELL_BURNING_HOT_POLE_DANCE, true);
                }
                else
                    target->AddAura(SPELL_RIBBON_DANCE, target);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_gen_ribbon_pole_dancer_check_AuraScript::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_gen_ribbon_pole_dancer_check_AuraScript();
        }
};

/*######
## npc_torch_tossing_bunny
######*/

enum
{
    SPELL_TORCH_TOSSING_COMPLETE_A = 45719,
    SPELL_TORCH_TOSSING_COMPLETE_H = 46651,
    SPELL_TORCH_TOSSING_TRAINING   = 45716,
    SPELL_TORCH_TOSSING_PRACTICE   = 46630,
    SPELL_TORCH_TOSS               = 46054,
    SPELL_TARGET_INDICATOR         = 45723,
    SPELL_BRAZIERS_HIT             = 45724,
    NPC_TOSSING_DUMMY              = 25535,
    ACTION_ACTIVATE                = 1,
    ACTION_DEACTIVATE              = 2,
};

class npc_torch_tossing_bunny : public CreatureScript
{
    public:
        npc_torch_tossing_bunny() : CreatureScript("npc_torch_tossing_bunny") { }

        struct npc_torch_tossing_bunnyAI : public ScriptedAI
        {
            npc_torch_tossing_bunnyAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                TargetTimer = urand(1000, 10000);
                active = true;
            }

            void SpellHit(Unit* caster, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_TORCH_TOSS && me->HasAura(SPELL_TARGET_INDICATOR))
                {
                    uint8 neededHits;

                    if (caster->HasAura(SPELL_TORCH_TOSSING_TRAINING))
                        neededHits = 8;
                    else if (caster->HasAura(SPELL_TORCH_TOSSING_PRACTICE))
                        neededHits = 20;
                    else
                        return;

                    DoCastSelf(SPELL_BRAZIERS_HIT, true);
                    caster->AddAura(SPELL_BRAZIERS_HIT, caster);

                    if (caster->GetAuraCount(SPELL_BRAZIERS_HIT) >= neededHits)
                    {
                        // complete quest
                        caster->CastSpell(caster, SPELL_TORCH_TOSSING_COMPLETE_A, true);
                        caster->CastSpell(caster, SPELL_TORCH_TOSSING_COMPLETE_H, true);
                        caster->RemoveAurasDueToSpell(SPELL_BRAZIERS_HIT);
                        caster->RemoveAurasDueToSpell(neededHits == 8 ? SPELL_TORCH_TOSSING_TRAINING : SPELL_TORCH_TOSSING_PRACTICE);
                    }
                }
                else if (spell->Id == SPELL_TARGET_INDICATOR)
                {
                    DoAction(ACTION_DEACTIVATE);

                    std::list<Creature*> others;
                    me->GetCreatureListWithEntryInGrid(others, NPC_TOSSING_DUMMY, 20.0f);

                    others.remove(me);

                    std::list<Creature*>::iterator choosen = others.begin();
                    std::advance(choosen, urand(0, others.size() - 1));

                    for (std::list<Creature*>::const_iterator itr = others.begin(); itr  != others.end(); ++itr)
                    {
                        (*itr)->GetAI()->DoAction(ACTION_DEACTIVATE);
                        (*itr)->RemoveAurasDueToSpell(SPELL_TARGET_INDICATOR);
                    }

                    (*choosen)->GetAI()->DoAction(ACTION_ACTIVATE);
                }
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_ACTIVATE:
                        TargetTimer = 3000;
                        active = true;
                        break;
                    case ACTION_DEACTIVATE:
                        active = false;
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (active)
                {
                    if (TargetTimer <= diff)
                        DoCast(SPELL_TARGET_INDICATOR);
                    else
                        TargetTimer -= diff;
                }
            }

        private:
            uint32 TargetTimer;
            bool active;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_torch_tossing_bunnyAI(creature);
        }
};

enum TorchCatchingData
{
    SPELL_FLING_TORCH_MISSILE     = 45669,
    SPELL_TOSS_TORCH_SHADOW       = 46105,
    SPELL_TORCH_TARGET_PICKER     = 45907,
    SPELL_TORCHES_COUGHT          = 45693,
    SPELL_JUGGLE_TORCH_MISSED     = 45676,
    SPELL_TORCH_CATCHING_SUCCESS  = 46081,
    SPELL_TORCH_DAILY_SUCCESS     = 46654,
    NPC_JUGGLE_TARGET             = 25515,
    QUEST_TORCH_CATCHING_A        = 11657,
    QUEST_TORCH_CATCHING_H        = 11923,
    QUEST_MORE_TORCH_CATCHING_A   = 11924,
    QUEST_MORE_TORCH_CATCHING_H   = 11925
};

class spell_gen_torch_target_picker : public SpellScriptLoader
{
    public:
        spell_gen_torch_target_picker() : SpellScriptLoader("spell_gen_torch_target_picker") {}

        class spell_gen_torch_target_picker_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_torch_target_picker_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_FLING_TORCH_MISSILE))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_TOSS_TORCH_SHADOW))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();

                if (!caster || !target)
                    return;

                caster->CastSpell(target, SPELL_FLING_TORCH_MISSILE, true);
                caster->CastSpell(target, SPELL_TOSS_TORCH_SHADOW, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_gen_torch_target_picker_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_torch_target_picker_SpellScript();
        }
};

class spell_gen_juggle_torch_catch : public SpellScriptLoader
{
    public:
        spell_gen_juggle_torch_catch() : SpellScriptLoader("spell_gen_juggle_torch_catch") {}

        class spell_gen_juggle_torch_catch_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_juggle_torch_catch_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellStore.LookupEntry(SPELL_TORCH_TARGET_PICKER))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_TORCHES_COUGHT))
                    return false;
                if (!sSpellStore.LookupEntry(SPELL_JUGGLE_TORCH_MISSED))
                    return false;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                Unit* caster = GetCaster();
                Unit* juggleTarget = NULL;
                bool missed = true;

                if (unitList.empty() || !caster || !caster->ToPlayer())
                     return;

                for (std::list<WorldObject*>::iterator iter = unitList.begin(); iter != unitList.end(); ++iter)
                {
                    if (*iter == caster)
                        missed = false;

                    if ((*iter)->GetTypeId() == TYPEID_UNIT)
                        juggleTarget = (*iter)->ToUnit();
                }

                if (missed)
                {
                    if (juggleTarget)
                        juggleTarget->CastSpell(juggleTarget, SPELL_JUGGLE_TORCH_MISSED, true);
                    caster->RemoveAurasDueToSpell(SPELL_TORCHES_COUGHT);
                }
                else
                {
                    uint8 neededCatches;

                    if (caster->ToPlayer()->GetQuestStatus(QUEST_TORCH_CATCHING_A) == QUEST_STATUS_INCOMPLETE
                        || caster->ToPlayer()->GetQuestStatus(QUEST_TORCH_CATCHING_H) == QUEST_STATUS_INCOMPLETE)
                    {
                        neededCatches = 4;
                    }
                    else if (caster->ToPlayer()->GetQuestStatus(QUEST_MORE_TORCH_CATCHING_A) == QUEST_STATUS_INCOMPLETE
                        || caster->ToPlayer()->GetQuestStatus(QUEST_MORE_TORCH_CATCHING_H) == QUEST_STATUS_INCOMPLETE)
                    {
                        neededCatches = 10;
                    }
                    else
                    {
                        caster->RemoveAurasDueToSpell(SPELL_TORCHES_COUGHT);
                        return;
                    }

                    caster->CastSpell(caster, SPELL_TORCH_TARGET_PICKER, true);
                    caster->CastSpell(caster, SPELL_TORCHES_COUGHT, true);

                    // reward quest
                    if (caster->GetAuraCount(SPELL_TORCHES_COUGHT) >= neededCatches)
                    {
                        caster->CastSpell(caster, SPELL_TORCH_CATCHING_SUCCESS, true);
                        caster->CastSpell(caster, SPELL_TORCH_DAILY_SUCCESS, true);
                        caster->RemoveAurasDueToSpell(SPELL_TORCHES_COUGHT);
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gen_juggle_torch_catch_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_juggle_torch_catch_SpellScript();
        }
};

enum JuggleTorch
{
    SPELL_JUGGLE_TORCH_ITEM_SPELL = 45819,
    SPELL_JUGGLE_TORCH_NEW_REWAD  = 45280,
    SPELL_JUGGLE_TORCH_SPEED_0    = 45638,
    SPELL_JUGGLE_TORCH_SPEED_1    = 45792,
    SPELL_JUGGLE_TORCH_SPEED_2    = 45806,
    SPELL_JUGGLE_TORCH_SPEED_3    = 45816,
    SPELL_JUGGLE_TORCH_CATCH      = 45644,
};

//! Actuall juggle torch -.-
class spell_gen_juggle_torch_juggle_catch : public SpellScriptLoader
{
    public:
        spell_gen_juggle_torch_juggle_catch() : SpellScriptLoader("spell_gen_juggle_torch_juggle_catch") {}

        class spell_gen_juggle_torch_juggle_catch_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_juggle_torch_juggle_catch_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(Trinity::ObjectTypeIdCheck(TYPEID_PLAYER, false));
                
                if (unitList.empty())
                    return;

                std::list<WorldObject*>::iterator itr = unitList.begin();
                std::advance(itr, urand(0, unitList.size() - 1));
                Unit* target = (*itr)->ToUnit();
                unitList.clear();

                unitList.push_back(target);
            }

            void HandleDummy()
            {
                if (Player* target = GetHitPlayer())
                    GetCaster()->CastSpell(target, SPELL_JUGGLE_TORCH_NEW_REWAD, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_gen_juggle_torch_juggle_catch_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
                OnHit += SpellHitFn(spell_gen_juggle_torch_juggle_catch_SpellScript::HandleDummy);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_juggle_torch_juggle_catch_SpellScript();
        }
};

class spell_gen_juggle_torch_throw : public SpellScriptLoader
{
    public:
        spell_gen_juggle_torch_throw() : SpellScriptLoader("spell_gen_juggle_torch_throw") {}

        class spell_gen_juggle_torch_throw_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gen_juggle_torch_throw_SpellScript);

            void HandleCast()
            {
                Unit* caster = GetCaster();
                const WorldLocation* target = GetExplTargetDest();
                if (!caster || !target)
                    return;

                float x, y, z;
                x = target->GetPositionX();
                y = target->GetPositionY();
                z = caster->GetMap()->GetHeight(x, y, target->GetPositionZ());

                float dist = caster->GetDistance2d(x, y);

                if (dist < 4.0f)
                    caster->CastSpell(x, y, z, SPELL_JUGGLE_TORCH_SPEED_0, true);
                else if (dist < 15.0f)
                    caster->CastSpell(x, y, z, SPELL_JUGGLE_TORCH_SPEED_1, true);
                else if (dist < 27.5f)
                    caster->CastSpell(x, y, z, SPELL_JUGGLE_TORCH_SPEED_2, true);
                else
                    caster->CastSpell(x, y, z, SPELL_JUGGLE_TORCH_SPEED_3, true);
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_gen_juggle_torch_throw_SpellScript::HandleCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gen_juggle_torch_throw_SpellScript();
        }
};


/*###########
# boss_ahune
############*/

#define EMOTE_SUBMERGE "Ahune zieht sich zurueck, seine Verteidigung laesst nach!"
#define EMOTE_EMERGE_SOON "Ahune wird bald wieder auftauchen!"
#define GOSSIP_STONE_ITEM "Disturb the stone and summon Lord Ahune."

enum Spells
{
    // Ahune
    SPELL_SLIPPERY_FLOOR_AMBIENT            = 46314,
    SPELL_SUMMON_ICE_SPEAR_BUNNY            = 46359,
    SPELL_SUMMON_ICE_SPEAR_OBJECT           = 46369,
    SPELL_SELF_STUN                         = 46416,
    SPELL_RESURFACE                         = 46402,
    SPELL_SHIELD                            = 45954,
    SPELL_BEAM_ATTACK                       = 46336,
    SPELL_BEAM_ATTACK_BEAM                  = 46363,
    SPELL_SUBMERGE                          = 37550,
    SPELL_EMERGE                            = 50142,
    SPELL_SNOWY                             = 46423,
    // Ahunite Hailstone
    SPELL_CHILLING_AURA                     = 46542,
    SPELL_PULVERIZE                         = 2676,
    // Ahunite Frostwind
    SPELL_WIND_BUFFET                       = 46568,
    SPELL_LIGHTNING_SHIELD                  = 12550,
    // Ahunite Coldwave
    SPELL_BITTER_BLAST                      = 46406,
    // Ice Spear
    SPELL_ICE_SPEAR_KNOCKBACK               = 46360,
    // Slippery Floor
    SPELL_SLIP                              = 45947, // stun
    SPELL_YOU_SLIPPED                       = 45946, // decrease speed
    SPELL_SLIPPERY_FLOOR                    = 45945, // periodic, channeled
    // Achievement stuff
    SPELL_AHUNE_ACHIEVEMENT                 = 62043
};

enum Npcs
{
    NPC_FROSTLORD_AHUNE                     = 25740,
    NPC_FROZEN_CORE                         = 25865,
    NPC_AHUNITE_HAILSTONE                   = 25755,
    NPC_AHUNITE_COLDWAVE                    = 25756,
    NPC_AHUNITE_FROSTWIND                   = 25757,
    NPC_SLIPPERY_FLOOR                      = 25952,
    NPC_EARTHEN_RING_TOTEM                  = 25961,
    NPC_ICE_SPEAR_BUNNY                     = 25985,
    NPC_AHUNE_CONTROLLER                    = 1010602,
};

enum Objects
{
    GO_ICE_SPEAR                            = 188077,
    GO_ICE_STONE                            = 187882,
    GO_ICE_CHEST                            = 187892
};

enum Quests
{
    QUEST_SUMMON_AHUNE                      = 11691
};

enum Data
{
    DATA_IS_ENCOUNTER_IN_PROGRESS,
    DATA_CAN_RETRY,
};

enum Actions
{
    ACTION_START_EVENT,
    ACTION_START_EARTHEN_RING_ATTACK,
    ACTION_TRY_ENCOUNTER_AGAIN,
    ACTION_RESET_ENCOUNTER,
    ACTION_ENCOUNTER_FINISHED,
};

enum Events
{
    EVENT_EMERGE = 1,
    EVENT_EMERGE_SOON,
    EVENT_SUBMERGED,
    EVENT_EARTHEN_RING_ATTACK,
    EVENT_SUMMON_HAILSTONE,
    EVENT_SUMMON_COLDWAVE,
    EVENT_ICE_SPEAR,
    EVENT_INTRO
};

enum Achievements
{
    ACHIEV_ICE_THE_FROST_LORD               = 263,
};

Position const AhunePositions[4] =
{
    {-67.4483f, -161.512f, -4.2f, 2.86949f},
    {-125.76f, -137.004f, -2.16776f, 5.15523f},
    {-130.33f, -138.993f, -1.9702f, 5.15131f},
    {-135.043f, -141.094f, -1.76587f, 5.1356f},
};

class npc_frostlord_ahune : public CreatureScript
{
public:
    npc_frostlord_ahune() : CreatureScript("npc_frostlord_ahune") { }

    struct npc_frostlord_ahuneAI : public ScriptedAI
    {
        npc_frostlord_ahuneAI(Creature* c) : ScriptedAI(c), _summons(me)
        {
            SetCombatMovement(false);
        }

        void Reset()
        {
            _summons.DespawnAll();
            _events.Reset();
            _submerged = false;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_PASSIVE);
            me->SetCorpseDelay(20);
            me->SetVisible(false);
            DoCastSelf(SPELL_SHIELD, true);
            DoCastSelf(SPELL_SUBMERGE, true);

            if (Creature* controller = me->FindNearestCreature(NPC_AHUNE_CONTROLLER, 250.f, true))
                controller->AI()->DoAction(ACTION_RESET_ENCOUNTER);
        }

        void JustSummoned(Creature* summon)
        {
            switch (summon->GetEntry())
            {
            case NPC_FROZEN_CORE:
                summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
                summon->SetHealth(me->GetHealth());
                summon->SetReactState(REACT_PASSIVE);
                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                break;
            case NPC_SLIPPERY_FLOOR:
                summon->SetReactState(REACT_PASSIVE);
                summon->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                summon->SetDisplayId(11686);
                summon->setFaction(14);
                me->AddAura(SPELL_SLIPPERY_FLOOR_AMBIENT, summon);
                break;
            case NPC_ICE_SPEAR_BUNNY:
                summon->SetInCombatWithZone();
                return;
            }

            _summons.Summon(summon);
            summon->SetInCombatWithZone();
        }

        void SummonedCreatureDespawn(Creature* summon)
        {
            if (me->isDead())
                return;

            if (summon->GetEntry() == NPC_FROZEN_CORE)
                me->SetHealth(summon->GetHealth());
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
        {
            if (summon->GetEntry() == NPC_FROZEN_CORE)
                me->DealDamage(me, me->GetHealth());
        }

        void JustDied(Unit* /*who*/)
        {
            if (Creature* controller = me->FindNearestCreature(NPC_AHUNE_CONTROLLER, 250.f, true))
                controller->AI()->DoAction(ACTION_ENCOUNTER_FINISHED);

            me->SummonGameObject(GO_ICE_CHEST, -97.095f, -203.866f, -1.191f, 1.5f, 0, 0, 0, 0, 0);
            _summons.DespawnAll();

            
            // lfg reward as there is no ahune entry in dbcs dungeonencounters
            // TODO: unhack
            Map* map = me->GetMap();
            if (map && map->IsDungeon())
            {
                for (Player& player : map->GetPlayers())
                {
                    if (player.IsGameMaster() || !player.GetGroup())
                        continue;

                    sLFGMgr->FinishDungeon(player.GetGroup()->GetGUID(), 286);
                    break;
                }
            }
            

            if (InstanceScript* instance = me->GetInstanceScript())
            {
                instance->DoCompleteAchievement(ACHIEV_ICE_THE_FROST_LORD);
                instance->DoRemoveAurasDueToSpellOnPlayers(71328 /*LFG_SPELL_DUNGEON_COOLDOWN*/);
            }
        }

        void DoAction(int32 /*action*/)
        {
            me->SetVisible(true);
            me->SetReactState(REACT_AGGRESSIVE);
            me->SetInCombatWithZone();
            _events.ScheduleEvent(EVENT_EMERGE, 10000);

            if (GameObject* chest = me->FindNearestGameObject(GO_ICE_CHEST, 100.0f))
                chest->RemoveFromWorld();
        }

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_EMERGE:
                    me->SummonCreature(NPC_SLIPPERY_FLOOR, *me, TEMPSUMMON_TIMED_DESPAWN, 90000);
                    me->RemoveAurasDueToSpell(SPELL_SELF_STUN);
                    me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    DoCastSelf(SPELL_RESURFACE, true);
                    DoCast(SPELL_EMERGE);
                    _attacks = 0;
                    _events.ScheduleEvent(EVENT_EARTHEN_RING_ATTACK, 10000);
                    _events.ScheduleEvent(EVENT_SUMMON_HAILSTONE, 2000);
                    _events.ScheduleEvent(EVENT_SUMMON_COLDWAVE, 5000);
                    _events.ScheduleEvent(EVENT_ICE_SPEAR, 10000);
                    break;
                case EVENT_EMERGE_SOON:
                    me->MonsterTextEmote(EMOTE_EMERGE_SOON, 0, true);
                    break;
                case EVENT_SUBMERGED:
                    DoCastSelf(SPELL_SELF_STUN, true);
                    if(Creature* floor =  me->FindNearestCreature(NPC_SLIPPERY_FLOOR, 100.f))
                        floor->DespawnOrUnsummon();

                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    if(Creature* core = me->SummonCreature(NPC_FROZEN_CORE, *me, TEMPSUMMON_TIMED_DESPAWN, 30000))
                    {
                        core->SetMaxHealth(me->GetHealth());
                        core->SetFullHealth();
                    }
                    _submerged = true;
                    _events.ScheduleEvent(EVENT_EMERGE_SOON, 25000);
                    _events.ScheduleEvent(EVENT_EMERGE, 30000);
                    break;
                case EVENT_EARTHEN_RING_ATTACK:
                    ++_attacks;
                    if (_attacks >= 9)
                    {
                        me->MonsterTextEmote(EMOTE_SUBMERGE, 0, true);
                        DoCast(SPELL_SUBMERGE);
                        _events.Reset();
                        _events.ScheduleEvent(EVENT_SUBMERGED, 4500);
                    }
                    else
                    {
                        if (Creature* controller = me->FindNearestCreature(ACTION_START_EARTHEN_RING_ATTACK, 250.f, true))
                            controller->AI()->DoAction(ACTION_START_EARTHEN_RING_ATTACK);

                        _events.ScheduleEvent(EVENT_EARTHEN_RING_ATTACK, 10000);
                    }
                    break;
                case EVENT_SUMMON_HAILSTONE:
                    me->SummonCreature(NPC_AHUNITE_HAILSTONE, float(irand(-110, -80)), float(irand(-225, -215)), -1.0f, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
                    break;
                case EVENT_SUMMON_COLDWAVE:
                    for (uint8 i = 0; i < 2; ++i)
                        me->SummonCreature(NPC_AHUNITE_COLDWAVE, float(irand(-110, -80)), float(irand(-225, -215)), -1.0f, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
                    if (_submerged)
                        me->SummonCreature(NPC_AHUNITE_FROSTWIND, float(irand(-110, -80)), float(irand(-225, -215)), -1.0f, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 3000);
                    _events.ScheduleEvent(EVENT_SUMMON_COLDWAVE, 7000);
                    break;
                case EVENT_ICE_SPEAR:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                        DoCast(target, SPELL_SUMMON_ICE_SPEAR_BUNNY);
                    _events.ScheduleEvent(EVENT_ICE_SPEAR, 7000);
                    break;
                default:
                    break;
                }
            }
        }

    private:
        SummonList _summons;
        EventMap _events;
        bool _submerged;
        uint8 _attacks;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_frostlord_ahuneAI(creature);
    }
};

class npc_ahune_ice_spear : public CreatureScript
{
public:
    npc_ahune_ice_spear() : CreatureScript("npc_ahune_ice_spear") { }

    struct npc_ahune_ice_spearAI : public ScriptedAI
    {
        npc_ahune_ice_spearAI(Creature* creature) : ScriptedAI(creature)
        {
            SetCombatMovement(false);
            _spikeTimer = 2500;
            _spiked = false;
            DoCastSelf(SPELL_SUMMON_ICE_SPEAR_OBJECT, true);
        }

        void UpdateAI(uint32 diff)
        {
            if (_spikeTimer <= diff)
            {
                GameObject* spike = me->FindNearestGameObject(GO_ICE_SPEAR, 10.0f);
                if (spike && !_spiked)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_MINDISTANCE, 0, 3.0f, true))
                        target->CastSpell(target, SPELL_ICE_SPEAR_KNOCKBACK, true);
                    spike->UseDoorOrButton();
                    _spiked = true;
                    _spikeTimer = 3500;
                }
                else if (spike)
                {
                    spike->RemoveFromWorld();
                    me->DespawnOrUnsummon();
                }
            }
            else
                _spikeTimer -= diff;
        }

    private:
        uint32 _spikeTimer;
        bool _spiked;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ahune_ice_spearAI(creature);
    }
};

class npc_ahune_controller : public CreatureScript
{
public:
    npc_ahune_controller() : CreatureScript("npc_ahune_controller") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ahune_controllerAI (creature);
    }

    struct npc_ahune_controllerAI : public ScriptedAI
    {
        npc_ahune_controllerAI(Creature* creature) : ScriptedAI(creature), summons(me), isEncounterInProgress(false)
        {
            instance = creature->GetInstanceScript();
        }

        InstanceScript* instance;
        EventMap events;
        ObjectGuid uiPlayerGUID;
        uint8 introSteps;
        SummonList summons;
        bool isEncounterInProgress;

        void Reset()
        {
            if(!me->FindNearestGameObject(GO_ICE_STONE, 70.f))
                me->SummonGameObject(GO_ICE_STONE, AhunePositions[0].GetPositionX(), AhunePositions[0].GetPositionY(), AhunePositions[0].GetPositionZ(), AhunePositions[0].GetOrientation(),0,0,0,0,0);
        }

        void UpdateAI(uint32 diff)
        {
            if(!uiPlayerGUID)
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if(eventId == EVENT_INTRO)
                {
                    switch(introSteps)
                    {
                    case 0:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayerGUID))
                            player->MonsterSay("Der Eisbrocken ist geschmolzen!",0,0);

                        events.ScheduleEvent(EVENT_INTRO, 2500);
                        break;
                    case 1:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayerGUID))
                            player->MonsterSay("Ahune, staerker sollst du nicht mehr werden!",0,0);

                        events.ScheduleEvent(EVENT_INTRO, 1000);
                        break;
                    case 2:
                        if (Creature* ahune = me->FindNearestCreature(NPC_FROSTLORD_AHUNE, 100.0f, true))
                            ahune->AI()->DoAction(ACTION_START_EVENT);

                        events.ScheduleEvent(EVENT_INTRO, 1000);
                        break;
                    case 3:
                    case 4:
                    case 5:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayerGUID))
                            if(Creature* totem = me->SummonCreature(NPC_EARTHEN_RING_TOTEM, *player))
                            {
                                summons.Summon(totem);
                                totem->GetMotionMaster()->MoveJump(AhunePositions[introSteps-2], 15.f, 15.f);
                            }

                            if(introSteps < 5)
                                events.ScheduleEvent(EVENT_INTRO, 500);
                            else
                                events.ScheduleEvent(EVENT_INTRO, 6000);

                            break;
                    case 6:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, uiPlayerGUID))
                            player->MonsterSay("Niemals wird Kaelte herrschen!",0,0);

                        events.ScheduleEvent(EVENT_INTRO, 3000);
                        break;

                    case 7:
                        StartEarthenRingAttack();
                        break;
                    }

                    if(introSteps >= 7)
                    {
                        events.Reset();
                        introSteps = 0;
                    }else
                        introSteps++;
                }
            }
        }

        void DoAction(int32 action)
        {
            switch(action)
            {
            case ACTION_START_EARTHEN_RING_ATTACK:
                StartEarthenRingAttack();
                break;
            case ACTION_ENCOUNTER_FINISHED:
                isEncounterInProgress = 0;
            case ACTION_RESET_ENCOUNTER:
                introSteps = 0;
                summons.DespawnAll();
                Reset();
                break;
            case ACTION_TRY_ENCOUNTER_AGAIN:
                if(uiPlayerGUID && isEncounterInProgress && introSteps == 0)
                {
                    events.Reset();
                    events.ScheduleEvent(EVENT_INTRO, 3000);
                }
                break;
            }
        }

        void SetGuidData(ObjectGuid uiGuid, uint32 /*iId*/) override
        {
            isEncounterInProgress = true;
            uiPlayerGUID = uiGuid;
            introSteps = 0;
            events.ScheduleEvent(EVENT_INTRO, 3000);
        }

        uint32 GetData(uint32 type) const
        {
            if(type == DATA_IS_ENCOUNTER_IN_PROGRESS)
                return isEncounterInProgress;
            else if (type == DATA_CAN_RETRY)
                return uiPlayerGUID && isEncounterInProgress && introSteps == 0;

            return 0;
        }

        void StartEarthenRingAttack()
        {
            for(SummonList::const_iterator itr = summons.begin(); itr != summons.end(); itr++)
                if (Creature* totem = ObjectAccessor::GetCreature(*me, *itr))
                {
                    if (Creature* ahune = me->FindNearestCreature(NPC_FROSTLORD_AHUNE, 100.0f, true))
                        totem->CastSpell(ahune, SPELL_BEAM_ATTACK_BEAM, false);
                }
        }
    };
};

class go_ahune_ice_stone : public GameObjectScript
{
public:
    go_ahune_ice_stone() : GameObjectScript("go_ahune_ice_stone") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (Creature* controller = go->FindNearestCreature(NPC_AHUNE_CONTROLLER, 100.0f, true))
        {
            if (player->GetQuestStatus(11691) == QUEST_STATUS_COMPLETE)
            {
                player->PrepareQuestMenu(go->GetGUID());
                player->SendPreparedQuest(go->GetGUID());
            }
            else if (controller->AI()->GetData(DATA_CAN_RETRY))
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_STONE_ITEM, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(player->GetGossipTextId(go), go->GetGUID());
            }
            return true;
        }

        return false;
    }

    bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action)
    {
        if (action == GOSSIP_ACTION_INFO_DEF)
        {
            if (Creature* controller = go->FindNearestCreature(NPC_AHUNE_CONTROLLER, 100.0f, true))
                controller->AI()->DoAction(ACTION_TRY_ENCOUNTER_AGAIN);

            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
        }

        return true;
    }

    bool OnQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 /*opt*/)
    {
        if(quest->GetQuestId() == QUEST_SUMMON_AHUNE)
        {
            if (Creature* controller = go->FindNearestCreature(NPC_AHUNE_CONTROLLER, 100.0f, true))
                if (!controller->AI()->GetData(DATA_IS_ENCOUNTER_IN_PROGRESS))
                    controller->AI()->SetGuidData(player->GetGUID());
        }

        return true;
    }
};

enum IceStone
{
    QUEST_STRINKING_BACK_22  = 11917,
    QUEST_STRINKING_BACK_32  = 11947,
    QUEST_STRINKING_BACK_43  = 11948,
    QUEST_STRINKING_BACK_51  = 11952,
    QUEST_STRINKING_BACK_60  = 11953,
    QUEST_STRINKING_BACK_64  = 11954,

    NPC_FROSTWAVE_LIEUTENANT = 26116,
    NPC_HAILSTONE_LIEUTENANT = 26178,
    NPC_CHILLWIND_LIEUTENANT = 26204,
    NPC_FRIGID_LIEUTENANT    = 26214,
    NPC_GLACIAL_LIEUTENANT   = 26215,
    NPC_GLACIAL_TEMPLAR      = 26216,

    OBJECT_ICE_STONE_22      = 188149,
    OBJECT_ICE_STONE_32      = 188148,
    OBJECT_ICE_STONE_43      = 188150,
    OBJECT_ICE_STONE_51      = 188049,
    OBJECT_ICE_STONE_60      = 188138,
    OBJECT_ICE_STONE_64      = 188137,
};

class go_ice_stone_quests : public GameObjectScript
{
public:
    go_ice_stone_quests() : GameObjectScript("go_ice_stone_quests") { }

    bool OnGossipHello(Player* player, GameObject* go)
    {
        if (!player || !go)
            return false;

        Position nearpos;
        go->GetNearPosition(nearpos, frand(10.0f, 15.0f), frand(0.0f, 6.283f));

        switch(go->GetEntry())
        {
            case OBJECT_ICE_STONE_22:
                if(player->GetQuestStatus(QUEST_STRINKING_BACK_22) == QUEST_STATUS_INCOMPLETE)
                    if(!player->FindNearestCreature(NPC_FROSTWAVE_LIEUTENANT, 20.0f, true))
                        player->SummonCreature(NPC_FROSTWAVE_LIEUTENANT, nearpos, TEMPSUMMON_TIMED_DESPAWN, 60000);
                break;
            case OBJECT_ICE_STONE_32:
                if(player->GetQuestStatus(QUEST_STRINKING_BACK_32) == QUEST_STATUS_INCOMPLETE)
                    if(!player->FindNearestCreature(NPC_HAILSTONE_LIEUTENANT, 20.0f, true))
                        player->SummonCreature(NPC_HAILSTONE_LIEUTENANT, nearpos, TEMPSUMMON_TIMED_DESPAWN, 60000);
                break;
            case OBJECT_ICE_STONE_43:
                if(player->GetQuestStatus(QUEST_STRINKING_BACK_43) == QUEST_STATUS_INCOMPLETE)
                    if(!player->FindNearestCreature(NPC_CHILLWIND_LIEUTENANT, 20.0f, true))
                        player->SummonCreature(NPC_CHILLWIND_LIEUTENANT, nearpos, TEMPSUMMON_TIMED_DESPAWN, 60000);
                break;
            case OBJECT_ICE_STONE_51:
                if(player->GetQuestStatus(QUEST_STRINKING_BACK_51) == QUEST_STATUS_INCOMPLETE)
                    if(!player->FindNearestCreature(NPC_FRIGID_LIEUTENANT, 20.0f, true))
                        player->SummonCreature(NPC_FRIGID_LIEUTENANT, nearpos, TEMPSUMMON_TIMED_DESPAWN, 60000);
                break;
            case OBJECT_ICE_STONE_60:
                if(player->GetQuestStatus(QUEST_STRINKING_BACK_60) == QUEST_STATUS_INCOMPLETE)
                    if(!player->FindNearestCreature(NPC_GLACIAL_LIEUTENANT, 20.0f, true))
                        player->SummonCreature(NPC_GLACIAL_LIEUTENANT, nearpos, TEMPSUMMON_TIMED_DESPAWN, 60000);
                break;
            case OBJECT_ICE_STONE_64:
                if(player->GetQuestStatus(QUEST_STRINKING_BACK_64) == QUEST_STATUS_INCOMPLETE)
                    if(!player->FindNearestCreature(NPC_GLACIAL_TEMPLAR, 20.0f, true))
                        player->SummonCreature(NPC_GLACIAL_TEMPLAR, nearpos, TEMPSUMMON_TIMED_DESPAWN, 60000);
                break;
        }

        player->CLOSE_GOSSIP_MENU();
        return true;
    }
};

enum IceCallerBriatha
{
    NPC_HERETIC_EMISSARY        = 25951,
    QUEST_AN_INNOCENT_DISGUISE  = 11891,
    SPELL_CRAB_DISGUISE         = 46337,
    SAY_BRIATHA_1               = 0,
    SAY_BRIATHA_2               = 1,
    SAY_HERETIC_1               = 0,  
    NPC_TOTEM                   = 25324
};

class npc_ice_caller_briatha : public CreatureScript
{
    public:
        npc_ice_caller_briatha() : CreatureScript("npc_ice_caller_briatha") { }

        struct npc_ice_caller_briathaAI : public ScriptedAI
        {
            npc_ice_caller_briathaAI(Creature* creature) : ScriptedAI(creature) { }

            bool eventstart;
            uint32 uiEventTimer;
            uint8 phase;

            void Reset()
            {
                eventstart = false;
                uiEventTimer = 4000;
                phase = 0;
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (me->IsWithinDistInMap(who, 10.0f))
                    if (who->HasAura(SPELL_CRAB_DISGUISE))
                        if (who->ToPlayer()->GetQuestStatus(QUEST_AN_INNOCENT_DISGUISE) == QUEST_STATUS_INCOMPLETE)
                            eventstart = true;
            }

            void UpdateAI(uint32 diff)
            {
                if (eventstart)
                {
                    if(uiEventTimer < diff)
                    {
                        switch(phase)
                        {
                            case 0:
                                Talk(SAY_BRIATHA_1, me);
                                uiEventTimer = 3*IN_MILLISECONDS;
                                phase++;
                                break;
                            case 1:
                                if (Creature* heretic = me->FindNearestCreature(NPC_HERETIC_EMISSARY, 40.0f, true))
                                    heretic->AI()->Talk(SAY_HERETIC_1);
                                uiEventTimer = 5 * IN_MILLISECONDS;
                                phase++;
                                break;
                            case 2:
                                Talk(SAY_BRIATHA_2);
                                uiEventTimer = 2 * IN_MILLISECONDS;
                                phase++;
                                break;
                            case 3:
                                CompleteQuest();
                                uiEventTimer = 4 * IN_MILLISECONDS;
                                eventstart = false;
                                phase = 0;
                                break;
                            default:
                                eventstart = false;
                                phase = 0;
                                uiEventTimer = 4 * IN_MILLISECONDS;
                                break;
                        }
                    }
                    else uiEventTimer -= diff;
                }
                
                if (!UpdateVictim())
                    return;
            }

            void CompleteQuest()
            {
                if (Map* map = me->GetMap())
                {
                    for (Player& player : map->GetPlayers())
                    {
                        if (me->IsInRange(&player, 0.0f, 30.0f))
                            if (player.GetQuestStatus(QUEST_AN_INNOCENT_DISGUISE) == QUEST_STATUS_INCOMPLETE)
                                player.CompleteQuest(QUEST_AN_INNOCENT_DISGUISE);
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ice_caller_briathaAI(creature);
        }
};

class npc_totemic_beacon : public CreatureScript
{
    public:
        npc_totemic_beacon() : CreatureScript("npc_totemic_beacon") { }

        struct npc_totemic_beaconAI : public ScriptedAI
        {
            npc_totemic_beaconAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SummonCreature(NPC_TOTEM, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 60000);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_totemic_beaconAI(creature);
        }
};

enum RibbonPole
{
    SPELL_RIBBON_POLE_CHANNEL_VISUAL = 29172,
    SPELL_RIBBON_POLE_XP             = 29175,
    SPELL_RIBBON_POLE_FIREWORKS      = 46971,

    NPC_RIBBON_POLE_DEBUG_TARGET     = 17066,
};

class spell_midsummer_ribbon_pole : public SpellScriptLoader
{
    public:
        spell_midsummer_ribbon_pole() : SpellScriptLoader("spell_midsummer_ribbon_pole") { }

        class spell_midsummer_ribbon_pole_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_midsummer_ribbon_pole_AuraScript)

            void HandleEffectPeriodic(AuraEffect const * aurEff)
            {
                PreventDefaultAction();
                if (Unit *target = GetTarget())
                {
                    Creature* cr = target->FindNearestCreature(NPC_RIBBON_POLE_DEBUG_TARGET, 10.0f);
                    if (!cr)
                    {
                        target->RemoveAura(SPELL_RIBBON_POLE_CHANNEL_VISUAL);
                        SetDuration(1);
                        return;
                    }

                    if (Aura* aur = target->GetAura(SPELL_RIBBON_POLE_XP))
                        aur->SetDuration(std::min(aur->GetDuration() + 3 * MINUTE*IN_MILLISECONDS, 60 * MINUTE*IN_MILLISECONDS));
                    else
                        target->CastSpell(target, SPELL_RIBBON_POLE_XP, true);

                    if (roll_chance_i(5))
                    {
                        cr->Relocate(cr->GetPositionX(), cr->GetPositionY(), cr->GetPositionZ() - 6.5f);
                        cr->CastSpell(cr, SPELL_RIBBON_POLE_FIREWORKS, true);
                        cr->Relocate(cr->GetPositionX(), cr->GetPositionY(), cr->GetPositionZ() + 6.5f);
                    }

                    // Achievement
                    if ((time(NULL) - GetApplyTime()) > 60 && target->GetTypeId() == TYPEID_PLAYER && target->HasAura(SPELL_HAS_FULL_MIDSUMMER_SET))
                        target->ToPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET, 58934, 0, target);
                }
            }

            void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* ar = GetTarget();
                ar->CastSpell(ar, SPELL_RIBBON_POLE_CHANNEL_VISUAL, true);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_midsummer_ribbon_pole_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_midsummer_ribbon_pole_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_midsummer_ribbon_pole_AuraScript();
        }
};
enum eBonfire
{
    GO_MIDSUMMER_BONFIRE    = 181288,

    SPELL_STAMP_OUT_BONFIRE = 45437,
    SPELL_LIGHT_BONFIRE     = 29831,
};

class go_midsummer_bonfire : public GameObjectScript
{
    public:
        go_midsummer_bonfire() : GameObjectScript("go_midsummer_bonfire") { }
    
        bool OnGossipSelect(Player* player, GameObject* go, uint32 /*sender*/, uint32 action)
        {
            player->CLOSE_GOSSIP_MENU();
            // we know that there is only one gossip.
            player->CastSpell(player, SPELL_STAMP_OUT_BONFIRE, true);
            return true;
        }
};

class npc_midsummer_bonfire : public CreatureScript
{
    public:
        npc_midsummer_bonfire() : CreatureScript("npc_midsummer_bonfire") { }
    
        struct npc_midsummer_bonfireAI : public ScriptedAI
        {
            npc_midsummer_bonfireAI(Creature* c) : ScriptedAI(c)
            {
                if (IsHolidayActive(HOLIDAY_FIRE_FESTIVAL))
                {
                    me->IsAIEnabled = true;
                    goGUID.Clear();
                    if (GameObject* go = me->SummonGameObject(GO_MIDSUMMER_BONFIRE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 0))
                    {
                        goGUID = go->GetGUID();
                        me->RemoveGameObject(go, false);
                    }
                }                
            }
    
            ObjectGuid goGUID;
    
            void SpellHit(Unit*, SpellInfo const* spellInfo) override
            {
                if (!goGUID)
                    return;
    
                // Extinguish fire
                if (spellInfo->Id == SPELL_STAMP_OUT_BONFIRE)
                {
                    if (GameObject* go = ObjectAccessor::GetGameObject(*me, goGUID))
                        go->SetPhaseMask(2, true);
                }
                else if (spellInfo->Id == SPELL_LIGHT_BONFIRE)
                {
                    if (GameObject* go = ObjectAccessor::GetGameObject(*me, goGUID))
                    {
                        go->SetPhaseMask(1, true);
                        go->SendCustomAnim(1);
                    }
                }
    
            }
        };
    
        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_midsummer_bonfireAI(creature);
        }
};


void AddSC_midsummer_fire_festival()
{
    new spell_gen_ribbon_pole_dancer_check();
    new npc_torch_tossing_bunny();
    new spell_gen_torch_target_picker();
    new spell_gen_juggle_torch_catch();
    new spell_gen_juggle_torch_juggle_catch();
    new spell_gen_juggle_torch_throw();
    new go_ice_stone_quests();
    new npc_ice_caller_briatha();
    new npc_totemic_beacon();
    new npc_frostlord_ahune();
    new npc_ahune_ice_spear();
    new npc_ahune_controller();
    new go_ahune_ice_stone();
    new spell_midsummer_ribbon_pole();
    new npc_midsummer_bonfire();
    new go_midsummer_bonfire();
}
