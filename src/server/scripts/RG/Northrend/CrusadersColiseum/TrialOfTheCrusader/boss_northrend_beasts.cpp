/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2010 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "trial_of_the_crusader.h"
#include "Vehicle.h"
#include "Player.h"
#include "SpellScript.h"
#include "WorldPacket.h"
#include "MoveSplineInit.h"
#include "Opcodes.h"
#include "Pet.h"
#include "RG/Logging/LogManager.hpp"

enum Yells
{
    // Gormok
    EMOTE_SNOBOLLED         = 0,

    // Acidmaw & Dreadscale
    EMOTE_ENRAGE            = 0,

    // Icehowl
    EMOTE_TRAMPLE_START     = 0,
    EMOTE_TRAMPLE_CRASH     = 1,
    EMOTE_TRAMPLE_FAIL      = 2
};

enum Model
{
    MODEL_ACIDMAW_STATIONARY     = 29815,
    MODEL_ACIDMAW_MOBILE         = 29816,
    MODEL_DREADSCALE_STATIONARY  = 26935,
    MODEL_DREADSCALE_MOBILE      = 24564
};

enum BeastSummons
{
  //NPC_SNOBOLD_VASSAL           = 34800,
    NPC_FIRE_BOMB                = 34854,
    NPC_SLIME_POOL               = 35176,
    MAX_SNOBOLDS                 = 4,

    ENTRY_ACIDMAW_TRIGGER        = 1010689,
    ENTRY_DREADSCALE_TRIGGER     = 1010690
};

enum BossSpells
{
    //Gormok
    SPELL_IMPALE            = 66331,
    SPELL_STAGGERING_STOMP  = 67648,
    SPELL_RISING_ANGER      = 66636,
    //Snobold
    SPELL_SNOBOLLED         = 66406,
    SPELL_BATTER            = 66408,
    SPELL_FIRE_BOMB         = 66313,
    SPELL_FIRE_BOMB_1       = 66317,
    SPELL_FIRE_BOMB_DOT     = 66318,
    SPELL_HEAD_CRACK        = 66407,
    SPELL_JUMP_TO_HAND      = 66342,

    //Acidmaw & Dreadscale
    SPELL_ACID_SPIT         = 66880,
    SPELL_PARALYTIC_BITE    = 66824,
    SPELL_PARALYTIC_SPRAY   = 66901,
    SPELL_PARALYTIC_TOXIN   = 66823,
    SPELL_ACID_SPEW         = 66818,
    SPELL_SWEEP_0           = 66794,
    SUMMON_SLIME_POOL       = 66883,
    SPELL_FIRE_SPIT         = 66796,
    SPELL_MOLTEN_SPEW       = 66821,
    SPELL_BURNING_BITE      = 66879,
    SPELL_BURNING_SPRAY     = 66902,
    SPELL_BURNING_BILE      = 66869,
    SPELL_SWEEP_1           = 67646,
    SPELL_EMERGE_0          = 66947,
    SPELL_SUBMERGE_0        = 66948,
    SPELL_ENRAGE_SOFT       = 68335,
    SPELL_SLIME_POOL_EFFECT = 66882, //In 60s it diameter grows from 10y to 40y (r=r+0.25 per second)

    //Icehowl
    SPELL_FEROCIOUS_BUTT    = 66770,
    SPELL_MASSIVE_CRASH     = 66683,
    SPELL_WHIRL             = 67345,
    SPELL_ARCTIC_BREATH     = 66689,
    SPELL_TRAMPLE           = 66734,
    SPELL_FROTHING_RAGE     = 66759,
    SPELL_STAGGERED_DAZE    = 66758,

    //All Beasts
    SPELL_ENRAGE            = 47008
};

enum Actions
{
    ACTION_ENABLE_FIRE_BOMB     = 1,
    ACTION_START_SNOBOLD_TIMER,
    // snakes
    ACTION_SUMMON_NEXT_BEAST,
};

enum Events
{
    // Gormok
    EVENT_GORMOK_START          = 1,
    EVENT_IMPALE,
    EVENT_STAGGERING_STOMP,
    EVENT_THROW,
    EVENT_JUMP_TO_HAND,

    // Snobold
    EVENT_FIRE_BOMB,
    EVENT_BATTER,
    EVENT_HEAD_CRACK,

    // Acidmaw & Dreadscale
    EVENT_BITE,
    EVENT_SPEW,
    EVENT_SLIME_POOL,
    EVENT_SPRAY,
    EVENT_SWEEP,
    EVENT_SUBMERGE,
    EVENT_EMERGE,
    EVENT_SUMMON_ACIDMAW,

    // Icehowl
    EVENT_ICEHOWL_START,
    EVENT_FEROCIOUS_BUTT,
    EVENT_MASSIVE_CRASH,
    EVENT_WHIRL,
    EVENT_ARCTIC_BREATH,
    EVENT_ARCTIC_BREATH_ENABLE_MOVE,
    EVENT_TRAMPLE,
    EVENT_ICEHOWL_CHARGE,
    EVENT_CHECK_LOS,
    EVENT_STARE,
    EVENT_ROAWR,

    // All Beasts
    EVENT_NEXT_BEAST,
    EVENT_ENRAGE
};

enum Phases
{
    PHASE_MOBILE            = 1,
    PHASE_STATIONARY        = 2,
    PHASE_SUBMERGED         = 3,
};

enum MiscData
{
    PLAYER_VEHICLE_ID       = 443,
};

bool SetPlayerVehicle(Player* player, bool apply)
{
    if (apply)
    {
        if (player->GetVehicleKit())
            return true;
        else
        {
            if (!player->CreateVehicleKit(PLAYER_VEHICLE_ID, 0))
                return false;
        }
    }
    else
    {
        if (!player->GetVehicleKit())
            return true;
        player->RemoveVehicleKit();
    }

    WorldPacket data(SMSG_PLAYER_VEHICLE_DATA, player->GetPackGUID().size()+4);
    data << player->GetPackGUID();
    data << uint32(apply ? PLAYER_VEHICLE_ID : 0);
    player->SendMessageToSet(data, true);

    return true;
}

void ResetPlayerVehicles(Map* map)
{
    for (Player& player : map->GetPlayers())
    {
        SetPlayerVehicle(&player, false);
        player.RemoveAura(SPELL_SNOBOLLED);
    }
}

