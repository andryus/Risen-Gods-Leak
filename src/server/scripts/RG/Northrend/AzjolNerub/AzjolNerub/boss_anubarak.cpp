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
#include "azjol_nerub.h"

enum Spells
{
    SPELL_CARRION_BEETLES                         = 53520,
    SPELL_SUMMON_CARRION_BEETLES                  = 53521,
    SPELL_LEECHING_SWARM                          = 53467,
    SPELL_POUND                                   = 53472,
    SPELL_POUND_H                                 = 59433,
    SPELL_SUBMERGE                                = 53421,
    SPELL_IMPALE_DMG                              = 53454,
    SPELL_IMPALE_DMG_H                            = 59446,
    SPELL_IMPALE_SHAKEGROUND                      = 53455,
    SPELL_IMPALE_SPIKE                            = 53539,   //this is not the correct visual effect
    //SPELL_IMPALE_TARGET                           = 53458,

    //Creature Spells
    SPELL_STRIKE                                  = 52532,
    SPELL_SUNDER_ARMOR                            = 53618,
    SPELL_SUNDER_ARMOR_H                          = 59350,
    SPELL_PISON_BOLT                              = 53617,
    SPELL_PISON_BOLT_H                            = 59359,
    SPELL_PISON_BOLT_VOLLEY                       = 53616,
    SPELL_PISON_BOLT_VOLLEY_H                     = 59360,
    SPELL_BACKSTAB                                = 52540,
};

enum Creatures
{
    BOSS_ANUB                                     = 29120,
    CREATURE_GUARDIAN                             = 29216,
    CREATURE_VENOMANCER                           = 29217,
    CREATURE_DATTER                               = 29213,
    CREATURE_ASSASIN                              = 29214,
    CREATURE_IMPALE_TARGET                        = 29184,
    DISPLAY_INVISIBLE                             = 11686
};

// not in db
enum Yells
{
    SAY_AGGRO                                     = 0,
    SAY_SLAY                                      = 1,
    SAY_DEATH                                     = 2,
    SAY_LOCUST                                    = 3,
    SAY_SUBMERGE                                  = 4,
    SAY_INTRO                                     = 5
};

enum Achiev
{
    ACHIEV_TIMED_START_EVENT                      = 20381,
};

const Position TrashSpawnPoint[2] =
{
    {563.260f, 254.433f, 223.441f, 0.0f},
    {538.641f, 263.223f, 223.441f, 0.0f},
};

const Position SpawnPoint[2] =
{
    { 550.348633f, 316.006805f, 234.2947f, 0.0f },
    { 550.188660f, 324.264557f, 237.7412f, 0.0f },
};

enum Events
{
    EVENT_UNDERGROUND_END = 1,
    EVENT_DELAY_EVENT_START,

    // phase melee
    EVENT_POUND,
    EVENT_CAN_TURN,
    EVENT_LEECHING_SWARM,
    EVENT_CARRION_BEETLES,

    // phase underground
    EVENT_SUMMON_GUARDIAN,
    EVENT_SUMMON_VENOMANCER,
    EVENT_SUMMON_DATTER,

    // Impale
    EVENT_IMPALE,
    EVENT_IMPALE_PHASE_TARGET,
    EVENT_IMPALE_PHASE_ATTACK,
    EVENT_IMPALE_PHASE_DMG,

};

enum Phases
{
    PHASE_MELEE                 = 1,
    PHASE_UNDERGROUND           = 2
};

enum Misc
{
    FACTION_IMPALE              = 7
};

class boss_anub_arak : public CreatureScript
{
public:
    boss_anub_arak() : CreatureScript("boss_anub_arak") { }

    struct boss_anub_arakAI : public BossAI
    {
        boss_anub_arakAI(Creature* creature) : BossAI(creature, BOSS_ANUBARAK) { }

        void Reset()
        {
            BossAI::Reset();

            events.Reset();
            events.SetPhase(PHASE_MELEE);
            UndergroundPhase = 0;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);

