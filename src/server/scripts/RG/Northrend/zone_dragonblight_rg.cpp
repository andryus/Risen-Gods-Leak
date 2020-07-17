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
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "CombatAI.h"
#include "GameObjectAI.h"
#include "Group.h"
#include "Vehicle.h"

/*#######
# Quest Rescue from Town Square
########*/
enum eEnumVillager
{
    QUEST_RESCUE_FROM_TOWN_SQUARE    = 12253,
    NPC_VILLAGER                     = 27359
};

#define GOSSIP_ITEM_HELP  "Rettet mich bitte!"

class npc_trapped_wintergarde_villager : public CreatureScript
{
public:
    npc_trapped_wintergarde_villager() : CreatureScript("npc_trapped_wintergarde_villager") {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(QUEST_RESCUE_FROM_TOWN_SQUARE) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_HELP, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*uiSender*/, uint32 uiAction)
    {
        player->PlayerTalkClass->ClearMenus();
        if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
        {
            player->CLOSE_GOSSIP_MENU();
            player->ToPlayer()->KilledMonsterCredit(NPC_VILLAGER);
            creature->DespawnOrUnsummon();
        }
        return true;
    }
};

/*#####
# npc_Hourglass_of_Eternity
######*/
enum HourglassOfEternityDatas
{
    NPC_INFINITE_ASSAILANT    = 27896,
    NPC_INFINITE_CHRONO_MAGUS = 27898,
    NPC_INFINITE_TIMERENDER   = 27900,
    NPC_FUTURE_YOU            = 27899,
    SPELL_CLONE_PLAYER        = 57507,
    QUEST_MYSTERY             = 12470,
    FACTION_NEUTRAL_H_A       = 250,

    // Texts
    SAY_GREETINGS_0           = 0, 
    SAY_GREETINGS_1           = 1,
    SAY_GREETINGS_2           = 2,
    SAY_GREETINGS_3           = 3,
    SAY_GREETINGS_4           = 4,
    SAY_GREETINGS_5           = 5,
    SAY_GREETINGS_6           = 6,
    SAY_GREETINGS_7           = 7,
};

class npc_hourglass_of_eternity : public CreatureScript
{
public:
    npc_hourglass_of_eternity() : CreatureScript("npc_hourglass_of_eternity") { }
    
    struct npc_hourglass_of_eternityAI : public ScriptedAI
    {
        npc_hourglass_of_eternityAI(Creature* creature) : ScriptedAI(creature) { }
        
        void Reset() override
        {
            resetTimer = 1000;
            step = 0;
            castTimer = 0;
            start = false;
            playerGUID.Clear();
            futureYGUID.Clear();
            renderGUID.Clear();
        }

        void JustDied(Unit* /*killer*/) override
        {
            SetGroupQuestStatus(false);
            me->DisappearAndDie();
        }

        void SetGroupQuestStatus(bool complete)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, playerGUID);
            if (!player)
                return;

            if (Group* group = player->GetGroup())
            {
                for (GroupReference* groupRef = group->GetFirstMember(); groupRef != NULL; groupRef = groupRef->next())
                    if (Player* member = groupRef->GetSource())
                        if (member->GetQuestStatus(QUEST_MYSTERY) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (complete)
                                member->CompleteQuest(QUEST_MYSTERY);
                            else
                                member->FailQuest(QUEST_MYSTERY);
                        }
            }
            else
            {
                if (player->GetQuestStatus(QUEST_MYSTERY) == QUEST_STATUS_INCOMPLETE)
                {
                    if (complete)
                        player->CompleteQuest(QUEST_MYSTERY);
                    else
                        player->FailQuest(QUEST_MYSTERY);
                }
            }
        }

        bool isPlayerInRange()
        {
            return me->FindNearestPlayer(60.0f);
        }

        void UpdateAI(uint32 diff) override
        {
            if (resetTimer <= diff)
            {
                if (!isPlayerInRange())
                {
                    if (Creature* future = ObjectAccessor::GetCreature(*me, futureYGUID))
                        future->DespawnOrUnsummon();
                    me->DisappearAndDie();
                }
                resetTimer = 1 * IN_MILLISECONDS;
            }
            else
                resetTimer -= diff;

            if (castTimer <= diff)
            {
                if (!start)
                {
                    std::list<Player*> players;
                    me->GetPlayerListInGrid(players, 10.f);
                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        Player* player = *itr;
                        if (player->GetQuestStatus(QUEST_MYSTERY) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (Creature* future = me->SummonCreature(NPC_FUTURE_YOU,player->GetPositionX()+2,player->GetPositionY(),player->GetPositionZ()+2,player->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 200000))
                            {
                                futureYGUID = future->GetGUID();
                                playerGUID = player->GetGUID();
                                player->CastSpell(future, SPELL_CLONE_PLAYER, false);
                                future->setFaction(FACTION_NEUTRAL_H_A);
                                me->setFaction(FACTION_NEUTRAL_H_A);
                                future->SetLevel(80);
                                if (Player* player = me->FindNearestPlayer(100.0f))
                                    future->AI()->Talk(SAY_GREETINGS_0, player);
                                start = true;
                                castTimer = 2 * IN_MILLISECONDS;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    Creature* future = ObjectAccessor::GetCreature(*me, futureYGUID);
                    if (!future || !future->IsAlive())
                    {
                        SetGroupQuestStatus(false);
                        return;
                    }
                    switch (step)
                    {
                        case 0:
                            if (Player* player = me->FindNearestPlayer(100.0f))
                                future->AI()->Talk(SAY_GREETINGS_1, player);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT,me->GetPositionX(),me->GetPositionY()+5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            castTimer = 10 * IN_MILLISECONDS;
                            break;
                        case 1:
                            if (Player* player = me->FindNearestPlayer(100.0f))
                                future->AI()->Talk(SAY_GREETINGS_2, player);
                            castTimer = 15 * IN_MILLISECONDS;
                            break;
                        case 2:
                            if (Player* player = me->FindNearestPlayer(100.0f))
                                future->AI()->Talk(SAY_GREETINGS_3);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT,me->GetPositionX(),me->GetPositionY()+5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            castTimer = 10 * IN_MILLISECONDS;
                            break;
                        case 3:
                            if (Player* player = me->FindNearestPlayer(100.0f))
                                future->AI()->Talk(SAY_GREETINGS_4);
                            castTimer = 15 * IN_MILLISECONDS;
                            break;
                        case 4:
                            castTimer = 1 * IN_MILLISECONDS;
                        case 5:
                            if (Player* player = me->FindNearestPlayer(100.0f))
                                future->AI()->Talk(SAY_GREETINGS_5);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT,me->GetPositionX(),me->GetPositionY()+5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_CHRONO_MAGUS,me->GetPositionX()-5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_CHRONO_MAGUS,me->GetPositionX(),me->GetPositionY()-5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            castTimer = 25 * IN_MILLISECONDS;
                            break;
                        case 6:
                            if (Player* player = me->FindNearestPlayer(100.0f))
                                future->AI()->Talk(SAY_GREETINGS_6, player);
                            if (Creature* render = me->SummonCreature(NPC_INFINITE_TIMERENDER,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ()+5,0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000))
                            {
                                render->setFaction(14);
                                renderGUID = render->GetGUID();
                            }
                            castTimer = 5 * IN_MILLISECONDS;
                            break;
                        case 7:
                            if (Creature* render = ObjectAccessor::GetCreature(*me, renderGUID))
                            {
                                if (render->isDead())
                                {
                                    if (Player* player = me->FindNearestPlayer(100.0f))
                                        future->AI()->Talk(SAY_GREETINGS_7, player);
                                    SetGroupQuestStatus(true);
                                    break;
                                }
                                else
                                {
                                    castTimer = 3 * IN_MILLISECONDS;
                                    return;
                                }
                            }
                        case 8:
                            future->DisappearAndDie();
                            me->DisappearAndDie();
                            break;
                    }
                    ++step;
                }
            }
            else 
                castTimer -= diff;
        }
        private:
            uint8 step;
            uint32 castTimer;
            ObjectGuid renderGUID;
            ObjectGuid futureYGUID;
            ObjectGuid playerGUID;
            bool start;
            uint32 resetTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_hourglass_of_eternityAI(creature);
    }
};

