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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SmartAI.h"
#include "GameObjectAI.h"
#include "Group.h"

/*#####
# npc_rethedron_the_subduer
######*/

enum RethedronMisc
{
    SAY_LOW_HP                      = 0,
    SAY_EVENT_END                   = 1,

    SPELL_CRIPPLE                   = 41281,
    SPELL_SHADOW_BOLT               = 41280,
    SPELL_ABYSSAL_TOSS              = 41283,                // summon npc 23416 at target position
    SPELL_ABYSSAL_IMPACT            = 41284,
    // SPELL_GROUND_AIR_PULSE       = 41270,                // spell purpose unk
    // SPELL_AGGRO_CHECK            = 41285,                // spell purpose unk
    // SPELL_AGGRO_BURST            = 41286,                // spell purpose unk

    SPELL_COSMETIC_LEGION_RING      = 41339,
    SPELL_QUEST_COMPLETE            = 41340,

    NPC_SPELLBINDER                 = 22342,
    NPC_RETHHEDRONS_TARGET          = 23416,

    POINT_ID_PORTAL_FRONT           = 0,
    POINT_ID_PORTAL                 = 1,
};

static const float afRethhedronPos[2][3] =
{
    { -1502.39f, 9772.33f, 200.421f },
    { -1557.93f, 9834.34f, 200.949f }
};

class npc_rethedron_the_subduer : public CreatureScript
{
    public : npc_rethedron_the_subduer() : CreatureScript("npc_rethedron_the_subduer") { }

    struct npc_rethedron_the_subduerAI : public ScriptedAI
    {
        npc_rethedron_the_subduerAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            CrippleTimer     = urand(5, 9) * IN_MILLISECONDS;
            ShadowBoltTimer  = urand(1, 3) * IN_MILLISECONDS;
            AbyssalTossTimer = 0 * IN_MILLISECONDS;
            DelayTimer       = 0 * IN_MILLISECONDS;

            m_bLowHpYell     = false;
            m_bEventFinished = false;
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) override
        {
            // go to epilog at 10% health
            if (!m_bEventFinished && me->GetHealthPct() < 10.0f)
            {
                me->InterruptNonMeleeSpells(false);
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MovePoint(POINT_ID_PORTAL_FRONT, afRethhedronPos[0][0], afRethhedronPos[0][1], afRethhedronPos[0][2]);
                m_bEventFinished = true;
            }

            // npc is not allowed to die
            if (me->GetHealth() < damage)
                damage = 0;
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id == POINT_ID_PORTAL_FRONT)
            {
                Talk(SAY_EVENT_END);
                me->GetMotionMaster()->MoveIdle();
                DelayTimer = 2 * IN_MILLISECONDS;
            }
            else if (id == POINT_ID_PORTAL)
            {
                DoCastSelf(SPELL_COSMETIC_LEGION_RING, true);
                DoCastSelf(SPELL_QUEST_COMPLETE, true);
                me->GetMotionMaster()->MoveIdle();
                me->DespawnOrUnsummon(2 * IN_MILLISECONDS);
                me->SetRespawnTime(60);
                me->SetCorpseDelay(60);
            }
        }

        void JustSummoned(Creature* summoned) override
        {
            if (summoned->GetEntry() == NPC_RETHHEDRONS_TARGET)
                summoned->CastSpell(summoned, SPELL_ABYSSAL_IMPACT, true);
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->DespawnOrUnsummon(2 * IN_MILLISECONDS);
            me->SetRespawnTime(60);
            me->SetCorpseDelay(60);
        }

        void UpdateAI(uint32 diff) override
        {
            if (DelayTimer)
            {
                if (DelayTimer <= diff)
                {
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MovePoint(POINT_ID_PORTAL, afRethhedronPos[1][0], afRethhedronPos[1][1], afRethhedronPos[1][2]);
                    DelayTimer = 0;
                }
                else
                    DelayTimer -= diff;
            }

            if (!me->GetVictim())
                return;

            if (m_bEventFinished)
                return;

            if (CrippleTimer < diff)
            {
                DoCastVictim(SPELL_CRIPPLE);
                CrippleTimer = urand(20, 30) * IN_MILLISECONDS;
            }
            else
                CrippleTimer -= diff;

            if (ShadowBoltTimer < diff)
            {
                DoCastVictim(SPELL_SHADOW_BOLT);
                ShadowBoltTimer = urand(20, 25) * IN_MILLISECONDS;
            }
            else
                ShadowBoltTimer -= diff;

            if (AbyssalTossTimer < diff)
            {
                DoCastVictim(SPELL_ABYSSAL_TOSS);
                AbyssalTossTimer = 30 * IN_MILLISECONDS;
            }
            else
                AbyssalTossTimer -= diff;

            if (!m_bLowHpYell && me->GetHealthPct() < 40.0f)
            {
                Talk(SAY_LOW_HP);
                m_bLowHpYell = true;
            }

            DoMeleeAttackIfReady();
        }
    private:
        uint32 CrippleTimer;
        uint32 ShadowBoltTimer;
        uint32 AbyssalTossTimer;
        uint32 DelayTimer;

        bool m_bLowHpYell;
        bool m_bEventFinished;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_rethedron_the_subduerAI(creature);
    }
};

