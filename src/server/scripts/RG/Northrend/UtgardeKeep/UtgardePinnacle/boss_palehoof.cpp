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

/* Script Data Start
SDName: Boss palehoof
SDAuthor: LordVanMartin
SD%Complete:
SDComment:
SDCategory:
Script Data End */

#include <algorithm>
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "utgarde_pinnacle.h"
#include "SpellScript.h"

enum Spells
{
    SPELL_ARCING_SMASH                          = 48260,
    SPELL_IMPALE                                = 48261,
    H_SPELL_IMPALE                              = 59268,
    SPELL_WITHERING_ROAR                        = 48256,
    H_SPELL_WITHERING_ROAR                      = 59267,
    SPELL_FREEZE                                = 16245,
    SPELL_BEASTS_MARK_TRIGGER                   = 48877
};

enum PalehoofEvents
{
    EVENT_ARCING_SMASH                          = 1,
    EVENT_IMPALE                                = 2,
    EVENT_WITHERING_ROAR                        = 3
};

//Orb spells
enum OrbSpells
{
    SPELL_ORB_VISUAL                            = 48044,
    SPELL_ORB_CHANNEL                           = 47669
};

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SLAY                                    = 1
};

enum Creatures
{
    MOB_STASIS_CONTROLLER                       = 26688,
    NPC_TUNDRA_WOLF                             = 26672
};

struct Locations
{
    float x, y, z;
};

struct Locations moveLocs[]=
{
    { 272.75f, -451.59f, 109.5f },
    { 272.75f, -451.59f, 109.5f },
    { 272.75f, -451.59f, 109.5f },
    { 272.75f, -451.59f, 109.5f },
    { 272.75f, -451.59f, 109.5f },
    { 238.6f,  -460.7f,  109.5 }
}; 

enum Phase
{
    PHASE_FRENZIED_WORGEN,
    PHASE_RAVENOUS_FURLBORG,
    PHASE_MASSIVE_JORMUNGAR,
    PHASE_FEROCIOUS_RHINO,
    PHASE_GORTOK_PALEHOOF,
    PHASE_NONE
};

class boss_palehoof : public CreatureScript
{
public:
    boss_palehoof() : CreatureScript("boss_palehoof") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_palehoofAI (creature);
    }

    struct boss_palehoofAI : public ScriptedAI
    {
        boss_palehoofAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
      
        void Reset()
        {
            /// There is a good reason to store them like this, we are going to shuffle the order.
            for (uint32 i = PHASE_FRENZIED_WORGEN; i < PHASE_GORTOK_PALEHOOF; ++i)
                Sequence[i] = Phase(i);

            /// This ensures a random order and only executes each phase once.
            std::random_shuffle(Sequence, Sequence + PHASE_GORTOK_PALEHOOF);

            _events.Reset();
            _events.ScheduleEvent(EVENT_ARCING_SMASH,   15 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_IMPALE,         12 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_WITHERING_ROAR, 10 * IN_MILLISECONDS);

            me->GetMotionMaster()->MoveTargetedHome();

            AddCount = 0;

            currentPhase = PHASE_NONE;

            if (instance)
            {
                instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, NOT_STARTED);

                Creature* temp = ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(DATA_MOB_FRENZIED_WORGEN)));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                temp = ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(DATA_MOB_FEROCIOUS_RHINO)));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                temp = ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(DATA_MOB_MASSIVE_JORMUNGAR)));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                temp = ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(DATA_MOB_RAVENOUS_FURBOLG)));
                if (temp && !temp->IsAlive())
                    temp->Respawn();

                temp = ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(DATA_MOB_ORB)));
                if (temp && temp->IsAlive())
                    temp->DespawnOrUnsummon();

                if (GameObject* go = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF_SPHERE))))
                {
                    go->SetGoState(GO_STATE_READY);
                    go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                }
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
        }

        void AttackStart(Unit* who)
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (currentPhase != PHASE_GORTOK_PALEHOOF)
                return;

            //Return since we have no target
            if (!UpdateVictim())
                return;

            Creature* temp = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_ORB)) : ObjectGuid::Empty);
            if (temp && temp->IsAlive())
                temp->DisappearAndDie();

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ARCING_SMASH:
                        DoCastSelf(SPELL_ARCING_SMASH);
                        _events.ScheduleEvent(EVENT_ARCING_SMASH, urand(13, 17)*IN_MILLISECONDS);
                        break;
                    case EVENT_IMPALE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_IMPALE);
                        _events.ScheduleEvent(EVENT_IMPALE, urand(8, 12)*IN_MILLISECONDS);
                        break;
                    case EVENT_WITHERING_ROAR:
                        DoCastSelf(SPELL_WITHERING_ROAR);
                        _events.ScheduleEvent(EVENT_WITHERING_ROAR, urand(7, 9)*IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }

            }

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
                instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, DONE);
            Creature* temp = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_ORB)) : ObjectGuid::Empty);
            if (temp && temp->IsAlive())
                temp->DisappearAndDie();
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_SLAY);
        }

        void NextPhase()
        {
            if (currentPhase == PHASE_NONE)
            {
                instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, IN_PROGRESS);
                me->SummonCreature(MOB_STASIS_CONTROLLER, moveLocs[5].x, moveLocs[5].y, moveLocs[5].z, 0, TEMPSUMMON_CORPSE_DESPAWN);
            }
            Phase move = PHASE_NONE;
            if (AddCount >= DUNGEON_MODE(2, 4))
                move = PHASE_GORTOK_PALEHOOF;
            else
                move = Sequence[AddCount++];
            //send orb to summon spot
            Creature* pOrb = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_ORB)) : ObjectGuid::Empty);
            if (pOrb && pOrb->IsAlive())
            {
                if (currentPhase == PHASE_NONE)
                    pOrb->CastSpell(me, SPELL_ORB_VISUAL, true);
                pOrb->GetMotionMaster()->MovePoint(move, moveLocs[move].x, moveLocs[move].y, moveLocs[move].z);
            }
            currentPhase = move;
        }

        void JustReachedHome()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetImmuneToPC(true);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCastSelf(SPELL_FREEZE);
        }
        private:
            Phase currentPhase;
            uint8 AddCount;
            Phase Sequence[4];
            InstanceScript* instance;
            EventMap _events;
    };

};

