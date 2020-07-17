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
#include "SpellAuraEffects.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "ulduar.h"
#include "Vehicle.h"
#include "Player.h"

// ###### Related Creatures/Object ######
enum Creatures
{
    NPC_RUBBLE_STALKER              = 33809,
    NPC_KOLOGARN_BRIDGE             = 34297,
    NPC_FOCUSED_EYEBEAM_LEFT        = 33632,
    NPC_FOCUSED_EYEBEAM_RIGHT       = 33802,
    NPC_LEFT_ARM                    = 32933,
    NPC_RIGHT_ARM                   = 32934,
};

// ###### Texts ######
enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SLAY                                    = 1,
    SAY_LEFT_ARM_GONE                           = 2,
    SAY_RIGHT_ARM_GONE                          = 3,
    SAY_SHOCKWAVE                               = 4,
    SAY_GRAB_PLAYER                             = 5,
    SAY_DEATH                                   = 6,
    SAY_BERSERK                                 = 7,
};

#define EMOTE_EYEBEAM           "Kologarn focusing his eyes on you"
#define EMOTE_LEFT              "The Left Arm has regrown!"
#define EMOTE_RIGHT             "The Right Arm has regrown!"
#define EMOTE_STONE             "Kologarn casts Stone Grip!"

const Position chestSpawnLoc = {1836.52f, -36.1111f, 448.81f, 0.558504f};

static Position Location[] =
{
    { 1756.25f + irand(-3, 3), -8.3f + irand(-3, 3), 448.8f, 0 }
};

// ###### Datas ######
enum Data
{
    DATA_RUBBLE_AND_ROLL                        = 1,
    DATA_DISARMED                               = 2,
    DATA_WITH_OPEN_ARMS                         = 3,
    DATA_EYEBEAM_TARGET                         = 4,
};

enum Achievements
{
    ACHIEV_DISARMED_START_EVENT                 = 21687,
};

enum ArmFlags
{
    ARM_RIGHT       = 0x1,
    ARM_LEFT        = 0x2,
};

// ###### Event Controlling ######
enum Events
{
    EVENT_MELEE_CHECK           = 1,
    EVENT_SMASH                 = 2,
    EVENT_SWEEP                 = 3,
    EVENT_STONE_SHOUT           = 4,
    EVENT_STONE_GRIP            = 5,
    EVENT_FOCUSED_EYEBEAM       = 6,
    EVENT_RESPAWN_LEFT_ARM      = 7,
    EVENT_RESPAWN_RIGHT_ARM     = 8,
    EVENT_ENRAGE                = 9,
};

enum Groups
{
    EVENT_GROUP_RIGHT_ARM       = 1,
    EVENT_GROUP_LEFT_ARM        = 2,
    EVENT_GROUP_NO_ARMS         = 3,
};

// ###### Spells ######
enum Spells
{
    // Passive
    SPELL_KOLOGARN_REDUCE_PARRY         = 64651,
    SPELL_KOLOGARN_PACIFY               = 63726,
    SPELL_KOLOGARN_UNK_0                = 65219,

    // Kologarn
    SPELL_ARM_DEAD_DAMAGE               = 63629,
    SPELL_TWO_ARM_SMASH                 = 63356,
    SPELL_ONE_ARM_SMASH                 = 63573,
    SPELL_ARM_SWEEP                     = 63766,
    SPELL_STONE_SHOUT                   = 63716,
    SPELL_PETRIFY_BREATH                = 62030,
    SPELL_STONE_GRIP                    = 62166,
    SPELL_FOCUSED_EYEBEAM_PERIODIC      = 63347,
    SPELL_BERSERK                       = 47008,
    // Rubble
    SPELL_RUBBLE_AOE                    = 63818,
    // Misc
    SPELL_SUMMON_RUBBLE                 = 63633,
    SPELL_FALLING_RUBBLE                = 63821,
    SPELL_ARM_ENTER_VEHICLE             = 65343,
    SPELL_ARM_ENTER_VISUAL              = 64753,
    SPELL_SUMMON_FOCUSED_EYEBEAM        = 63342,
    SPELL_FOCUSED_EYEBEAM_VISUAL_LEFT   = 63676,
    SPELL_FOCUSED_EYEBEAM_VISUAL_RIGHT  = 63702,
};


