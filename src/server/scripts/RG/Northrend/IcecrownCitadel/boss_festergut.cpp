/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "icecrown_citadel.h"
#include "area_plague_works.hpp"

enum ScriptTexts
{
    // Festergut
    SAY_STINKY_DEAD             = 0,
    SAY_AGGRO                   = 1,
    EMOTE_GAS_SPORE             = 2,
    EMOTE_WARN_GAS_SPORE        = 3,
    SAY_PUNGENT_BLIGHT          = 4,
    EMOTE_WARN_PUNGENT_BLIGHT   = 5,
    EMOTE_PUNGENT_BLIGHT        = 6,
    SAY_KILL                    = 7,
    SAY_BERSERK                 = 8,
    SAY_DEATH                   = 9,

    // Putricide
    SAY_FESTERGUT_DEATH         = 1,
};

enum Spells
{
    // Festergut
    SPELL_INHALE_BLIGHT         = 69165,
    SPELL_PUNGENT_BLIGHT        = 69195,
    SPELL_GASTRIC_BLOAT         = 72219, // 72214 is the proper way (with proc) but atm procs can't have cooldown for creatures
    SPELL_GASTRIC_EXPLOSION     = 72227,
    SPELL_GAS_SPORE             = 69278,
    SPELL_VILE_GAS              = 71307,
    SPELL_INOCULATED            = 69291,

    // Stinky
    SPELL_MORTAL_WOUND          = 71127,
    SPELL_DECIMATE              = 71123,
    SPELL_PLAGUE_STENCH         = 71805,
};

// Used for HasAura checks
#define PUNGENT_BLIGHT_HELPER RAID_MODE<uint32>(69195, 71219, 73031, 73032)
#define INOCULATED_HELPER     RAID_MODE<uint32>(69291, 72101, 72102, 72103)

constexpr uint32 gaseousBlight[3]        = { 69157, 69162, 69164 };
constexpr uint32 gaseousBlightVisual[3]  = { 69126, 69152, 69154 };

enum Events
{
    // Festergut
    EVENT_CALL_PROF = 1,
    EVENT_BERSERK,
    EVENT_GASTRIC_BLOAT,
    EVENT_INHALE_BLIGHT,
    EVENT_GAS_SPORE,
    EVENT_VILE_GAS,
    EVENT_RESET_TARGET,

    // Stinky
    EVENT_DECIMATE,
    EVENT_MORTAL_WOUND,
    EVENT_DOUBLE_DAMAGE,
    EVENT_NORMAL_DAMAGE,
};

enum Misc
{
    DATA_VILE_TARGET        = 71307,
    DATA_INOCULATED_STACK   = 69291,
};

struct boss_festergutAI : public BossAI
{
    boss_festergutAI(Creature* creature) : BossAI(creature, DATA_FESTERGUT) { }