class boss_gormok : public CreatureScript
{
    public:
        boss_gormok() : CreatureScript("boss_gormok") { }

        struct boss_gormokAI : public BossAI
        {
            boss_gormokAI(Creature* creature) : BossAI(creature, BOSS_BEASTS) 
            {
                me->SetSpeedRate(MOVE_WALK, 3.0f);
                me->SetWalk(true);
            }

            void Reset()
            {
                snoboldGUID.Clear();
                nextBeastSummoned = false;

                if (IsHeroic())
                    events.ScheduleEvent(EVENT_NEXT_BEAST, 145*IN_MILLISECONDS); // so they will exactly arrive in time
                else
                    events.ScheduleEvent(EVENT_ENRAGE, 15*MINUTE*IN_MILLISECONDS);

                summons.DespawnAll();
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                ResetPlayerVehicles(me->GetMap());

                instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                ScriptedAI::EnterEvadeMode();
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (pointId)
                {
                    case 0:
                        me->SetInCombatWithZone();
                        events.ScheduleEvent(EVENT_GORMOK_START, 6.2*IN_MILLISECONDS); // so Gormok will start in Time with DBM warning
                        break;
                    default:
                        break;
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                if (instance && !nextBeastSummoned)
                {
                    instance->SetData(TYPE_NORTHREND_BEASTS, GORMOK_DONE);
                    RG_LOG<EncounterLogModule>(me, EncounterLogModule::Type::KILL);
                }
            }

            void JustReachedHome()
            {
                for (uint8 i = 0; i < MAX_SNOBOLDS; ++i)
                    if (Unit* snobold = me->GetVehicleKit()->GetPassenger(i))
                        snobold->ToCreature()->DespawnOrUnsummon();

                if (instance)
                {
                    instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                    instance->SetData(TYPE_NORTHREND_BEASTS, FAIL);
                    RG_LOG<EncounterLogModule>(me, EncounterLogModule::Type::WIPE);
                }
                me->DespawnOrUnsummon();
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);
                me->SetInCombatWithZone();
                instance->SetData(TYPE_NORTHREND_BEASTS, GORMOK_IN_PROGRESS);
                RG_LOG<EncounterLogModule>(me, EncounterLogModule::Type::PULL);
            }

            void DamageTaken(Unit* /*who*/, uint32& damage)
            {
                // exit the remaining passengers on death
                if (damage >= me->GetHealth())
                    for (uint8 i = 0; i < MAX_SNOBOLDS; ++i)
                    {
                        if (Unit* snobold = me->GetVehicleKit()->GetPassenger(i))
                        {
                            if (Player* target = SelectValidSnoboldTarget())
                            {
                                if (SetPlayerVehicle(target, true))
                                {
                                    snobold->ExitVehicle();
                                    snobold->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                                    snobold->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                                    snobold->EnterVehicle(target);
                                    snobold->AddAura(SPELL_SNOBOLLED, target);
                                    AddThreat(target, 500000, snobold);
                                    snobold->ToCreature()->AI()->SetGuidData(target->GetGUID());
                                }
                            }
                            else
                                snobold->ToCreature()->DespawnOrUnsummon(); // no valid target found
                        }
                    }
            }

            void DoAction(int32 action)
            {
                if (action == ACTION_START_SNOBOLD_TIMER)
                    events.RescheduleEvent(EVENT_JUMP_TO_HAND, 10*IN_MILLISECONDS);
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
                        case EVENT_GORMOK_START:
                            instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                            me->SetWalk(false);
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->SetInCombatWithZone();
                            events.ScheduleEvent(EVENT_IMPALE, 10*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_STAGGERING_STOMP, 15*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_JUMP_TO_HAND, 15*IN_MILLISECONDS);

                            for (uint8 i = 0; i < MAX_SNOBOLDS; i++)
                            {
                                if (Creature* snobold = DoSpawnCreature(NPC_SNOBOLD_VASSAL, 0, 0, 0, 0, TEMPSUMMON_CORPSE_DESPAWN, 0))
                                {
                                    snobold->EnterVehicle(me, i);
                                    snobold->SetInCombatWithZone();
                                    snobold->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                                    snobold->AI()->DoAction(ACTION_ENABLE_FIRE_BOMB);
                                }
                            }
                        return;
                        case EVENT_IMPALE:
                            if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED))
                            {
                                if(me->IsWithinMeleeRange(me->GetVictim()))
                                    DoCastVictim(SPELL_IMPALE, true);
                                else
                                {
                                    events.ScheduleEvent(EVENT_IMPALE, 0.5f * IN_MILLISECONDS);
                                    return;
                                }
                            }
                            events.ScheduleEvent(EVENT_IMPALE, 10*IN_MILLISECONDS);
                            return;
                        case EVENT_STAGGERING_STOMP:
                            DoCastVictim(SPELL_STAGGERING_STOMP);
                            events.ScheduleEvent(EVENT_STAGGERING_STOMP, 20*IN_MILLISECONDS);
                            return;
                        case EVENT_JUMP_TO_HAND:
                            for (uint8 i = 0; i < MAX_SNOBOLDS; ++i)
                            {
                                if (Unit* snobold = me->GetVehicleKit()->GetPassenger(i))
                                {
                                    snobold->ExitVehicle();
                                    snobold->CastSpell(me, SPELL_JUMP_TO_HAND, true);
                                    snoboldGUID = snobold->GetGUID();
                                    snobold->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                                    snobold->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                                    me->AddAura(SPELL_RISING_ANGER, me);
                                    events.ScheduleEvent(EVENT_THROW, 3200);
                                    break;
                                }
                            }
                            return;
                        case EVENT_THROW:
                            if (Unit* snobold = ObjectAccessor::GetUnit(*me, snoboldGUID))
                            {
                                if (Unit* target = SelectValidSnoboldTarget())
                                {
                                    if (Player* player = target->ToPlayer())
                                    {
                                        if (SetPlayerVehicle(player, true))
                                        {
                                            snobold->ExitVehicle();
                                            snobold->EnterVehicle(player);
                                            snobold->AddAura(SPELL_SNOBOLLED, target);
                                            AddThreat(player, 500000, snobold);
                                            snobold->ToCreature()->AI()->SetGuidData(player->GetGUID());
                                            Talk(EMOTE_SNOBOLLED);
                                        }
                                    }
                                }
                                else
                                    snobold->ToCreature()->DespawnOrUnsummon();
                            }
                            events.ScheduleEvent(EVENT_JUMP_TO_HAND, 20*IN_MILLISECONDS);
                            return;
                        case EVENT_NEXT_BEAST:
                            if (instance && !nextBeastSummoned)
                            {
                                instance->SetData(TYPE_NORTHREND_BEASTS, GORMOK_DONE);
                                nextBeastSummoned = true;
                            }
                            return;
                        case EVENT_ENRAGE:
                            DoCastSelf(SPELL_ENRAGE, true);
                            return;
                        default:
                            return;
                    }
                }

                DoMeleeAttackIfReady(true);
            }

            Player* SelectValidSnoboldTarget()
            {
                std::list<Player*> ValidSnoboldTargetList;
                for (Player& player : me->GetMap()->GetPlayers())
                    if (!player.GetVehicleKit() && me->GetVictim() != &player && !player.HasAura(SPELL_SNOBOLLED) && !player.IsGameMaster())
                        ValidSnoboldTargetList.push_back(&player);

                if (ValidSnoboldTargetList.empty())
                    return NULL;

                return Trinity::Containers::SelectRandomContainerElement(ValidSnoboldTargetList);
            }

            private:
                ObjectGuid snoboldGUID;
                bool nextBeastSummoned;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_gormokAI(creature);
        }
};

