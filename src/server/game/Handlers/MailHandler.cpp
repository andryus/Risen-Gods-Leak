/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#include "DatabaseEnv.h"
#include "Mail.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Language.h"
#include "DBCStores.h"
#include "Item.h"
#include "AccountMgr.h"
#include "RG/Logging/LogManager.hpp"
#include "Entities/Player/PlayerCache.hpp"
#include "MailMgr.hpp"

bool WorldSession::CanOpenMailBox(ObjectGuid guid)
{
    if (guid == GetPlayer()->GetGUID())
    {
        if (!HasPermission(rbac::RBAC_PERM_COMMAND_MAILBOX))
        {
            TC_LOG_WARN("cheat", "%s attempt open mailbox in cheating way.", GetPlayer()->GetName().c_str());
            return false;
        }
    }
    else if (guid.IsGameObject())
    {
        if (!GetPlayer()->GetGameObjectIfCanInteractWith(guid, GAMEOBJECT_TYPE_MAILBOX))
            return false;
    }
    else if (guid.IsAnyTypeCreature())
    {
        if (!GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_MAILBOX))
            return false;
    }
    else
        return false;

    return true;
}

void WorldSession::HandleSendMail(WorldPacket& recvData)
{
    ObjectGuid mailbox, unk3;
    std::string receiverName, subject, body;
    uint32 stationery, package, money, COD;
    uint8 unk4;
    uint8 items_count;
    recvData >> mailbox >> receiverName >> subject >> body
             >> stationery                                 // stationery?
             >> package                                    // 0x00000000
             >> items_count;                               // attached items count

    if (items_count > MAX_MAIL_ITEMS)                      // client limit
    {
        GetPlayer()->SendMailResult(0, MAIL_SEND, MAIL_ERR_TOO_MANY_ATTACHMENTS);
        recvData.rfinish();                   // set to end to avoid warnings spam
        return;
    }

    ObjectGuid itemGUIDs[MAX_MAIL_ITEMS];

    for (uint8 i = 0; i < items_count; ++i)
    {
        recvData.read_skip<uint8>();                       // item slot in mail, not used
        recvData >> itemGUIDs[i];
    }

    recvData >> money >> COD;                              // money and cod
    recvData >> unk3;                                      // const 0
    recvData >> unk4;                                      // const 0

    // packet read complete, now do check

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    if (receiverName.empty())
        return;

    Player* player = GetPlayer();

    if (player->getLevel() < sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_MAIL_SENDER_REQ), sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ));
        return;
    }

    ObjectGuid receiverGuid;
    if (normalizePlayerName(receiverName))
        receiverGuid = player::PlayerCache::GetGUID(receiverName);

    if (!receiverGuid)
    {
        TC_LOG_INFO("network", "Player %u is sending mail to %s (GUID: not existed!) with subject %s "
            "and body %s includes %u items, %u copper and %u COD copper with unk1 = %u, unk2 = %u",
            player->GetGUID().GetCounter(), receiverName.c_str(), subject.c_str(), body.c_str(),
            items_count, money, COD, stationery, package);
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_NOT_FOUND);
        return;
    }

    TC_LOG_INFO("network", "Player %u is sending mail to %s (%s) with subject %s and body %s "
        "includes %u items, %u copper and %u COD copper with unk1 = %u, unk2 = %u",
        player->GetGUID().GetCounter(), receiverName.c_str(), receiverGuid.ToString().c_str(), subject.c_str(),
        body.c_str(), items_count, money, COD, stationery, package);

    if (player->GetGUID() == receiverGuid)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANNOT_SEND_TO_SELF);
        return;
    }

    uint32 cost = items_count ? 30 * items_count : 30;  // price hardcoded in client

    uint32 reqmoney = cost + money;

    // Check for overflow
    if (reqmoney < money)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    if (!player->HasEnoughMoney(reqmoney) && !player->IsGameMaster())
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_ENOUGH_MONEY);
        return;
    }

    Player* receiver = ObjectAccessor::FindConnectedPlayer(receiverGuid);

    uint32 receiverTeam = 0;
    uint8 mailsCount = 0;                                  //do not allow to send to one player more than 100 mails
    uint8 receiverLevel = 0;
    uint32 receiverAccountId = 0;

    if (receiver)
    {
        receiverTeam = receiver->GetOTeam();
        receiverLevel = receiver->getLevel();
        receiverAccountId = receiver->GetSession()->GetAccountId();
    }
    else
    {
        receiverTeam = player::PlayerCache::GetTeam(receiverGuid);
        receiverLevel = player::PlayerCache::GetLevel(receiverGuid);
        receiverAccountId = player::PlayerCache::GetAccountId(receiverGuid);
    }

    mailsCount = MailMgr::GetMailCount(receiverGuid);

    // do not allow to have more than 100 mails in mailbox.. mails count is in opcode uint8!!! - so max can be 255..
    if (mailsCount > 100)
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_RECIPIENT_CAP_REACHED);
        return;
    }

    // test the receiver's Faction... or all items are account bound
    bool accountBound = items_count ? true : false;
    for (uint8 i = 0; i < items_count; ++i)
    {
        if (Item* item = player->GetItemByGuid(itemGUIDs[i]))
        {
            ItemTemplate const* itemProto = item->GetTemplate();
            if (!itemProto || !(itemProto->Flags & ITEM_PROTO_FLAG_BIND_TO_ACCOUNT))
            {
                accountBound = false;
                break;
            }
        }
    }

    if (((accountBound && (money > 0 || COD > 0)) || (!accountBound)) && player->GetOTeam() != receiverTeam && !HasPermission(rbac::RBAC_PERM_TWO_SIDE_INTERACTION_MAIL))
    {
        player->SendMailResult(0, MAIL_SEND, MAIL_ERR_NOT_YOUR_TEAM);
        return;
    }

    if (receiverLevel < sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ))
    {
        SendNotification(GetTrinityString(LANG_MAIL_RECEIVER_REQ), sWorld->getIntConfig(CONFIG_MAIL_LEVEL_REQ));
        return;
    }

    Item* items[MAX_MAIL_ITEMS];

    for (uint8 i = 0; i < items_count; ++i)
    {
        if (!itemGUIDs[i])
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        Item* item = player->GetItemByGuid(itemGUIDs[i]);

        // prevent sending bag with items (cheat: can be placed in bag after adding equipped empty bag to mail)
        if (!item)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_MAIL_ATTACHMENT_INVALID);
            return;
        }

        if (!item->IsBoundAccountWide() && !item->CanBeTraded(true))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (item->IsBoundAccountWide() && item->IsSoulBound() && player->GetSession()->GetAccountId() != receiverAccountId)
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_ARTEFACTS_ONLY_FOR_OWN_CHARACTERS);
            return;
        }

        if (item->GetTemplate()->Flags & ITEM_PROTO_FLAG_CONJURED || item->GetUInt32Value(ITEM_FIELD_DURATION))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_MAIL_BOUND_ITEM);
            return;
        }

        if (COD && item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_CANT_SEND_WRAPPED_COD);
            return;
        }

        if (item->IsNotEmptyBag())
        {
            player->SendMailResult(0, MAIL_SEND, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_CAN_ONLY_DO_WITH_EMPTY_BAGS);
            return;
        }

        items[i] = item;
    }

    player->SendMailResult(0, MAIL_SEND, MAIL_OK);

    player->ModifyMoney(-int32(reqmoney), ::RG::Logging::MoneyLogType::MAIL, receiverGuid.GetCounter());
    player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL, cost);

    bool needItemDelay = false;

    MailDraft draft(subject, body);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    if (items_count > 0 || money > 0)
    {
        bool log = HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE);
        if (items_count > 0)
        {
            for (uint8 i = 0; i < items_count; ++i)
            {
                Item* item = items[i];
                if (log)
                {
                    sLog->outCommand(GetAccountId(), "GM %s (GUID: %u) (Account: %u) mail item: %s (Entry: %u Count: %u) "
                        "to: %s (%s) (Account: %u)", GetPlayerName().c_str(), GetGuidLow(), GetAccountId(),
                        item->GetTemplate()->Name1.c_str(), item->GetEntry(), item->GetCount(),
                        receiverName.c_str(), receiverGuid.ToString().c_str(), receiverAccountId);
                }

                item->ClearEnchantment(TEMP_ENCHANTMENT_SLOT);   // Remove temp. enchantments (e.g. Rogue->Poison, Shaman->Earthliving Weapon, etc.)
                item->SetNotRefundable(GetPlayer());             // makes the item no longer refundable
                player->MoveItemFromInventory(items[i]->GetBagSlot(), item->GetSlot(), true);

                item->DeleteFromInventoryDB(trans);              // deletes item from character's inventory
                item->SetOwnerGUID(receiverGuid);
                item->SaveToDB(trans);                           // recursive and not have transaction guard into self, item not in inventory and can be save standalone

                draft.AddItem(item);
            }

            // if item send to character at another account, then apply item delivery delay
            needItemDelay = player->GetSession()->GetAccountId() != receiverAccountId;
        }

        if (log && money > 0)
        {
            sLog->outCommand(GetAccountId(), "GM %s (GUID: %u) (Account: %u) mail money: %u to: %s (%s) (Account: %u)",
                GetPlayerName().c_str(), GetGuidLow(), GetAccountId(), money, receiverName.c_str(), receiverGuid.ToString().c_str(), receiverAccountId);
        }
    }

    // If theres is an item, there is a one hour delivery delay if sent to another account's character. Not for Game Masters
    uint32 deliver_delay = (needItemDelay && !player->IsGameMaster()) ? sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY) : 0;

    // don't ask for COD if there are no items
    if (items_count == 0)
        COD = 0;

    // will delete item or place to receiver mail list
    draft
        .AddMoney(money)
        .AddCOD(COD)
        .SendMailTo(trans, MailReceiver(receiver, receiverGuid), MailSender(player), body.empty() ? MAIL_CHECK_MASK_COPIED : MAIL_CHECK_MASK_HAS_BODY, deliver_delay);

    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
}

