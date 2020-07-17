/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2012-2015 Rising-Gods <http://www.rising-gods.de/>
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
#include "ScriptedEscortAI.h"
#include "Vehicle.h"
#include "trial_of_the_champion.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellScript.h"

enum Spells
{
    //Vehicle
    SPELL_CHARGE                    = 63010,
    SPELL_SHIELD_BREAKER            = 68504,
    SPELL_SHIELD                    = 62719, // used by boss vehicles
    SPELL_SHIELD_NO_VISUAL          = 64100, // used by add vehicles
    SPELL_THRUST                    = 62544,

    SPELL_ROLLING_THROW             = 11428, // correct spell?

    SPELL_FEIGN_DEATH               = 66804,

    // Marshal Jacob Alerius && Mokra the Skullcrusher || Warrior
    SPELL_MORTAL_STRIKE             = 68783,
    SPELL_MORTAL_STRIKE_H           = 68784,
    SPELL_BLADESTORM                = 63784,
    SPELL_INTERCEPT                 = 67540,
    SPELL_ROLLING_THROW_BOSS        = 67546,

    // Ambrose Boltspark && Eressea Dawnsinger || Mage
    SPELL_FIREBALL                  = 66042,
    SPELL_FIREBALL_H                = 68310,
    SPELL_BLAST_WAVE                = 66044,
    SPELL_BLAST_WAVE_H              = 68312,
    SPELL_HASTE                     = 66045,
    SPELL_POLYMORPH                 = 66043,
    SPELL_POLYMORPH_H               = 68311,

    // Colosos && Runok Wildmane || Shaman
    SPELL_CHAIN_LIGHTNING           = 67529,
    SPELL_CHAIN_LIGHTNING_H         = 68319,
    SPELL_EARTH_SHIELD              = 67530,
    SPELL_HEALING_WAVE              = 67528,
    SPELL_HEALING_WAVE_H            = 68318,
    SPELL_HEX_OF_MENDING            = 67534,

    // Jaelyne Evensong && Zul'tore || Hunter
    SPELL_DISENGAGE                 = 68340,
    SPELL_LIGHTNING_ARROWS          = 66083, // 66085 is applyed on caster if aura is removed. handled in db
    SPELL_MULTISHOT                 = 66081,
    SPELL_SHOOT                     = 65868,

    // Lana Stouthammer Evensong && Deathstalker Visceri || Rouge
    SPELL_EVISCERATE                = 67709,
    SPELL_EVISCERATE_H              = 68317,
    SPELL_FAN_OF_KNIVES             = 67706,
    SPELL_POISON_BOTTLE             = 67701,
    SPELL_DEADLY_POISON             = 67711,

    SPELL_DEFENSE_PLAYER            = 62552
};

enum Yells
{
    SAY_TRAMPLED        = 0
};

Position const MovementPoint[] =
{
    {746.84f, 623.15f, 411.41f},
    {747.96f, 620.29f, 411.09f},
    {750.23f, 618.35f, 411.09f}
};

enum Events
{
    // mounted & phase vehicle
    EVENT_VEHICLE_BUFF  = 1,
    EVENT_RANDOM_MOVEMENT,
    EVENT_VEHICLE_CHARGE,
    EVENT_VEHICLE_SHIELD_BREAKER,

    // knocked & phase vehicle
    EVENT_CHECK_MOUNTED,
    EVENT_CAN_KNOCKED,

    // warrior
    EVENT_BLADE_STORM,
    EVENT_INTERCEPT,
    EVENT_MORTAL_STRIKE,
    EVENT_ROLLING_THROW_BOSS,

    // mage
    EVENT_POLYMORPH,
    EVENT_BLAST_WAVE,
    EVENT_HASTE,

    // shaman
    EVENT_CHAIN_LIGHTNING,
    EVENTS_EARTH_SHIELD,
    EVENT_HEALING_WAVE,
    EVENT_HEX_MENDING,

    // hunter
    EVENT_DISENGAGE,
    EVENT_MULTISHOT,
    EVENT_LIGHTNING_ARROWS,
    EVENT_CHECK_LOS,

    // rouge
    EVENT_EVISCERATE,
    EVENT_FAN_KIVES,
    EVENT_POISON_BOTTLE,
};

enum Phases
{
    PHASE_VEHICLE       = 1,
    PHASE_NON_VEHICLE   = 2,
};

enum MovementPoints
{
    MOVEMENT_POINT_RANDOM   = 100,
    MOVEMENT_POINT_WALL     = 101,
    MOVEMENT_POINT_GATE     = 102,
};