class npc_snobold_vassal : public CreatureScript
{
    public:
        npc_snobold_vassal() : CreatureScript("npc_snobold_vassal") { }

        struct npc_snobold_vassalAI : public ScriptedAI
        {
            npc_snobold_vassalAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
                SetCombatMovement(false);
            }

            void Reset()
            {
                _events.ScheduleEvent(EVENT_BATTER, 5*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_HEAD_CRACK, 10*IN_MILLISECONDS);

                _targetGUID.Clear();

                if (_instance)
                    _bossGUID = ObjectGuid(_instance->GetGuidData(NPC_GORMOK));
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                if (Player* target = ObjectAccessor::GetPlayer(*me, _targetGUID))
                    SetPlayerVehicle(target, false);

                ScriptedAI::EnterEvadeMode();
            }

            void JustDied(Unit* /*killer*/)
            {
                if (Player* target = ObjectAccessor::GetPlayer(*me, _targetGUID))
                {
                    SetPlayerVehicle(target, false);
                    if (target->IsAlive())
                        target->RemoveAurasDueToSpell(SPELL_SNOBOLLED);
                }
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->SetInCombatWithZone();
            }

            void JustReachedHome()
            {
                me->DespawnOrUnsummon();
            }

            void DoAction(int32 action)
            {
                switch (action)
                {
                    case ACTION_ENABLE_FIRE_BOMB:
                        _events.RescheduleEvent(EVENT_FIRE_BOMB, urand(1, 5)*IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            void SetGuidData(ObjectGuid guid, uint32 /*id*/ = 0) override
            {
                _targetGUID = guid;
            }

            Player* SelectValidSnoboldTarget()
            {
                std::list<Player*> ValidSnoboldTargetList;
                for (Player& player : me->GetMap()->GetPlayers())
                    if (!player.GetVehicleKit() && !player.HasAura(SPELL_SNOBOLLED) && !player.IsGameMaster())
                        ValidSnoboldTargetList.push_back(&player);

                if (ValidSnoboldTargetList.empty())
                    return NULL;

                return Trinity::Containers::SelectRandomContainerElement(ValidSnoboldTargetList);
            }

            Player* SelectValidFireBombTargetOnGormok()
            {
                if (!_instance)
                    return NULL;
                Creature* gormok = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(NPC_GORMOK)));
                if (!gormok)
                    return NULL;
                std::list<Player*> ValidFireBombTargetList;
                for (Player& player : _instance->instance->GetPlayers())
                    if ((player.GetDistance2d(gormok->GetPositionX(), gormok->GetPositionY()) > 15.0f) && !player.IsGameMaster())
                        ValidFireBombTargetList.push_back(&player);
                if (ValidFireBombTargetList.empty())
                    return NULL;
                return Trinity::Containers::SelectRandomContainerElement(ValidFireBombTargetList);
            }

            Player* SelectValidFireBombTargetOnPlayer()
            {
                if (!_instance)
                    return NULL;
                std::list<Player*> ValidFireBombTargetList;
                for (Player& player : _instance->instance->GetPlayers())
                    if ((player.GetDistance2d(me->GetPositionX(), me->GetPositionY()) < 100.0f) && !player.IsGameMaster())
                        ValidFireBombTargetList.push_back(&player);
                if (ValidFireBombTargetList.empty())
                    return NULL;
                return Trinity::Containers::SelectRandomContainerElement(ValidFireBombTargetList);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (Unit* target = ObjectAccessor::GetPlayer(*me, _targetGUID))
                {
                    if (!target->IsAlive())
                    {
                        if (_instance)
                        {
                            Unit* gormok = ObjectAccessor::GetCreature(*me, ObjectGuid(_instance->GetGuidData(NPC_GORMOK)));
                            if (gormok && gormok->IsAlive())
                            {
                                // looping through Gormoks seats
                                for (uint8 i = 0; i < MAX_SNOBOLDS; i++)
                                {
                                    if (!gormok->GetVehicleKit()->GetPassenger(i))
                                    {
                                        me->EnterVehicle(gormok, i);
                                        gormok->ToCreature()->AI()->DoAction(ACTION_START_SNOBOLD_TIMER);
                                        DoAction(ACTION_ENABLE_FIRE_BOMB);
                                        break;
                                    }
                                }
                            }
                            else if (Player* target2 = SelectValidSnoboldTarget())
                            {
                                if (SetPlayerVehicle(target2, true))
                                {
                                    me->EnterVehicle(target2);
                                    me->AddAura(SPELL_SNOBOLLED, target2);
                                    AddThreat(target2, 500000);
                                    _targetGUID = target2->GetGUID();
                                }
                            }
                            else
                                me->DespawnOrUnsummon();
                        }
                    }
                }

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FIRE_BOMB:
                            if (me->GetVehicleBase())
                            {
                                if (me->GetVehicleBase()->GetEntry() == NPC_GORMOK)
                                {
                                    if (Unit* target = SelectValidFireBombTargetOnGormok())
                                        me->CastSpell(target, SPELL_FIRE_BOMB, true);
                                }
                                else if (Unit* target = SelectValidFireBombTargetOnPlayer())
                                    me->CastSpell(target, SPELL_FIRE_BOMB, true);
                            }
                            _events.ScheduleEvent(EVENT_FIRE_BOMB, 20*IN_MILLISECONDS);
                            return;
                        case EVENT_HEAD_CRACK:
                            if (me->GetVehicleBase() && me->GetVehicleBase()->GetEntry() != NPC_GORMOK)
                                if (Unit* target = me->GetVehicleBase())
                                    DoCast(target, SPELL_HEAD_CRACK);
                            _events.ScheduleEvent(EVENT_HEAD_CRACK, urand(7, 10)*IN_MILLISECONDS);
                            return;
                        case EVENT_BATTER:
                            if (me->GetVehicleBase() && me->GetVehicleBase()->GetEntry() != NPC_GORMOK)
                                if (Unit* target = me->GetVehicleBase())
                                    DoCast(target, SPELL_BATTER);
                            _events.ScheduleEvent(EVENT_BATTER, urand(3, 5)*IN_MILLISECONDS);
                            return;
                        default:
                            return;
                    }
                }

