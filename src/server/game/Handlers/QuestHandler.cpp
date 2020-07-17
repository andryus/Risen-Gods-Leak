/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "Common.h"
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "GossipDef.h"
#include "QuestDef.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Battleground.h"
#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "Chat.h"
#include "RG/Logging/LogManager.hpp"

#include "CreatureHooks.h"

void WorldSession::HandleQuestgiverStatusQueryOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;
    uint32 questStatus = DIALOG_STATUS_NONE;

    Object* questGiver = ObjectAccessor::GetObjectByTypeMask(*GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
    if (!questGiver)
    {
        TC_LOG_INFO("network", "Error in CMSG_QUESTGIVER_STATUS_QUERY, called for non-existing questgiver (%s)", guid.ToString().c_str());
        return;
    }

    switch (questGiver->GetTypeId())
    {
        case TYPEID_UNIT:
        {
            TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_STATUS_QUERY for npc, guid = %u", questGiver->GetGUID().GetCounter());
            if (!questGiver->ToCreature()->IsHostileTo(GetPlayer())) // do not show quest status to enemies
                questStatus = GetPlayer()->GetQuestDialogStatus(questGiver);
            break;
        }
        case TYPEID_GAMEOBJECT:
        {
            TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_STATUS_QUERY for GameObject guid = %u", questGiver->GetGUID().GetCounter());
            questStatus = GetPlayer()->GetQuestDialogStatus(questGiver);
            break;
        }
        default:
            TC_LOG_ERROR("network", "QuestGiver called for unexpected type %hhu", questGiver->GetTypeId());
            break;
    }

    // inform client about status of quest
    GetPlayer()->PlayerTalkClass->SendQuestGiverStatus(uint8(questStatus), guid);
}

void WorldSession::HandleQuestgiverHelloOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData >> guid;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_HELLO %s", guid.ToString().c_str());

    Creature* creature = GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    if (!creature)
    {
        TC_LOG_DEBUG("network", "WORLD: HandleQuestgiverHelloOpcode - %s not found or you can't interact with him.",
            guid.ToString().c_str());
        return;
    }

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);
    // Stop the npc if moving
    creature->StopMoving();

    if (sScriptMgr->OnGossipHello(GetPlayer(), creature))
        return;

    GetPlayer()->PrepareGossipMenu(creature, creature->GetCreatureTemplate()->GossipMenuId, true);
    GetPlayer()->SendPreparedGossip(creature);

    if (CreatureAI* AI = creature->AI())
        AI->sGossipHello(GetPlayer());
}

void WorldSession::HandleQuestgiverAcceptQuestOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId;
    uint32 startCheat;
    recvData >> guid >> questId >> startCheat;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_ACCEPT_QUEST npc %s, quest = %u, startCheat = %u", guid.ToString().c_str(), questId, startCheat);

    Object* object;
    if (!guid.IsPlayer())
        object = ObjectAccessor::GetObjectByTypeMask(*GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM);
    else
        object = ObjectAccessor::GetPlayer(*GetPlayer(), guid);