/*#####
# npc_future_you
######*/

class npc_future_you : public CreatureScript
{
public:
    npc_future_you() : CreatureScript("npc_future_you") { }
   
    struct npc_future_youAI : public ScriptedAI
    {
        npc_future_youAI(Creature* creature) : ScriptedAI(creature) { }


        void InitializeAI() override
        {
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 40.f);
            for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* player = *itr;
                if (player->GetQuestStatus(QUEST_MYSTERY) != QUEST_STATUS_INCOMPLETE)
                {
                    player->CastSpell(me, SPELL_CLONE_PLAYER, false);
                    break;
                }
            }
        }

        void UpdateAI(uint32 /*diff*/)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_future_youAI(creature);
    }
};

/*#####
# spell_skytalon_molts
######*/

enum SkytalonMolts
{
    NPC_ALYSTROS = 27249
};

class spell_skytalon_molts : public SpellScriptLoader
{
    public:
        spell_skytalon_molts() : SpellScriptLoader("spell_skytalon_molts") { }

        class spell_skytalon_molts_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_skytalon_molts_SpellScript);

            void HandleSendEvent(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Creature* alystros = caster->FindNearestCreature(NPC_ALYSTROS, 500.0f))
                    {
                        alystros->SetImmuneToPC(false);
                        alystros->AI()->AttackStart(caster);
                    }
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_skytalon_molts_SpellScript::HandleSendEvent, EFFECT_0, SPELL_EFFECT_SEND_EVENT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_skytalon_molts_SpellScript();
        }
};

/*######
## npc_rokhan
######*/

#define ROKHAN_GOSSIP_ITEM1 "Bitte ruft den Drachen, damit ich ihm das Herz entreissen kann!"

enum eRokhan
{
    QUEST_SARATHSTRA_COURGE_OF_THE_NORTH = 12097,
    QUEST_TO_DRAGONS_FALL = 12095,
    NPC_ROKHAN = 26859,
    CREATURE_SARATHSTRA = 26858,
};

class npc_rokhan : public CreatureScript
{
    public:
        npc_rokhan() : CreatureScript("npc_rokhan") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (player->GetQuestStatus(QUEST_SARATHSTRA_COURGE_OF_THE_NORTH) == QUEST_STATUS_INCOMPLETE && !player->FindNearestCreature(CREATURE_SARATHSTRA, 200.0f, true))
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ROKHAN_GOSSIP_ITEM1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*uiSender*/, uint32 /*uiAction*/)
        {
            player->CLOSE_GOSSIP_MENU();

            if (Creature* tmp = player->FindNearestCreature(CREATURE_SARATHSTRA, 200.0f, false))
                tmp->DespawnOrUnsummon();

            if (Creature* tmpsarath = player->SummonCreature(CREATURE_SARATHSTRA, 4393.188f, 904.801f, 114.622f, 2.28f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000))
            {
                tmpsarath->SetCanFly(true);
                tmpsarath->SetHomePosition(4362.881f, 946.37f, 87.94f, tmpsarath->GetOrientation());
                tmpsarath->GetMotionMaster()->MoveTargetedHome();
            }

            return true;
        }
};

/*######
## npc_7th_legion_siege_engineer
######*/

enum LegionEnginerMisc
{
    NPC_ENGENEER                        = 27163,
    NPC_SCOURGE_PLAGE_CATAPULT          = 27607,
    SPELL_PLACE_SCOURGE_DISCOMBOBULATER = 49114, // Summons Object for 15 secounds
    SPELL_DESTURCTION                   = 49215,
    SPELL_DESTURCTION_TRIGGER           = 49218,
    NPC_PLAGUE_PULT_CREDIT              = 27625,
    SAY_ACTION                          = 0,
    SAY_DESTRUCTION                     = 1
};

class npc_7th_legion_siege_engineer : public CreatureScript
{
    public:
        npc_7th_legion_siege_engineer() : CreatureScript("npc_7th_legion_siege_engineer") { }
   
        struct npc_7th_legion_siege_engineerAI : public ScriptedAI
        {
            npc_7th_legion_siege_engineerAI(Creature* creature) : ScriptedAI (creature) { }
           
            void Reset() 
            {
                guid_owner.Clear();
                guid_pult.Clear();
                phase       = 0; //0 - Check for PlageCatapult, 1 - MoveToPult, 2 - Summon Object, 3 - Give Credit
                check_Timer = 2 * IN_MILLISECONDS;
            }

            void SpellHitTarget(Unit* target, SpellInfo const* spell)
            {   
                if (Player* owner = ObjectAccessor::GetPlayer(*me, guid_owner))
                    if (spell->Id == SPELL_DESTURCTION_TRIGGER)
                        owner->KilledMonsterCredit(NPC_SCOURGE_PLAGE_CATAPULT);
                if (Creature* catapult = me->FindNearestCreature(NPC_SCOURGE_PLAGE_CATAPULT, 10.0f))
                    catapult->KillSelf();
                Talk(SAY_DESTRUCTION);
            }

            void MoveInLineOfSight(Unit* who)
            {
                if (guid_owner)
                    return;

                if (me->GetDistance2d(who) > 20)
                    return;

                if (who->GetTypeId() == TYPEID_UNIT)
                {
                    if (who->IsVehicle() && who->GetCharmer() && who->GetCharmer()->ToPlayer())
                        guid_owner = who->GetCharmer()->ToPlayer()->GetGUID();
                }

                if (who->GetTypeId() == TYPEID_PLAYER)
                    guid_owner = who->ToPlayer()->GetGUID();
            }

            void AttackStart(Unit *attacker) { }

            void MovementInform(uint32 type, uint32 id)
            {
                if (type != POINT_MOTION_TYPE)
                    return;

                if (id != 1)
                    return;

                phase = 2;
                check_Timer = 2 * IN_MILLISECONDS;
            }

