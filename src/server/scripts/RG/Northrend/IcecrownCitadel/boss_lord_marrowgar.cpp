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
#include "MoveSplineInit.h"
#include "Player.h"
#include "Vehicle.h"
#include "icecrown_citadel.h"

enum ScriptTexts
{
    SAY_ENTER_ZONE              = 0,
    SAY_AGGRO                   = 1,
    SAY_BONE_STORM              = 2,
    SAY_BONESPIKE               = 3,
    SAY_KILL                    = 4,
    SAY_DEATH                   = 5,
    SAY_BERSERK                 = 6,
    EMOTE_BONE_STORM            = 7
};

enum Spells
{
    // Lord Marrowgar
    SPELL_BONE_SLICE            = 69055,
    SPELL_BONE_STORM            = 69076,
    SPELL_COLDFLAME_NORMAL      = 69140,
    SPELL_COLDFLAME_BONE_STORM  = 72705,

    // Bone Spike
    SPELL_IMPALED               = 69065,
    SPELL_RIDE_VEHICLE          = 46598,

    // Coldflame
    SPELL_COLDFLAME_PASSIVE     = 69145,
    SPELL_COLDFLAME_SUMMON      = 69147,

    // Misc
    SPELL_SPIRIT_OF_REDEMPTION  = 27827
};

#define SPELL_BONE_SPIKE_GRAVEYARD RAID_MODE<uint32>(69057, 70826, 72088, 72089)
uint32 const BoneSpikeSummonId[3] = {69062, 72669, 72670};

enum Events
{
    EVENT_BONE_SPIKE_GRAVEYARD = 1,
    EVENT_COLDFLAME,
    EVENT_BONE_STORM,
    EVENT_BONE_STORM_BEGIN,
    EVENT_BONE_STORM_MOVE,
    EVENT_BONE_STORM_END,
    EVENT_ENABLE_BONE_SLICE,
    EVENT_ENRAGE,

    EVENT_COLDFLAME_TRIGGER,
    EVENT_FAIL_BONED,
    EVENT_CHECK_IMPALED_AURA,

    EVENT_GROUP_SPECIAL         = 1
};

enum MovementPoints
{
    POINT_TARGET_BONESTORM_PLAYER   = 36612631,
    POINT_TARGET_COLDFLAME          = 36672631
};

enum MiscInfo
{
    DATA_COLDFLAME_GUID             = 0,

    // Manual marking for targets hit by Bone Slice as no aura exists for this purpose
    // These units are the tanks in this encounter
    // and should be immune to Bone Spike Graveyard
    DATA_SPIKE_IMMUNE               = 1,
    //DATA_SPIKE_IMMUNE_1,          = 2, // Reserved & used
    //DATA_SPIKE_IMMUNE_2,          = 3, // Reserved & used

    ACTION_CLEAR_SPIKE_IMMUNITIES   = 1,

    MAX_BONE_SPIKE_IMMUNE           = 3,

    RANGE_TO_PLAYER                 = 0,
};

class BoneSpikeTargetSelector
{
    public:
        BoneSpikeTargetSelector(UnitAI* ai) : _ai(ai) { }

        bool operator()(Unit* unit) const
        {
            if (unit->GetTypeId() != TYPEID_PLAYER)
                return false;

            if (unit->HasAura(SPELL_IMPALED))
                return false;

            if (unit->HasAura(SPELL_SPIRIT_OF_REDEMPTION))
                return false;            
            
            if (unit->isDead())
                return false;

            // Check if it is one of the tanks soaking Bone Slice
            for (uint32 i = 0; i < MAX_BONE_SPIKE_IMMUNE; ++i)
                if (unit->GetGUID() == _ai->GetGuidData(DATA_SPIKE_IMMUNE + i))
                    return false;

            return true;
        }

    private:
        UnitAI* _ai;
};

class boss_lord_marrowgar : public CreatureScript
{
    public:
        boss_lord_marrowgar() : CreatureScript("boss_lord_marrowgar") { }