//called when mail is read
void WorldSession::HandleMailMarkAsRead(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;
    recvData >> mailbox;
    recvData >> mailId;

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    MailMgr::MarkAsRead(mailId, GetPlayer()->GetGUID());
    GetPlayer()->UpdateNextMailTimeAndUnreads();
}

//called when client deletes mail
void WorldSession::HandleMailDelete(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;
    recvData >> mailbox;
    recvData >> mailId;
    recvData.read_skip<uint32>();                          // mailTemplateId

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    bool success = true;
    {
        auto m = MailMgr::Get(mailId);
        if (m && m->receiver == GetPlayer()->GetGUID())
        {
            // delete shouldn't show up for COD mails
            if (m->COD && !m->items.empty())
                success = false;
        }
        else
            success = false;
    }

    if (success)
        success = MailMgr::Delete(mailId);

    GetPlayer()->SendMailResult(mailId, MAIL_DELETED, success ? MAIL_OK : MAIL_ERR_INTERNAL_ERROR);
}

void WorldSession::HandleMailReturnToSender(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;
    recvData >> mailbox;
    recvData >> mailId;
    recvData.read_skip<uint64>();                          // original sender GUID for return to, not used

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    bool success = MailMgr::ReturnToSender(mailId, GetPlayer()->GetGUID(), GetAccountId());

    GetPlayer()->SendMailResult(mailId, MAIL_RETURNED_TO_SENDER, success ? MAIL_OK : MAIL_ERR_INTERNAL_ERROR);
}

