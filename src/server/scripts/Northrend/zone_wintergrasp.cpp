/* Copyright (C) 2008 - 2009 Trinity <http://www.trinitycore.org/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "BattlefieldMgr.h"
#include "BattlefieldWG.h"
#include "Battlefield.h"
#include "ScriptSystem.h"
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "Vehicle.h"
#include "GameObjectAI.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "Player.h"
#include "Group.h"
#include "PoolMgr.h"
#include "PassiveAI.h"
#include "CombatAI.h"
#include "DBCStores.h"


#define GOSSIP_HELLO_DEMO1  "Katapult bauen."
#define GOSSIP_HELLO_DEMO2  "Verwuester bauen."
#define GOSSIP_HELLO_DEMO3  "Belagerungsmaschine bauen."
#define GOSSIP_HELLO_DEMO4  "Ich kann nicht mehr bauen!"

enum WGqueuenpctext
{
    WG_NPCQUEUE_TEXT_H_NOWAR            = 14775,
    WG_NPCQUEUE_TEXT_H_QUEUE            = 14790,
    WG_NPCQUEUE_TEXT_H_WAR              = 14777,
    WG_NPCQUEUE_TEXT_A_NOWAR            = 14782,
    WG_NPCQUEUE_TEXT_A_QUEUE            = 14791,
    WG_NPCQUEUE_TEXT_A_WAR              = 14781,
    WG_NPCQUEUE_TEXTOPTION_JOIN         = 20077,
};

enum Spells
{
    // Demolisher engineers spells
    SPELL_BUILD_SIEGE_VEHICLE_FORCE_HORDE     = 61409,
    SPELL_BUILD_SIEGE_VEHICLE_FORCE_ALLIANCE  = 56662,
    SPELL_BUILD_CATAPULT_FORCE                = 56664,
    SPELL_BUILD_DEMOLISHER_FORCE              = 56659,
    SPELL_ACTIVATE_CONTROL_ARMS               = 49899,
    SPELL_RIDE_WG_VEHICLE                     = 60968,

    SPELL_VEHICLE_TELEPORT                    = 49759,

    // Spirit guide
    SPELL_CHANNEL_SPIRIT_HEAL                 = 22011,

    SPELL_WATER_OF_WINTERGRASP                = 36444
};

enum CreatureIds
{
    NPC_GOBLIN_MECHANIC                             = 30400,
    NPC_GNOMISH_ENGINEER                            = 30499,

    NPC_WINTERGRASP_CONTROL_ARMS                    = 27852,

    NPC_WORLD_TRIGGER_LARGE_AOI_NOT_IMMUNE_PC_NPC   = 23472,
    NPC_WORLD_TRIGGER_LARGE_OLD                     = 15384,

    NPC_WINTERGRASP_VEHICLE_KILLCREDIT              = 31093
};

enum TriggerGUIDs
{
    GUID_TRIGGER_VEHICLE_TELEPORT_1         = 131721,
    GUID_TRIGGER_VEHICLE_TELEPORT_2         = 131724,
};

enum QuestIds
{
    QUEST_BONES_AND_ARROWS_HORDE_ATT              = 13193,
    QUEST_JINXING_THE_WALLS_HORDE_ATT             = 13202,
    QUEST_SLAY_THEM_ALL_HORDE_ATT                 = 13180,
    QUEST_FUELING_THE_DEMOLISHERS_HORDE_ATT       = 13200,
    QUEST_HEALING_WITH_ROSES_HORDE_ATT            = 13201,
    QUEST_DEFEND_THE_SIEGE_HORDE_ATT              = 13223,

    QUEST_BONES_AND_ARROWS_HORDE_DEF              = 13199,
    QUEST_WARDING_THE_WALLS_HORDE_DEF             = 13192,
    QUEST_SLAY_THEM_ALL_HORDE_DEF                 = 13178,
    QUEST_FUELING_THE_DEMOLISHERS_HORDE_DEF       = 13191,
    QUEST_HEALING_WITH_ROSES_HORDE_DEF            = 13194,
    QUEST_TOPPLING_THE_TOWERS_HORDE_DEF           = 13539,
    QUEST_STOP_THE_SIEGE_HORDE_DEF                = 13185,

    QUEST_BONES_AND_ARROWS_ALLIANCE_ATT           = 13196,
    QUEST_WARDING_THE_WARRIORS_ALLIANCE_ATT       = 13198,
    QUEST_NO_MERCY_FOR_THE_MERCILESS_ALLIANCE_ATT = 13179,
    QUEST_DEFEND_THE_SIEGE_ALLIANCE_ATT           = 13222,
    QUEST_A_RARE_HERB_ALLIANCE_ATT                = 13195,
    QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_ATT    = 13197,

    QUEST_BONES_AND_ARROWS_ALLIANCE_DEF           = 13154,
    QUEST_WARDING_THE_WARRIORS_ALLIANCE_DEF       = 13153,
    QUEST_NO_MERCY_FOR_THE_MERCILESS_ALLIANCE_DEF = 13177,
    QUEST_SHOUTHERN_SABOTAGE_ALLIANCE_DEF         = 13538,
    QUEST_STOP_THE_SIEGE_ALLIANCE_DEF             = 13186,
    QUEST_A_RARE_HERB_ALLIANCE_DEF                = 13156,
    QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_DEF    = 236,
};

enum ZoneMisc
{
    ZONE_WINTERGRASP                              = 4197
};

uint8 const MAX_WINTERGRASP_VEHICLES = 4;

uint32 const vehiclesList[MAX_WINTERGRASP_VEHICLES] =
{
    NPC_WINTERGRASP_CATAPULT,
    NPC_WINTERGRASP_DEMOLISHER,
    NPC_WINTERGRASP_SIEGE_ENGINE_ALLIANCE,
    NPC_WINTERGRASP_SIEGE_ENGINE_HORDE
};

class npc_wg_demolisher_engineer : public CreatureScript
{
    public:
        npc_wg_demolisher_engineer() : CreatureScript("npc_wg_demolisher_engineer") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            if (CanBuild(creature))
            {
                if (player->HasAura(SPELL_CORPORAL))
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                else if (player->HasAura(SPELL_LIEUTENANT))
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                }
            }
            else
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_HELLO_DEMO4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 9);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            player->CLOSE_GOSSIP_MENU();

            if (CanBuild(creature))
            {
                switch (action - GOSSIP_ACTION_INFO_DEF)
                {
                    case 0:
                        creature->CastSpell(player, SPELL_BUILD_CATAPULT_FORCE, true);
                        break;
                    case 1:
                        creature->CastSpell(player, SPELL_BUILD_DEMOLISHER_FORCE, true);
                        break;
                    case 2:
                        creature->CastSpell(player, player->GetTeamId() == TEAM_ALLIANCE ? SPELL_BUILD_SIEGE_VEHICLE_FORCE_ALLIANCE : SPELL_BUILD_SIEGE_VEHICLE_FORCE_HORDE, true);
                        break;
                }
                if (Creature* controlArms = creature->FindNearestCreature(NPC_WINTERGRASP_CONTROL_ARMS, 30.0f, true))
                    creature->CastSpell(controlArms, SPELL_ACTIVATE_CONTROL_ARMS, true);
            }
            return true;
        }

    private:
        bool CanBuild(Creature* creature)
        {
            Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
            if (!wintergrasp)
                return false;

            switch (creature->GetEntry())
            {
                case NPC_GOBLIN_MECHANIC:
                    return (wintergrasp->GetData(BATTLEFIELD_WG_DATA_MAX_VEHICLE_H) > wintergrasp->GetData(BATTLEFIELD_WG_DATA_VEHICLE_H));
                case NPC_GNOMISH_ENGINEER:
                    return (wintergrasp->GetData(BATTLEFIELD_WG_DATA_MAX_VEHICLE_A) > wintergrasp->GetData(BATTLEFIELD_WG_DATA_VEHICLE_A));
                default:
                    return false;
            }
        }
};

class npc_wg_spirit_guide : public CreatureScript
{
    public:
        npc_wg_spirit_guide() : CreatureScript("npc_wg_spirit_guide") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
            if (!wintergrasp)
                return true;

            GraveyardVect graveyard = wintergrasp->GetGraveyardVector();
            for (uint8 i = 0; i < graveyard.size(); i++)
                if (graveyard[i]->GetControlTeamId() == player->GetTeamId())
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetSession()->GetTrinityString(((BfGraveyardWG*)graveyard[i])->GetTextId()), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + i);

            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 action) override
        {
            player->CLOSE_GOSSIP_MENU();

            Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
            if (wintergrasp)
            {
                GraveyardVect gy = wintergrasp->GetGraveyardVector();
                for (uint8 i = 0; i < gy.size(); i++)
                    if (action - GOSSIP_ACTION_INFO_DEF == i && gy[i]->GetControlTeamId() == player->GetTeamId())
                        if (WorldSafeLocsEntry const* safeLoc = sWorldSafeLocsStore.LookupEntry(gy[i]->GetGraveyardId()))
                            player->TeleportTo(safeLoc->map_id, safeLoc->x, safeLoc->y, safeLoc->z, 0);
            }
            return true;
        }

        struct npc_wg_spirit_guideAI : public ScriptedAI
        {
            npc_wg_spirit_guideAI(Creature* creature) : ScriptedAI(creature), gy(NULL)
            {
                teamId = creature->GetEntry() == NPC_DWARVEN_SPIRIT_GUIDE ? TEAM_ALLIANCE : TEAM_HORDE;
            }

            void UpdateAI(uint32 /*diff*/) override
            {
                if (BattlefieldWG* bf = static_cast<BattlefieldWG*>(sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG)))
                {
                    if (!bf->IsWarTime())
                        return;

                    if (!me->HasUnitState(UNIT_STATE_CASTING))
                    {
                        if (!gy)
                            gy = bf->GetGraveyardById(bf->GetSpiritGraveyardId(me->GetAreaId()));
                        if (!gy || gy->GetControlTeamId() != teamId)
                            return;

                        DoCastSelf(SPELL_CHANNEL_SPIRIT_HEAL);
                        bf->SetLastResurrectTime(me->GetGUID());
                    }
                }
            }

            TeamId teamId;
            BfGraveyard* gy;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_wg_spirit_guideAI(creature);
        }
};