// Ravenous Furbolg
enum RavenousSpells
{
    SPELL_CHAIN_LIGHTING                        = 48140,
    H_SPELL_CHAIN_LIGHTING                      = 59273,
    SPELL_CRAZED                                = 48139,
    SPELL_TERRIFYING_ROAR                       = 48144
};

enum RavenousEvents
{
    EVENT_CHAIN_LIGHTING                        = 1,
    EVENT_CRAZED                                = 2,
    EVENT_TERRIFYING_ROAR                       = 3
};

class npc_ravenous_furbolg : public CreatureScript
{
public:
    npc_ravenous_furbolg() : CreatureScript("npc_ravenous_furbolg") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ravenous_furbolgAI (creature);
    }

    struct npc_ravenous_furbolgAI : public ScriptedAI
    {
        npc_ravenous_furbolgAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_CHAIN_LIGHTING,   5 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_CRAZED,          10 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_TERRIFYING_ROAR, 15 * IN_MILLISECONDS);

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* palehoof = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty);
                    if (palehoof && palehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, palehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHAIN_LIGHTING:
                        DoCastVictim(SPELL_CHAIN_LIGHTING);
                        _events.ScheduleEvent(EVENT_CHAIN_LIGHTING, urand(5, 10)*IN_MILLISECONDS);
                        break;
                    case EVENT_CRAZED:
                        DoCastSelf(SPELL_CRAZED);
                        me->GetThreatManager().resetAllAggro();
                        if (Unit* target = SelectTarget(SELECT_TARGET_MAXDISTANCE, 1))
                            AttackStart(target);
                        _events.ScheduleEvent(EVENT_CRAZED, urand(8, 12)*IN_MILLISECONDS);
                        break;
                    case EVENT_TERRIFYING_ROAR:
                        DoCastSelf(SPELL_TERRIFYING_ROAR);
                        _events.ScheduleEvent(EVENT_CRAZED, urand(10, 20)*IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }

            }

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who)
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                if(Creature* palehoof = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty))
                if (palehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, palehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetImmuneToPC(true);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCastSelf(SPELL_FREEZE);
        }
    private:
        InstanceScript* instance;
        EventMap _events;
    };

};