enum Misc
{
    ARENA_RADIUS        = 48,
};

Position const Middle = {746.927f, 618.101f, 411.090f};

bool IsGrandChampion(uint32 entry)
{
    switch (entry)
    {
    case NPC_MOKRA:
    case NPC_ERESSEA:
    case NPC_RUNOK:
    case NPC_ZULTORE:
    case NPC_VISCERI:
    case NPC_JACOB:
    case NPC_AMBROSE:
    case NPC_COLOSOS:
    case NPC_JAELYNE:
    case NPC_LANA:
        return true;
    default:
        return false;
    }
}

uint32 GetAchievmentCriteriaIdByEntry(uint32 entry, bool heroic)
{
    switch (entry)
    {
    case NPC_MOKRA:     return heroic ? 12318 : 12302;
    case NPC_ERESSEA:   return heroic ? 12321 : 12305;
    case NPC_RUNOK:     return heroic ? 12320 : 12304;
    case NPC_ZULTORE:   return heroic ? 12322 : 12306;
    case NPC_VISCERI:   return heroic ? 12319 : 12303;
    case NPC_JACOB:     return heroic ? 12310 : 11420;
    case NPC_AMBROSE:   return heroic ? 12313 : 12300;
    case NPC_COLOSOS:   return heroic ? 12312 : 12299;
    case NPC_JAELYNE:   return heroic ? 12314 : 12301;
    case NPC_LANA:      return heroic ? 12311 : 12298;
    default:            return 0;
    }
}

class vehicle_champion_toc5 : public CreatureScript
{
public:
    vehicle_champion_toc5() : CreatureScript("vehicle_champion_toc5") { }

    struct vehicle_champion_toc5AI : public ScriptedAI
    {
        vehicle_champion_toc5AI(Creature* creature) : ScriptedAI(creature) {}

        void MovementInform(uint32 type, uint32 id)
        {
            if (!me->IsVehicle())
                return;

            if (Unit* passenger = me->GetVehicleKit()->GetPassenger(0))
                if (Creature* rider = passenger->ToCreature())
                    rider->AI()->MovementInform(type, id);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            if (InstanceScript* instance = me->GetInstanceScript())
                instance->SetData(DATA_CANCEL_FIRST_ENCOUNTER, 0);
        }

        void PassengerBoarded(Unit* who, int8 seatId, bool apply)
        {
            if (apply)
                who->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void SpellHitTarget(Unit* target, SpellInfo const* spell)
        {
            if (spell->SpellIconID == 1716)
                me->GetMotionMaster()->MoveChase(target);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        if (creature->GetMap()->GetId() == 650)
            return new vehicle_champion_toc5AI(creature);

        return NULL;
    }
};

struct generic_champion_toc5AI : public ScriptedAI
{
    generic_champion_toc5AI(Creature* creature) : ScriptedAI(creature)
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetImmuneToPC(true);
        me->setFaction(FACTION_HOSTILE);
        me->SetReactState(REACT_PASSIVE);

        instance = creature->GetInstanceScript();
    }

    void AddWaypoint(uint8 /*id*/, float x, float y, float z)
    {
        Position pos;
        pos.Relocate(x, y, z);
        positions.push_back(pos);
    }

    void DoAction(int32 action)
    {
        positions.clear();
        switch (action)
        {
            case ACTION_GRAND_CHAMPIONS_MOVE_1:
                AddWaypoint(0, 746.451f, 647.031f, 411.57f);
                AddWaypoint(1, 771.434f, 642.606f, 411.91f);
                AddWaypoint(2, 779.807f, 617.535f, 411.71f);
                AddWaypoint(3, 771.098f, 594.635f, 411.62f);
                AddWaypoint(4, 746.887f, 583.425f, 411.66f);
                AddWaypoint(5, 715.176f, 583.782f, 412.39f);
                AddWaypoint(6, 720.719f, 591.141f, 411.73f);
                break;
            case ACTION_GRAND_CHAMPIONS_MOVE_2:
                AddWaypoint(0, 746.451f, 647.031f, 411.57f);
                AddWaypoint(1, 771.434f, 642.606f, 411.91f);
                AddWaypoint(2, 779.807f, 617.535f, 411.71f);
                AddWaypoint(3, 771.098f, 594.635f, 411.62f);
                AddWaypoint(4, 746.887f, 583.425f, 411.66f);
                AddWaypoint(5, 746.161f, 571.678f, 412.38f);
                AddWaypoint(6, 746.887f, 583.425f, 411.66f);
                break;
            case ACTION_GRAND_CHAMPIONS_MOVE_3:
                AddWaypoint(0, 746.451f, 647.031f, 411.57f);
                AddWaypoint(1, 771.434f, 642.606f, 411.91f);
                AddWaypoint(2, 779.807f, 617.535f, 411.71f);
                AddWaypoint(3, 771.098f, 594.635f, 411.62f);
                AddWaypoint(4, 777.759f, 584.577f, 412.39f);
                AddWaypoint(5, 772.481f, 592.991f, 411.68f);
                break;
        }
        moveCounter = 0;

        if (Creature* base = ObjectAccessor::GetCreature(*me, vehicleGUID))
            base->GetMotionMaster()->MovePoint(moveCounter, positions[5]);
    }

