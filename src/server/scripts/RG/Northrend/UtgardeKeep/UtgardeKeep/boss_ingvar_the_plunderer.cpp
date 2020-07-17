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

/* ScriptData
SDName: Boss_Ingvar_The_Plunderer
SD%Complete: 95
SDComment: Some Problems with Annhylde Movement, Blizzlike Timers (just shadow axe summon needs a new timer)
SDCategory: Udgarde Keep
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "utgarde_keep.h"

enum Yells
{
    //Yells Ingvar
    YELL_AGGRO_1                                = 0,
    YELL_KILL_1                                 = 1,
    YELL_DEAD_1                                 = 2,

    YELL_AGGRO_2                                = 0,
    YELL_KILL_2                                 = 1,
    YELL_DEAD_2                                 = 2,

    YELL_RESSURECT                              = 0,

    YELL_INTRO_SPEECH                           = 6
};

enum Creatures
{
    NPC_INGVAR_HUMAN                            = 23954,
    NPC_ANNHYLDE_THE_CALLER                     = 24068,
    NPC_INGVAR_UNDEAD                           = 23980,
    NPC_PROTO_DRAKE                             = 24849
};

enum Gameobjects
{
    ENTRY_GIANT_PORTCULLIS_2                    = 186694
};

enum Events
{
    EVENT_CLEAVE = 1,
    EVENT_SMASH,
    EVENT_STAGGERING_ROAR,
    EVENT_ENRAGE,

    EVENT_DARK_SMASH,
    EVENT_DREADFUL_ROAR,
    EVENT_WOE_STRIKE,
    EVENT_SHADOW_AXE,
    EVENT_JUST_TRANSFORMED,
    EVENT_SUMMON_BANSHEE,
    EVENT_CAN_TURN
};

enum Phases
{
    PHASE_HUMAN = 1,
    PHASE_UNDEAD,
    PHASE_EVENT,
};

enum Spells
{
    //Ingvar Spells human form
    SPELL_CLEAVE                                = 42724,
    SPELL_SMASH                                 = 42669,
    SPELL_STAGGERING_ROAR                       = 42708,
    SPELL_ENRAGE                                = 42705,

    SPELL_INGVAR_FEIGN_DEATH                    = 42795,
    SPELL_SUMMON_BANSHEE                        = 42912,
    SPELL_SCOURG_RESURRECTION                   = 42863, // Spawn resurrect effect around Ingvar

    //Ingvar Spells undead form
    SPELL_DARK_SMASH                            = 42723,
    SPELL_DREADFUL_ROAR                         = 42729,
    SPELL_WOE_STRIKE                            = 42730,

    ENTRY_THROW_TARGET                          = 23996,
    SPELL_SHADOW_AXE_SUMMON                     = 42748
};

class boss_ingvar_the_plunderer : public CreatureScript
{
public:
    boss_ingvar_the_plunderer() : CreatureScript("boss_ingvar_the_plunderer") { }
    
    struct boss_ingvar_the_plundererAI : public ScriptedAI
    {
        boss_ingvar_the_plundererAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            bIsUndead = false;
        }
        
        void Reset() override
        {
            if (bIsUndead)
                me->UpdateEntry(NPC_INGVAR_HUMAN);

            bIsUndead = false;
            bAttack = true;
            YelledText = false;

            me->SetImmuneToPC(false);
            me->SetStandState(UNIT_STAND_STATE_STAND);

            events.Reset();
            events.SetPhase(PHASE_HUMAN);

            events.ScheduleEvent(EVENT_CLEAVE, urand(6,12)*IN_MILLISECONDS, 0, PHASE_HUMAN);
            events.ScheduleEvent(EVENT_ENRAGE, urand(7,14)*IN_MILLISECONDS, 0, PHASE_HUMAN);
            events.ScheduleEvent(EVENT_SMASH, urand(12,17)*IN_MILLISECONDS, 0, PHASE_HUMAN);
            IntroTimer = 6 * IN_MILLISECONDS;

            if (instance)
                instance->SetData(DATA_INGVAR_EVENT, NOT_STARTED);
        }

        void DamageTaken(Unit* /*done_by*/, uint32 &damage) override
        {
            if (damage >= me->GetHealth() && !bIsUndead)
            {
                //DoCastSelf(SPELL_INGVAR_FEIGN_DEATH, true);  // Dont work ???
                // visuel hack
                me->SetHealth(0);
                me->InterruptNonMeleeSpells(true);
                me->RemoveAllAuras();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                bAttack = false;
                me->GetMotionMaster()->MovementExpired(false);
                me->GetMotionMaster()->MoveIdle();
                me->SetStandState(UNIT_STAND_STATE_DEAD);
                // visuel hack end

                events.SetPhase(PHASE_EVENT);
                events.ScheduleEvent(EVENT_SUMMON_BANSHEE, 3 * IN_MILLISECONDS, 0, PHASE_EVENT);

                if (!YelledText)
                { 
                    Talk(YELL_DEAD_1);
                    YelledText = true;
                }
            }

            if (events.IsInPhase(PHASE_EVENT))
                damage = 0;
        }

        void StartZombiePhase()
        {
            bIsUndead = true;
            me->UpdateEntry(NPC_INGVAR_UNDEAD);
            events.ScheduleEvent(EVENT_JUST_TRANSFORMED, 2 * IN_MILLISECONDS, 0, PHASE_EVENT);

            Talk(YELL_AGGRO_2);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            if (!bIsUndead)
                Talk(YELL_AGGRO_1);

            if (instance)
                instance->SetData(DATA_INGVAR_EVENT, IN_PROGRESS);

            me->SetInCombatWithZone();
            me->ClearUnitState(UNIT_STATE_EVADE);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(YELL_DEAD_2);

            if (instance)
            {
                // Ingvar has NPC_INGVAR_UNDEAD id in this moment, so we have to update encounter state for his original id
                instance->UpdateEncounterState(ENCOUNTER_CREDIT_KILL_CREATURE, NPC_INGVAR_HUMAN, me);
                instance->SetData(DATA_INGVAR_EVENT, DONE);
            }

            if (GameObject* door = me->FindNearestGameObject(ENTRY_GIANT_PORTCULLIS_2, 1000.0f))
                door->UseDoorOrButton();

        }

        void ScheduleSecondPhase()
        {
            events.SetPhase(PHASE_UNDEAD);
            events.ScheduleEvent(EVENT_DARK_SMASH, urand(14,18)*IN_MILLISECONDS, 0, PHASE_UNDEAD);
            events.ScheduleEvent(EVENT_WOE_STRIKE, urand(10,14)*IN_MILLISECONDS, 0, PHASE_UNDEAD);
            events.ScheduleEvent(EVENT_SHADOW_AXE, urand(21,25)*IN_MILLISECONDS, 0, PHASE_UNDEAD);
        }

        void KilledUnit(Unit* /*victim*/) override
        {
            Talk(bIsUndead ? YELL_KILL_1 : YELL_KILL_2);
        }

        void UpdateAI(uint32 diff) override
        {
            if (IntroTimer <= diff)
            {
                if (Creature* protodrake = me->FindNearestCreature(NPC_PROTO_DRAKE, 25.0f))
                {
                    me->SetFacingToObject(protodrake);
                    Talk(YELL_INTRO_SPEECH);
                    protodrake->GetMotionMaster()->MovePath(protodrake->GetSpawnId() * 10, false);
                }
                IntroTimer = 6 * IN_MILLISECONDS;
            }
            else
                IntroTimer -= diff;

            if (!UpdateVictim() && !events.IsInPhase(PHASE_EVENT))
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    // ALL PHASES
                    case EVENT_CAN_TURN:
                        me->ClearUnitState(UNIT_STATE_CANNOT_TURN);
                        break;
                    // PHASE ONE
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, urand(6,12)*IN_MILLISECONDS, 0, PHASE_HUMAN);
                        break;
                    case EVENT_STAGGERING_ROAR:
                        DoCastSelf(SPELL_STAGGERING_ROAR);
                        break;
                    case EVENT_ENRAGE:
                        DoCastSelf(SPELL_ENRAGE);
                        events.ScheduleEvent(EVENT_ENRAGE, urand(7,14)*IN_MILLISECONDS, 0, PHASE_HUMAN);
                        break;
                    case EVENT_SMASH:
                        DoCastAOE(SPELL_SMASH);

                        me->AddUnitState(UNIT_STATE_CANNOT_TURN);
                        events.ScheduleEvent(EVENT_CAN_TURN, 3000);

                        events.ScheduleEvent(EVENT_SMASH, urand(14,18)*IN_MILLISECONDS, 0, PHASE_HUMAN);
                        events.ScheduleEvent(EVENT_STAGGERING_ROAR, urand(5,7)*IN_MILLISECONDS, 0, PHASE_HUMAN);
                        break;
                    case EVENT_JUST_TRANSFORMED:
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        bAttack = true;
                        me->SetInCombatWithZone();
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                        ScheduleSecondPhase();
                        return;
                    case EVENT_SUMMON_BANSHEE:
                        DoCastSelf(SPELL_SUMMON_BANSHEE);
                        return;
                    // PHASE TWO
                    case EVENT_DARK_SMASH:
                        DoCastVictim(SPELL_DARK_SMASH);

                        me->AddUnitState(UNIT_STATE_CANNOT_TURN);
                        events.ScheduleEvent(EVENT_CAN_TURN, 3000);

                        events.ScheduleEvent(EVENT_DARK_SMASH, urand(14,18)*IN_MILLISECONDS, 0, PHASE_UNDEAD);
                        events.ScheduleEvent(EVENT_DREADFUL_ROAR, urand(5,7)*IN_MILLISECONDS, 0, PHASE_UNDEAD);
                        break;
                    case EVENT_DREADFUL_ROAR:
                        DoCastSelf(SPELL_DREADFUL_ROAR);
                        break;
                    case EVENT_WOE_STRIKE:
                        DoCastVictim(SPELL_WOE_STRIKE);
                        events.ScheduleEvent(EVENT_WOE_STRIKE, urand(10,14)*IN_MILLISECONDS, 0, PHASE_UNDEAD);
                        break;
                    case EVENT_SHADOW_AXE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_MAXTHREAT, 1))
                            DoCast(target, SPELL_SHADOW_AXE_SUMMON);
                        events.ScheduleEvent(EVENT_SHADOW_AXE, urand(21,25)*IN_MILLISECONDS, 0, PHASE_UNDEAD);
                        break;
                }
            }

            if (bAttack)
                DoMeleeAttackIfReady();
        }
    private:
        InstanceScript* instance;
        EventMap events;        
        bool bIsUndead;
        bool bAttack;
        uint32 IntroTimer;
        bool YelledText;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_ingvar_the_plundererAI(creature);
    }

};

