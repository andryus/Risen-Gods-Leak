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
#include "SpellAuraEffects.h"
#include "Group.h"
#include "Spell.h"
#include "icecrown_citadel.h"
#include "Vehicle.h"
#include "GridNotifiers.h"
#include "CombatAI.h"
#include "DBCStores.h"
#include "area_plague_works.hpp"

enum Say
{
    // Professor Putricide
    SAY_AGGRO                       = 4,
    EMOTE_UNSTABLE_EXPERIMENT       = 5,
    SAY_PHASE_TRANSITION_HEROIC     = 6,
    SAY_TRANSFORM_1                 = 7,
    SAY_TRANSFORM_2                 = 8,    // always used for phase2 change, DO NOT GROUP WITH SAY_TRANSFORM_1
    EMOTE_MALLEABLE_GOO             = 9,
    EMOTE_CHOKING_GAS_BOMB          = 10,
    SAY_KILL                        = 11,
    SAY_BERSERK                     = 12,
    SAY_DEATH                       = 13,
};

enum Spells
{
    // Professor Putricide
    SPELL_SLIME_PUDDLE_TRIGGER              = 70341,
    SPELL_MALLEABLE_GOO                     = 70852,
    SPELL_UNSTABLE_EXPERIMENT               = 70351,
    SPELL_TEAR_GAS                          = 71617,    // phase transition
    SPELL_TEAR_GAS_CREATURE                 = 71618,
    SPELL_TEAR_GAS_CANCEL                   = 71620,
    SPELL_TEAR_GAS_PERIODIC_TRIGGER         = 73170,
    SPELL_CREATE_CONCOCTION                 = 71621,
    SPELL_GUZZLE_POTIONS                    = 71893,
    SPELL_OOZE_TANK_PROTECTION              = 71770,    // protects the tank
    SPELL_CHOKING_GAS_BOMB                  = 71255,
    SPELL_OOZE_VARIABLE                     = 74118,
    SPELL_GAS_VARIABLE                      = 74119,
    SPELL_UNBOUND_PLAGUE                    = 70911,
    SPELL_UNBOUND_PLAGUE_SEARCHER           = 70917,
    SPELL_PLAGUE_SICKNESS                   = 70953,
    SPELL_UNBOUND_PLAGUE_PROTECTION         = 70955,
    SPELL_MUTATED_PLAGUE                    = 72451,
    SPELL_MUTATED_PLAGUE_CLEAR              = 72618,
    SPELL_MUTATED_STRENGTH_NH               = 71603,
    SPELL_MUTATED_STRENGTH_HC               = 72461,

    // Slime Puddle
    SPELL_GROW_STACKER                      = 70345,
    SPELL_GROW                              = 70347,
    SPELL_SLIME_PUDDLE_AURA                 = 70343,

    // Gas Cloud
    SPELL_GASEOUS_BLOAT_PROC                = 70215,
    SPELL_GASEOUS_BLOAT                     = 70672,
    SPELL_GASEOUS_BLOAT_PROTECTION          = 70812,
    SPELL_EXPUNGED_GAS                      = 70701,

    // Volatile Ooze
    SPELL_OOZE_ERUPTION                     = 70492,
    SPELL_VOLATILE_OOZE_ADHESIVE            = 70447,
    SPELL_OOZE_ERUPTION_SEARCH_PERIODIC     = 70457,
    SPELL_VOLATILE_OOZE_PROTECTION          = 70530,

    // Choking Gas Bomb
    SPELL_CHOKING_GAS_BOMB_PERIODIC         = 71259,
    SPELL_CHOKING_GAS_EXPLOSION_TRIGGER     = 71280,

    // Mutated Abomination vehicle
    SPELL_ABOMINATION_VEHICLE_POWER_DRAIN   = 70385,
    SPELL_MUTATED_TRANSFORMATION            = 70311,
    SPELL_MUTATED_TRANSFORMATION_DAMAGE     = 70405,
    SPELL_MUTATED_TRANSFORMATION_NAME       = 72401,

    // Unholy Infusion
    SPELL_UNHOLY_INFUSION_CREDIT            = 71518,
    QUEST_UNHOLY_INFUSION                   = 24749,
    SPELL_SHADOW_INFUSION                   = 71516,

    // Misc
    SPELL_IMMUNITY                          = 7743,
    SPELL_ICE_BLOCK                         = 45438,
    SPELL_ROOT_SELF                         = 33356
};

enum Actions
{
    ACTION_STOP_GROW                        = 1,
    ACTION_UNROOT                           = 2,
};

#define SPELL_GASEOUS_BLOAT_HELPER RAID_MODE<uint32>(70672, 72455, 72832, 72833)

enum Events
{
    // Professor Putricide
    EVENT_BERSERK               = 6,    // all phases
    EVENT_SLIME_PUDDLE          = 7,    // P1 + P2
    EVENT_UNSTABLE_EXPERIMENT   = 8,    // P1 && P2
    EVENT_TEAR_GAS              = 9,    // phase transition not heroic
    EVENT_RESUME_ATTACK         = 10,
    EVENT_MALLEABLE_GOO         = 11,
    EVENT_CHOKING_GAS_BOMB      = 12,
    EVENT_UNBOUND_PLAGUE        = 13,
    EVENT_MUTATED_PLAGUE        = 14,
    EVENT_PHASE_TRANSITION      = 15,
    EVENT_SLIME_PUDDLEP3        = 16,   // slime in P3
    EVENT_SLIME_GROW            = 17,
    EVENT_SLIME_STOP_MOVING     = 18,
};

enum Phases : uint32
{
    PHASE_NONE          = 0,
    PHASE_COMBAT_1      = 1,
    PHASE_COMBAT_2      = 2,
    PHASE_COMBAT_3      = 3
};

enum Points
{
    POINT_TABLE     = 1
};

constexpr float SPEED_RATE = 1.14286f;
constexpr Position tablePos{ 4356.190f, 3262.90f, 389.4820f, 1.483530f };

enum PutricideData
{
    DATA_EXPERIMENT_STAGE                = 1,
    DATA_PHASE                           = 2,
    DATA_ABOMINATION                     = 3,
    DATA_VARIABLE                        = 5,
    DATA_OOZE_VARIABLE                   = 5
};

#define EXPERIMENT_STATE_OOZE   false
#define EXPERIMENT_STATE_GAS    true

class AbominationDespawner
{
    public:
        explicit AbominationDespawner(Unit* owner) : _owner(owner) { }

        bool operator()(ObjectGuid guid)
        {
            if (Unit* summon = ObjectAccessor::GetUnit(*_owner, guid))
            {
                if (summon->GetEntry() == NPC_MUTATED_ABOMINATION_10 || summon->GetEntry() == NPC_MUTATED_ABOMINATION_25)
                {
                    if (Vehicle* veh = summon->GetVehicleKit())
                        veh->RemoveAllPassengers(); // also despawns the vehicle

                    // Found unit is Mutated Abomination, remove it
                    return true;
                }

                // Found unit is not Mutated Abomintaion, leave it
                return false;
            }

            // No unit found, remove from SummonList
            return true;
        }

    private:
        Unit* _owner;
};

boss_professor_putricideAI::boss_professor_putricideAI(Creature* creature) : BossAI(creature, DATA_PROFESSOR_PUTRICIDE),
    _baseSpeed(SPEED_RATE), _experimentState(EXPERIMENT_STATE_OOZE)
{
    _phase = PHASE_NONE;
}