    void Reset() override
    {
        BossAI::Reset();
        me->SetReactState(REACT_AGGRESSIVE);
        _maxInoculatedStack = 0;
        _inhaleCounter = 0;
    }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);

        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_CALL_PROF, 100ms);
        events.ScheduleEvent(EVENT_GASTRIC_BLOAT, 2s);
        events.ScheduleEvent(EVENT_VILE_GAS, 12s);
        events.ScheduleEvent(EVENT_GAS_SPORE, 20s);
        events.ScheduleEvent(EVENT_INHALE_BLIGHT, 30s);
        events.ScheduleEvent(EVENT_BERSERK, 5min);

        DoCastSelf(SPELL_GASTRIC_BLOAT);

        if (Creature* gasDummy = me->FindNearestCreature(NPC_GAS_DUMMY, 100.0f, true))
            _gasDummyGUID = gasDummy->GetGUID();
    }

    void JustDied(Unit* killer) override
    {
        BossAI::JustDied(killer);
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_INOCULATED);
        Talk(SAY_DEATH);

        if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
        {
            professor->AI()->Talk(SAY_FESTERGUT_DEATH);
            professor->AI()->EnterEvadeMode();
        }

        RemoveBlight();
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        RemoveBlight();
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_INOCULATED);

        if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
            professor->AI()->EnterEvadeMode();

        _DespawnAtEvade();
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
    }

    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        if (spell->Id == PUNGENT_BLIGHT_HELPER)
            target->RemoveAurasDueToSpell(INOCULATED_HELPER);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_CALL_PROF:
                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_FESTERGUT_COMBAT);
                break;
            case EVENT_GASTRIC_BLOAT:
                DoCastVictim(SPELL_GASTRIC_BLOAT);
                events.Repeat(10s, 13s);
                break;
            case EVENT_INHALE_BLIGHT:
                RemoveBlight();
                if (_inhaleCounter == 3)
                {
                    Talk(EMOTE_WARN_PUNGENT_BLIGHT);
                    Talk(SAY_PUNGENT_BLIGHT);
                    DoCastAOE(SPELL_PUNGENT_BLIGHT);
                    _inhaleCounter = 0;
                    if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
                        professor->AI()->DoAction(ACTION_FESTERGUT_GAS);
                    events.RescheduleEvent(EVENT_GAS_SPORE, 20s);
                }
                else
                {
                    DoCastSelf(SPELL_INHALE_BLIGHT);
                    // just cast and dont bother with target, conditions will handle it
                    ++_inhaleCounter;
                    if (_inhaleCounter < 3)
                        me->CastSpell(me, gaseousBlight[_inhaleCounter], true, NULL, NULL, me->GetGUID());
                }

                events.Repeat(34s);
                break;
            case EVENT_VILE_GAS:
                DoCastAOE(SPELL_VILE_GAS);
                events.Repeat(30s, 45s);
                break;
            case EVENT_RESET_TARGET:
                if (me->GetVictim())
                    me->SetTarget(me->EnsureVictim()->GetGUID());
                break;
            case EVENT_GAS_SPORE:
                Talk(EMOTE_WARN_GAS_SPORE);
                Talk(EMOTE_GAS_SPORE);
                me->CastCustomSpell(SPELL_GAS_SPORE, SPELLVALUE_MAX_TARGETS, RAID_MODE<int32>(2, 3, 2, 3), me);
                events.Repeat(41s);

                if ((events.GetNextEventTime(EVENT_VILE_GAS) - events.GetTimer()) < 20000)
                    events.RescheduleEvent(EVENT_VILE_GAS, 20s, 25s);
                break;
            case EVENT_BERSERK:
                DoCastSelf(SPELL_BERSERK2, true);
                Talk(SAY_BERSERK);
                break;
            default:
                break;
        }
    }

    void SetGuidData(ObjectGuid guid, uint32 type) override
    {
        if (type == DATA_VILE_TARGET)
        {
            me->SetTarget(guid);
            events.ScheduleEvent(EVENT_RESET_TARGET, 750ms);
        }
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == DATA_INOCULATED_STACK && data > _maxInoculatedStack)
            _maxInoculatedStack = data;
    }

    uint32 GetData(uint32 type) const override
    {
        if (type == DATA_INOCULATED_STACK)
            return uint32(_maxInoculatedStack);

        return 0;
    }

    void RemoveBlight()
    {
        if (Creature* gasDummy = ObjectAccessor::GetCreature(*me, _gasDummyGUID))
            for (uint8 i = 0; i < 3; ++i)
            {
                me->RemoveAurasDueToSpell(gaseousBlight[i]);
                gasDummy->RemoveAurasDueToSpell(gaseousBlightVisual[i]);
            }
    }

private:
    ObjectGuid _gasDummyGUID;
    uint32 _maxInoculatedStack;
    uint32 _inhaleCounter;
};

struct npc_stinky_iccAI : public ScriptedAI
{
    npc_stinky_iccAI(Creature* creature) : ScriptedAI(creature)
    {
        _instance = creature->GetInstanceScript();
        me->ApplySpellImmune(0, IMMUNITY_ID, 9484, true);  // Shackle Undead
        me->ApplySpellImmune(0, IMMUNITY_ID, 9485, true);  // Shackle Undead
        me->ApplySpellImmune(0, IMMUNITY_ID, 10955, true); // Shackle Undead
    }

    void Reset() override
    {
        _events.Reset();
        _events.ScheduleEvent(EVENT_DECIMATE, 20s, 25s);
        _events.ScheduleEvent(EVENT_MORTAL_WOUND, 2s);
        _events.ScheduleEvent(EVENT_DOUBLE_DAMAGE, 4s);
        if (_instance->GetBossState(DATA_FESTERGUT) == DONE)
            me->DespawnOrUnsummon();
    }

    void EnterCombat(Unit* /*target*/) override
    {
        DoCastSelf(SPELL_PLAGUE_STENCH);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_DECIMATE:
                    DoCastVictim(SPELL_DECIMATE);
                    _events.ScheduleEvent(EVENT_DECIMATE, 20s, 25s);
                    break;
                case EVENT_MORTAL_WOUND:
                    DoCastVictim(SPELL_MORTAL_WOUND, true);
                    _events.ScheduleEvent(EVENT_MORTAL_WOUND, 2s, 4s);
                    break;
                case EVENT_DOUBLE_DAMAGE:
                    DoubleDamage();
                    _events.ScheduleEvent(EVENT_NORMAL_DAMAGE, 5s);
                    break;
                case EVENT_NORMAL_DAMAGE:
                    NormalDamage();
                    _events.ScheduleEvent(EVENT_DOUBLE_DAMAGE, 10s);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* festergut = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_FESTERGUT)))
            if (festergut->IsAlive())
                festergut->AI()->Talk(SAY_STINKY_DEAD);
    }

private:
    void NormalDamage()
    {
        const CreatureTemplate* cinfo = me->GetCreatureTemplate();
        me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, cinfo->mindmg);
        me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, cinfo->maxdmg);
        me->UpdateDamagePhysical(BASE_ATTACK);
    }

    void DoubleDamage()
    {
        const CreatureTemplate* cinfo = me->GetCreatureTemplate();
        me->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, 2 * cinfo->mindmg);
        me->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, 2 * cinfo->maxdmg);
        me->UpdateDamagePhysical(BASE_ATTACK);
    }

    EventMap _events;
    InstanceScript* _instance;
};

