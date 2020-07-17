/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
#include "SpellInfo.h"
#include "temple_of_ahnqiraj.h"
#include "PassiveAI.h"

enum Spells
{
    SPELL_POISON_SHOCK          = 25993,
    SPELL_POISONBOLT_VOLLEY     = 25991,
    SPELL_TOXIN_CLOUD           = 25989,

    SPELL_SLOW                  = 26034,

    SPELL_SLOW_MORE             = 26036,
    SPELL_FREEZE                = 25937,
    SPELL_SUICIDE               = 26003,
};

enum Creatures
{
    NPC_GLOB                    = 15667,
    NPC_TRIGGER                 = 1010604,
};

enum Phase
{
    PHASE_NONE          = 1,
    PHASE_SLOW,
    PHASE_SLOW_MORE,
    PHASE_FREEZE,
    PHASE_MEELE_1,
    PHASE_MEELE_2,
    PHASE_MEELE_3,
    PHASE_BLOBS
};

enum Action
{
    ACTION_DAMAGE   = 1,
};

enum Events
{
    EVENT_POISON_SHOCK          = 1,
    EVENT_POISONBOLT_VOLLEY,
    EVENT_TOXIN_CLOUD,
    EVENT_PHASE_CHECK_1,
    EVENT_PHASE_CHECK_2,
    EVENT_PHASE_CHECK_3,
    EVENT_PHASE_CHECK_4,
    EVENT_EXPLODE,
    EVENT_VISIBLE,
    EVENT_CHECK_DISTANCE,
};

enum Texts
{
    EMOTE_SLOW       = 0,
    EMOTE_SLOW_MORE  = 1,
    EMOTE_FREEZE     = 2,
    EMOTE_MEELE_1    = 3,
    EMOTE_MEELE_2    = 4,
    EMOTE_MEELE_3    = 5,
};

#define GLOBS_JUMP_POS 1
#define EMOTE_DEFROST    "Viscidus defrosts."

Position const GlobsJumpPos[20] =
{
    {-8037.3f, 914.9f, -44.9047f, 6.0162f},
    {-8033.43f, 925.112f, -45.7227f, 5.72952f},
    {-8032.74f, 938.727f, -43.7159f, 5.56067f},
    {-8028.38f, 951.282f, -42.5365f, 5.43501f},
    {-8039.98f, 895.413f, -43.9583f, 0.243515f},
    {-8039.26f, 904.621f, -44.5158f, 0.0157485f},
    {-8021.24f, 874.924f, -45.093f, 0.71242f},
    {-8031.06f, 882.763f, -43.2667f, 0.712419f},
    {-7947.01f, 889.78f, -47.8233f, 2.04367f},
    {-7942.65f, 906.94f, -47.8282f, 2.79294f},
    {-7939.71f, 920.237f, -45.3671f, 3.30738f},
    {-7945.25f, 931.187f, -45.1496f, 3.63724f},
    {-7950.99f, 940.551f, -44.5806f, 3.38356f},
    {-7957.85f, 951.719f, -43.8528f, 3.64274f},
    {-7963.08f, 962.544f, -42.9764f, 3.73306f},
    {-7968.65f, 969.293f, -42.1423f, 3.78804f},
    {-7976.56f, 976.172f, -41.3808f, 3.94198f},
    {-7985.14f, 981.195f, -41.1404f, 4.15403f},
    {-8013.66f, 973.509f, -42.4872f, 4.95906f},
    {-8020.99f, 960.941f, -42.6628f, 5.06588f},
};

class boss_viscidus : public CreatureScript
{
    public:
        boss_viscidus() : CreatureScript("boss_viscidus") { }

        struct boss_viscidusAI : public BossAI
        {
            boss_viscidusAI(Creature* creature) : BossAI(creature, BOSS_VISCIDUS) { }
        