enum eSpells
{
//we don't have that text in db so comment it until we get this text
//    YELL_RESSURECT                      = -1574025,

//Spells for Annhylde
    SPELL_SCOURG_RESURRECTION_HEAL              = 42704, //Heal Max + DummyAura
    SPELL_SCOURG_RESURRECTION_BEAM              = 42857, //Channeling Beam of Annhylde
    SPELL_SCOURG_RESURRECTION_DUMMY             = 42862, //Some Emote Dummy?
    SPELL_INGVAR_TRANSFORM                      = 42796
};

class npc_annhylde_the_caller : public CreatureScript
{
public:
    npc_annhylde_the_caller() : CreatureScript("npc_annhylde_the_caller") { }
    
    struct npc_annhylde_the_callerAI : public ScriptedAI
    {
        npc_annhylde_the_callerAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            YelledText = false;
        }

        float x, y, z;
        InstanceScript* instance;
        uint32 uiResurectTimer;
        uint32 uiResurectPhase;
        bool YelledText;

        void Reset() override
        {
            //! HACK: Creature's can't have MOVEMENTFLAG_FLYING
            me->AddUnitMovementFlag(MOVEMENTFLAG_FLYING | MOVEMENTFLAG_HOVER);
            me->SetSpeedRate(MOVE_FLIGHT, 1.0f);

            me->GetPosition(x, y, z);
            DoTeleportTo(x+1.0f, y, z+30.0f);

            Unit* ingvar = ObjectAccessor::GetUnit(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_INGVAR)) : ObjectGuid::Empty);
            if (ingvar)
            {
                me->GetMotionMaster()->MovePoint(1, x, y, z+15.0f);

                if (!YelledText)
                {
                    Talk(YELL_RESSURECT);
                    YelledText = true;
                }                
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;
            Unit* ingvar = ObjectAccessor::GetUnit(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_INGVAR)) : ObjectGuid::Empty);
            if (ingvar)
            {
                switch (id)
                {
                case 1:
                    ingvar->RemoveAura(SPELL_SUMMON_BANSHEE);
                    ingvar->CastSpell(ingvar, SPELL_SCOURG_RESURRECTION_DUMMY, true);
                    DoCast(ingvar, SPELL_SCOURG_RESURRECTION_BEAM);
                    uiResurectTimer = 8000;
                    uiResurectPhase = 1;
                    break;
                case 2:
                    me->SetVisible(false);
                    me->DealDamage(me, me->GetHealth());
                    me->RemoveCorpse();
                    break;
                }
            }
        }

        void AttackStart(Unit* /*who*/) override { }
        void MoveInLineOfSight(Unit* /*who*/) override { }
        void EnterCombat(Unit* /*who*/) override { }

        void UpdateAI(uint32 diff) override
        {
            if (uiResurectTimer)
            {
                if (uiResurectTimer <= diff)
                {
                    if (uiResurectPhase == 1)
                    {
                        Unit* ingvar = ObjectAccessor::GetUnit(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_INGVAR)) : ObjectGuid::Empty);
                        if (ingvar)
                        {
                            ingvar->SetStandState(UNIT_STAND_STATE_STAND);
                            ingvar->CastSpell(ingvar, SPELL_SCOURG_RESURRECTION_HEAL, false);
                        }
                        uiResurectTimer = 3000;
                        uiResurectPhase = 2;
                    }
                    else if (uiResurectPhase == 2)
                    {
                        if (Creature* ingvar = ObjectAccessor::GetCreature(*me, instance ? ObjectGuid(instance->GetGuidData(DATA_INGVAR)) : ObjectGuid::Empty))
                        {
                            ingvar->RemoveAurasDueToSpell(SPELL_SCOURG_RESURRECTION_DUMMY);

                            if (ingvar->GetVictim())
                                if (boss_ingvar_the_plunderer::boss_ingvar_the_plundererAI* ai = CAST_AI(boss_ingvar_the_plunderer::boss_ingvar_the_plundererAI, ingvar->AI()))
                                    ai->StartZombiePhase();

                            me->GetMotionMaster()->MovePoint(2, x+1, y, z+30.0f);
                            ++uiResurectPhase;
                            uiResurectTimer = 0;
                        }
                    }
                } else uiResurectTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_annhylde_the_callerAI(creature);
    }
};