        struct boss_lord_marrowgarAI : public BossAI
        {
            boss_lord_marrowgarAI(Creature* creature) : BossAI(creature, DATA_LORD_MARROWGAR)
            {
                _boneStormDuration = RAID_MODE<uint32>(20000, 20000, 30000, 30000);
                _baseSpeed = creature->GetSpeedRate(MOVE_RUN);
                _coldflameLastPos.Relocate(creature);
                _introDone = false;
                _boneSlice = false;
            }

            void Reset()
            {
                BossAI::Reset();
                me->SetSpeedRate(MOVE_RUN, _baseSpeed);
                me->RemoveAurasDueToSpell(SPELL_BONE_STORM);
                me->RemoveAurasDueToSpell(SPELL_BERSERK);
                me->SetReactState(REACT_AGGRESSIVE);
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                events.ScheduleEvent(EVENT_ENABLE_BONE_SLICE, 10000);
                events.ScheduleEvent(EVENT_BONE_SPIKE_GRAVEYARD, 15000, EVENT_GROUP_SPECIAL);
                events.ScheduleEvent(EVENT_COLDFLAME, 5000, EVENT_GROUP_SPECIAL);
                events.ScheduleEvent(EVENT_BONE_STORM, urand(45000, 50000));
                events.ScheduleEvent(EVENT_ENRAGE, 600000);
                _boneSlice = false;
                _boneSpikeImmune.clear();
                BoneSpikeDespawn();
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_IMPALED);
            }

            void EnterCombat(Unit* /*who*/)
            {
                Talk(SAY_AGGRO);

                me->setActive(true);
                DoZoneInCombat();
                instance->SetBossState(DATA_LORD_MARROWGAR, IN_PROGRESS);
            }

            void BoneSpikeDespawn()
            {
                std::list<Creature*> BonespikeList;
                me->GetCreatureListWithEntryInGrid(BonespikeList, NPC_BONE_SPIKE, 200.0f);
                if (!BonespikeList.empty())
                    for (std::list<Creature*>::iterator itr = BonespikeList.begin(); itr != BonespikeList.end(); itr++)
                    {
                        // vehicle id 533 has only seat 0
                        if (Vehicle* vehicle = (*itr)->GetVehicleKit())
                            if (Unit* passenger = vehicle->GetPassenger(0))
                            {
                                passenger->RemoveAurasDueToSpell(SPELL_IMPALED);
                                passenger->ExitVehicle();
                            }
                        
                        (*itr)->DespawnOrUnsummon();
                    }
            }

            void JustDied(Unit* killer)
            {
                Talk(SAY_DEATH);
                BoneSpikeDespawn();
                instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_IMPALED);

