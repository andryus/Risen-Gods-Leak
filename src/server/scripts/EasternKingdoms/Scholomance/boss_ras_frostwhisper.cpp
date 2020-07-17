/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
#include "SpellInfo.h"
#include "SpellScript.h"
#include "ScriptedCreature.h"

enum RasFrostwhisper
{
    NPC_RAS_FROSTWHISPER = 10508,

    SPELL_FROSTBOLT = 21369,
    SPELL_ICE_ARMOR = 18100, // This is actually a buff he gives himself
    SPELL_FREEZE = 18763,
    SPELL_FEAR = 26070,
    SPELL_CHILL_NOVA = 18099,
    SPELL_FROSTVOLLEY = 8398,

    SPELL_BOON_OF_LIFE = 17179, // Soulbound Keepsake - Item 13752
    SPELL_RAS_FROSTWHISPER_DUMMY = 17190, // Triggered Dummy Spell
    SPELL_RAS_BECOMES_BOY = 17186, // Transformation
    SPELL_FROSTWHISPERS_LIFEBLOOD = 17189, // Not sure when this spell is used

    EVENT_FROSTBOLT = 1,
    EVENT_ICE_ARMOR,
    EVENT_FREEZE,
    EVENT_FEAR,
    EVENT_CHILL_NOVA,
    EVENT_FROSTVOLLEY,

    ACTION_TRANSFORM
};


struct boss_rasfrostAI : public ScriptedAI
{
    boss_rasfrostAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        events.Reset();
        me->SetLootMode(LOOT_MODE_DEFAULT);
        me->RemoveAurasDueToSpell(SPELL_RAS_BECOMES_BOY);
        DoCastSelf(SPELL_ICE_ARMOR);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        events.ScheduleEvent(EVENT_ICE_ARMOR, 2000);
        events.ScheduleEvent(EVENT_FROSTBOLT, 8000);
        events.ScheduleEvent(EVENT_CHILL_NOVA, 12000);
        events.ScheduleEvent(EVENT_FREEZE, 18000);
        events.ScheduleEvent(EVENT_FEAR, 45000);
    }

    // On Periodic - Spell 17179
    void DoAction(int32 id) override
    {
        if (id == ACTION_TRANSFORM)
        {
            DoCastSelf(SPELL_RAS_BECOMES_BOY, true);
            me->SetLootMode(LOOT_MODE_HARD_MODE_1);     // Use Lootmode to switch to the loot template with the human head
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
            case EVENT_ICE_ARMOR:
                DoCastSelf(SPELL_ICE_ARMOR);
                events.ScheduleEvent(EVENT_ICE_ARMOR, 180000);
                break;
            case EVENT_FROSTBOLT:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 40.0f, true))
                    DoCast(target, SPELL_FROSTBOLT);
                events.ScheduleEvent(EVENT_FROSTBOLT, 8000);
                break;
            case EVENT_FREEZE:
                DoCastVictim(SPELL_FREEZE);
                events.ScheduleEvent(EVENT_FREEZE, 24000);
                break;
            case EVENT_FEAR:
                DoCastVictim(SPELL_FEAR);
                events.ScheduleEvent(EVENT_FEAR, 30000);
                break;
            case EVENT_CHILL_NOVA:
                DoCastVictim(SPELL_CHILL_NOVA);
                events.ScheduleEvent(EVENT_CHILL_NOVA, 14000);
                break;
            case EVENT_FROSTVOLLEY:
                DoCastVictim(SPELL_FROSTVOLLEY);
                events.ScheduleEvent(EVENT_FROSTVOLLEY, 15000);
                break;
            default:
                break;
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap events;
};


// 17179 - Boon of Life



struct spell_boon_of_life_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_boon_of_life_AuraScript);

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Creature* target = GetTarget()->ToCreature())
        {
            if (!target->GetVictim())
            {
                target->GetAI()->AttackStart(GetCaster());
            }
        }
    }

    void HandleEffectPeriodic(AuraEffect const* aurEff)
    {
        PreventDefaultAction();

        if (Creature* target = GetTarget()->ToCreature())
        {
            if (target->GetEntry() == NPC_RAS_FROSTWHISPER)
            {
                // Check if Ras is already in human form
                if (target->HasAura(SPELL_RAS_BECOMES_BOY))
                    return;
                target->GetAI()->DoAction(ACTION_TRANSFORM);
                target->GetThreatManager().doAddThreat(GetCaster(), 1000.0f);
            }
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_boon_of_life_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_boon_of_life_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};


void AddSC_boss_rasfrost()
{
    new CreatureScriptLoaderEx<boss_rasfrostAI>("boss_boss_ras_frostwhisper");
    new SpellScriptLoaderEx<spell_boon_of_life_AuraScript>("spell_boon_of_life");
}