class boss_kologarn : public CreatureScript
{
    public:
        boss_kologarn() : CreatureScript("boss_kologarn") { }

        struct boss_kologarnAI : public BossAI
        {
            boss_kologarnAI(Creature* creature) : BossAI(creature, BOSS_KOLOGARN), vehicle(me->GetVehicleKit())
            {
                ASSERT(vehicle);
                ASSERT(instance);

                me->SetDisableGravity(true);
                ArmFlag = 0;
            }

            void Reset()
            {
                BossAI::Reset();

                me->SetReactState(REACT_DEFENSIVE);

                DoCast(SPELL_KOLOGARN_REDUCE_PARRY);

                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_DISARMED_START_EVENT);
            }

            void EnterCombat(Unit* who)
            {
                //! Called endless when evading due to no target
                if (who->GetEntry() == NPC_LEFT_ARM || who->GetEntry() == NPC_RIGHT_ARM)
                    EnterEvadeMode();

                BossAI::EnterCombat(who);
                Talk(SAY_AGGRO);

                NoArmDied = true;
                RubbleCount = 0;
                EyebeamTargetGUID.Clear();

                events.ScheduleEvent(EVENT_MELEE_CHECK,       6*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SMASH,             5*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SWEEP,            19*IN_MILLISECONDS, EVENT_GROUP_LEFT_ARM);
                events.ScheduleEvent(EVENT_STONE_GRIP,       25*IN_MILLISECONDS, EVENT_GROUP_RIGHT_ARM);
                events.ScheduleEvent(EVENT_FOCUSED_EYEBEAM,  21*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_ENRAGE,          600*IN_MILLISECONDS);

                for (uint8 i = 0; i < 2; ++i)
                    if (Unit* arm = vehicle->GetPassenger(i))
                        DoZoneInCombat(arm->ToCreature());

                me->SetReactState(REACT_AGGRESSIVE);
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);