                // do melee attack only when not on Gormoks back
                if (me->GetVehicleBase() && me->GetVehicleBase()->GetEntry() != NPC_GORMOK)
                    DoMeleeAttackIfReady(true);
            }

            private:
                EventMap _events;
                InstanceScript* _instance;
                ObjectGuid _bossGUID;
                ObjectGuid _targetGUID;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_snobold_vassalAI(creature);
        }
};

class npc_firebomb : public CreatureScript
{
    public:
        npc_firebomb() : CreatureScript("npc_firebomb") { }

        struct npc_firebombAI : public ScriptedAI
        {
            npc_firebombAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                DoCastSelf(SPELL_FIRE_BOMB_DOT, true);
                SetCombatMovement(false);
                me->SetReactState(REACT_PASSIVE);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid2);
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (_instance && _instance->GetBossState(BOSS_BEASTS) != IN_PROGRESS)
                    me->DespawnOrUnsummon();
            }

            void DamageDealt(Unit* victim, uint32 &damage, DamageEffectType /*damageType*/)
            {
                if (Creature* nearestFire = victim->FindNearestCreature(NPC_FIRE_BOMB, 10.0f))
                {
                    if (nearestFire->GetGUID() != me->GetGUID())
                    {
                        damage = 0;
                    }
                }
            }

            private:
                InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_firebombAI(creature);
        }
};

struct boss_jormungarAI : public BossAI
{
    boss_jormungarAI(Creature* creature) : BossAI(creature, BOSS_BEASTS) { }

    void Reset()
    {
        Enraged = false;
        nextBeastSummoned = false;

        events.RescheduleEvent(EVENT_SPRAY, 13*SEC, 0, PHASE_STATIONARY);
        events.RescheduleEvent(EVENT_SWEEP, 18*SEC, 0, PHASE_STATIONARY);
        events.RescheduleEvent(EVENT_BITE, 6*SEC, 0, PHASE_MOBILE);
        events.RescheduleEvent(EVENT_SPEW, 11*SEC, 0, PHASE_MOBILE);
        events.RescheduleEvent(EVENT_SLIME_POOL, 12*SEC, 0, PHASE_MOBILE);

        if (IsHeroic())
            events.ScheduleEvent(EVENT_NEXT_BEAST, 174*IN_MILLISECONDS); // so Icehowl arrives exactly in time
        else
            events.ScheduleEvent(EVENT_ENRAGE, 15*MINUTE*IN_MILLISECONDS);
    }

