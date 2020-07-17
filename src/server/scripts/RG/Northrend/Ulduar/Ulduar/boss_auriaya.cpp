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
#include "SpellScript.h"
#include "PassiveAI.h"
#include "ulduar.h"

// ###### Related Creatures/Object ######
enum Creatures
{
    NPC_FERAL_DEFENDER                           = 34035,
    NPC_FERAL_DEFENDER_TRIGGER                   = 34096,
    NPC_SEEPING_TRIGGER                          = 34098,
    NPC_SWARMING_GUARDIAN                        = 34034,
};

// ###### Texts ######
enum AuriayaYells
{
    // Yells
    SAY_AGGRO                                    = 0,
    SAY_SLAY                                     = 1,
    SAY_DEATH                                    = 2,
    SAY_BERSERK                                  = 3,
    // Emotes
    EMOTE_FEAR                                   = 4,
    EMOTE_DEFENDER                               = 5
};

// ###### Datas ######
enum Actions
{
    ACTION_CRAZY_CAT_LADY                        = 1,
    ACTION_RESPAWN_DEFENDER                      = 2,
};

enum Datas
{
    DATA_CRAZY_CAT_LADY                          = 30063007,
};

enum Achievements
{
    ACHIEV_NINE_LIVES_10                         = 3076,
    ACHIEV_NINE_LIVES_25                         = 3077,
};

// ###### Event Controlling ######
enum AuriayaEvents
{
    // Auriaya
    EVENT_SCREECH                                = 1,
    EVENT_BLAST                                  = 2,
    EVENT_TERRIFYING                             = 3,
    EVENT_SUMMON                                 = 4,
    EVENT_DEFENDER                               = 5,
    EVENT_ACTIVATE_DEFENDER                      = 6,
    EVENT_RESPAWN_DEFENDER                       = 7,
    EVENT_BERSERK                                = 8,

    // Sanctum Sentry
    EVENT_RIP                                    = 9,
    EVENT_POUNCE                                 = 10,

    // Feral Defender
    EVENT_FERAL_POUNCE                           = 11,
    EVENT_RUSH                                   = 12,
};

// ###### Spells ######
enum AuriayaSpells
{
    // Auriaya
    SPELL_SENTINEL_BLAST                         = 64389,
    SPELL_SONIC_SCREECH                          = 64422,
    SPELL_TERRIFYING_SCREECH                     = 64386,
    SPELL_SUMMON_SWARMING_GUARDIAN               = 64396,
    SPELL_SWARMING_GUARDIAN_FOCUS                = 65029,
    SPELL_ACTIVATE_DEFENDER                      = 64449,
    SPELL_DEFENDER_TRIGGER                       = 64448,
    SPELL_SUMMON_DEFENDER                        = 64447,
    SPELL_BERSERK                                = 47008,

    // Feral Defender
    SPELL_FERAL_RUSH                             = 64496,
    SPELL_FERAL_POUNCE                           = 64478,
    SPELL_SEEPING_ESSENCE                        = 64458,
    SPELL_SUMMON_ESSENCE                         = 64457,
    SPELL_FERAL_ESSENCE                          = 64455,

    // Sanctum Sentry
    SPELL_SAVAGE_POUNCE                          = 64374,
    SPELL_RIP_FLESH                              = 64375,
};

class boss_auriaya : public CreatureScript
{
    public:
        boss_auriaya() : CreatureScript("boss_auriaya") { }

        struct boss_auriayaAI : public BossAI
        {
            boss_auriayaAI(Creature* creature) : BossAI(creature, BOSS_AURIAYA)
            {
                ASSERT(instance);
            }

            void Reset()
            {
                StopCombat();
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);
                Talk(SAY_AGGRO);

                DefenderGUID.Clear();
                defenderLives = 9;
                AchievCrazyCatLady = true;
                AchievNineLives = false;

                events.RescheduleEvent(EVENT_SCREECH,    urand(45, 65) * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_TERRIFYING, urand(20, 30) * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_BLAST, urand(20, 25) * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_DEFENDER, urand(40, 55) * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_SUMMON, urand(45, 55) * IN_MILLISECONDS);
                events.RescheduleEvent(EVENT_BERSERK, 600 * IN_MILLISECONDS);
            }