                Talk(SAY_DEATH);
                DoCast(SPELL_KOLOGARN_PACIFY);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                me->SummonGameObject(me->GetMap()->GetDifficulty() == RAID_DIFFICULTY_25MAN_NORMAL ? GO_KOLOGARN_CHEST_HERO : GO_KOLOGARN_CHEST, chestSpawnLoc.m_positionX, chestSpawnLoc.m_positionY, chestSpawnLoc.m_positionZ, chestSpawnLoc.m_orientation, 0, 0, 0, 0, DAY);
                // not selectable wehen kologarn is unfriendly
                me->setFaction(35);
                me->GetMotionMaster()->MoveTargetedHome();
            }

            void DamageTaken(Unit* /*who*/, uint32& damage)
            {
                //! Needed because JustDied is called after Creature::setDeathState which triggers falling down.
                if (damage >= me->GetHealth())
                    me->SetDisableGravity(false);
            }

            void KilledUnit(Unit* /*victim*/)
            {
                if (!urand(0, 2))
                    Talk(SAY_SLAY);
            }

            void JustSummoned(Creature* summon)
            {
                summons.Summon(summon);
                switch (summon->GetEntry())
                {
                    case NPC_LEFT_ARM:
                        ArmFlag |= ARM_LEFT;
                        summon->CastSpell(summon, SPELL_ARM_ENTER_VISUAL, true);
                        break;
                    case NPC_RIGHT_ARM:
                        ArmFlag |= ARM_RIGHT;
                        summon->CastSpell(summon, SPELL_ARM_ENTER_VISUAL, true);
                        break;
                    case NPC_FOCUSED_EYEBEAM_LEFT:
                    case NPC_FOCUSED_EYEBEAM_RIGHT:
                        summon->CastSpell(me, summon->GetEntry() == NPC_FOCUSED_EYEBEAM_LEFT ? SPELL_FOCUSED_EYEBEAM_VISUAL_LEFT : SPELL_FOCUSED_EYEBEAM_VISUAL_RIGHT, true);
                        summon->CastSpell(summon, SPELL_FOCUSED_EYEBEAM_PERIODIC, true);
                        summon->SetReactState(REACT_PASSIVE);
                        if (Unit* target = ObjectAccessor::GetUnit(*summon, EyebeamTargetGUID))
                        {
                            summon->Attack(target, false);
                            summon->GetMotionMaster()->MoveChase(target, 1.0f, summon->GetEntry() == NPC_FOCUSED_EYEBEAM_LEFT ? -M_PI/2 : M_PI/2);
                        }
                        return;
                    case NPC_RUBBLE:
                        ++RubbleCount;
                        DoZoneInCombat(summon);
                        return;
                    default:
                        return;
                }

                if (ArmFlag)
                    instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_DISARMED_START_EVENT);
            }

            void SummonedCreatureDies(Creature* summon, Unit* /*killer*/)
            {
                summons.Despawn(summon);
                switch (summon->GetEntry())
                {
                    case NPC_LEFT_ARM:
                        ArmFlag &= ~ARM_LEFT;
                        Talk(SAY_LEFT_ARM_GONE);
                        events.ScheduleEvent(EVENT_RESPAWN_LEFT_ARM, 40*IN_MILLISECONDS);
                        events.CancelEventGroup(EVENT_GROUP_LEFT_ARM);
                        break;
                    case NPC_RIGHT_ARM:
                        ArmFlag &= ~ARM_RIGHT;
                        Talk(SAY_RIGHT_ARM_GONE);
                        events.ScheduleEvent(EVENT_RESPAWN_RIGHT_ARM, 40*IN_MILLISECONDS);
                        events.CancelEventGroup(EVENT_GROUP_RIGHT_ARM);
                        break;
                    default:
                        return;
                }

                NoArmDied = false;
                summon->CastSpell(me, SPELL_ARM_DEAD_DAMAGE, true);
                summon->DespawnOrUnsummon();

                if (!ArmFlag)
                {
                    events.ScheduleEvent(EVENT_STONE_SHOUT, 5000, EVENT_GROUP_NO_ARMS);
                    instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_DISARMED_START_EVENT);
                }
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_RUBBLE_AND_ROLL:
                        return RubbleCount >= 25;
                    case DATA_DISARMED:
                        return ArmFlag == (ARM_RIGHT | ARM_LEFT);
                    case DATA_WITH_OPEN_ARMS:
                        return NoArmDied;
                }

                return 0;
            }

            void SetGuidData(ObjectGuid guid, uint32 id) override
            {
                if (id == DATA_EYEBEAM_TARGET)
                    EyebeamTargetGUID = guid;
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
                        case EVENT_MELEE_CHECK:
                            if (!me->IsWithinMeleeRange(me->GetVictim()))
                                DoCast(SPELL_PETRIFY_BREATH);
                            events.ScheduleEvent(EVENT_MELEE_CHECK, 4*IN_MILLISECONDS);
                            break;
                        case EVENT_SWEEP:
                            Talk(SAY_SHOCKWAVE);
                            DoCast(SPELL_ARM_SWEEP);
                            events.ScheduleEvent(EVENT_SWEEP, urand(18000, 25000), EVENT_GROUP_LEFT_ARM);
                            break;
                        case EVENT_SMASH:
                            if (ArmFlag == (ARM_LEFT | ARM_RIGHT))
                                DoCastVictim(SPELL_TWO_ARM_SMASH);
                            else if ((ArmFlag & ARM_LEFT) || (ArmFlag & ARM_RIGHT))
                                DoCastVictim(SPELL_ONE_ARM_SMASH);
                            events.ScheduleEvent(EVENT_SMASH, 15*IN_MILLISECONDS);
                            break;
                        case EVENT_STONE_SHOUT:
                            DoCast(SPELL_STONE_SHOUT);
                            events.ScheduleEvent(EVENT_STONE_SHOUT, 2*IN_MILLISECONDS, EVENT_GROUP_NO_ARMS);
                            break;
                        case EVENT_ENRAGE:
                            Talk(SAY_BERSERK);
                            DoCastSelf(SPELL_BERSERK, true);
                            break;
                        case EVENT_RESPAWN_LEFT_ARM:
                            RespawnArm(NPC_LEFT_ARM);
                            me->MonsterTextEmote(EMOTE_LEFT, 0, true);
                            events.ScheduleEvent(EVENT_SWEEP, 19*IN_MILLISECONDS, EVENT_GROUP_LEFT_ARM);
                            events.CancelEventGroup(EVENT_GROUP_NO_ARMS);
                            break;
                        case EVENT_RESPAWN_RIGHT_ARM:
                            RespawnArm(NPC_RIGHT_ARM);
                            me->MonsterTextEmote(EMOTE_RIGHT, 0, true);
                            events.ScheduleEvent(EVENT_STONE_GRIP, 25*IN_MILLISECONDS, EVENT_GROUP_RIGHT_ARM);
                            events.CancelEventGroup(EVENT_GROUP_NO_ARMS);
                            break;
                        case EVENT_STONE_GRIP:
                            Talk(SAY_GRAB_PLAYER);
                            me->MonsterTextEmote(EMOTE_STONE, 0, true);
                            DoCast(SPELL_STONE_GRIP);
                            events.ScheduleEvent(EVENT_STONE_GRIP, 25*IN_MILLISECONDS, EVENT_GROUP_RIGHT_ARM);
                            break;
                        case EVENT_FOCUSED_EYEBEAM:
                            EyebeamTargetGUID.Clear();
                            if (me->GetHealth() < 25000)
                                return;
                            DoCast(SPELL_SUMMON_FOCUSED_EYEBEAM);
                            events.ScheduleEvent(EVENT_FOCUSED_EYEBEAM, urand(15*IN_MILLISECONDS, 35*IN_MILLISECONDS));
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void RespawnArm(uint32 entry)
            {
                vehicle->InstallAccessory(entry, entry == NPC_LEFT_ARM ? 0 : 1, true, TEMPSUMMON_MANUAL_DESPAWN, 0);
            }

            private:
                Vehicle* vehicle;
                uint8 ArmFlag;
                bool NoArmDied;
                uint8 RubbleCount;
                ObjectGuid EyebeamTargetGUID;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_kologarnAI(creature);
        }
};

