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

#include "ulduar.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellInfo.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "DBCStores.h"

// ###### Related Creatures/Object ######
enum NPCs
{
    NPC_GENERAL_VEZAX                         = 33271,
    NPC_SARONIT_VAPOR                         = 33488,
    NPC_SARONIT_ANIMUS                        = 33524,
};

// ###### Texts ######
enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SLAY                                    = 1,
    SAY_KITE                                    = 2,
    SAY_DEATH                                   = 3,
    SAY_BERSERK                                 = 4,
    SAY_HARDMODE_ON                             = 5,
};

enum Emotes
{
    EMOTE_VAPORS                                = 6,
    EMOTE_ANIMUS                                = 7,
    EMOTE_BARRIER                               = 8,
    EMOTE_SURGE_OF_DARKNESS                     = 9
};

// ###### Datas ######
enum Datas
{
    DATA_SMELL_OF_SARONITE  = 1,
    DATA_SHADOWDODGER       = 2,
};

enum Actions
{
    ACTION_VAPOR_KILLED      = 1,
    ACTION_ANIMUS_KILLED     = 2,
};

// ###### Event Controlling ######
enum Events
{
    EVENT_BERSERK = 1,
    EVENT_SUMMON_VAPOR,
    EVENT_SURGE_OF_DARKNESS,
    EVENT_SHADOW_CRASH,
    EVENT_RESET_TARGET,
    EVENT_SEARING_FLAMES,
    EVENT_RESET_IMMUNITY,
    EVENT_MARK_OF_THE_FACELESS,
};

// ###### Spells ######
enum Spells
{
    // General Vezax
    SPELL_AURA_OF_DESPAIR                       = 62692, // on combat start
    SPELL_AURA_OF_DESPAIR_EFFEKT_DESPAIR        = 64848, // dont know if needet ... need test
    SPELL_CORRUPTED_RAGE                        = 68415,
    SPELL_CORRUPTED_WISDOM                      = 64646,
    SPELL_MARK_OF_THE_FACELESS                  = 63276, // Unknown Aura
    SPELL_MARK_OF_THE_FACELESS_LEECH            = 63278, // Leech Health 1 ... need custom cast
    SPELL_SARONIT_BARRIER                       = 63364, // Script Effekt, Apply while Saronit Animus spawned
    SPELL_SEARING_FLAMES                        = 62661,
    SPELL_SHADOW_CRASH                          = 62660, // Trigger Missile 62659 and 63277
    SPELL_SHADOW_CRASH_DAMAGE                   = 62659, // Explosion Damage
    SPELL_SHADOW_CRASH_AURA                     = 63277, // Triggered Cloud
    SPELL_SURGE_OF_DARKNESS                     = 62662, // every 60 seconds
    SPELL_SUMMON_SARONIT_VAPOR                  = 63081, // every 30 seconds
    SPELL_SUMMON_SARONIT_ANIMUS                 = 63145,
    // Saronit Animus - Spawnd after 8th Saronit Vapor
    SPELL_PROFOUND_DARKNESS                     = 63420,
    SPELL_VISUAL_SARONITE_ANIMUS                = 63319,
    // Saronit Vapor
    SPELL_SARONIT_VAPOR                         = 63323, // Casted on Death trigger 63322
    SPELL_SARONIT_VAPOR_AURA                    = 63322, // Unknown Aura Dummy needs Scripting ?
    SPELL_SARONIT_VAPOR_ENERGIZE                = 63337,
    SPELL_SARONIT_VAPOR_DAMAGE                  = 63338,
    // Player Shaman
    SPELL_SHAMANTIC_RAGE                        = 30823,
    SPELL_JUDGEMENTS_OF_THE_WISE_1              = 31876,
    SPELL_JUDGEMENTS_OF_THE_WISE_2              = 31877,
    SPELL_JUDGEMENTS_OF_THE_WISE_3              = 31878,
    SPELL_BERSERK                               = 47008,
};

enum VezaxConsts
{
    MIN_PLAYERS_IN_RANGE_10                     = 4,
    MIN_PLAYERS_IN_RANGE_25                     = 9,
    VAPORS_TILL_ANIMUS                          = 6,
};

class boss_general_vezax : public CreatureScript
{
    public:
        boss_general_vezax() : CreatureScript("boss_general_vezax") { }

