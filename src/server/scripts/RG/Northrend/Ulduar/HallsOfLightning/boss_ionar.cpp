/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/*
 * Comment: Timer check pending
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "halls_of_lightning.h"
#include "SpellInfo.h"

enum Spells
{
    SPELL_BALL_LIGHTNING                          = 52780,
    H_SPELL_BALL_LIGHTNING                        = 59800,
    SPELL_STATIC_OVERLOAD                         = 52658,
    H_SPELL_STATIC_OVERLOAD                       = 59795,

    SPELL_DISPERSE                                = 52770,
    SPELL_SUMMON_SPARK                            = 52746,
    SPELL_SPARK_DESPAWN                           = 52776,

    //Spark of Ionar
    SPELL_SPARK_VISUAL_TRIGGER                    = 52667,
    H_SPELL_SPARK_VISUAL_TRIGGER                  = 59833
};

enum Yells
{
    SAY_AGGRO                                     = 0,
    SAY_SLAY                                      = 1,
    SAY_DEATH                                     = 3,
    SAY_SPLIT                                     = 2,
};

enum Creatures
{
    NPC_SPARK_OF_IONAR                            = 28926
};

enum Misc
{
    DATA_MAX_SPARKS                               = 5,
    DATA_MAX_SPARK_DISTANCE                       = 110, // Distance to boss - prevent runs through the whole instance
    DATA_POINT_CALLBACK                           = 0,
    DATA_STOP_CHASE                               = 10
};

/*######
## Boss Ionar
######*/

class boss_ionar : public CreatureScript
{
public:
    boss_ionar() : CreatureScript("boss_ionar") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_ionarAI(creature);
    }

    struct boss_ionarAI : public ScriptedAI
    {
        boss_ionarAI(Creature* creature) : ScriptedAI(creature), lSparkList(creature)
        {
            pInstance = creature->GetInstanceScript();
        }

        InstanceScript* pInstance;

        SummonList lSparkList;

        bool bIsSplitPhase;
        bool bHasDispersed;

        uint32 uiSplitTimer;

        uint32 uiStaticOverloadTimer;
        uint32 uiBallLightningTimer;

        uint32 uiDisperseHealth;

        void Reset()
        {
            lSparkList.DespawnAll();

            bIsSplitPhase = true;
            bHasDispersed = false;

            uiSplitTimer = 25*IN_MILLISECONDS;

            uiStaticOverloadTimer = urand(5*IN_MILLISECONDS, 6*IN_MILLISECONDS);
            uiBallLightningTimer = urand(10*IN_MILLISECONDS, 11*IN_MILLISECONDS);

            uiDisperseHealth = 50;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_REMOVE_CLIENT_CONTROL);

            if (!me->IsVisible())
                me->SetVisible(true);

            if (pInstance)
                pInstance->SetData(TYPE_IONAR, NOT_STARTED);
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);

            if (pInstance)
                pInstance->SetData(TYPE_IONAR, IN_PROGRESS);
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);

            lSparkList.DespawnAll();

            if (pInstance)
                pInstance->SetData(TYPE_IONAR, DONE);
        }

        void KilledUnit(Unit* /*victim*/)
        {
            Talk(SAY_SLAY);
        }

        void SpellHit(Unit* /*caster*/, const SpellInfo* spell)
        {
            if (spell->Id == SPELL_DISPERSE)
            {
                std::list<Player*> players;
                me->GetPlayerListInGrid(players, 150.f);
                for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
                {
                    Player* player = *pitr;
                    if (Creature* spark = me->SummonCreature(NPC_SPARK_OF_IONAR, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
                    {
                        spark->SetInCombatWith(player);
                        spark->GetMotionMaster()->Clear();
                        spark->GetMotionMaster()->MoveFollow(player, 0.0f, 0.0f);
                        spark->AI()->SetGuidData(player->GetGUID());
                        spark->SetSpeedRate(MOVE_RUN, 0.75f);
                    }
                }

                me->AttackStop();
                me->SetVisible(false);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_REMOVE_CLIENT_CONTROL);

                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MoveIdle();
            }
        }

        //make sparks come back
        void CallBackSparks()
        {
            //should never be empty here, but check
            if (lSparkList.empty())
                return;

            Position pos;
            me->GetPosition(&pos);

            for (ObjectGuid guid : lSparkList)
            {
                if (Creature* pSpark = ObjectAccessor::GetCreature(*me, guid))
                {
                    if (pSpark->IsAlive())
                    {
                        pSpark->AI()->SetData(DATA_STOP_CHASE, DATA_STOP_CHASE);
                        pSpark->SetSpeedRate(MOVE_RUN, 2.0f);
                        pSpark->GetMotionMaster()->Clear();
                        pSpark->GetMotionMaster()->MovePoint(DATA_POINT_CALLBACK, pos);
                    }
                    else
                        pSpark->DespawnOrUnsummon();
                }
            }
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32 &uiDamage)
        {
            if (!me->IsVisible())
                uiDamage = 0;
        }

        void JustSummoned(Creature* summoned)
        {
            if (summoned->GetEntry() == NPC_SPARK_OF_IONAR)
            {
                lSparkList.Summon(summoned);

                summoned->CastSpell(summoned, DUNGEON_MODE(SPELL_SPARK_VISUAL_TRIGGER, H_SPELL_SPARK_VISUAL_TRIGGER), true);
            }
        }

        void SummonedCreatureDespawn(Creature* summoned)
        {
            if (summoned->GetEntry() == NPC_SPARK_OF_IONAR)
                lSparkList.Despawn(summoned);
        }

        void UpdateAI(uint32 uiDiff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;



            // Splitted
            if (!me->IsVisible())
            {
                if (uiSplitTimer <= uiDiff)
                {
                    uiSplitTimer = 2500;

                    // Return sparks to where Ionar splitted
                    if (bIsSplitPhase)
                    {
                        CallBackSparks();
                        bIsSplitPhase = false;
                    }
                    // Lightning effect and restore Ionar
                    else if (lSparkList.empty())
                    {
                        me->SetVisible(true);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE|UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_REMOVE_CLIENT_CONTROL);

                        DoCastSelf(SPELL_SPARK_DESPAWN, false);

                        uiSplitTimer = 25*IN_MILLISECONDS;
                        bIsSplitPhase = true;

                        if (me->GetVictim())
                            me->GetMotionMaster()->MoveChase(me->GetVictim());
                    }
                }
                else
                    uiSplitTimer -= uiDiff;

                return;
            }

            if (uiStaticOverloadTimer <= uiDiff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(target, SPELL_STATIC_OVERLOAD);

                uiStaticOverloadTimer = urand(11*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            }
            else
                uiStaticOverloadTimer -= uiDiff;

            if (uiBallLightningTimer <= uiDiff)
            {
                if (Unit* victim = SelectTarget(SELECT_TARGET_RANDOM, 0))
                    DoCast(victim, SPELL_BALL_LIGHTNING);

                uiBallLightningTimer = urand(10*IN_MILLISECONDS, 11*IN_MILLISECONDS);
            }
            else
                uiBallLightningTimer -= uiDiff;

            // Health check
            if (!bHasDispersed && HealthBelowPct(uiDisperseHealth))
            {
                bHasDispersed = true;

                Talk(SAY_SPLIT);

                if (me->IsNonMeleeSpellCast(false))
                    me->InterruptNonMeleeSpells(false);

                DoCastSelf(SPELL_DISPERSE, false);
            }

            DoMeleeAttackIfReady();
        }
    };

};