    void JustDied(Unit* /*killer*/)
    {
        if (instance)
        {
            if (Creature* otherWorm = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(OtherWormEntry))))
            {
                if (!otherWorm->IsAlive())
                {
                    if (!nextBeastSummoned)
                        instance->SetData(TYPE_NORTHREND_BEASTS, SNAKES_DONE);

                    me->DespawnOrUnsummon();
                    otherWorm->DespawnOrUnsummon();
                }
                else
                    instance->SetData(TYPE_NORTHREND_BEASTS, SNAKES_SPECIAL);
            }
            RG_LOG<EncounterLogModule>(me, EncounterLogModule::Type::KILL);
        }
    }

    void JustReachedHome()
    {
        // prevent losing 2 attempts at once on heroics
        if (instance && instance->GetData(TYPE_NORTHREND_BEASTS) != FAIL)
            instance->SetData(TYPE_NORTHREND_BEASTS, FAIL);

        me->DespawnOrUnsummon();
        
        // make sure both worms despawn
        if (Creature* otherWorm = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(OtherWormEntry))))
        {
            otherWorm->DespawnOrUnsummon();
            RG_LOG<EncounterLogModule>(otherWorm, EncounterLogModule::Type::WIPE);
        }
        RG_LOG<EncounterLogModule>(me, EncounterLogModule::Type::WIPE);
    }

    void EnterCombat(Unit* who)
    {
        BossAI::EnterCombat(who);
        me->SetInCombatWithZone();
        if (instance)
            instance->SetData(TYPE_NORTHREND_BEASTS, SNAKES_IN_PROGRESS);
        RG_LOG<EncounterLogModule>(me, EncounterLogModule::Type::PULL);
    }

    bool DoSpellAttackIfReady(uint32 spell)
    {
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return true;

        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell))
        {
            if (me->IsWithinCombatRange(me->GetVictim(), spellInfo->GetMaxRange(false)))
            {
                me->CastSpell(me->GetVictim(), spell, false);
                me->resetAttackTimer();
                return true;
            }
        }

        return false;
    }

    void DoAction(int32 action)
    {
        if (action == ACTION_SUMMON_NEXT_BEAST)
            nextBeastSummoned = true;
    }

    void UpdateAI(uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if (!Enraged && instance && instance->GetData(TYPE_NORTHREND_BEASTS) == SNAKES_SPECIAL)
        {
            me->RemoveAurasDueToSpell(SPELL_SUBMERGE_0);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            DoCast(SPELL_ENRAGE_SOFT);
            Enraged = true;
            Talk(EMOTE_ENRAGE);
        }

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_EMERGE:
                    Emerge();
                    return;
                case EVENT_SUBMERGE:
                    Submerge();
                    return;
                case EVENT_BITE:
                    DoCastVictim(BiteSpell);
                    events.RescheduleEvent(EVENT_BITE, 15*IN_MILLISECONDS, 0, PHASE_MOBILE);
                    return;
                case EVENT_SPEW:
                    DoCastAOE(SpewSpell);
                    events.RescheduleEvent(EVENT_SPEW, 22*SEC, 0, PHASE_MOBILE);
                    return;
                case EVENT_SLIME_POOL:
                    DoCastSelf(SUMMON_SLIME_POOL);
                    events.RescheduleEvent(EVENT_SLIME_POOL, 12*SEC, 0, PHASE_MOBILE);
                    return;
                case EVENT_SUMMON_ACIDMAW:
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    if (Creature* acidmaw = me->SummonCreature(NPC_ACIDMAW, ToCCommonLoc[9].GetPositionX(), ToCCommonLoc[9].GetPositionY(), ToCCommonLoc[9].GetPositionZ(), 5, TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        acidmaw->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        acidmaw->SetReactState(REACT_AGGRESSIVE);
                        acidmaw->SetInCombatWithZone();
                        acidmaw->CastSpell(acidmaw, SPELL_EMERGE_0);
                    }
                    return;
                case EVENT_SPRAY:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SpraySpell);
                    events.RescheduleEvent(EVENT_SPRAY, 20*SEC, 0, PHASE_STATIONARY);
                    return;
                case EVENT_SWEEP:
                    DoCastAOE(SPELL_SWEEP_0);
                    events.RescheduleEvent(EVENT_SWEEP, 20*SEC, 0, PHASE_STATIONARY);
                    return;
                case EVENT_NEXT_BEAST:
                    if (instance && !nextBeastSummoned)
                    {
                        if (Creature* otherWorm = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(OtherWormEntry))))
                            otherWorm->AI()->DoAction(ACTION_SUMMON_NEXT_BEAST);
                        instance->SetData(TYPE_NORTHREND_BEASTS, SNAKES_DONE);
                        nextBeastSummoned = true;
                    }
                    return;
                case EVENT_ENRAGE:
                    DoCastSelf(SPELL_ENRAGE, true);
                    return;
                default:
                    return;
            }
        }
        if (events.IsInPhase(PHASE_MOBILE))
            DoMeleeAttackIfReady(true);
        if (events.IsInPhase(PHASE_STATIONARY))
            DoSpellAttackIfReady(SpitSpell);
    }

    void Submerge()
    {
        DoCastSelf(SPELL_SUBMERGE_0);
        me->RemoveAurasDueToSpell(SPELL_EMERGE_0);
        me->SetInCombatWithZone();
        events.SetPhase(PHASE_SUBMERGED);
        events.RescheduleEvent(EVENT_EMERGE, 5*SEC, 0, PHASE_SUBMERGED);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->GetMotionMaster()->MovePoint(1, ToCCommonLoc[1].GetPositionX()+ frand(-40.0f, 40.0f), ToCCommonLoc[1].GetPositionY() + frand(-40.0f, 40.0f), ToCCommonLoc[1].GetPositionZ());
        WasMobile = !WasMobile;
    }

    void Emerge()
    {
        DoCastSelf(SPELL_EMERGE_0);
        me->SetDisplayId(ModelMobile);
        me->RemoveAurasDueToSpell(SPELL_SUBMERGE_0);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->GetMotionMaster()->Clear();

        // if the worm was mobile before submerging, make him stationary now
        if (WasMobile)
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
            SetCombatMovement(false);
            me->SetDisplayId(ModelStationary);
            events.SetPhase(PHASE_STATIONARY);
            events.RescheduleEvent(EVENT_SUBMERGE, 40*SEC, 0, PHASE_STATIONARY);
            events.RescheduleEvent(EVENT_SPRAY, 13*SEC, 0, PHASE_STATIONARY);
            events.RescheduleEvent(EVENT_SWEEP, 18*SEC, 0, PHASE_STATIONARY);
        }
        else
        {
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
            SetCombatMovement(true);
            me->GetMotionMaster()->MoveChase(me->GetVictim());
            me->SetDisplayId(ModelMobile);
            events.SetPhase(PHASE_MOBILE);
            events.RescheduleEvent(EVENT_SUBMERGE, 40*SEC, 0, PHASE_MOBILE);
            events.RescheduleEvent(EVENT_BITE, 7*SEC, 0, PHASE_MOBILE);
            events.RescheduleEvent(EVENT_SPEW, 11*SEC, 0, PHASE_MOBILE);
            events.RescheduleEvent(EVENT_SLIME_POOL, 12*SEC, 0, PHASE_MOBILE);
        }
    }

    protected:
        uint32 OtherWormEntry;
        uint32 ModelStationary;
        uint32 ModelMobile;

        uint32 BiteSpell;
        uint32 SpewSpell;
        uint32 SpitSpell;
        uint32 SpraySpell;

        Phases Phase;
        bool Enraged;
        bool WasMobile;
        bool nextBeastSummoned;
};

class boss_acidmaw : public CreatureScript
{
    public:
        boss_acidmaw() : CreatureScript("boss_acidmaw") { }

        struct boss_acidmawAI : public boss_jormungarAI
        {
            boss_acidmawAI(Creature* creature) : boss_jormungarAI(creature) { }

            void Reset()
            {
                if (!me->FindNearestCreature(NPC_DREADSCALE, 2000.0f))
                {
                    // prevent losing 2 attempts at once on heroics
                    if (instance && instance->GetData(TYPE_NORTHREND_BEASTS) != FAIL)
                        instance->SetData(TYPE_NORTHREND_BEASTS, FAIL);

                    me->DespawnOrUnsummon();
        
                    // make sure both worms despawn
                    if (Creature* otherWorm = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(OtherWormEntry))))
                    {
                        otherWorm->DespawnOrUnsummon();
                    }
                }

                boss_jormungarAI::Reset();
                BiteSpell = SPELL_PARALYTIC_BITE;
                SpewSpell = SPELL_ACID_SPEW;
                SpitSpell = SPELL_ACID_SPIT;
                SpraySpell = SPELL_PARALYTIC_SPRAY;
                ModelStationary = MODEL_ACIDMAW_STATIONARY;
                ModelMobile = MODEL_ACIDMAW_MOBILE;
                OtherWormEntry = NPC_DREADSCALE;

                WasMobile = true;
                Emerge();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_acidmawAI(creature);
        }
};

class boss_dreadscale : public CreatureScript
{
    public:
        boss_dreadscale() : CreatureScript("boss_dreadscale") { }