enum WGQueue
{
    SPELL_FROST_ARMOR                               = 12544
};

class npc_wg_queue : public CreatureScript
{
    public:
        npc_wg_queue() : CreatureScript("npc_wg_queue") { }

    struct npc_wg_queueAI : public ScriptedAI
    {
        npc_wg_queueAI(Creature* creature) : ScriptedAI(creature)
        {
            FrostArmor_Timer = 0;
        }

        uint32 FrostArmor_Timer;

        void Reset() override
        {
            FrostArmor_Timer = 0;
        }

        void EnterCombat(Unit* /*who*/) override { }

        void UpdateAI(uint32 diff) override
        {
            if (FrostArmor_Timer <= diff)
            {
                DoCastSelf(SPELL_FROST_ARMOR);
                FrostArmor_Timer = 180000;
            }
            else FrostArmor_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_wg_queueAI(creature);
    }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());

            Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
            if (!wintergrasp)
                return true;

            if (wintergrasp->IsWarTime())
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetSession()->GetTrinityString(WG_NPCQUEUE_TEXTOPTION_JOIN), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                player->SEND_GOSSIP_MENU(wintergrasp->GetDefenderTeam() ? WG_NPCQUEUE_TEXT_H_WAR : WG_NPCQUEUE_TEXT_A_WAR, creature->GetGUID());
            }
            else
            {
                uint32 timer = wintergrasp->GetTimer() / 1000;
                player->SendUpdateWorldState(4354, time(NULL) + timer);
                if (timer < 15 * MINUTE)
                {
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, player->GetSession()->GetTrinityString(WG_NPCQUEUE_TEXTOPTION_JOIN), GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
                    player->SEND_GOSSIP_MENU(wintergrasp->GetDefenderTeam() ? WG_NPCQUEUE_TEXT_H_QUEUE : WG_NPCQUEUE_TEXT_A_QUEUE, creature->GetGUID());
                }
                else
                    player->SEND_GOSSIP_MENU(wintergrasp->GetDefenderTeam() ? WG_NPCQUEUE_TEXT_H_NOWAR : WG_NPCQUEUE_TEXT_A_NOWAR, creature->GetGUID());
            }
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 /*sender*/, uint32 /*action*/) override
        {
            player->CLOSE_GOSSIP_MENU();

            Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
            if (!wintergrasp)
                return true;

            if (wintergrasp->IsWarTime())
                wintergrasp->InvitePlayerToWar(player);
            else
            {
                uint32 timer = wintergrasp->GetTimer() / 1000;
                if (timer < 15 * MINUTE)
                    wintergrasp->InvitePlayerToQueue(player);
            }
            return true;
        }
};

