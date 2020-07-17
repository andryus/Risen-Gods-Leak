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
#include "SpellInfo.h"
#include "gundrak.h"
#include "SpellScript.h"

enum Spells
{
    SPELL_DETERMINED_GORE       = 55102,
    SPELL_DETERMINED_STAB       = 55104,
    SPELL_GROUND_TREMOR         = 55142,
    SPELL_NUMBING_SHOUT         = 55106,
    SPELL_QUAKE                 = 55101,
    SPELL_NUMBING_ROAR          = 55100,
    SPELL_MOJO_FRENZY           = 55163,
    SPELL_MOJO_FRENCY_EFFECT    = 55096,
    SPELL_TRANSFORMATION        = 55098, // Periodic, The caster transforms into a powerful mammoth, increasing Physical damage done by 25% and granting immunity to Stun effects.
};

enum Says
{
    SAY_AGGRO                   = 0,
    SAY_SLAY                    = 1,
    SAY_DEATH                   = 2,
    SAY_TRANSFORM               = 3,
    SAY_QUAKE                   = 4,
    EMOTE_BEGIN_TRANSFORM       = 5,
    EMOTE_TRANSFORMED           = 6,
    EMOTE_ACTIVATE_ALTAR        = 7
};

enum Events
{
    EVENT_GROUND_TREMOR         = 1,
    EVENT_NUMBLING_SHOUT,
    EVENT_DETERMINED_STAB,
    EVENT_TRANFORMATION
};

enum Misc
{
    DATA_LESS_RABI              = 1
};

struct boss_moorabiAI : public BossAI
{
    boss_moorabiAI(Creature* creature) : BossAI(creature, DATA_MOORABI_EVENT) { }

    void Reset() override
    {
        _transformed = false;
        BossAI::Reset();
        instance->SetData(DATA_MOORABI_EVENT, NOT_STARTED);
    }

    void EnterCombat(Unit* who) override
    {
        BossAI::EnterCombat(who);
        Talk(SAY_AGGRO);
        DoCast(me, SPELL_MOJO_FRENZY, true);

        events.ScheduleEvent(EVENT_GROUND_TREMOR,   Seconds(18));
        events.ScheduleEvent(EVENT_NUMBLING_SHOUT,  Seconds(10));
        events.ScheduleEvent(EVENT_DETERMINED_STAB, Seconds(20));
        events.ScheduleEvent(EVENT_TRANFORMATION,   Seconds(12));

        instance->SetData(DATA_MOORABI_EVENT, IN_PROGRESS);
    }

    uint32 GetData(uint32 type) const override
    {
        if (type == DATA_LESS_RABI)
            return _transformed ? 0 : 1;
        return 0;
    }

    void KilledUnit(Unit* victim) override
    {
        BossAI::KilledUnit(victim);
        if (victim->GetTypeId() == TYPEID_PLAYER)
            Talk(SAY_SLAY);
    }

    void JustDied(Unit* killer) override
    {
        BossAI::JustDied(killer);
        Talk(SAY_DEATH);
        Talk(EMOTE_ACTIVATE_ALTAR);

        instance->SetData(DATA_MOORABI_EVENT, DONE);
    }

    void SpellHit(Unit* /*caster*/, SpellInfo const* spellInfo) override
    {
        if (spellInfo->Id == SPELL_TRANSFORMATION)
        {
            _transformed = true;
            Talk(EMOTE_TRANSFORMED);
            events.CancelEvent(EVENT_TRANFORMATION);
            me->RemoveAurasDueToSpell(SPELL_MOJO_FRENZY);
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_GROUND_TREMOR:
                Talk(SAY_QUAKE);
                DoCastAOE(_transformed ? SPELL_QUAKE : SPELL_GROUND_TREMOR);
                events.ScheduleEvent(eventId, Seconds(10));
                break;
            case EVENT_NUMBLING_SHOUT:
                DoCastAOE(_transformed ? SPELL_NUMBING_ROAR : SPELL_NUMBING_SHOUT);
                events.ScheduleEvent(eventId, Seconds(10));
                break;
            case EVENT_DETERMINED_STAB:
                DoCastAOE(_transformed ? SPELL_DETERMINED_GORE : SPELL_DETERMINED_STAB);
                events.ScheduleEvent(eventId, Seconds(8));
                break;
            case EVENT_TRANFORMATION:
                Talk(EMOTE_BEGIN_TRANSFORM);
                Talk(SAY_TRANSFORM);
                DoCastSelf(SPELL_TRANSFORMATION);
                events.ScheduleEvent(eventId, Seconds(10));
                break;
            default:
                break;
        }
    }

private:
    bool _transformed;
};

class achievement_less_rabi : public AchievementCriteriaScript
{
    public:
        achievement_less_rabi() : AchievementCriteriaScript("achievement_less_rabi") { }

        bool OnCheck(Player* /*player*/, Unit* target) override
        {
            if (!target)
                return false;

            if (Creature* Moorabi = target->ToCreature())
                if (Moorabi->AI()->GetData(DATA_LESS_RABI))
                    return true;

            return false;
        }
};

class spell_moorabi_mojo_frency_AuraScript : public AuraScript
{
    PrepareAuraScript(spell_moorabi_mojo_frency_AuraScript);

    void CalcCastTime(AuraEffect const* /*aurEff*/)
    {
        int32 pct = 100 - int32(GetUnitOwner()->GetHealthPct());
        pct *= 3;
        GetUnitOwner()->CastCustomSpell(SPELL_MOJO_FRENCY_EFFECT, SPELLVALUE_BASE_POINT0, pct, GetUnitOwner(), TRIGGERED_FULL_MASK);
    }

    void Register()
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_moorabi_mojo_frency_AuraScript::CalcCastTime, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

void AddSC_boss_moorabi()
{
    new CreatureScriptLoaderEx<boss_moorabiAI>("boss_moorabi");
    new achievement_less_rabi();
    new SpellScriptLoaderEx<spell_moorabi_mojo_frency_AuraScript>("spell_moorabi_mojo_frency");
}
