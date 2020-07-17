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
#include "GridNotifiers.h"
#include "MoveSplineInit.h"
#include "icecrown_citadel.h"
#include "area_plague_works.hpp"

enum Texts
{
    SAY_PRECIOUS_DIES           = 0,
    SAY_AGGRO                   = 1,
    EMOTE_SLIME_SPRAY           = 2,
    SAY_SLIME_SPRAY             = 3,
    SAY_UNSTABLE_EXPLOSION      = 5,
    SAY_KILL                    = 6,
    SAY_BERSERK                 = 7,
    SAY_DEATH                   = 8,
    EMOTE_MUTATED_INFECTION     = 9,

    EMOTE_UNSTABLE_2            = 0,
    EMOTE_UNSTABLE_3            = 1,
    EMOTE_UNSTABLE_4            = 2,
    EMOTE_UNSTABLE_EXPLOSION    = 3,

    // Putricide
    SAY_ROTFACE_DEATH           = 3,

    // Precious
    EMOTE_PRECIOUS_ZOMBIES      = 0,
};

enum Spells
{
    // Rotface
    SPELL_SLIME_SPRAY                       = 69508,
    SPELL_MUTATED_INFECTION                 = 69674,

    // Oozes
    SPELL_LITTLE_OOZE_COMBINE               = 69537,    // combine 2 Small Oozes
    SPELL_LARGE_OOZE_COMBINE                = 69552,    // combine 2 Large Oozes
    SPELL_LARGE_OOZE_BUFF_COMBINE           = 69611,    // combine Large and Small Ooze
    SPELL_OOZE_MERGE                        = 69889,    // 2 Small Oozes summon a Large Ooze
    SPELL_WEAK_RADIATING_OOZE               = 69750,    // passive damage aura - small
    SPELL_RADIATING_OOZE                    = 69760,    // passive damage aura - large
    SPELL_UNSTABLE_OOZE                     = 69558,    // damage boost and counter for explosion
    SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC = 70001,    // prevents getting hit by infection
    SPELL_UNSTABLE_OOZE_EXPLOSION           = 69839,
    SPELL_STICKY_OOZE                       = 69774,
    SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER   = 69832,
    SPELL_VERTEX_COLOR_PINK                 = 53213,
    SPELL_VERTEX_COLOR_BRIGHT_RED           = 69844,
    SPELL_VERTEX_COLOR_DARK_RED             = 44773,

    // Precious
    SPELL_MORTAL_WOUND                      = 71127,
    SPELL_DECIMATE                          = 71123,
    SPELL_AWAKEN_PLAGUED_ZOMBIES            = 71159,
    SPELL_RIBBON                            = 70404,

    // Professor Putricide
    SPELL_VILE_GAS_H                        = 72272,
    SPELL_VILE_GAS_TRIGGER                  = 72285,
};

enum Events
{
    // Rotface
    EVENT_CALL_PROF = 1,
    EVENT_SLIME_SPRAY,
    EVENT_MUTATED_INFECTION,
    EVENT_VILE_GAS,
    EVENT_ENRAGE,

    // Ooze
    EVENT_STICKY_OOZE,
    EVENT_BIG_OOZE_SUMMON,

    // Precious
    EVENT_DECIMATE,
    EVENT_MORTAL_WOUND,
    EVENT_SUMMON_ZOMBIES,
    EVENT_DOUBLE_DAMAGE,
    EVENT_NORMAL_DAMAGE,
};

enum Action
{
    ACTION_ADD_MERGE = 1,
};

enum Misc
{
    DISPLAY_INVISIBLE = 11686,
};

struct boss_rotfaceAI : public BossAI
{
    boss_rotfaceAI(Creature* creature) : BossAI(creature, DATA_ROTFACE) { }

