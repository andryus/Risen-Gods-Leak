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
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "blackrock_depths.h"
#include "LFGMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "SpellInfo.h"
#include "Group.h"
#include "SpellAuraEffects.h" 			

//go_shadowforge_brazier
class go_shadowforge_brazier : public GameObjectScript
{
public:
    go_shadowforge_brazier() : GameObjectScript("go_shadowforge_brazier") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go) override
    {
        if (InstanceScript* instance = go->GetInstanceScript())
        {
            if (instance->GetData(TYPE_LYCEUM) == IN_PROGRESS)
                instance->SetData(TYPE_LYCEUM, DONE);
            else
                instance->SetData(TYPE_LYCEUM, IN_PROGRESS);
            // If used brazier open linked doors (North or South)
            if (go->GetGUID() == ObjectGuid(instance->GetGuidData(DATA_SF_BRAZIER_N)))
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_GOLEM_DOOR_N)), true);
            else if (go->GetGUID() == ObjectGuid(instance->GetGuidData(DATA_SF_BRAZIER_S)))
                instance->HandleGameObject(ObjectGuid(instance->GetGuidData(DATA_GOLEM_DOOR_S)), true);
        }
        return false;
    }
};

uint32 RingMob[]=
{
    8925,                                                   // Dredge Worm
    8926,                                                   // Deep Stinger
    8927,                                                   // Dark Screecher
    8928,                                                   // Burrowing Thundersnout
    8933,                                                   // Cave Creeper
    8932,                                                   // Borer Beetle
};

uint32 RingBoss[]=
{
    9027,                                                   // Gorosh
    9028,                                                   // Grizzle
    9029,                                                   // Eviscerator
    9030,                                                   // Ok'thor
    9031,                                                   // Anub'shiah
    9032,                                                   // Hedrum
};

class at_ring_of_law : public AreaTriggerScript
{
public:
    at_ring_of_law() : AreaTriggerScript("at_ring_of_law") { }

    bool OnTrigger(Player* player, const AreaTriggerEntry* /*at*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            if (instance->GetData(TYPE_RING_OF_LAW) == IN_PROGRESS || instance->GetData(TYPE_RING_OF_LAW) == DONE)
                return false;

            instance->SetData(TYPE_RING_OF_LAW, IN_PROGRESS);
            player->SummonCreature(NPC_GRIMSTONE, 625.559f, -205.618f, -52.735f, 2.609f, TEMPSUMMON_DEAD_DESPAWN, 0);

            return false;
        }
        return false;
    }

};

/*######
## npc_grimstone
######*/

enum GrimstoneTexts
{
    SAY_TEXT1                   = 0,
    SAY_TEXT2                   = 1,
    SAY_TEXT3                   = 2,
    SAY_TEXT4                   = 3,
    SAY_TEXT5                   = 4,
    SAY_TEXT6                   = 5
};

enum GrimstoneSpells
{
    // Other spells used by Grimstone
    SPELL_ASHCROMBES_TELEPORT_A = 15742,
    SPELL_ASHCROMBES_TELEPORT_B = 6422,
    SPELL_ARENA_FLASH_A         = 15737,
    SPELL_ARENA_FLASH_B         = 15739,
    SPELL_ARENA_FLASH_C         = 15740,
    SPELL_ARENA_FLASH_D         = 15741,
};

class npc_grimstone : public CreatureScript
{
public:
    npc_grimstone() : CreatureScript("npc_grimstone") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<npc_grimstoneAI>(creature);
    }

    struct npc_grimstoneAI : public npc_escortAI
    {
        npc_grimstoneAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
            initAddList();
        }

        InstanceScript* instance;

        uint8 EventPhase;
        uint32 Event_Timer;

        uint8 MobCount;
        uint32 MobDeath_Timer;

        ObjectGuid RingMobGUID[4];
        ObjectGuid RingBossGUID;
        ObjectGuid RingTheldrenAddGUID[4];
        uint32 uiTheldrenAdds[8];

        bool CanWalk;

        void Reset() override
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

            EventPhase = 0;
            Event_Timer = 1000;

            MobCount = 0;
            MobDeath_Timer = 0;

            for (uint8 i = 0; i < MAX_NPC_AMOUNT; ++i)
                RingMobGUID[i].Clear();

            for (uint8 i = 0; i < 4; ++i)
                RingTheldrenAddGUID[i].Clear();

            RingBossGUID.Clear();

            CanWalk = false;
            initAddList();
        }

        //TODO: move them to center
        void SummonRingMob()
        {
            if (Creature* tmp = me->SummonCreature(RingMob[rand32()%6], 608.960f, -235.322f, -53.907f, 1.857f, TEMPSUMMON_DEAD_DESPAWN, 0))
            {
                RingMobGUID[MobCount] = tmp->GetGUID();
                tmp->GetMotionMaster()->MovePoint(0,594.732f+urand(0,5),-203.786f+urand(0,5),-53.959f);
            }

            ++MobCount;

            if (MobCount == MAX_NPC_AMOUNT)
                MobDeath_Timer = 2500;
        }

        uint32 GetRandomAdd()
        {
            uint32 uiRand;
            uint32 uiEntry;
            while(true)
            {
                uiRand = urand(0,7);
                if(uiTheldrenAdds[uiRand]!=0)
                {
                    uiEntry = uiTheldrenAdds[uiRand];
                    uiTheldrenAdds[uiRand]=0;
                    return uiEntry;
                }
            }
        }

        void initAddList()
        {
            uiTheldrenAdds[0] = 16053;
            uiTheldrenAdds[1] = 16055;
            uiTheldrenAdds[2] = 16050;
            uiTheldrenAdds[3] = 16051;
            uiTheldrenAdds[4] = 16049;
            uiTheldrenAdds[5] = 16052;
            uiTheldrenAdds[6] = 16054;
            uiTheldrenAdds[7] = 16058;
        }