#define CLOSE_GOSSIP_CLEAR_DIVIDER() \
    do { \
        GetPlayer()->PlayerTalkClass->SendCloseGossip(); \
        GetPlayer()->SetQuestSharer(ObjectGuid::Empty); \
    } while (0)

    // no or incorrect quest giver
    if (!object)
    {
        CLOSE_GOSSIP_CLEAR_DIVIDER();
        return;
    }

    if (Player* playerQuestObject = object->ToPlayer())
    {
        if ((_player->GetQuestSharer() && _player->GetQuestSharer() != guid) ||
           (!playerQuestObject->CanShareQuest(questId)))
        {
            CLOSE_GOSSIP_CLEAR_DIVIDER();
            return;
        }
        if (!GetPlayer()->IsInSameRaidWith(playerQuestObject))
        {
            CLOSE_GOSSIP_CLEAR_DIVIDER();
            return;
        }
    }
    else
    {
        if (!object->hasQuest(questId))
        {
            CLOSE_GOSSIP_CLEAR_DIVIDER();
            return;
        }
    }

    // some kind of WPE protection
    if (!GetPlayer()->CanInteractWithQuestGiver(object))
    {
        // @TODO: inform player
        CLOSE_GOSSIP_CLEAR_DIVIDER();
        return;
    }

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        // prevent cheating
        if (!GetPlayer()->CanTakeQuest(quest, true))
        {
            CLOSE_GOSSIP_CLEAR_DIVIDER();
            return;
        }

        if (!GetPlayer()->GetQuestSharer().IsEmpty())
        {
            Player* player = ObjectAccessor::GetPlayer(*GetPlayer(), GetPlayer()->GetQuestSharer());
            if (player)
            {
                player->SendPushToPartyResponse(GetPlayer(), QUEST_PARTY_MSG_ACCEPT_QUEST);
            }
            GetPlayer()->SetQuestSharer(ObjectGuid::Empty);
        }

        if (GetPlayer()->CanAddQuest(quest, true))
        {
            GetPlayer()->AddQuestAndCheckCompletion(quest, object);

            if (quest->HasFlag(QUEST_FLAGS_PARTY_ACCEPT))
            {
                if (auto group = GetPlayer()->GetGroup())
                {
                    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
                    {
                        Player* player = itr->GetSource();

                        if (!player || player == GetPlayer())     // not self
                            continue;

                        if (player->CanTakeQuest(quest, true))
                        {
                            player->SetQuestSharer(GetPlayer()->GetGUID());

                            // need confirmation that any gossip window will close
                            player->PlayerTalkClass->SendCloseGossip();

                            GetPlayer()->SendQuestConfirmAccept(quest, player);
                        }
                    }
                }
            }
            
            if (TicketQuestState const* state = sObjectMgr->GetQuestState(questId))
            {
                ChatHandler chat(GetPlayer()->GetSession());

                if (GetPlayer()->GetSession()->GetSessionDbcLocale() == LOCALE_deDE) 
                {
                    chat.PSendSysMessage("Diese Quest wurde als verbuggt gemeldet und kann wahrscheinlich nicht korrekt abgeschlossen werden!");
                    chat.PSendSysMessage("Mehr Informationen findest du auf http://redmine.rising-gods.de/issues/%u", state->ticket);
                }
                else
                {
                    chat.PSendSysMessage("This quest was reported as bugged and cannot be completed correctly!");
                    chat.PSendSysMessage("For more information, visit http://redmine.rising-gods.de/issues/%u", state->ticket);
                }

            }

            CLOSE_GOSSIP_CLEAR_DIVIDER();

            if (quest->GetSrcSpell() > 0)
                GetPlayer()->CastSpell(GetPlayer(), quest->GetSrcSpell(), true);

            return;
        }
    }

    CLOSE_GOSSIP_CLEAR_DIVIDER();

#undef CLOSE_GOSSIP_CLEAR_DIVIDER
}

void WorldSession::HandleQuestgiverQueryQuestOpcode(WorldPacket& recvData)
{
    ObjectGuid guid;
    uint32 questId;
    uint8 unk1;
    recvData >> guid >> questId >> unk1;
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_QUERY_QUEST npc = %s, quest = %u, unk1 = %u", guid.ToString().c_str(), questId, unk1);

    // Verify that the guid is valid and is a questgiver or involved in the requested quest
    Object* object = ObjectAccessor::GetObjectByTypeMask(*GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT | TYPEMASK_ITEM);
    if (!object || (!object->hasQuest(questId) && !object->hasInvolvedQuest(questId)))
    {
        GetPlayer()->PlayerTalkClass->SendCloseGossip();
        return;
    }

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        // not sure here what should happen to quests with QUEST_FLAGS_AUTOCOMPLETE
        // if this breaks them, add && object->GetTypeId() == TYPEID_ITEM to this check
        // item-started quests never have that flag
        if (!GetPlayer()->CanTakeQuest(quest, true))
            return;

        if (quest->IsAutoAccept() && GetPlayer()->CanAddQuest(quest, true))
            GetPlayer()->AddQuestAndCheckCompletion(quest, object);

        if (quest->HasFlag(QUEST_FLAGS_AUTOCOMPLETE))
            GetPlayer()->PlayerTalkClass->SendQuestGiverRequestItems(quest, object->GetGUID(), GetPlayer()->CanCompleteQuest(quest->GetQuestId()), true);
        else
            GetPlayer()->PlayerTalkClass->SendQuestGiverQuestDetails(quest, object->GetGUID(), true);
    }
}