            void SpellHit(Unit* target, SpellInfo const* /*spell*/)
            {
                // Arachnopod Destroyer - prevent buguse ;)
                if (target->GetEntry() == 34183)
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                        me->Kill(&player, true);
                }
            }

            void StopCombat()
            {
                for (Player& player : me->GetMap()->GetPlayers())
                    player.CombatStop();
            }

            void KilledUnit(Unit* /*victim*/)
            {
                Talk(SAY_SLAY);
            }

            void JustSummoned(Creature* summon)
            {
                BossAI::JustSummoned(summon);

                switch (summon->GetEntry())
                {
                    case NPC_SWARMING_GUARDIAN:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0, true, true, SPELL_SWARMING_GUARDIAN_FOCUS))
                        {
                            summon->SetReactState(REACT_PASSIVE);
                            summon->AI()->AttackStart(target);
                        }
                        break;
                    case NPC_FERAL_DEFENDER:
                        summon->AI()->AttackStart(me->GetVictim());
                        summon->SetAuraStack(SPELL_FERAL_ESSENCE, summon, 9);
                        DefenderGUID = summon->GetGUID();
                        break;
                }
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_CRAZY_CAT_LADY:
                        AchievCrazyCatLady = false;
                        break;
                    case ACTION_RESPAWN_DEFENDER:
                        if (--defenderLives)
                            events.ScheduleEvent(EVENT_RESPAWN_DEFENDER, 30000);
                        else
                            AchievNineLives = true;
                        break;
                    default:
                        break;
                }
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_CRAZY_CAT_LADY:
                        return AchievCrazyCatLady ?  1 : 0;
                }

                return 0;
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);
                StopCombat();

                Talk(SAY_DEATH);

                if (AchievNineLives)
                    me->GetInstanceScript()->DoCompleteAchievement(RAID_MODE(ACHIEV_NINE_LIVES_10, ACHIEV_NINE_LIVES_25));

                instance->SetBossState(BOSS_AURIAYA, DONE);
            }

            void UpdateAI(uint32 diff)
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
                        case EVENT_SCREECH:
                            DoCast(SPELL_SONIC_SCREECH);
                            events.ScheduleEvent(EVENT_SCREECH, urand(40, 60) * IN_MILLISECONDS);
                            break;
                        case EVENT_TERRIFYING:
                            Talk(EMOTE_FEAR);
                            DoCast(SPELL_TERRIFYING_SCREECH);
                            events.ScheduleEvent(EVENT_TERRIFYING, urand(20, 30) * IN_MILLISECONDS);
                            break;
                        case EVENT_BLAST:
                            DoCastAOE(SPELL_SENTINEL_BLAST);
                            events.ScheduleEvent(EVENT_BLAST, urand(25, 35) * IN_MILLISECONDS);
                            break;
                        case EVENT_DEFENDER:
                            if (me->HasUnitState(UNIT_STATE_CASTING)) //to prevent never summon defender while casting
                                events.RescheduleEvent(EVENT_DEFENDER, 3 * IN_MILLISECONDS);
                            else
                            {
                                Talk(EMOTE_DEFENDER);
                                DoCast(SPELL_DEFENDER_TRIGGER);
                                if (Creature* trigger = me->FindNearestCreature(NPC_FERAL_DEFENDER_TRIGGER, 30.0f, true))
                                    DoCast(trigger, SPELL_ACTIVATE_DEFENDER, true);
                            }
                            break;
                        case EVENT_RESPAWN_DEFENDER:
                            if (Creature* defender = ObjectAccessor::GetCreature(*me, DefenderGUID))
                            {
                                defender->Respawn();
                                if (defenderLives)
                                    defender->SetAuraStack(SPELL_FERAL_ESSENCE, defender, defenderLives);
                                DoZoneInCombat(defender);
                                defender->AI()->AttackStart(me->GetVictim());
                            }
                            break;
                        case EVENT_SUMMON:
                            DoCast(SPELL_SUMMON_SWARMING_GUARDIAN);
                            events.ScheduleEvent(EVENT_SUMMON, urand(30, 45) * IN_MILLISECONDS);
                            break;
                        case EVENT_BERSERK:
                            DoCastSelf(SPELL_BERSERK, true);
                            Talk(SAY_BERSERK);
                            events.CancelEvent(EVENT_BERSERK);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            ObjectGuid DefenderGUID;
            uint8 defenderLives;
            bool AchievCrazyCatLady;
            bool AchievNineLives;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_auriayaAI(creature);
        }
};

class npc_auriaya_seeping_trigger : public CreatureScript
{
    public:
        npc_auriaya_seeping_trigger() : CreatureScript("npc_auriaya_seeping_trigger") { }

        struct npc_auriaya_seeping_triggerAI : public NullCreatureAI
        {
            npc_auriaya_seeping_triggerAI(Creature* creature) : NullCreatureAI(creature)
            {
                me->DespawnOrUnsummon(600*IN_MILLISECONDS);
                DoCastSelf(SPELL_SEEPING_ESSENCE);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_auriaya_seeping_triggerAI(creature);
        }
};

class npc_sanctum_sentry : public CreatureScript
{
    public:
        npc_sanctum_sentry() : CreatureScript("npc_sanctum_sentry") { }

        struct npc_sanctum_sentryAI : public ScriptedAI
        {
            npc_sanctum_sentryAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
            }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                if (!Instance->CheckRequiredBosses(BOSS_AURIAYA))
                {
                    EnterEvadeMode();
                    return;
                }