class npc_kologarn_arm : public CreatureScript
{
    public:
        npc_kologarn_arm() : CreatureScript("npc_kologarn_arm") { }

        struct npc_kologarn_armAI : public NullCreatureAI
        {
            npc_kologarn_armAI(Creature* creature) : NullCreatureAI(creature) { }

            void EnterCombat(Unit* who)
            {
                if (Creature* kologarn = me->GetVehicleCreatureBase())
                    kologarn->AI()->AttackStart(who);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_kologarn_armAI(creature);
        }
};

class npc_rubble : public CreatureScript
{
    public:
        npc_rubble() : CreatureScript("npc_rubble") { }

        struct npc_rubbleAI : public ScriptedAI
        {
            npc_rubbleAI(Creature* creature) : ScriptedAI(creature){}

            uint32 AoeTimer;
            bool AoeDone;

            void Reset()
            {
                AoeTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
                AoeDone = false;
            }

            void MoveInLineOfSight(Unit* who)
            {
                if(me->IsWithinMeleeRange(who) && !AoeDone)
                    AoeTimer = 200;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (AoeTimer <= diff)
                {
                    DoCast(SPELL_RUBBLE_AOE);
                    AoeDone = true;
                    AoeTimer = urand(3*IN_MILLISECONDS, 4*IN_MILLISECONDS);
                }
                else
                    AoeTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_rubbleAI(creature);
        }
};

class DistanceTargetSelector
{
    public:
        DistanceTargetSelector(Unit* unit, int32 dist) : me(unit), distance(dist) {}

        bool operator() (WorldObject* other)
        {
            if (me->GetExactDist2d(other) < distance)
                return true;

            return false;
        }

        Unit* me;
        float distance;
};

//! Effect 2 SPELL_EFFECT_SCRIPT_EFFECT with BasePoints = 23 seems to be the range
class spell_kologarn_eyebeam_summon : public SpellScriptLoader
{
    public:
        spell_kologarn_eyebeam_summon() : SpellScriptLoader("spell_kologarn_eyebeam_summon") { }