void boss_professor_putricideAI::Reset()
{
    BossAI::Reset();
    instance->SetData(DATA_NAUSEA_ACHIEVEMENT, uint32(true));
    me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_DECREASE_SPEED, true);

    VaribaleCounter = 0;
    _experimentState = EXPERIMENT_STATE_OOZE;
    me->SetReactState(REACT_AGGRESSIVE);
    me->SetWalk(false);
    me->RemoveAurasDueToSpell(SPELL_MUTATED_STRENGTH_NH);
    me->RemoveAurasDueToSpell(SPELL_MUTATED_STRENGTH_HC);
    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GAS_VARIABLE);
    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OOZE_VARIABLE);
    me->ApplySpellImmune(0, IMMUNITY_ID, RAID_MODE(70539, 72457, 72875, 72876), true); // Regurgitated Ooze
    if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
        me->GetMotionMaster()->MovementExpired();

    if (instance->GetBossState(DATA_ROTFACE) == DONE && instance->GetBossState(DATA_FESTERGUT) == DONE)
    {
        me->SetImmuneToPC(false);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }

    // Open the Door only if all Prebosses and the TrapEvent is Done
    if (instance->GetBossState(DATA_ROTFACE) == DONE && instance->GetBossState(DATA_FESTERGUT) == DONE && instance->GetData(DATA_PUTRICIDE_TRAP) != 0)
        instance->HandleGameObject(instance->GetGuidData(DATA_PUTRICIDE_DOOR), true);
}

void boss_professor_putricideAI::MoveInLineOfSight(Unit* who)
{
    if (me->IsValidAttackTarget(who) && me->IsWithinDistInMap(who, 100.0f))
    {
        if (instance->GetBossState(DATA_ROTFACE) == DONE && instance->GetBossState(DATA_FESTERGUT) == DONE)
        {
            me->SetImmuneToPC(false);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetReactState(REACT_AGGRESSIVE);
        }
    }
    ScriptedAI::MoveInLineOfSight(who);
}

void boss_professor_putricideAI::EnterCombat(Unit* who)
{
    BossAI::EnterCombat(who);

    events.Reset();
    events.ScheduleEvent(EVENT_BERSERK, 600000);
    events.ScheduleEvent(EVENT_SLIME_PUDDLE, 10000);
    events.ScheduleEvent(EVENT_UNSTABLE_EXPERIMENT, urand(30000, 35000));
    if (IsHeroic())
        events.ScheduleEvent(EVENT_UNBOUND_PLAGUE, 20000);

    me->SetSpeedRate(MOVE_RUN, SPEED_RATE);
    SetPhase(PHASE_COMBAT_1);
    Talk(SAY_AGGRO);
    DoCastSelf(SPELL_OOZE_TANK_PROTECTION, true);

    instance->HandleGameObject(instance->GetGuidData(DATA_PUTRICIDE_DOOR), false);
}

void boss_professor_putricideAI::EnterEvadeMode(EvadeReason /*why*/)
{
    _DespawnAtEvade();
}

void boss_professor_putricideAI::KilledUnit(Unit* victim)
{
    if (victim->GetTypeId() == TYPEID_PLAYER)
        Talk(SAY_KILL);
}

void boss_professor_putricideAI::JustDied(Unit* killer)
{
    BossAI::JustDied(killer);
    Talk(SAY_DEATH);
    me->RemoveAurasDueToSpell(SPELL_MUTATED_STRENGTH_NH);
    me->RemoveAurasDueToSpell(SPELL_MUTATED_STRENGTH_HC);
    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GAS_VARIABLE);
    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OOZE_VARIABLE);

    if (IsHeroic())
    {
        if (instance->GetData(DATA_PROFESSOR_PUTRICIDE_HC) == 0)
            instance->SetData(DATA_PROFESSOR_PUTRICIDE_HC, 1);

        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNBOUND_PLAGUE);
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_UNBOUND_PLAGUE_SEARCHER);
    }

    if (Is25ManRaid() && me->HasAura(SPELL_SHADOWS_FATE))
        DoCastAOE(SPELL_UNHOLY_INFUSION_CREDIT, true); // ReqTargetAura in dbc

    DoCast(SPELL_MUTATED_PLAGUE_CLEAR);

    // Open the Door only if all Prebosses and the TrapEvent is Done
    if (instance->GetBossState(DATA_ROTFACE) == DONE && instance->GetBossState(DATA_FESTERGUT) == DONE && instance->GetData(DATA_PUTRICIDE_TRAP) != 0)
        instance->HandleGameObject(instance->GetGuidData(DATA_PUTRICIDE_DOOR), true);

    // Despawn all Puddles
    std::list<Creature*> addList;
    me->GetCreatureListWithEntryInGrid(addList, NPC_GROWING_OOZE_PUDDLE, 1000.0f);
    for (Creature* puddle : addList)
        puddle->DespawnOrUnsummon();
}

void boss_professor_putricideAI::JustSummoned(Creature* summon)
{
    summons.Summon(summon);
    switch (summon->GetEntry())
    {
        case NPC_GROWING_OOZE_PUDDLE:
            summon->SetDisplayId(summon->GetCreatureTemplate()->Modelid1);
            summon->CastSpell(summon, SPELL_SLIME_PUDDLE_AURA, true);
            // blizzard casts this spell 7 times initially (confirmed in sniff)
            for (uint8 i = 0; i < 7; ++i)
                summon->CastSpell(summon, SPELL_GROW, true);
            break;
        case NPC_GAS_CLOUD:
            // no possible aura seen in sniff adding the aurastate
            summon->ModifyAuraState(AURA_STATE_UNKNOWN22, true);
            summon->CastSpell(summon, SPELL_GASEOUS_BLOAT_PROC, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
            summon->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_STUN, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SILENCE, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SLEEP, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_BANISH, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_KNOCKOUT, true);
            summon->SetReactState(REACT_AGGRESSIVE);
            break;
        case NPC_VOLATILE_OOZE:
            // no possible aura seen in sniff adding the aurastate
            summon->ModifyAuraState(AURA_STATE_UNKNOWN19, true);
            summon->CastSpell(summon, SPELL_OOZE_ERUPTION_SEARCH_PERIODIC, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);
            summon->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_STUN, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SILENCE, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SLEEP, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_BANISH, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, true);
            summon->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_KNOCKOUT, true);
            summon->SetReactState(REACT_AGGRESSIVE);
            break;
        case NPC_CHOKING_GAS_BOMB:
            return;
        case NPC_MUTATED_ABOMINATION_10:
        case NPC_MUTATED_ABOMINATION_25:
            return;
        default:
            break;
    }

    if (me->IsInCombat())
        DoZoneInCombat(summon);
}

void boss_professor_putricideAI::DamageTaken(Unit* /*attacker*/, uint32& /*damage*/)
{
    switch (_phase)
    {
        case PHASE_COMBAT_1:
            if (HealthAbovePct(80))
                return;
            me->SetReactState(REACT_PASSIVE);
            DoAction(ACTION_CHANGE_PHASE);
            StopOozeGrow();
            break;
        case PHASE_COMBAT_2:
            if (HealthAbovePct(35))
                return;
            me->SetReactState(REACT_PASSIVE);
            DoAction(ACTION_CHANGE_PHASE);
            StopOozeGrow();
            break;
        default:
            break;
    }
}

void boss_professor_putricideAI::MovementInform(uint32 type, uint32 id)
{
    if (type != POINT_MOTION_TYPE || id != POINT_TABLE)
        return;

    // stop attack
    me->GetMotionMaster()->MoveIdle();
    me->SetSpeedRate(MOVE_RUN, _baseSpeed);
    if (GameObject* table = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(DATA_PUTRICIDE_TABLE)))
        me->SetFacingToObject(table);
    // operating on new phase already
    switch (_phase)
    {
        case PHASE_COMBAT_2:
        {
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(SPELL_CREATE_CONCOCTION);
            DoCastSelf(SPELL_CREATE_CONCOCTION);
            events.ScheduleEvent(EVENT_PHASE_TRANSITION, sSpellMgr->GetSpellForDifficultyFromSpell(spell, me)->CalcCastTime() + 100);
            break;
        }
        case PHASE_COMBAT_3:
        {
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(SPELL_GUZZLE_POTIONS);
            DoCastSelf(SPELL_GUZZLE_POTIONS);
            events.ScheduleEvent(EVENT_PHASE_TRANSITION, sSpellMgr->GetSpellForDifficultyFromSpell(spell, me)->CalcCastTime() + 100);
            break;
        }
        default:
            break;
    }
}