                BossAI::JustDied(killer);
            }

            void JustReachedHome()
            {
                BossAI::JustReachedHome();
                instance->SetBossState(DATA_LORD_MARROWGAR, FAIL);
                instance->SetData(DATA_BONED_ACHIEVEMENT, uint32(true));    // reset
            }

            void KilledUnit(Unit* victim)
            {
                if (victim->GetTypeId() == TYPEID_PLAYER)
                    Talk(SAY_KILL);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (!_introDone && me->IsWithinDistInMap(who, 70.0f))
                {
                    Talk(SAY_ENTER_ZONE);
                    _introDone = true;
                }
                // Normal Aggrobehaviour
                ScriptedAI::MoveInLineOfSight(who);
            }

            void AttackStart(Unit* victim)
            {
                if (victim && me->Attack(victim, true))
                    me->GetMotionMaster()->MoveChase(victim, (float)RANGE_TO_PLAYER);
            }

            bool UpdateVictim()
            {
                if (!me->IsInCombat())
                    return false;

                if (!me->HasReactState(REACT_PASSIVE))
                {
                    if (Unit* victim = me->SelectVictim())
                        AttackStart(victim);
                    return me->GetVictim();
                }
                else if (me->GetThreatManager().isThreatListEmpty())
                {
                    EnterEvadeMode();
                    return false;
                }

                return true;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->isAttackReady() && me->HasAura(SPELL_BONE_SPIKE_GRAVEYARD))
                    me->InterruptNonMeleeSpells(false, SPELL_BONE_SPIKE_GRAVEYARD);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BONE_SPIKE_GRAVEYARD:
                            if (IsHeroic() || !me->HasAura(SPELL_BONE_STORM))
                                DoCastSelf(SPELL_BONE_SPIKE_GRAVEYARD);
                            events.ScheduleEvent(EVENT_BONE_SPIKE_GRAVEYARD, urand(15000, 20000), EVENT_GROUP_SPECIAL);
                            break;
                        case EVENT_COLDFLAME:
                            _coldflameLastPos.Relocate(me);
                            _coldflameTarget.Clear();
                            if (!me->HasAura(SPELL_BONE_STORM))
                                DoCastAOE(SPELL_COLDFLAME_NORMAL);
                            events.ScheduleEvent(EVENT_COLDFLAME, 5000, EVENT_GROUP_SPECIAL);
                            break;
                        case EVENT_BONE_STORM:
                            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                            _boneSlice = false;
                            TeleportCheaters();
                            Talk(EMOTE_BONE_STORM);
                            Talk(SAY_BONE_STORM);
                            me->FinishSpell(CURRENT_MELEE_SPELL, false);
                            me->SetReactState(REACT_PASSIVE); // to prevent chasing another target on UpdateVictim()
                            me->AddUnitState(UNIT_STATE_ROOT);
                            DoCastSelf(SPELL_BONE_STORM);
                            events.DelayEvents(3000, EVENT_GROUP_SPECIAL);
                            events.ScheduleEvent(EVENT_BONE_STORM_BEGIN, 3100);
                            events.ScheduleEvent(EVENT_BONE_STORM, urand(90, 95) * IN_MILLISECONDS);
                            break;
                        case EVENT_BONE_STORM_BEGIN:
                        {
                            Aura* boneStorm = me->GetAura(SPELL_BONE_STORM);
                            if (!boneStorm)
                                boneStorm = me->AddAura(SPELL_BONE_STORM, me);
                            boneStorm->SetDuration(int32(_boneStormDuration));
                            me->SetSpeedRate(MOVE_RUN, _baseSpeed * 3.f);
                            events.ScheduleEvent(EVENT_BONE_STORM_END, _boneStormDuration + 1000);
                            events.ScheduleEvent(EVENT_BONE_STORM_MOVE, 1000);
                            break;
                        }
                        case EVENT_BONE_STORM_MOVE:
                            me->ClearUnitState(UNIT_STATE_ROOT);
                            BonestormMovement();
                            events.ScheduleEvent(EVENT_BONE_STORM_MOVE, (IsHeroic() ? 6 : 5) * IN_MILLISECONDS);
                            break;
                        case EVENT_BONE_STORM_END:
                            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->SetSpeedRate(MOVE_RUN, _baseSpeed);
                            if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() == POINT_MOTION_TYPE)
                                me->GetMotionMaster()->MovementExpired();
                            me->GetMotionMaster()->MoveChase(me->GetVictim());
                            events.CancelEvent(EVENT_BONE_STORM_MOVE);
                            events.ScheduleEvent(EVENT_ENABLE_BONE_SLICE, 10000);
                            if (!IsHeroic())
                                events.RescheduleEvent(EVENT_BONE_SPIKE_GRAVEYARD, 15000, EVENT_GROUP_SPECIAL);
                            break;
                        case EVENT_ENABLE_BONE_SLICE:
                            _boneSlice = true;
                            break;
                        case EVENT_ENRAGE:
                            DoCastSelf(SPELL_BERSERK, true);
                            Talk(SAY_BERSERK);
                            break;
                    }
                }

                // We should not melee attack when storming
                if (me->HasAura(SPELL_BONE_STORM))
                    return;

                // 10 seconds since encounter start Bone Slice replaces melee attacks
                if (_boneSlice && !me->GetCurrentSpell(CURRENT_MELEE_SPELL))
                    DoCastVictim(SPELL_BONE_SLICE);

                DoMeleeAttackIfReady(true);
            }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE || id != POINT_TARGET_BONESTORM_PLAYER)
                    return;

                // lock movement
                me->GetMotionMaster()->MoveIdle();
                _coldflameLastPos.Relocate(me);
                _coldflameTarget.Clear();
                DoCastSelf(SPELL_COLDFLAME_BONE_STORM);
            }

            Position const* GetLastColdflamePosition() const
            {
                return &_coldflameLastPos;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_COLDFLAME_GUID:
                        return _coldflameTarget;
                    case DATA_SPIKE_IMMUNE + 0:
                    case DATA_SPIKE_IMMUNE + 1:
                    case DATA_SPIKE_IMMUNE + 2:
                    {
                        uint32 index = uint32(type - DATA_SPIKE_IMMUNE);
                        if (index < _boneSpikeImmune.size())
                            return _boneSpikeImmune[index];
                        break;
                    }
                }

                return ObjectGuid::Empty;
            }

            void SetGuidData(ObjectGuid guid, uint32 type /*= 0 */) override
            {
                switch (type)
                {
                    case DATA_COLDFLAME_GUID:
                        _coldflameTarget = guid;
                        break;
                    case DATA_SPIKE_IMMUNE:
                        _boneSpikeImmune.push_back(guid);
                        break;
                }
            }

            void DoAction(int32 action)
            {
                if (action != ACTION_CLEAR_SPIKE_IMMUNITIES)
                    return;

                _boneSpikeImmune.clear();
            }

            void BonestormMovement()
            {
                std::list<Player*> plrList;
                me->GetPlayerListInGrid(plrList, 1000.f);
                std::list<Player*> targetGroup1; // > 70 m
                std::list<Player*> targetGroup2; // 45 m ... 70 m
                std::list<Player*> targetGroup3; // 15 m ... 45 m
                std::list<Player*> targetGroup4; // < 15 m

                for (std::list<Player*>::const_iterator itr = plrList.begin(); itr != plrList.end(); ++itr)
                {
                    if (Player* player = (*itr)->ToPlayer())
                    {
                        if (player->IsGameMaster())
                            continue;

                        float distance = player->GetExactDist(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
                        if (distance >= 70.0f)
                            targetGroup1.push_back(player);
                        else if (distance >= 45.0f)
                            targetGroup2.push_back(player);
                        else if (distance >= 15.0f)
                            targetGroup3.push_back(player);
                        else
                            targetGroup4.push_back(player);
                    }
                }

                uint32 TargetGroup = 0;

                // There are 8 cases to distinguish for target selection
                if (!targetGroup1.empty() && targetGroup2.empty() && targetGroup3.empty())
                {
                    TargetGroup = 1;
                }
                else if (targetGroup1.empty() && !targetGroup2.empty() && targetGroup3.empty())
                {
                    TargetGroup = 2;
                }
                else if (targetGroup1.empty() && targetGroup2.empty() && !targetGroup3.empty())
                {
                    TargetGroup = 3;
                }
                else if (!targetGroup1.empty() && !targetGroup2.empty() && targetGroup3.empty())
                {
                    if (urand(1, 95) <= 5)
                    {
                        TargetGroup = 2;
                    }
                    else
                    {
                        TargetGroup = 1;
                    }
                }
                else if (!targetGroup1.empty() && targetGroup2.empty() && !targetGroup3.empty())
                {
                    if (urand(1, 95) <= 5)
                    {
                        TargetGroup = 3;
                    }
                    else
                    {
                        TargetGroup = 1;
                    }
                }
                else if (targetGroup1.empty() && !targetGroup2.empty() && !targetGroup3.empty())
                {
                    if (urand(1, 100) > 5)
                    {
                        TargetGroup = 2;
                    }
                    else
                    {
                        TargetGroup = 3;
                    }
                }
                else if (!targetGroup1.empty() && !targetGroup2.empty() && !targetGroup3.empty())
                {
                    uint32 rndTargetGroup = urand(1, 100);
                    if (rndTargetGroup >= 11) // 90% chance to charge someone >= 70 m away
                    {
                        TargetGroup = 1;
                    }
                    else if (rndTargetGroup >= 6) // 5% chance to charge someone 45 m ... 70 m away
                    {
                        TargetGroup = 2;
                    }
                    else // 5% chance to charge some 15 m ... 45 m away
                    {
                        TargetGroup = 3;
                    }
                }
                else
                {
                    TargetGroup = 4;
                }

                std::list<Player*>::iterator playeritr;
                switch (TargetGroup)
                {
                    case 1:
                        playeritr = targetGroup1.begin();
                        if (playeritr != targetGroup1.end())
                        {
                            std::advance(playeritr, urand(0, targetGroup1.size() - 1));
                            break;
                        }
                        // fall-through
                    case 2:
                        playeritr = targetGroup2.begin();
                        if (playeritr != targetGroup2.end())
                        {
                            std::advance(playeritr, urand(0, targetGroup2.size() - 1));
                            break;
                        }
                        // fall-through
                    case 3:
                        playeritr = targetGroup3.begin();
                        if (playeritr != targetGroup3.end())
                        {
                            std::advance(playeritr, urand(0, targetGroup3.size() - 1));
                            break;
                        }
                        // fall-through
                    default:
                        playeritr = targetGroup4.begin();
                        if (playeritr != targetGroup4.end())
                        {
                            std::advance(playeritr, urand(0, targetGroup4.size() - 1));
                            break;
                        }
                        else
                        {
                            // no target found in any target group, return
                            return;
                        }
                }
                Player* target = *playeritr; // iterator is valid
                me->GetMotionMaster()->MovePoint(POINT_TARGET_BONESTORM_PLAYER, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ());
            }

        private:
            Position _coldflameLastPos;
            std::vector<ObjectGuid> _boneSpikeImmune;
            ObjectGuid _coldflameTarget;
            uint32 _boneStormDuration;
            float _baseSpeed;
            bool _introDone;
            bool _boneSlice;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<boss_lord_marrowgarAI>(creature);
        }
};