        struct boss_dreadscaleAI : public boss_jormungarAI
        {
            boss_dreadscaleAI(Creature* creature) : boss_jormungarAI(creature) { }

            void Reset()
            {
                boss_jormungarAI::Reset();
                BiteSpell = SPELL_BURNING_BITE;
                SpewSpell = SPELL_MOLTEN_SPEW;
                SpitSpell = SPELL_FIRE_SPIT;
                SpraySpell = SPELL_BURNING_SPRAY;
                ModelStationary = MODEL_DREADSCALE_STATIONARY;
                ModelMobile = MODEL_DREADSCALE_MOBILE;
                OtherWormEntry = NPC_ACIDMAW;

                events.SetPhase(PHASE_MOBILE);
                events.ScheduleEvent(EVENT_SUMMON_ACIDMAW, 3*IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SUBMERGE, 40*IN_MILLISECONDS, 0, PHASE_MOBILE);
                WasMobile = false;
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                switch (pointId)
                {
                    case 0:
                        instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                        me->SetInCombatWithZone();
                        break;
                    default:
                        break;
                }
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                boss_jormungarAI::EnterEvadeMode();
            }

            void JustReachedHome()
            {
                if (instance)
                    instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));

                boss_jormungarAI::JustReachedHome();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_dreadscaleAI(creature);
        }
};

class npc_slime_pool : public CreatureScript
{
    public:
        npc_slime_pool() : CreatureScript("npc_slime_pool") { }

        struct npc_slime_poolAI : public ScriptedAI
        {
            npc_slime_poolAI(Creature* creature) : ScriptedAI(creature)
            {
                _instance = creature->GetInstanceScript();
            }

            void Reset()
            {
                _cast = false;
                me->SetReactState(REACT_PASSIVE);
            }

            void UpdateAI(uint32 /*diff*/)
            {
                if (!_cast)
                {
                    _cast = true;
                    DoCastSelf(SPELL_SLIME_POOL_EFFECT);
                }

                if (_instance->GetData(TYPE_NORTHREND_BEASTS) != SNAKES_IN_PROGRESS && _instance->GetData(TYPE_NORTHREND_BEASTS) != SNAKES_SPECIAL)
                    me->DespawnOrUnsummon();
            }
            private:
                InstanceScript* _instance;
                bool _cast;

        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_slime_poolAI(creature);
        }
};

class spell_gormok_fire_bomb : public SpellScriptLoader
{
    public:
        spell_gormok_fire_bomb() : SpellScriptLoader("spell_gormok_fire_bomb") { }

        class spell_gormok_fire_bomb_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_gormok_fire_bomb_SpellScript);

            void TriggerFireBomb(SpellEffIndex /*effIndex*/)
            {
                if (const WorldLocation* pos = GetExplTargetDest())
                {
                    if (Unit* caster = GetCaster())
                        caster->SummonCreature(NPC_FIRE_BOMB, pos->GetPositionX(), pos->GetPositionY(), pos->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 30*IN_MILLISECONDS);
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_gormok_fire_bomb_SpellScript::TriggerFireBomb, EFFECT_0, SPELL_EFFECT_TRIGGER_MISSILE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_gormok_fire_bomb_SpellScript();
        }
};

class spell_jormungar_dot : public SpellScriptLoader
{
    public:
        spell_jormungar_dot() : SpellScriptLoader("spell_jormungar_dot") { }

        class spell_jormungar_dot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_jormungar_dot_SpellScript);

            bool Load()
            {
                if (sSpellMgr->GetSpellIdForDifficulty(SPELL_PARALYTIC_SPRAY, GetCaster()) == m_scriptSpellId ||
                    sSpellMgr->GetSpellIdForDifficulty(SPELL_PARALYTIC_TOXIN, GetCaster()) == m_scriptSpellId)
                {
                    procSpellId = SPELL_PARALYTIC_TOXIN;
                    triggerEntry = ENTRY_ACIDMAW_TRIGGER;
                    return true;
                }

                if (sSpellMgr->GetSpellIdForDifficulty(SPELL_BURNING_BITE, GetCaster()) == m_scriptSpellId ||
                    sSpellMgr->GetSpellIdForDifficulty(SPELL_BURNING_SPRAY, GetCaster()) == m_scriptSpellId)
                {
                    procSpellId = SPELL_BURNING_BILE;
                    triggerEntry = ENTRY_DREADSCALE_TRIGGER;
                    return true;
                }
                return false;
            }

            void HandleOnHit()
            {
                if (Unit* target = GetHitUnit())
                    if (Unit* caster = GetCaster())
                        if (Creature* trigger = caster->SummonCreature(triggerEntry, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 60*IN_MILLISECONDS))
                            trigger->AI()->DoCast(target, procSpellId, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_jormungar_dot_SpellScript::HandleOnHit);
            }

        private:
            uint32 procSpellId;
            uint32 triggerEntry;

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_jormungar_dot_SpellScript();
        }
};

class boss_icehowl : public CreatureScript
{
    public:
        boss_icehowl() : CreatureScript("boss_icehowl") { }

        struct boss_icehowlAI : public BossAI
        {
            boss_icehowlAI(Creature* creature) : BossAI(creature, BOSS_BEASTS)
            {
                me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_PASSIVE);
                me->SetSpeedRate(MOVE_WALK, 4.0f);
                me->SetWalk(true);
            }

            void Reset()
            {
                events.RescheduleEvent(EVENT_FEROCIOUS_BUTT, 8*SEC);
                events.RescheduleEvent(EVENT_ARCTIC_BREATH, urand(15*SEC, 25*SEC));
                events.RescheduleEvent(EVENT_WHIRL, 15*SEC);
                events.RescheduleEvent(EVENT_ENRAGE, IsHeroic() ? 220*IN_MILLISECONDS : 15*MINUTE*IN_MILLISECONDS); //heroic enrage for the case counting fails, which should not happen
                events.RescheduleEvent(EVENT_MASSIVE_CRASH, 30*SEC);
                events.RescheduleEvent(EVENT_CHECK_LOS, 10*SEC);
                _movementStarted = false;
                _movementFinish = false;
                _trampleCasted = false;
                _trampleTargetGUID.Clear();
                _trampleTargetX = 0;
                _trampleTargetY = 0;
                _trampleTargetZ = 0;
                _trampleOrientation = 0;
                _stage = 0;
                firstTrampleStarted = false;
                stampCount = 0;
                stampCheck = true;
            }

