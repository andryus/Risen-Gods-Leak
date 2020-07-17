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

#include "event.h"
#include "GameEventMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

class npc_love_is_in_the_air_trigger : public CreatureScript
{
public:
    npc_love_is_in_the_air_trigger() : CreatureScript("npc_love_is_in_the_air_trigger") { }

    struct npc_love_is_in_the_air_triggerAI : public ScriptedAI
    {
        npc_love_is_in_the_air_triggerAI(Creature* creature) : ScriptedAI(creature) {}

        void JustDied(Unit* /*killer*/)
        {
            me->SetCorpseDelay(1);
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new npc_love_is_in_the_air_triggerAI(creature);
    }
};

// 69445 - Perfume Spritz

class spell_perfume_spritz : public SpellScriptLoader
{
    public:
        spell_perfume_spritz() : SpellScriptLoader("spell_perfume_spritz") { }

        class spell_perfume_spritz_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_perfume_spritz_SpellScript)

            SpellCastResult CheckCast()
            {
                Unit * target = GetExplTargetUnit();
                if (target->HasAura(SPELL_AURA_MOD_SHAPESHIFT))
                    return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_perfume_spritz_SpellScript::CheckCast);
            }
        };

        SpellScript * GetSpellScript() const
        {
            return new spell_perfume_spritz_SpellScript();
        }
};

class spell_event_lovely_card : public SpellScriptLoader
{
    public:
        spell_event_lovely_card() : SpellScriptLoader("spell_event_lovely_card") { }

        class spell_event_lovely_card_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_event_lovely_card_AuraScript);

            void HandleEffectCalcAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Unit* caster = GetCaster())
                {
                    const int32 level = caster->getLevel();
                    amount = std::max(level - 15, 0) * 0.4155f + 3;
                }
            }

            void Register() override
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_event_lovely_card_AuraScript::HandleEffectCalcAmount, EFFECT_0, SPELL_AURA_MOD_STAT);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_event_lovely_card_AuraScript();
        }
};

enum ThrowCubidsDart
{
    NPC_KWEE_Q_PEDDLEFEET   = 16075,
    SPELL_SUMMON_PEDDLEFEET = 27570,
};

class spell_throw_cubids_dart_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_throw_cubids_dart_SpellScript);

    SpellCastResult CheckRequirement()
    {
        if (Player* target = GetExplTargetUnit()->ToPlayer())
            for (auto minion : target->m_Controlled)
                if (minion->GetCreatureType() == CREATURE_TYPE_NON_COMBAT_PET)
                    return SPELL_FAILED_BAD_TARGETS;


        return SPELL_CAST_OK;
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            target->CastSpell(target, SPELL_SUMMON_PEDDLEFEET);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_throw_cubids_dart_SpellScript::CheckRequirement);
        OnEffectHitTarget += SpellEffectFn(spell_throw_cubids_dart_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

enum Quest_HotOnTheTrailMisc
{
    AREA_TRIGGER_SW_BANK        = 5710,
    AREA_TRIGGER_SW_BARBER      = 5712,
    AREA_TRIGGER_SW_AH          = 5711,

    AREA_TRIGGER_OG_BANK        = 5715,
    AREA_TRIGGER_OG_BARBER      = 5716,
    AREA_TRIGGER_OG_AH          = 5714,

    QUEST_HOT_IN_TRAIL_A        = 24849,
    QUEST_HOT_IN_TRAIL_H        = 24851,
    
    EVENT_LOVE_IS_IN_THE_AIR    = 8,

    SPELL_CREDIT_SW_BANK        = 71712,
    SPELL_CREDIT_SW_AH          = 71744,
    SPELL_CREDIT_SW_BARBER      = 71751,

    SPELL_CREDIT_OG_BANK        = 71762,
    SPELL_CREDIT_OG_AH          = 71763,
    SPELL_CREDIT_OG_BARBER      = 71765,
};

class at_love_is_in_the_air_hot_on_the_trail : public AreaTriggerScript
{
public:
    at_love_is_in_the_air_hot_on_the_trail() : AreaTriggerScript("at_love_is_in_the_air_hot_on_the_trail") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* areaTrigger) override
    {
        if (sGameEventMgr->IsActiveEvent(EVENT_LOVE_IS_IN_THE_AIR) && player)
        {
            if (player->GetQuestStatus(QUEST_HOT_IN_TRAIL_A) == QUEST_STATUS_INCOMPLETE || player->GetQuestStatus(QUEST_HOT_IN_TRAIL_H) == QUEST_STATUS_INCOMPLETE)
            {
                if (areaTrigger->id == AREA_TRIGGER_SW_BANK && player->GetKilledCreatureCountForQuest(QUEST_HOT_IN_TRAIL_A, 0, 0))
                    player->CastSpell(player, SPELL_CREDIT_SW_BANK);
                else if (areaTrigger->id == AREA_TRIGGER_SW_BARBER && player->GetKilledCreatureCountForQuest(QUEST_HOT_IN_TRAIL_A, 2, 0))
                    player->CastSpell(player, SPELL_CREDIT_SW_BARBER);
                else if (areaTrigger->id == AREA_TRIGGER_SW_AH && player->GetKilledCreatureCountForQuest(QUEST_HOT_IN_TRAIL_A, 1, 0))
                    player->CastSpell(player, SPELL_CREDIT_SW_AH);
                else if (areaTrigger->id == AREA_TRIGGER_OG_BANK && player->GetKilledCreatureCountForQuest(QUEST_HOT_IN_TRAIL_H, 0, 0))
                    player->CastSpell(player, SPELL_CREDIT_OG_BANK);
                else if (areaTrigger->id == AREA_TRIGGER_OG_BARBER && player->GetKilledCreatureCountForQuest(QUEST_HOT_IN_TRAIL_H, 2, 0))
                    player->CastSpell(player, SPELL_CREDIT_OG_BARBER);
                else if (areaTrigger->id == AREA_TRIGGER_OG_AH && player->GetKilledCreatureCountForQuest(QUEST_HOT_IN_TRAIL_H, 1, 0))
                    player->CastSpell(player, SPELL_CREDIT_OG_AH);
            }
        }

        return true;
    }
};

void AddSC_event_love_is_in_the_air()
{
    new npc_love_is_in_the_air_trigger();
    new spell_perfume_spritz();
    new spell_event_lovely_card();
    new SpellScriptLoaderEx<spell_throw_cubids_dart_SpellScript>("spell_throw_cubids_dart");
    new at_love_is_in_the_air_hot_on_the_trail();
}