typedef boss_lord_marrowgar::boss_lord_marrowgarAI MarrowgarAI;

class npc_coldflame : public CreatureScript
{
    public:
        npc_coldflame() : CreatureScript("npc_coldflame") { }

        struct npc_coldflameAI : public ScriptedAI
        {
            npc_coldflameAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
                me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), 42.274891f, me->GetOrientation());
            }

            void IsSummonedBy(Unit* owner)
            {
                if (owner->GetTypeId() != TYPEID_UNIT)
                    return;

                Position pos;
                if (MarrowgarAI* marrowgarAI = CAST_AI(MarrowgarAI, owner->GetAI()))
                    pos.Relocate(marrowgarAI->GetLastColdflamePosition());
                else
                    pos.Relocate(owner);

                if (owner->HasAura(SPELL_BONE_STORM))
                {
                    float ang = Position::NormalizeOrientation(pos.GetAngle(me));
                    me->SetOrientation(ang);
                    owner->GetNearPoint2D(pos.m_positionX, pos.m_positionY, 5.0f - owner->GetObjectSize(), ang);
                }
                else
                {
                    Player* target = ObjectAccessor::GetPlayer(*owner, owner->GetAI()->GetGuidData(DATA_COLDFLAME_GUID));
                    if (!target)
                    {
                        me->DespawnOrUnsummon();
                        return;
                    }

                    float ang = Position::NormalizeOrientation(pos.GetAngle(target));
                    me->SetOrientation(ang);

                    // Summon Coldflame closer to Marrowgar if everyone is in Meleerange
                    if (Map* map = me->GetMap())
                    {
                        bool NearMeleeRange = true;
                        for (Player& player : map->GetPlayers())
                            if (player.GetDistance(owner) >= 0.1f)
                            {
                                NearMeleeRange = false;
                                owner->GetNearPoint2D(pos.m_positionX, pos.m_positionY, 15.0f - owner->GetObjectSize(), ang);                                        
                            }
                        if (NearMeleeRange)
                            owner->GetNearPoint2D(pos.m_positionX, pos.m_positionY, 2.0f - owner->GetObjectSize(), ang);
                    }
                }

                me->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                DoCast(SPELL_COLDFLAME_SUMMON);
                _events.ScheduleEvent(EVENT_COLDFLAME_TRIGGER, 500);
            }

            void UpdateAI(uint32 diff)
            {
                _events.Update(diff);

                if (_events.ExecuteEvent() == EVENT_COLDFLAME_TRIGGER)
                {
                    Position newPos;
                    me->GetNearPosition(newPos, 3.0f, 0.0f);
                    if (me->GetExactDist(&newPos) < 2.0f)
                        return;
                    me->NearTeleportTo(newPos.GetPositionX(), newPos.GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                    DoCast(SPELL_COLDFLAME_SUMMON);
                    _events.ScheduleEvent(EVENT_COLDFLAME_TRIGGER, 0.7*IN_MILLISECONDS);
                }
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_coldflameAI>(creature);
        }
};