            void EnterCombat(Unit* who)
            {
                BossAI::EnterCombat(who);

                events.ScheduleEvent(EVENT_POISON_SHOCK,      10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_POISONBOLT_VOLLEY, 10 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_TOXIN_CLOUD,       30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_CHECK_DISTANCE,   2.5 * IN_MILLISECONDS);
            }

            void Reset() override
            {
                BossAI::Reset();
                FrostHit = 0;
                MeeleHit = 0;
                Phase = PHASE_NONE;
                DoPhase = true;
                me->SetObjectScale(1);
                me->SetVisible(true);
            }

            void JustDied(Unit* killer) override
            {
                BossAI::JustDied(killer);

                me->SetObjectScale(1);
            }

            void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
            {
                if (spell->SchoolMask == uint32(SPELL_SCHOOL_MASK_FROST))
                    FrostHit++;

                if (spell->SchoolMask == uint32(SPELL_SCHOOL_MASK_NORMAL))
                    MeeleHit++;
            }

            void DamageTaken(Unit* /*who*/, uint32& damage) override
            {
                damage = damage*0.5f;
            }

            void DoAction(int32 action) override
            {
                switch(action)
                {
                    case ACTION_DAMAGE:
                        if (me->GetHealthPct() >= 5)
                        {
                            uint32 damage = me->CountPctFromMaxHealth(5);
                            me->ModifyHealth(-int32(damage));
                            me->LowerPlayerDamageReq(damage);
                        }
                        else
                        {
                            uint32 damage = me->CountPctFromMaxHealth(3);
                            me->ModifyHealth(-int32(damage));
                            me->LowerPlayerDamageReq(damage);
                        }
                        break;
                }
            }

            void SetViscidusPhase(uint8 Phase) 
            {
                switch(Phase)
                {
                    case PHASE_NONE:
                        me->RemoveAurasDueToSpell(SPELL_SLOW);
                        me->RemoveAurasDueToSpell(SPELL_SLOW_MORE);
                        FrostHit = 0;
                        MeeleHit = 0;
                        break;
                    case PHASE_SLOW:
                        Talk(EMOTE_SLOW);
                        DoCastSelf(SPELL_SLOW);
                        break;
                    case PHASE_SLOW_MORE:
                        Talk(EMOTE_SLOW_MORE);
                        DoCastSelf(SPELL_SLOW_MORE);
                        break;
                    case PHASE_FREEZE:
                        Talk(EMOTE_FREEZE);
                        me->RemoveAurasDueToSpell(SPELL_SLOW);
                        me->RemoveAurasDueToSpell(SPELL_SLOW_MORE);
                        DoCastSelf(SPELL_FREEZE);
                        MeeleHit = 0;
                        break;
                    case PHASE_MEELE_1:
                        Talk(EMOTE_MEELE_1);
                        break;
                    case PHASE_MEELE_2:
                        Talk(EMOTE_MEELE_2);
                        break;
                    case PHASE_MEELE_3:
                        Talk(EMOTE_MEELE_3);
                        DoCastSelf(SPELL_SUICIDE);
                        events.ScheduleEvent(EVENT_EXPLODE, 0.8 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_POISON_SHOCK);
                        events.CancelEvent(EVENT_POISONBOLT_VOLLEY);
                        events.CancelEvent(EVENT_TOXIN_CLOUD);
                        break;
                }
                DoPhase = false;
            }