enum eShadowAxe
{
    SPELL_SHADOW_AXE_DAMAGE                     = 42750,
    H_SPELL_SHADOW_AXE_DAMAGE                   = 59719,
    POINT_TARGET                                = 28
};

class npc_ingvar_throw_dummy : public CreatureScript
{
public:
    npc_ingvar_throw_dummy() : CreatureScript("npc_ingvar_throw_dummy") { }
   
    struct npc_ingvar_throw_dummyAI : public ScriptedAI
    {
        npc_ingvar_throw_dummyAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            if (Creature* target = me->FindNearestCreature(ENTRY_THROW_TARGET, 50.0f))
            {
                float x, y, z;
                target->GetPosition(x, y, z);
                me->GetMotionMaster()->MoveCharge(x, y, z, 42.0f, POINT_TARGET);
                target->DisappearAndDie();
            }
            else
            {
                me->DisappearAndDie();
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == POINT_TARGET)
            {
                DoCastSelf(SPELL_SHADOW_AXE_DAMAGE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_REMOVE_CLIENT_CONTROL);
                if (TempSummon* summon = me->ToTempSummon())
                {
                    summon->UnSummon(10000);
                }
                else
                    me->DisappearAndDie();
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_ingvar_throw_dummyAI(creature);
    }
};

void AddSC_boss_ingvar_the_plunderer()
{
    new boss_ingvar_the_plunderer();
    new npc_annhylde_the_caller();
    new npc_ingvar_throw_dummy();
}