//called when player takes item attached in mail
void WorldSession::HandleMailTakeItem(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;
    uint32 itemId;
    recvData >> mailbox;
    recvData >> mailId;
    recvData >> itemId;                                    // item guid low

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    MailMgr::TakeItem(mailId, ObjectGuid(HighGuid::Item, itemId), *GetPlayer());
}

void WorldSession::HandleMailTakeMoney(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;
    recvData >> mailbox;
    recvData >> mailId;

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    MailMgr::TakeMoney(mailId, *GetPlayer());
}

//called when player lists his received mails
void WorldSession::HandleGetMailList(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    recvData >> mailbox;

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    // client can't work with packets > max int16 value
    const uint32 maxPacketSize = 32767;

    uint32 mailsCount = 0;                                 // real send to client mails amount
    uint32 realCount  = 0;                                 // real mails amount

    WorldPacket data(SMSG_MAIL_LIST_RESULT, (200));         // guess size
    data << uint32(0);                                      // real mail's count
    data << uint8(0);                                       // mail's count
    time_t cur_time = time(NULL);

    for (auto itr = MailMgr::GetPlayerMailBegin(GetPlayer()->GetGUID()); itr != MailMgr::GetPlayerMailEnd(); ++itr)
    {
        // Only first 50 mails are displayed
        if (mailsCount >= 50)
        {
            realCount += 1;
            continue;
        }

        // skip deleted or not delivered (deliver delay not expired) mails
        if (cur_time < (*itr)->deliver_time)
            continue;

        uint8 item_count = (*itr)->items.size();            // max count is MAX_MAIL_ITEMS (12)

        size_t next_mail_size = 2+4+1+((*itr)->messageType == MAIL_NORMAL ? 8 : 4)+4*8+((*itr)->subject.size()+1)+((*itr)->body.size()+1)+1+item_count*(1+4+4+MAX_INSPECTED_ENCHANTMENT_SLOT*3*4+4+4+4+4+4+4+1);

        if (data.wpos()+next_mail_size > maxPacketSize)
        {
            realCount += 1;
            continue;
        }

        data << uint16(next_mail_size);                    // Message size
        data << uint32((*itr)->messageID);                 // Message ID
        data << uint8((*itr)->messageType);                // Message Type

        switch ((*itr)->messageType)
        {
            case MAIL_NORMAL:                               // sender guid
                data << uint64(ObjectGuid(HighGuid::Player, (*itr)->sender).GetRawValue());
                break;
            case MAIL_CREATURE:
            case MAIL_GAMEOBJECT:
            case MAIL_AUCTION:
            case MAIL_CALENDAR:
                data << uint32((*itr)->sender);            // creature/gameobject entry, auction id, calendar event id?
                break;
        }

        data << uint32((*itr)->COD);                         // COD
        data << uint32(0);                                   // package (Package.dbc)
        data << uint32((*itr)->stationery);                  // stationery (Stationery.dbc)
        data << uint32((*itr)->money);                       // Gold
        data << uint32((*itr)->checked);                     // flags
        data << float(float((*itr)->expire_time-time(NULL))/DAY); // Time
        data << uint32((*itr)->mailTemplateId);              // mail template (MailTemplate.dbc)
        data << (*itr)->subject;                             // Subject string - once 00, when mail type = 3, max 256
        data << (*itr)->body;                                // message? max 8000
        data << uint8(item_count);                           // client limit is 0x10

        for (uint8 i = 0; i < item_count; ++i)
        {
            Item* item = (*itr)->items[i];
            // item index (0-6?)
            data << uint8(i);
            // item guid low?
            data << uint32((item ? item->GetGUID().GetCounter() : 0));
            // entry
            data << uint32((item ? item->GetEntry() : 0));
            for (uint8 j = 0; j < MAX_INSPECTED_ENCHANTMENT_SLOT; ++j)
            {
                data << uint32((item ? item->GetEnchantmentId((EnchantmentSlot)j) : 0));
                data << uint32((item ? item->GetEnchantmentDuration((EnchantmentSlot)j) : 0));
                data << uint32((item ? item->GetEnchantmentCharges((EnchantmentSlot)j) : 0));
            }
            // can be negative
            data << int32((item ? item->GetItemRandomPropertyId() : 0));
            // unk
            data << uint32((item ? item->GetItemSuffixFactor() : 0));
            // stack count
            data << uint32((item ? item->GetCount() : 0));
            // charges
            data << uint32((item ? item->GetSpellCharges() : 0));
            // durability
            data << uint32((item ? item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY) : 0));
            // durability
            data << uint32((item ? item->GetUInt32Value(ITEM_FIELD_DURABILITY) : 0));
            // unknown wotlk
            data << uint8(0);
        }

        ++realCount;
        ++mailsCount;
    }

    data.put<uint32>(0, realCount);                         // this will display warning about undelivered mail to player if realCount > mailsCount
    data.put<uint8>(4, mailsCount);                        // set real send mails to client
    SendPacket(std::move(data));

    // recalculate m_nextMailDelivereTime and unReadMails
    GetPlayer()->UpdateNextMailTimeAndUnreads();
}