class spell_festergut_pungent_blight_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_festergut_pungent_blight_SpellScript);

    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_UNIT;
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        // Get Inhaled Blight id for our difficulty
        uint32 blightId = sSpellMgr->GetSpellIdForDifficulty(uint32(GetEffectValue()), GetCaster());

        // ...and remove it
        GetCaster()->RemoveAurasDueToSpell(blightId);
        GetCaster()->ToCreature()->AI()->Talk(EMOTE_PUNGENT_BLIGHT);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_festergut_pungent_blight_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_festergut_gastric_bloat_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_festergut_gastric_bloat_SpellScript);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        if (!sSpellMgr->GetSpellInfo(SPELL_GASTRIC_EXPLOSION))
            return false;
        return true;
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        Aura const* aura = GetHitUnit()->GetAura(GetSpellInfo()->Id);
        if (!(aura && aura->GetStackAmount() == 10))
            return;

        GetHitUnit()->RemoveAurasDueToSpell(GetSpellInfo()->Id);
        GetHitUnit()->CastSpell(GetHitUnit(), SPELL_GASTRIC_EXPLOSION, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_festergut_gastric_bloat_SpellScript::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_festergut_blighted_spores_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_festergut_blighted_spores_AuraScript);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        if (!sSpellMgr->GetSpellInfo(SPELL_INOCULATED))
            return false;
        return true;
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        uint32 spellForDifficulty = sSpellMgr->GetSpellIdForDifficulty(SPELL_INOCULATED, GetTarget());
        uint32 stackAmount = GetTarget()->GetAuraCount(spellForDifficulty) + 1;

        GetTarget()->CastSpell(GetTarget(), SPELL_INOCULATED, true);
        if (InstanceScript* instance = GetTarget()->GetInstanceScript())
            if (Creature* festergut = ObjectAccessor::GetCreature(*GetTarget(), instance->GetGuidData(DATA_FESTERGUT)))
                festergut->AI()->SetData(DATA_INOCULATED_STACK, stackAmount);

        HandleResidue();
    }

    void HandleResidue()
    {
        Player* target = GetUnitOwner()->ToPlayer();
        if (!target)
            return;

        if (target->HasAura(SPELL_ORANGE_BLIGHT_RESIDUE))
            return;

        if (target->GetMap() && !target->GetMap()->Is25ManRaid())
        {
            if (target->GetQuestStatus(QUEST_RESIDUE_RENDEZVOUS_10) != QUEST_STATUS_INCOMPLETE)
                return;

            target->CastSpell(target, SPELL_ORANGE_BLIGHT_RESIDUE, TRIGGERED_FULL_MASK);
        }

        if (target->GetMap() && target->GetMap()->Is25ManRaid())
        {
            if (target->GetQuestStatus(QUEST_RESIDUE_RENDEZVOUS_25) != QUEST_STATUS_INCOMPLETE)
                return;

            target->CastSpell(target, SPELL_ORANGE_BLIGHT_RESIDUE, TRIGGERED_FULL_MASK);
        }
    }

    void Register() override
    {
        OnEffectRemove += AuraEffectRemoveFn(spell_festergut_blighted_spores_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_festergut_vile_gas_initial_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_festergut_vile_gas_initial_SpellScript);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        BossDistanceCheck(GetCaster(), targets);

        Trinity::Containers::RandomResize(targets, 1);
    }

    void HandleScript(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);

        Unit* target = GetHitUnit();

        GetCaster()->ToCreature()->AI()->SetGuidData(target->GetGUID(), DATA_VILE_TARGET);
        GetCaster()->CastSpell(target, GetSpellInfo()->Effects[EFFECT_0].CalcValue(), true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_festergut_vile_gas_initial_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_festergut_vile_gas_initial_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class achievement_flu_shot_shortage : public AchievementCriteriaScript
{
    public:
        achievement_flu_shot_shortage() : AchievementCriteriaScript("achievement_flu_shot_shortage") { }

        bool OnCheck(Player* /*source*/, Unit* target) override
        {
            if (target && target->GetTypeId() == TYPEID_UNIT)
                return target->ToCreature()->AI()->GetData(DATA_INOCULATED_STACK) < 3;

            return false;
        }
};

void AddSC_boss_festergut()
{
    new CreatureScriptLoaderEx<boss_festergutAI>("boss_festergut");
    new CreatureScriptLoaderEx<npc_stinky_iccAI>("npc_stinky_icc");
    new SpellScriptLoaderEx<spell_festergut_pungent_blight_SpellScript>("spell_festergut_pungent_blight");
    new SpellScriptLoaderEx<spell_festergut_gastric_bloat_SpellScript>("spell_festergut_gastric_bloat");
    new SpellScriptLoaderEx<spell_festergut_blighted_spores_AuraScript>("spell_festergut_blighted_spores");
    new SpellScriptLoaderEx<spell_festergut_vile_gas_initial_SpellScript>("spell_festergut_vile_gas_initial");
    new achievement_flu_shot_shortage();
}