        struct boss_general_vezaxAI : public BossAI
        {
            boss_general_vezaxAI(Creature* creature) : BossAI(creature, BOSS_VEZAX) {}

            void Reset()
            {
                BossAI::Reset();

                VaporCount = 0;
                SaronitAnimusGUID.Clear();

                HitByShadowCrash = false;
                AnimusKilled = false;
                VaporKilled = false;
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                //DoCastSelf(SPELL_CORRUPTED_RAGE, true);
                DoCast(SPELL_AURA_OF_DESPAIR);
                Talk(SAY_AGGRO);

                events.ScheduleEvent(EVENT_SUMMON_VAPOR, 30*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SURGE_OF_DARKNESS, 60*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SHADOW_CRASH, 10*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SEARING_FLAMES, urand(5, 10) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_MARK_OF_THE_FACELESS, urand(15, 25) * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_BERSERK, 10*MINUTE*IN_MILLISECONDS);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (target->GetTypeId() == TYPEID_PLAYER && spell->Id == SPELL_SHADOW_CRASH_DAMAGE)
                    HitByShadowCrash = true;
            }

            void JustSummoned(Creature* summoned)
            {
                BossAI::JustSummoned(summoned);

                switch (summoned->GetEntry())
                {
                    case NPC_SARONIT_VAPOR:
                        ++VaporCount;
                        if (!VaporKilled && VaporCount >= VAPORS_TILL_ANIMUS)
                            DoCast(SPELL_SUMMON_SARONIT_ANIMUS);
                        break;
                    case NPC_SARONIT_ANIMUS:
                        Talk(SAY_HARDMODE_ON);
                        Talk(EMOTE_BARRIER);

                        SaronitAnimusGUID = summoned->GetGUID();

                        std::list<Creature*> vaporList;
                        me->GetCreatureListWithEntryInGrid(vaporList, NPC_SARONIT_VAPOR, 220.0f);
                        for (std::list<Creature*>::const_iterator itr = vaporList.begin(); itr != vaporList.end(); ++itr)
                            (*itr)->DespawnOrUnsummon();

                        summoned->AI()->AttackStart(me->GetVictim());
                        DoCast(SPELL_SARONIT_BARRIER);

                        if (me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                            events.CancelEvent(EVENT_SEARING_FLAMES);
                        break;
                }
            }

            void KilledUnit(Unit* /*victim*/)
            {
                if (!urand(0, 5))
                    Talk(SAY_SLAY);
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);
                Talk(SAY_DEATH);
            }