        void SummonRingBoss()
        {
            if(me->FindNearestGameObject(GO_BANNER_OF_PROVOCATION,60.0f))
            {
                //Summon Theldren
                if(Creature* theldren = me->SummonCreature(NPC_THELDREN,643.703857f,-176.493271f,-53.604298f,3.381721f))
                {
                    initAddList();                    
                    float TheldrenOrientation = 3.381721f;

                    //Summon and move Add 1
                    if(Creature* add = theldren->SummonCreature(GetRandomAdd(),theldren->GetPositionX()+1,theldren->GetPositionY()+1,theldren->GetPositionZ(),TheldrenOrientation))
                    {
                        RingTheldrenAddGUID[0] = add->GetGUID();
                        add->GetMotionMaster()->MovePoint(0,615.806213f,-189.081802f,-53.49757f);
                        add->SetHomePosition(615.806213f,-189.081802f,-53.49757f,TheldrenOrientation);
                    }

                    //Summon and move Add 2
                    if(Creature* add = theldren->SummonCreature(GetRandomAdd(),theldren->GetPositionX()-1,theldren->GetPositionY()+1,theldren->GetPositionZ(),TheldrenOrientation))
                    {
                        RingTheldrenAddGUID[1] = add->GetGUID();
                        add->GetMotionMaster()->MovePoint(0,615.032288f,-185.874588f,-53.54398f);
                        add->SetHomePosition(615.032288f,-185.874588f,-53.54398f,TheldrenOrientation);
                    }

                    //Summon and move Add 3
                    if(Creature* add = theldren->SummonCreature(GetRandomAdd(),theldren->GetPositionX()+1,theldren->GetPositionY()-1,theldren->GetPositionZ(),TheldrenOrientation))
                    {
                        RingTheldrenAddGUID[2] = add->GetGUID();
                        add->GetMotionMaster()->MovePoint(0,614.125122f,-182.482590f,-53.55122f);
                        add->SetHomePosition(614.125122f,-182.482590f,-53.55122f,TheldrenOrientation);
                    }

                    //Summon and move Add 4
                    if(Creature* add = theldren->SummonCreature(GetRandomAdd(),theldren->GetPositionX()-1,theldren->GetPositionY()-1,theldren->GetPositionZ(),TheldrenOrientation))
                    {
                        RingTheldrenAddGUID[3] = add->GetGUID();
                        add->GetMotionMaster()->MovePoint(0,613.287903f,-179.352188f,-53.52018f);
                        add->SetHomePosition(613.287903f,-179.352188f,-53.52018f,TheldrenOrientation);
                    }

                    //Move Theldren
                    theldren->GetMotionMaster()->MovePoint(0,612.10022f,-184.725967f,-53.681396f);
                    theldren->SetHomePosition(612.10022f,-184.725967f,-53.681396f,TheldrenOrientation);
                    RingBossGUID = theldren->GetGUID();

                    while(GameObject* banner = me->FindNearestGameObject(GO_BANNER_OF_PROVOCATION,60.0f))
                        banner->Delete();
                }
            }
            else if(Creature* tmp = me->SummonCreature(RingBoss[rand32()%6], 644.300f, -175.989f, -53.739f, 3.418f, TEMPSUMMON_DEAD_DESPAWN, 0))
            {
                RingBossGUID = tmp->GetGUID();
                tmp->GetMotionMaster()->MovePoint(0,605.928f,-187.492f,-54.02f);
            }
            MobDeath_Timer = 2500;
        }

        void WaypointReached(uint32 waypointId) override
        {
            switch (waypointId)
            {
                case 0:
                {
                    Talk(SAY_TEXT1);
                    DoCastSelf(SPELL_ASHCROMBES_TELEPORT_A, true);
                    CanWalk = false;
                    Event_Timer = 5000;
                    // Cheeremote for some Spectators
                    std::list<Creature*> ArenaSpectatorList;
                    me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_ARENA_SPECTATOR,     76.0f);
                    me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_SHADOWFORGE_PEASANT, 76.0f);
                    me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_SHADOWFORGE_CITIZEN, 76.0f);
                    me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_SHADOWFORGE_SENATOR, 76.0f);
                    if (!ArenaSpectatorList.empty())
                        for (std::list<Creature*>::iterator itr = ArenaSpectatorList.begin(); itr != ArenaSpectatorList.end(); itr++)
                            (*itr)->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                    break;
                }
                case 1:
                    Talk(SAY_TEXT2);
                    CanWalk = false;
                    Event_Timer = 5000;
                    break;
                case 2:
                    CanWalk = false;
                    break;
                case 3:
                    Talk(SAY_TEXT3);
                    break;
                case 4:
                    Talk(SAY_TEXT4);
                    CanWalk = false;
                    Event_Timer = 5000;
                    break;
                case 5:
                    instance->UpdateEncounterState(ENCOUNTER_CREDIT_KILL_CREATURE, NPC_GRIMSTONE, me);
                    instance->SetData(TYPE_RING_OF_LAW, DONE);
                    TC_LOG_DEBUG("scripts", "npc_grimstone: event reached end and set complete.");
                    break;
            }
        }

        void SetArenaSpoilsPhase(uint32 phase)
        {
            if(instance)
            {
                if(ObjectGuid chestGUID = ObjectGuid(instance->GetGuidData(DATA_GO_ARENA_SPOILS)))
                    GameObject::GetGameObject(*me,chestGUID)->SetPhaseMask(phase,true);
            }
        }

        void HandleGameObject(uint32 id, bool open)
        {
            instance->HandleGameObject(ObjectGuid(instance->GetGuidData(id)), open);
        }

        void UpdateAI(uint32 diff) override
        {
            if (MobDeath_Timer)
            {
                if (MobDeath_Timer <= diff)
                {
                    MobDeath_Timer = 2500;

                    if (RingBossGUID)
                    {
                        Creature* boss = ObjectAccessor::GetCreature(*me, RingBossGUID);
                        if (boss && !boss->IsAlive() && boss->isDead())
                        {
                            if(boss->GetEntry()==NPC_THELDREN)
                            {
                                for(uint8 i = 0; i < 4; ++i)
                                {
                                    Creature* add = ObjectAccessor::GetCreature(*me, RingTheldrenAddGUID[i]);
                                    if(!add || add->IsAlive())
                                        return;

                                }
                                SetArenaSpoilsPhase(1);
                                for (Player& player : boss->GetMap()->GetPlayers())
                                {
                                    if (boss->GetDistance2d(&player) < 50.0f)
                                    {
                                        player.KilledMonsterCredit(16166);
                                    }
                                }
                            }
                            RingBossGUID.Clear();
                            Event_Timer = 5000;
                            MobDeath_Timer = 0;
                            return;

                        }
                        return;
                    }

                    for (uint8 i = 0; i < MAX_NPC_AMOUNT; ++i)
                    {
                        Creature* mob = ObjectAccessor::GetCreature(*me, RingMobGUID[i]);
                        if (mob && !mob->IsAlive() && mob->isDead())
                        {
                            RingMobGUID[i].Clear();
                            --MobCount;

                            //seems all are gone, so set timer to continue and discontinue this
                            if (!MobCount)
                            {
                                Event_Timer = 5000;
                                MobDeath_Timer = 0;
                            }
                        }
                    }
                } else MobDeath_Timer -= diff;
            }

            if (Event_Timer)
            {
                if (Event_Timer <= diff)
                {
                    switch (EventPhase)
                    {
                        case 0:
                            Talk(SAY_TEXT5);
                            HandleGameObject(DATA_ARENA4, false);
                            Start(false, false);
                            CanWalk = true;
                            Event_Timer = 0;
                            break;
                        case 1:
                            CanWalk = true;
                            Event_Timer = 0;
                            break;
                        case 2:
                            Event_Timer = 2000;
                            break;
                        case 3:
                            HandleGameObject(DATA_ARENA1, true);
                            DoCastSelf(SPELL_ARENA_FLASH_A, true);
                            DoCastSelf(SPELL_ARENA_FLASH_B, true);
                            Event_Timer = 3000;
                            break;
                        case 4:
                            CanWalk = true;
                            me->SetVisible(false);
                            SummonRingMob();
                            Event_Timer = 8000;
                            break;
                        case 5:
                            SummonRingMob();
                            SummonRingMob();
                            Event_Timer = 8000;
                            break;
                        case 6:
                            SummonRingMob();
                            Event_Timer = 0;
                            break;
                        case 7:
                            me->SetVisible(true);
                            DoCastSelf(SPELL_ASHCROMBES_TELEPORT_A, true);
                            HandleGameObject(DATA_ARENA1, false);
                            Talk(SAY_TEXT6);
                            CanWalk = true;
                            Event_Timer = 0;
                            break;
                        case 8:
                            HandleGameObject(DATA_ARENA2, true);
                            DoCastSelf(SPELL_ARENA_FLASH_C, true);
                            DoCastSelf(SPELL_ARENA_FLASH_D, true);
                            Event_Timer = 5000;
                            break;
                        case 9:
                            me->SetVisible(false);
                            SummonRingBoss();
                            me->GetMotionMaster()->MovePoint(0, 596.578f, -188.769f, -54.155f, true);
                            Event_Timer = 0;
                            break;
                        case 10:
                        {
                            //if quest, complete
                            HandleGameObject(DATA_ARENA2, false);
                            HandleGameObject(DATA_ARENA3, true);
                            HandleGameObject(DATA_ARENA4, true);
                            std::list<Creature*> ArenaSpectatorList;
                            me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_ARENA_SPECTATOR,     76.0f);
                            me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_SHADOWFORGE_PEASANT, 76.0f);
                            me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_SHADOWFORGE_CITIZEN, 76.0f);
                            me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_SHADOWFORGE_SENATOR, 76.0f);
                            me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_ANVILRAGE_SOLDIER,   76.0f);
                            me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_ANVILRAGE_MEDIC,     76.0f);
                            me->GetCreatureListWithEntryInGrid(ArenaSpectatorList, NPC_ANVILRAGE_OFFICER,   76.0f);
                            if (!ArenaSpectatorList.empty())
                                for (std::list<Creature*>::iterator itr = ArenaSpectatorList.begin(); itr != ArenaSpectatorList.end(); itr++)
                                    (*itr)->setFaction(FACTION_ARENA_NEUTRAL);
                            CanWalk = true;
                            Event_Timer = 0;
                            break;
                        }
                    }
                    ++EventPhase;
                } else Event_Timer -= diff;
            }

            if (CanWalk)
                npc_escortAI::UpdateAI(diff);
           }
    };

};