class npc_halaa_spawns : public CreatureScript
{
public:
    npc_halaa_spawns() : CreatureScript("npc_halaa_spawns") { }

    struct npc_halaa_spawnsAI : public SmartAI
    {
        npc_halaa_spawnsAI(Creature* creature) : SmartAI(creature) 
        {
            Reset();
        }

        void EnterCombat(Unit* who)
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
                who->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
        }
        
        void JustDied(Unit* /*killer*/)
        {
            me->DespawnOrUnsummon();
            me->SetRespawnTime(240);
            me->SetCorpseDelay(60);
        }
        
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_halaa_spawnsAI(creature);
    }
};

enum MogorMisc
{
    SPELL_REVIVE = 32343,
    SPELL_RAGE   = 28747
};

class npc_mogor : public CreatureScript
{
    public:
        npc_mogor() : CreatureScript("npc_mogor") { }

        struct npc_mogorAI : public SmartAI
        {
            npc_mogorAI(Creature* creature) : SmartAI(creature) { }

            void JustReachedHome()
            {
                Respawned = false;
                SmartAI::JustReachedHome();
            }

            void DamageTaken(Unit* who, uint32& Damage)
            {
                if (Damage > me->GetHealth() && !Respawned)
                {
                    Respawned = true;
                    Damage = 0;
                    me->InterruptNonMeleeSpells(false);
                    DoCastSelf(SPELL_REVIVE, true);
                    me->SetHealth(me->GetMaxHealth());
                    me->RemoveAura(SPELL_RAGE);
                }
            }

        private:
            bool Respawned;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_mogorAI(creature);
        }
};

enum QuestBringMeTheEgg
{
    GO_MYSTERIOUS_EGG   = 183147,
    NPC_WINDROC         = 19055,

    SAY_SPAWN           = 0,

    SPELL_EAGLE_CLAW    = 30285,
    SPELL_WING_BUFFET   = 32914, 

    EVENT_EAGLE_CLAW    = 1,
    EVENT_WING_BUFFET   = 2,
};

const Position windrocPos = { -2413.4f, 6914.48f, 25.01f, 3.67f };

class go_mysterious_egg : public GameObjectScript
{
    public:
        go_mysterious_egg() : GameObjectScript("go_mysterious_egg") { }

        bool OnGossipHello(Player* player, GameObject* /*go*/) override
        {
            if (Group* group = player->GetGroup()) // all groupmember must get the credits
            {
                for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                    if (Player* member = itr->GetSource())
                        member->KillCreditGO(GO_MYSTERIOUS_EGG);
            }
            else // if no group exists
                player->KillCreditGO(GO_MYSTERIOUS_EGG);

            if (!player->FindNearestCreature(NPC_WINDROC, 75.0f))
                player->SummonCreature(NPC_WINDROC, windrocPos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);

            return false;
        }
};

const Position windrocLand = { -2400.64f, 6887.52f, -2.5f };

class npc_windroc_matriarch : public CreatureScript
{
public:
    npc_windroc_matriarch() : CreatureScript("npc_windroc_matriarch") { }

    struct npc_windroc_matriarchAI : public ScriptedAI
    {
        npc_windroc_matriarchAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            me->SetImmuneToPC(true);
            me->GetMotionMaster()->MovePath(19055, false);
            Talk(SAY_SPAWN);
            me->SetHover(true, true);
            me->SetCanFly(true);
        }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(EVENT_EAGLE_CLAW, 4000);
            _events.ScheduleEvent(EVENT_WING_BUFFET, urand(7000, 8000));
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != WAYPOINT_MOTION_TYPE)
                return;

            if (id == 2)
            {
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MoveLand(0, windrocLand);
                me->setFaction(FACTION_HOSTILE);
                me->SetImmuneToPC(false);
                me->SetHomePosition(-2400.64f, 6887.52f, -2.5f, 2.109f);

                if (me->ToTempSummon())
                    if (Unit* target = me->ToTempSummon()->GetSummoner())
                        me->GetAI()->AttackStart(target);

                _events.ScheduleEvent(EVENT_EAGLE_CLAW, 4000);
                _events.ScheduleEvent(EVENT_WING_BUFFET, urand(7000, 8000));
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_EAGLE_CLAW:
                        DoCastVictim(SPELL_EAGLE_CLAW);
                        _events.ScheduleEvent(EVENT_EAGLE_CLAW, 4000);
                        break;
                    case EVENT_WING_BUFFET:
                        DoCastVictim(SPELL_WING_BUFFET);
                        _events.ScheduleEvent(EVENT_WING_BUFFET, urand(11000, 14000));
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_windroc_matriarchAI(creature);
    }
};


void AddSC_nagrand_rg()
{
    new npc_rethedron_the_subduer();
    new npc_halaa_spawns();
    new npc_mogor();
    new go_mysterious_egg();
    new npc_windroc_matriarch();
}