class npc_bone_spike : public CreatureScript
{
    public:
        npc_bone_spike() : CreatureScript("npc_bone_spike") { }

        struct npc_bone_spikeAI : public ScriptedAI
        {
            npc_bone_spikeAI(Creature* creature) : ScriptedAI(creature), _hasTrappedUnit(false)
            {
                ASSERT(creature->GetVehicleKit());

                SetCombatMovement(false);
                instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                _events.ScheduleEvent(EVENT_CHECK_IMPALED_AURA, IN_MILLISECONDS);
            }

            void JustDied(Unit* /*killer*/)
            {
                if (TempSummon* summ = me->ToTempSummon())
                    if (Unit* trapped = summ->GetSummoner())
                        trapped->RemoveAurasDueToSpell(SPELL_IMPALED);

                me->DespawnOrUnsummon();
            }

            void KilledUnit(Unit* victim)
            {
                me->DespawnOrUnsummon();
                victim->RemoveAurasDueToSpell(SPELL_IMPALED);
            }

            void IsSummonedBy(Unit* summoner)
            {
                DoCast(summoner, SPELL_IMPALED);
                summoner->CastSpell(me, SPELL_RIDE_VEHICLE, true);
                _events.ScheduleEvent(EVENT_FAIL_BONED, 8000);
                _hasTrappedUnit = true;
            }