class npc_wg_tower_cannon : public CreatureScript
{
public:
    npc_wg_tower_cannon() : CreatureScript("npc_wg_tower_cannon") {}

    struct npc_wg_tower_cannonAI : public PassiveAI
    {
        npc_wg_tower_cannonAI(Creature* creature) : PassiveAI(creature)
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
            me->SetControlled(true, UNIT_STATE_ROOT);
        }

        void Reset()
        {
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        }

        void PassengerBoarded(Unit* /*who*/, int8 /*seatId*/, bool apply)
        {
            if (apply)
            {
                if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                    if (!bf->IsWarTime())
                        me->Kill(me, false);
                // Just for Savety
                me->SetControlled(true, UNIT_STATE_ROOT);
            }
        }

        void EnterEvadeMode(EvadeReason /*why*/) override { }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_wg_tower_cannonAI(creature);
    }
};

class npc_wintergrasp_vehicle : public CreatureScript
{
    public:
        npc_wintergrasp_vehicle() : CreatureScript("npc_wintergrasp_vehicle") { }

        struct npc_wintergrasp_vehicleAI : public VehicleAI
        {
            npc_wintergrasp_vehicleAI(Creature* creature) : VehicleAI(creature) { }

            void InitializeAI() override
            {
                if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                    if (!bf->IsWarTime())
                        me->DespawnOrUnsummon();
            }

            void JustDied(Unit* killer) override
            {
                if (!me->HasAura(SPELL_WATER_OF_WINTERGRASP))
                    if (Player* owner = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                    {
                        if (Group* group = owner->GetGroup())
                        {
                            for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                                if (Player* member = itr->GetSource())
                                    member->KilledMonsterCredit(NPC_WINTERGRASP_VEHICLE_KILLCREDIT);
                        }
                        else // without group
                        {
                            owner->KilledMonsterCredit(NPC_WINTERGRASP_VEHICLE_KILLCREDIT);
                        }
                    }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_wintergrasp_vehicleAI(creature);
        }
};

class go_wg_vehicle_teleporter : public GameObjectScript
{
public:
    go_wg_vehicle_teleporter() : GameObjectScript("go_wg_vehicle_teleporter") { }

    struct go_wg_vehicle_teleporterAI : public GameObjectAI
    {
        go_wg_vehicle_teleporterAI(GameObject* gameObject) : GameObjectAI(gameObject), _checkTimer(1000) { }

        void UpdateAI(uint32 diff) override
        {
            if (_checkTimer <= diff)
            {
                if (Battlefield* wg = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                {                    
                        for (uint8 i = 0; i < MAX_WINTERGRASP_VEHICLES; i++)
                            if (Creature* vehicleCreature = go->FindNearestCreature(vehiclesList[i], 3.0f, true))
                            {
                                if (Player* player = go->FindNearestPlayer(3.0f))
                                { 
                                    TeamId fieldTeam    = wg->GetDefenderTeam();
                                    TeamId plrTeam      = player->GetTeamId();

                                    if (fieldTeam == plrTeam)
                                    {
                                        if (Vehicle* vehicle = player->GetVehicle())
                                        {
                                            if (!vehicle->GetBase()->HasAura(SPELL_VEHICLE_TELEPORT))
                                            {
                                                std::list<Creature*> teleportTriggerList;
                                                player->GetCreatureListWithEntryInGrid(teleportTriggerList, NPC_WORLD_TRIGGER_LARGE_AOI_NOT_IMMUNE_PC_NPC, 100.0f);
                                                for (std::list<Creature*>::const_iterator itr = teleportTriggerList.begin(); itr != teleportTriggerList.end(); ++itr)
                                                {
                                                    if ((*itr)->GetGUID().GetCounter() != GUID_TRIGGER_VEHICLE_TELEPORT_1 && (*itr)->GetGUID().GetCounter() != GUID_TRIGGER_VEHICLE_TELEPORT_2)
                                                        continue;

                                                    float x, y, z;
                                                    (*itr)->GetPosition(x, y, z);
                                                    (*itr)->CastSpell(vehicle->GetBase(), SPELL_VEHICLE_TELEPORT, true);
                                                    vehicle->TeleportVehicle(x, y, z, vehicle->GetBase()->GetOrientation());
                                                }
                                            }
                                        }
                                    }
                                }
                    }
                }
                _checkTimer = 1000;
            }
            else _checkTimer -= diff;
        }

        private:
            uint32 _checkTimer;
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_wg_vehicle_teleporterAI(go);
    }
};

class npc_wg_quest_giver : public CreatureScript
{
    public:
        npc_wg_quest_giver() : CreatureScript("npc_wg_quest_giver") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (creature->IsQuestGiver())
                player->PrepareQuestMenu(creature->GetGUID());
    
            Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
            if (!wintergrasp)
                return true;

