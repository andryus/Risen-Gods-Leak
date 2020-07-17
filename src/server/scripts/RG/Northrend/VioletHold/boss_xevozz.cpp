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
#include "violet_hold.h"
#include "Player.h"

enum Spells
{
    SPELL_ARCANE_BARRAGE_VOLLEY                 = 54202,
    SPELL_ARCANE_BARRAGE_VOLLEY_H               = 59483,
    SPELL_ARCANE_BUFFET                         = 54226,
    SPELL_ARCANE_BUFFET_H                       = 59485,
    SPELL_SUMMON_ETHEREAL_SPHERE_1              = 54102,
    SPELL_SUMMON_ETHEREAL_SPHERE_2              = 54137,
    SPELL_SUMMON_ETHEREAL_SPHERE_3              = 54138
};

enum NPCs
{
    NPC_ETHEREAL_SPHERE                         = 29271,
};

enum CreatureSpells
{
    SPELL_ARCANE_POWER                          = 54160,
    H_SPELL_ARCANE_POWER                        = 59474,
    SPELL_SUMMON_PLAYERS                        = 54164,
    SPELL_POWER_BALL_VISUAL                     = 54141,
    SPELL_ARCANE_TEMPEST                        = 35845
};

enum Yells
{
    SAY_AGGRO                                   = 0,
    SAY_SLAY                                    = 1,
    SAY_DEATH                                   = 2,
    SAY_SPAWN                                   = 3,
    SAY_CHARGED                                 = 4,
    SAY_REPEAT_SUMMON                           = 5,
    SAY_SUMMON_ENERGY                           = 6
};

class boss_xevozz : public CreatureScript
{
public:
    boss_xevozz() : CreatureScript("boss_xevozz") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_xevozzAI (creature);
    }

    struct boss_xevozzAI : public ScriptedAI
    {
        boss_xevozzAI(Creature* creature) : ScriptedAI(creature)
        {
            instance  = creature->GetInstanceScript();
        }

        InstanceScript* instance;
      
        void Reset()
        {
            if (instance)
            {
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                    instance->SetData(DATA_1ST_BOSS_EVENT, NOT_STARTED);
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                    instance->SetData(DATA_2ND_BOSS_EVENT, NOT_STARTED);
            }

            uiSummonEtherealSphere_Timer = urand(10, 12) * IN_MILLISECONDS;
            uiArcaneBarrageVolley_Timer  = urand(20, 22) * IN_MILLISECONDS;
            uiArcaneBuffet_Timer = uiSummonEtherealSphere_Timer + urand(5, 6) * IN_MILLISECONDS;
            DespawnSphere();
        }

        void DespawnSphere()
        {
            std::list<Creature*> assistList;
            me->GetCreatureListWithEntryInGrid(assistList, NPC_ETHEREAL_SPHERE, 150.0f);

            if (assistList.empty())
                return;

            for (std::list<Creature*>::const_iterator iter = assistList.begin(); iter != assistList.end(); ++iter)
            {
                if (Creature* pSphere = *iter)
                    pSphere->Kill(pSphere, false);
            }
        }

        void JustSummoned(Creature* summoned)
        {
            summoned->SetSpeedRate(MOVE_RUN, 0.5f);
            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
            {
                AddThreat(target, 0.00f, summoned);
                summoned->AI()->AttackStart(target);
            }
        }

        void AttackStart(Unit* who)
        {
            if (me->IsImmuneToPC() || me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                return;

            if (me->Attack(who, true))
            {
                AddThreat(who, 0.0f);
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);
                DoStartMovement(who);
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            Talk(SAY_AGGRO);
            if (instance)
            {
                if (GameObject* pDoor = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(DATA_XEVOZZ_CELL))))
                    if (pDoor->GetGoState() == GO_STATE_READY)
                    {
                        EnterEvadeMode();
                        return;
                    }
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                    instance->SetData(DATA_1ST_BOSS_EVENT, IN_PROGRESS);
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                    instance->SetData(DATA_2ND_BOSS_EVENT, IN_PROGRESS);
            }
        }

        void MoveInLineOfSight(Unit* /*who*/) { }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (uiArcaneBarrageVolley_Timer < diff)
            {
                DoCastSelf(SPELL_ARCANE_BARRAGE_VOLLEY);
                uiArcaneBarrageVolley_Timer = urand(20, 22) * IN_MILLISECONDS;
            }
            else uiArcaneBarrageVolley_Timer -= diff;

            if (uiArcaneBuffet_Timer)
            {
                if (uiArcaneBuffet_Timer < diff)
                {
                    DoCastVictim(SPELL_ARCANE_BUFFET, true);
                    uiArcaneBuffet_Timer = 0 * IN_MILLISECONDS;
                }
                else uiArcaneBuffet_Timer -= diff;
            }

            if (uiSummonEtherealSphere_Timer < diff)
            {
                Talk(SAY_SPAWN);
                DoCastSelf(SPELL_SUMMON_ETHEREAL_SPHERE_1);
                if (IsHeroic()) // extra one for heroic
                    me->SummonCreature(NPC_ETHEREAL_SPHERE, me->GetPositionX()-5+rand32()%10, me->GetPositionY()-5+rand32()%10, me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 40000);

                uiSummonEtherealSphere_Despawn_Timer = 40 * IN_MILLISECONDS;
                uiSummonEtherealSphere_Timer = urand(45, 47) * IN_MILLISECONDS;
                uiArcaneBuffet_Timer = urand(5, 6) * IN_MILLISECONDS;
            }
            else uiSummonEtherealSphere_Timer -= diff;

            if (uiSummonEtherealSphere_Despawn_Timer < diff)
            {
                DespawnSphere();
            }
            else
                uiSummonEtherealSphere_Despawn_Timer -= diff;

            DoMeleeAttackIfReady();
        }

        void JustDied(Unit* /*killer*/)
        {
            Talk(SAY_DEATH);

            DespawnSphere();

            if (instance)
            {
                if (instance->GetData(DATA_WAVE_COUNT) == 6)
                {
                    instance->SetData(DATA_1ST_BOSS_EVENT, DONE);
                    instance->SetData(DATA_WAVE_COUNT, 7);
                }
                else if (instance->GetData(DATA_WAVE_COUNT) == 12)
                {
                    instance->SetData(DATA_2ND_BOSS_EVENT, NOT_STARTED);
                    instance->SetData(DATA_WAVE_COUNT, 13);
                }
            }
        }
        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(SAY_SLAY);
        }
        private:
            uint32 uiSummonEtherealSphere_Timer;
            uint32 uiSummonEtherealSphere_Despawn_Timer;
            uint32 uiArcaneBarrageVolley_Timer;
            uint32 uiArcaneBuffet_Timer;
    };

};