void boss_professor_putricideAI::StopOozeGrow()
{
    // delay Puddlegrow
    std::list<Creature*> puddleList;
    me->GetCreatureListWithEntryInGrid(puddleList, NPC_GROWING_OOZE_PUDDLE, 1000.0f);
    for (Creature* puddle : puddleList)
        puddle->AI()->DoAction(ACTION_STOP_GROW);
}

void boss_professor_putricideAI::DoAction(int32 action)
{
    switch (action)
    {
        case ACTION_CHANGE_PHASE:
            me->SetSpeedRate(MOVE_RUN, _baseSpeed*2.0f);
            events.DelayEvents(30000);
            events.CancelEvent(EVENT_UNBOUND_PLAGUE);
            events.CancelEvent(EVENT_SLIME_PUDDLE);
            events.CancelEvent(EVENT_MALLEABLE_GOO);
            events.CancelEvent(EVENT_CHOKING_GAS_BOMB);
            events.CancelEvent(EVENT_UNSTABLE_EXPERIMENT);
            me->AttackStop();
            if (!IsHeroic())
            {
                me->CastStop();
                DoCastSelf(SPELL_TEAR_GAS);
                events.ScheduleEvent(EVENT_TEAR_GAS, 2500);
            }
            else
            {
                me->CastStop();
                VaribaleCounter = 0;
                Talk(SAY_PHASE_TRANSITION_HEROIC);
                DoCastSelf(SPELL_UNSTABLE_EXPERIMENT, true);
                DoCastSelf(SPELL_UNSTABLE_EXPERIMENT, true);
                // cast variables
                if (Is25ManRaid())
                {
                    std::list<Unit*> targetList;

                    for (Player& player : me->GetMap()->GetPlayers())
                        if (!player.IsGameMaster() && !player.HasAura(SPELL_MUTATED_TRANSFORMATION_DAMAGE) && !player.HasAura(VEHICLE_SPELL_RIDE_HARDCODED))
                            targetList.push_back(&player);

                    size_t half = targetList.size()/2;
                    // half gets ooze variable
                    while (half < targetList.size())
                    {
                        std::list<Unit*>::iterator itr = targetList.begin();
                        advance(itr, urand(0, targetList.size() - 1));
                        (*itr)->CastSpell(*itr, SPELL_OOZE_VARIABLE, true);
                        targetList.erase(itr);
                    }
                    // and half gets gas
                    for (std::list<Unit*>::iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        (*itr)->CastSpell(*itr, SPELL_GAS_VARIABLE, true);
                }
                me->GetMotionMaster()->MovePoint(POINT_TABLE, tablePos);
            }
            switch (_phase)
            {
                case PHASE_COMBAT_1:
                    SetPhase(PHASE_COMBAT_2);
                    break;
                case PHASE_COMBAT_2:
                    SetPhase(PHASE_COMBAT_3);
                    events.ScheduleEvent(EVENT_MUTATED_PLAGUE, 25000);
                    events.CancelEvent(EVENT_UNSTABLE_EXPERIMENT);
                    events.CancelEvent(EVENT_SLIME_PUDDLE);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

uint32 boss_professor_putricideAI::GetData(uint32 type) const
{
    switch (type)
    {
        case DATA_EXPERIMENT_STAGE:
            return _experimentState;
        case DATA_PHASE:
            return _phase;
        case DATA_ABOMINATION:
            return uint32(summons.HasEntry(NPC_MUTATED_ABOMINATION_10) || summons.HasEntry(NPC_MUTATED_ABOMINATION_25));
        default:
            break;
    }

    return 0;
}

void boss_professor_putricideAI::SetData(uint32 id, uint32 data)
{
    if (id == DATA_EXPERIMENT_STAGE)
        _experimentState = data != 0;

    if (id == DATA_VARIABLE && data == DATA_OOZE_VARIABLE)
        ++VaribaleCounter;
}

void boss_professor_putricideAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
        return;

    if (VaribaleCounter == 2)
    {
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_GAS_VARIABLE);
        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OOZE_VARIABLE);
        VaribaleCounter = 0;
    }

    events.Update(diff);

    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    while (uint32 eventId = events.ExecuteEvent())
    {
        switch (eventId)
        {
            case EVENT_BERSERK:
                Talk(SAY_BERSERK);
                DoCastSelf(SPELL_BERSERK2);
                break;
            case EVENT_SLIME_PUDDLE:
            {
                std::list<Unit*> targets;
                SelectTargetList(targets, 2, SELECT_TARGET_RANDOM, 0, 0.0f, true);
                for (auto target : targets)
                    DoCast(target, SPELL_SLIME_PUDDLE_TRIGGER);
                events.ScheduleEvent(EVENT_SLIME_PUDDLE, 35000);
                break;
            }
            case EVENT_SLIME_PUDDLEP3:
            {
                std::list<Unit*> targets;
                SelectTargetList(targets, 2, SELECT_TARGET_RANDOM, 0, 0.0f, true);
                for (auto target : targets)
                    DoCast(target, SPELL_SLIME_PUDDLE_TRIGGER);
                events.ScheduleEvent(EVENT_SLIME_PUDDLEP3, 20000);
                break;
            }
            case EVENT_UNSTABLE_EXPERIMENT:
                Talk(EMOTE_UNSTABLE_EXPERIMENT);
                DoCastSelf(SPELL_UNSTABLE_EXPERIMENT);
                events.ScheduleEvent(EVENT_UNSTABLE_EXPERIMENT, urand(35000, 40000));
                break;
            case EVENT_TEAR_GAS:
                me->GetMotionMaster()->MovePoint(POINT_TABLE, tablePos);
                DoCastSelf(SPELL_TEAR_GAS_PERIODIC_TRIGGER, true);
                break;
            case EVENT_RESUME_ATTACK:
                me->SetReactState(REACT_DEFENSIVE);
                AttackStart(me->GetVictim());
                // remove Tear Gas
                me->RemoveAurasDueToSpell(SPELL_TEAR_GAS_PERIODIC_TRIGGER);
                instance->DoRemoveAurasDueToSpellOnPlayers(71615);
                DoCastAOE(SPELL_TEAR_GAS_CANCEL);
                if (IsHeroic())
                    events.RescheduleEvent(EVENT_UNBOUND_PLAGUE,   61 * IN_MILLISECONDS);
                if (events.IsInPhase(PHASE_COMBAT_2))
                {
                    events.RescheduleEvent(EVENT_SLIME_PUDDLE,     10 * IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_MALLEABLE_GOO,    16 * IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_CHOKING_GAS_BOMB, 18 * IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_UNSTABLE_EXPERIMENT, 21 * IN_MILLISECONDS);
                }
                if (events.IsInPhase(PHASE_COMBAT_3))
                {
                    events.RescheduleEvent(EVENT_MALLEABLE_GOO,    10 * IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_CHOKING_GAS_BOMB, 18 * IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_SLIME_PUDDLEP3,   20 * IN_MILLISECONDS);
                }
                break;
            case EVENT_MALLEABLE_GOO:
            {
                const uint32 required_count = RAID_MODE<uint32>(3, 8, 3, 8);
                const uint32 target_count = RAID_MODE<uint32>(1, 2, 1, 3);

                std::list<Unit*> targets;

                // check if there are enough range targets
                SelectTargetList(targets, required_count, SELECT_TARGET_RANDOM, 0, -15.0f, true);
                if (targets.size() < required_count)
                {
                    // fallback to meele targets (without tank)
                    targets.clear();
                    SelectTargetList(targets, required_count, SELECT_TARGET_RANDOM, 0, 0.0f, true, false);
                }
                if (!targets.empty())
                {
                    Trinity::Containers::RandomResize(targets, target_count);

                    Talk(EMOTE_MALLEABLE_GOO);
                    for (auto target : targets)
                        DoCast(target, SPELL_MALLEABLE_GOO);
                }

                events.ScheduleEvent(EVENT_MALLEABLE_GOO, urand(25000, 30000));
                break;
            }
            case EVENT_CHOKING_GAS_BOMB:
                Talk(EMOTE_CHOKING_GAS_BOMB);
                DoCastSelf(SPELL_CHOKING_GAS_BOMB);
                events.ScheduleEvent(EVENT_CHOKING_GAS_BOMB, urand(35000, 40000));
                break;
            case EVENT_UNBOUND_PLAGUE:
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 45.f, true, false))
                {
                    DoCast(target, SPELL_UNBOUND_PLAGUE);
                    DoCast(target, SPELL_UNBOUND_PLAGUE_SEARCHER);
                }
                if (events.IsInPhase(PHASE_COMBAT_3))
                    events.ScheduleEvent(EVENT_UNBOUND_PLAGUE, 40 * IN_MILLISECONDS);
                else
                    events.ScheduleEvent(EVENT_UNBOUND_PLAGUE, 61 * IN_MILLISECONDS);
                break;
            }
            case EVENT_MUTATED_PLAGUE:
                DoCastVictim(SPELL_MUTATED_PLAGUE);
                events.ScheduleEvent(EVENT_MUTATED_PLAGUE, 10000);
                break;
            case EVENT_PHASE_TRANSITION:
            {
                switch (_phase)
                {
                    case PHASE_COMBAT_2:
                        if (Creature* face = me->FindNearestCreature(NPC_TEAR_GAS_TARGET_STALKER, 50.0f))
                            me->SetFacingToObject(face);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
                        Talk(SAY_TRANSFORM_1);
                        events.ScheduleEvent(EVENT_RESUME_ATTACK, 15*IN_MILLISECONDS, 0, PHASE_COMBAT_2);
                        events.CancelEvent(EVENT_UNSTABLE_EXPERIMENT);
                        break;
                    case PHASE_COMBAT_3:
                        if (Creature* face = me->FindNearestCreature(NPC_TEAR_GAS_TARGET_STALKER, 50.0f))
                            me->SetFacingToObject(face);
                        me->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
                        Talk(SAY_TRANSFORM_2);
                        summons.DespawnIf(AbominationDespawner(me));
                        events.ScheduleEvent(EVENT_RESUME_ATTACK, 15*IN_MILLISECONDS, 0, PHASE_COMBAT_3);
                        break;
                    default:
                        break;
                }
            }
            default:
                break;
        }
    }

    DoMeleeAttackIfReady(true);
}