            Unit* CheckPlayersInRange(uint32 PlayersMin, float RangeMin, float RangeMax)
            {
                if (!me->GetMap()->IsDungeon())
                    return NULL;

                std::list<Player*> playerList;
                for (Player& player : me->GetMap()->GetPlayers())
                {
                    if (!player.IsAlive() || player.IsGameMaster())
                        continue;

                    float distance = me->GetDistance(player);
                    if (distance < RangeMin || distance > RangeMax)
                        continue;

                    playerList.push_back(&player);
                }

                if (playerList.size() < PlayersMin)
                    return NULL;

                return Trinity::Containers::SelectRandomContainerElement(playerList);
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
                        case EVENT_SUMMON_VAPOR:
                            if (VaporCount < VAPORS_TILL_ANIMUS)
                                DoCast(SPELL_SUMMON_SARONIT_VAPOR);

                            if (!SaronitAnimusGUID || VaporCount < VAPORS_TILL_ANIMUS)
                                events.ScheduleEvent(EVENT_SUMMON_VAPOR, 30*IN_MILLISECONDS);
                            else
                                summons.DespawnEntry(NPC_SARONIT_VAPOR);
                            return;
                        case EVENT_SURGE_OF_DARKNESS:
                            Talk(SAY_KITE);
                            Talk(EMOTE_SURGE_OF_DARKNESS);
                            DoCastSelf(SPELL_SURGE_OF_DARKNESS);
                            events.ScheduleEvent(EVENT_SURGE_OF_DARKNESS, 63*IN_MILLISECONDS);
                            return;
                        case EVENT_SHADOW_CRASH:
                        {
                            Unit* target = CheckPlayersInRange(RAID_MODE(MIN_PLAYERS_IN_RANGE_10, MIN_PLAYERS_IN_RANGE_25), 15.0f, 100.0f);
                            if (!target)
                                target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                            if (target)
                            {
                                me->SetTarget(target->GetGUID());
                                DoCast(target, SPELL_SHADOW_CRASH);
                            }
                            events.ScheduleEvent(EVENT_SHADOW_CRASH, 13*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_RESET_TARGET, 1*IN_MILLISECONDS);
                            return;
                        }
                        case EVENT_RESET_TARGET:
                            if (me->GetVictim())
                                me->SetTarget(me->GetVictim()->GetGUID());
                            break;
                        case EVENT_MARK_OF_THE_FACELESS:
                        {
                            Unit* target = CheckPlayersInRange(RAID_MODE(MIN_PLAYERS_IN_RANGE_10, MIN_PLAYERS_IN_RANGE_25), 15.0f, 100.0f);
                            if (!target)
                                target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                            if (target)
                                DoCast(target, SPELL_MARK_OF_THE_FACELESS);
                            events.ScheduleEvent(EVENT_MARK_OF_THE_FACELESS, urand(18, 21) * IN_MILLISECONDS);
                            return;
                        }
                        case EVENT_BERSERK:
                            DoCastSelf(SPELL_BERSERK, true);
                            Talk(SAY_BERSERK);
                            break;
                        case EVENT_SEARING_FLAMES:
                            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, false);
                            DoCast(SPELL_SEARING_FLAMES);
                            events.ScheduleEvent(EVENT_SEARING_FLAMES, urand(10, 15) * IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_RESET_IMMUNITY, 0);
                            return;
                        case EVENT_RESET_IMMUNITY: // called right after Searing Flames' UNIT_STATE_CASTING gets removed
                            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            uint32 GetData(uint32 type) const
            {
                if (type == DATA_SMELL_OF_SARONITE)
                    return (!VaporKilled && SaronitAnimusGUID && AnimusKilled) ? 1 : 0;
                else if (type == DATA_SHADOWDODGER)
                    return !HitByShadowCrash ? 1 : 0;

                return 0;
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_VAPOR_KILLED:
                        VaporKilled = true;
                        break;
                    case ACTION_ANIMUS_KILLED:
                        AnimusKilled = true;

                        me->RemoveAurasDueToSpell(SPELL_SARONIT_BARRIER);

                        if (me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                            events.ScheduleEvent(EVENT_SEARING_FLAMES, urand(10, 15) * IN_MILLISECONDS);

                        me->AddLootMode(LOOT_MODE_HARD_MODE_1);
                        break;
                }
            }

        private:
            uint8 VaporCount;
            ObjectGuid SaronitAnimusGUID;
            bool AnimusKilled;
            bool VaporKilled;
            bool HitByShadowCrash;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_general_vezaxAI(creature);
        }
};

class npc_saronit_vapor : public CreatureScript
{
    public:
        npc_saronit_vapor() : CreatureScript("npc_saronit_vapor") { }

        struct npc_saronit_vaporAI : public ScriptedAI
        {
            npc_saronit_vaporAI(Creature* creature) : ScriptedAI(creature)
            {
                Talk(EMOTE_VAPORS);

                instance = me->GetInstanceScript();

                me->SetReactState(REACT_PASSIVE);

                me->GetMotionMaster()->MoveRandom(50.0f);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override {}

            void DamageTaken(Unit* /*attacker*/, uint32 &damage)
            {
                if (damage >= me->GetHealth())
                {
                    damage = me->GetHealth() - 1;

                    me->GetMotionMaster()->Clear();
                    me->SetControlled(true, UNIT_STATE_ROOT);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                    me->SetStandState(UNIT_STAND_STATE_DEAD);
                    me->SetHealth(me->GetMaxHealth());
                    me->RemoveAllAuras();

                    me->DespawnOrUnsummon(30000);

                    DoCastSelf(SPELL_SARONIT_VAPOR, true);

                    if (Creature* vezax = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_VEZAX)) : ObjectGuid::Empty))
                        vezax->AI()->DoAction(ACTION_VAPOR_KILLED);
                }
            }

            void UpdateAI(uint32 /*diff*/) {}

        private:
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_saronit_vaporAI(creature);
        }
};