// npc_phalanx
enum PhalanxSpells
{
    SPELL_THUNDERCLAP                   = 8732,
    SPELL_FIREBALLVOLLEY                = 22425,
    SPELL_MIGHTYBLOW                    = 14099
};

class npc_phalanx : public CreatureScript
{
public:
    npc_phalanx() : CreatureScript("npc_phalanx") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_phalanxAI(creature);
    }

    struct npc_phalanxAI : public ScriptedAI
    {
        npc_phalanxAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 ThunderClap_Timer;
        uint32 FireballVolley_Timer;
        uint32 MightyBlow_Timer;

        void Reset() override
        {
            ThunderClap_Timer = 12000;
            FireballVolley_Timer = 0;
            MightyBlow_Timer = 15000;
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            //ThunderClap_Timer
            if (ThunderClap_Timer <= diff)
            {
                DoCastVictim(SPELL_THUNDERCLAP);
                ThunderClap_Timer = 10000;
            } else ThunderClap_Timer -= diff;

            //FireballVolley_Timer
            if (HealthBelowPct(51))
            {
                if (FireballVolley_Timer <= diff)
                {
                    DoCastVictim(SPELL_FIREBALLVOLLEY);
                    FireballVolley_Timer = 15000;
                } else FireballVolley_Timer -= diff;
            }

            //MightyBlow_Timer
            if (MightyBlow_Timer <= diff)
            {
                DoCastVictim(SPELL_MIGHTYBLOW);
                MightyBlow_Timer = 10000;
            } else MightyBlow_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

// npc_kharan_mighthammer
enum KharamQuests
{
    QUEST_4001                          = 4001,
    QUEST_4342                          = 4342
};

#define GOSSIP_ITEM_KHARAN_1    "I need to know where the princess are, Kharan!"
#define GOSSIP_ITEM_KHARAN_2    "All is not lost, Kharan!"
#define GOSSIP_ITEM_KHARAN_3    "Gor'shak is my friend, you can trust me."
#define GOSSIP_ITEM_KHARAN_4    "Not enough, you need to tell me more."
#define GOSSIP_ITEM_KHARAN_5    "So what happened?"
#define GOSSIP_ITEM_KHARAN_6    "Continue..."
#define GOSSIP_ITEM_KHARAN_7    "So you suspect that someone on the inside was involved? That they were tipped off?"
#define GOSSIP_ITEM_KHARAN_8    "Continue with your story please."
#define GOSSIP_ITEM_KHARAN_9    "Indeed."
#define GOSSIP_ITEM_KHARAN_10   "The door is open, Kharan. You are a free man."

class npc_kharan_mighthammer : public CreatureScript
{
public:
    npc_kharan_mighthammer() : CreatureScript("npc_kharan_mighthammer") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
                player->SEND_GOSSIP_MENU(2475, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
                player->SEND_GOSSIP_MENU(2476, creature->GetGUID());
                break;

            case GOSSIP_ACTION_INFO_DEF+3:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
                player->SEND_GOSSIP_MENU(2477, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
                player->SEND_GOSSIP_MENU(2478, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+5:
                 player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_7, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
                player->SEND_GOSSIP_MENU(2479, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+6:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_8, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+7);
                player->SEND_GOSSIP_MENU(2480, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+7:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_9, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+8);
                player->SEND_GOSSIP_MENU(2481, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+8:
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_10, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+9);
                player->SEND_GOSSIP_MENU(2482, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+9:
                player->CLOSE_GOSSIP_MENU();
                if (player->GetTeam() == HORDE)
                    player->AreaExploredOrEventHappens(QUEST_4001);
                else
                    player->AreaExploredOrEventHappens(QUEST_4342);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(QUEST_4001) == QUEST_STATUS_INCOMPLETE)
             player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        if (player->GetQuestStatus(4342) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_KHARAN_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);

        if (player->GetTeam() == HORDE)
            player->SEND_GOSSIP_MENU(2473, creature->GetGUID());
        else
            player->SEND_GOSSIP_MENU(2474, creature->GetGUID());

        return true;
    }
};

// npc_lokhtos_darkbargainer
enum Lokhtos
{
    QUEST_A_BINDING_CONTRACT                      = 7604,
    ITEM_SULFURON_INGOT                           = 17203,
    ITEM_THRORIUM_BROTHERHOOD_CONTRACT            = 18628,
    SPELL_CREATE_THORIUM_BROTHERHOOD_CONTRACT_DND = 23059
};

#define GOSSIP_ITEM_SHOW_ACCESS     "Show me what I have access to, Lothos."
#define GOSSIP_ITEM_GET_CONTRACT    "Get Thorium Brotherhood Contract"

class npc_lokhtos_darkbargainer : public CreatureScript
{
public:
    npc_lokhtos_darkbargainer() : CreatureScript("npc_lokhtos_darkbargainer") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF + 1)
        {
            player->CLOSE_GOSSIP_MENU();
            player->CastSpell(player, SPELL_CREATE_THORIUM_BROTHERHOOD_CONTRACT_DND, false);
        }
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (creature->IsVendor() && player->GetReputationRank(59) >= REP_FRIENDLY)
              player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_ITEM_SHOW_ACCESS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        if (player->GetQuestRewardStatus(QUEST_A_BINDING_CONTRACT) != 1 &&
            !player->HasItemCount(ITEM_THRORIUM_BROTHERHOOD_CONTRACT, 1, true) &&
            player->HasItemCount(ITEM_SULFURON_INGOT))
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_GET_CONTRACT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        if (player->GetReputationRank(59) < REP_FRIENDLY)
            player->SEND_GOSSIP_MENU(3673, creature->GetGUID());
        else
            player->SEND_GOSSIP_MENU(3677, creature->GetGUID());

        return true;
    }
};

// npc_rocknot
enum Rocknot
{
    SAY_GOT_BEER       = 0,
    SAY_MORE_BEER      = 1,
    SPELL_DRUNKEN_RAGE = 14872,
    QUEST_ALE          = 4295,
    ACTION_EMOTE       = 1
};

class npc_rocknot : public CreatureScript
{
public:
    npc_rocknot() : CreatureScript("npc_rocknot") { }

    bool OnQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 /*item*/) override
    {
        InstanceScript* instance = creature->GetInstanceScript();
        if (!instance)
            return true;

        if (instance->GetData(TYPE_BAR) == DONE || instance->GetData(TYPE_BAR) == SPECIAL)
            return true;

        if (quest->GetQuestId() == QUEST_ALE)
        {
            if (instance->GetData(TYPE_BAR) != IN_PROGRESS)
                instance->SetData(TYPE_BAR, IN_PROGRESS);

            creature->SetFacingToObject(player);
            creature->AI()->DoAction(ACTION_EMOTE);

            instance->SetData(TYPE_BAR, SPECIAL);

            //keep track of amount in instance script, returns SPECIAL if amount ok and event in progress
            if (instance->GetData(TYPE_BAR) == SPECIAL)
            {
                creature->AI()->Talk(SAY_GOT_BEER);
                creature->CastSpell(creature, SPELL_DRUNKEN_RAGE, false);

                if (npc_escortAI* escortAI = CAST_AI(npc_rocknot::npc_rocknotAI, creature->AI()))
                    escortAI->Start(false, false);
            }
        }

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetInstanceAI<npc_rocknotAI>(creature);
    }

    struct npc_rocknotAI : public npc_escortAI
    {
        npc_rocknotAI(Creature* creature) : npc_escortAI(creature)
        {
            instance = creature->GetInstanceScript();
        }
       
        void Reset() override
        {
            if (HasEscortState(STATE_ESCORT_ESCORTING))
                return;

            BreakKeg_Timer  = 0;
            BreakDoor_Timer = 0;
            EmoteTimer      = 0;
        }

        void DoGo(uint32 id, uint32 state)
        {
            if (GameObject* go = instance->instance->GetGameObject(ObjectGuid(instance->GetGuidData(id))))
                go->SetGoState((GOState)state);
        }

        void WaypointReached(uint32 waypointId) override
        {
            switch (waypointId)
            {
                case 1:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_KICK);
                    break;
                case 2:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK_UNARMED);
                    break;
                case 3:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK_UNARMED);
                    break;
                case 4:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_KICK);
                    break;
                case 5:
                    me->HandleEmoteCommand(EMOTE_ONESHOT_KICK);
                    BreakKeg_Timer = 2 * IN_MILLISECONDS;
                    break;
            }
        }

         void DoAction(int32 action)
        {
            if (action == ACTION_EMOTE)
                EmoteTimer = 1.5 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff) override
        {
            if (BreakKeg_Timer)
            {
                if (BreakKeg_Timer <= diff)
                {
                    DoGo(DATA_GO_BAR_KEG, 0);
                    BreakKeg_Timer = 0;
                    BreakDoor_Timer = 1 * IN_MILLISECONDS;
                } else BreakKeg_Timer -= diff;
            }

            if (BreakDoor_Timer)
            {
                if (BreakDoor_Timer <= diff)
                {
                    DoGo(DATA_GO_BAR_DOOR, 2);
                    DoGo(DATA_GO_BAR_KEG_TRAP, 0);               //doesn't work very well, leaving code here for future
                    //spell by trap has effect61, this indicate the bar go hostile

                    if (Unit* tmp = ObjectAccessor::GetUnit(*me, ObjectGuid(instance->GetGuidData(DATA_PHALANX))))
                    {
                        tmp->setFaction(14);
                        tmp->ToCreature()->AI()->SetData(3, 3);
                    }                       

                    //for later, this event(s) has alot more to it.
                    //optionally, DONE can trigger bar to go hostile.
                    instance->SetData(TYPE_BAR, DONE);

                    BreakDoor_Timer = 0;
                } else BreakDoor_Timer -= diff;
            }

            // Several times Rocknot is supposed to perform an action (text, spell cast...) followed closely by an emote
            // we handle it here
            if (EmoteTimer)
            {
                if (EmoteTimer <= diff)
                {
                    Talk(SAY_MORE_BEER);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_CHEER);
                    EmoteTimer = 0;
                }
                else
                    EmoteTimer -= diff;
            }

            npc_escortAI::UpdateAI(diff);
        }
        private:
            InstanceScript* instance;
            uint32 BreakKeg_Timer;
            uint32 BreakDoor_Timer;
            uint32 EmoteTimer;
    };
};