            void JustDied(Unit* killer)
            {
                BossAI::JustDied(killer);
                if (instance)
                    instance->SetData(TYPE_NORTHREND_BEASTS, ICEHOWL_DONE);
                ResetPlayerVehicles(me->GetMap());
            }

            void MovementInform(uint32 type, uint32 pointId)
            {
                if (type != POINT_MOTION_TYPE && type != EFFECT_MOTION_TYPE)
                    return;

                switch (pointId)
                {
                    case 0:
                        if (_stage != 0)
                        {
                            if (me->GetDistance2d(ToCCommonLoc[1].GetPositionX(), ToCCommonLoc[1].GetPositionY()) < 6.0f && stampCheck)
                            {
                                // Middle of the room and stop moving
                                me->GetMotionMaster()->Clear();
                                me->StopMoving();
                                me->AttackStop();
                                // Teleport to prevent Icehowl moving strange in the middle of the room - Player wont notice it anyway
                                me->NearTeleportTo(ToCCommonLoc[1].GetPositionX(), ToCCommonLoc[1].GetPositionY(), ToCCommonLoc[1].GetPositionZ(), 0.0f);
                                me->GetMotionMaster()->MovementExpired();
                                _stage = 1;
                            }
                            else
                            {
                                // Landed from Hop backwards (start trample)
                                if (ObjectAccessor::GetPlayer(*me, _trampleTargetGUID))
                                {
                                    _stage = 4;
                                    events.ScheduleEvent(EVENT_ICEHOWL_CHARGE, 1*SEC);
                                }
                                else
                                    _stage = 6;
                            }
                        }
                        break;
                    case 1: // Finish trample
                        _movementFinish = true;
                        break;
                    case 2:
                        me->SetInCombatWithZone();
                        events.ScheduleEvent(EVENT_ICEHOWL_START, 1*SEC);
                        break;
                    default:
                        break;
                }
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                ScriptedAI::EnterEvadeMode();
            }