            void UpdateAI(uint32 diff)
            {
                if (check_Timer < diff)
                {
                    switch (phase)
                    {
                        case 0:
                            if (ObjectAccessor::GetPlayer(*me, guid_owner))
                            {
                                Creature* pult = me->FindNearestCreature(NPC_SCOURGE_PLAGE_CATAPULT, 50);
                                if (pult)
                                {
                                    guid_pult = pult->GetGUID();
                                    Talk(SAY_ACTION);
                                    me->GetMotionMaster()->MovePoint(1, pult->GetPositionX(), pult->GetPositionY(), pult->GetPositionZ());
                                    phase = 1;
                                }
                            }
                            else
                            {
                                me->KillSelf();
                            }
                            break;
                        case 1:
                            return;
                        case 2:
                            if (Creature* pult = ObjectAccessor::GetCreature(*me, guid_pult))
                            {
                                DoCastSelf(SPELL_DESTURCTION, true);
                                DoCastSelf(SPELL_PLACE_SCOURGE_DISCOMBOBULATER, false);
                                pult->SetInCombatWithZone();
                                phase++;
                            }
                            break;
                    }
                }
                else
                {
                    check_Timer -= diff;
                }
            }
        private:
            ObjectGuid guid_owner;
            ObjectGuid guid_pult;
            uint32 check_Timer;
            uint32 phase;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_7th_legion_siege_engineerAI(creature);
    }
};

/*######
## vehicle_alliance_steamtank
######*/

#define AREA_CARRION_FIELDS         4188
#define AREA_WINTERGARD_MAUSOLEUM   4246
#define AREA_THORSONS_POINT         4190

enum SteamtankMisc
{
   EVENT_AREA_CONTROL       = 1, 
   EVENT_CHECK_HAS_QUEST    = 2,
   EVENT_SOLDAT_TALK        = 3,
   QUEST_STEAMTANK_SURPRISE = 12326,
   NPC_LEGION_SOLDAT        = 27588,
   SAY_RANDOM               = 1,
};

class vehicle_alliance_steamtank : public CreatureScript
{
public:
    vehicle_alliance_steamtank() : CreatureScript("vehicle_alliance_steamtank") { }
    
    struct vehicle_alliance_steamtankAI : public VehicleAI
    {
        vehicle_alliance_steamtankAI(Creature* creature) : VehicleAI(creature) { }

        EventMap events;
        bool isInUse;

        void Reset()
        {
            events.Reset();
        }

        void PassengerBoarded(Unit* unit, int8 /*seat*/, bool apply)
        {
            if (apply)
            {
                if (unit->GetTypeId() == TYPEID_PLAYER)
                {
                    isInUse = true;
                    events.ScheduleEvent(EVENT_CHECK_HAS_QUEST, 0);
                    events.ScheduleEvent(EVENT_SOLDAT_TALK, 15 * IN_MILLISECONDS);
                    events.ScheduleEvent(EVENT_AREA_CONTROL, 1 * IN_MILLISECONDS);
                }
            }
            else
            {
                if (unit->GetTypeId() == TYPEID_PLAYER)
                {
                    isInUse = false;
                    Reset();
                    me->setActive(true);
                    me->DespawnOrUnsummon(10 * IN_MILLISECONDS);
                    me->SetRespawnTime(10);
                    me->SetCorpseDelay(0);
                }
            }
        }

        void CheckQuest()
        {
            if(Player* player = me->ToUnit()->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                if (!player->IsActiveQuest(QUEST_STEAMTANK_SURPRISE))
                    me->GetVehicleKit()->RemoveAllPassengers();
                else
                    events.ScheduleEvent(EVENT_CHECK_HAS_QUEST, 5 * IN_MILLISECONDS);
            }
        }

        void CheckArea()
        {
            uint32 area = me->GetAreaId();
            switch (area)
            {
                case AREA_CARRION_FIELDS:
                case AREA_WINTERGARD_MAUSOLEUM:
                case AREA_THORSONS_POINT:
                    break;
                default: 
                    me->DealDamage(me,me->GetHealth());
                    me->GetVehicleKit()->RemoveAllPassengers();
                    me->SetRespawnTime(10);
                    me->SetCorpseDelay(0);
                    break;
            }
        }