        class spell_kologarn_eyebeam_summon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_kologarn_eyebeam_summon_SpellScript);

            void TargetSelector(std::list<WorldObject*>& unitList)
            {
                targetList = unitList;

                targetList.remove_if(DistanceTargetSelector(GetCaster(), GetSpellInfo()->Effects[EFFECT_2].CalcValue()));

                if (targetList.empty())
                    targetList.push_back(Trinity::Containers::SelectRandomContainerElement(unitList));

                unitList.clear();
            }

            void SelectTargets(std::list<WorldObject*>& unitList)
            {
                unitList = targetList;
            }

            void SayText()
            {
                if (Unit* target = GetHitUnit())
                {
                    GetCaster()->MonsterWhisper(EMOTE_EYEBEAM, target->ToPlayer(), true);
                    GetCaster()->GetAI()->SetGuidData(target->GetGUID(), DATA_EYEBEAM_TARGET);
                }
            }

            void HandleForceCast(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);

                uint32 triggerId = GetSpellInfo()->Effects[effIndex].TriggerSpell;
                if (Unit* target = GetHitUnit())
                    target->CastSpell(target, triggerId, true, 0, 0, GetCaster()->GetGUID());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kologarn_eyebeam_summon_SpellScript::TargetSelector, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kologarn_eyebeam_summon_SpellScript::SelectTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_kologarn_eyebeam_summon_SpellScript::SelectTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                BeforeHit += SpellHitFn(spell_kologarn_eyebeam_summon_SpellScript::SayText);
                OnEffectHitTarget += SpellEffectFn(spell_kologarn_eyebeam_summon_SpellScript::HandleForceCast, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
                OnEffectHitTarget += SpellEffectFn(spell_kologarn_eyebeam_summon_SpellScript::HandleForceCast, EFFECT_1, SPELL_EFFECT_FORCE_CAST);
            }

            private:
                std::list<WorldObject*> targetList;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_kologarn_eyebeam_summon_SpellScript();
        }
};

class spell_kologarn_arm_damage : public SpellScriptLoader
{
    public:
        spell_kologarn_arm_damage() : SpellScriptLoader("spell_kologarn_arm_damage") { }

        class spell_kologarn_arm_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_kologarn_arm_damage_SpellScript);

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Creature* target = GetHitCreature())
                    target->LowerPlayerDamageReq(GetHitDamage());

                Unit* caster = GetCaster();

                Creature* rubbleStalker = NULL;
                std::list<Creature*> rubbleStalkers;
                caster->GetCreatureListWithEntryInGrid(rubbleStalkers, NPC_RUBBLE_STALKER, 70.0f);

                for (std::list<Creature*>::iterator itr = rubbleStalkers.begin(); itr != rubbleStalkers.end(); ++itr)
                {
                    Creature* stalker = *itr;
                    if (caster->GetEntry() == NPC_LEFT_ARM)                    // Left Arm: choose left Stalker
                        if (stalker->GetPositionY() < caster->GetPositionY())
                        {
                            rubbleStalker = stalker;
                            break;
                        }

                    if (caster->GetEntry() == NPC_RIGHT_ARM)                   // Right Arm: choose right Stalker
                        if (stalker->GetPositionY() > caster->GetPositionY())
                        {
                            rubbleStalker = stalker;
                            break;
                        }
                }

                if (rubbleStalker)
                {
                    rubbleStalker->CastSpell(rubbleStalker, SPELL_FALLING_RUBBLE, true);
                    rubbleStalker->CastSpell(rubbleStalker, SPELL_SUMMON_RUBBLE, true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_kologarn_arm_damage_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_kologarn_arm_damage_SpellScript();
        }
};

class spell_ulduar_rubble_summon : public SpellScriptLoader
{
    public:
        spell_ulduar_rubble_summon() : SpellScriptLoader("spell_ulduar_rubble_summon") { }