    void Reset() override
    {
        BossAI::Reset();
        me->SetReactState(REACT_AGGRESSIVE);

        infectionStage = 0;
        infectionCooldown = 14s;
    }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);

        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_CALL_PROF, 100ms);
        events.ScheduleEvent(EVENT_SLIME_SPRAY, 15s);
        events.ScheduleEvent(EVENT_MUTATED_INFECTION, 14s);
        events.ScheduleEvent(EVENT_ENRAGE, 10min);

        if (IsHeroic())
            events.ScheduleEvent(EVENT_VILE_GAS, 25s);

        DoCastSelf(SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);
    }

    void JustDied(Unit* killer) override
    {
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MUTATED_INFECTION);

        BossAI::JustDied(killer);
        Talk(SAY_DEATH);

        if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
        {
            professor->AI()->Talk(SAY_ROTFACE_DEATH);
            professor->AI()->EnterEvadeMode();
        }
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
            professor->AI()->EnterEvadeMode();

        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_MUTATED_INFECTION);
        instance->SetData(DATA_OOZE_DANCE_ACHIEVEMENT, uint32(true));

        _DespawnAtEvade();
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_KILL);
    }

    void SpellHitTarget(Unit* /*target*/, SpellInfo const* spell) override
    {
        if (spell->Id == SPELL_SLIME_SPRAY)
            Talk(SAY_SLIME_SPRAY);
    }

    void JustSummoned(Creature* summon) override
    {
        if (summon->GetEntry() == NPC_VILE_GAS_STALKER)
            if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
                professor->CastSpell(summon, SPELL_VILE_GAS_H, true);

        BossAI::JustSummoned(summon);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (summon->GetEntry() == NPC_OOZE_SPRAY_STALKER)
        {
            me->SetTarget(ObjectGuid::Empty);
            me->SetReactState(REACT_AGGRESSIVE);
            if (me->GetVictim())
                AttackStart(me->EnsureVictim());
        }

        BossAI::SummonedCreatureDespawn(summon);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_CALL_PROF:
                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
                    professor->AI()->DoAction(ACTION_ROTFACE_COMBAT);
                break;
            case EVENT_SLIME_SPRAY:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 200, true))
                {
                    if (Creature* stalker = DoSummon(NPC_OOZE_SPRAY_STALKER, *target, 6600, TEMPSUMMON_TIMED_DESPAWN))
                    {
                        me->SetReactState(REACT_PASSIVE);
                        me->AttackStop();
                        me->SetTarget(stalker->GetGUID());

                        Talk(EMOTE_SLIME_SPRAY);
                        me->CastSpell(stalker, SPELL_SLIME_SPRAY, false);
                    }
                }

                events.Repeat(20s, 30s);
                break;
            case EVENT_MUTATED_INFECTION:
                DoCastAOE(SPELL_MUTATED_INFECTION);
                infectionStage++;

                if (Is25ManRaid())
                {
                    if ((infectionStage <= 12 && infectionStage % 4 == 0) || infectionStage == 18)
                        infectionCooldown -= 2s;
                }
                else
                {
                    if (infectionStage == 8 || infectionStage == 12 || infectionStage == 16 || infectionStage == 22)
                        infectionCooldown -= 2s;
                }

                events.Repeat(infectionCooldown);
                break;
            case EVENT_VILE_GAS:
                DoCastAOE(SPELL_VILE_GAS_TRIGGER);
                events.Repeat(30s);
                break;
            case EVENT_ENRAGE:
                Talk(SAY_BERSERK);
                DoCastSelf(SPELL_BERSERK2, true);
                break;
            default:
                break;
        }
    }

private:
    Milliseconds infectionCooldown;
    uint32 infectionStage;
};