            void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply)
            {
                if (!apply)
                    return;
                /// @HACK - Change passenger offset to the one taken directly from sniffs
                /// Remove this when proper calculations are implemented.
                /// This fixes healing spiked people
                Movement::MoveSplineInit init(passenger);
                init.DisableTransportPathTransformations();
                init.MoveTo(-0.02206125f, -0.02132235f, 5.514783f, false);
                init.Launch();
            }

            void UpdateAI(uint32 diff)
            {
                if (instance->GetBossState(DATA_LORD_MARROWGAR) != IN_PROGRESS)
                {
                    me->DespawnOrUnsummon();
                    if (Vehicle* vehicle = me->GetVehicleKit())
                        if (Unit* passenger = vehicle->GetPassenger(0))
                        {
                            passenger->RemoveAurasDueToSpell(SPELL_IMPALED);
                            me->Kill(passenger, true);
                        }
                    return;
                }

                if (!_hasTrappedUnit)
                    return;

                _events.Update(diff);

                switch (_events.ExecuteEvent())
                {
                case EVENT_FAIL_BONED:
                    if (InstanceScript* instance = me->GetInstanceScript())
                        instance->SetData(DATA_BONED_ACHIEVEMENT, uint32(false));
                    break;
                case EVENT_CHECK_IMPALED_AURA:
                    if (Vehicle* vehicle = me->GetVehicleKit())
                        if (Unit* passenger = vehicle->GetPassenger(0))
                            if (!passenger->HasAura(SPELL_IMPALED))
                                DoCast(passenger, SPELL_IMPALED);
                    _events.ScheduleEvent(EVENT_CHECK_IMPALED_AURA, IN_MILLISECONDS);
                    break;
                default:
                    break;
                }
            }

        private:
            EventMap _events;
            InstanceScript* instance;
            bool _hasTrappedUnit;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_bone_spikeAI>(creature);
        }
};