        class spell_ulduar_rubble_summonSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ulduar_rubble_summonSpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                ObjectGuid originalCaster = caster->GetInstanceScript() ? ObjectGuid(caster->GetInstanceScript()->GetGuidData(BOSS_KOLOGARN)) : ObjectGuid::Empty;
                uint32 spellId = GetEffectValue();
                for (uint8 i = 0; i < 5; ++i)
                    caster->CastSpell(caster, spellId, true, NULL, NULL, originalCaster);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_ulduar_rubble_summonSpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ulduar_rubble_summonSpellScript();
        }
};

struct ExcludeTankCheck
{
    public:
        ExcludeTankCheck(Creature* source) : _source(source) { }
        bool operator()(WorldObject const* target) const
        {
            if (target == _source->GetVictim())
                return true;

            return false;
        }

    private:
        Creature const* _source;
};

//! 1st with : 62166, directly casted
class spell_ulduar_stone_grip_cast_target : public SpellScriptLoader
{
    public:
        spell_ulduar_stone_grip_cast_target() : SpellScriptLoader("spell_ulduar_stone_grip_cast_target") { }

        class spell_ulduar_stone_grip_cast_target_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ulduar_stone_grip_cast_target_SpellScript);

            bool Load()
            {
                if (GetCaster()->GetTypeId() != TYPEID_UNIT)
                    return false;
                return true;
            }

            void FilterTargetsInitial(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(ExcludeTankCheck(GetCaster()->ToCreature()));

                uint32 maxTargets = 1;
                if (GetSpellInfo()->Id == 63981)
                    maxTargets = urand(1, 3);

                Trinity::Containers::RandomResize(unitList, maxTargets);

                targetList = unitList;
            }

            void FillTargetsSubsequential(std::list<WorldObject*>& unitList)
            {
                unitList = targetList;
            }

            void HandleForceCast(SpellEffIndex effIndex)
            {
                Player* player = GetHitPlayer();

                if (!player)
                    return;

                // Don't send m_originalCasterGUID param here or underlying AuraEffect::HandleAuraControlVehicle will fail on caster == target
                player->CastSpell(GetExplTargetUnit(), GetSpellInfo()->Effects[effIndex].TriggerSpell, true);
                PreventHitEffect(effIndex);
            }


            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ulduar_stone_grip_cast_target_SpellScript::FilterTargetsInitial, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_ulduar_stone_grip_cast_target_SpellScript::HandleForceCast, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ulduar_stone_grip_cast_target_SpellScript::FillTargetsSubsequential, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_ulduar_stone_grip_cast_target_SpellScript::FillTargetsSubsequential, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
            }

            // Shared between effects
            std::list<WorldObject*> targetList;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ulduar_stone_grip_cast_target_SpellScript();
        }
};

class spell_ulduar_cancel_stone_grip : public SpellScriptLoader
{
    public:
        spell_ulduar_cancel_stone_grip() : SpellScriptLoader("spell_ulduar_cancel_stone_grip") { }

        class spell_ulduar_cancel_stone_gripSpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ulduar_cancel_stone_gripSpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* target = GetHitPlayer();
                if (!target)
                    return;

                if (!target->GetVehicle())
                    return;

                switch (target->GetMap()->GetDifficulty())
                {
                    case RAID_DIFFICULTY_10MAN_NORMAL:
                        target->RemoveAura(GetSpellInfo()->Effects[EFFECT_0].CalcValue());
                        break;
                    case RAID_DIFFICULTY_25MAN_NORMAL:
                        target->RemoveAura(GetSpellInfo()->Effects[EFFECT_1].CalcValue());
                        break;
                    default:
                        break;
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_ulduar_cancel_stone_gripSpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ulduar_cancel_stone_gripSpellScript();
        }
};

//! 4th 64702, kills player after 15secs in grip, triggered by 64708 -> 64290 -> 62166(spell_ulduar_stone_grip_cast_target)
class spell_ulduar_squeezed_lifeless : public SpellScriptLoader
{
    public:
        spell_ulduar_squeezed_lifeless() : SpellScriptLoader("spell_ulduar_squeezed_lifeless") { }