void boss_professor_putricideAI::SetPhase(Phases newPhase)
{
    _phase = newPhase;
    events.SetPhase(newPhase);
}

class npc_putricide_oozeAI : public ScriptedAI
{
    public:
        npc_putricide_oozeAI(Creature* creature, uint32 hitTargetSpellId) : ScriptedAI(creature),
            _hitTargetSpellId(hitTargetSpellId), _newTargetSelectTimer(0)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            SelectNewTarget();
        }

        void SpellHitTarget(Unit* /*target*/, SpellInfo const* spell)
        {
            if (!_newTargetSelectTimer && spell->Id == sSpellMgr->GetSpellIdForDifficulty(_hitTargetSpellId, me))
            {
                _newTargetSelectTimer = 1000;
                _targetGUID.Clear();
            }
        }

        void SelectNewTarget()
        {
            _targetGUID.Clear();
            me->InterruptNonMeleeSpells(true);
            me->AttackStop();
            me->GetMotionMaster()->Clear();
            DoCastSelf(SPELL_ROOT_SELF, true);
            DoAction(ACTION_UNROOT);
            _newTargetSelectTimer = 1000;
        }

        void IsSummonedBy(Unit* /*summoner*/)
        {
            if (InstanceScript* instance = me->GetInstanceScript())
                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
                {
                    if (!professor->IsInCombat())
                        me->DespawnOrUnsummon(1);
                    else
                        professor->AI()->JustSummoned(me);
                }
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_UNROOT)
                _unrootTimer = 5 * IN_MILLISECONDS;
        }

        void SpellHit(Unit* /*caster*/, SpellInfo const* spell)
        {
            if (spell->Id == SPELL_TEAR_GAS_CREATURE)
                SelectNewTarget();
        }

        void SetGuidData(ObjectGuid guid, uint32 /*id*/ = 0) override
        {
            _targetGUID = guid;
        }

        void JustDied(Unit* /*killer*/)
        {
            if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
                professor->AI()->SetData(DATA_VARIABLE, DATA_OOZE_VARIABLE);
        }

        void UpdateAI(uint32 diff)
        {
            if (_unrootTimer <= diff)
                me->RemoveAurasDueToSpell(SPELL_ROOT_SELF);
            else
                _unrootTimer -= diff;

            if (!_newTargetSelectTimer)
            {
                if ((!me->HasUnitState(UNIT_STATE_CASTING) && !me->GetVictim()) || !me->IsNonMeleeSpellCast(false, false, true, false, true))
                    SelectNewTarget();
                else if (_targetGUID)
                {
                    Unit* target = ObjectAccessor::GetUnit(*me, _targetGUID);
                    if (!me->GetVictim() || me->GetVictim()->GetGUID() != _targetGUID || !target || !me->IsValidAttackTarget(target) || target->isDead() || target->HasAuraType(SPELL_AURA_SCHOOL_IMMUNITY))
                        SelectNewTarget();
                }
            }

            if (me->GetEntry() == NPC_GAS_CLOUD)
                DoMeleeAttackIfReady();

            if (!_newTargetSelectTimer)
                return;

            if (me->HasAura(SPELL_TEAR_GAS_CREATURE))
                return;

            if (_newTargetSelectTimer <= diff)
            {
                _newTargetSelectTimer = 0;
                CastMainSpell();
            }
            else
                _newTargetSelectTimer -= diff;
        }

        virtual void CastMainSpell() = 0;

    private:
        ObjectGuid _targetGUID;
        uint32 _hitTargetSpellId;
        uint32 _newTargetSelectTimer;
        uint32 _unrootTimer;
        InstanceScript* instance;
};

class npc_volatile_ooze : public CreatureScript
{
    public:
        npc_volatile_ooze() : CreatureScript("npc_volatile_ooze") { }

        struct npc_volatile_oozeAI : public npc_putricide_oozeAI
        {
            npc_volatile_oozeAI(Creature* creature) : npc_putricide_oozeAI(creature, SPELL_OOZE_ERUPTION) { }

            void CastMainSpell()
            {
                DoCastSelf(SPELL_ROOT_SELF, true);
                DoAction(ACTION_UNROOT);
                DoCastSelf(SPELL_VOLATILE_OOZE_ADHESIVE, false);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_volatile_oozeAI>(creature);
        }
};

class npc_gas_cloud : public CreatureScript
{
    public:
        npc_gas_cloud() : CreatureScript("npc_gas_cloud") { }

        struct npc_gas_cloudAI : public npc_putricide_oozeAI
        {
            npc_gas_cloudAI(Creature* creature) : npc_putricide_oozeAI(creature, SPELL_EXPUNGED_GAS)
            {
                _newTargetSelectTimer = 0;
                instance = creature->GetInstanceScript();
            }

            void CastMainSpell()
            {
                DoCastSelf(SPELL_ROOT_SELF, true);
                DoAction(ACTION_UNROOT);
                if (Creature* professor = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE)))
                    if (Unit* target = professor->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 200.f, true, false))
                        me->CastCustomSpell(SPELL_GASEOUS_BLOAT, SPELLVALUE_AURA_STACK, 10, target, false);
            }

        private:
            uint32 _newTargetSelectTimer;
            InstanceScript* instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_gas_cloudAI>(creature);
        }
};

class npc_chocking_gas_bomb : public CreatureScript
{
    public:
        npc_chocking_gas_bomb() : CreatureScript("npc_chocking_gas_bomb") { }