    void Reset()
    {
        wasInfight = false;

        events.Reset();

        events.ScheduleEvent(EVENT_VEHICLE_BUFF, urand(30, 60)*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_RANDOM_MOVEMENT, urand (10, 15)*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_VEHICLE_CHARGE, 5*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_VEHICLE_SHIELD_BREAKER, 8*IN_MILLISECONDS);
    }

    void SetGuidData(ObjectGuid guid, uint32 /*id*/) override
    {
        vehicleGUID = guid;
        if (Creature* base = ObjectAccessor::GetCreature(*me, guid))
        {
            base->setFaction(FACTION_HOSTILE);  // needed for SPELL_CHARGE
            base->SetReactState(REACT_PASSIVE);
            base->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            base->SetImmuneToPC(true);
        }
    }

    void CastShield()
    {
        if (IsGrandChampion(me->GetEntry()))
        {
            DoCastSelf(SPELL_SHIELD);
            events.RescheduleEvent(EVENT_VEHICLE_BUFF, urand(15, 20)*IN_MILLISECONDS);
        }
        else
        {
            DoCastSelf(SPELL_SHIELD_NO_VISUAL);
            events.RescheduleEvent(EVENT_VEHICLE_BUFF, urand(30, 60)*IN_MILLISECONDS);
        }
    }

    void EnterCombat(Unit* /*who*/)
    {
        CastShield();
        if (IsGrandChampion(me->GetEntry()))
        {
            DoCastSelf(SPELL_SHIELD);
            DoCastSelf(SPELL_SHIELD);
        }

        wasInfight = true;
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (instance)
            instance->SetData(DATA_CANCEL_FIRST_ENCOUNTER, 0);
    }

    void MovementInform(uint32 type, uint32 id)
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (id == MOVEMENT_POINT_RANDOM)
        {
            switch (urand(0, 1))
            {
                case 0:
                    events.RescheduleEvent(EVENT_VEHICLE_CHARGE, 100);
                    break;
                case 1:
                    events.RescheduleEvent(EVENT_VEHICLE_SHIELD_BREAKER, 100);
                    break;
            }
        }
        else if (id == moveCounter && id != positions.size())
        {
            ++moveCounter;
            WPReached = true;
        }
        else if (id == positions.size())
        {
            me->SetFacingTo(M_PI_F / 2.0f);
        }
    }

    bool UpdateVictim()
    {
        if (!me->IsInCombat())
            return false;

        if (!me->HasReactState(REACT_PASSIVE))
        {
            if (Unit* victim = me->SelectVictim())
            {
                AttackStart(victim);
                if (Creature* base = ObjectAccessor::GetCreature(*me, vehicleGUID))
                {
                    if (base->Attack(victim, true))
                        base->GetMotionMaster()->MoveChase(victim);
                }
            }
            return me->GetVictim();
        }
        else if (me->GetThreatManager().isThreatListEmpty())
        {
            EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
            return false;
        }

        return true;
    }