class spell_marrowgar_coldflame : public SpellScriptLoader
{
    public:
        spell_marrowgar_coldflame() : SpellScriptLoader("spell_marrowgar_coldflame") { }

        class spell_marrowgar_coldflame_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marrowgar_coldflame_SpellScript);

            void SelectTarget(std::list<WorldObject*>& targets)
            {
                targets.clear();
                // select any unit but not the tank (by owners threatlist)
                Unit* target = GetCaster()->GetAI()->SelectTarget(SELECT_TARGET_RANDOM, 0, -GetCaster()->GetObjectSize(), true, false, -SPELL_IMPALED);
                if (!target)
                    target = GetCaster()->GetAI()->SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true, true, -SPELL_IMPALED); // or the tank if its solo
                if (!target)
                    return;

                GetCaster()->GetAI()->SetGuidData(target->GetGUID(), DATA_COLDFLAME_GUID);
                targets.push_back(target);
            }

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_marrowgar_coldflame_SpellScript::SelectTarget, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_marrowgar_coldflame_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marrowgar_coldflame_SpellScript();
        }
};

class spell_marrowgar_coldflame_bonestorm : public SpellScriptLoader
{
    public:
        spell_marrowgar_coldflame_bonestorm() : SpellScriptLoader("spell_marrowgar_coldflame_bonestorm") { }

        class spell_marrowgar_coldflame_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marrowgar_coldflame_SpellScript);

            void HandleScriptEffect(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                for (uint8 i = 0; i < 4; ++i)
                {
                    float x = GetCaster()->GetPositionX();
                    float y = GetCaster()->GetPositionY();
                    float o = 0.0f;
                    switch (i)
                    {
                        case 0: o = 0.0f; break;
                        case 1: o = M_PI_F; break;
                        case 2: o = M_PI_F + (M_PI_F / 2.0f); break;
                        case 3: o = M_PI_F * 2.0f; break;
                        default: break;
                    }
                    x += 7.0f * cos(o);
                    y += 7.0f * sin(o);

                    GetCaster()->CastSpell(x, y, GetCaster()->GetPositionZ(), uint32(GetEffectValue() + i), true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_marrowgar_coldflame_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marrowgar_coldflame_SpellScript();
        }
};

class spell_marrowgar_coldflame_damage : public SpellScriptLoader
{
    public:
        spell_marrowgar_coldflame_damage() : SpellScriptLoader("spell_marrowgar_coldflame_damage") { }

        class spell_marrowgar_coldflame_damage_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_marrowgar_coldflame_damage_AuraScript);

            bool CanBeAppliedOn(Unit* target)
            {
                if (target->HasAura(SPELL_IMPALED))
                    return false;

                if (target->GetExactDist2d(GetOwner()) > GetSpellInfo()->Effects[EFFECT_0].CalcRadius())
                    return false;

                if (Aura* aur = target->GetAura(GetId()))
                    if (aur->GetOwner() != GetOwner())
                        return false;

                return true;
            }

            void Register()
            {
                DoCheckAreaTarget += AuraCheckAreaTargetFn(spell_marrowgar_coldflame_damage_AuraScript::CanBeAppliedOn);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_marrowgar_coldflame_damage_AuraScript();
        }
};

class spell_marrowgar_bone_spike_graveyard : public SpellScriptLoader
{
    public:
        spell_marrowgar_bone_spike_graveyard() : SpellScriptLoader("spell_marrowgar_bone_spike_graveyard") { }

        class spell_marrowgar_bone_spike_graveyard_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marrowgar_bone_spike_graveyard_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                for (uint32 i = 0; i < 3; ++i)
                    if (!sSpellMgr->GetSpellInfo(BoneSpikeSummonId[i]))
                        return false;