        struct npc_chocking_gas_bombAI : public ScriptedAI
        {
            npc_chocking_gas_bombAI(Creature* creature) : ScriptedAI(creature)
            {
                SetCombatMovement(false);
            }

            void Reset()
            {
                chockingGasTimer = 1500;
                DoCastSelf(SPELL_CHOKING_GAS_EXPLOSION_TRIGGER, true);
            }

            void UpdateAI(uint32 diff) override
            {
                if (chockingGasTimer <= diff)
                {
                    DoCastSelf(SPELL_CHOKING_GAS_BOMB_PERIODIC, true);
                    chockingGasTimer = 10000;
                }
                else chockingGasTimer -= diff;
            }

        private:
            uint32 chockingGasTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_chocking_gas_bombAI(creature);
        }
};

class npc_ooze_puddle : public CreatureScript
{
    public:
        npc_ooze_puddle() : CreatureScript("npc_ooze_puddle") { }

        struct npc_ooze_puddleAI : public ScriptedAI
        {
            npc_ooze_puddleAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset()
            {
                _events.Reset();
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                _events.ScheduleEvent(EVENT_SLIME_GROW, 1.5*IN_MILLISECONDS);
                if (me->GetPositionZ() < 389.3f) // If the Puddle spawns under the floor move them to visible height
                {
                    me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), 389.4f, me->GetOrientation(), false);
                    _events.ScheduleEvent(EVENT_SLIME_STOP_MOVING, 0.5*IN_MILLISECONDS); // In Case Slime is Moving after Teleport
                }
            }

            void DoAction(int32 action)
            {
                if (action != ACTION_STOP_GROW)
                    return;
                _events.CancelEvent(EVENT_SLIME_GROW);
                _events.ScheduleEvent(EVENT_SLIME_GROW, 23*IN_MILLISECONDS);
            }

            void UpdateAI(uint32 diff)
            {

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SLIME_GROW:
                            DoCastSelf(SPELL_GROW);
                            if (!me->FindNearestCreature(NPC_PROFESSOR_PUTRICIDE, 1000.0f))
                                me->DespawnOrUnsummon();
                            _events.ScheduleEvent(EVENT_SLIME_GROW, 1.8*IN_MILLISECONDS);
                            break;
                        case EVENT_SLIME_STOP_MOVING:
                            SetCombatMovement(false);
                            me->GetMotionMaster()->Clear();
                            me->SetReactState(REACT_PASSIVE);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_ooze_puddleAI(creature);
        }
};

class npc_mutated_abomination : public CreatureScript
{
    public:
        npc_mutated_abomination() : CreatureScript("npc_mutated_abomination") { }

        struct npc_mutated_abominationAI : public VehicleAI
        {
            npc_mutated_abominationAI(Creature* creature) : VehicleAI(creature) { }

            void PassengerBoarded(Unit* unit, int8 /*seat*/, bool apply) override
            {
                if (unit->GetTypeId() == TYPEID_PLAYER)
                    if (apply)
                        unit->AddAura(SPELL_IMMUNITY, unit);
                    if (!apply)
                        unit->RemoveAurasDueToSpell(SPELL_IMMUNITY, unit->GetGUID());
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mutated_abominationAI(creature);
        }
};

class spell_putricide_gaseous_bloat : public SpellScriptLoader
{
    public:
        spell_putricide_gaseous_bloat() : SpellScriptLoader("spell_putricide_gaseous_bloat") { }

        class spell_putricide_gaseous_bloat_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_putricide_gaseous_bloat_AuraScript);

            void HandleExtraEffect(AuraEffect const* /*aurEff*/)
            {
                Unit* target = GetTarget();
                if (Unit* caster = GetCaster())
                {
                    if (AuraEffect* effect = GetAura()->GetEffect(EFFECT_0))
                        effect->RecalculateAmount(caster);

                    target->RemoveAuraFromStack(GetSpellInfo()->Id, GetCasterGUID());
                    if (!target->HasAura(GetId()))
                    {
                        caster->GetMotionMaster()->Clear(true);
                        caster->GetMotionMaster()->MoveIdle();
                        caster->CastSpell(caster, SPELL_ROOT_SELF, true);
                        caster->ToCreature()->AI()->DoAction(ACTION_UNROOT);
                        if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                        { 
                            if (Creature* professor = ObjectAccessor::GetCreature(*caster, ObjectGuid(instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE))))
                                if (Unit* target = professor->AI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 200.f, true, false))
                                    caster->CastCustomSpell(SPELL_GASEOUS_BLOAT, SPELLVALUE_AURA_STACK, 10, target, false);
                        }
                    }
                }
            }

            void CalculateBonus(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
            {
                canBeRecalculated = true;
            }

            void OnProc(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
            {
                PreventDefaultAction();

                uint8 stacks = GetStackAmount();
                int32 const mod = (GetTarget()->GetMap()->GetSpawnMode() & 1) ? 1500 : 1250;
                int32 dmg = 0;
                for (uint8 i = 1; i <= stacks; ++i)
                    dmg += mod * i;
                if (Unit* caster = GetCaster())
                    caster->CastCustomSpell(SPELL_EXPUNGED_GAS, SPELLVALUE_BASE_POINT0, dmg);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_putricide_gaseous_bloat_AuraScript::HandleExtraEffect, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_putricide_gaseous_bloat_AuraScript::CalculateBonus, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectProc += AuraEffectProcFn(spell_putricide_gaseous_bloat_AuraScript::OnProc, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_putricide_gaseous_bloat_AuraScript();
        }
};

class spell_putricide_ooze_channel : public SpellScriptLoader
{
    public:
        spell_putricide_ooze_channel() : SpellScriptLoader("spell_putricide_ooze_channel") { }

        class spell_putricide_ooze_channel_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_ooze_channel_SpellScript);

            bool Validate(SpellInfo const* spell)
            {
                if (!spell->ExcludeTargetAuraSpell)
                    return false;
                if (!sSpellMgr->GetSpellInfo(spell->ExcludeTargetAuraSpell))
                    return false;
                return true;
            }

            // set up initial variables and check if caster is creature
            // this will let use safely use ToCreature() casts in entire script
            bool Load()
            {
                _target = NULL;
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void SelectTarget(std::list<WorldObject*>& targets)
            {
                // All players protected
                if (targets.empty())
                {
                    // Ignore protection
                    for (Player& player : GetCaster()->GetMap()->GetPlayers())
                        if (!player.IsGameMaster() && !player.isDead() && !player.HasAura(SPELL_MUTATED_TRANSFORMATION_DAMAGE) && !player.HasAura(VEHICLE_SPELL_RIDE_HARDCODED))
                            targets.push_back(&player);
                    // No players alive
                    if (targets.empty())
                    {
                        FinishCast(SPELL_FAILED_NO_VALID_TARGETS);
                        GetCaster()->ToCreature()->DespawnOrUnsummon(1);    // despawn next update
                        return;
                    }
                }

                // Select any target as fallback
                WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targets);

                // dbc has only 1 field for excluding, this will prevent anyone from getting both at the same time
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_VOLATILE_OOZE_PROTECTION));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_GASEOUS_BLOAT_PROTECTION));

                // Select a target without protection
                if (!targets.empty())
                    target = Trinity::Containers::SelectRandomContainerElement(targets);

                targets.clear();
                targets.push_back(target);
                _target = target;
            }

            void SetTarget(std::list<WorldObject*>& targets)
            {
                targets.clear();
                if (_target)
                    targets.push_back(_target);
            }

            void StartAttack()
            {
                GetCaster()->ToCreature()->AI()->SetGuidData(GetHitUnit()->GetGUID());
                GetCaster()->ClearUnitState(UNIT_STATE_CASTING);
                GetCaster()->GetThreatManager().ClearAllThreat();
                GetCaster()->ToCreature()->AI()->AttackStart(GetHitUnit());
                GetCaster()->GetThreatManager().AddThreat(GetHitUnit(), 500000000.0f); // value seen in sniff
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_ooze_channel_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_ooze_channel_SpellScript::SetTarget, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_ooze_channel_SpellScript::SetTarget, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
                AfterHit += SpellHitFn(spell_putricide_ooze_channel_SpellScript::StartAttack);
            }

            WorldObject* _target;
        };

        class spell_putricide_ooze_channel_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_putricide_ooze_channel_AuraScript);

            void HandleTriggerSpell(AuraEffect const* /*aurEff*/)
            {
                if (GetTarget()->HasAura(SPELL_ICE_BLOCK))
                    PreventDefaultAction();
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_putricide_ooze_channel_AuraScript::HandleTriggerSpell, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_ooze_channel_SpellScript();
        }

        AuraScript* GetAuraScript() const
        {
            return new spell_putricide_ooze_channel_AuraScript();
        }
};