    void ExecuteEvent(uint32 eventId) { // no override because this is not a BossAI
        switch (eventId)
        {
        case EVENT_VEHICLE_BUFF:
            CastShield();
            break;
        case EVENT_RANDOM_MOVEMENT:
        {
            Position pos;
            pos.Relocate(me);
            if (Creature* base = ObjectAccessor::GetCreature(*me, vehicleGUID))
            {
                pos = base->MovePositionToFirstCollision2D(pos, frand(10.0f, 20.0f), frand(0.0f, 2.0f * M_PI_F) + base->GetOrientation());
                base->GetMotionMaster()->MovePoint(MOVEMENT_POINT_RANDOM, pos);
            }
            events.RescheduleEvent(EVENT_RANDOM_MOVEMENT, urand(7, 10)*IN_MILLISECONDS);
            break;
        }
        case EVENT_VEHICLE_CHARGE:
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
            {
                Unit* victim = target;
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (target->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
                        victim = target->GetVehicleBase();
                    else
                    {
                        DoCast(target, SPELL_ROLLING_THROW);
                        events.RescheduleEvent(EVENT_VEHICLE_CHARGE, 2 * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_RANDOM_MOVEMENT, urand(8, 10)*IN_MILLISECONDS);
                        break;
                    }
                }

                if (Creature* base = ObjectAccessor::GetCreature(*me, vehicleGUID))
                    base->AI()->DoCast(victim, SPELL_CHARGE);
            }
            events.RescheduleEvent(EVENT_VEHICLE_CHARGE, urand(10, 15)*IN_MILLISECONDS);
            break;
        case EVENT_VEHICLE_SHIELD_BREAKER:
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
            {
                Unit* victim = target;
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    if (target->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
                        victim = target->GetVehicleBase();
                    else
                    {
                        DoCast(target, SPELL_ROLLING_THROW);
                        events.RescheduleEvent(EVENT_VEHICLE_CHARGE, 2 * IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_RANDOM_MOVEMENT, urand(8, 10)*IN_MILLISECONDS);
                        break;
                    }
                }

                DoCast(victim, SPELL_SHIELD_BREAKER);
            }
            events.RescheduleEvent(EVENT_VEHICLE_SHIELD_BREAKER, urand(10, 15)*IN_MILLISECONDS);
            break;
        }
    }

    void UpdateAI(uint32 diff) 
    {
        if (WPReached)
        {
            WPReached = false;
            if (moveCounter < positions.size())
                if (Creature* base = ObjectAccessor::GetCreature(*me, vehicleGUID))
                    base->GetMotionMaster()->MovePoint(moveCounter, positions[moveCounter]);
        }

        if (!UpdateVictim())
        {
            if (wasInfight)
            {
                if (Player* player = me->FindNearestPlayer(20.0f))
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                    {
                        Unit* base = &player;
                        if (player.HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
                            base = player.GetVehicleBase();

                        AddThreat(base, frand(0.0f, 10.0f));
                        me->SetInCombatWith(base);
                        player.SetInCombatWith(me);
                    }
                }
            }
            
            return;
        }

        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        DoSpellAttackIfReady(SPELL_THRUST);
    }

protected:
    InstanceScript* instance;
    ObjectGuid vehicleGUID = ObjectGuid::Empty;
    EventMap events;
private:
    uint8 moveCounter;
    bool WPReached;
    bool wasInfight;
    std::vector<Position> positions;
};

struct boss_argent_championAI : public BossAI
{
    boss_argent_championAI(Creature* creature) : BossAI(creature, BOSS_GRAND_CHAMPIONS) 
    {
        me->SetReactState(REACT_AGGRESSIVE);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetImmuneToPC(false);
    }

    void Reset()
    {
        defeated = false;
    }

    void DamageTaken(Unit* /*who*/, uint32& damage)
    {
        // sometimes DamageTaken is called several times even if boss is already friendly
        if (defeated)
            return;

        if (damage >= me->GetHealth())
        {
            damage = 0;
            defeated = true;

            me->RemoveAllAuras();
            me->StopMoving();
            me->SetControlled(true, UNIT_STATE_STUNNED);
            me->CombatStop(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->setFaction(FACTION_FRIENDLY_TO_ALL);
            me->SetHealth(me->GetMaxHealth());

            if (!instance)
                return;

            if (Creature* GrandChampion1 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_1))))
                if (GrandChampion1->getFaction() != FACTION_FRIENDLY_TO_ALL)
                    return;
            if (Creature* GrandChampion2 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_2))))
                if (GrandChampion2->getFaction() != FACTION_FRIENDLY_TO_ALL)
                    return;
            if (Creature* GrandChampion3 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_3))))
                if (GrandChampion3->getFaction() != FACTION_FRIENDLY_TO_ALL)
                    return;

            for (uint8 i=0; i<3; ++i)
                if (Creature* GrandChampion = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_1 +i))))
                {
                    GrandChampion->DespawnOrUnsummon(5*IN_MILLISECONDS);
                    if (IsHeroic())
                    { 
                        instance->DoCompleteCriteria(GetAchievmentCriteriaIdByEntry(GrandChampion->GetEntry(), IsHeroic()));
                        instance->DoCompleteCriteria(GetAchievmentCriteriaIdByEntry(GrandChampion->GetEntry(), false));
                    }
                    else
                        instance->DoCompleteCriteria(GetAchievmentCriteriaIdByEntry(GrandChampion->GetEntry(), IsHeroic()));
                }

            if (instance->GetBossState(BOSS_GRAND_CHAMPIONS) != DONE)
                damage = me->GetMaxHealth() + 1;
        }
    }

    void JustReachedHome()
    {
        if (instance)
        {
            for (uint8 i=0; i<3; ++i)
                if (Creature* GrandChampion = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_1 +i))))
                    if (GrandChampion->isDead())
                    {
                        GrandChampion->Respawn();
                        GrandChampion->GetMotionMaster()->MoveTargetedHome();
                    }
        }
    }