//used when player copies mail body to his inventory
void WorldSession::HandleMailCreateTextItem(WorldPacket& recvData)
{
    ObjectGuid mailbox;
    uint32 mailId;

    recvData >> mailbox;
    recvData >> mailId;

    if (!CanOpenMailBox(mailbox))
        if (!GetPlayer()->GetNPCIfCanInteractWith(mailbox, UNIT_NPC_FLAG_MAILBOX))
            return;

    Player* player = GetPlayer();

    Item* bodyItem;

    { // enter read lock 
    auto m = MailMgr::Get(mailId);
    if (!m || (m->checked & MAIL_CHECK_MASK_COPIED) || m->receiver != GetPlayer()->GetGUID() || (m->body.empty() && !m->mailTemplateId) || m->deliver_time > time(NULL))
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
        return;
    }

    bodyItem = new Item;                              // This is not bag and then can be used new Item.
    if (!bodyItem->Create(sObjectMgr->GenerateLowGuid(HighGuid::Item), MAIL_BODY_ITEM_TEMPLATE, player))
    {
        delete bodyItem;
        return;
    }

    // in mail template case we need create new item text
    if (m->mailTemplateId)
    {
        MailTemplateEntry const* mailTemplateEntry = sMailTemplateStore.LookupEntry(m->mailTemplateId);
        if (!mailTemplateEntry)
        {
            player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_INTERNAL_ERROR);
            return;
        }

        bodyItem->SetText(mailTemplateEntry->content[GetSessionDbcLocale()]);
    }
    else
        bodyItem->SetText(m->body);

    if (m->messageType == MAIL_NORMAL)
        bodyItem->SetGuidValue(ITEM_FIELD_CREATOR, ObjectGuid(HighGuid::Player, m->sender));

    bodyItem->SetFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_MAIL_TEXT_MASK);

    TC_LOG_INFO("network", "HandleMailCreateTextItem mailid=%u", mailId);
    } // unlock 

    ItemPosCountVec dest;
    uint8 msg = GetPlayer()->CanStoreItem(NULL_BAG, NULL_SLOT, dest, bodyItem, false);
    if (msg == EQUIP_ERR_OK)
    {
        MailMgr::MarkAsCopied(mailId, GetPlayer()->GetGUID());

        player->StoreItem(dest, bodyItem, true);
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_OK);
    }
    else
    {
        player->SendMailResult(mailId, MAIL_MADE_PERMANENT, MAIL_ERR_EQUIP_ERROR, msg);
        delete bodyItem;
    }
}