class ExactDistanceCheck
{
    public:
        ExactDistanceCheck(Unit* source, float dist) : _source(source), _dist(dist) {}

        bool operator()(WorldObject* unit) const
        {
            return _source->GetExactDist2d(unit) > _dist;
        }

    private:
        Unit* _source;
        float _dist;
};

class spell_putricide_slime_puddle : public SpellScriptLoader
{
    public:
        spell_putricide_slime_puddle() : SpellScriptLoader("spell_putricide_slime_puddle") { }

        class spell_putricide_slime_puddle_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_slime_puddle_SpellScript);

            void ScaleRange(std::list<WorldObject*>& targets)
            {
                targets.remove_if(ExactDistanceCheck(GetCaster(), 2.5f * GetCaster()->GetFloatValue(OBJECT_FIELD_SCALE_X)));
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_slime_puddle_SpellScript::ScaleRange, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_slime_puddle_SpellScript::ScaleRange, EFFECT_1, TARGET_UNIT_DEST_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_slime_puddle_SpellScript();
        }
};

// this is here only because on retail you dont actually enter HEROIC mode for ICC
class spell_putricide_slime_puddle_aura : public SpellScriptLoader
{
    public:
        spell_putricide_slime_puddle_aura() : SpellScriptLoader("spell_putricide_slime_puddle_aura") { }

        class spell_putricide_slime_puddle_aura_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_slime_puddle_aura_SpellScript);

            void ReplaceAura()
            {
                if (Unit* target = GetHitUnit())
                    GetCaster()->AddAura((GetCaster()->GetMap()->GetSpawnMode() & 1) ? 72456 : 70346, target);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_putricide_slime_puddle_aura_SpellScript::ReplaceAura);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_slime_puddle_aura_SpellScript();
        }
};

class spell_putricide_unstable_experiment : public SpellScriptLoader
{
    public:
        spell_putricide_unstable_experiment() : SpellScriptLoader("spell_putricide_unstable_experiment") { }

        class spell_putricide_unstable_experiment_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_unstable_experiment_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (GetCaster()->GetTypeId() != TYPEID_UNIT)
                    return;

                Creature* creature = GetCaster()->ToCreature();

                uint32 stage = creature->AI()->GetData(DATA_EXPERIMENT_STAGE);
                creature->AI()->SetData(DATA_EXPERIMENT_STAGE, stage ^ true);

                Creature* target = NULL;
                std::list<Creature*> creList;
                GetCaster()->GetCreatureListWithEntryInGrid(creList, NPC_ABOMINATION_WING_MAD_SCIENTIST_STALKER, 200.0f);
                // 2 of them are spawned at green place - weird trick blizz
                for (std::list<Creature*>::iterator itr = creList.begin(); itr != creList.end(); ++itr)
                {
                    target = *itr;
                    std::list<Creature*> tmp;
                    target->GetCreatureListWithEntryInGrid(tmp, NPC_ABOMINATION_WING_MAD_SCIENTIST_STALKER, 10.0f);
                    if ((!stage && tmp.size() > 1) || (stage && tmp.size() == 1))
                        break;
                }

                GetCaster()->CastSpell(target, uint32(GetSpellInfo()->Effects[stage].CalcValue()), true, NULL, NULL, GetCaster()->GetGUID());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_putricide_unstable_experiment_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_unstable_experiment_SpellScript();
        }
};

class spell_putricide_ooze_eruption_searcher : public SpellScriptLoader
{
    public:
        spell_putricide_ooze_eruption_searcher() : SpellScriptLoader("spell_putricide_ooze_eruption_searcher") { }

        class spell_putricide_ooze_eruption_searcher_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_ooze_eruption_searcher_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                uint32 adhesiveId = sSpellMgr->GetSpellIdForDifficulty(SPELL_VOLATILE_OOZE_ADHESIVE, GetCaster());
                if (GetHitUnit()->HasAura(adhesiveId))
                {
                    GetHitUnit()->RemoveAurasDueToSpell(adhesiveId, GetCaster()->GetGUID(), 0, AURA_REMOVE_BY_ENEMY_SPELL);
                    GetCaster()->CastSpell(GetHitUnit(), SPELL_OOZE_ERUPTION, true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_putricide_ooze_eruption_searcher_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_ooze_eruption_searcher_SpellScript();
        }
};

class spell_putricide_choking_gas_bomb : public SpellScriptLoader
{
    public:
        spell_putricide_choking_gas_bomb() : SpellScriptLoader("spell_putricide_choking_gas_bomb") { }

        class spell_putricide_choking_gas_bomb_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_choking_gas_bomb_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                uint32 skipIndex = urand(0, 2);
                for (uint32 i = 0; i < 3; ++i)
                {
                    if (i == skipIndex)
                        continue;

                    uint32 spellId = uint32(GetSpellInfo()->Effects[i].CalcValue());
                    GetCaster()->CastSpell(GetCaster(), spellId, true, NULL, NULL, GetCaster()->GetGUID());
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_putricide_choking_gas_bomb_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_choking_gas_bomb_SpellScript();
        }
};


///! Unbound Plague: Script handles spreading of the plague to nearby players
class spell_putricide_unbound_plague : public SpellScriptLoader
{
    public:
        spell_putricide_unbound_plague() : SpellScriptLoader("spell_putricide_unbound_plague") { }