/*######
## Brew Fest Evento.
######*/

enum CorenDirebrew
{
    SPELL_DISARM                = 47310,
    SPELL_DISARM_PRECAST        = 47407,
    SPELL_MOLE_MACHINE_EMERGE   = 50313,
    NPC_ILSA_DIREBREW           = 26764,
    NPC_URSULA_DIREBREW         = 26822,
    NPC_DIREBREW_MINION         = 26776,
    EQUIP_ID_TANKARD            = 48663,
    FACTION_HOSTILE_CORE        = 736,
    QUEST_INSULT_COREN_DIREBREW = 12062,
    SAY_INSULT                  = 0,
    GOSSIP_COREN                = 60084
};



static Position _pos[]=
{
    {890.87f, -133.95f, -48.0f, 1.53f},
    {892.47f, -133.26f, -48.0f, 2.16f},
    {893.54f, -131.81f, -48.0f, 2.75f}
};

class npc_coren_direbrew : public CreatureScript
{
    public:
        npc_coren_direbrew() : CreatureScript("npc_coren_direbrew") { }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
        {
            player->PlayerTalkClass->ClearMenus();
            if (uiAction == GOSSIP_ACTION_INFO_DEF)
            {
                   creature->AI()->Talk(SAY_INSULT, player);

                   creature->setFaction(FACTION_HOSTILE_CORE);
                   creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                   creature->AI()->AttackStart(player);
                   creature->AI()->DoZoneInCombat();
                   player->CLOSE_GOSSIP_MENU();
            }

            return true;
        }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetOptionTextWithEntry(GOSSIP_COREN, 0), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