private:
    bool defeated;
};

struct boss_warrior_toc5AI : public boss_argent_championAI
{
    boss_warrior_toc5AI(Creature* creature) : boss_argent_championAI(creature) {}

    void EnterCombat(Unit* /*who*/)
    {
        events.ScheduleEvent(EVENT_BLADE_STORM, urand(15, 20)*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_INTERCEPT, 7*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(8, 12)*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_ROLLING_THROW_BOSS, 25*IN_MILLISECONDS);
    }

    void UpdateAI(uint32 diff)
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_BLADE_STORM:
                    DoCastVictim(SPELL_BLADESTORM);
                    events.RescheduleEvent(EVENT_BLADE_STORM, urand(15, 20)*IN_MILLISECONDS);
                    break;
                case EVENT_INTERCEPT:
                    DoCast(SPELL_INTERCEPT);
                    events.ScheduleEvent(EVENT_INTERCEPT, 7*IN_MILLISECONDS);
                    break;
                case EVENT_MORTAL_STRIKE:
                    DoCastVictim(SPELL_MORTAL_STRIKE);
                    events.ScheduleEvent(EVENT_MORTAL_STRIKE, urand(8, 12)*IN_MILLISECONDS);
                    break;
                case EVENT_ROLLING_THROW_BOSS:
                    me->GetVictim()->CastSpell(me->GetVictim(), SPELL_ROLLING_THROW_BOSS);
                    events.ScheduleEvent(EVENT_ROLLING_THROW_BOSS, 25*IN_MILLISECONDS);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }
};

struct boss_mage_toc5AI : public boss_argent_championAI
{
    boss_mage_toc5AI(Creature* creature) : boss_argent_championAI(creature) {}

    void EnterCombat(Unit* /*who*/)
    {
        events.RescheduleEvent(EVENT_POLYMORPH, 8*IN_MILLISECONDS);
        events.RescheduleEvent(EVENT_BLAST_WAVE, 12*IN_MILLISECONDS);
        events.RescheduleEvent(EVENT_HASTE, 22*IN_MILLISECONDS);
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
                case EVENT_POLYMORPH:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_POLYMORPH);
                    events.RescheduleEvent(EVENT_POLYMORPH, 8*IN_MILLISECONDS);
                    break;
                case EVENT_BLAST_WAVE:
                    DoCastAOE(SPELL_BLAST_WAVE, false);
                    events.RescheduleEvent(EVENT_BLAST_WAVE, 13*IN_MILLISECONDS);
                    break;
                case EVENT_HASTE:
                    me->InterruptNonMeleeSpells(true);
                    DoCastSelf(SPELL_HASTE);
                    events.RescheduleEvent(EVENT_HASTE, 40*IN_MILLISECONDS);
                    break;
            }
        }
        DoSpellAttackIfReady(SPELL_FIREBALL);
    }
};

struct boss_shaman_toc5AI : public boss_argent_championAI
{
    boss_shaman_toc5AI(Creature* creature) : boss_argent_championAI(creature) {}


    void EnterCombat(Unit* who)
    {
        DoCastSelf(SPELL_EARTH_SHIELD);
        DoCast(who, SPELL_HEX_OF_MENDING);

        events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 16*IN_MILLISECONDS);
        events.ScheduleEvent(EVENTS_EARTH_SHIELD, urand(30, 35)*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_HEALING_WAVE, 12*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_HEX_MENDING, urand(20, 25)*IN_MILLISECONDS);
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
                case EVENT_CHAIN_LIGHTNING:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_CHAIN_LIGHTNING);
                    events.RescheduleEvent(EVENT_CHAIN_LIGHTNING, 16*IN_MILLISECONDS);
                    break;
                case EVENT_HEALING_WAVE:
                    {
                        bool Chance = urand(0, 1);
                        if (!Chance)
                        {
                            if(Unit* champion = DoSelectLowestHpFriendly(40))
                                DoCast(champion, SPELL_HEALING_WAVE);
                        }
                        else
                            DoCastSelf(SPELL_HEALING_WAVE);
                    }
                    events.RescheduleEvent(EVENT_HEALING_WAVE, 12*IN_MILLISECONDS);
                    break;
                case EVENTS_EARTH_SHIELD:
                    DoCastSelf(SPELL_EARTH_SHIELD);
                    events.RescheduleEvent(EVENTS_EARTH_SHIELD, urand(30, 35)*IN_MILLISECONDS);
                    break;
                case EVENT_HEX_MENDING:
                    DoCastVictim(SPELL_HEX_OF_MENDING, true);
                    events.RescheduleEvent(EVENT_HEX_MENDING, urand(20, 25)*IN_MILLISECONDS);
                    break;
            }
        }
    }
};