        void UpdateAI(uint32 diff) 
        {
            events.Update(diff);

            if(!me->IsVehicle())
                return;
            
            if(isInUse)
            {                  
                while (uint32 eventID = events.ExecuteEvent())
                {
                    switch (eventID) 
                    {
                        case EVENT_AREA_CONTROL:
                            CheckArea();
                            events.ScheduleEvent(EVENT_AREA_CONTROL, 10 * IN_MILLISECONDS);
                            break;
                        case EVENT_CHECK_HAS_QUEST:
                            CheckQuest();
                            break;
                        case EVENT_SOLDAT_TALK:
                            if (Creature* soldat = me->FindNearestCreature(NPC_LEGION_SOLDAT, 20.0f))
                                soldat->AI()->Talk(SAY_RANDOM);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new  vehicle_alliance_steamtankAI(creature);
    }
};

/*######
## npc_injured_7th_legion_soldier
######*/

enum InjuredSoldierWaypoints
{
    WAYPOINT_01             = 27792,
    WAYPOINT_02             = 27793,
    WAYPOINT_03             = 27794
};

enum InjuredSoldierSpells
{
    SPELL_FEAR              = 49553,
    SPELL_GUN_COMMANDER     = 49584
};

Position const InjuredSoldierEndPos = {3671.24f, -1192.43f, 102.34f};

class npc_injured_7th_legion_soldier : public CreatureScript
{
public:
    npc_injured_7th_legion_soldier() : CreatureScript("npc_injured_7th_legion_soldier") { }

    struct npc_injured_7th_legion_soldierAI : public ScriptedAI
    {
        npc_injured_7th_legion_soldierAI(Creature* c) : ScriptedAI(c)
        {
            nextWaypoint = true;
            curWaypoint = 1;
            walkTimer = 1000;
        }
        
        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            DoCast(SPELL_FEAR);
        }
        
        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (id >= 1 && id <= 3)
            {
                ++curWaypoint;
                walkTimer = 1000;
                nextWaypoint = true;
            }
            else if (id == 4)
            {
                std::list<Player*> players;
                me->GetPlayerListInGrid(players, 40.f);
                for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    if(Player* player = *itr)
                        if (player->HasAura(SPELL_GUN_COMMANDER))
                            player->KilledMonsterCredit(me->GetEntry());
                }

                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(uint32 diff)
        {
            if (!nextWaypoint)
                return;

            if (walkTimer <= diff)
            {
                switch (curWaypoint)
                {
                    case 1:
                        if (Unit* trigger = me->FindNearestCreature(WAYPOINT_01, 100.0f))
                            me->GetMotionMaster()->MovePoint(curWaypoint, trigger->GetPositionX(), trigger->GetPositionY(), trigger->GetPositionZ());
                        break;
                    case 2:
                        if (Unit* trigger = me->FindNearestCreature(WAYPOINT_02, 100.0f))
                            me->GetMotionMaster()->MovePoint(curWaypoint, trigger->GetPositionX(), trigger->GetPositionY(), trigger->GetPositionZ());
                        break;
                    case 3:
                        if (Unit* trigger = me->FindNearestCreature(WAYPOINT_03, 100.0f))
                            me->GetMotionMaster()->MovePoint(curWaypoint, trigger->GetPositionX(), trigger->GetPositionY(), trigger->GetPositionZ());
                        break;
                    case 4:
                        me->GetMotionMaster()->MovePoint(curWaypoint, InjuredSoldierEndPos);
                        break;
                }

                nextWaypoint = false;
            }
            else
            {
                walkTimer -= diff;
                return;
            }
        }

        private:
            bool nextWaypoint;
            uint64 curWaypoint;
            uint32 walkTimer;
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_injured_7th_legion_soldierAI (creature);
    }
};

/*######
## npc_mindless_ghoul
######*/

uint32 const NPC_7th_LEGION_RIFLEMAN = 27791;

class npc_mindless_ghoul : public CreatureScript
{
public:
    npc_mindless_ghoul() : CreatureScript("npc_mindless_ghoul") { }

    struct npc_mindless_ghoulAI : public ScriptedAI
    {
        npc_mindless_ghoulAI(Creature* c) : ScriptedAI(c) {}

        void Reset()
        {
            me->SetCorpseDelay(1);
        }

        void AttackStart(Unit* target)
        {
            if (target->GetEntry() == NPC_7th_LEGION_RIFLEMAN)
                return;
            ScriptedAI::AttackStart(target);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mindless_ghoulAI (creature);
    }
};

/*######
## spell_oriks_song
######*/

enum OriksSong
{
    GO_ORIKS_CRYSTALLINE_ORB    = 189305
};

Position const OrbSpawnPosition = {3100.139893f, -1240.829956f, 10.50f, 0.0f};

class spell_q12301_oriks_song : public SpellScriptLoader // Oriks Song
{
public:
    spell_q12301_oriks_song() : SpellScriptLoader("spell_q12301_oriks_song") { }

    class spell_q12301_oriks_song_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_q12301_oriks_song_SpellScript);

        void HandleOnCast()
        {
            if (Unit* caster = GetCaster())
                if (!caster->FindNearestGameObject(GO_ORIKS_CRYSTALLINE_ORB, 10.0f))
                    caster->SummonGameObject(GO_ORIKS_CRYSTALLINE_ORB, OrbSpawnPosition.GetPositionX(), OrbSpawnPosition.GetPositionY(), OrbSpawnPosition.GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
        }

        void Register()
        {
            OnCast += SpellCastFn(spell_q12301_oriks_song_SpellScript::HandleOnCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_q12301_oriks_song_SpellScript();
    }
};

/*######
## go_oriks_crystalline_orb
######*/

enum NpcForgottenTroops
{
    NPC_VALONFORTH              = 27476, // Captain Luc Valonforth
    NPC_EMISSARY                = 27492, // Alliance Emissary
    NPC_ARTHAS                  = 27455, // Prince Arthas
    NPC_MURADIN                 = 27480, // Muradin

    NPC_CAPTAIN                 = 27220, // Forgotten Captain (Gryphon)
    NPC_KNIGHT                  = 27224, // Forgotten Knight (Horse)
    NPC_FOOTMAN                 = 27229, // Forgotten Footman
    NPC_RIFLEMAN                = 27225  // Forgotten Rifleman
};

enum Mounts
{
    MOUNT_GRYPHON               = 24447, // Gryphon
    MOUNT_HORSE                 = 2410   // Horse
};

enum Visitors
{
    VIEWER_CAPTAIN1                   = 0,
    VIEWER_CAPTAIN2,
    VIEWER_KNIGHT1,
    VIEWER_KNIGHT2,
    VIEWER_FOOTMAN1,
    VIEWER_FOOTMAN2,
    VIEWER_FOOTMAN3,
    VIEWER_RIFLEMAN1,
    VIEWER_RIFLEMAN2
};

Position const ValonforthPos =  {3097.29f, -1250.75f, 11.0157f, 1.63972f};
Position const EmissaryPos =    {3090.71f, -1206.27f, 12.3084f, 4.86142f};
Position const ArthasPos =      {3124.43f, -1223.47f, 16.7660f, 4.08304f};
Position const MuradinPos =     {3127.29f, -1223.91f, 17.2939f, 3.87731f};

Position const VisitorPos[] =
{
    {3106.43f, -1224.32f, 13.6212f, 4.39804f}, // Captains
    {3084.64f, -1232.05f, 12.2744f, 5.32951f},
    {3100.69f, -1237.35f, 10.2998f, 4.49857f}, // Knights
    {3091.70f, -1237.19f, 10.2485f, 4.77581f},
    {3105.79f, -1238.36f, 10.8660f, 4.07210f}, // Footmen
    {3100.60f, -1234.08f, 10.1739f, 4.40197f},
    {3088.87f, -1239.15f, 10.4241f, 5.30909f},
    {3102.11f, -1239.47f, 10.5343f, 4.14121f}, // Riflemen
    {3088.71f, -1243.22f, 10.3330f, 6.23429f}
};

Position const TroopPos[] =
{
    {3087.68f, -1256.36f, 10.5736f, 0.353227f},
    {3088.88f, -1253.18f, 10.6643f, 0.345373f},
    {3086.23f, -1252.06f, 10.2609f, 0.239344f},
    {3087.88f, -1249.29f, 10.3733f, 0.227564f},
    {3084.92f, -1247.77f, 10.1669f, 0.204002f},
    {3086.63f, -1245.35f, 10.2619f, 0.113681f},
    {3084.22f, -1243.47f, 10.2609f, 0.058702f},
    {3086.05f, -1241.40f, 10.3671f, 0.090119f},
    {3083.01f, -1239.58f, 10.7396f, 6.251570f},
    {3084.91f, -1237.60f, 10.8927f, 0.090118f}
};

Position const EmissaryWalk =           {3096.57f, -1246.36f, 10.7306f, 4.91717f};
Position const TroopWalk =              {3048.47f, -1244.38f, 11.2454f, 2.91991f};
Position const ArthasWalk =             {3100.76f, -1246.12f, 10.9998f, 4.09482f};
Position const MuradinWalk =            {3102.42f, -1247.08f, 11.2486f, 3.76814f};

float const ValonforthLeftTurn =        3.041656f;
float const ValonforthRightTurn =       0.862176f;

enum YellsValonforth
{
    SAY_VALONFORTH_0        = 0, // 12719 - Valonforth: I apologize, emissary, ...
    SAY_VALONFORTH_1        = 1, // 12720 - Valonforth: We're to just ...
    SAY_VALONFORTH_2        = 2, // 12721 - Valonforth: To hell with the undead! ...
    SAY_VALONFORTH_3        = 3  // 12722 - Valonforth: Well, milord, your father ...
};

enum YellsEmissary
{
    SAY_EMISSARY_0          = 0, // 12723 - Emissary: By royal edict, ...
    SAY_EMISSARY_1          = 1  // 12724 - Emissary: That's correct. ...
};

enum YellsArthas
{
    SAY_ARTHAS_0            = 0, // 12725 - Arthas: Captain, why are the guards ...
    SAY_ARTHAS_1            = 1, // 12726 - Arthas: Uther had my troops recalled? ...
    SAY_ARTHAS_2            = 2, // 12727 - Arthas: Burned down to their frames! ...
    SAY_ARTHAS_3            = 3  // 12728 - Arthas: Spare me, Muradin. ...
};

enum YellsMuradin
{
    SAY_MURADIN_0           = 0, // 12733 - Muradin: Isn't that a bit much, lad?
    SAY_MURADIN_1           = 1  // 12734 - Muradin: You lied to your men ...
};

enum YellsCaptain
{
    SAY_CAPTAIN_0           = 0, // What's this now? ...
    SAY_CAPTAIN_1           = 1  // Look alive, lads! ...
};

enum YellsFootman
{
    SAY_FOOTMAN_0           = 0  // But why? - It was all lies... - etc.
};

enum Enums
{
    MODEL_GHOST                 = 5430,
    QUEST_TRUTH                 = 12301,
    SPELL_QUEST_TRUTH_CREDIT    = 48882
};

class go_oriks_crystalline_orb : public GameObjectScript
{
public:
    go_oriks_crystalline_orb() : GameObjectScript("go_oriks_crystalline_orb") { }

    struct go_oriks_crystalline_orbAI : public GameObjectAI
    {
        go_oriks_crystalline_orbAI(GameObject* go) : GameObjectAI(go)
        {
            go->setActive(true);

            eventStarted = false;
            eventStep = 0;
            eventTimer = 0;
            playerTimer = 5000;

            valonforthGUID.Clear();
            emissaryGUID.Clear();
            arthasGUID.Clear();
            muradinGUID.Clear();
            for (uint8 i = 0; i < 9; ++i)
                if (viewerGUID[i])
                    viewerGUID[i].Clear();
            for (uint8 i = 0; i < 10; ++i)
                if (troopGUID[i])
                    troopGUID[i].Clear();
        }

        void SpawnTroops()
        {
            // Valonforth
            if (Creature* valonforth = go->SummonCreature(NPC_VALONFORTH, ValonforthPos, TEMPSUMMON_MANUAL_DESPAWN))
                valonforthGUID = valonforth->GetGUID();

            // Viewer
            for (uint8 i = 0; i < 9; ++i)
            {
                Creature* viewer;

                switch (i)
                {
                    case VIEWER_CAPTAIN1:
                    case VIEWER_CAPTAIN2:
                        if ((viewer = go->SummonCreature(NPC_CAPTAIN, VisitorPos[i], TEMPSUMMON_MANUAL_DESPAWN)))
                        {
                            viewerGUID[i] = viewer->GetGUID();
                            viewer->Mount(MOUNT_GRYPHON);
                        }
                        break;
                    case VIEWER_KNIGHT1:
                    case VIEWER_KNIGHT2:
                        if ((viewer = go->SummonCreature(NPC_KNIGHT, VisitorPos[i], TEMPSUMMON_MANUAL_DESPAWN)))
                        {
                            viewerGUID[i] = viewer->GetGUID();
                            viewer->Mount(MOUNT_HORSE);
                        }
                        break;
                    case VIEWER_FOOTMAN1:
                    case VIEWER_FOOTMAN2:
                    case VIEWER_FOOTMAN3:
                        if ((viewer = go->SummonCreature(NPC_FOOTMAN, VisitorPos[i], TEMPSUMMON_MANUAL_DESPAWN)))
                            viewerGUID[i] = viewer->GetGUID();
                        break;
                    case VIEWER_RIFLEMAN1:
                    case VIEWER_RIFLEMAN2:
                        if ((viewer = go->SummonCreature(NPC_RIFLEMAN, VisitorPos[i], TEMPSUMMON_MANUAL_DESPAWN)))
                            viewerGUID[i] = viewer->GetGUID();
                        break;
                }

                if (viewer)
                    viewer->setFaction(35); // Neutral
            }

            // Troop
            for (uint8 i = 0; i < 10; ++i)
                if (Creature* troop = go->SummonCreature(NPC_FOOTMAN, TroopPos[i], TEMPSUMMON_MANUAL_DESPAWN))
                {
                    troopGUID[i] = troop->GetGUID();
                    troop->setFaction(35); // Neutral
                }
        }

        void AbortEvent()
        {
            if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                valonforth->DespawnOrUnsummon();
            if (Creature* emissary = ObjectAccessor::GetCreature(*go, emissaryGUID))
                emissary->DespawnOrUnsummon();
            if (Creature* arthas = ObjectAccessor::GetCreature(*go, arthasGUID))
                arthas->DespawnOrUnsummon();
            if (Creature* muradin = ObjectAccessor::GetCreature(*go, muradinGUID))
                muradin->DespawnOrUnsummon();
            for (uint8 i = 0; i < 9; ++i)
                if (Creature* viewer = ObjectAccessor::GetCreature(*go, viewerGUID[i]))
                    viewer->DespawnOrUnsummon();
            for (uint8 i = 0; i < 10; ++i)
                if (Creature* troop = ObjectAccessor::GetCreature(*go, troopGUID[i]))
                    troop->DespawnOrUnsummon();
            go->SetLootState(GO_JUST_DEACTIVATED);
        }

        void UpdateAI(uint32 diff)
        {
            if (!eventStarted)
            {
                eventStarted = true;
                eventStep = 0;
                eventTimer = 0;
            }

            // Playercheck
            if (playerTimer <= diff)
            {
                Unit* player = go->FindNearestPlayer(20.0f);
                if (!player)
                {
                    AbortEvent();
                    return;
                }
                playerTimer = 5000;
            }
            else
                playerTimer -= diff;

            // Eventcheck
            if (eventTimer <= diff)
                ++eventStep;
            else
            {
                eventTimer -= diff;
                return; // Next step not yet reached
            }

            // Next Eventstep
            switch (eventStep)
            {
                case 1:     // Spawn troops, Emissary walking, First Captain: What's this now? ...
                    SpawnTroops();
                    if (Creature* emissary = go->SummonCreature(NPC_EMISSARY, EmissaryPos, TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        emissaryGUID = emissary->GetGUID();
                        emissary->SetWalk(true);
                        emissary->GetMotionMaster()->MovePoint(0, EmissaryWalk);
                    }
                    if (Creature* viewer = ObjectAccessor::GetCreature(*go, viewerGUID[VIEWER_CAPTAIN1]))
                        viewer->AI()->Talk(SAY_CAPTAIN_0);
                    eventTimer = 5000;
                    break;
                case 2:     // Second Captain: Look alive, lads! ...
                    if (Creature* viewer = ObjectAccessor::GetCreature(*go, viewerGUID[VIEWER_CAPTAIN2]))
                        viewer->AI()->Talk(SAY_CAPTAIN_1);
                    eventTimer = 11000;
                    break;
                case 3:     // Valonforth: I apologize, emissary, ...
                    if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                        valonforth->AI()->Talk(SAY_VALONFORTH_0);
                    eventTimer = 8000;
                    break;
                case 4:     // Emissary: By royal edict, ...
                    if (Creature* emissary = ObjectAccessor::GetCreature(*go, emissaryGUID))
                        emissary->AI()->Talk(SAY_EMISSARY_0);
                    eventTimer = 9000;
                    break;
                case 5:     // Valonforth: We're to just ...
                    if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                        valonforth->AI()->Talk(SAY_VALONFORTH_1);
                    eventTimer = 4000;
                    break;
                case 6:     // Emissary: That's correct. ...
                    if (Creature* emissary = ObjectAccessor::GetCreature(*go, emissaryGUID))
                        emissary->AI()->Talk(SAY_EMISSARY_1);
                    eventTimer = 9000;
                    break;
                case 7:     // Valonforth turns to lumber troop
                    if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                        valonforth->SetFacingTo(ValonforthLeftTurn);
                    eventTimer = 1000;
                    break;
                case 8:     // Valonforth: To hell with the undead! ...
                    if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                        valonforth->AI()->Talk(SAY_VALONFORTH_2);
                    eventTimer = 5000;
                    break;
                case 9:     // Lumber troop salutes, emissary exit, Enter Arthas and Muradin, Valonforth turns to Arthas
                    for (uint8 i = 0; i < 10; ++i)
                        if (Creature* troop = ObjectAccessor::GetCreature(*go, troopGUID[i]))
                        {
                            troop->SetSheath(SHEATH_STATE_UNARMED);
                            troop->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                        }
                    if (Creature* emissary = ObjectAccessor::GetCreature(*go, emissaryGUID))
                        emissary->GetMotionMaster()->MovePoint(0, EmissaryPos);
                    if (Creature* arthas = go->SummonCreature(NPC_ARTHAS, ArthasPos, TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        arthasGUID = arthas->GetGUID();
                        arthas->SetWalk(false);
                        arthas->GetMotionMaster()->MovePoint(0, ArthasWalk);
                    }
                    if (Creature* muradin = go->SummonCreature(NPC_MURADIN, MuradinPos, TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        muradinGUID = muradin->GetGUID();
                        muradin->SetWalk(false);
                        muradin->GetMotionMaster()->MovePoint(0, MuradinWalk);
                    }
                    if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                        valonforth->SetFacingTo(ValonforthRightTurn);
                    eventTimer = 1000;
                    break;
                case 10:    // Lumber troop exit
                    for (uint8 i = 0; i < 10; ++i)
                        if (Creature* troop = ObjectAccessor::GetCreature(*go, troopGUID[i]))
                        {
                            troop->SetWalk(false);
                            troop->GetMotionMaster()->MovePoint(0, TroopWalk.GetPositionX() + frand(-10, 10), TroopWalk.GetPositionY() + frand(-10, 10), TroopWalk.GetPositionZ());
                        }
                    eventTimer = 6000;
                    break;
                case 11:    // Lumber troop and emissary desappears, Arthas (arrives): Captain, why are the guards ...
                    if (Creature* arthas = ObjectAccessor::GetCreature(*go, arthasGUID))
                        arthas->AI()->Talk(SAY_ARTHAS_0);
                    for (uint8 i = 0; i < 10; ++i)
                        if (Creature* troop = ObjectAccessor::GetCreature(*go, troopGUID[i]))
                            troop->DespawnOrUnsummon();
                    if (Creature* emissary = ObjectAccessor::GetCreature(*go, emissaryGUID))
                        emissary->DespawnOrUnsummon();
                    eventTimer = 6000;
                    break;
                    break;
                case 12:    // Valonforth: Well, milord, your father ...
                    if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                    {
                        valonforth->AI()->Talk(SAY_VALONFORTH_3);
                    }
                    eventTimer = 5500;
                    break;
                case 13:    // Arthas: Uther had my troops recalled? ...
                    if (Creature* arthas = ObjectAccessor::GetCreature(*go, arthasGUID))
                        arthas->AI()->Talk(SAY_ARTHAS_1);
                    eventTimer = 12000;
                    break;
                case 14:    // Muradin: Isn't that a bit much, lad?
                    if (Creature* muradin = ObjectAccessor::GetCreature(*go, muradinGUID))
                        muradin->AI()->Talk(SAY_MURADIN_0);
                    eventTimer = 4000;
                    break;
                case 15:    // Arthas: Burned down to their frames! ...
                    if (Creature* arthas = ObjectAccessor::GetCreature(*go, arthasGUID))
                        arthas->AI()->Talk(SAY_ARTHAS_2);
                    eventTimer = 6000;
                    break;
                case 16:    // Muradin: You lied to your men ...
                    if (Creature* muradin = ObjectAccessor::GetCreature(*go, muradinGUID))
                        muradin->AI()->Talk(SAY_MURADIN_1);
                    eventTimer = 10000;
                    break;
                case 17:    // Arthas: Spare me, Muradin. ...
                    if (Creature* arthas = ObjectAccessor::GetCreature(*go, arthasGUID))
                        arthas->AI()->Talk(SAY_ARTHAS_3);
                    eventTimer = 6000;
                    break;
                case 18:    // Arthas cast on troops (Quest Complete)
                    if (Creature* arthas = ObjectAccessor::GetCreature(*go, arthasGUID))
                        arthas->CastSpell(arthas, SPELL_QUEST_TRUTH_CREDIT, true);
                    eventTimer = 2000;
                    break;
                case 19:    // Valonforth disappears, Footman: You gave us your word. betrayer!
                    for (uint8 i = VIEWER_FOOTMAN1; i <= VIEWER_FOOTMAN3; ++i)
                        if (Creature* viewer = ObjectAccessor::GetCreature(*go, viewerGUID[i]))
                            viewer->AI()->Talk(SAY_FOOTMAN_0);
                    if (Creature* valonforth = ObjectAccessor::GetCreature(*go, valonforthGUID))
                        valonforth->DespawnOrUnsummon();
                    eventTimer = 3000;
                    break;
                case 20:    // Arthas and Muradin disappear, viewer turn undead
                    if (Creature* arthas = ObjectAccessor::GetCreature(*go, arthasGUID))
                        arthas->DespawnOrUnsummon();
                    if (Creature* muradin = ObjectAccessor::GetCreature(*go, muradinGUID))
                        muradin->DespawnOrUnsummon();
                    for (uint8 i = 0; i < 9; ++i)
                        if (Creature* viewer = ObjectAccessor::GetCreature(*go, viewerGUID[i]))
                        {
                            if (viewer->IsMounted())
                                viewer->Dismount();
                            viewer->SetDisplayId(MODEL_GHOST);
                        }
                    eventTimer = 1000;
                    break;
                case 21:    // Viewer die
                    for (uint8 i = 0; i < 9; ++i)
                        if (Creature* viewer = ObjectAccessor::GetCreature(*go, viewerGUID[i]))
                            viewer->KillSelf();
                    eventTimer = 1000;
                    break;
                case 22:    // Viewer disappear, orb disappear
                    for (uint8 i = 0; i < 9; ++i)
                        if (Creature* viewer = ObjectAccessor::GetCreature(*go, viewerGUID[i]))
                            viewer->DespawnOrUnsummon();
                    go->SetLootState(GO_JUST_DEACTIVATED);
                    eventTimer = 1000;
                    break;
            }
        }

    private:
        // Creatures
        ObjectGuid valonforthGUID;
        ObjectGuid emissaryGUID;
        ObjectGuid arthasGUID;
        ObjectGuid muradinGUID;
        ObjectGuid viewerGUID[9];
        ObjectGuid troopGUID[10];

        // Event
        bool eventStarted;
        uint32 eventStep;
        uint32 eventTimer;
        uint32 playerTimer;
    };

    GameObjectAI* GetAI(GameObject* go) const
    {
        return new go_oriks_crystalline_orbAI(go);
    }
};

/*#####
# npc_hourglass_of_eternity_two // Mystery of the Infinite, Redux
######*/
enum HourglassOfEternityTwoDatas
{
    NPC_INFINITE_ASSAILANT_TWO   = 27896,
    NPC_INFINITE_DESTROYER       = 27897,
    NPC_INFINITE_TIMERENDER_TWO  = 27900,
    NPC_PAST_YOU                 = 32331,
    SPELL_CLONE                  = 57507,
    QUEST_MYSTERY_TWO            = 13343,    
    TEXT_0                       = 0,
    TEXT_1                       = 1,
    TEXT_2                       = 2,
    TEXT_3                       = 3,
    TEXT_4                       = 4,
    TEXT_5                       = 5,
    TEXT_6                       = 6,
    TEXT_7                       = 7
};

class npc_hourglass_of_eternity_two : public CreatureScript
{
public:
    npc_hourglass_of_eternity_two() : CreatureScript("npc_hourglass_of_eternity_two") { }
    
    struct npc_hourglass_of_eternity_twoAI : public ScriptedAI
    {
        npc_hourglass_of_eternity_twoAI(Creature* creature) : ScriptedAI(creature) { }
        
        void Reset() override
        {
            resetTimer = 1000;
            step = 0;
            castTimer = 0;
            start = false;
            playerGUID.Clear();
            pastYGUID.Clear();
            renderGUID.Clear();
        }

        void JustDied(Unit* /*killer*/) override
        {
            SetGroupQuestStatus(false);
            me->DisappearAndDie();
        }

        void SetGroupQuestStatus(bool complete)
        {
            Player* player = ObjectAccessor::GetPlayer(*me, playerGUID);
            if (!player)
                return;

            if (Group* group = player->GetGroup())
            {
                for (GroupReference* groupRef = group->GetFirstMember(); groupRef != NULL; groupRef = groupRef->next())
                    if (Player* member = groupRef->GetSource())
                        if (member->GetQuestStatus(QUEST_MYSTERY_TWO) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (complete)
                                member->CompleteQuest(QUEST_MYSTERY_TWO);
                            else
                                member->FailQuest(QUEST_MYSTERY_TWO);
                        }
            }
            else
            {
                if (player->GetQuestStatus(QUEST_MYSTERY_TWO) == QUEST_STATUS_INCOMPLETE)
                {
                    if (complete)
                        player->CompleteQuest(QUEST_MYSTERY_TWO);
                    else
                        player->FailQuest(QUEST_MYSTERY_TWO);
                }
            }
        }

        bool isPlayerInRange()
        {
            return me->FindNearestPlayer(60.0f);
        }

        void UpdateAI(uint32 diff) override
        {
            if (resetTimer <= diff)
            {
                if (!isPlayerInRange())
                {
                    if (Creature* past = ObjectAccessor::GetCreature(*me, pastYGUID))
                        past->DespawnOrUnsummon();
                    me->DisappearAndDie();
                }
                resetTimer = 1 * IN_MILLISECONDS;
            }
            else
                resetTimer -= diff;

            if (castTimer <= diff)
            {
                if (!start)
                {
                    std::list<Player*> players;
                    me->GetPlayerListInGrid(players, 10.f);
                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        Player* player = *itr;
                        if (player->GetQuestStatus(QUEST_MYSTERY_TWO) == QUEST_STATUS_INCOMPLETE)
                        {
                            if (Creature* past = me->SummonCreature(NPC_PAST_YOU,player->GetPositionX()+2,player->GetPositionY(),player->GetPositionZ()+2,player->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 200000))
                            {
                                pastYGUID = past->GetGUID();
                                playerGUID = player->GetGUID();
                                player->CastSpell(past, SPELL_CLONE, false);
                                past->setFaction(FACTION_NEUTRAL_H_A);
                                me->setFaction(FACTION_NEUTRAL_H_A);

                                past->AI()->Talk(TEXT_0);
                                start = true;
                                castTimer = 2 * IN_MILLISECONDS;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    Creature* past = ObjectAccessor::GetCreature(*me, pastYGUID);
                    if (!past || !past->IsAlive())
                    {
                        SetGroupQuestStatus(false);
                        return;
                    }
                    switch (step)
                    {
                        case 0:
                            past->AI()->Talk(TEXT_1);
                            castTimer = 5 * IN_MILLISECONDS;
                            break;
                        case 1:
                            past->AI()->Talk(TEXT_2);
                            me->SummonCreature(NPC_INFINITE_DESTROYER,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX(),me->GetPositionY()+5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            castTimer = 33 * IN_MILLISECONDS;
                            break;
                        case 2:
                            past->AI()->Talk(TEXT_3);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX(),me->GetPositionY()+5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            castTimer = 30 * IN_MILLISECONDS;
                            break;
                        case 3:
                            past->AI()->Talk(TEXT_4);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX(),me->GetPositionY()+5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_DESTROYER,me->GetPositionX()-5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            castTimer = 30 * IN_MILLISECONDS;
                            break;
                        case 5:
                            past->AI()->Talk(TEXT_5);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX(),me->GetPositionY()+5,me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            me->SummonCreature(NPC_INFINITE_ASSAILANT_TWO,me->GetPositionX()-5,me->GetPositionY(),me->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000);
                            castTimer = 32 * IN_MILLISECONDS;
                            break;
                        case 6:
                            past->AI()->Talk(TEXT_6);
                            if (Creature* render = me->SummonCreature(NPC_INFINITE_TIMERENDER_TWO,me->GetPositionX()+5,me->GetPositionY(),me->GetPositionZ()+5,0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,100000))
                            {
                                render->setFaction(14);
                                renderGUID = render->GetGUID();
                            }
                            castTimer = 42 * IN_MILLISECONDS;
                            break;
                        case 7:
                            if (Creature* render = ObjectAccessor::GetCreature(*me, renderGUID))
                            {
                                if (render->isDead())
                                {
                                    past->AI()->Talk(TEXT_7);
                                    SetGroupQuestStatus(true);
                                    break;
                                }
                                else
                                {
                                    castTimer = 3 * IN_MILLISECONDS;
                                    return;
                                }
                            }
                        case 8:
                            past->DisappearAndDie();
                            me->DisappearAndDie();
                            break;
                    }
                    ++step;
                }
            }
            else 
                castTimer -= diff;
        }
        private:
            uint8 step;
            uint32 castTimer;
            ObjectGuid renderGUID;
            ObjectGuid pastYGUID;
            ObjectGuid playerGUID;
            bool start;
            uint32 resetTimer;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_hourglass_of_eternity_twoAI(creature);
    }
};

/*#####
# npc_past_you
######*/

class npc_past_you : public CreatureScript
{
public:
    npc_past_you() : CreatureScript("npc_past_you") { }
    
    struct npc_past_youAI : public ScriptedAI
    {
        npc_past_youAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            std::list<Player*> players;
            me->GetPlayerListInGrid(players, 40.f);
            for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* player = *itr;
                if (player->GetQuestStatus(QUEST_MYSTERY_TWO) != QUEST_STATUS_INCOMPLETE)
                {
                    player->CastSpell(me, SPELL_CLONE, false);
                    break;
                }
            }
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_past_youAI(creature);
    }
};

enum WarhoreseMisc
{
    NPC_ONSLAUGHT_KNIGHT = 27206
};

class npc_onslaught_warhorse : public CreatureScript
{
public:
    npc_onslaught_warhorse() : CreatureScript("npc_onslaught_warhorse") { }

    struct npc_onslaught_warhorseAI : public ScriptedAI
    {
        npc_onslaught_warhorseAI(Creature* creature) : ScriptedAI(creature) { }

        void InitializeAI() override
        {
            me->SummonCreature(NPC_ONSLAUGHT_KNIGHT, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
        }

        void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
        {
            if (apply && passenger->GetTypeId() == TYPEID_PLAYER)
            {
                me->SetReactState(REACT_PASSIVE);
                me->SetImmuneToNPC(true);
                me->DespawnOrUnsummon(240 * IN_MILLISECONDS);
            }
        }

        void JustRespawned() override
        {
            me->SummonCreature(NPC_ONSLAUGHT_KNIGHT, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void UpdateAI(uint32 /*diff*/) override
        { 
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_onslaught_warhorseAI(creature);
    }
};

class npc_forsaken_blight_spreader : public CreatureScript
{
    public:
        npc_forsaken_blight_spreader() : CreatureScript("npc_forsaken_blight_spreader") { }

        struct npc_forsaken_blight_spreaderAI : public VehicleAI
        {
            npc_forsaken_blight_spreaderAI(Creature* creature) : VehicleAI(creature) { }

            void PassengerBoarded(Unit* /*passenger*/, int8 /*seatId*/, bool apply) override
            {
                if (!apply)
                {
                    me->setActive(true);
                    me->DespawnOrUnsummon(10 * IN_MILLISECONDS);
                    me->SetRespawnTime(10);
                    me->SetCorpseDelay(0);
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_forsaken_blight_spreaderAI(creature);
        }
};

enum WintergardeGryphonMisc
{
    // Creatures
    NPC_WINTERGARDE_GRYPHON_RIDER = 27662,

    // Quests
    QUEST_INTO_HOSTILE_TERRITORY  = 12325,

    // Paths
    PATH_INTO_HOSTILE_TERRITORY   = 2766100,

    // Texts
    SAY_START                     = 0,
    SAY_ENDING                    = 1
};

class npc_wintergarde_gryphon : public CreatureScript
{
    public:
        npc_wintergarde_gryphon() : CreatureScript("npc_wintergarde_gryphon") { }

        struct npc_wintergarde_gryphonAI : public VehicleAI
        {
            npc_wintergarde_gryphonAI(Creature* creature) : VehicleAI(creature) { }

            void PassengerBoarded(Unit* passenger, int8 seatId, bool apply) override
            {
                if (apply && passenger->GetTypeId() == TYPEID_PLAYER)
                {
                    /// @workaround - Because accessory gets unmounted when using vehicle_template_accessory.
                    /// When vehicle spawns accessory is mounted to seat 0,but when player mounts
                    /// he uses the same seat (instead of mounting to seat 1) kicking the accessory out.
                    passenger->ChangeSeat(1, false);
                    me->GetVehicleKit()->InstallAccessory(NPC_WINTERGARDE_GRYPHON_RIDER, 0, true, TEMPSUMMON_DEAD_DESPAWN, 0);
                    if (passenger->ToPlayer()->GetQuestStatus(QUEST_INTO_HOSTILE_TERRITORY) == QUEST_STATUS_COMPLETE)
                        me->GetMotionMaster()->MovePath(PATH_INTO_HOSTILE_TERRITORY, false);
                }
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type != WAYPOINT_MOTION_TYPE)
                    return;

                if (Creature* rider = me->FindNearestCreature(NPC_WINTERGARDE_GRYPHON_RIDER, 10.0f))
                    switch (id)
                    {
                        case 1:
                            rider->AI()->Talk(SAY_START);
                            break;
                        case 13:
                            rider->AI()->Talk(SAY_ENDING);
                            if (Vehicle* vehicle = me->GetVehicleKit())
                                vehicle->RemoveAllPassengers();
                            me->DespawnOrUnsummon(2 * IN_MILLISECONDS);
                            break;
                        case 14:
                            me->DespawnOrUnsummon();
                            break;
                        default:
                            break;
                     }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_wintergarde_gryphonAI(creature);
        }
};

enum TaunkaTransformMisc
{
    MODEL_TAUNKA_EQUIP_FEMALE  = 23841,
    MODEL_TAUNKA_EQUIP_MALE    = 23840
};

class spell_taunka_transform : public SpellScriptLoader
{
    public:
        spell_taunka_transform() : SpellScriptLoader("spell_taunka_transform") { }

        class spell_taunka_transform_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_taunka_transform_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                GetCaster()->SetDisplayId(GetHitUnit()->getGender() == GENDER_FEMALE ? MODEL_TAUNKA_EQUIP_FEMALE : MODEL_TAUNKA_EQUIP_MALE);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_taunka_transform_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_taunka_transform_SpellScript();
        }
};

enum IceCannonMisc
{
    // Spells
    SPELL_ICE_PRISON = 49333
};

class spell_dragonblight_ice_cannon : public SpellScriptLoader
{
    public:
        spell_dragonblight_ice_cannon() : SpellScriptLoader("spell_dragonblight_ice_cannon") { }
    
        class spell_dragonblight_ice_cannon_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_dragonblight_ice_cannon_SpellScript);
    
            void RecalculateDamage()
            {
                if (Aura* aura = GetHitUnit()->GetAura(SPELL_ICE_PRISON))
                {
                    int32 dmg = GetHitDamage();
                    AddPct(dmg, 300);
                    SetHitDamage(dmg);
                }
            }
    
            void Register() override
            {
                OnHit += SpellHitFn(spell_dragonblight_ice_cannon_SpellScript::RecalculateDamage);
            }
        };
    
        SpellScript* GetSpellScript() const override
        {
            return new spell_dragonblight_ice_cannon_SpellScript();
        }
};

enum QuestNoMercyForTheCaptured
{
    FACTION_THE_HAND_OF_VENGEANCE = 1928,

    EVENT_TALK                    = 1,

    SAY_COMBAT                    = 0,
    SAY_HELLO                     = 1,
};

class npc_no_mercy_for_the_captured : public CreatureScript
{
public:
    npc_no_mercy_for_the_captured() : CreatureScript("npc_no_mercy_for_the_captured") { }

    struct npc_no_mercy_for_the_capturedAI : public ScriptedAI
    {
        npc_no_mercy_for_the_capturedAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->NearTeleportTo(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY(), me->GetHomePosition().GetPositionZ(), me->GetHomePosition().GetOrientation());
            me->setFaction(FACTION_THE_HAND_OF_VENGEANCE);
            _events.RescheduleEvent(EVENT_TALK, 10 * IN_MILLISECONDS);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_TALK:
                        Talk(SAY_HELLO);
                        _events.ScheduleEvent(EVENT_TALK, urand(180, 300) * IN_MILLISECONDS);
                        break;
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }

        private:
            EventMap _events;
    };

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 /*action*/) override
    {
        if (player && creature)
        {
            player->PlayerTalkClass->ClearMenus();
            player->CLOSE_GOSSIP_MENU();
            creature->setFaction(FACTION_HOSTILE);
            creature->AI()->Talk(SAY_COMBAT);
            creature->GetAI()->AttackStart(player);
        }
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_no_mercy_for_the_capturedAI(creature);
    }
};

void AddSC_dragonblight_rg()
{
    new npc_trapped_wintergarde_villager();
    new npc_hourglass_of_eternity();
    new npc_future_you();
    new npc_rokhan();
    new spell_skytalon_molts();
    new npc_7th_legion_siege_engineer();
    new vehicle_alliance_steamtank();
    new npc_injured_7th_legion_soldier();
    new npc_mindless_ghoul();
    new spell_q12301_oriks_song();
    new go_oriks_crystalline_orb();
    new npc_hourglass_of_eternity_two();
    new npc_past_you();
    new npc_onslaught_warhorse();
    new npc_forsaken_blight_spreader();
    new npc_wintergarde_gryphon();
    new spell_taunka_transform();
    new spell_dragonblight_ice_cannon();
    new npc_no_mercy_for_the_captured();
}