// Frenzied Worgen
enum FrenziedSpells
{
    SPELL_MORTAL_WOUND                          = 48137,
    H_SPELL_MORTAL_WOUND                        = 59265,
    SPELL_ENRAGE_1                              = 48138,
    SPELL_ENRAGE_2                              = 48142
};

enum FrenziedEvents
{
    EVENT_MORTAL_WOUND                          = 1,
    EVENT_ENRAGE_1                              = 2,
    EVENT_ENRAGE_2                              = 3
};

class npc_frenzied_worgen : public CreatureScript
{
public:
    npc_frenzied_worgen() : CreatureScript("npc_frenzied_worgen") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_frenzied_worgenAI (creature);
    }

    struct npc_frenzied_worgenAI : public ScriptedAI
    {
        npc_frenzied_worgenAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_MORTAL_WOUND, 5 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_ENRAGE_1,    15 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_ENRAGE_2,    10 * IN_MILLISECONDS);

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* palehoof = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty);
                    if (palehoof && palehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, palehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_MORTAL_WOUND:
                        DoCastVictim(SPELL_MORTAL_WOUND);
                        _events.ScheduleEvent(EVENT_CHAIN_LIGHTING, urand(3, 7)*IN_MILLISECONDS);
                        break;
                    case EVENT_ENRAGE_1:
                        DoCastSelf(SPELL_ENRAGE_1);
                        _events.ScheduleEvent(EVENT_ENRAGE_1, 15 * IN_MILLISECONDS);
                        break;
                    case EVENT_ENRAGE_2:
                        DoCastSelf(SPELL_ENRAGE_2);
                        _events.ScheduleEvent(EVENT_ENRAGE_2, 10 * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }

            }

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who)
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
            if (instance)
                instance->SetData(DATA_GORTOK_PALEHOOF_EVENT, IN_PROGRESS);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                Creature* palehoof = ObjectAccessor::GetCreature((*me), ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)));
                if (palehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, palehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetImmuneToPC(true);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCastSelf(SPELL_FREEZE);
        }
    private:
        InstanceScript* instance;
        EventMap _events;
    };

};

//Ferocious Rhino
enum FerociousSpells
{
    SPELL_GORE                                  = 48130,
    H_SPELL_GORE                                = 59264,
    SPELL_GRIEVOUS_WOUND                        = 48105,
    H_SPELL_GRIEVOUS_WOUND                      = 59263,
    SPELL_STOMP                                 = 48131
};

enum FerociousEvents
{
    EVENT_GORE                                  = 1, // yes gore
    EVENT_GRIEVOUS_WOUND                        = 2,
    EVENT_STOMP                                 = 3
};

class npc_ferocious_rhino : public CreatureScript
{
public:
    npc_ferocious_rhino() : CreatureScript("npc_ferocious_rhino") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ferocious_rhinoAI (creature);
    }

    struct npc_ferocious_rhinoAI : public ScriptedAI
    {
        npc_ferocious_rhinoAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
        
        void Reset()
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_GORE,           15 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_GRIEVOUS_WOUND, 20 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_STOMP,          10 * IN_MILLISECONDS);

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* pPalehoof = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty);
                    if (pPalehoof && pPalehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_GORE:
                        DoCastVictim(SPELL_STOMP);
                        _events.ScheduleEvent(EVENT_GORE, urand(8, 12)*IN_MILLISECONDS);
                        break;
                    case EVENT_GRIEVOUS_WOUND:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_GRIEVOUS_WOUND);
                        _events.ScheduleEvent(EVENT_GRIEVOUS_WOUND, urand(18, 22)*IN_MILLISECONDS);
                        break;
                    case EVENT_STOMP:
                        DoCastVictim(SPELL_STOMP);
                        _events.ScheduleEvent(EVENT_STOMP, urand(8, 12)*IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }

            }

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who)
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                Creature* palehoof = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty);
                if (palehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, palehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetImmuneToPC(true);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCastSelf(SPELL_FREEZE);
        }
    private:
        InstanceScript* instance;
        EventMap _events;
    };

};