struct boss_hunter_toc5AI : public boss_argent_championAI
{
    boss_hunter_toc5AI(Creature* creature) : boss_argent_championAI(creature) {}

    void EnterCombat(Unit* /*who*/)
    {
        events.ScheduleEvent(EVENT_DISENGAGE, 10*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_MULTISHOT, 1*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_LIGHTNING_ARROWS, 7*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_CHECK_LOS, 2*IN_MILLISECONDS);
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
                case EVENT_DISENGAGE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_MINDISTANCE))
                    {
                        if (target->IsWithinMeleeRange(me))
                            DoCast(target, SPELL_DISENGAGE, true);
                        else
                            events.RescheduleEvent(EVENT_DISENGAGE, 5*IN_MILLISECONDS);
                    }
                    events.RescheduleEvent(EVENT_DISENGAGE, 20*IN_MILLISECONDS);
                    break;
                case EVENT_MULTISHOT:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_MULTISHOT, true);
                    events.RescheduleEvent(EVENT_MULTISHOT, 12*IN_MILLISECONDS);
                    break;
                case EVENT_LIGHTNING_ARROWS:
                    if (Unit* victim = me->GetVictim())
                    {
                        if (victim->GetDistance2d(me->GetPositionX(), me->GetPositionY()) <= 30.0f)
                        {
                            DoCastSelf(SPELL_LIGHTNING_ARROWS);
                            events.RescheduleEvent(EVENT_LIGHTNING_ARROWS, 25*IN_MILLISECONDS);
                        }
                    }
                    else
                        events.RescheduleEvent(EVENT_LIGHTNING_ARROWS, 2*IN_MILLISECONDS);
                    break;
                case EVENT_CHECK_LOS:
                    if (Unit* victim = me->GetVictim())
                    {
                        if (!me->IsWithinLOSInMap(victim))
                            me->NearTeleportTo(victim->GetPositionX(), victim->GetPositionY(), victim->GetPositionZ(), me->GetOrientation());
                    }
                    events.RescheduleEvent(EVENT_CHECK_LOS, 2*IN_MILLISECONDS);
                    break;
            }
        }
        DoSpellAttackIfReady(SPELL_SHOOT);
    }
};

struct boss_rouge_toc5AI : public boss_argent_championAI
{
    boss_rouge_toc5AI(Creature* creature) : boss_argent_championAI(creature) {}

    void EnterCombat(Unit* /*who*/)
    {
        events.ScheduleEvent(EVENT_EVISCERATE, 8*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_FAN_KIVES, 14*IN_MILLISECONDS);
        events.ScheduleEvent(EVENT_POISON_BOTTLE, 19*IN_MILLISECONDS);
        DoCastSelf(SPELL_DEADLY_POISON);
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
                case EVENT_EVISCERATE:
                    DoCastVictim(SPELL_EVISCERATE);
                    events.RescheduleEvent(EVENT_EVISCERATE, 8*IN_MILLISECONDS);
                    break;
                case EVENT_FAN_KIVES:
                    DoCastAOE(SPELL_FAN_OF_KNIVES);
                    events.RescheduleEvent(EVENT_FAN_KIVES, 14*IN_MILLISECONDS);
                    break;
                case EVENT_POISON_BOTTLE:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                        DoCast(target, SPELL_POISON_BOTTLE);
                    events.RescheduleEvent(EVENT_POISON_BOTTLE, 19*IN_MILLISECONDS);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }
};

class normal_champion_toc5 : public CreatureScript
{
public:
    normal_champion_toc5() : CreatureScript("normal_champion_toc5") { }

    struct normal_champion_toc5AI : public generic_champion_toc5AI
    {
        normal_champion_toc5AI(Creature* creature) : generic_champion_toc5AI(creature) {}