class npc_saronit_animus : public CreatureScript
{
    public:
        npc_saronit_animus() : CreatureScript("npc_saronit_animus") { }

        struct npc_saronit_animusAI : public ScriptedAI
        {
            npc_saronit_animusAI(Creature* creature) : ScriptedAI(creature)
            {
                instance = me->GetInstanceScript();
            }

            void Reset()
            {
                ProfoundDarknessTimer = 1000;
                DoCastSelf(SPELL_VISUAL_SARONITE_ANIMUS);
            }

            void EnterCombat(Unit* /*who*/)
            {
                ProfoundDarknessTimer = 1000;
            }

            void JustDied(Unit* /*killer*/)
            {
                if (Creature* vezax = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(BOSS_VEZAX)) : ObjectGuid::Empty))
                    vezax->AI()->DoAction(ACTION_ANIMUS_KILLED);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() )
                    return;

                if (ProfoundDarknessTimer < diff)
                {
                    DoCast(SPELL_PROFOUND_DARKNESS);
                    ProfoundDarknessTimer = RAID_MODE(7000, 3000);
                }
                else
                    ProfoundDarknessTimer -= diff;

                DoMeleeAttackIfReady();
            }

        private:
            InstanceScript* instance;
            uint32 ProfoundDarknessTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_saronit_animusAI(creature);
        }
};

class spell_general_vezax_aura_of_despair_aura : public SpellScriptLoader
{
public:
    spell_general_vezax_aura_of_despair_aura() : SpellScriptLoader("spell_general_vezax_aura_of_despair_aura") { }

    class spell_general_vezax_aura_of_despair_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_general_vezax_aura_of_despair_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_AURA_OF_DESPAIR_EFFEKT_DESPAIR))
                return false;
            if (!sSpellStore.LookupEntry(SPELL_CORRUPTED_RAGE))
                return false;
            return true;
        }

        void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                return;

            if (Player* target = GetTarget()->ToPlayer())
            {
                switch (target->getClass())
                {
                    case CLASS_SHAMAN:
                        if (target->HasSpell(SPELL_SHAMANTIC_RAGE))
                            target->CastSpell(target, SPELL_CORRUPTED_RAGE, true);
                        break;
                    case CLASS_PALADIN:
                        if (target->HasSpell(SPELL_JUDGEMENTS_OF_THE_WISE_1) || target->HasSpell(SPELL_JUDGEMENTS_OF_THE_WISE_2) || target->HasSpell(SPELL_JUDGEMENTS_OF_THE_WISE_3))
                            target->CastSpell(target, SPELL_CORRUPTED_WISDOM, true);
                        break;
                }

                target->CastSpell(target, SPELL_AURA_OF_DESPAIR_EFFEKT_DESPAIR, true);
            }
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* target = GetTarget()->ToPlayer();

            target->RemoveAurasDueToSpell(SPELL_CORRUPTED_RAGE);
            target->RemoveAurasDueToSpell(SPELL_CORRUPTED_WISDOM);
            target->RemoveAurasDueToSpell(SPELL_AURA_OF_DESPAIR_EFFEKT_DESPAIR);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_general_vezax_aura_of_despair_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PREVENT_REGENERATE_POWER, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_general_vezax_aura_of_despair_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PREVENT_REGENERATE_POWER, AURA_EFFECT_HANDLE_REAL);
        }

    };

    AuraScript* GetAuraScript() const
    {
        return new spell_general_vezax_aura_of_despair_AuraScript();
    }
};

class spell_general_vezax_mark_of_the_faceless_aura : public SpellScriptLoader
{
public:
    spell_general_vezax_mark_of_the_faceless_aura() : SpellScriptLoader("spell_general_vezax_mark_of_the_faceless_aura") { }

    class spell_general_vezax_mark_of_the_faceless_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_general_vezax_mark_of_the_faceless_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/)
        {
            if (!sSpellStore.LookupEntry(SPELL_MARK_OF_THE_FACELESS_LEECH))
                return false;
            return true;
        }

        void HandleDummyTick(AuraEffect const* aurEff)
        {
            GetCaster()->CastCustomSpell(SPELL_MARK_OF_THE_FACELESS_LEECH, SPELLVALUE_BASE_POINT1, aurEff->GetAmount(), GetTarget(), true);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_general_vezax_mark_of_the_faceless_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }

    };

    AuraScript* GetAuraScript() const
    {
        return new spell_general_vezax_mark_of_the_faceless_AuraScript();
    }
};