class npc_ethereal_sphere : public CreatureScript
{
public:
    npc_ethereal_sphere() : CreatureScript("npc_ethereal_sphere") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ethereal_sphereAI (creature);
    }

    struct npc_ethereal_sphereAI : public ScriptedAI
    {
        npc_ethereal_sphereAI(Creature* creature) : ScriptedAI(creature)
        {
            instance   = creature->GetInstanceScript();
        }

        InstanceScript* instance;
       
        void Reset()
        {
            uiSummonPlayers_Timer = urand(33, 35) * IN_MILLISECONDS;
            uiRangeCheck_Timer    =             1 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff)
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            if (uiRangeCheck_Timer < diff)
            {
                if (instance)
                {
                    if (Creature* pXevozz = ObjectAccessor::GetCreature(*me, ObjectGuid(instance->GetGuidData(DATA_XEVOZZ))))
                    {
                        float fDistance = me->GetDistance2d(pXevozz);
                        if (fDistance <= 3)
                            DoCast(pXevozz, SPELL_ARCANE_POWER);
                        else
                            DoCastSelf(SPELL_ARCANE_TEMPEST); //Is it blizzlike?
                    }
                }
                uiRangeCheck_Timer = 1 * IN_MILLISECONDS;
            }
            else uiRangeCheck_Timer -= diff;

            if (uiSummonPlayers_Timer < diff)
            {                
                if (Creature* xevozz = me->FindNearestCreature(CREATURE_XEVOZZ, 300.0f))
                    if (xevozz->IsWithinLOS(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
                    {
                        // Port Xevozz
                        xevozz->NearTeleportTo(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), true);
                        // Port Player
                        Map* map = me->GetMap();
                        if (map && map->IsDungeon())
                        {
                            for (Player& player : map->GetPlayers())
                                if (player.IsAlive())
                                    DoTeleportPlayer(&player, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), player.GetOrientation());
                        }

                        DoCastSelf(SPELL_SUMMON_PLAYERS); // not working right
                    }

                uiSummonPlayers_Timer = urand(33, 35) * IN_MILLISECONDS;
            }
            else uiSummonPlayers_Timer -= diff;
        }
    private:
        uint32 uiSummonPlayers_Timer;
        uint32 uiRangeCheck_Timer;
    };

};

void AddSC_boss_xevozz()
{
    new boss_xevozz();
    new npc_ethereal_sphere();
}