            if (instance)
                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);
        }

        Creature* DoSummonImpaleTarget(Unit* target, uint32 duration)
        {
            Position targetPos;
            target->GetPosition(&targetPos);

            if (Creature* impaleTarget = me->SummonCreature(CREATURE_IMPALE_TARGET, targetPos, TEMPSUMMON_TIMED_DESPAWN, duration*IN_MILLISECONDS))
            {
                ImpaleTarget = impaleTarget->GetGUID();
                impaleTarget->setFaction(FACTION_IMPALE);
                impaleTarget->SetReactState(REACT_PASSIVE);
                impaleTarget->SetDisplayId(DISPLAY_INVISIBLE);
                impaleTarget->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL|UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                impaleTarget->SetSpeedRate(MOVE_RUN, 0.0f);
                impaleTarget->SetSpeedRate(MOVE_WALK, 0.0f);
                return impaleTarget;
            }
            return NULL;
        }

        void EnterCombat(Unit* who)
        {
            Talk(SAY_AGGRO);
            BossAI::EnterCombat(who);
            if (instance)
                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_TIMED_START_EVENT);

            events.RescheduleEvent(EVENT_DELAY_EVENT_START, 5*IN_MILLISECONDS);
        }

        void JustDied(Unit* killer)
        {
            BossAI::JustDied(killer);

            Talk(SAY_DEATH);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(SAY_SLAY);
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
                    case EVENT_DELAY_EVENT_START:
                        BossAI::EnterCombat(nullptr);
                        events.RescheduleEvent(EVENT_CARRION_BEETLES, 15*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        events.RescheduleEvent(EVENT_LEECHING_SWARM, 20*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        events.RescheduleEvent(EVENT_POUND, 15*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        break;
                    case EVENT_UNDERGROUND_END:
                        me->RemoveAura(SPELL_SUBMERGE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);
                        events.RescheduleEvent(EVENT_CARRION_BEETLES, 15*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        events.RescheduleEvent(EVENT_LEECHING_SWARM, 20*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        events.RescheduleEvent(EVENT_POUND, 15*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        events.SetPhase(PHASE_MELEE);
                        break;
                    case EVENT_POUND:
                        if (Creature* impaleTarget = DoSummonImpaleTarget( me->GetVictim(), 10))
                            me->CastSpell(impaleTarget, DUNGEON_MODE(SPELL_POUND, SPELL_POUND_H), false);
                        me->AddUnitState(UNIT_STATE_CANNOT_TURN);
                        events.ScheduleEvent(EVENT_CAN_TURN, 3500);
                        events.RescheduleEvent(EVENT_POUND, 16*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        break;
                    case EVENT_CAN_TURN:
                        me->ClearUnitState(UNIT_STATE_CANNOT_TURN);
                        break;
                    case EVENT_LEECHING_SWARM:
                        DoCastSelf(SPELL_LEECHING_SWARM, true);
                        events.RescheduleEvent(EVENT_LEECHING_SWARM, 23*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        break;
                    case EVENT_CARRION_BEETLES:
                        //DoCastVictim(SPELL_CARRION_BEETLES);
                        for (uint8 i = 0; i < 8; ++i)
                            DoCastVictim(SPELL_SUMMON_CARRION_BEETLES);
                        events.RescheduleEvent(EVENT_CARRION_BEETLES, 20*IN_MILLISECONDS, 0U, PHASE_MELEE);
                        break;
                    case EVENT_IMPALE: // impale casted in both phases
                        events.RescheduleEvent(EVENT_IMPALE, 15*IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_IMPALE_PHASE_TARGET, 0);
                        events.RescheduleEvent(EVENT_IMPALE_PHASE_ATTACK, 3*IN_MILLISECONDS);
                        events.RescheduleEvent(EVENT_IMPALE_PHASE_DMG, 4*IN_MILLISECONDS);
                        break;
                    case EVENT_IMPALE_PHASE_TARGET:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0U, 100.0f, true))
                            if (Creature* impaleTarget = DoSummonImpaleTarget(target, 6))
                                impaleTarget->CastSpell(impaleTarget, SPELL_IMPALE_SHAKEGROUND, true);
                        break;
                    case EVENT_IMPALE_PHASE_ATTACK:
                        if (Creature* impaleTarget = ObjectAccessor::GetCreature(*me, ImpaleTarget))
                        {
                            impaleTarget->CastSpell(impaleTarget, SPELL_IMPALE_SPIKE, false);
                            impaleTarget->RemoveAurasDueToSpell(SPELL_IMPALE_SHAKEGROUND);
                        }
                        break;
                    case EVENT_IMPALE_PHASE_DMG:
                        if (Creature* impaleTarget = ObjectAccessor::GetCreature(*me, ImpaleTarget))
                            me->CastSpell(impaleTarget, DUNGEON_MODE(SPELL_IMPALE_DMG, SPELL_IMPALE_DMG_H), true);
                        break;
                    case EVENT_SUMMON_GUARDIAN:
                        for (uint8 i = 0; i < 2; ++i)
                            if (Creature* Guard = me->SummonCreature(CREATURE_GUARDIAN, SpawnPoint[i], TEMPSUMMON_CORPSE_DESPAWN, 0))
                            {
                                AddThreat(me->GetVictim(), 0.0f, Guard);
                                DoZoneInCombat(Guard);
                            }

                        for (uint8 i = 0; i < 2; ++i)
                            if (Creature* Assasin = me->SummonCreature(CREATURE_ASSASIN, TrashSpawnPoint[i], TEMPSUMMON_CORPSE_DESPAWN, 0))
                                Assasin->AI()->AttackStart(SelectTarget(SELECT_TARGET_MINDISTANCE));
                        break;
                    case EVENT_SUMMON_VENOMANCER:
                        for (uint8 i = 0; i < 2; ++i)
                            if (Creature* Venomancer = me->SummonCreature(CREATURE_VENOMANCER, SpawnPoint[i], TEMPSUMMON_CORPSE_DESPAWN, 0))
                            {
                                AddThreat(me->GetVictim(), 0.0f, Venomancer);
                                DoZoneInCombat(Venomancer);
                            }
                        break;
                    case EVENT_SUMMON_DATTER:
                        for (uint8 i = 0; i < 2; ++i)
                            if (Creature* Datter = me->SummonCreature(CREATURE_DATTER, TrashSpawnPoint[i], TEMPSUMMON_CORPSE_DESPAWN, 0))
                                Datter->AI()->AttackStart(SelectTarget(SELECT_TARGET_MINDISTANCE));
                        break;
                }
            }
            if (events.IsInPhase(PHASE_MELEE))
                DoMeleeAttackIfReady();

            if (((UndergroundPhase == 0 && HealthBelowPct(66)) ||
                (UndergroundPhase == 1 && HealthBelowPct(33))  ||
                (UndergroundPhase == 2 && HealthBelowPct(15))) &&
                !me->HasUnitState(UNIT_STATE_CASTING))
                {
                    events.CancelEvent(EVENT_CARRION_BEETLES);
                    events.CancelEvent(EVENT_LEECHING_SWARM);
                    events.CancelEvent(EVENT_POUND);
                    events.RescheduleEvent(EVENT_IMPALE, 9*IN_MILLISECONDS);
                    events.RescheduleEvent(EVENT_UNDERGROUND_END, 40*IN_MILLISECONDS);
                    events.SetPhase(PHASE_UNDERGROUND);

                    switch (UndergroundPhase)
                    {
                        case 2:
                            events.RescheduleEvent(EVENT_SUMMON_DATTER, 32*IN_MILLISECONDS, 0U, PHASE_UNDERGROUND);
                            // Fallthrough
                        case 1:
                            events.RescheduleEvent(EVENT_SUMMON_VENOMANCER, 15*IN_MILLISECONDS, 0U, PHASE_UNDERGROUND);
                            // Fallthrough
                        case 0:
                            events.RescheduleEvent(EVENT_SUMMON_GUARDIAN, 0, 0U, PHASE_UNDERGROUND);
                            break;
                    }

                    me->RemoveAllAuras();
                    DoCastSelf(SPELL_SUBMERGE, false);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE);

                    ++UndergroundPhase;
                }
        }

        private:
            uint8 UndergroundPhase;
            ObjectGuid ImpaleTarget;
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_anub_arakAI(creature);
    }
};

class npc_anub_assasin : public CreatureScript
{
public:
    npc_anub_assasin() : CreatureScript("npc_anub_assasin") {}

    struct npc_anub_assasinAI : public ScriptedAI
    {
        npc_anub_assasinAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset()
        {
            BackstabTimer = 5000;
        };

        void UpdateAI(uint32 diff)
        {
            if (!UpdateVictim())
                return;

            if (BackstabTimer <= diff)
            {
                DoCastVictim(SPELL_BACKSTAB);
                BackstabTimer = 5000;
            }
            else
                BackstabTimer -= diff;

            DoMeleeAttackIfReady();
        }

    private:
        uint32 BackstabTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_anub_assasinAI(creature);
    }
};

void AddSC_boss_anub_arak()
{
    new boss_anub_arak();
    new npc_anub_assasin();
}