/*######
## npc_spark_of_ionar
######*/

class npc_spark_of_ionar : public CreatureScript
{
public:
    npc_spark_of_ionar() : CreatureScript("npc_spark_of_ionar") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_spark_of_ionarAI(creature);
    }

    struct npc_spark_of_ionarAI : public ScriptedAI
    {
        npc_spark_of_ionarAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = creature->GetInstanceScript();
        }

        void Reset()
        {
            uiCheckTimer = 2*IN_MILLISECONDS;
            me->SetReactState(REACT_PASSIVE);
            _chaseDistanceTimer = IN_MILLISECONDS;
            StopChase = false;
        }

        void SetData(uint32 type, uint32 data)
        {
            if (type == DATA_STOP_CHASE && data == DATA_STOP_CHASE)
                StopChase = true;
        }

        void SetGuidData(ObjectGuid guid, uint32 /*type*/) override
        {
            _chaseGUID = guid;
        }

        void MovementInform(uint32 uiType, uint32 uiPointId)
        {
            if (uiType != POINT_MOTION_TYPE || !pInstance)
                return;

            if (uiPointId == DATA_POINT_CALLBACK)
                me->DespawnOrUnsummon();
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32 &uiDamage)
        {
            uiDamage = 0;
        }

        void UpdateAI(uint32 uiDiff)
        {
            // Despawn if the encounter is not running
            if (pInstance && pInstance->GetData(TYPE_IONAR) != IN_PROGRESS)
            {
                me->DespawnOrUnsummon();
                return;
            }

            if (_chaseDistanceTimer <= uiDiff)
            {
                if (!StopChase)
                {
                    _chaseDistanceTimer = IN_MILLISECONDS;
                    if (Player* target = ObjectAccessor::GetPlayer(*me, _chaseGUID))
                        me->GetMotionMaster()->MoveChase(target, 0.0f, 0.0f);
                }                
            }
            else
                _chaseDistanceTimer -= uiDiff;

            // Prevent them to follow players through the whole instance
            if (uiCheckTimer <= uiDiff)
            {
                if (pInstance)
                {
                    Creature* pIonar = pInstance->instance->GetCreature(ObjectGuid(pInstance->GetGuidData(DATA_IONAR)));
                    if (pIonar && pIonar->IsAlive())
                    {
                        if (me->GetDistance(pIonar) > DATA_MAX_SPARK_DISTANCE)
                        {
                            Position pos;
                            pIonar->GetPosition(&pos);

                            me->SetSpeedRate(MOVE_RUN, 2.0f);
                            me->GetMotionMaster()->Clear();
                            me->GetMotionMaster()->MovePoint(DATA_POINT_CALLBACK, pos);
                        }
                    }
                    else
                        me->DespawnOrUnsummon();
                }
                uiCheckTimer = 2*IN_MILLISECONDS;
            }
            else
                uiCheckTimer -= uiDiff;

            // No melee attack at all!
        }

    private:
        uint32 _chaseDistanceTimer;
        ObjectGuid _chaseGUID;
        InstanceScript* pInstance;
        uint32 uiCheckTimer;
        bool StopChase;
    };

};

void AddSC_boss_ionar()
{
    new boss_ionar();
    new npc_spark_of_ionar();
}