                return true;
            }

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT && GetCaster()->IsAIEnabled;
            }

            SpellCastResult CheckCast()
            {
                return GetCaster()->GetAI()->SelectTarget(SELECT_TARGET_RANDOM, 0, BoneSpikeTargetSelector(GetCaster()->GetAI())) ? SPELL_CAST_OK : SPELL_FAILED_NO_VALID_TARGETS;
            }

            void HandleSpikes(SpellEffIndex effIndex)
            {
                if (Creature* marrowgar = GetCaster()->ToCreature())
                {
                    if (marrowgar->HasAura(SPELL_BONE_STORM))
                        PreventHitDefaultEffect(effIndex);

                    CreatureAI* marrowgarAI = marrowgar->AI();
                    uint8 boneSpikeCount = uint8(GetCaster()->GetMap()->GetSpawnMode() & 1 ? 3 : 1);

                    std::list<Unit*> targets;
                    marrowgarAI->SelectTargetList(targets, boneSpikeCount, SELECT_TARGET_RANDOM, 0, BoneSpikeTargetSelector(marrowgarAI));
                    if (targets.empty())
                        return;

                    uint32 i = 0;
                    for (std::list<Unit*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr, ++i)
                    {
                        Unit* target = *itr;
                        target->CastSpell(target, BoneSpikeSummonId[i], true);
                    }

                    marrowgarAI->Talk(SAY_BONESPIKE);
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_marrowgar_bone_spike_graveyard_SpellScript::CheckCast);
                OnEffectHitTarget += SpellEffectFn(spell_marrowgar_bone_spike_graveyard_SpellScript::HandleSpikes, EFFECT_1, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marrowgar_bone_spike_graveyard_SpellScript();
        }
};

class spell_marrowgar_bone_storm : public SpellScriptLoader
{
    public:
        spell_marrowgar_bone_storm() : SpellScriptLoader("spell_marrowgar_bone_storm") { }

        class spell_marrowgar_bone_storm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marrowgar_bone_storm_SpellScript);

            void RecalculateDamage()
            {
                if (GetHitUnit()->GetExactDist2d(GetCaster()) >= 6.0f)
                    SetHitDamage(int32(GetHitDamage() / std::max(cbrtf(GetHitUnit()->GetExactDist2d(GetCaster())), 1.0f)));
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_marrowgar_bone_storm_SpellScript::RecalculateDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marrowgar_bone_storm_SpellScript();
        }
};

class spell_marrowgar_bone_slice : public SpellScriptLoader
{
    public:
        spell_marrowgar_bone_slice() : SpellScriptLoader("spell_marrowgar_bone_slice") { }

        class spell_marrowgar_bone_slice_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_marrowgar_bone_slice_SpellScript);

            bool Load()
            {
                _targetCount = 0;
                return true;
            }

            void ClearSpikeImmunities()
            {
                GetCaster()->GetAI()->DoAction(ACTION_CLEAR_SPIKE_IMMUNITIES);
            }

            void CountTargets(std::list<WorldObject*>& targets)
            {
                _targetCount = std::min<uint32>(targets.size(), GetSpellInfo()->MaxAffectedTargets);
            }

            void SplitDamage()
            {
                // Mark the unit as hit, even if the spell missed or was dodged/parried
                GetCaster()->GetAI()->SetGuidData(GetHitUnit()->GetGUID(), DATA_SPIKE_IMMUNE);

                if (!_targetCount)
                    return; // This spell can miss all targets

                SetHitDamage(GetHitDamage() / _targetCount);
            }

            void Register()
            {
                BeforeCast += SpellCastFn(spell_marrowgar_bone_slice_SpellScript::ClearSpikeImmunities);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_marrowgar_bone_slice_SpellScript::CountTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnHit += SpellHitFn(spell_marrowgar_bone_slice_SpellScript::SplitDamage);
            }

            uint32 _targetCount;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_marrowgar_bone_slice_SpellScript();
        }
};

void AddSC_boss_lord_marrowgar()
{
    new boss_lord_marrowgar();
    new npc_coldflame();
    new npc_bone_spike();
    new spell_marrowgar_coldflame();
    new spell_marrowgar_coldflame_bonestorm();
    new spell_marrowgar_coldflame_damage();
    new spell_marrowgar_bone_spike_graveyard();
    new spell_marrowgar_bone_storm();
    new spell_marrowgar_bone_slice();
}
