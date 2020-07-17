/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2016 Rising-Gods <https://www.rising-gods.de/>
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
#include "Player.h"
#include "ruins_of_ahnqiraj.h"

enum Spells
{
    SPELL_STINGER_SPRAY         =  25749,
    SPELL_POISON_STINGER        =  25748,
    SPELL_PARALYZE              =  25725,
    //larva
    SPELL_FEED                  =  25721,
    SPELL_FRENZY                =  8269
};

enum Events
{
    EVENT_STINGERSPRAY      = 1,
    EVENT_PARALYZE          = 2,
    EVENT_MOVE              = 3,
    EVENT_SUMMON            = 4,
    EVENT_FRENZY            = 5
};

enum MovePoints
{
    MOVE_AYAMISS_AIR     = 1,
    MOVE_AYAMISS_GROUND  = 2,
    MOVE_LARVA_START     = 3
};

enum Summons
{
    NPC_LARVA               = 15555,
    NPC_SWARMER             = 15546
};

#define FLY_HIGHT 15.0f

class boss_ayamiss : public CreatureScript
{
    public:
        boss_ayamiss() : CreatureScript("boss_ayamiss") { }

        struct boss_ayamissAI : public BossAI
        {
            boss_ayamissAI(Creature* creature) : BossAI(creature, BOSS_AYAMISS) { }
            
            void Reset() override
            {
                BossAI::Reset();
                
                me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
                Phase_Floor = false;
                Enraged     = false;
            }
            
            void EnterCombat(Unit* who) override
            {
                BossAI::EnterCombat(who);

                events.ScheduleEvent(EVENT_STINGERSPRAY,        30 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_PARALYZE, urand(2000, 3500));
                events.ScheduleEvent(EVENT_MOVE,               1.5 * IN_MILLISECONDS);
                events.ScheduleEvent(EVENT_SUMMON,   urand(30, 45) * IN_MILLISECONDS);
            }

            void MovementInform(uint32 /*type*/, uint32 id) override
            {
                if (id == MOVE_AYAMISS_GROUND)
                {
                    if (me->GetVictim())
                        me->GetMotionMaster()->MoveChase(me->GetVictim());
                    else
                        me->GetMotionMaster()->Clear();
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_STINGERSPRAY:
                            DoCastAOE(SPELL_STINGER_SPRAY);
                            events.ScheduleEvent(EVENT_STINGERSPRAY, 30 * IN_MILLISECONDS);
                            break;
                        case EVENT_PARALYZE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 300.0f, true))
                            {
                                target->NearTeleportTo(-9717.2f, 1517.81f, 27.6f, 5.4f);
                                me->AddAura(SPELL_PARALYZE, target); 
                                if (Creature* larva = me->SummonCreature(NPC_LARVA, -9678.849609f, 1526.297363f, 24.385103f, 1.829077f))
                                {
                                    larva->AI()->SetGuidData(target->GetGUID());
                                    larva->GetMotionMaster()->MovePoint(MOVE_LARVA_START, -9699.295898f, 1533.934692f, 21.444967f);
                                }
                            }
                            events.ScheduleEvent(EVENT_PARALYZE, urand(20, 40) * IN_MILLISECONDS);
                            break;
                        case EVENT_MOVE:
                            if (Unit* victim = me->GetVictim())
                                me->GetMotionMaster()->MovePoint(MOVE_AYAMISS_AIR, victim->GetPositionX(), victim->GetPositionY(), victim->GetPositionZ() + FLY_HIGHT);
                            break;
                        case EVENT_SUMMON:
                            for(uint8 i=0;i<20;i++)
                            {
                                if (Creature* swarmer = me->SummonCreature(NPC_SWARMER, me->GetPositionX()+i, me->GetPositionY()+i, me->GetPositionZ()+15, me->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000))
                                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                        swarmer->AI()->AttackStart(target);
                            }
                            events.ScheduleEvent(EVENT_SUMMON, urand(30, 45) * IN_MILLISECONDS);
                            break;
                    }
                }

                if (HealthBelowPct(70) && !Phase_Floor)
                {
                    if (Unit* target = me->GetVictim())
                        me->GetMotionMaster()->MovePoint(MOVE_AYAMISS_GROUND, me->GetPositionX(), me->GetPositionY(), target->GetPositionZ());
                    me->GetThreatManager().resetAllAggro();
                    Phase_Floor = true;
                }

                if (Phase_Floor)
                {
                    DoMeleeAttackIfReady();
                }
                else
                    DoSpellAttackIfReady(SPELL_POISON_STINGER);
                
                if (HealthBelowPct(30) && !Enraged)
                {
                    DoCastSelf(SPELL_FRENZY);
                    Enraged = true;
                }
            }

        private:
            bool Phase_Floor;
            bool Enraged;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<boss_ayamissAI>(creature);
        }
};

class npc_hive_zara_larva : public CreatureScript
{
    public:
        npc_hive_zara_larva() : CreatureScript("npc_hive_zara_larva") { }

        struct npc_hive_zara_larvaAI : public ScriptedAI
        {
            npc_hive_zara_larvaAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetReactState(REACT_PASSIVE);
            }
        
            void SetGuidData(ObjectGuid guid, uint32 /*type*/) override
            {
                VictimGUID = guid;
            }

            void Reset() override
            {
                KillTimer = 120 * IN_MILLISECONDS;
            }

            void MovementInform(uint32 /*type*/, uint32 id) override
            {
                switch (id)
                {
                    case MOVE_LARVA_START:
                        if (Unit* victim = ObjectAccessor::GetUnit(*me, VictimGUID))
                            me->GetMotionMaster()->MoveFollow(victim, 0, 0);
                        KillTimer = 3 * IN_MILLISECONDS;
                        break;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (KillTimer <= diff)
                {
                    if (Unit* victim = ObjectAccessor::GetUnit(*me, VictimGUID))
                        DoCast(victim, SPELL_FEED);
                }
                else 
                    KillTimer -= diff;
            }

        private:
            uint32 KillTimer;
            ObjectGuid VictimGUID;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return GetInstanceAI<npc_hive_zara_larvaAI>(creature);
        }
};

void AddSC_boss_ayamiss()
{
    new boss_ayamiss();
    new npc_hive_zara_larva();
}