/// @todo Fix me! ... this void has probably bad condition, but good data are sent
void WorldSession::HandleQueryNextMailTime(WorldPacket & /*recvData*/)
{
    WorldPacket data(MSG_QUERY_NEXT_MAIL_TIME, 8);

    // if (!GetPlayer()->m_mailsLoaded)
    //     GetPlayer()->_LoadMail();

    if (GetPlayer()->unReadMails > 0)
    {
        data << float(0);                                  // float
        data << uint32(0);                                 // count

        uint32 count = 0;
        time_t now = time(NULL);
        std::set<uint32> sentSenders;
        for (auto itr = MailMgr::GetPlayerMailBegin(GetPlayer()->GetGUID()); itr != MailMgr::GetPlayerMailEnd(); ++itr)
        {
            const Mail* m = (*itr);
            // must be not checked yet
            if (m->checked & MAIL_CHECK_MASK_READ)
                continue;

            // and already delivered
            if (now < m->deliver_time)
                continue;

            // only send each mail sender once
            if (sentSenders.count(m->sender))
                continue;

            data << uint64(m->messageType == MAIL_NORMAL ? m->sender : 0);  // player guid
            data << uint32(m->messageType != MAIL_NORMAL ? m->sender : 0);  // non-player entries
            data << uint32(m->messageType);
            data << uint32(m->stationery);
            data << float(m->deliver_time - now);

            sentSenders.insert(m->sender);
            ++count;
            if (count == 2)                                  // do not display more than 2 mails
                break;
        }

        data.put<uint32>(4, count);
    }
    else
    {
        data << float(-DAY);
        data << uint32(0);
    }

    SendPacket(std::move(data));
}
