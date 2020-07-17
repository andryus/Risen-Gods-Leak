/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2014-2015 Rising Gods <http://www.rising-gods.de/>
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
#include "ScriptedGossip.h"
#include "Player.h"

/*###
## npc_thrall_og
###*/

#define POS_PORTAL_OG_TO_SW     1921.25f, -4148.62f, 40.63f
#define POS_PORTAL_OG_TO_UC_1   1911.84f, -4148.88f, 40.63f
#define POS_PORTAL_OG_TO_UC_2   1931.74f, -4149.33f, 40.63f
#define POS_JAINA_MOVE_1        1920.612f, -4139.77f, 40.589f
#define POS_WINDRUNNER_MOVE_1   1917.376f, -4136.041f, 40.569f
#define POS_THRALL_MOVE_1       1920.878f, -4136.392f, 40.534f

enum ThrallOg
{
    NPC_JAINA            = 32364,
    NPC_WINDRUNNER       = 32365,
    NPC_WORLDTRIGGER     = 22515,
    NPC_KOR_KRON_GUARD  = 35460,
    
    ACTION_SUMMON_PORTAL = 1,

    PHASEMASK_HORDE      = 64,
    PHASEMASK_ALLIANCE   = 128,
    
    QUEST_HERALD_OF_WAR  = 13257,
    
    GO_OG_PORTAL_TO_UC   = 193955,
    GO_OG_PORTAL_TO_SW   = 193207,

    SPELL_COSMETIC_TELEPORT_EFFECT  = 52096
};

class npc_thrall_og : public CreatureScript
{
public:
    npc_thrall_og() : CreatureScript("npc_thrall_og") { }

    bool OnQuestReward(Player* player, Creature* creature, const Quest* quest, uint32 /*slot*/)
    {
        //short way only for horde
        if (quest->GetQuestId() == QUEST_HERALD_OF_WAR)
                creature->AI()->DoAction(ACTION_SUMMON_PORTAL);
        
        return true;
    }
 
    struct npc_thrall_ogAI : public ScriptedAI
    {
        npc_thrall_ogAI(Creature* c) : ScriptedAI(c), summons(me) 
        {
            memset(GOs,  0, sizeof(GOs));
        }

        bool isActive;
        uint32 step;
        uint32 stepTimer;
        
        SummonList summons;
        GameObject* GOs[3];
        
        uint32 talkThrall;
        uint32 talkJaina;
        uint32 talkWindrunner;

        void Reset()
        {
            isActive = false;
            step = 0;
            stepTimer = 0;
            
            talkThrall = 0;
            talkJaina = 0;
            talkWindrunner = 0;
            
            for (uint32 i = 0; i < 3; i++)
            {
                if (GameObject* go = GOs[i])
                {
                    go->Delete();
                    GOs[i] = NULL;
                }
            }
            summons.DespawnAll();

            me->SetWalk(true);
        }

        void DoAction(int32 action)
        {
            if (action == ACTION_SUMMON_PORTAL)
            {
                if(!me->FindNearestGameObject(GO_OG_PORTAL_TO_UC, 15.0f))
                {
                    if (GameObject* go = me->SummonGameObject(GO_OG_PORTAL_TO_UC, POS_PORTAL_OG_TO_UC_1, 0, 0, 0, 0, 0, 180 * IN_MILLISECONDS))
                        GOs[0] = go;

                    if (GameObject* go = me->SummonGameObject(GO_OG_PORTAL_TO_UC, POS_PORTAL_OG_TO_UC_2, 0, 0, 0, 0, 0, 180 * IN_MILLISECONDS))
                        GOs[1] = go;
                }
            }
        }
        
        void JustSummoned(Creature* summon)
        {
            summons.Summon(summon);
        }
        
        void JumpToNextStep(uint32 time)
        {
            stepTimer = time * IN_MILLISECONDS;
            ++step;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!who && 
                who->GetPhaseMask() != (PHASEMASK_HORDE | PHASEMASK_ALLIANCE))
                return;
            