struct npc_little_oozeAI : public ScriptedAI
{
    npc_little_oozeAI(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(Unit* summoner) override
    {
        if (me->GetInstanceScript()->GetBossState(DATA_ROTFACE) != IN_PROGRESS)
        {
            me->DespawnOrUnsummon();
            return;
        }

        // register in Rotface's summons - not summoned with Rotface as owner
        if (Creature* rotface = ObjectAccessor::GetCreature(*me, me->GetInstanceScript()->GetGuidData(DATA_ROTFACE)))
            rotface->AI()->JustSummoned(me);

        DoCastSelf(SPELL_LITTLE_OOZE_COMBINE, true);
        DoCastSelf(SPELL_WEAK_RADIATING_OOZE, true);

        me->GetThreatManager().AddThreat(summoner, 500000.0f);

        _events.ScheduleEvent(EVENT_STICKY_OOZE, 5s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->DespawnOrUnsummon(12 * IN_MILLISECONDS);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);

        if (_events.ExecuteEvent() == EVENT_STICKY_OOZE)
        {
            DoCastVictim(SPELL_STICKY_OOZE);
            _events.Repeat(15s);
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap _events;
};

struct npc_big_oozeAI : public ScriptedAI
{
    npc_big_oozeAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript()) { }

    void Reset() override
    {
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
    }

    void IsSummonedBy(Unit* /*summoner*/) override
    {
        DoCastSelf(SPELL_LARGE_OOZE_COMBINE, true);
        DoCastSelf(SPELL_LARGE_OOZE_BUFF_COMBINE, true);
        DoCastSelf(SPELL_RADIATING_OOZE, true);
        DoCastSelf(SPELL_UNSTABLE_OOZE, true);
        DoCastSelf(SPELL_GREEN_ABOMINATION_HITTIN__YA_PROC, true);

        events.ScheduleEvent(EVENT_STICKY_OOZE, 5s);
        // register in Rotface's summons - not summoned with Rotface as owner
        if (Creature* rotface = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ROTFACE)))
            rotface->AI()->JustSummoned(me);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* rotface = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ROTFACE)))
            rotface->AI()->SummonedCreatureDespawn(me);
        me->DespawnOrUnsummon(12 * IN_MILLISECONDS);
    }

    void DoAction(int32 action) override
    {
        if (action == ACTION_ADD_MERGE)
        {
            if (Aura const* unstableAura = me->GetAura(SPELL_UNSTABLE_OOZE))
            {
                switch (uint32 count = unstableAura->GetStackAmount())
                {
                    case 2:
                        me->SetObjectScale(1.2f);
                        Talk(EMOTE_UNSTABLE_2, me);
                        DoCastSelf(SPELL_VERTEX_COLOR_PINK, true);
                        break;
                    case 3:
                        me->SetObjectScale(1.5f);
                        Talk(EMOTE_UNSTABLE_3, me);
                        DoCastSelf(SPELL_VERTEX_COLOR_BRIGHT_RED, true);
                        break;
                    case 4:
                        me->SetObjectScale(1.7f);
                        Talk(EMOTE_UNSTABLE_4, me);
                        DoCastSelf(SPELL_VERTEX_COLOR_DARK_RED, true);
                        break;
                    default:
                        if (count >= 5)
                        {
                            events.CancelEvent(EVENT_STICKY_OOZE);

                            me->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
                            me->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);

                            me->SetObjectScale(2.0f);
                            if (!me->HasAura(SPELL_VERTEX_COLOR_DARK_RED))
                                DoCastSelf(SPELL_VERTEX_COLOR_DARK_RED, true);

                            Talk(EMOTE_UNSTABLE_EXPLOSION);

                            if (Creature* rotface = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_ROTFACE)))
                                if (rotface->IsAlive())
                                    rotface->AI()->Talk(SAY_UNSTABLE_EXPLOSION);

                            me->CastSpell(me, SPELL_UNSTABLE_OOZE_EXPLOSION);

                            instance->SetData(DATA_OOZE_DANCE_ACHIEVEMENT, uint32(false));
                        }
                        break;
                }
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_STICKY_OOZE:
                    DoCastVictim(SPELL_STICKY_OOZE);
                    events.Repeat(15s);
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

private:
    EventMap events;
    InstanceScript* instance;
};

struct npc_precious_iccAI : public ScriptedAI
{
    npc_precious_iccAI(Creature* creature) : ScriptedAI(creature), _summons(me)
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
        _events.ScheduleEvent(EVENT_SUMMON_ZOMBIES, 20s, 22s);
        _events.ScheduleEvent(EVENT_DOUBLE_DAMAGE, 4s);
        _summons.DespawnAll();

