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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellScript.h"
#include "ahnkahet.h"
#include "DBCStores.h"

enum Yells
{
    SAY_AGGRO       = 0,
    SAY_SLAY        = 1,
    SAY_DEATH       = 2,
    SAY_EGG_SAC     = 3,
    EMOTE_HATCHES   = 4
};

enum Spells
{
    // Elder Nadox
    SPELL_BROOD_PLAGUE          = 56130,
    H_SPELL_BROOD_RAGE          = 59465,
    SPELL_ENRAGE                = 26662, // Enraged if too far away from home
    SPELL_SUMMON_SWARMERS       = 56119, // 2x 30178  -- 2x every 10secs
    SPELL_SUMMON_SWARM_GUARD    = 56120, // 1x 30176

    // Adds
    SPELL_SWARM_BUFF            = 56281,
    SPELL_SPRINT                = 56354
};

enum Events
{
    EVENT_PLAGUE = 1,
    EVENT_RAGE,
    EVENT_SUMMON_SWARMER,
    EVENT_CHECK_ENRAGE,
    EVENT_SPRINT,
    DATA_RESPECT_YOUR_ELDERS
};

enum Creatures
{
    MOB_AHNKAHAR_SWARMER           = 30178,
    MOB_AHNKAHAR_GUARDIAN_ENTRY    = 30176,
    MOB_AHNKAHAR_SWARMER_EGG       = 30172,
    MOB_AHNKAHAR_GUARDIAN_EGG      = 30173
};

#define ACTION_AHNKAHAR_GUARDIAN_DEAD             1
#define DATA_RESPECT_YOUR_ELDERS                  2
#define ACHIEVEMENT_RESPECT_YOUR_ELDERS           2038

class boss_elder_nadox : public CreatureScript
{
    public:
        boss_elder_nadox() : CreatureScript("boss_elder_nadox") { }

        struct boss_elder_nadoxAI : public BossAI
        {
            boss_elder_nadoxAI(Creature* creature) : BossAI(creature, DATA_ELDER_NADOX) { }

            void Reset() override
            {
                BossAI::Reset();
                GuardianSummoned = false;
                GuardianDied = false;
                respectYourElders = false;
            }

            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_PLAGUE, 13 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SUMMON_SWARMER, 10 * IN_MILLISECONDS);

                if (IsHeroic())
                {
                    events.ScheduleEvent(EVENT_RAGE, 12 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_CHECK_ENRAGE, 5 * IN_MILLISECONDS);
                }
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
            {
                if (summon->GetEntry() == MOB_AHNKAHAR_GUARDIAN_ENTRY)
                    GuardianDied = true;
            }

            void DoAction(int32 action) override
            {
                if (action == ACTION_AHNKAHAR_GUARDIAN_DEAD)
                    respectYourElders = false;
            }

            uint32 GetData(uint32 type) const override
            {
                if (type == DATA_RESPECT_YOUR_ELDERS)
                    return !GuardianDied ? 1 : 0;

                return 0;
            }