            if (creature->IsVendor())
            {
                player->ADD_GOSSIP_ITEM_DB(Player::GetDefaultGossipMenuForSource(creature), 0, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_VENDOR);
                player->PlayerTalkClass->GetGossipMenu().AddGossipMenuItemData(0, 0, 0);
            }

            /// @todo: move this to conditions or something else

            // Player::PrepareQuestMenu(guid)
            if (creature->IsQuestGiver())
            {
                QuestRelationBounds objectQR = sObjectMgr->GetCreatureQuestRelationBounds(creature->GetEntry());
                QuestRelationBounds objectQIR = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(creature->GetEntry());

                QuestMenu& qm = player->PlayerTalkClass->GetQuestMenu();
                qm.ClearMenu();

                for (QuestRelations::const_iterator i = objectQIR.first; i != objectQIR.second; ++i)
                {
                    uint32 quest_id = i->second;
                    QuestStatus status = player->GetQuestStatus(quest_id);
                    if (status == QUEST_STATUS_COMPLETE)
                        qm.AddMenuItem(quest_id, 4);
                    else if (status == QUEST_STATUS_INCOMPLETE)
                        qm.AddMenuItem(quest_id, 4);
                    //else if (status == QUEST_STATUS_AVAILABLE)
                    //    qm.AddMenuItem(quest_id, 2);
                }

                std::vector<uint32> questRelationVector;
                for (QuestRelations::const_iterator i = objectQR.first; i != objectQR.second; ++i)
                {
                    uint32 questId = i->second;
                    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                    if (!quest)
                        continue;
    
                    switch (questId)
                    {
                        case QUEST_BONES_AND_ARROWS_ALLIANCE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_BONES_AND_ARROWS_ALLIANCE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_BONES_AND_ARROWS_ALLIANCE_ATT);
                            break;
                        case QUEST_WARDING_THE_WARRIORS_ALLIANCE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_WARDING_THE_WARRIORS_ALLIANCE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_WARDING_THE_WARRIORS_ALLIANCE_ATT);
                            break;
                        case QUEST_A_RARE_HERB_ALLIANCE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_A_RARE_HERB_ALLIANCE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_A_RARE_HERB_ALLIANCE_ATT);
                            break;
                        case QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_ATT);
                            break;
                        case QUEST_BONES_AND_ARROWS_HORDE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_BONES_AND_ARROWS_HORDE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_BONES_AND_ARROWS_HORDE_ATT);
                            break;
                        case QUEST_JINXING_THE_WALLS_HORDE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_WARDING_THE_WALLS_HORDE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_JINXING_THE_WALLS_HORDE_ATT);
                            break;
                        case QUEST_FUELING_THE_DEMOLISHERS_HORDE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_FUELING_THE_DEMOLISHERS_HORDE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_FUELING_THE_DEMOLISHERS_HORDE_ATT);
                            break;
                        case QUEST_HEALING_WITH_ROSES_HORDE_ATT:
                            if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_HEALING_WITH_ROSES_HORDE_DEF))
                                continue;
                            questRelationVector.push_back(QUEST_HEALING_WITH_ROSES_HORDE_ATT);
                            break;
                        default:
                            questRelationVector.push_back(questId);
                            break;
                    }
                }
    
                for (std::vector<uint32>::const_iterator i = questRelationVector.begin(); i != questRelationVector.end(); ++i)
                {
                    uint32 questId = *i;
                    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                    switch (questId)
                    {
                            // Horde attacker
                        case QUEST_BONES_AND_ARROWS_HORDE_ATT:
                        case QUEST_JINXING_THE_WALLS_HORDE_ATT:
                        case QUEST_SLAY_THEM_ALL_HORDE_ATT:
                        case QUEST_FUELING_THE_DEMOLISHERS_HORDE_ATT:
                        case QUEST_HEALING_WITH_ROSES_HORDE_ATT:
                        case QUEST_DEFEND_THE_SIEGE_HORDE_ATT:
                            if (wintergrasp->GetAttackerTeam() == TEAM_HORDE)
                            {
                                QuestStatus status = player->GetQuestStatus(questId);
                        
                                if (quest->IsAutoComplete() && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 4);
                                else if (status == QUEST_STATUS_NONE && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 2);
                            }
                            break;
                            // Horde defender
                        case QUEST_BONES_AND_ARROWS_HORDE_DEF:
                        case QUEST_WARDING_THE_WALLS_HORDE_DEF:
                        case QUEST_SLAY_THEM_ALL_HORDE_DEF:
                        case QUEST_FUELING_THE_DEMOLISHERS_HORDE_DEF:
                        case QUEST_HEALING_WITH_ROSES_HORDE_DEF:
                        case QUEST_TOPPLING_THE_TOWERS_HORDE_DEF:
                        case QUEST_STOP_THE_SIEGE_HORDE_DEF:
                            if (wintergrasp->GetDefenderTeam() == TEAM_HORDE)
                            {
                                QuestStatus status = player->GetQuestStatus(questId);
                        
                                if (quest->IsAutoComplete() && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 4);
                                else if (status == QUEST_STATUS_NONE && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 2);
                            }
                            break;
                            // Alliance attacker
                        case QUEST_BONES_AND_ARROWS_ALLIANCE_ATT:
                        case QUEST_WARDING_THE_WARRIORS_ALLIANCE_ATT:
                        case QUEST_NO_MERCY_FOR_THE_MERCILESS_ALLIANCE_ATT:
                        case QUEST_DEFEND_THE_SIEGE_ALLIANCE_ATT:
                        case QUEST_A_RARE_HERB_ALLIANCE_ATT:
                        case QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_ATT:
                            if (wintergrasp->GetAttackerTeam() == TEAM_ALLIANCE)
                            {
                                QuestStatus status = player->GetQuestStatus(questId);
                        
                                if (quest->IsAutoComplete() && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 4);
                                else if (status == QUEST_STATUS_NONE && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 2);
                            }
                            break;
                            // Alliance defender
                        case QUEST_BONES_AND_ARROWS_ALLIANCE_DEF:
                        case QUEST_WARDING_THE_WARRIORS_ALLIANCE_DEF:
                        case QUEST_NO_MERCY_FOR_THE_MERCILESS_ALLIANCE_DEF:
                        case QUEST_SHOUTHERN_SABOTAGE_ALLIANCE_DEF:
                        case QUEST_STOP_THE_SIEGE_ALLIANCE_DEF:
                        case QUEST_A_RARE_HERB_ALLIANCE_DEF:
                        case QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_DEF:
                            if (wintergrasp->GetDefenderTeam() == TEAM_ALLIANCE)
                            {
                                QuestStatus status = player->GetQuestStatus(questId);
                        
                                if (quest->IsAutoComplete() && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 4);
                                else if (status == QUEST_STATUS_NONE && player->CanTakeQuest(quest, false))
                                    qm.AddMenuItem(questId, 2);
                            }
                            break;
                        default:
                            QuestStatus status = player->GetQuestStatus(questId);
                        
                            if (quest->IsAutoComplete() && player->CanTakeQuest(quest, false))
                                qm.AddMenuItem(questId, 4);
                            else if (status == QUEST_STATUS_NONE && player->CanTakeQuest(quest, false))
                                qm.AddMenuItem(questId, 2);
                            break;
                    }
                }
            }
            player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
            return true;
        }
    
        uint32 GetDialogStatus(Player* player, Creature* creature)
        {
            QuestRelationBounds qr = sObjectMgr->GetCreatureQuestRelationBounds(creature->GetEntry());
            QuestRelationBounds qir = sObjectMgr->GetCreatureQuestInvolvedRelationBounds(creature->GetEntry());
            QuestGiverStatus result = DIALOG_STATUS_NONE;
    
            for (QuestRelations::const_iterator i = qir.first; i != qir.second; ++i)
            {
                QuestGiverStatus result2 = DIALOG_STATUS_NONE;
                uint32 questId = i->second;
                Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                if (!quest)
                    continue;
    
                if (!sConditionMgr->IsObjectMeetingNotGroupedConditions(CONDITION_SOURCE_TYPE_QUEST_ACCEPT, quest->GetQuestId(), player))
                    continue;
    
                QuestStatus status = player->GetQuestStatus(questId);
                if ((status == QUEST_STATUS_COMPLETE && !player->GetQuestRewardStatus(questId)) ||
                    (quest->IsAutoComplete() && player->CanTakeQuest(quest, false)))
                {
                    if (quest->IsAutoComplete() && quest->IsRepeatable() && !quest->IsDailyOrWeekly())
                        result2 = DIALOG_STATUS_REWARD_REP;
                    else
                        result2 = DIALOG_STATUS_REWARD;
                }
                else if (status == QUEST_STATUS_INCOMPLETE)
                    result2 = DIALOG_STATUS_INCOMPLETE;
    
                if (result2 > result)
                    result = result2;
            }
    
            for (QuestRelations::const_iterator i = qr.first; i != qr.second; ++i)
            {
                QuestGiverStatus result2 = DIALOG_STATUS_NONE;
                uint32 questId = i->second;
                Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                if (!quest)
                    continue;

                if (!sConditionMgr->IsObjectMeetingNotGroupedConditions(CONDITION_SOURCE_TYPE_QUEST_ACCEPT, quest->GetQuestId(), player))
                    continue;
    
                switch (questId)
                {
                    case QUEST_BONES_AND_ARROWS_ALLIANCE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_BONES_AND_ARROWS_ALLIANCE_DEF))
                            continue;
                        break;
                    case QUEST_WARDING_THE_WARRIORS_ALLIANCE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_WARDING_THE_WARRIORS_ALLIANCE_DEF))
                            continue;
                        break;
                    case QUEST_A_RARE_HERB_ALLIANCE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_A_RARE_HERB_ALLIANCE_DEF))
                            continue;
                        break;
                    case QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_FUELING_THE_DEMOLISHERS_ALLIANCE_DEF))
                            continue;
                        break;
                    case QUEST_BONES_AND_ARROWS_HORDE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_BONES_AND_ARROWS_HORDE_DEF))
                            continue;
                        break;
                    case QUEST_JINXING_THE_WALLS_HORDE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_WARDING_THE_WALLS_HORDE_DEF))
                            continue;
                        break;
                    case QUEST_FUELING_THE_DEMOLISHERS_HORDE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_FUELING_THE_DEMOLISHERS_HORDE_DEF))
                            continue;
                        break;
                    case QUEST_HEALING_WITH_ROSES_HORDE_ATT:
                        if (!sPoolMgr->IsSpawnedObject<Quest>(QUEST_HEALING_WITH_ROSES_HORDE_DEF))
                            continue;
                        break;
                }
    
                QuestStatus status = player->GetQuestStatus(questId);
                if (status == QUEST_STATUS_NONE)
                {
                    if (player->CanSeeStartQuest(quest))
                    {
                        if (player->SatisfyQuestLevel(quest, false))
                        {
                            if (quest->IsAutoComplete())
                                result2 = DIALOG_STATUS_REWARD_REP;
                            else if (player->getLevel() <= (player->GetQuestLevel(quest) + sWorld->getIntConfig(CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF)))
                            {
                                if (quest->IsDaily())
                                    result2 = DIALOG_STATUS_AVAILABLE_REP;
                                else
                                    result2 = DIALOG_STATUS_AVAILABLE;
                            }
                            else
                                result2 = DIALOG_STATUS_LOW_LEVEL_AVAILABLE;
                        }
                        else
                            result2 = DIALOG_STATUS_UNAVAILABLE;
                    }
                }
    
                if (result2 > result)
                    result = result2;
            }
    
            return result;
        }
};

