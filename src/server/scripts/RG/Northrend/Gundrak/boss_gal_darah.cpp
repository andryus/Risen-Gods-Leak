/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2018 Rising Gods <http://www.rising-gods.de/>
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
#include "gundrak.h"
#include "SpellScript.h"

enum Spells
{
    SPELL_ENRAGE                                  = 55285,
    SPELL_IMPALING_CHARGE                         = 54956,
    SPELL_IMPALING_CHARGE_VISUAL                  = 54958,
    SPELL_STOMP                                   = 55292,
    SPELL_PUNCTURE                                = 55276,
    SPELL_STAMPEDE                                = 55218,
    SPELL_WHIRLING_SLASH                          = 55250,
    SPELL_STAMPEDE_CHARGE                         = 55220,
    SPELL_CLEAR_PUNCTURE                          = 60022,
    SPELL_HEARTH_BEAM_VISUAL                      = 54988,
    SPELL_TRANSFORM_RHINO                         = 55297,
    SPELL_TRANSFORM_BACK                          = 55299,
};

enum Yells
{
    SAY_AGGRO                                     = 0,
    SAY_SLAY                                      = 1,
    SAY_DEATH                                     = 2,
    SAY_SUMMON_RHINO                              = 3,
    SAY_TRANSFORM_1                               = 4,  //Phase change
    SAY_TRANSFORM_2                               = 5,  //Phase change
};

enum CombatPhase
{
    PHASE_TROLL = 1,
    PHASE_RHINO = 2
};

enum Misc
{
    NPC_RHINO_SPIRT                               = 29791,

    DATA_SHARE_THE_LOVE                           = 1,
};

enum Events
{
    EVENT_STAMPEDE = 1,
    EVENT_WHIRLING_SLASH,
    EVENT_PUNCTURE,
    EVENT_ENRAGE,
    EVENT_IMPALING_CHARGE,
    EVENT_STOMP,
    EVENT_TRANSFORMATION,
};

struct boss_gal_darahAI : public BossAI
{
    boss_gal_darahAI(Creature* creature) : BossAI(creature, DATA_GAL_DARAH_EVENT) { }

    void Reset() override
    {
        BossAI::Reset();

        phaseCounter = 0;

        impaledList.clear();
        stampedeTargetGUID.Clear();

        DoCastAOE(SPELL_HEARTH_BEAM_VISUAL, true);

        instance->SetData(DATA_GAL_DARAH_EVENT, NOT_STARTED);
    }