        if (_instance->GetBossState(DATA_ROTFACE) == DONE)
            me->DespawnOrUnsummon();

        me->AddAura(SPELL_RIBBON, me);
    }

    void JustSummoned(Creature* summon) override
    {
        _summons.Summon(summon);
        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
            summon->AI()->AttackStart(target);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        _summons.Despawn(summon);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* rotface = ObjectAccessor::GetCreature(*me, _instance->GetGuidData(DATA_ROTFACE)))
            if (rotface->IsAlive())
                rotface->AI()->Talk(SAY_PRECIOUS_DIES);
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
                    _events.Repeat(20s, 25s);
                    break;
                case EVENT_MORTAL_WOUND:
                    DoCastVictim(SPELL_MORTAL_WOUND, true);
                    _events.Repeat(2s, 4s);
                    break;
                case EVENT_SUMMON_ZOMBIES:
                    Talk(EMOTE_PRECIOUS_ZOMBIES);
                    for (uint32 i = 0; i < 11; ++i)
                        DoCastSelf(SPELL_AWAKEN_PLAGUED_ZOMBIES);

                    _events.Repeat(20s, 22s);
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
    SummonList _summons;
    InstanceScript* _instance;
};

class spell_rotface_ooze_flood_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_ooze_flood_SpellScript);

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (!GetHitUnit())
            return;

        GetHitUnit()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), false, NULL, NULL, GetOriginalCaster() ? GetOriginalCaster()->GetGUID() : ObjectGuid::Empty);
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.clear();
        targets.push_back(GetCaster());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rotface_ooze_flood_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_ooze_flood_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
    }
};

class spell_rotface_mutated_infection_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_mutated_infection_SpellScript);

    bool Load() override
    {
        return GetCaster()->GetTypeId() == TYPEID_UNIT;
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.remove_if([&](WorldObject* target)
        {
            uint32 spellId = sSpellMgr->GetSpellIdForDifficulty(SPELL_MUTATED_INFECTION, GetCaster());
            return target->ToPlayer()->HasAura(spellId);
        });

        if (targets.empty())
            return;

        Trinity::Containers::RandomResize(targets, 1);
    }

    void NotifyTargets()
    {
        if (Creature* caster = GetCaster()->ToCreature())
            if (Unit* target = GetHitUnit())
                caster->AI()->Talk(EMOTE_MUTATED_INFECTION, target);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_mutated_infection_SpellScript::FilterTargets, EFFECT_ALL, TARGET_UNIT_SRC_AREA_ENEMY);
        AfterHit += SpellHitFn(spell_rotface_mutated_infection_SpellScript::NotifyTargets);
    }
};

class spell_rotface_mutated_infection_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_rotface_mutated_infection_AuraScript);

    void HandleEffectRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();

        if (GetCasterGUID() != ObjectGuid::Empty)
            target->CastSpell(target, GetSpellInfo()->Effects[EFFECT_2].CalcValue(), true, nullptr, aurEff, target->GetGUID());
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_rotface_mutated_infection_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