class spell_wintergrasp_force_building : public SpellScriptLoader
{
    public:
        spell_wintergrasp_force_building() : SpellScriptLoader("spell_wintergrasp_force_building") { }

        class spell_wintergrasp_force_building_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_wintergrasp_force_building_SpellScript);

            bool Validate(SpellInfo const* /*spell*/) override
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_BUILD_CATAPULT_FORCE)
                    || !sSpellMgr->GetSpellInfo(SPELL_BUILD_DEMOLISHER_FORCE)
                    || !sSpellMgr->GetSpellInfo(SPELL_BUILD_SIEGE_VEHICLE_FORCE_HORDE)
                    || !sSpellMgr->GetSpellInfo(SPELL_BUILD_SIEGE_VEHICLE_FORCE_ALLIANCE))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetHitUnit()->CastSpell(GetHitUnit(), GetEffectValue(), false);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_wintergrasp_force_building_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_wintergrasp_force_building_SpellScript();
        }
};

class spell_wintergrasp_create_vehicle : public SpellScriptLoader
{
    public:
        spell_wintergrasp_create_vehicle() : SpellScriptLoader("spell_wintergrasp_create_vehicle") { }

        class spell_wintergrasp_create_vehicle_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_wintergrasp_create_vehicle_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                float x, y, z, o;
                if (Creature* arms = GetCaster()->FindNearestCreature(NPC_WINTERGRASP_CONTROL_ARMS, 10.0f))
                    arms->GetPosition(x, y, z, o);
                else
                    return;
                GetCaster()->SummonCreature(GetSpellInfo()->Effects[EFFECT_1].MiscValue, x, y, z, o);
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_wintergrasp_create_vehicle_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_wintergrasp_create_vehicle_SpellScript();
        }
};