            if (who->GetTypeId() == TYPEID_PLAYER)
            {
                if (isActive == false && who->GetDistance2d(me) <= 25.0f)
                    isActive = true;
                // cancel event, if player is to far away
                else if (isActive == true && who->GetDistance2d(me) >= 70.0f)
                    Reset();
            }
        }
        
        void UpdateAI(uint32 diff)
        {
            if (isActive)
            {
                if (stepTimer <= diff)
                {
                    switch (step)
                    {
                        case 0:
                            //me->CastSpell(SPELL_, me->FindNearestCreautre(NC_WORLDTRIGGER,20.0f), true);
                            if (GameObject* go = me->SummonGameObject(GO_OG_PORTAL_TO_SW, POS_PORTAL_OG_TO_SW, 0, 0, 0, 0, 0, 180 * IN_MILLISECONDS))
                                GOs[2] = go;
                            JumpToNextStep(1);
                            break;
                        case 1:
                            me->GetMotionMaster()->MovePoint(0, POS_THRALL_MOVE_1);
                            if(Creature* windrunner = me->FindNearestCreature(NPC_WINDRUNNER,20.0f))
                            {
                                windrunner->SetWalk(true);
                                windrunner->HandleEmoteCommand(ANIM_KNEEL_END);
                                if (Creature* trigger = me->FindNearestCreature(NPC_WORLDTRIGGER, 20.0f))
                                    windrunner->SetFacingToObject(trigger);
                            }
                            
                            if (true)
                            {
                                std::list<Creature*> GuardsList;
                                me->GetCreatureListWithEntryInGrid(GuardsList, NPC_KOR_KRON_GUARD, 35.0f);
                                if (GuardsList.size())
                                    for (std::list<Creature*>::iterator itr = GuardsList.begin(); itr != GuardsList.end(); ++itr)
                                        if (Creature* trigger = me->FindNearestCreature(NPC_WORLDTRIGGER, 20.0f))
                                            (*itr)->GetMotionMaster()->MoveChase(trigger);
                            }
                            
                            JumpToNextStep(4);
                            break;
                        case 2:
                            Talk(talkThrall++);
                            if(Creature* trigger = me->FindNearestCreature(NPC_WORLDTRIGGER, 20.0f))
                                if (Creature* jaina = me->SummonCreature(NPC_JAINA, *trigger))
                                    jaina->CastSpell(jaina, SPELL_COSMETIC_TELEPORT_EFFECT, true);
                            
                            if (true)
                            {
                                std::list<Creature*> GuardsList;
                                me->GetCreatureListWithEntryInGrid(GuardsList, NPC_KOR_KRON_GUARD, 35.0f);
                                if (GuardsList.size())
                                    for (std::list<Creature*>::iterator itr = GuardsList.begin(); itr != GuardsList.end(); ++itr)
                                        (*itr)->GetMotionMaster()->MoveTargetedHome();
                            }                                    
                            
                            JumpToNextStep(2);
                            break;
                        case 3:
                            Talk(talkThrall++);
                            JumpToNextStep(2);
                            break;
                        case 4:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                            {
                                jaina->SetWalk(true);
                                jaina->GetMotionMaster()->MovePoint(0, POS_JAINA_MOVE_1);
                            }
                            JumpToNextStep(3);
                            break;
                        case 5:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                                jaina->AI()->Talk(talkJaina++);
                            JumpToNextStep(8);
                            break;
                        case 6:
                            Talk(talkThrall++);
                            JumpToNextStep(8);
                            break;
                        case 7  :
                            if(Creature* windrunner = me->FindNearestCreature(NPC_WINDRUNNER, 20.0f))
                                windrunner->GetMotionMaster()->MovePoint(0, POS_WINDRUNNER_MOVE_1); 
                            JumpToNextStep(6);
                            break;
                        case 8:
                            if(Creature* windrunner = me->FindNearestCreature(NPC_WINDRUNNER, 20.0f))
                                if (Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                                {
                                    windrunner->AI()->Talk(talkWindrunner++);
                                    windrunner->SetFacingToObject(jaina);
                                }
                            JumpToNextStep(12);
                            break;
                        case 9:
                            if(Creature* windrunner = me->FindNearestCreature(NPC_WINDRUNNER, 20.0f))
                                windrunner->AI()->Talk(talkWindrunner++);
                            JumpToNextStep(16);
                            break;
                        case 10:
                            Talk(talkThrall++);
                            JumpToNextStep(5);
                            break;
                        case 11:
                            Talk(talkThrall++);
                            JumpToNextStep(10);
                            break;
                        case 12:
                            Talk(talkThrall++);
                            JumpToNextStep(7);
                            break;
                        case 13:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                                jaina->AI()->Talk(talkJaina++);
                            JumpToNextStep(4);
                            break;
                        case 14:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                                jaina->AI()->Talk(talkJaina++);
                            JumpToNextStep(15);
                            break;
                        case 15:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                                jaina->AI()->Talk(talkJaina++);
                            JumpToNextStep(9);
                            break;
                        case 16:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                                jaina->AI()->Talk(talkJaina++);
                            JumpToNextStep(8);
                            break;
                        case 17:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                                if (Creature* trigger = me->FindNearestCreature(NPC_WORLDTRIGGER, 20.0f))
                                    jaina->GetMotionMaster()->MovePoint(0, *trigger);
                            JumpToNextStep(2);
                            break;
                        case 18:
                            if(Creature* jaina = me->FindNearestCreature(NPC_JAINA, 20.0f))
                            {
                                jaina->CastSpell(jaina, SPELL_COSMETIC_TELEPORT_EFFECT, true);
                                jaina->DespawnOrUnsummon(1500);
                            }
                            JumpToNextStep(7);
                            break;
                        case 19:
                            Talk(talkThrall++);
                            DoAction(ACTION_SUMMON_PORTAL);
                            JumpToNextStep(5);
                            break;
                        case 20:
                            me->GetMotionMaster()->MoveTargetedHome();
                            if(Creature* windrunner = me->FindNearestCreature(NPC_WINDRUNNER, 20.0f))
                                windrunner->GetMotionMaster()->MoveTargetedHome();
                            //TODO: We need a bedder way to Reset
                            JumpToNextStep(60);
                            break;
                        case 21:
                            Reset();
                            break;
                    }
                } else stepTimer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_thrall_ogAI(creature);
    }
};

/*##
## npc_jaina_sw
##*/

enum jainaSW
{
    GOSSIP_MENU_JAINA_SW                = 60080,
    SPELL_SW_PORTAL                     = 60904,
    GO_PORTAL_SW_TO_OG                  = 193948,
    
    QUEST_FATE_UP_AGAINST_YOUR_WILL     = 13369,
    ACTION_ACTIVE                       = 1
};

#define POS_PORTAL_SW_TO_OG             -8447.163f, 332.150f, 121.746f

// Maybe write in SmartAI
class npc_jaina_sw : public CreatureScript
{
public:
    npc_jaina_sw() : CreatureScript("npc_jaina_sw") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
        {
            creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            creature->AI()->SetGuidData(player->GetGUID(), ACTION_ACTIVE);
        }
        
        player->CLOSE_GOSSIP_MENU();
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(QUEST_FATE_UP_AGAINST_YOUR_WILL) == QUEST_STATUS_COMPLETE)
            player->AddGossipItem(GOSSIP_MENU_JAINA_SW, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

    struct npc_jaina_stormwindAI : public ScriptedAI
    {
        npc_jaina_stormwindAI(Creature* c) : ScriptedAI(c) {}

        uint32 step;
        uint32 timer;
        bool isActive;
        ObjectGuid playerGUID;

        void Reset()
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            
            if (GameObject* go = me->FindNearestGameObject(GO_PORTAL_SW_TO_OG, 15.0f))
                go->Delete();
            
            step = 0;
            timer = 1000;
            isActive = false;
            playerGUID.Clear();
        }
        
        void SetGuidData(ObjectGuid guid, uint32 id) override
        {
            if (id == ACTION_ACTIVE)
            {
                isActive = true;
                playerGUID = guid;
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (isActive)
            {
                if (timer <= diff)
                {
                    switch (step)
                    {
                        case 0:
                            Talk(0, ObjectAccessor::GetPlayer(*me, playerGUID));
                            timer = 6000;
                            step++;
                            break;
                        case 1:
                            if(!me->FindNearestGameObject(GO_PORTAL_SW_TO_OG, 15.0f))
                                DoCastSelf(SPELL_SW_PORTAL, false);
                            timer = 5000;
                            step++;
                            break;
                        case 2:
                            if(!me->FindNearestGameObject(GO_PORTAL_SW_TO_OG, 15.0f))
                                me->SummonGameObject(GO_PORTAL_SW_TO_OG, POS_PORTAL_SW_TO_OG, 0, 0, 0, 0, 0, 60 * IN_MILLISECONDS);
                            timer = 2000;
                            step++;
                        case 3:
                            Talk(1, ObjectAccessor::GetPlayer(*me, playerGUID));
                            timer = 2000;
                            step++;
                            break;
                        case 4:
                            timer = 60 * IN_MILLISECONDS;
                            step++;
                            break;
                        case 5:
                            Reset();
                            break;
                    }
                } else timer -= diff;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_jaina_stormwindAI(creature);
    }
};

enum rundthak
{
    NPC_GAMON       = 31424,
    NPC_KAJA        = 31423,
    NPC_THATUNG     = 31430,
    NPC_GRYSHKA     = 31433,
    NPC_SANA        = 31429,
    NPC_FELIKA      = 31427
};

class npc_overlord_rundthak_bftu : public CreatureScript
{
    public:

        npc_overlord_rundthak_bftu() : CreatureScript("npc_overlord_rundthak_bftu") { }

        struct npc_overlord_rundthak_bftuAI : public ScriptedAI
        {
            npc_overlord_rundthak_bftuAI(Creature* creature) : ScriptedAI(creature) 
            {
                memset(creatures,  0, sizeof(creatures));
            }
            
            uint32 resetTimer;
            uint32 talkTimer;
            bool vendorsTalk;
            uint32 talkStep;
            uint32 getOnNervesCounter;

            Creature* creatures[6];
            
            void Reset()
            {
                resetTimer          = 120 * IN_MILLISECONDS;
                talkTimer           = 1 * IN_MILLISECONDS;
                vendorsTalk         = true;
                talkStep            = 0;
                getOnNervesCounter  = 0;

                if (Creature* c = me->FindNearestCreature(NPC_GAMON, 20.0f))
                    creatures[0] = c;
                if (Creature* c = me->FindNearestCreature(NPC_KAJA, 20.0f))
                    creatures[1] = c;
                if (Creature* c = me->FindNearestCreature(NPC_THATUNG, 20.0f))
                    creatures[2] = c;
                if (Creature* c = me->FindNearestCreature(NPC_GRYSHKA, 20.0f))
                    creatures[3] = c;
                if (Creature* c = me->FindNearestCreature(NPC_SANA, 20.0f))
                    creatures[4] = c;
                if (Creature* c = me->FindNearestCreature(NPC_FELIKA, 20.0f))
                    creatures[5] = c;
            }
            
            void UpdateAI(uint32 diff)
            {
                if (resetTimer <= diff)
                {
                    Reset();
                    resetTimer = 120 * IN_MILLISECONDS;
                } else resetTimer -= diff;

                if (talkTimer <= diff)
                {
                    if (vendorsTalk)
                    {
                        if (Creature* talker = creatures[urand(0, 5)])
                            talker->AI()->Talk(0);

                        if (getOnNervesCounter++ >= 10)
                            vendorsTalk = false;

                        talkTimer = urand(4, 10) * IN_MILLISECONDS;
                    } else 
                    {
                        Talk(talkStep);

                        if (talkStep++ >= 3)
                            talkTimer = 120 * IN_MILLISECONDS;
                        else talkTimer = 3 * IN_MILLISECONDS;
                    }
                } else talkTimer -= diff;
	        }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_overlord_rundthak_bftuAI(creature);
        }
};

void AddSC_battle_for_the_undercity()
{
    new npc_thrall_og();
    new npc_jaina_sw();
    new npc_overlord_rundthak_bftu();
}