                events.ScheduleEvent(EVENT_RIP,    urand( 4*IN_MILLISECONDS, 8*IN_MILLISECONDS));
                events.ScheduleEvent(EVENT_POUNCE, urand(10*IN_MILLISECONDS,12*IN_MILLISECONDS));
            }

            void UpdateAI(uint32 diff)
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
                        case EVENT_RIP:
                            DoCastVictim(SPELL_RIP_FLESH);
                            events.ScheduleEvent(EVENT_RIP, urand(12*IN_MILLISECONDS, 15*IN_MILLISECONDS));
                            break;
                        case EVENT_POUNCE:
                            DoCast(SPELL_SAVAGE_POUNCE);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*who*/)
            {
                if (Creature* Auriaya = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_AURIAYA))))
                    Auriaya->AI()->DoAction(ACTION_CRAZY_CAT_LADY);
            }

        private:
            InstanceScript* Instance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_sanctum_sentryAI(creature);
        }
};

class npc_feral_defender : public CreatureScript
{
    public:
        npc_feral_defender() : CreatureScript("npc_feral_defender") { }

        struct npc_feral_defenderAI : public ScriptedAI
        {
            npc_feral_defenderAI(Creature* creature) : ScriptedAI(creature), Instance(me->GetInstanceScript())
            {
                ASSERT(Instance);
            }

            void Reset()
            {
                events.Reset();
            }

            void EnterCombat(Unit* /*who*/)
            {
                events.ScheduleEvent(EVENT_FERAL_POUNCE,  5*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_RUSH,         10*IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff)
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
                        case EVENT_FERAL_POUNCE:
                            DoCast(SPELL_FERAL_POUNCE);
                            events.ScheduleEvent(EVENT_FERAL_POUNCE, urand(10*IN_MILLISECONDS, 12*IN_MILLISECONDS));
                            break;
                        case EVENT_RUSH:
                            DoCast(SPELL_FERAL_RUSH);
                            events.ScheduleEvent(EVENT_RUSH, urand(10*IN_MILLISECONDS, 12*IN_MILLISECONDS));
                            break;
                    }
                }

                if (!me->HasUnitState(UNIT_STATE_JUMPING))
                    DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*who*/)
            {
                DoCastSelf(SPELL_SUMMON_ESSENCE);
                if (Creature* Auriaya = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_AURIAYA))))
                    Auriaya->AI()->DoAction(ACTION_RESPAWN_DEFENDER);
            }

            void JustSummoned(Creature* summon)
            {
                if (Creature* Auriaya = ObjectAccessor::GetCreature(*me, ObjectGuid(Instance->GetGuidData(BOSS_AURIAYA))))
                    Auriaya->AI()->JustSummoned(summon);
            }

        private:
            InstanceScript* Instance;
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_feral_defenderAI(creature);
        }
};

class SanctumSentryCheck
{
   public:
      SanctumSentryCheck(Unit* unit) : me(unit) {}

    bool operator() (WorldObject* object)
    {
        if (object->ToUnit()->GetEntry() == NPC_SANCTUM_SENTRY && object != me)
            return false;

        return true;
    }

    private:
        Unit* me;
};

class spell_auriaya_strenght_of_the_pack : public SpellScriptLoader
{
    public:
        spell_auriaya_strenght_of_the_pack() : SpellScriptLoader("spell_auriaya_strenght_of_the_pack") { }

        class spell_auriaya_strenght_of_the_pack_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_auriaya_strenght_of_the_pack_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(SanctumSentryCheck(GetCaster()));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_auriaya_strenght_of_the_pack_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_auriaya_strenght_of_the_pack_SpellScript();
        }
};

class spell_auriaya_sentinel_blast : public SpellScriptLoader
{
    public:
        spell_auriaya_sentinel_blast() : SpellScriptLoader("spell_auriaya_sentinel_blast") { }

        class spell_auriaya_sentinel_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_auriaya_sentinel_blast_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(PlayerOrPetCheck());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_auriaya_sentinel_blast_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_auriaya_sentinel_blast_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_auriaya_sentinel_blast_SpellScript();
        }
};

class achievement_crazy_cat_lady : public AchievementCriteriaScript
{
    public:
        achievement_crazy_cat_lady() : AchievementCriteriaScript("achievement_crazy_cat_lady") {}

        bool OnCheck(Player* /*player*/, Unit* target)
        {
            if (!target)
                return false;

            if (Creature* Auriaya = target->ToCreature())
                if (Auriaya->AI()->GetData(DATA_CRAZY_CAT_LADY))
                    return true;

            return false;
        }
};

void AddSC_boss_auriaya()
{
    new boss_auriaya();

    new npc_auriaya_seeping_trigger();
    new npc_feral_defender();
    new npc_sanctum_sentry();

    new spell_auriaya_strenght_of_the_pack();
    new spell_auriaya_sentinel_blast();

    new achievement_crazy_cat_lady();
}