class spell_wintergrasp_grab_passenger : public SpellScriptLoader
{
    public:
        spell_wintergrasp_grab_passenger() : SpellScriptLoader("spell_wintergrasp_grab_passenger") { }

        class spell_wintergrasp_grab_passenger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_wintergrasp_grab_passenger_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Player* target = GetHitPlayer())
                    target->CastSpell(GetCaster(), SPELL_RIDE_WG_VEHICLE, false);
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_wintergrasp_grab_passenger_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_wintergrasp_grab_passenger_SpellScript();
        }
};

class achievement_wg_did_not_stand_a_chance : public AchievementCriteriaScript
{
    public:
        achievement_wg_did_not_stand_a_chance() : AchievementCriteriaScript("achievement_wg_did_not_stand_a_chance") { }

        bool OnCheck(Player* source, Unit* target)
        {
            if (!target || !target->IsMounted())
                return false;
            
            if (source->GetVehicleBase() && source->GetVehicleBase()->GetEntry() == NPC_WINTERGRASP_TOWER_CANNON)
                return true;
            
            return false;
        }
};

enum WgTeleport
{
    SPELL_WINTERGRASP_TELEPORT_TRIGGER = 54643,
};

class spell_wintergrasp_defender_teleport : public SpellScriptLoader
{
public:
    spell_wintergrasp_defender_teleport() : SpellScriptLoader("spell_wintergrasp_defender_teleport") { }

    class spell_wintergrasp_defender_teleport_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wintergrasp_defender_teleport_SpellScript);

        SpellCastResult CheckCast()
        {
            if (Battlefield* wg = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                if (Player* target = GetExplTargetUnit()->ToPlayer())
                    // check if we are in Wintergrasp at all, SotA uses same teleport spells
                    if ((target->GetZoneId() == 4197 && target->GetTeamId() != wg->GetDefenderTeam()) || target->HasAura(SPELL_WINTERGRASP_TELEPORT_TRIGGER))
                        return SPELL_FAILED_BAD_TARGETS;
            return SPELL_CAST_OK;
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_wintergrasp_defender_teleport_SpellScript::CheckCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_wintergrasp_defender_teleport_SpellScript();
    }
};

class spell_wintergrasp_defender_teleport_trigger : public SpellScriptLoader
{
public:
    spell_wintergrasp_defender_teleport_trigger() : SpellScriptLoader("spell_wintergrasp_defender_teleport_trigger") { }