    void JustReachedHome() override
    {
        BossAI::JustReachedHome();
        DoCastAOE(SPELL_HEARTH_BEAM_VISUAL, true);
    }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);
        Talk(SAY_AGGRO);

        instance->SetData(DATA_GAL_DARAH_EVENT, IN_PROGRESS);

        SetPhase(PHASE_TROLL);

        me->CastStop();
    }

    void SetPhase(CombatPhase phase)
    {
        events.Reset();
        events.SetPhase(phase);
        switch (phase)
        {
            case PHASE_TROLL:
                events.ScheduleEvent(EVENT_STAMPEDE, 10s, 0, PHASE_TROLL);
                events.ScheduleEvent(EVENT_WHIRLING_SLASH, 21s, 0, PHASE_TROLL);
                break;
            case PHASE_RHINO:
                events.ScheduleEvent(EVENT_PUNCTURE, 10s, 0, PHASE_RHINO);
                events.ScheduleEvent(EVENT_ENRAGE, 15s, 0, PHASE_RHINO);
                events.ScheduleEvent(EVENT_IMPALING_CHARGE, 21s, 0, PHASE_RHINO);
                events.ScheduleEvent(EVENT_STOMP, 25s, 0, PHASE_RHINO);
                break;
            default:
                break;
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_TRANSFORMATION:
                if (events.IsInPhase(PHASE_TROLL))
                {
                    Talk(SAY_TRANSFORM_1);
                    DoCastSelf(SPELL_TRANSFORM_RHINO);
                    SetPhase(PHASE_RHINO);
                }
                else if (events.IsInPhase(PHASE_RHINO))
                {
                    Talk(SAY_TRANSFORM_2);
                    DoCastSelf(SPELL_TRANSFORM_BACK);
                    SetPhase(PHASE_TROLL);
                }
                phaseCounter = 0;
                break;
            // combat phase Troll
            case EVENT_STAMPEDE:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true))
                {
                    DoCast(target, SPELL_STAMPEDE);
                    stampedeTargetGUID = target->GetGUID();
                }
                Talk(SAY_SUMMON_RHINO);
                events.Repeat(15s);
                break;
            case EVENT_WHIRLING_SLASH:
                DoCastVictim(SPELL_WHIRLING_SLASH);
                if (++phaseCounter >= 2)
                    events.ScheduleEvent(EVENT_TRANSFORMATION, 5s);
                events.Repeat(21s);
                break;
            // combat phase Rhino
            case EVENT_PUNCTURE:
                DoCastVictim(SPELL_PUNCTURE);
                events.Repeat(8s);
                break;
            case EVENT_ENRAGE:
                DoCastSelf(SPELL_ENRAGE);
                events.Repeat(20s);
                break;
            case EVENT_IMPALING_CHARGE:
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, false))
                {
                    DoCast(target, SPELL_IMPALING_CHARGE);
                    target->CastSpell(me, SPELL_IMPALING_CHARGE_VISUAL, true);
                    impaledList.insert(target->GetGUID());
                }

                if (++phaseCounter >= 2)
                    events.ScheduleEvent(EVENT_TRANSFORMATION, 5s);
                events.Repeat(31s);
                break;
            case EVENT_STOMP:
                DoCastVictim(SPELL_STOMP);
                events.Repeat(20s);
                break;
            default:
                break;
        }
    }

    void SpellHit(Unit* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_TRANSFORM_BACK)
            me->RemoveAurasDueToSpell(SPELL_TRANSFORM_RHINO);
    }

    void JustSummoned(Creature* summon) override
    {
        BossAI::JustSummoned(summon);
        if (summon->GetEntry() != NPC_RHINO_SPIRT)
            return;

        Unit* target = ObjectAccessor::GetUnit(*me, stampedeTargetGUID);
        if (!target)
            target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true);
        if (target)
            summon->CastSpell(target, SPELL_STAMPEDE_CHARGE, false);

        stampedeTargetGUID.Clear();
    }

    uint32 GetData(uint32 type) const
    {
        if (type == DATA_SHARE_THE_LOVE)
            return impaledList.size();

        return 0;
    }

    void JustDied(Unit* killer) override
    {
        BossAI::JustDied(killer);

        Talk(SAY_DEATH);

        if (instance)
            instance->SetData(DATA_GAL_DARAH_EVENT, DONE);

        DoCastAOE(SPELL_CLEAR_PUNCTURE, true);
    }

    void KilledUnit(Unit* victim) override
    {
        BossAI::KilledUnit(victim);

        if (victim && victim->GetTypeId() != TYPEID_PLAYER)
            return;

        Talk(SAY_SLAY);
    }

    private:
        std::unordered_set<ObjectGuid>  impaledList;
        ObjectGuid                      stampedeTargetGUID;
        uint8                           phaseCounter;
};

struct npc_rhino_spiritAI : public ScriptedAI
{
    npc_rhino_spiritAI(Creature* creature) : ScriptedAI(creature) { }

    void MovementInform(uint32 type, uint32 /*id*/)
    {
        if (type == POINT_MOTION_TYPE)
            me->DespawnOrUnsummon(500);
    }
};

class achievement_share_the_love : public AchievementCriteriaScript
{
    public:
        achievement_share_the_love() : AchievementCriteriaScript("achievement_share_the_love") { }

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* GalDarah = target->ToCreature())
                if (GalDarah->AI()->GetData(DATA_SHARE_THE_LOVE) >= 5)
                    return true;

            return false;
        }
};

class spell_gal_darah_clear_puncture_SpellScript : public SpellScript
{
    PrepareSpellScript(spell_gal_darah_clear_puncture_SpellScript);
            
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        if (!sSpellMgr->GetSpellInfo(SPELL_PUNCTURE))
            return false;
        return true;
    }
            
    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        if (Unit* target = GetHitUnit())
            target->RemoveAura(SPELL_PUNCTURE);              
    }
 
    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_gal_darah_clear_puncture_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

void AddSC_boss_gal_darah()
{
    new CreatureScriptLoaderEx<boss_gal_darahAI>("boss_gal_darah");
    new CreatureScriptLoaderEx<npc_rhino_spiritAI>("npc_rhino_spirit");
    new SpellScriptLoaderEx<spell_gal_darah_clear_puncture_SpellScript>("spell_gal_darah_clear_puncture");
    new achievement_share_the_love();
}