        class spell_putricide_unbound_plague_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_unbound_plague_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_UNBOUND_PLAGUE))
                    return false;
                if (!sSpellMgr->GetSpellInfo(SPELL_UNBOUND_PLAGUE_SEARCHER))
                    return false;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (AuraEffect const* eff = GetCaster()->GetAuraEffect(SPELL_UNBOUND_PLAGUE_SEARCHER, EFFECT_0))
                {
                    if (eff->GetTickNumber() < 2)
                    {
                        targets.clear();
                        return;
                    }
                }

                targets.remove_if([](WorldObject* obj) { return obj->GetTypeId() != TYPEID_PLAYER; });
                targets.remove_if(Trinity::UnitAuraCheck(true, sSpellMgr->GetSpellIdForDifficulty(SPELL_UNBOUND_PLAGUE, GetCaster())));
                Trinity::Containers::RandomResize(targets, 1);
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* oldPlagueTarget = GetCaster();
                Unit* newPlagueTarget = GetHitUnit();
                if (!oldPlagueTarget || !newPlagueTarget)
                    return;

                uint32 plagueId = sSpellMgr->GetSpellIdForDifficulty(SPELL_UNBOUND_PLAGUE, oldPlagueTarget);

                if (!newPlagueTarget->HasAura(plagueId))
                {
                    if (Aura* oldPlague = oldPlagueTarget->GetAura(plagueId))
                    {
                        if (Unit* plagueStarter = oldPlague->GetCaster())
                        {
                            if (Aura* newPlague = plagueStarter->AddAura(plagueId, newPlagueTarget))
                            {
                                newPlague->SetMaxDuration(oldPlague->GetMaxDuration());
                                newPlague->SetDuration(oldPlague->GetDuration());
                                // The periodic timer should begin where it was at the last target
                                newPlague->GetEffect(0)->SetPeriodicTimer(oldPlague->GetEffect(0)->GetPeriodicTimer());
                                oldPlague->Remove();

                                oldPlagueTarget->CastSpell(oldPlagueTarget, SPELL_PLAGUE_SICKNESS, true);
                                oldPlagueTarget->AddAura(SPELL_UNBOUND_PLAGUE_PROTECTION, oldPlagueTarget);

                                // The last owner of the plague, the one before oldPlagueTarget should no longer be
                                // protected from the plague. Only one target at the time should be protected
                                // To be able to refind the last owner, the old plague target itselfs
                                // cast SPELL_UNBOUND_PLAGUE_SEARCHER on the new target
                                // and we can refind him by getting the caster of the searcher on the new target.
                                oldPlagueTarget->CastSpell(GetHitUnit(), SPELL_UNBOUND_PLAGUE_SEARCHER, true);

                                // Find the caster of SPELL_UNBOUND_PLAGUE_SEARCHER, the last owner,
                                // and remove the SPELL_UNBOUND_PLAGUE_PROTECTION on him
                                if (Aura* oldSearcherAura = oldPlagueTarget->GetAura(SPELL_UNBOUND_PLAGUE_SEARCHER))
                                    if (Unit* oldProtectedTarget = oldSearcherAura->GetCaster())
                                        oldProtectedTarget->RemoveAurasDueToSpell(SPELL_UNBOUND_PLAGUE_PROTECTION);

                                // Remove the searcher on the oldPlagueTarget
                                oldPlagueTarget->RemoveAurasDueToSpell(SPELL_UNBOUND_PLAGUE_SEARCHER);

                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_unbound_plague_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
                OnEffectHitTarget += SpellEffectFn(spell_putricide_unbound_plague_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_unbound_plague_SpellScript();
        }
};

class spell_putricide_eat_ooze : public SpellScriptLoader
{
    public:
        spell_putricide_eat_ooze() : SpellScriptLoader("spell_putricide_eat_ooze") { }

        class spell_putricide_eat_ooze_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_eat_ooze_SpellScript);

            void SelectTarget(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                targets.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
                WorldObject* target = targets.front();
                targets.clear();
                targets.push_back(target);
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Creature* target = GetHitCreature();
                if (!target)
                    return;

                if (Aura* grow = target->GetAura(uint32(GetEffectValue())))
                {
                    if (grow->GetStackAmount() < 3)
                    {
                        target->RemoveAurasDueToSpell(SPELL_GROW_STACKER);
                        target->RemoveAura(grow);
                        target->DespawnOrUnsummon(1);
                    }
                    else
                        grow->ModStackAmount(-3);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_putricide_eat_ooze_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_eat_ooze_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_eat_ooze_SpellScript();
        }
};

class spell_putricide_mutated_plague : public SpellScriptLoader
{
    public:
        spell_putricide_mutated_plague() : SpellScriptLoader("spell_putricide_mutated_plague") { }

        class spell_putricide_mutated_plague_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_putricide_mutated_plague_AuraScript);

            void HandleTriggerSpell(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                uint32 triggerSpell = GetSpellInfo()->Effects[aurEff->GetEffIndex()].TriggerSpell;
                SpellInfo const* spell = sSpellMgr->GetSpellInfo(triggerSpell);
                spell = sSpellMgr->GetSpellForDifficultyFromSpell(spell, caster);

                int32 damage = spell->Effects[EFFECT_0].CalcValue(caster);
                float multiplier = 2.0f;
                if (GetTarget()->GetMap()->GetSpawnMode() & 1)
                    multiplier = 3.0f;

                damage = damage * pow(multiplier, GetStackAmount());

                GetTarget()->CastCustomSpell(triggerSpell, SPELLVALUE_BASE_POINT0, damage, GetTarget(), true, NULL, aurEff, GetCasterGUID());
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                uint32 healSpell = uint32(GetSpellInfo()->Effects[EFFECT_0].CalcValue());
                SpellInfo const* healSpellInfo = sSpellMgr->GetSpellInfo(healSpell);
                healSpellInfo = sSpellMgr->GetSpellForDifficultyFromSpell(healSpellInfo, caster);

                if (!healSpellInfo)
                    return;

                int32 heal = healSpellInfo->Effects[0].CalcValue() * GetStackAmount();
                GetTarget()->CastCustomSpell(healSpell, SPELLVALUE_BASE_POINT0, heal, GetTarget(), true, NULL, NULL, GetCasterGUID());

            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_putricide_mutated_plague_AuraScript::HandleTriggerSpell, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_putricide_mutated_plague_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_putricide_mutated_plague_AuraScript();
        }
};

class spell_putricide_mutation_init : public SpellScriptLoader
{
    public:
        spell_putricide_mutation_init() : SpellScriptLoader("spell_putricide_mutation_init") { }

        class spell_putricide_mutation_init_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_mutation_init_SpellScript);

            SpellCastResult CheckRequirementInternal(SpellCustomErrors& extendedError)
            {
                InstanceScript* instance = GetExplTargetUnit()->GetInstanceScript();
                if (!instance)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                Creature* professor = ObjectAccessor::GetCreature(*GetExplTargetUnit(), instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE));
                if (!professor)
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                if (professor->AI()->GetData(DATA_PHASE) == PHASE_COMBAT_3 || !professor->IsAlive())
                {
                    extendedError = SPELL_CUSTOM_ERROR_ALL_POTIONS_USED;
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                if (professor->AI()->GetData(DATA_ABOMINATION))
                {
                    extendedError = SPELL_CUSTOM_ERROR_TOO_MANY_ABOMINATIONS;
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                return SPELL_CAST_OK;
            }

            SpellCastResult CheckRequirement()
            {
                if (!GetExplTargetUnit())
                    return SPELL_FAILED_BAD_TARGETS;

                if (GetExplTargetUnit()->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_TARGET_NOT_PLAYER;

                SpellCustomErrors extension = SPELL_CUSTOM_ERROR_NONE;
                SpellCastResult result = CheckRequirementInternal(extension);
                if (result != SPELL_CAST_OK)
                {
                    Spell::SendCastResult(GetExplTargetUnit()->ToPlayer(), GetSpellInfo(), 0, result, extension);
                    return result;
                }

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_putricide_mutation_init_SpellScript::CheckRequirement);
            }
        };

        class spell_putricide_mutation_init_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_putricide_mutation_init_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                uint32 spellId = 70311;
                if (GetTarget()->GetMap()->GetSpawnMode() & 1)
                    spellId = 71503;

                GetTarget()->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
                GetTarget()->CastSpell(GetTarget(), spellId, true);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_putricide_mutation_init_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_mutation_init_SpellScript();
        }

        AuraScript* GetAuraScript() const
        {
            return new spell_putricide_mutation_init_AuraScript();
        }
};

class spell_putricide_mutated_transformation_dismiss : public SpellScriptLoader
{
    public:
        spell_putricide_mutated_transformation_dismiss() : SpellScriptLoader("spell_putricide_mutated_transformation_dismiss") { }

        class spell_putricide_mutated_transformation_dismiss_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_putricide_mutated_transformation_dismiss_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Vehicle* veh = GetTarget()->GetVehicleKit())
                    veh->RemoveAllPassengers();
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_putricide_mutated_transformation_dismiss_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_putricide_mutated_transformation_dismiss_AuraScript();
        }
};

class spell_putricide_mutated_transformation : public SpellScriptLoader
{
    public:
        spell_putricide_mutated_transformation() : SpellScriptLoader("spell_putricide_mutated_transformation") { }