    class spell_wintergrasp_defender_teleport_trigger_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wintergrasp_defender_teleport_trigger_SpellScript);

        void HandleDummy(SpellEffIndex /*effindex*/)
        {
            Unit* caster = GetCaster();
            std::list<Creature*> teleportTriggerList;
            caster->GetCreatureListWithEntryInGrid(teleportTriggerList, NPC_WORLD_TRIGGER_LARGE_OLD, 100.0f);
            for (std::list<Creature*>::const_iterator itr = teleportTriggerList.begin(); itr != teleportTriggerList.end(); ++itr)
            {
                WorldLocation loc;
                (*itr)->GetPosition(&loc);
                SetExplTargetDest(loc);
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_wintergrasp_defender_teleport_trigger_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_wintergrasp_defender_teleport_trigger_SpellScript();
    }
};

// 58622 - teleport to wintergrasp
class spell_wintergrasp_teleport_from_dalaran : public SpellScriptLoader
{
public:
    spell_wintergrasp_teleport_from_dalaran() : SpellScriptLoader("spell_wintergrasp_teleport_from_dalaran") { }

    class spell_wintergrasp_teleport_from_dalaran_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wintergrasp_teleport_from_dalaran_SpellScript);

        void HandleScriptEffect()
        {
            Battlefield* wg = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG);
            Player* player = GetCaster()->ToPlayer();
            if (!wg || !player)
                return;

            if (player->GetTeamId() == wg->GetDefenderTeam())
                player->TeleportTo(WgDefenderTeleportLoc);
            else
            {
                if (player->GetTeamId() == TEAM_HORDE)
                    player->TeleportTo(WgAttackerTeleportLoc[TEAM_HORDE]);
                else
                    player->TeleportTo(WgAttackerTeleportLoc[TEAM_ALLIANCE]);
            }
        }

        void Register()
        {
            OnCast += SpellCastFn(spell_wintergrasp_teleport_from_dalaran_SpellScript::HandleScriptEffect);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_wintergrasp_teleport_from_dalaran_SpellScript();
    }
};

class spell_wintergrasp_rocket_goblin_grenade : public SpellScriptLoader
{
public:
    spell_wintergrasp_rocket_goblin_grenade() : SpellScriptLoader("spell_wintergrasp_rocket_goblin_grenade") { }

    class spell_wintergrasp_rocket_goblin_grenade_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wintergrasp_rocket_goblin_grenade_SpellScript);

        void HandleDummy(SpellEffIndex /*effindex*/)
        {
            if (Unit* caster = GetCaster())
            {
                float x, y, z;
                GetExplTargetDest()->GetPosition(x, y, z);
                caster->CastSpell(x, y, z, 49769, true);
            }
        }

        void Register()
        {
            OnEffectLaunch += SpellEffectFn(spell_wintergrasp_rocket_goblin_grenade_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_wintergrasp_rocket_goblin_grenade_SpellScript();
    }
};

// Fire Cannon trigger spell - 51422
class spell_wintergrasp_tower_cannon : public SpellScriptLoader
{
public:
    spell_wintergrasp_tower_cannon() : SpellScriptLoader("spell_wintergrasp_tower_cannon") { }

    class spell_wintergrasp_tower_cannon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_wintergrasp_tower_cannon_SpellScript);

        void HandleDummyHitTarget(SpellEffIndex /*effIndex*/)
        {
            if (Unit* target = GetHitUnit())
                if (target->GetTypeId() == TYPEID_PLAYER)
                {
                    uint32 damage = 3096 + urand(0, GetSpellInfo()->Effects[EFFECT_0].DieSides);
                    damage = target->SpellDamageBonusTaken(GetCaster(), GetSpellInfo(), damage, SPELL_DIRECT_DAMAGE);
                    SetHitDamage(damage);
                }
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_wintergrasp_tower_cannon_SpellScript::HandleDummyHitTarget, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_wintergrasp_tower_cannon_SpellScript();
    }
};

class WintergraspAuraRemoveScript : public PlayerScript
{
public:
    WintergraspAuraRemoveScript() : PlayerScript("wg_aura_remove_script") {}

    void OnUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/, uint32 /*oldZone*/)
    {
        // zone wintergrasp
        if (player->GetZoneId() != 4197)
            return;

        player->RemoveAurasDueToSpell(SPELL_SPIRITUAL_IMMUNITY);
    }
};

enum eWGVehicle
{
    NPC_CATAPULT          = 27881,
    NPC_DEMOLISHER        = 28094,
    NPC_TOWER_CANON       = 28366,
    NPC_SIEGE_ENGINE_A    = 28312,
    NPC_SIEGE_ENGINE_H    = 32627,
    NPC_SARAHS_BG_BRUISER = 32898,
};

class achievement_vehicular_gnomeslaughter : public AchievementCriteriaScript
{
    public:
        achievement_vehicular_gnomeslaughter() : AchievementCriteriaScript("achievement_vehicular_gnomeslaughter") { }
        
        bool OnCheck(Player* source, Unit* target)
        {
            if (!target)
                return false;

            if (!source->GetVehicleBase())
                return false;
            
            switch (source->GetVehicleBase()->GetEntry())
            {
                case NPC_CATAPULT:
                case NPC_DEMOLISHER:
                case NPC_SIEGE_ENGINE_A:
                case NPC_SIEGE_ENGINE_H:
                case NPC_SARAHS_BG_BRUISER:
                    return true;
                    break;
                default:
                    break;
            }
            return false;
        }
};

class spell_wintergrasp_swift_spectral_gryphon : public SpellScriptLoader
{
    public:
        spell_wintergrasp_swift_spectral_gryphon() : SpellScriptLoader("spell_wintergrasp_swift_spectral_gryphon") { }