class spell_rotface_little_ooze_combine_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_little_ooze_combine_SpellScript);

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (!(GetHitCreature() && GetHitUnit()->IsAlive()))
            return;

        if (GetHitCreature()->HasUnitState(UNIT_STATE_CASTING))
            return;

        GetCaster()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
        GetHitCreature()->RemoveAurasDueToSpell(SPELL_LITTLE_OOZE_COMBINE);
        GetHitCreature()->CastSpell(GetCaster(), SPELL_OOZE_MERGE, true);
        GetHitCreature()->KillSelf();
        GetCaster()->KillSelf();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rotface_little_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_rotface_large_ooze_combine_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_large_ooze_combine_SpellScript);

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (!(GetHitCreature() && GetHitCreature()->IsAlive()))
            return;

        if (GetHitCreature()->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (GetHitCreature()->GetDisplayId() == DISPLAY_INVISIBLE || GetCaster()->GetDisplayId() == DISPLAY_INVISIBLE)
            return;

        if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
        {
            if (Aura* targetAura = GetHitCreature()->GetAura(SPELL_UNSTABLE_OOZE))
                unstable->ModStackAmount(targetAura->GetStackAmount());
            else
                unstable->ModStackAmount(1);

            if (Creature* creature = GetCaster()->ToCreature())
                creature->AI()->DoAction(ACTION_ADD_MERGE);
        }

        GetHitCreature()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_BUFF_COMBINE);
        GetHitCreature()->RemoveAurasDueToSpell(SPELL_LARGE_OOZE_COMBINE);
        GetHitCreature()->KillSelf();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rotface_large_ooze_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_rotface_large_ooze_buff_combine_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_large_ooze_buff_combine_SpellScript);

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (!(GetHitCreature() && GetHitCreature()->IsAlive()))
            return;

        if (GetHitCreature()->GetDisplayId() == DISPLAY_INVISIBLE || GetCaster()->GetDisplayId() == DISPLAY_INVISIBLE)
            return;

        if (Aura* unstable = GetCaster()->GetAura(SPELL_UNSTABLE_OOZE))
        {
            unstable->SetStackAmount(unstable->GetStackAmount() + 1);

            if (Creature* creature = GetCaster()->ToCreature())
                creature->AI()->DoAction(ACTION_ADD_MERGE);
        }

        GetHitCreature()->KillSelf();
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rotface_large_ooze_buff_combine_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_rotface_unstable_ooze_explosion_init_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_unstable_ooze_explosion_init_SpellScript);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        if (!sSpellMgr->GetSpellInfo(SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER))
            return false;
        return true;
    }

    void HandleCast(SpellEffIndex effIndex)
    {
        PreventHitEffect(effIndex);
        if (!GetHitUnit())
            return;

        float x, y, z;
        GetHitUnit()->GetPosition(x, y, z);
        Creature* dummy = GetCaster()->SummonCreature(NPC_UNSTABLE_EXPLOSION_STALKER, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000);
        GetCaster()->CastSpell(dummy, SPELL_UNSTABLE_OOZE_EXPLOSION_TRIGGER, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rotface_unstable_ooze_explosion_init_SpellScript::HandleCast, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
    }
};

class spell_rotface_unstable_ooze_explosion_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_unstable_ooze_explosion_SpellScript);

    void CheckTarget(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(EFFECT_0);
        if (!GetExplTargetDest())
            return;

        uint32 triggered_spell_id = GetSpellInfo()->Effects[effIndex].TriggerSpell;

        float x, y, z;
        GetExplTargetDest()->GetPosition(x, y, z);
        GetCaster()->CastSpell(x, y, z, triggered_spell_id, true);
    }

    void Register() override
    {
        OnEffectHit += SpellEffectFn(spell_rotface_unstable_ooze_explosion_SpellScript::CheckTarget, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
    }
};