// Massive jormungar
enum MassiveSpells
{
    SPELL_ACID_SPIT                             = 48132,
    SPELL_ACID_SPLATTER                         = 48136,
    H_SPELL_ACID_SPLATTER                       = 59272,
    SPELL_POISON_BREATH                         = 48133,
    H_SPELL_POISON_BREATH                       = 59271
};

enum MassiveAdds
{
    CREATURE_JORMUNGAR_WORM                     = 27228
};

enum MassiveEvents
{
    EVENT_ACID_SPIT                             = 1,
    EVENT_ACID_SPLATTER                         = 2,
    EVENT_POISON_BREATH                         = 3
};

class npc_massive_jormungar : public CreatureScript
{
public:
    npc_massive_jormungar() : CreatureScript("npc_massive_jormungar") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_massive_jormungarAI (creature);
    }

    struct npc_massive_jormungarAI : public ScriptedAI
    {
        npc_massive_jormungarAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset()
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_ACID_SPIT,      3 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_ACID_SPLATTER, 12 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_POISON_BREATH, 10 * IN_MILLISECONDS);

            me->GetMotionMaster()->MoveTargetedHome();

            if (instance)
                if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) == IN_PROGRESS)
                {
                    Creature* pPalehoof = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty);
                    if (pPalehoof && pPalehoof->IsAlive())
                        CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->Reset();
                }
        }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_ACID_SPIT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_ACID_SPIT);
                        _events.ScheduleEvent(EVENT_ACID_SPIT, urand(5, 8)*IN_MILLISECONDS);
                        break;
                    case EVENT_ACID_SPLATTER:
                        DoCastSelf(SPELL_ACID_SPLATTER, true);
                        _events.ScheduleEvent(EVENT_ACID_SPLATTER, urand(8, 12)*IN_MILLISECONDS);
                        break;
                    case EVENT_POISON_BREATH:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                            DoCast(target, SPELL_POISON_BREATH);
                        _events.ScheduleEvent(EVENT_POISON_BREATH, urand(8, 12)*IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }

            }

            DoMeleeAttackIfReady();
        }

        void AttackStart(Unit* who)
        {
            if (!who)
                return;

            if (me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            if (instance)
            {
                Creature* palehoof = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty);
                if (palehoof)
                    CAST_AI(boss_palehoof::boss_palehoofAI, palehoof->AI())->NextPhase();
            }
        }

        void JustReachedHome()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
            me->SetImmuneToPC(true);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            DoCastSelf(SPELL_FREEZE);
        }
    private:
        InstanceScript* instance;
        EventMap _events;
    };

};