void WorldSession::HandleQuestQueryOpcode(WorldPacket& recvData)
{
    if (!GetPlayer())
        return;

    uint32 questId;
    recvData >> questId;
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUEST_QUERY quest = %u", questId);

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
        GetPlayer()->PlayerTalkClass->SendQuestQueryResponse(quest);
}

void WorldSession::HandleQuestgiverChooseRewardOpcode(WorldPacket& recvData)
{
    uint32 questId, reward;
    ObjectGuid guid;
    recvData >> guid >> questId >> reward;

    if (reward >= QUEST_REWARD_CHOICES_COUNT)
    {
        TC_LOG_ERROR("network", "Error in CMSG_QUESTGIVER_CHOOSE_REWARD: player %s (guid %d) tried to get invalid reward (%u) (possible packet-hacking detected)", GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter(), reward);
        return;
    }

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_CHOOSE_REWARD npc = %s, quest = %u, reward = %u", guid.ToString().c_str(), questId, reward);

    Object* object = ObjectAccessor::GetObjectByTypeMask(*GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
    if (!object || !object->hasInvolvedQuest(questId))
        return;

    // some kind of WPE protection
    if (!GetPlayer()->CanInteractWithQuestGiver(object))
        return;

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        if ((!GetPlayer()->CanSeeStartQuest(quest) &&  GetPlayer()->GetQuestStatus(questId) == QUEST_STATUS_NONE) ||
            (GetPlayer()->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE && !quest->IsAutoComplete()))
        {
            TC_LOG_ERROR("network", "Error in QUEST_STATUS_COMPLETE: player %s (guid %u) tried to complete quest %u, but is not allowed to do so (possible packet-hacking or high latency)",
                           GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter(), questId);
            return;
        }
        if (GetPlayer()->CanRewardQuest(quest, reward, true))
        {
            GetPlayer()->RewardQuest(quest, reward, object);

            switch (object->GetTypeId())
            {
                case TYPEID_UNIT:
                {
                    Creature* questgiver = object->ToCreature();
                    if (!sScriptMgr->OnQuestReward(GetPlayer(), questgiver, quest, reward))
                    {
                        // Send next quest
                        if (Quest const* nextQuest = GetPlayer()->GetNextQuest(guid, quest))
                        {
                            // Only send the quest to the player if the conditions are met
                            if (GetPlayer()->CanTakeQuest(nextQuest, false))
                            {
                                if (nextQuest->IsAutoAccept() && GetPlayer()->CanAddQuest(nextQuest, true))
                                    GetPlayer()->AddQuestAndCheckCompletion(nextQuest, object);

                                GetPlayer()->PlayerTalkClass->SendQuestGiverQuestDetails(nextQuest, guid, true);
                            }
                        }

                        *questgiver <<= Fire::QuestReward(questId);

                        if (questgiver->IsAIEnabled)
                            questgiver->AI()->sQuestReward(GetPlayer(), quest, reward);
                    }
                    break;
                }
                case TYPEID_GAMEOBJECT:
                {
                    GameObject* questGiver = object->ToGameObject();
                    if (!sScriptMgr->OnQuestReward(GetPlayer(), questGiver, quest, reward))
                    {
                        // Send next quest
                        if (Quest const* nextQuest = GetPlayer()->GetNextQuest(guid, quest))
                        {
                            // Only send the quest to the player if the conditions are met
                            if (GetPlayer()->CanTakeQuest(nextQuest, false))
                            {
                                if (nextQuest->IsAutoAccept() && GetPlayer()->CanAddQuest(nextQuest, true))
                                    GetPlayer()->AddQuestAndCheckCompletion(nextQuest, object);

                                GetPlayer()->PlayerTalkClass->SendQuestGiverQuestDetails(nextQuest, guid, true);
                            }
                        }
                        if (questGiver->AI())
                            questGiver->AI()->QuestReward(GetPlayer(), quest, reward);
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else
            GetPlayer()->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
    }
}

void WorldSession::HandleQuestgiverRequestRewardOpcode(WorldPacket& recvData)
{
    uint32 questId;
    ObjectGuid guid;
    recvData >> guid >> questId;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_REQUEST_REWARD npc = %s, quest = %u", guid.ToString().c_str(), questId);

    Object* object = ObjectAccessor::GetObjectByTypeMask(*GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
    if (!object || !object->hasInvolvedQuest(questId))
        return;

    // some kind of WPE protection
    if (!GetPlayer()->CanInteractWithQuestGiver(object))
        return;

    if (GetPlayer()->CanCompleteQuest(questId))
        GetPlayer()->CompleteQuest(questId);

    if (GetPlayer()->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
        return;

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
        GetPlayer()->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
}

void WorldSession::HandleQuestgiverCancel(WorldPacket& /*recvData*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_CANCEL");

    GetPlayer()->PlayerTalkClass->SendCloseGossip();
}

void WorldSession::HandleQuestLogSwapQuest(WorldPacket& recvData)
{
    uint8 slot1, slot2;
    recvData >> slot1 >> slot2;

    if (slot1 == slot2 || slot1 >= MAX_QUEST_LOG_SIZE || slot2 >= MAX_QUEST_LOG_SIZE)
        return;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTLOG_SWAP_QUEST slot 1 = %u, slot 2 = %u", slot1, slot2);

    GetPlayer()->SwapQuestSlot(slot1, slot2);
}

void WorldSession::HandleQuestLogRemoveQuest(WorldPacket& recvData)
{
    uint8 slot;
    recvData >> slot;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTLOG_REMOVE_QUEST slot = %u", slot);

    if (slot < MAX_QUEST_LOG_SIZE)
    {
        if (uint32 questId = GetPlayer()->GetQuestSlotQuestId(slot))
        {
            if (!GetPlayer()->TakeQuestSourceItem(questId, true))
                return;                                     // can't un-equip some items, reject quest cancel

            if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            {
                if (quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED))
                    GetPlayer()->RemoveTimedQuest(questId);

                if (quest->HasFlag(QUEST_FLAGS_FLAGS_PVP))
                {
                    GetPlayer()->pvpInfo.IsHostile = GetPlayer()->pvpInfo.IsInHostileArea || GetPlayer()->HasPvPForcingQuest();
                    GetPlayer()->UpdatePvPState();
                }
            }

            /*
            # 11407 = Bark for Drohn's Distillery!
            # 11408 = Bark for T'chali's Voodoo Brewery!
            # 11293 = Bark for the Thunderbrews!
            # 11294 = Bark for the Barleybrews!
            */
            if ((questId == 11407) || (questId == 11408) || (questId == 11294) || (questId == 11293) )
            {
                GetPlayer()->RemoveAura(43880); // SPELL_RAMSTEIN_SWIFT_WORK_RAM
                GetPlayer()->RemoveAura(43883); // SPELL_BREWFEST_RAM
            }

            GetPlayer()->TakeQuestSourceItem(questId, true); // remove quest src item from player
            GetPlayer()->RemoveActiveQuest(questId);
            GetPlayer()->RemoveTimedAchievement(ACHIEVEMENT_TIMED_TYPE_QUEST, questId);

            TC_LOG_INFO("network", "Player %u abandoned quest %u", GetPlayer()->GetGUID().GetCounter(), questId);

            if (sWorld->getBoolConfig(CONFIG_QUEST_ENABLE_QUEST_TRACKER)) // check if Quest Tracker is enabled
            {
                // prepare Quest Tracker datas
                PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_QUEST_TRACK_ABANDON_TIME);
                stmt->setUInt32(0, questId);
                stmt->setUInt32(1, GetPlayer()->GetGUID().GetCounter());

                // add to Quest Tracker
                CharacterDatabase.Execute(stmt);
            }
        }

        GetPlayer()->SetQuestSlot(slot, 0);

        GetPlayer()->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED, 1);
    }
}

void WorldSession::HandleQuestConfirmAccept(WorldPacket& recvData)
{
    uint32 questId;
    recvData >> questId;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUEST_CONFIRM_ACCEPT quest = %u", questId);

    if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
    {
        if (!quest->HasFlag(QUEST_FLAGS_PARTY_ACCEPT))
            return;

        Player* originalPlayer = ObjectAccessor::GetPlayer(*GetPlayer(), GetPlayer()->GetQuestSharer());
        if (!originalPlayer)
            return;

        if (!GetPlayer()->IsInSameRaidWith(originalPlayer))
            return;

        if (!originalPlayer->IsActiveQuest(questId))
            return;

        if (!GetPlayer()->CanTakeQuest(quest, true))
            return;

        if (GetPlayer()->CanAddQuest(quest, true))
        {
            GetPlayer()->AddQuestAndCheckCompletion(quest, NULL); // NULL, this prevent DB script from duplicate running

            if (quest->GetSrcSpell() > 0)
                GetPlayer()->CastSpell(GetPlayer(), quest->GetSrcSpell(), true);
        }
    }
}

void WorldSession::HandleQuestgiverCompleteQuest(WorldPacket& recvData)
{
    uint32 questId;
    ObjectGuid guid;

    recvData >> guid >> questId;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_COMPLETE_QUEST npc = %s, quest = %u", guid.ToString().c_str(), questId);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    Object* object = ObjectAccessor::GetObjectByTypeMask(*GetPlayer(), guid, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
    if (!object || !object->hasInvolvedQuest(questId))
        return;

    // some kind of WPE protection
    if (!GetPlayer()->CanInteractWithQuestGiver(object))
        return;

    if (!GetPlayer()->CanSeeStartQuest(quest) && GetPlayer()->GetQuestStatus(questId) == QUEST_STATUS_NONE)
    {
        TC_LOG_ERROR("network", "Possible hacking attempt: Player %s [guid: %u] tried to complete quest [entry: %u] without being in possession of the quest!",
                      GetPlayer()->GetName().c_str(), GetPlayer()->GetGUID().GetCounter(), questId);
        return;
    }

    if (Battleground* bg = GetPlayer()->GetBattleground())
        bg->HandleQuestComplete(questId, GetPlayer());

    if (GetPlayer()->GetQuestStatus(questId) != QUEST_STATUS_COMPLETE)
    {
        if (quest->IsRepeatable())
            GetPlayer()->PlayerTalkClass->SendQuestGiverRequestItems(quest, guid, GetPlayer()->CanCompleteRepeatableQuest(quest), false);
        else
            GetPlayer()->PlayerTalkClass->SendQuestGiverRequestItems(quest, guid, GetPlayer()->CanRewardQuest(quest, false), false);
    }
    else
    {
        if (quest->GetReqItemsCount())                  // some items required
            GetPlayer()->PlayerTalkClass->SendQuestGiverRequestItems(quest, guid, GetPlayer()->CanRewardQuest(quest, false), false);
        else                                            // no items required
            GetPlayer()->PlayerTalkClass->SendQuestGiverOfferReward(quest, guid, true);
    }
}

void WorldSession::HandleQuestgiverQuestAutoLaunch(WorldPacket& /*recvPacket*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_QUEST_AUTOLAUNCH");
}

void WorldSession::HandlePushQuestToParty(WorldPacket& recvPacket)
{
    uint32 questId;
    recvPacket >> questId;

    if (!GetPlayer()->CanShareQuest(questId))
        return;

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_PUSHQUESTTOPARTY questId = %u", questId);

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return;

    Player * const sender = GetPlayer();

    Group* group = sender->GetGroup();
    if (!group)
    {
        sender->SendPushToPartyResponse(sender, QUEST_PARTY_MSG_NOT_IN_PARTY);
        return;
    }

    for (GroupReference* itr = group->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player* receiver = itr->GetSource();

        if (!receiver || receiver == sender)
            continue;

        if (!receiver->IsInWorld())
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_BUSY);
            continue;
        }

        // receiver cant accept quest (server side) when dead or in different map
        if (receiver->FindMap() != sender->FindMap() || !receiver->IsAlive())
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_CANT_TAKE_QUEST);
            continue;
        }

        if (!receiver->SatisfyQuestStatus(quest, false))
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_HAVE_QUEST);
            continue;
        }

        if (receiver->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_FINISH_QUEST);
            continue;
        }

        if (!receiver->SatisfyQuestDay(quest, false))
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_NOT_ELIGIBLE_TODAY);
            continue;
        }

        if (!receiver->CanTakeQuest(quest, false))
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_CANT_TAKE_QUEST);
            continue;
        }

        if (!receiver->SatisfyQuestLog(false))
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_LOG_FULL);
            continue;
        }

        if (receiver->GetQuestSharer())
        {
            sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_BUSY);
            continue;
        }

        sender->SendPushToPartyResponse(receiver, QUEST_PARTY_MSG_SHARING_QUEST);

        if (quest->IsAutoAccept() && receiver->CanAddQuest(quest, true) && receiver->CanTakeQuest(quest, true))
            receiver->AddQuestAndCheckCompletion(quest, sender);

        if ((quest->IsAutoComplete() && quest->IsRepeatable() && !quest->IsDailyOrWeekly()) || quest->HasFlag(QUEST_FLAGS_AUTOCOMPLETE))
            receiver->PlayerTalkClass->SendQuestGiverRequestItems(quest, sender->GetGUID(), receiver->CanCompleteRepeatableQuest(quest), true);
        else
        {
            receiver->SetQuestSharer(sender->GetGUID());
            receiver->PlayerTalkClass->SendQuestGiverQuestDetails(quest, receiver->GetGUID(), true);
        }
    }
}

void WorldSession::HandleQuestPushResult(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    uint32 questId;
    uint8 msg;
    recvPacket >> guid >> questId >> msg;

    TC_LOG_DEBUG("network", "WORLD: Received MSG_QUEST_PUSH_RESULT");

    Player* player = GetPlayer();
    ObjectGuid sharerGuid = player->GetQuestSharer();

    if (!sharerGuid)
        return;

    if (sharerGuid == guid)
    {
        Player* sharer = ObjectAccessor::GetPlayer(*player, sharerGuid);
        if (sharer)
            sharer->SendPushToPartyResponse(player, msg);
    }
    player->SetQuestSharer(ObjectGuid::Empty);
}

void WorldSession::HandleQuestgiverStatusMultipleQuery(WorldPacket& /*recvPacket*/)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_QUESTGIVER_STATUS_MULTIPLE_QUERY");

    GetPlayer()->SendQuestGiverStatusMultiple();
}

void WorldSession::HandleQueryQuestsCompleted(WorldPacket & /*recvData*/)
{
    size_t average_rew_count = GetPlayer()->GetRewardedQuestCount();
    size_t exact_rew_count = 0;

    WorldPacket data(SMSG_QUERY_QUESTS_COMPLETED_RESPONSE, 4 + 4 * average_rew_count);
    data << uint32(average_rew_count); // set any count here as we don't know the exact number of completed quests yet

    const RewardedQuestSet &rewQuests = GetPlayer()->getRewardedQuests();
    for (RewardedQuestSet::const_iterator itr = rewQuests.begin(); itr != rewQuests.end(); ++itr)
    {
        const Quest* currentQuest = sObjectMgr->GetQuestTemplate(*itr);

        if (!currentQuest)
            continue;

        // if the quest is a repeating quest (daily, weekly, monthly, seasonal) and was not completed that day/week/.. 
        // do not add it to the package
        if ((currentQuest->IsDaily() && GetPlayer()->SatisfyQuestDay(currentQuest, false))
            || (currentQuest->IsWeekly() && GetPlayer()->SatisfyQuestWeek(currentQuest, false))
            || (currentQuest->IsMonthly() &&  GetPlayer()->SatisfyQuestMonth(currentQuest, false))
            || (currentQuest->IsSeasonal() && GetPlayer()->SatisfyQuestSeasonal(currentQuest, false)))
            continue;

        data << uint32(*itr);
        exact_rew_count++;
    }

    data.put(0, uint32(exact_rew_count)); // fix the count here

    SendPacket(std::move(data));
}