class spell_rotface_unstable_ooze_explosion_suicide_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_rotface_unstable_ooze_explosion_suicide_AuraScript);

    void DespawnSelf(AuraEffect const* /*aurEff*/)
    {
        PreventDefaultAction();
        Unit* target = GetTarget();
        if (target->GetTypeId() != TYPEID_UNIT)
            return;

        // Remove every aura but SPELL_UNSTABLE_OOZE
        auto auras = target->GetAppliedAuras();
        for (auto&[id, aura] : auras)
            if (id != SPELL_UNSTABLE_OOZE)
                aura->GetBase()->Remove();

        target->SetDisplayId(DISPLAY_INVISIBLE);
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        target->ToCreature()->SetReactState(REACT_PASSIVE);
        target->CombatStop(false);
        target->ToCreature()->DespawnOrUnsummon(60 * IN_MILLISECONDS);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_rotface_unstable_ooze_explosion_suicide_AuraScript::DespawnSelf, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class spell_rotface_vile_gas_trigger_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_vile_gas_trigger_SpellScript);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        BossDistanceCheck(GetCaster(), targets);

        Trinity::Containers::RandomResize(targets, 1);
    }

    void HandleDummy(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
        GetCaster()->CastSpell(GetHitUnit(), GetSpellInfo()->Effects[EFFECT_0].CalcValue(), true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_rotface_vile_gas_trigger_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_rotface_vile_gas_trigger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

class spell_rotface_slime_spray_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_rotface_slime_spray_SpellScript);

    void ChangeOrientation()
    {
        Unit* caster = GetCaster();
        // find stalker and set caster orientation to face it
        if (Creature* target = caster->FindNearestCreature(NPC_OOZE_SPRAY_STALKER, 200.0f))
            caster->SetOrientation(caster->GetAngle(target));
    }

    void HandleResidue()
    {
        Player* target = GetHitPlayer();
        if (!target)
            return;

        if (target->HasAura(SPELL_GREEN_BLIGHT_RESIDUE))
            return;

        if (target->GetMap() && !target->GetMap()->Is25ManRaid())
        {
            if (target->GetQuestStatus(QUEST_RESIDUE_RENDEZVOUS_10) != QUEST_STATUS_INCOMPLETE)
                return;

            target->CastSpell(target, SPELL_GREEN_BLIGHT_RESIDUE, TRIGGERED_FULL_MASK);
        }

        if (target->GetMap() && target->GetMap()->Is25ManRaid())
        {
            if (target->GetQuestStatus(QUEST_RESIDUE_RENDEZVOUS_25) != QUEST_STATUS_INCOMPLETE)
                return;

            target->CastSpell(target, SPELL_GREEN_BLIGHT_RESIDUE, TRIGGERED_FULL_MASK);
        }
    }

    void Register() override
    {
        BeforeCast += SpellCastFn(spell_rotface_slime_spray_SpellScript::ChangeOrientation);
        OnHit += SpellHitFn(spell_rotface_slime_spray_SpellScript::HandleResidue);
    }
};

void AddSC_boss_rotface()
{
    new CreatureScriptLoaderEx<boss_rotfaceAI>("boss_rotface");
    new CreatureScriptLoaderEx<npc_little_oozeAI>("npc_little_ooze");
    new CreatureScriptLoaderEx<npc_big_oozeAI>("npc_big_ooze");
    new CreatureScriptLoaderEx<npc_precious_iccAI>("npc_precious_icc");
    new SpellScriptLoaderEx<spell_rotface_ooze_flood_SpellScript>("spell_rotface_ooze_flood");
    new SpellScriptLoaderEx<spell_rotface_mutated_infection_SpellScript, spell_rotface_mutated_infection_AuraScript>("spell_rotface_mutated_infection");
    new SpellScriptLoaderEx<spell_rotface_little_ooze_combine_SpellScript>("spell_rotface_little_ooze_combine");
    new SpellScriptLoaderEx<spell_rotface_large_ooze_combine_SpellScript>("spell_rotface_large_ooze_combine");
    new SpellScriptLoaderEx<spell_rotface_large_ooze_buff_combine_SpellScript>("spell_rotface_large_ooze_buff_combine");
    new SpellScriptLoaderEx<spell_rotface_unstable_ooze_explosion_init_SpellScript>("spell_rotface_unstable_ooze_explosion_init");
    new SpellScriptLoaderEx<spell_rotface_unstable_ooze_explosion_SpellScript>("spell_rotface_unstable_ooze_explosion");
    new SpellScriptLoaderEx<spell_rotface_unstable_ooze_explosion_suicide_AuraScript>("spell_rotface_unstable_ooze_explosion_suicide");
    new SpellScriptLoaderEx<spell_rotface_vile_gas_trigger_SpellScript>("spell_rotface_vile_gas_trigger");
    new SpellScriptLoaderEx<spell_rotface_slime_spray_SpellScript>("spell_rotface_slime_spray");
}