            return true;
        }

        struct npc_coren_direbrewAI : public ScriptedAI
        {
            npc_coren_direbrewAI(Creature* creature) : ScriptedAI(creature), _summons(me)
            {
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void Reset()
            {
                me->RestoreFaction();
                me->SetCorpseDelay(90); // 1.5 minutes

                _addTimer = 20000;
                _disarmTimer = urand(10, 15) *IN_MILLISECONDS;

                _spawnedIlsa = false;
                _spawnedUrsula = false;
                _summons.DespawnAll();

                for (uint8 i = 0; i < 3; ++i)
                    if (Creature* creature = me->SummonCreature(NPC_DIREBREW_MINION, _pos[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000))
                    {
                        _add[i] = creature->GetGUID();
                        creature->setFaction(me->getFaction());
                        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    }
            }

            void EnterCombat(Unit* /*who*/)
            {
                SetEquipmentSlots(false, EQUIP_ID_TANKARD, EQUIP_ID_TANKARD, EQUIP_NO_CHANGE);

                for (uint8 i = 0; i < 3; ++i)
                {
                    if (_add[i])
                    {
                        Creature* creature = ObjectAccessor::GetCreature((*me), _add[i]);
                        if (creature && creature->IsAlive())
                        {
                            creature->setFaction(FACTION_HOSTILE_CORE);
                            creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            creature->SetInCombatWithZone();
                        }
                        _add[i].Clear();
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                // disarm
                if (_disarmTimer <= diff)
                {
                    DoCast(SPELL_DISARM_PRECAST);
                    DoCastVictim(SPELL_DISARM, false);
                    _disarmTimer = urand(20, 25) *IN_MILLISECONDS;
                }
                else
                    _disarmTimer -= diff;

                // spawn non-elite adds
                if (_addTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    {
                        float posX, posY, posZ;
                        target->GetPosition(posX, posY, posZ);
                        target->CastSpell(target, SPELL_MOLE_MACHINE_EMERGE, false, 0, 0, me->GetGUID());
                        me->SummonCreature(NPC_DIREBREW_MINION, posX, posY, posZ, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);

                        _addTimer = 15000;
                        if (_spawnedIlsa)
                            _addTimer -= 3000;
                        if (_spawnedUrsula)
                            _addTimer -= 3000;
                    }
                }
                else
                    _addTimer -= diff;

                if (!_spawnedIlsa && HealthBelowPct(66))
                {
                    DoSpawnCreature(NPC_ILSA_DIREBREW, 0, 0, 0, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
                    _spawnedIlsa = true;
                }

                if (!_spawnedUrsula && HealthBelowPct(33))
                {
                    DoSpawnCreature(NPC_URSULA_DIREBREW, 0, 0, 0, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
                    _spawnedUrsula = true;
                }

                DoMeleeAttackIfReady();
            }

            void JustSummoned(Creature* summon)
            {
                if (me->getFaction() == FACTION_HOSTILE_CORE)
                {
                    summon->setFaction(FACTION_HOSTILE_CORE);

                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        summon->AI()->AttackStart(target);
                }

                _summons.Summon(summon);
            }

            void JustDied(Unit* /*killer*/)
            {
                _summons.DespawnAll();

                // TODO: unhack
                Map* map = me->GetMap();
                if (map && map->IsDungeon())
                {
                    for (Player& player : map->GetPlayers())
                    {
                        if (player.IsGameMaster() || !player.GetGroup())
                            continue;

                        sLFGMgr->FinishDungeon(player.GetGroup()->GetGUID(), 287);
                        return;
                    }
                }
            }

        private:
            SummonList _summons;
            ObjectGuid _add[3];
            uint32 _addTimer;
            uint32 _disarmTimer;
            bool _spawnedIlsa;
            bool _spawnedUrsula;
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_coren_direbrewAI(c);
        }
};

/*######
## dark iron brewmaiden
######*/

enum Brewmaiden
{
    SPELL_SEND_FIRST_MUG = 47333,
    SPELL_SEND_SECOND_MUG = 47339,
    //SPELL_CREATE_BREW = 47345,
    SPELL_HAS_BREW_BUFF = 47376,
    //SPELL_HAS_BREW = 47331,
    //SPELL_DARK_BREWMAIDENS_STUN = 47340,
    SPELL_CONSUME_BREW = 47377,
    SPELL_BARRELED = 47442,
    SPELL_CHUCK_MUG = 50276
};

class npc_brewmaiden : public CreatureScript
{
    public:
        npc_brewmaiden() : CreatureScript("npc_brewmaiden") { }

        struct npc_brewmaidenAI : public ScriptedAI
        {
            npc_brewmaidenAI(Creature* c) : ScriptedAI(c)
            {
            }

            void Reset()
            {
                _brewTimer = 2000;
                _barrelTimer = 5000;
                _chuckMugTimer = 10000;
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->SetInCombatWithZone();
            }

            void AttackStart(Unit* who)
            {
                if (!who)
                    return;

                if (me->Attack(who, true))
                {
                    AddThreat(who, 1.0f);
                    me->SetInCombatWith(who);
                    who->SetInCombatWith(me);

                    if (me->GetEntry() == NPC_URSULA_DIREBREW)
                        me->GetMotionMaster()->MoveFollow(who, 10.0f, 0.0f);
                    else
                        me->GetMotionMaster()->MoveChase(who);
                }
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {
                // TODO: move to DB

                if (spell->Id == SPELL_SEND_FIRST_MUG)
                    target->CastSpell(target, SPELL_HAS_BREW_BUFF, true);

                if (spell->Id == SPELL_SEND_SECOND_MUG)
                    target->CastSpell(target, SPELL_CONSUME_BREW, true);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (_brewTimer <= diff)
                {
                    if (!me->IsNonMeleeSpellCast(false))
                    {
                        Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true);

                        if (target && me->GetDistance(target) > 5.0f)
                        {
                            DoCast(target, SPELL_SEND_FIRST_MUG, true);
                            _brewTimer = 12000;
                        }
                    }
                }
                else
                    _brewTimer -= diff;

                if (_chuckMugTimer <= diff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                        DoCast(target, SPELL_CHUCK_MUG, true);

                    _chuckMugTimer = 15000;
                }
                else
                    _chuckMugTimer -= diff;

                if (me->GetEntry() == NPC_URSULA_DIREBREW)
                {
                    if (_barrelTimer <= diff)
                    {
                        if (!me->IsNonMeleeSpellCast(false))
                        {
                            DoCastVictim(SPELL_BARRELED);
                            _barrelTimer = urand(15, 18) *IN_MILLISECONDS;
                        }
                    }
                    else
                        _barrelTimer -= diff;
                }
                else
                    DoMeleeAttackIfReady();
            }

        private:
            uint32 _brewTimer;
            uint32 _barrelTimer;
            uint32 _chuckMugTimer;
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_brewmaidenAI(c);
        }
};

/*######
## go_mole_machine_console
######*/

enum MoleMachineConsole
{
    SPELL_TELEPORT = 49466 // bad id?
};

#define GOSSIP_ITEM_MOLE_CONSOLE "[PH] Please teleport me."

class go_mole_machine_console : public GameObjectScript
{
    public:
        go_mole_machine_console() : GameObjectScript("go_mole_machine_console") { }

        bool OnGossipHello (Player* player, GameObject* go)
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_MOLE_CONSOLE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(12709, go->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, GameObject* /*go*/, uint32 /*sender*/, uint32 action)
        {
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
                player->CastSpell(player, SPELL_TELEPORT, true);

            return true;
        }
};

/*######
## Hurley Blackbreath
##
## npc_hurley_blackbreath
## go_thunderbrew_lager_keg
######*/

enum HurleyBlackbreath
{
    NPC_BLACKBREATH_CRONY = 9541,
    SPELL_FLAME_BREATH = 9573
};

static Position _blackbreathPos[]=
{
    {890.87f, -133.95f, -48.0f, 1.53f},
    {892.47f, -133.26f, -48.0f, 2.16f},
    {893.54f, -131.81f, -48.0f, 2.75f}
};

class npc_hurley_blackbreath : public CreatureScript
{
    public:
        npc_hurley_blackbreath() : CreatureScript("npc_hurley_blackbreath") { }
        
        struct npc_hurley_blackbreathAI : public ScriptedAI
        {
            npc_hurley_blackbreathAI(Creature* creature) : ScriptedAI(creature){}

            void Reset()
            {
                me->SetCorpseDelay(90); // 1.5 minutes
                _falmeBreathTimer = 10 * IN_MILLISECONDS;
                _enraged = false;

                for (uint8 i = 0; i <= 3; ++i)
                    if (Creature* creature = me->SummonCreature(NPC_BLACKBREATH_CRONY, _blackbreathPos[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000))
                    {
                        _add[i] = creature->GetGUID();
                    }
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->AI()->DoZoneInCombat();
                
                for (uint8 i = 0; i < 3; ++i)
                {
                    if (_add[i])
                    {
                        Creature* creature = ObjectAccessor::GetCreature((*me), _add[i]);
                        if (creature && creature->IsAlive())
                        {
                            creature->SetInCombatWithZone();
                        }
                        _add[i].Clear();
                    }
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (_falmeBreathTimer <= diff)
                {
                    DoCastVictim(SPELL_FLAME_BREATH, false);
                    _falmeBreathTimer =10 * IN_MILLISECONDS;
                }
                else
                    _falmeBreathTimer -= diff;
                
                if (HealthBelowPct(30) && !_enraged)
                {
                    DoCastSelf(SPELL_DRUNKEN_RAGE); //is defined in script of Racknot
                    _enraged = true;
                }

                DoMeleeAttackIfReady();
            }
            
        private:
            
            ObjectGuid _add[3];
            uint32 _falmeBreathTimer;
            bool _enraged;
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_hurley_blackbreathAI(c);
        }
};

class go_thunderbrew_lager_keg : public GameObjectScript
{
    public:
        go_thunderbrew_lager_keg() : GameObjectScript("go_thunderbrew_lager_keg") { }

        bool OnGossipHello(Player* /*player*/, GameObject* go)
        {
            go->GetInstanceScript()->SetData(DATA_THUNDERBREW_KEG, 0);
            go->SetGoState(GO_STATE_ACTIVE);
            go->SetLootState(GO_ACTIVATED);
            return true;
        }
};

enum QuestRibblyScrewspigot
{
    NPC_RIBBLYS_CRONY = 10043,
    SPELL_HAMSTRING   = 9080,
    SPELL_GOUGE       = 12540,
    TEXT_AGGRO        = 0
};

class npc_ribbly_screwspigot : public CreatureScript
{
    public:
        npc_ribbly_screwspigot() : CreatureScript("npc_ribbly_screwspigot") { }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*Sender*/, uint32 /*Action*/)
        {
            player->PlayerTalkClass->ClearMenus();
            creature->AI()->Talk(TEXT_AGGRO);
            creature->setFaction(16);
            creature->AI()->AttackStart(player);
            std::list<Creature*> cronys;
            creature->GetCreatureListWithEntryInGrid(cronys, NPC_RIBBLYS_CRONY, 50.0f);
            for (std::list<Creature*>::iterator itr = cronys.begin(); itr != cronys.end(); itr++)
            {
                (*itr)->setFaction(16);
                (*itr)->AI()->AttackStart(player);
            }
            player->CLOSE_GOSSIP_MENU();
            
            return true;
        }

        struct npc_ribbly_screwspigotAI : public ScriptedAI
        {
            npc_ribbly_screwspigotAI(Creature* creature) : ScriptedAI(creature){}

            uint32 HamstringTimer;
            uint32 GougeTimer;

            void Reset()
            {
                HamstringTimer = 1000;
                GougeTimer = 5000;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (HamstringTimer <= diff)
                {
                    DoCastVictim(SPELL_HAMSTRING);
                    HamstringTimer = 10000;
                }
                else
                {
                    HamstringTimer -= diff;
                }

                if (GougeTimer <= diff)
                {
                    DoCastVictim(SPELL_GOUGE);
                    GougeTimer = 20000;
                }
                else
                {
                    GougeTimer -= diff;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_ribbly_screwspigotAI(c);
        }
};

enum QuestHeartOfTheMountain
{
    NPC_WATCHMAN_DOOMGRIP = 9476,
    NPC_WARBRINGER_CONSTRUCT = 8905,
    GO_SECRET_DOOR = 174553,
};

uint32 RelicCofferDoors[] = {174554, 174555, 174556, 174557, 174558, 174559, 174560, 174561, 174562, 174563, 174564, 174566};

class npc_heart_of_the_mountain_trigger : public CreatureScript
{
    public:
        npc_heart_of_the_mountain_trigger() : CreatureScript("npc_heart_of_the_mountain_trigger") { }

        struct npc_heart_of_the_mountain_triggerAI : public ScriptedAI
        {
            npc_heart_of_the_mountain_triggerAI(Creature* creature) : ScriptedAI(creature){}

            bool AllDoorsOpen;
            uint32 checkTimer;

            void Reset()
            {
                AllDoorsOpen = false;
                checkTimer = 1000;
            }

            void UpdateAI(uint32 diff)
            {
                if (AllDoorsOpen)
                    return;

                if (checkTimer > diff)
                {
                    checkTimer -= diff;
                    return;
                }
                checkTimer = urand(5000, 7500);

                for(uint8 i = 0; i < 12; i++)
                {
                    if (GameObject* reliccofferdoor = me->FindNearestGameObject(RelicCofferDoors[i], 50.0f))
                    {
                        if (reliccofferdoor->GetGoState() != GO_STATE_ACTIVE)
                            return;
                    }
                }

                AllDoorsOpen = true;

                me->SummonCreature(NPC_WATCHMAN_DOOMGRIP, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                
                std::list<Creature*> constructs;
                me->GetCreatureListWithEntryInGrid(constructs, NPC_WARBRINGER_CONSTRUCT, 50.0f);
                for (std::list<Creature*>::iterator itr = constructs.begin(); itr != constructs.end(); itr++)
                {
                    (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE|UNIT_FLAG_NON_ATTACKABLE);
					(*itr)->SetImmuneToPC(false);
                }
                
                if (GameObject* secretdoor = me->FindNearestGameObject(GO_SECRET_DOOR, 50.0f))
                {
                    secretdoor->UseDoorOrButton();
                }
            }
        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_heart_of_the_mountain_triggerAI(c);
        }
};

enum QuestWhatIsGoingOn
{
    QUEST_WHAT_IS_GOING_ON  = 3982,
    NPC_ANVILRAGE_WARDEN    = 8890,
    NPC_ANVILRAGE_GUARDSMAN = 8891,
    ACTION_START_EVENT      = 1
};

Position WhatIsGoingOnSpawnPos[] = {{382.459f, -191.257f, -68.848f, 3.001f},
{381.098f, -194.544f, -69.419f, 2.459f},
{378.039f, -196.116f, -70.081f, 1.921f},
{374.3f, -195.719f, -70.874f, 1.269f}};

class npc_commander_gorshak : public CreatureScript
{
    public:
        npc_commander_gorshak() : CreatureScript("npc_commander_gorshak"){}

        bool OnQuestAccept(Player* /*player*/, Creature* creature, Quest const* quest)
        {
            if (quest->GetQuestId() == QUEST_WHAT_IS_GOING_ON)
            {
                creature->AI()->DoAction(ACTION_START_EVENT);
            }
            return true;
        }

        struct npc_commander_gorshakAI : public ScriptedAI
        {
            npc_commander_gorshakAI(Creature* creature) : ScriptedAI(creature), Summons(me)
            {
                EventRunning = false;
                NextWave = 0;
                WaveNumber = 1;
            }

            bool EventRunning;
            uint32 NextWave;
            uint8 WaveNumber;
            SummonList Summons;

            void DoAction(int32 action)
            {
                if (!EventRunning && action == ACTION_START_EVENT)
                {
                    EventRunning = true;
                    NextWave = 1000;
                    WaveNumber = 1;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (EventRunning)
                {
                    if (WaveNumber <= 2)
                    {
                        if ((NextWave <= diff))
                        {
                            for(uint8 i = 0; i < 4; i++)
                            {
                                if (WaveNumber == 1)
                                {
                                    me->SummonCreature(NPC_ANVILRAGE_WARDEN, WhatIsGoingOnSpawnPos[i]);
                                }
                                else
                                {
                                    me->SummonCreature(NPC_ANVILRAGE_GUARDSMAN, WhatIsGoingOnSpawnPos[i]);
                                }
                            }
                            WaveNumber++;
                            NextWave = 30000;
                        }
                        else
                        {
                            NextWave -= diff;
                        }
                    }
                    else
                    {
                        uint8 deadsummons = 0;
                        for (std::list<ObjectGuid>::const_iterator itr = Summons.begin(); itr != Summons.end(); ++itr)
                        {
                            if (Creature* creature = ObjectAccessor::GetCreature(*me, *itr))
                            {
                                if (creature->isDead())
                                {
                                    deadsummons++;
                                }
                            }
                        }

                        if (deadsummons == Summons.size())
                        {
                            std::list<Player*> players;
                            me->GetPlayerListInGrid(players, 50.f);
                            for (std::list<Player*>::const_iterator pitr = players.begin(); pitr != players.end(); ++pitr)
                            {
                                Player* player = *pitr;
                                if (player->GetQuestStatus(QUEST_WHAT_IS_GOING_ON) == QUEST_STATUS_INCOMPLETE)
                                {
                                    player->AreaExploredOrEventHappens(QUEST_WHAT_IS_GOING_ON);
                                }
                            }

                            EventRunning = false;
                        }
                    }
                }

                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }

            void JustSummoned(Creature* summon)
            {
                if (Player* player = summon->FindNearestPlayer(50.0f))
                    summon->AI()->AttackStart(player);
                
                Summons.Summon(summon);
            }

        };

        CreatureAI* GetAI(Creature* c) const
        {
            return new npc_commander_gorshakAI(c);
        }
};


/*######
## boss_theldren
######*/

enum eTheldren
{
    //First after 20 seconds, then once per minute
    SPELL_INTERCEPT         = 27577,

    //Once per minute
    SPELL_DISARM_THELDREN            = 27581,

    //Once per 30seconds under 35% life
    SPELL_HEAL_POTION       = 15503,

    //Once per 15 seconds
    SPELL_HAMSTRING2        = 27584,
    SPELL_MORTAL_STRIKE     = 27580,

    //Once per 30 seconds
    SPELL_DEMO_SHOUT        = 27579,
    SPELL_BATTLE_SHOUT      = 27578,
    SPELL_FRIGHT_SHOUT      = 19134
};

enum TheldrenEvents
{
    EVENT_INTERCEPT         = 0,
    EVENT_DISARM            = 1,
    EVENT_HEAL              = 2,
    EVENT_HAMSTRING         = 3,
    EVENT_SHOUT             = 4
};

class boss_theldren : public CreatureScript
{
    public:
        boss_theldren() : CreatureScript("boss_theldren"){ }

        CreatureAI* GetAI(Creature* Creature) const
        {
            return new boss_theldrenAI(Creature);
        }

        struct boss_theldrenAI : public ScriptedAI
        {
            boss_theldrenAI(Creature* Creature) : ScriptedAI (Creature)
            {
                Reset();
                pInstance = me->GetInstanceScript();
            }

            InstanceScript* pInstance;

            void Reset()
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_HEAL, 0*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_INTERCEPT, 20*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_DISARM, (60+urand(0,10))*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_HAMSTRING, (15+urand(0,3))*IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_SHOUT, (30+urand(0,5))*IN_MILLISECONDS);
            }

            void Intercept()
            {
                if (me->GetMap()->IsDungeon() && !me->GetMap()->GetPlayers().empty())
                {
                    for (Player& player : me->GetMap()->GetPlayers())
                    {
                        if (!player.IsGameMaster() && me->IsInRange(&player, 8.0f, 25.0f, false))
                        {
                            ResetThreatList();
                            AddThreat(&player, 5.0f);
                            DoCast(&player, SPELL_INTERCEPT);
                            break;
                        }
                     }
                 }
            }


            void UpdateAI(uint32 diff)
            {
                if(!UpdateVictim())
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_INTERCEPT:
                            Intercept();
                            _events.ScheduleEvent(EVENT_INTERCEPT, (45+urand(0,10))*IN_MILLISECONDS);
                            break;
                        case EVENT_DISARM:
                            me->CastSpell(me->GetVictim(),SPELL_DISARM_THELDREN,false);
                            _events.ScheduleEvent(EVENT_DISARM, (60+urand(0,10))*IN_MILLISECONDS);
                            break;
                        case EVENT_SHOUT:
                            {
                                switch(urand(0,2))
                                {
                                    case 0:
                                       me->CastSpell(SelectTarget(SELECT_TARGET_RANDOM,0,10,true),SPELL_DEMO_SHOUT,false);
                                       break;
                                    case 1:
                                       me->CastSpell(SelectTarget(SELECT_TARGET_RANDOM,0,10,true),SPELL_FRIGHT_SHOUT,false);
                                       break;
                                    case 2:
                                       me->CastSpell(me,SPELL_BATTLE_SHOUT,false);
                                       break;
                                 }
                            }
                            _events.ScheduleEvent(EVENT_SHOUT, (30+urand(0, 5))*IN_MILLISECONDS);
                            break;
                        case EVENT_HAMSTRING:
                            {
                                if(urand(0,1))
                                me->CastSpell(me->GetVictim(),SPELL_HAMSTRING2,false);
                                else
                                    me->CastSpell(me->GetVictim(),SPELL_MORTAL_STRIKE,false);
                            }
                            _events.ScheduleEvent(EVENT_HAMSTRING, (15+urand(0, 3))*IN_MILLISECONDS);
                            break;
                        case EVENT_HEAL:
                            {
                                if (me->GetHealthPct() <=35)
                                {
                                    me->CastSpell(me,SPELL_HEAL_POTION,true);
                                }
                            }
                            _events.ScheduleEvent(EVENT_HEAL, (30+urand(0, 5))*IN_MILLISECONDS);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            void EnterCombat(Unit* who)
            {
                Reset();
            }
        private:
            EventMap _events;

        };
};

/*######
## boss_golemlord_argelmach
######*/

enum GolemlordMisc
{
    // Events
    EVENT_CHAIN_LIGHTNING  = 1,
    EVENT_SHOCK            = 2,
    EVENT_CALL_HELP        = 3,

    // Spells
    SPELL_LIGHTNING_SHIELD = 15507,
    SPELL_CHAIN_LIGHTNING  = 15305,
    SPELL_SHOCK            = 15605
};

class boss_golemlord_argelmach : public CreatureScript
{
    public:
        boss_golemlord_argelmach() : CreatureScript("boss_golemlord_argelmach"){ }
   
        struct boss_golemlord_argelmachAI : public ScriptedAI
        {
            boss_golemlord_argelmachAI(Creature* Creature) : ScriptedAI(Creature) { }

            void Reset() override
            {
                _events.Reset();
            }

            void EnterCombat(Unit* /*who*/) override
            {
                DoCastSelf(SPELL_LIGHTNING_SHIELD);
                _events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 14 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_SHOCK,            6 * IN_MILLISECONDS);
                _events.ScheduleEvent(EVENT_CALL_HELP,        2 * IN_MILLISECONDS);
            }
            
            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_CHAIN_LIGHTNING:
                            DoCastVictim(SPELL_CHAIN_LIGHTNING);
                            _events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, 15 * IN_MILLISECONDS);
                            break;
                        case EVENT_SHOCK:
                            DoCastVictim(SPELL_SHOCK);
                            _events.ScheduleEvent(EVENT_SHOCK, 7 * IN_MILLISECONDS);
                            break;
                        case EVENT_CALL_HELP:
                        {
                            if (me->GetVictim())
                            {
                                std::list<Creature*> HelperList;
                                me->GetCreatureListWithEntryInGrid(HelperList, NPC_RAGEREAVER_GOLEM, 200.0f);
                                me->GetCreatureListWithEntryInGrid(HelperList, NPC_HAMMER_CONSTRUCT, 200.0f);
                                if (!HelperList.empty())
                                    for (std::list<Creature*>::iterator itr = HelperList.begin(); itr != HelperList.end(); itr++)
                                        (*itr)->AI()->AttackStart(me->GetVictim());
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }       
        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* Creature) const override
        {
            return new boss_golemlord_argelmachAI(Creature);
        }
};

/*######
## at_shadowforge_bridge
######*/

enum BridgeTexts
{
    SAY_GUARD_AGGRO = 0
};

static const float aGuardSpawnPositions[2][4] =
{
    {642.3660f, -274.5155f, -43.10918f, 0.4712389f},                // First guard spawn position
    {740.1137f, -283.3448f, -42.75082f, 2.8623400f}                 // Meeting point (middle of the bridge)
};

// Two NPCs spawn when AT-1786 is triggered
class AreaTrigger_at_shadowforge_bridge : public AreaTriggerScript
{
    public:
        AreaTrigger_at_shadowforge_bridge() : AreaTriggerScript("at_shadowforge_bridge") 
        {
            activated = false;
        }
        
        bool OnTrigger(Player* player, AreaTriggerEntry const* /*trigger*/) override
        {
            if (activated)
                return false;

            if (player->IsGameMaster() || !player->IsAlive())
                return false;

            Creature* pyromancer = player->FindNearestCreature(NPC_LOREGRAIN, 1000.0f);

            if (!pyromancer)
                return false;
            
            if (Creature* masterGuard = pyromancer->SummonCreature(NPC_ANVILRAGE_GUARDMAN, aGuardSpawnPositions[0][0], aGuardSpawnPositions[0][1], aGuardSpawnPositions[0][2], aGuardSpawnPositions[0][3], TEMPSUMMON_TIMED_DESPAWN, 3 * HOUR * MINUTE * IN_MILLISECONDS))
            {
                activated = true;
                masterGuard->SetWalk(false);
                masterGuard->GetMotionMaster()->MovePath(PATH_ANVILRAGE_GUARDMAN, false);
                masterGuard->AI()->Talk(SAY_GUARD_AGGRO);

                if (Creature* slaveGuard = pyromancer->SummonCreature(NPC_ANVILRAGE_GUARDMAN, aGuardSpawnPositions[0][0], aGuardSpawnPositions[0][1], aGuardSpawnPositions[0][2], aGuardSpawnPositions[0][3], TEMPSUMMON_TIMED_DESPAWN, 3 * HOUR * MINUTE * IN_MILLISECONDS))
                    slaveGuard->GetMotionMaster()->MoveFollow(masterGuard, 2.0f, 0);
            }
            return false;
        }
        private:
            bool activated;
};

void AddSC_blackrock_depths()
{
    new go_shadowforge_brazier();
    new at_ring_of_law();
    new npc_grimstone();
    new npc_phalanx();
    new npc_kharan_mighthammer();
    new npc_lokhtos_darkbargainer();
    new npc_rocknot();
    new npc_coren_direbrew();
    new npc_brewmaiden();
    new go_mole_machine_console();
    new npc_hurley_blackbreath();
    new go_thunderbrew_lager_keg();
    new npc_heart_of_the_mountain_trigger();
    new npc_ribbly_screwspigot();
    new npc_commander_gorshak();
    new boss_theldren();
    new boss_golemlord_argelmach();
    new AreaTrigger_at_shadowforge_bridge();
}