            void JustReachedHome()
            {
                if (instance)
                {
                    instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                    instance->SetData(TYPE_NORTHREND_BEASTS, FAIL);
                }
                me->DespawnOrUnsummon();
            }

            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);
                if (instance)
                    instance->SetData(TYPE_NORTHREND_BEASTS, ICEHOWL_IN_PROGRESS);
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                if (spell->Id == SPELL_TRAMPLE && target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (!_trampleCasted)
                    {
                        DoCastSelf(SPELL_FROTHING_RAGE, true);
                        _trampleCasted = true;
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                switch (_stage)
                {
                    case 0:
                    {
                        while (uint32 eventId = events.ExecuteEvent())
                        {
                            switch (eventId)
                            {
                                case EVENT_ICEHOWL_START:
                                    instance->DoUseDoorOrButton(ObjectGuid(instance->GetGuidData(GO_MAIN_GATE_DOOR)));
                                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                                    me->SetWalk(false);
                                    me->SetReactState(REACT_AGGRESSIVE);
                                    return;
                                case EVENT_FEROCIOUS_BUTT:
                                    DoCastVictim(SPELL_FEROCIOUS_BUTT);
                                    events.RescheduleEvent(EVENT_FEROCIOUS_BUTT, 16*SEC);
                                    return;
                                case EVENT_ARCTIC_BREATH:
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                    {
                                        SetCombatMovement(false);
                                        me->GetMotionMaster()->Clear();
                                        me->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f);
                                        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);
                                        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                                        me->ApplySpellImmune(0, IMMUNITY_ID, 7922, true); // Charge Stun
                                        DoCast(target, SPELL_ARCTIC_BREATH);
                                        events.ScheduleEvent(EVENT_ARCTIC_BREATH_ENABLE_MOVE, 0.5*IN_MILLISECONDS);
                                    }
                                    return;
                                case EVENT_ARCTIC_BREATH_ENABLE_MOVE:
                                    SetCombatMovement(true);
                                    me->GetMotionMaster()->MoveChase(me->GetVictim());
                                    me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
                                    me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
                                    if (instance)
                                    {
                                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_ARCTIC_BREATH);
                                        instance->DoRemoveAurasDueToSpellOnPlayers(67650); // All Difficulties of Arctic Breath
                                        instance->DoRemoveAurasDueToSpellOnPlayers(67651);
                                        instance->DoRemoveAurasDueToSpellOnPlayers(67652);
                                    }
                                    me->resetAttackTimer();
                                    return;
                                case EVENT_WHIRL:
                                    DoCastVictim(SPELL_WHIRL);
                                    if (firstTrampleStarted)
                                        events.RescheduleEvent(EVENT_WHIRL, 15*SEC);
                                    return;
                                case EVENT_MASSIVE_CRASH:
                                    me->AttackStop();
                                    me->GetMotionMaster()->MoveJump(ToCCommonLoc[1], 40.0f, 20.0f, 0); // 1: Middle of the room
                                    SetCombatMovement(false);
                                    _stage = 7; //Invalid (Do nothing more than move)
                                    firstTrampleStarted = true;
                                    stampCheck = true;
                                    events.ScheduleEvent(EVENT_MASSIVE_CRASH, 60*IN_MILLISECONDS);
                                    return;
                                case EVENT_ENRAGE:
                                    DoCastSelf(SPELL_ENRAGE, true);
                                    return;
                                case EVENT_CHECK_LOS:
                                    if (Unit* target = me->GetVictim())
                                        if (!me->IsWithinLOS(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ()))
                                            me->NearTeleportTo(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f);
                                    events.RescheduleEvent(EVENT_CHECK_LOS, 10*SEC);
                                    return;
                                default:
                                    break;
                            }
                        }
                        DoMeleeAttackIfReady(true);
                        break;
                    }
                    case 1:
                        if (stampCheck) // only cast Spell if not Casted before - prevents double Massive_Crash.
                        {
                            instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_FEROCIOUS_BUTT); // to prevent Tank gets not knockbacked from Massive Crash
                            instance->DoRemoveAurasDueToSpellOnPlayers(67656); // all difficulties of Ferocius Butt
                            instance->DoRemoveAurasDueToSpellOnPlayers(67654);
                            instance->DoRemoveAurasDueToSpellOnPlayers(67655);
                            DoCastAOE(SPELL_MASSIVE_CRASH);
                        }
                        me->StopMoving();
                        me->AttackStop();
                        stampCheck = false;
                        _stage = 2;
                        break;
                    case 2:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        {
                            if (IsHeroic())
                            {
                                ++stampCount;
                                if (stampCount == 4)
                                    me->AddAura(SPELL_ENRAGE, me);
                            }
                            me->StopMoving();
                            me->AttackStop();
                            _trampleTargetGUID = target->GetGUID();
                            me->SetTarget(_trampleTargetGUID);
                            _trampleCasted = false;
                            SetCombatMovement(false);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MoveIdle();
                            events.ScheduleEvent(EVENT_STARE, 4*IN_MILLISECONDS);
                            events.ScheduleEvent(EVENT_ROAWR, 4200);
                            events.ScheduleEvent(EVENT_TRAMPLE, 6500);
                            _stage = 3;
                        }
                        else
                            _stage = 6;
                        break;
                    case 3:
                        while (uint32 eventId = events.ExecuteEvent())
                        {
                            switch (eventId)
                            {
                                case EVENT_STARE:
                                {
                                    if (Unit* target = ObjectAccessor::GetPlayer(*me, _trampleTargetGUID))
                                    {
                                        Talk(EMOTE_TRAMPLE_START, target);
                                        _trampleTargetX = me->GetPositionX() + 0.85f * (target->GetPositionX() - me->GetPositionX());
                                        _trampleTargetY = me->GetPositionY() + 0.85f * (target->GetPositionY() - me->GetPositionY());
                                        _trampleTargetZ = target->GetPositionZ() + 0.5f; // to prevent casting of Trample under the map
                                        _trampleOrientation = me->GetAngle(_trampleTargetX, _trampleTargetY);
                                        me->AttackStop();
                                        me->SetReactState(REACT_PASSIVE); // so Icehowl can face to the target
                                        me->SetGuidValue(UNIT_FIELD_TARGET, _trampleTargetGUID);
                                        me->SetOrientation(_trampleOrientation);
                                    }
                                break;
                                }
                                case EVENT_ROAWR:
                                    me->HandleEmoteCommand(EMOTE_ONESHOT_BATTLE_ROAR); // Roawr Emote
                                    break;
                                case EVENT_TRAMPLE:
                                {
                                    if (Unit* target = ObjectAccessor::GetPlayer(*me, _trampleTargetGUID))
                                    {
                                        me->StopMoving();
                                        _trampleCasted = false;
                                        _movementStarted = true;
                                        // 2: Hop Backwards which looks way better than MoveJump
                                        me->GetMotionMaster()->MoveKnockbackFrom(_trampleTargetX, _trampleTargetY, 40.0f, 2.5f);
                                        _stage = 7; //Invalid (Do nothing more than move)
                                    }
                                    else
                                        _stage = 6;
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                        break;
                    case 4:
                        if (events.ExecuteEvent() == EVENT_ICEHOWL_CHARGE)
                        {
                            if (Unit* target = ObjectAccessor::GetPlayer(*me, _trampleTargetGUID))
                            {
                                me->StopMoving();
                                me->AttackStop();
                                me->SetReactState(REACT_AGGRESSIVE);
                                me->GetMotionMaster()->MoveCharge(_trampleTargetX, _trampleTargetY, _trampleTargetZ, 80, 1);
                                _stage = 5;
                            }
                        }
                        break;
                    case 5:
                        if (_movementFinish)
                        {
                            DoCastAOE(SPELL_TRAMPLE);
                            _movementFinish = false;

                            // Range check in case SPELL_TRAMPLE did non Hit a Target in range (expl: Lag)
                            for (Player& player : me->GetMap()->GetPlayers())
                            {
                                if (player.IsAlive() && player.GetDistance2d(_trampleTargetX, _trampleTargetY) < 12.0f && !_trampleCasted)
                                {
                                    me->NearTeleportTo(_trampleTargetX, _trampleTargetY, _trampleTargetZ, 0.0f);
                                    DoCastAOE(SPELL_TRAMPLE); // in case someone stands in Range and first Trample failed to hit him
                                    _trampleCasted = true;
                                }
                            }
                            _stage = 6;
                        }
                        break;
                    case 6:
                        if (!_trampleCasted)
                        {
                            DoCastSelf(SPELL_STAGGERED_DAZE);
                            me->SetOrientation(_trampleOrientation);
                            Movement::MoveSplineInit init(me);
                            init.MoveTo(_trampleTargetX, _trampleTargetY, _trampleTargetZ);
                            init.SetFacing(_trampleOrientation);
                            init.Launch();
                            Talk(EMOTE_TRAMPLE_CRASH);
                        }
                        else
                        {
                            DoCastSelf(SPELL_FROTHING_RAGE, true);
                            Talk(EMOTE_TRAMPLE_FAIL);
                        }
                        _movementStarted = false;
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                        SetCombatMovement(true);
                        me->GetMotionMaster()->MovementExpired();
                        me->GetMotionMaster()->Clear();
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                        AttackStart(me->GetVictim());
                        events.ScheduleEvent(EVENT_ARCTIC_BREATH, !_trampleCasted ? 17*SEC : 2*SEC);
                        events.ScheduleEvent(EVENT_ARCTIC_BREATH, !_trampleCasted ? 42*SEC : 27*SEC);
                        events.RescheduleEvent(EVENT_WHIRL, !_trampleCasted ? urand(18, 23)*SEC : urand(3, 8)*SEC);
                        events.RescheduleEvent(EVENT_FEROCIOUS_BUTT, !_trampleCasted ? 16*SEC : 1*SEC);
                        _stage = 0;
                        break;
                    default:
                        break;
                }
            }

            private:
                float  _trampleTargetX, _trampleTargetY, _trampleTargetZ, _trampleOrientation;
                ObjectGuid _trampleTargetGUID;
                bool   _movementStarted;
                bool   _movementFinish;
                bool   _trampleCasted;
                bool   firstTrampleStarted;
                uint8  _stage;
                uint32 stampCount;
                bool stampCheck;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_icehowlAI(creature);
        }
};

void AddSC_boss_northrend_beasts()
{
    new boss_gormok();
    new npc_snobold_vassal();
    new npc_firebomb();
    new spell_gormok_fire_bomb();

    new boss_acidmaw();
    new boss_dreadscale();
    new npc_slime_pool();
    new spell_jormungar_dot();

    new boss_icehowl();
}