        void JustDied(Unit* /*attacker*/)
        {
            if (Creature* base = ObjectAccessor::GetCreature(*me, vehicleGUID))
                base->DespawnOrUnsummon();

            if (instance)
                instance->SetData(DATA_LESSER_CHAMPIONS_DEFEATED, 0);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new normal_champion_toc5AI(creature);
    }
};

class grand_champion_toc5 : public CreatureScript
{
public:
    grand_champion_toc5() : CreatureScript("grand_champion_toc5") { }

    struct grand_champion_toc5AI : public generic_champion_toc5AI
    {
        grand_champion_toc5AI(Creature* creature) : generic_champion_toc5AI(creature) {}

        void Reset()
        {
            generic_champion_toc5AI::Reset();
            defeated = false;
            finished = false;

            if (InstanceScript* instance = me->GetInstanceScript())
                SetEquipmentSlots(false, instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE ? ITEM_HORDE_LANCE : ITEM_ALLIANCE_LANCE, EQUIP_UNEQUIP, EQUIP_UNEQUIP);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage)
        {
            if (damage < me->GetHealth())
                return;

            damage = me->GetHealth() - 1;

            if (defeated)
                return;

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetImmuneToAll(true);
            me->SetReactState(REACT_PASSIVE);

            if (Creature* base = ObjectAccessor::GetCreature(*me, vehicleGUID))
            {
                vehicleEntry = base->GetEntry();
                base->DespawnOrUnsummon();
            }

            me->AttackStop();
            me->StopMoving();

            Talk(SAY_TRAMPLED, me);
            DoCastSelf(SPELL_FEIGN_DEATH, true);
            me->SetStandState(UNIT_STAND_STATE_DEAD);
            me->RemoveAura(SPELL_SHIELD);
            defeated = true;

            checkTimer = 5000;
            phaseCheck = 1*IN_MILLISECONDS;
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == MOVEMENT_POINT_WALL)
            {
                if (Creature* vehicle = me->SummonCreature(vehicleEntry, *me, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    me->EnterVehicle(vehicle, 0);
                    me->AI()->SetGuidData(vehicle->GetGUID());

                    defeated = false;

                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetImmuneToAll(false);
                    me->SetReactState(REACT_AGGRESSIVE);
                    me->SetFullHealth();

                    for (Player& player : me->GetMap()->GetPlayers())
                    {
                        Unit* base = &player;
                        if (player.HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
                            base = player.GetVehicleBase();

                        AddThreat(base, 0.0f);
                        me->SetInCombatWith(base);
                        player.SetInCombatWith(me);
                    }
                }
            }
            else if (id == MOVEMENT_POINT_GATE)
            {
                if (instance)
                    instance->SetData(DATA_PHASE_TWO_MOVEMENT, 0);
                me->SetFacingTo(ToCCommonPos[4].GetOrientation());
            }
            else
                generic_champion_toc5AI::MovementInform(type, id);
        }

        void UpdateAI(uint32 diff)
        {
            if (finished)
                return;

            if (!defeated)
            {
                generic_champion_toc5AI::UpdateAI(diff);
                return;
            }

            if (checkTimer <= diff)
            {
                Player* player = me->FindNearestPlayer(100.0f);
                if (player && me->IsWithinMeleeRange(player))
                {
                    if (!me->HasAura(SPELL_FEIGN_DEATH))
                    {
                        me->GetMotionMaster()->Clear();
                        DoCastSelf(SPELL_FEIGN_DEATH, true);
                        me->SetStandState(UNIT_STAND_STATE_DEAD);
                        checkTimer = 5000;
                    }
                }
                else if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != POINT_MOTION_TYPE)
                {
                    me->RemoveAurasDueToSpell(SPELL_FEIGN_DEATH);
                    me->SetStandState(UNIT_STAND_STATE_STAND);

                    float x = me->GetPositionX() - Middle.GetPositionX();
                    float y = me->GetPositionY() - Middle.GetPositionY();
                    float length = sqrt(x * x + y * y);
                    x *= ARENA_RADIUS / length;
                    y *= ARENA_RADIUS / length;
                    x += Middle.GetPositionX();
                    y += Middle.GetPositionY();

                    me->GetMotionMaster()->MovePoint(MOVEMENT_POINT_WALL, x, y, me->GetPositionZ());

                    me->SetSpeedRate(MOVE_RUN, 0.4f);

                    checkTimer = 250;
                }
                else
                    checkTimer = 250;
            }
            else
                checkTimer -= diff;

            if (phaseCheck <= diff)
            {
                if (GrandChampionsOutVehicle(me))
                {
                    std::list<Creature*> vehicleList;
                    me->GetCreatureListWithEntryInGrid(vehicleList, VEHICLE_ARGENT_WARHORSE, 1000.0f);
                    me->GetCreatureListWithEntryInGrid(vehicleList, VEHICLE_ARGENT_BATTLEWORG, 1000.0f);

                    for (std::list<Creature*>::const_iterator itr = vehicleList.begin(); itr != vehicleList.end(); ++itr)
                        (*itr)->DisappearAndDie();

                    //! restore defaults
                    SetEquipmentSlots(true);

                    if (instance)
                    {
                        me->RemoveAurasDueToSpell(SPELL_FEIGN_DEATH);
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DEFENSE_PLAYER);
                        for (uint8 i=0; i<3; i++)
                        {
                            if (me->GetGUID() == ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_1 + i)))
                            {
                                me->GetMotionMaster()->MovePoint(MOVEMENT_POINT_GATE, ToCCommonPos[3+i]);
                                me->SetSpeedRate(MOVE_RUN, 1.428f);
                                finished = true;
                                break;
                            }
                        }
                    }
                }
            }
            else
                phaseCheck -= diff;
        }

        bool GrandChampionsOutVehicle(Creature* me)
        {
            Creature* GrandChampion1 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_1)));
            Creature* GrandChampion2 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_2)));
            Creature* GrandChampion3 = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_GRAND_CHAMPION_3)));

            if (GrandChampion1 && GrandChampion2 && GrandChampion3)
            {
                if (!GrandChampion1->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) &&
                    !GrandChampion2->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT) &&
                    !GrandChampion3->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
                    return true;
            }
            return false;
        }

    private:
        uint32 checkTimer;
        uint32 phaseCheck;
        bool defeated;
        uint32 vehicleEntry;
        bool finished;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        if (InstanceScript* instance = creature->GetInstanceScript())
            if (instance->GetData(DATA_EVENT_STAGE) == EVENT_STAGE_CHAMPIONS_WAVE_BOSS_P2)
            {
                switch (creature->GetEntry())
                {
                case NPC_MOKRA:
                case NPC_JACOB:
                    return new boss_warrior_toc5AI(creature);
                case NPC_ERESSEA:
                case NPC_AMBROSE:
                    return new boss_mage_toc5AI(creature);
                case NPC_RUNOK:
                case NPC_COLOSOS:
                    return new boss_shaman_toc5AI(creature);
                case NPC_ZULTORE:
                case NPC_JAELYNE:
                    return new boss_hunter_toc5AI(creature);
                case NPC_VISCERI:
                case NPC_LANA:
                    return new boss_rouge_toc5AI(creature);
                default:
                    break;
                }
            }

        return new grand_champion_toc5AI(creature);
    }
};