class spell_general_vezax_mark_of_the_faceless_drain : public SpellScriptLoader
{
    public:
        spell_general_vezax_mark_of_the_faceless_drain() : SpellScriptLoader("spell_general_vezax_mark_of_the_faceless_drain") { }

        class spell_general_vezax_mark_of_the_faceless_drain_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_general_vezax_mark_of_the_faceless_drain_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove(GetExplTargetUnit());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_general_vezax_mark_of_the_faceless_drain_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_general_vezax_mark_of_the_faceless_drain_SpellScript();
        }
};

class spell_general_vezax_shadow_crash_aura : public SpellScriptLoader
{
public:
    spell_general_vezax_shadow_crash_aura() : SpellScriptLoader("spell_general_vezax_shadow_crash_aura") { }

    class spell_general_vezax_shadow_crash_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_general_vezax_shadow_crash_aura_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/)
        {
            if (!sSpellStore.LookupEntry(65269))
                return false;
            return true;
        }

        void HandleOnEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* target = GetTarget())
            {
                if (!target->HasAura(65269))
                    target->AddAura(65269, target);
            }
        }

        void HandleOnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* target = GetTarget())
                target->RemoveAura(65269);
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_general_vezax_shadow_crash_aura_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(spell_general_vezax_shadow_crash_aura_AuraScript::HandleOnEffectRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
        }

    };

    AuraScript* GetAuraScript() const
    {
        return new spell_general_vezax_shadow_crash_aura_AuraScript();
    }
};

class spell_general_vezax_saronit_vapor : public SpellScriptLoader
{
public:
    spell_general_vezax_saronit_vapor() : SpellScriptLoader("spell_general_vezax_saronit_vapor") { }

    class spell_general_vezax_saronit_vapor_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_general_vezax_saronit_vapor_AuraScript);

        void HandleOnEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
        {
            int32 mana = int32(aurEff->GetAmount()/aurEff->GetBase()->GetStackAmount() * pow(2.0f, aurEff->GetBase()->GetStackAmount())); // mana restore - bp * 2^stackamount
            int32 damage = mana * 2; // damage
            if (GetCaster() && GetTarget())
            {
                GetCaster()->CastCustomSpell(GetTarget(), SPELL_SARONIT_VAPOR_ENERGIZE, &mana, NULL, NULL, true);
                GetCaster()->CastCustomSpell(GetTarget(), SPELL_SARONIT_VAPOR_DAMAGE, &damage, NULL, NULL, true);
            }
        }

        void Register()
        {
            OnEffectApply += AuraEffectApplyFn(spell_general_vezax_saronit_vapor_AuraScript::HandleOnEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_CHANGE_AMOUNT);
        }

    };

    AuraScript* GetAuraScript() const
    {
        return new spell_general_vezax_saronit_vapor_AuraScript();
    }
};

class achievement_shadowdodger : public AchievementCriteriaScript
{
    public:
        achievement_shadowdodger() : AchievementCriteriaScript("achievement_shadowdodger") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            return target && target->GetAI()->GetData(DATA_SHADOWDODGER);
        }
};

class achievement_i_love_the_smell_of_saronite_in_the_morning : public AchievementCriteriaScript
{
    public:
        achievement_i_love_the_smell_of_saronite_in_the_morning() : AchievementCriteriaScript("achievement_i_love_the_smell_of_saronite_in_the_morning") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            return target && target->GetAI()->GetData(DATA_SMELL_OF_SARONITE);
        }
};

void AddSC_boss_general_vezax()
{
    new boss_general_vezax();

    new npc_saronit_vapor();
    new npc_saronit_animus();

    new spell_general_vezax_aura_of_despair_aura();
    new spell_general_vezax_mark_of_the_faceless_aura();
    new spell_general_vezax_mark_of_the_faceless_drain();
    new spell_general_vezax_shadow_crash_aura();
    new spell_general_vezax_saronit_vapor();

    new achievement_shadowdodger();
    new achievement_i_love_the_smell_of_saronite_in_the_morning();
}