            void UpdateAI(uint32 diff) override
            {
                events.Update(diff);

                if (!UpdateVictim())
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch(eventId)
                    {
                        case EVENT_POISON_SHOCK:
                            DoCastVictim(SPELL_POISON_SHOCK);
                            events.ScheduleEvent(EVENT_POISON_SHOCK, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_POISONBOLT_VOLLEY:
                            DoCastAOE(SPELL_POISONBOLT_VOLLEY);
                            events.ScheduleEvent(EVENT_POISONBOLT_VOLLEY, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_TOXIN_CLOUD:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                if (Creature* trigger = me->SummonCreature(NPC_TRIGGER, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 10000))
                                    trigger->CastSpell(target, SPELL_TOXIN_CLOUD);
                            events.ScheduleEvent(EVENT_TOXIN_CLOUD, 30 * IN_MILLISECONDS);
                            break;
                        case EVENT_CHECK_DISTANCE:
                            if (me->GetDistance(me->GetHomePosition()) >= 75.0f)
                                EnterEvadeMode();
                            events.ScheduleEvent(EVENT_CHECK_DISTANCE, 2.5 * IN_MILLISECONDS);
                            break;
                        case EVENT_EXPLODE:
                            me->SetVisible(false);
                            me->SetReactState(REACT_PASSIVE);
                            {
                                uint8 amount = me->GetHealthPct()/5; //this controls how many globs will spawn the lower the life the less will spawn
                                for (uint8 i = amount; i > 0 ; --i)
                                    if (Creature* summon =  me->SummonCreature(NPC_GLOB, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), TEMPSUMMON_MANUAL_DESPAWN))
                                    {
                                        summon->GetMotionMaster()->MoveJump(GlobsJumpPos[i], 60.0f, 20.0f, GLOBS_JUMP_POS);
                                        summon->SetHomePosition(GlobsJumpPos[i]);
                                        summon->AI()->SetGuidData(me->GetGUID());
                                        summon->SetReactState(REACT_PASSIVE);
                                    }
                            }
                            events.ScheduleEvent(EVENT_VISIBLE, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_VISIBLE:
                        {
                            if (me->GetHealthPct() > 40)
                            {
                                uint8 size = me->GetHealthPct()/10; //the scale from Viscidus gets smaller when globs die
                                me->SetObjectScale(size*0.1);
                            }
                        }
                        me->SetVisible(true);
                        me->SetReactState(REACT_AGGRESSIVE);
                        SetViscidusPhase(PHASE_NONE);
                        events.ScheduleEvent(EVENT_POISON_SHOCK,      10 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_POISONBOLT_VOLLEY, 10 * IN_MILLISECONDS);
                        events.ScheduleEvent(EVENT_TOXIN_CLOUD,       30 * IN_MILLISECONDS);
                        break;
                    case EVENT_PHASE_CHECK_1:
                        SetViscidusPhase(PHASE_NONE);
                        DoPhase = true;
                        events.CancelEvent(EVENT_PHASE_CHECK_1);
                        break;
                    case EVENT_PHASE_CHECK_2:
                        me->MonsterTextEmote(EMOTE_DEFROST, 0, true);
                        SetViscidusPhase(PHASE_NONE);
                        events.CancelEvent(EVENT_PHASE_CHECK_2);
                        break;
                    case EVENT_PHASE_CHECK_3:
                        me->MonsterTextEmote(EMOTE_DEFROST, 0, true);
                        SetViscidusPhase(PHASE_SLOW);
                        FrostHit = 90;
                        events.ScheduleEvent(EVENT_PHASE_CHECK_2, 15 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_PHASE_CHECK_3);
                        break;
                    case EVENT_PHASE_CHECK_4:
                        me->MonsterTextEmote(EMOTE_DEFROST, 0, true);
                        SetViscidusPhase(PHASE_SLOW_MORE);
                        FrostHit = 140;
                        events.ScheduleEvent(EVENT_PHASE_CHECK_3, 15 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_PHASE_CHECK_4);
                        break;
                    }
                }

                switch (FrostHit)
                {
                    case 80:
                        events.ScheduleEvent(EVENT_PHASE_CHECK_1, 15 * IN_MILLISECONDS);
                        break;
                    case 90:
                        DoPhase = true;
                        break;
                    case 100:
                        if (DoPhase)
                        {
                            SetViscidusPhase(PHASE_SLOW);
                            events.ScheduleEvent(EVENT_PHASE_CHECK_2, 15 * IN_MILLISECONDS);
                            events.CancelEvent(EVENT_PHASE_CHECK_1);
                        }
                        break;
                    case 140:
                        DoPhase = true;
                        break;
                    case 150:
                        if (DoPhase)
                        {
                            SetViscidusPhase(PHASE_SLOW_MORE);
                            events.ScheduleEvent(EVENT_PHASE_CHECK_3, 15 * IN_MILLISECONDS);
                            events.CancelEvent(EVENT_PHASE_CHECK_1);
                            events.CancelEvent(EVENT_PHASE_CHECK_2);
                        }
                        break;
                    case 190:
                        DoPhase = true;
                        break;
                    case 200:
                        if (DoPhase)
                            SetViscidusPhase(PHASE_FREEZE);
                        events.ScheduleEvent(EVENT_PHASE_CHECK_4, 15 * IN_MILLISECONDS);
                        events.CancelEvent(EVENT_PHASE_CHECK_1);
                        events.CancelEvent(EVENT_PHASE_CHECK_2);
                        events.CancelEvent(EVENT_PHASE_CHECK_3);
                        break;
                }

                switch (MeeleHit)
                {
                    case 40:
                        DoPhase = true;
                        break;
                    case 50:
                        if (DoPhase)
                            SetViscidusPhase(PHASE_MEELE_1);
                        break;
                    case 90:
                        DoPhase = true;
                        break;
                    case 100:
                        if (DoPhase)
                            SetViscidusPhase(PHASE_MEELE_2);
                        break;
                    case 140:
                        DoPhase = true;
                        break;
                    case 150:
                        if (DoPhase)
                            SetViscidusPhase(PHASE_MEELE_3);
                        break;
                }

                DoMeleeAttackIfReady();
        }

        private:
            uint8 Phase;
            uint16 FrostHit; //meele- and frosthitcounter uint16 because uint8 is not enough
            uint16 MeeleHit;
            bool DoPhase;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_viscidusAI (creature);
    }
};

class boss_glob_of_viscidus : public CreatureScript
{
    public:
        boss_glob_of_viscidus() : CreatureScript("boss_glob_of_viscidus") { }