            void KilledUnit(Unit* who) override
            {
                if (who->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);
                Talk(SAY_DEATH);

                if (IsHeroic() && respectYourElders)
                {
                    if (AchievementEntry const *AchievRespectYourElders = sAchievementStore.LookupEntry(ACHIEVEMENT_RESPECT_YOUR_ELDERS))
                    {
                        Map* map = me->GetMap();
                        if (map && map->IsDungeon())
                        {
                            for (Player& player : map->GetPlayers())
                                player.CompletedAchievement(AchievRespectYourElders);
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
                        case EVENT_PLAGUE:
                            DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true), SPELL_BROOD_PLAGUE, true);
                            events.ScheduleEvent(EVENT_PLAGUE, 15 * IN_MILLISECONDS);
                            break;
                        case EVENT_RAGE:
                            DoCast(H_SPELL_BROOD_RAGE);
                            events.ScheduleEvent(EVENT_RAGE, urand(10 * IN_MILLISECONDS, 50 * IN_MILLISECONDS));
                            break;
                        case EVENT_SUMMON_SWARMER:
                            if (Creature* Egg = me->FindNearestCreature(MOB_AHNKAHAR_SWARMER_EGG, 300.0f, true))
                            {
                                me->SummonCreature(MOB_AHNKAHAR_SWARMER, Egg->GetPositionX(), Egg->GetPositionY(), Egg->GetPositionZ(), Egg->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
                                me->SummonCreature(MOB_AHNKAHAR_SWARMER, Egg->GetPositionX(), Egg->GetPositionY(), Egg->GetPositionZ(), Egg->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
                                Egg->KillSelf();
                            }
                            if (urand(1, 3) == 3) // 33% chance of dialog
                                Talk(SAY_EGG_SAC);
                            events.ScheduleEvent(EVENT_SUMMON_SWARMER, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_CHECK_ENRAGE:
                            if (me->HasAura(SPELL_ENRAGE))
                                return;
                            if (me->GetPositionZ() < 24.0f)
                                DoCastSelf(SPELL_ENRAGE, true);
                            events.ScheduleEvent(EVENT_CHECK_ENRAGE, 5 * IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                if (!GuardianSummoned && me->HealthBelowPct(50))
                {
                    Talk(EMOTE_HATCHES, me);
                    if (Creature* Egg = me->FindNearestCreature(MOB_AHNKAHAR_GUARDIAN_EGG, 300.0f, true))
                    {
                        me->SummonCreature(MOB_AHNKAHAR_GUARDIAN_ENTRY, Egg->GetPositionX(), Egg->GetPositionY(), Egg->GetPositionZ(), Egg->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
                        Egg->KillSelf();
                    }
                    GuardianSummoned = true;
                }

                DoMeleeAttackIfReady();
            }

        private:
            bool GuardianSummoned;
            bool GuardianDied;
            bool respectYourElders;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_elder_nadoxAI(creature);
        }
};

class npc_ahnkahar_nerubian : public CreatureScript
{
    public:
        npc_ahnkahar_nerubian() : CreatureScript("npc_ahnkahar_nerubian") { }

        struct npc_ahnkahar_nerubianAI : public ScriptedAI
        {
            npc_ahnkahar_nerubianAI(Creature* creature) : ScriptedAI(creature) 
            {
                instance = me->GetInstanceScript();
            }

            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_SPRINT, 13 * IN_MILLISECONDS);
            }

            void JustDied(Unit* /*who*/) override
            {
                if (me->GetEntry() == MOB_AHNKAHAR_GUARDIAN_ENTRY)
                    if (Creature* Nadox = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_ELDER_NADOX))))
                        Nadox->AI()->DoAction(ACTION_AHNKAHAR_GUARDIAN_DEAD);
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
                        case EVENT_SPRINT:
                            DoCastSelf(SPELL_SPRINT);
                            _events.ScheduleEvent(EVENT_SPRINT, 20 * IN_MILLISECONDS);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_ahnkahar_nerubianAI(creature);
        }
};

// 56159 - Swarm
class spell_ahn_kahet_swarm : public SpellScriptLoader
{
    public:
        spell_ahn_kahet_swarm() : SpellScriptLoader("spell_ahn_kahet_swarm") { }

        class spell_ahn_kahet_swarm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ahn_kahet_swarm_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SWARM_BUFF))
                    return false;
                return true;
            }

            bool Load() override
            {
                _targetCount = 0;
                return true;
            }

            void CountTargets(std::list<WorldObject*>& targets)
            {
                _targetCount = targets.size();
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (_targetCount)
                {
                    if (Aura* aura = GetCaster()->GetAura(SPELL_SWARM_BUFF))
                    {
                        aura->SetStackAmount(_targetCount);
                        aura->RefreshDuration();
                    }
                    else
                        GetCaster()->CastCustomSpell(SPELL_SWARM_BUFF, SPELLVALUE_AURA_STACK, _targetCount, GetCaster(), TRIGGERED_FULL_MASK);
                }
                else
                    GetCaster()->RemoveAurasDueToSpell(SPELL_SWARM_BUFF);
            }

            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ahn_kahet_swarm_SpellScript::CountTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
                OnEffectHit += SpellEffectFn(spell_ahn_kahet_swarm_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }

        private:
            uint32 _targetCount;
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_ahn_kahet_swarm_SpellScript();
        }
};

class achievement_respect_your_elders : public AchievementCriteriaScript
{
    public:
        achievement_respect_your_elders() : AchievementCriteriaScript("achievement_respect_your_elders") { }

        bool OnCheck(Player* /*player*/, Unit* target) override
        {
            return target && target->GetAI()->GetData(DATA_RESPECT_YOUR_ELDERS);
        }
};

void AddSC_boss_elder_nadox()
{
    new boss_elder_nadox();
    new npc_ahnkahar_nerubian();
    new spell_ahn_kahet_swarm();
    new achievement_respect_your_elders();
}