        class spell_wintergrasp_swift_spectral_gryphon_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_wintergrasp_swift_spectral_gryphon_AuraScript);

            // Add Update for Areacheck and BF-Timercheck
            void CalcPeriodic(AuraEffect const* /*effect*/, bool& isPeriodic, int32& amplitude)
            {
                isPeriodic = true;
                amplitude = 1 * IN_MILLISECONDS;
            }

            void Update(AuraEffect* /*effect*/)
            {
                if (Player* owner = GetUnitOwner()->ToPlayer())
                {
                    if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                        if (bf->IsWarTime() && owner->GetZoneId() == ZONE_WINTERGRASP)
                            owner->RemoveAura(GetSpellInfo()->Id);
                }
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Player* caster = GetCaster()->ToPlayer();

                if (!caster)
                    return;

                if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                    if (bf->IsWarTime() && caster->GetZoneId() == ZONE_WINTERGRASP)
                        caster->RemoveAura(GetSpellInfo()->Id);
            }

            void Register()
            {
               DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_wintergrasp_swift_spectral_gryphon_AuraScript::CalcPeriodic, EFFECT_0, SPELL_AURA_MOUNTED);
               OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_wintergrasp_swift_spectral_gryphon_AuraScript::Update, EFFECT_0, SPELL_AURA_MOUNTED);
               OnEffectApply += AuraEffectApplyFn(spell_wintergrasp_swift_spectral_gryphon_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOUNTED, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_wintergrasp_swift_spectral_gryphon_AuraScript();
        }
};

class spell_wintergrasp_swift_flying_wisp : public SpellScriptLoader
{
   public:
       spell_wintergrasp_swift_flying_wisp() : SpellScriptLoader("spell_wintergrasp_swift_flying_wisp") { }

       class spell_wintergrasp_swift_flying_wisp_AuraScript : public AuraScript
       {
           PrepareAuraScript(spell_wintergrasp_swift_flying_wisp_AuraScript);

           // Add Update for Areacheck and BF-Timercheck
           void CalcPeriodic(AuraEffect const* /*effect*/, bool& isPeriodic, int32& amplitude)
           {
               isPeriodic = true;
               amplitude = 1 * IN_MILLISECONDS;
           }

           void Update(AuraEffect* /*effect*/)
           {
               if (Player* owner = GetUnitOwner()->ToPlayer())
               {
                   if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                       if (bf->IsWarTime() && owner->GetZoneId() == ZONE_WINTERGRASP)
                           owner->RemoveAura(GetSpellInfo()->Id);
               }
           }

           void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
           {
               Player* caster = GetCaster()->ToPlayer();

               if (!caster)
                   return;

               if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldByBattleId(BATTLEFIELD_BATTLEID_WG))
                   if (bf->IsWarTime() && caster->GetZoneId() == ZONE_WINTERGRASP)
                       caster->RemoveAura(GetSpellInfo()->Id);
           }

           void Register()
           {
               DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_wintergrasp_swift_flying_wisp_AuraScript::CalcPeriodic, EFFECT_0, SPELL_AURA_FLY);
               OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_wintergrasp_swift_flying_wisp_AuraScript::Update, EFFECT_0, SPELL_AURA_FLY);
               OnEffectApply += AuraEffectApplyFn(spell_wintergrasp_swift_flying_wisp_AuraScript::OnApply, EFFECT_0, SPELL_AURA_FLY, AURA_EFFECT_HANDLE_REAL);
           }
       };

       AuraScript* GetAuraScript() const override
       {
           return new spell_wintergrasp_swift_flying_wisp_AuraScript();
       }
};

class spell_wintergrasp_hide_small_elementals : public SpellScriptLoader
{
    public:
        spell_wintergrasp_hide_small_elementals() : SpellScriptLoader("spell_wintergrasp_hide_small_elementals") { }

        class spell_wintergrasp_hide_small_elementals_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_wintergrasp_hide_small_elementals_AuraScript);

            void HandlePeriodicDummy(AuraEffect const* aurEff)
            {
                Unit* target = GetTarget();
                Battlefield* Bf = sBattlefieldMgr->GetBattlefieldToZoneId(target->GetZoneId());
                bool enable = !Bf || !Bf->IsWarTime();
                target->SetPhaseMask(enable ? 1 : 512, true);
                PreventDefaultAction();
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_wintergrasp_hide_small_elementals_AuraScript::HandlePeriodicDummy, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const override
        {
            return new spell_wintergrasp_hide_small_elementals_AuraScript();
        }
};

void AddSC_wintergrasp()
{
    new npc_wg_queue();
    new npc_wg_spirit_guide();
    new npc_wg_demolisher_engineer();
    new npc_wg_tower_cannon();
    new npc_wintergrasp_vehicle();
    new go_wg_vehicle_teleporter();
    new npc_wg_quest_giver();
    new spell_wintergrasp_force_building();
    new spell_wintergrasp_grab_passenger();
    new achievement_wg_did_not_stand_a_chance();
    new spell_wintergrasp_defender_teleport();
    new spell_wintergrasp_defender_teleport_trigger();
    new spell_wintergrasp_teleport_from_dalaran();

    // wintergrasp vehicle and weapon spells
    new spell_wintergrasp_rocket_goblin_grenade();
    new spell_wintergrasp_tower_cannon();
    new achievement_vehicular_gnomeslaughter();

    new WintergraspAuraRemoveScript();
    new spell_wintergrasp_swift_spectral_gryphon();
    new spell_wintergrasp_swift_flying_wisp();
    new spell_wintergrasp_hide_small_elementals();
}