        struct boss_glob_of_viscidusAI : public PassiveAI
        {
            boss_glob_of_viscidusAI(Creature* creature) : PassiveAI(creature) { }
      
            void Reset() override
            {
                CheckRejoinTimer = 5 * IN_MILLISECONDS;
                StartMovementTimer = 5 * IN_MILLISECONDS;
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Creature* viscidus = ObjectAccessor::GetCreature(*me, ViscidusGUID))
                    viscidus->AI()->DoAction(ACTION_DAMAGE);
            }

            void SetGuidData(ObjectGuid guid, uint32 /*type*/) override
            {
                ViscidusGUID = guid;
            }

            void MovementInform(uint32 /*type*/, uint32 id) override
            {
                if (id == GLOBS_JUMP_POS)
                    StartMovementTimer = 1.5 * IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff) override
            {
                if (StartMovementTimer <= diff)
                {
                    if (Creature* viscidus = ObjectAccessor::GetCreature(*me, ViscidusGUID))
                        me->GetMotionMaster()->MoveFollow(viscidus, 0, 0);
                }
                else 
                    StartMovementTimer -= diff;


                if (CheckRejoinTimer <= diff)
                {
                    if (Creature* viscidus = ObjectAccessor::GetCreature(*me, ViscidusGUID))
                        if (me->IsWithinDist(viscidus, 2.0f, false))
                            me->DespawnOrUnsummon();

                    CheckRejoinTimer = 1 * IN_MILLISECONDS;
                }
                else 
                    CheckRejoinTimer -= diff;
            }

        private:
            uint32 CheckRejoinTimer;
            uint32 StartMovementTimer;
            ObjectGuid ViscidusGUID;
        };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<boss_glob_of_viscidusAI>(creature);
    }
};

void AddSC_boss_viscidus()
{
    new boss_viscidus();
    new boss_glob_of_viscidus();
}