class npc_palehoof_orb : public CreatureScript
{
public:
    npc_palehoof_orb() : CreatureScript("npc_palehoof_orb") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_palehoof_orbAI (creature);
    }

    struct npc_palehoof_orbAI : public ScriptedAI
    {
        npc_palehoof_orbAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
        
        void Reset()
        {
            currentPhase = PHASE_NONE;
            SummonTimer = 5000;
            //! HACK: Creature's can't have MOVEMENTFLAG_FLYING
            me->AddUnitMovementFlag(MOVEMENTFLAG_FLYING);
            me->RemoveAurasDueToSpell(SPELL_ORB_VISUAL);
            me->SetSpeedRate(MOVE_FLIGHT, 0.5f);
        }

        void UpdateAI(uint32 diff)
        {
            if (currentPhase == PHASE_NONE)
                return;

            if (instance->GetData(DATA_GORTOK_PALEHOOF_EVENT) != IN_PROGRESS)
                me->DespawnOrUnsummon(5000);

            if (SummonTimer <= diff)
            {
                if (currentPhase < 5 && currentPhase >= 0)
                {
                   Creature* next = NULL;
                   switch (currentPhase)
                   {
                        case PHASE_FRENZIED_WORGEN: 
                            next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_FRENZIED_WORGEN)) : ObjectGuid::Empty); 
                            break;
                        case PHASE_RAVENOUS_FURLBORG: 
                            next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_RAVENOUS_FURBOLG)) : ObjectGuid::Empty); 
                            break;
                        case PHASE_MASSIVE_JORMUNGAR: 
                            next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_MASSIVE_JORMUNGAR)) : ObjectGuid::Empty); 
                            break;
                        case PHASE_FEROCIOUS_RHINO: 
                            next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_FEROCIOUS_RHINO)) : ObjectGuid::Empty); 
                            break;
                        case PHASE_GORTOK_PALEHOOF: 
                            next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty); 
                            break;
                        default: 
                            break;
                   }

                   if (next)
                   {
                        next->RemoveAurasDueToSpell(SPELL_FREEZE);
                        next->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1);
                        next->SetImmuneToPC(false);
                        next->SetStandState(UNIT_STAND_STATE_STAND);
                        next->SetInCombatWithZone();
                        next->AI()->AttackStart(next->FindNearestPlayer(100.0f));

                   }
                   currentPhase = PHASE_NONE;
                }
            } else SummonTimer -= diff;
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id > 4)
                return;

            Creature* next = NULL;

            switch (id)
            {
                case PHASE_FRENZIED_WORGEN: 
                    next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_FRENZIED_WORGEN)) : ObjectGuid::Empty); 
                    break;
                case PHASE_RAVENOUS_FURLBORG: 
                    next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_RAVENOUS_FURBOLG)) : ObjectGuid::Empty); 
                    break;
                case PHASE_MASSIVE_JORMUNGAR: 
                    next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_MASSIVE_JORMUNGAR)) : ObjectGuid::Empty); 
                    break;
                case PHASE_FEROCIOUS_RHINO: 
                    next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_MOB_FEROCIOUS_RHINO)) : ObjectGuid::Empty); 
                    break;
                case PHASE_GORTOK_PALEHOOF: next = ObjectAccessor::GetCreature((*me), instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty); 
                    break;
                default: break;
            }

            if (next)
            {
                DoCast(next, SPELL_ORB_CHANNEL, false);
                next->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            currentPhase = (Phase)id;
            SummonTimer = 8 * IN_MILLISECONDS;
        }
    private:
        InstanceScript* instance;
        uint32 SummonTimer;
        Phase currentPhase;
    };

};

class go_palehoof_sphere : public GameObjectScript
{
public:
    go_palehoof_sphere() : GameObjectScript("go_palehoof_sphere") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go)
    {
        InstanceScript* instance = go->GetInstanceScript();

        Creature* pPalehoof = ObjectAccessor::GetCreature(*go, instance ? ObjectGuid(instance->GetGuidData(DATA_GORTOK_PALEHOOF)) : ObjectGuid::Empty);
        if (pPalehoof && pPalehoof->IsAlive())
        {
            go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
            go->SetGoState(GO_STATE_ACTIVE);

            CAST_AI(boss_palehoof::boss_palehoofAI, pPalehoof->AI())->NextPhase();
        }
        return true;
    }

};

class spell_beasts_mark : public SpellScriptLoader
{
    public:
        spell_beasts_mark() : SpellScriptLoader("spell_beasts_mark") { }

        class spell_beasts_mark_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_beasts_mark_AuraScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_BEASTS_MARK_TRIGGER))
                    return false;
                return true;
            }

            void HandleEffectProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction(); // will prevent effect spamm
                Unit* target = GetTarget();
                target->CastSpell(target, SPELL_BEASTS_MARK_TRIGGER);
            }

            bool Check(ProcEventInfo& eventInfo) // np procc on own spell, else crash
            {
                if (SpellInfo const* spellInfo = eventInfo.GetSpellInfo())
                    if (spellInfo->Id == SPELL_BEASTS_MARK_TRIGGER) 
                        return false;  
                return eventInfo.GetProcTarget() && eventInfo.GetProcTarget()->GetEntry() == NPC_TUNDRA_WOLF;
            }

            void Register()
            {
                OnEffectProc += AuraEffectProcFn(spell_beasts_mark_AuraScript::HandleEffectProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
                DoCheckProc += AuraCheckProcFn(spell_beasts_mark_AuraScript::Check);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_beasts_mark_AuraScript();
        }
};

void AddSC_boss_palehoof()
{
    new boss_palehoof();
    new npc_ravenous_furbolg();
    new npc_frenzied_worgen();
    new npc_ferocious_rhino();
    new npc_massive_jormungar();
    new npc_palehoof_orb();
    new go_palehoof_sphere();
    new spell_beasts_mark();
}