class spell_toc5_hunter_disengage : public SpellScriptLoader
{
    public:
        spell_toc5_hunter_disengage() : SpellScriptLoader("spell_toc5_hunter_disengage") { }

        class spell_toc5_hunter_disengage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_toc5_hunter_disengage_SpellScript);

            void HandleDummyLaunch(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    caster->JumpTo(20.0f, 10.0f, false);
            }

            void Register()
            {
                OnEffectLaunch += SpellEffectFn(spell_toc5_hunter_disengage_SpellScript::HandleDummyLaunch, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_toc5_hunter_disengage_SpellScript();
        }
};

class spell_toc5_hunter_multishot : public SpellScriptLoader
{
    public:
        spell_toc5_hunter_multishot() : SpellScriptLoader("spell_toc5_hunter_multishot") { }

        class spell_toc5_hunter_multishot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_toc5_hunter_multishot_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_UNIT;
            }

            void CalculateDamage(SpellEffIndex /*effIndex*/)
            {
                int32 damage = 0;
                if (Creature* creature = GetCaster()->ToCreature())
                {
                    damage = irand(creature->GetCreatureTemplate()->minrangedmg, creature->GetCreatureTemplate()->maxrangedmg);
                    damage *= creature->GetCreatureTemplate()->dmg_multiplier;
                }
                SetHitDamage(damage);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_toc5_hunter_multishot_SpellScript::CalculateDamage, EFFECT_0, SPELL_EFFECT_WEAPON_PERCENT_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_toc5_hunter_multishot_SpellScript();
        }
};

void AddSC_boss_grand_champions()
{
    new vehicle_champion_toc5();
    new normal_champion_toc5();
    new grand_champion_toc5();

    new spell_toc5_hunter_disengage();
    new spell_toc5_hunter_multishot();
}