        class spell_ulduar_squeezed_lifeless_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ulduar_squeezed_lifeless_SpellScript);

            void MoveCorpse()
            {
                GetHitUnit()->ExitVehicle();
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_ulduar_squeezed_lifeless_SpellScript::MoveCorpse);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ulduar_squeezed_lifeless_SpellScript();
        }
};

//! 3rd 64224, triggered by spell_ulduar_stone_grip
class spell_ulduar_stone_grip_absorb : public SpellScriptLoader
{
    public:
        spell_ulduar_stone_grip_absorb() : SpellScriptLoader("spell_ulduar_stone_grip_absorb") { }

        class spell_ulduar_stone_grip_absorb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ulduar_stone_grip_absorb_AuraScript);

            //! This will be called when Right Arm (vehicle) has sustained a specific amount of damage depending on instance mode
            //! What we do here is remove all harmful aura's related and teleport to safe spot.
            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_ENEMY_SPELL)
                    return;

                if (!GetOwner()->ToCreature())
                    return;

                if (Vehicle* vehicle = GetOwner()->ToCreature()->GetVehicleKit())
                    for (uint8 i = 0; i < 3; ++i)
                        if (Unit* passenger = vehicle->GetPassenger(i))
                            passenger->ExitVehicle();
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_ulduar_stone_grip_absorb_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_ulduar_stone_grip_absorb_AuraScript();
        }
};

//! 2nd 62056, force cast by spell_ulduar_stone_grip_cast_target
class spell_ulduar_stone_grip : public SpellScriptLoader
{
    public:
        spell_ulduar_stone_grip() : SpellScriptLoader("spell_ulduar_stone_grip") { }

        class spell_ulduar_stone_grip_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_ulduar_stone_grip_AuraScript);

            void OnRemoveStun(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Player* owner = GetOwner()->ToPlayer())
                {
                    owner->RemoveAurasDueToSpell(aurEff->GetAmount());
                    owner->RemoveAurasDueToSpell(64708);
                }
            }

            void OnRemoveVehicle(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    caster->StopMoving();
                    caster->GetMotionMaster()->MoveJump(Location[0], 20.0f, 20.0f);
                }
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_ulduar_stone_grip_AuraScript::OnRemoveVehicle, EFFECT_0, SPELL_AURA_CONTROL_VEHICLE, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_ulduar_stone_grip_AuraScript::OnRemoveStun, EFFECT_2, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_ulduar_stone_grip_AuraScript();
        }
};

class achievement_rubble_and_roll : public AchievementCriteriaScript
{
    public:
        achievement_rubble_and_roll() : AchievementCriteriaScript("achievement_rubble_and_roll") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (target && target->IsAIEnabled)
                return target->GetAI()->GetData(DATA_RUBBLE_AND_ROLL);

            return false;
        }
};

class achievement_disarmed : public AchievementCriteriaScript
{
    public:
        achievement_disarmed() : AchievementCriteriaScript("achievement_disarmed") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (target && target->IsAIEnabled)
                return !target->GetAI()->GetData(DATA_DISARMED);

            return false;
        }
};

class achievement_with_open_arms : public AchievementCriteriaScript
{
    public:
        achievement_with_open_arms() : AchievementCriteriaScript("achievement_with_open_arms") { }

        bool OnCheck(Player* /*source*/, Unit* target)
        {
            if (target && target->IsAIEnabled)
                return target->GetAI()->GetData(DATA_WITH_OPEN_ARMS);

            return false;
        }
};

void AddSC_boss_kologarn()
{
    new boss_kologarn();
    new npc_kologarn_arm();
    new npc_rubble();

    new spell_kologarn_eyebeam_summon();
    new spell_kologarn_arm_damage();
    new spell_ulduar_rubble_summon();
    new spell_ulduar_squeezed_lifeless();
    new spell_ulduar_cancel_stone_grip();
    new spell_ulduar_stone_grip_cast_target();
    new spell_ulduar_stone_grip_absorb();
    new spell_ulduar_stone_grip();

    new achievement_rubble_and_roll();
    new achievement_disarmed();
    new achievement_with_open_arms();
}