        class spell_putricide_mutated_transformation_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_mutated_transformation_SpellScript);

            void HandleSummon(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                Unit* caster = GetOriginalCaster();
                if (!caster)
                    return;

                InstanceScript* instance = caster->GetInstanceScript();
                if (!instance)
                    return;

                Creature* putricide = ObjectAccessor::GetCreature(*caster, instance->GetGuidData(DATA_PROFESSOR_PUTRICIDE));
                if (!putricide)
                    return;

                if (putricide->AI()->GetData(DATA_ABOMINATION))
                {
                    if (Player* player = caster->ToPlayer())
                        Spell::SendCastResult(player, GetSpellInfo(), 0, SPELL_FAILED_CUSTOM_ERROR, SPELL_CUSTOM_ERROR_TOO_MANY_ABOMINATIONS);
                    return;
                }

                uint32 entry = uint32(GetSpellInfo()->Effects[effIndex].MiscValue);
                SummonPropertiesEntry const* properties = sSummonPropertiesStore.LookupEntry(uint32(GetSpellInfo()->Effects[effIndex].MiscValueB));
                uint32 duration = uint32(GetSpellInfo()->GetDuration());

                Position pos;
                caster->GetPosition(&pos);
                TempSummon* summon = caster->GetMap()->SummonCreature(entry, pos, properties, duration, caster, GetSpellInfo()->Id);
                if (!summon || !summon->IsVehicle())
                    return;

                summon->CastSpell(summon, SPELL_ABOMINATION_VEHICLE_POWER_DRAIN, true);
                summon->CastSpell(summon, SPELL_MUTATED_TRANSFORMATION_DAMAGE, true);
                caster->CastSpell(summon, SPELL_MUTATED_TRANSFORMATION_NAME, true);

                caster->EnterVehicle(summon, 0);    // VEHICLE_SPELL_RIDE_HARDCODED is used according to sniff, this is ok
                summon->SetCreatorGUID(caster->GetGUID());
                putricide->AI()->JustSummoned(summon);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_putricide_mutated_transformation_SpellScript::HandleSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_mutated_transformation_SpellScript();
        }
};

class spell_putricide_mutated_transformation_dmg : public SpellScriptLoader
{
    public:
        spell_putricide_mutated_transformation_dmg() : SpellScriptLoader("spell_putricide_mutated_transformation_dmg") { }

        class spell_putricide_mutated_transformation_dmg_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_mutated_transformation_dmg_SpellScript);

            void FilterTargetsInitial(std::list<WorldObject*>& targets)
            {
                if (Unit* owner = ObjectAccessor::GetUnit(*GetCaster(), GetCaster()->GetCreatorGUID()))
                    targets.remove(owner);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_mutated_transformation_dmg_SpellScript::FilterTargetsInitial, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_mutated_transformation_dmg_SpellScript();
        }
};

class spell_putricide_regurgitated_ooze : public SpellScriptLoader
{
    public:
        spell_putricide_regurgitated_ooze() : SpellScriptLoader("spell_putricide_regurgitated_ooze") { }

        class spell_putricide_regurgitated_ooze_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_regurgitated_ooze_SpellScript);

            // the only purpose of this hook is to fail the achievement
            void ExtraEffect(SpellEffIndex /*effIndex*/)
            {
                if (InstanceScript* instance = GetCaster()->GetInstanceScript())
                    instance->SetData(DATA_NAUSEA_ACHIEVEMENT, uint32(false));
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_putricide_regurgitated_ooze_SpellScript::ExtraEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_regurgitated_ooze_SpellScript();
        }
};

// Removes aura with id stored in effect value
class spell_putricide_clear_aura_effect_value : public SpellScriptLoader
{
    public:
        spell_putricide_clear_aura_effect_value() : SpellScriptLoader("spell_putricide_clear_aura_effect_value") { }

        class spell_putricide_clear_aura_effect_value_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_clear_aura_effect_value_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                uint32 auraId = sSpellMgr->GetSpellIdForDifficulty(uint32(GetEffectValue()), GetCaster());
                GetHitUnit()->RemoveAurasDueToSpell(auraId);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_putricide_clear_aura_effect_value_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_putricide_clear_aura_effect_value_SpellScript();
        }
};

// Stinky and Precious spell, it's here because its used for both (Festergut and Rotface "pets")
class spell_stinky_precious_decimate : public SpellScriptLoader
{
    public:
        spell_stinky_precious_decimate() : SpellScriptLoader("spell_stinky_precious_decimate") { }

        class spell_stinky_precious_decimate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_stinky_precious_decimate_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (GetHitUnit()->GetHealthPct() > float(GetEffectValue()))
                {
                    uint32 newHealth = GetHitUnit()->GetMaxHealth() * uint32(GetEffectValue()) / 100;
                    GetHitUnit()->SetHealth(newHealth);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_stinky_precious_decimate_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_stinky_precious_decimate_SpellScript();
        }
};

class spell_shadow_infusion : public SpellScriptLoader
{
    public:
        spell_shadow_infusion() : SpellScriptLoader("spell_shadow_infusion") { }

        class spell_shadow_infusion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_shadow_infusion_SpellScript)
                
            void HandleAfterCast()
            {
                if (Unit* vehicle = GetCaster())
                {
                    if (Unit* player = vehicle->GetVehicleKit()->GetPassenger(0))
                        player->AddAura(SPELL_SHADOW_INFUSION, player);
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_shadow_infusion_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_shadow_infusion_SpellScript();
        }
};

class spell_mutated_slash : public SpellScriptLoader
{
    public:
        spell_mutated_slash() : SpellScriptLoader("spell_mutated_slash") { }

        class spell_mutated_slash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mutated_slash_SpellScript);

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                Unit* target = GetExplTargetUnit();
                uint32 damage = 0;
                switch (target->GetMap()->GetDifficulty())
                {
                    case RAID_DIFFICULTY_10MAN_NORMAL:
                        damage = 12500;
                        break;
                    case RAID_DIFFICULTY_25MAN_NORMAL:
                        damage = 16500;
                        break;
                    case RAID_DIFFICULTY_10MAN_HEROIC:
                        damage = 14400;
                        break;
                    case RAID_DIFFICULTY_25MAN_HEROIC:
                        damage = 21500;
                        break;
                }
                SetHitDamage(damage);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_mutated_slash_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mutated_slash_SpellScript();
        }
};

class spell_putricide_tear_gas_effect : public SpellScriptLoader
{
    public:
        spell_putricide_tear_gas_effect() : SpellScriptLoader("spell_putricide_tear_gas_effect") { }
    
        class spell_putricide_tear_gas_effect_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_putricide_tear_gas_effect_SpellScript);
    
            void FilterTargets(std::list<WorldObject*>& targets)
            {
                // vanish rank 1-3, mage invisibility
                targets.remove_if(Trinity::UnitAuraCheck(true, 11327));
                targets.remove_if(Trinity::UnitAuraCheck(true, 11329));
                targets.remove_if(Trinity::UnitAuraCheck(true, 26888));
                targets.remove_if(Trinity::UnitAuraCheck(true, 32612));
            }
    
            void Register() override
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_tear_gas_effect_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_putricide_tear_gas_effect_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_putricide_tear_gas_effect_SpellScript();
        }
};

void AddSC_boss_professor_putricide()
{
    new CreatureScriptLoaderEx<boss_professor_putricideAI>("boss_professor_putricide");
    new npc_volatile_ooze();
    new npc_gas_cloud();
    new npc_chocking_gas_bomb();
    new npc_ooze_puddle();
    new npc_mutated_abomination();
    new spell_putricide_gaseous_bloat();
    new spell_putricide_ooze_channel();
    new spell_putricide_slime_puddle();
    new spell_putricide_slime_puddle_aura();
    new spell_putricide_unstable_experiment();
    new spell_putricide_ooze_eruption_searcher();
    new spell_putricide_choking_gas_bomb();
    new spell_putricide_unbound_plague();
    new spell_putricide_eat_ooze();
    new spell_putricide_mutated_plague();
    new spell_putricide_mutation_init();
    new spell_putricide_mutated_transformation_dismiss();
    new spell_putricide_mutated_transformation();
    new spell_putricide_mutated_transformation_dmg();
    new spell_putricide_regurgitated_ooze();
    new spell_putricide_clear_aura_effect_value();
    new spell_stinky_precious_decimate();
    new spell_shadow_infusion();
    new spell_mutated_slash();
}
