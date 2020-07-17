#include "MailMgr.hpp"
#include "DatabaseEnv.h"
#include "Timer.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "PlayerCache.hpp"
#include "Language.h"
#include "LogManager.hpp"

MailMgr::Store MailMgr::storage{};
MailMgr::Mutex MailMgr::mutex;

MailMgr::PlayerMailEntry MailMgr::Get(uint32 mailId)
{
    ReadLock lock(mutex);
    const auto& index = storage.get<id_tag>();
    auto itr = index.find(mailId);
    if (itr != index.end())
        return PlayerMailEntry(&(*itr), std::move(lock));
    else
        return PlayerMailEntry();
}

void MailMgr::Init()
{
    TC_LOG_INFO("player.mail", "Initializing mail manager...");
    uint32 start = getMSTime();
    uint32 count = 0;

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAILS);
    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();

            Mail entry;
            entry.messageID = fields[0].GetUInt32();
            entry.messageType = fields[1].GetUInt8();
            entry.stationery = fields[2].GetUInt8();
            entry.mailTemplateId = fields[3].GetUInt16();
            entry.sender = fields[4].GetUInt32();
            entry.receiver = ObjectGuid(HighGuid::Player, fields[5].GetUInt32());
            entry.subject = fields[6].GetString();
            entry.body = fields[7].GetString();
            entry.expire_time = fields[8].GetUInt32();
            entry.deliver_time = fields[9].GetUInt32();
            entry.money = fields[10].GetUInt32();
            entry.COD = fields[11].GetUInt32();
            entry.checked = fields[12].GetUInt8();

            storage.insert(std::move(entry));
            ++count;
        } while (result->NextRow());
    }


    TC_LOG_INFO("player.mail", ">> Loaded %u player mails in %u ms", count, GetMSTimeDiffToNow(start));

    start = getMSTime();
    count = 0;

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_MAIL_ITEMS);
    if (PreparedQueryResult result = CharacterDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();

            ObjectGuid itemGuid = ObjectGuid(HighGuid::Item, fields[11].GetUInt32());
            uint32 itemTemplate = fields[12].GetUInt32();
            uint32 mailId = fields[14].GetUInt32();

            ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemTemplate);

            if (!proto)
            {
                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_MAIL_ITEM);
                stmt->setUInt32(0, itemGuid.GetCounter());
                trans->Append(stmt);

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                stmt->setUInt32(0, itemGuid.GetCounter());
                trans->Append(stmt);
                CharacterDatabase.CommitTransaction(trans);
                continue;
            }

            Item* item = NewItemOrBag(proto);

            if (!item->LoadFromDB(itemGuid.GetCounter(), ObjectGuid(HighGuid::Player, fields[13].GetUInt32()), fields, itemTemplate))
            {
                TC_LOG_ERROR("player.mail", "Item in mail (%u) doesn't exist !!!! - %s, deleted from mail", mailId, itemGuid.ToString().c_str());

                SQLTransaction trans = CharacterDatabase.BeginTransaction();
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM);
                stmt->setUInt32(0, itemGuid.GetCounter());
                trans->Append(stmt);

                item->FSetState(ITEM_REMOVED);
                item->SaveToDB(trans);                               // it also deletes item object !
                CharacterDatabase.CommitTransaction(trans);
                continue;
            }

            if (!Update(mailId, [item](Mail& mail) { mail.AddItem(item); }))
                continue;

            count++;
        } while (result->NextRow());
    }

    TC_LOG_INFO("player.mail", ">> Loaded %u player mail items in %u ms", count, GetMSTimeDiffToNow(start));
}

bool MailMgr::Update(uint32 mailId, std::function<void(Mail&)> updater)
{
    ReadLock lock(mutex);
    auto& index = storage.get<id_tag>();
    auto itr = index.find(mailId);
    if (itr != index.end())
    {
        index.modify(itr, updater);
        return true;
    }
    else
        return false;
}

bool MailMgr::MarkAsRead(uint32 mailId, ObjectGuid playerGuid)
{
    bool isOwner;
    bool mailFound = MailMgr::Update(mailId, [&](Mail& m)
    {
        if (m.receiver == playerGuid)
        {
            m.checked = m.checked | MAIL_CHECK_MASK_READ;
            isOwner = true;
        }
        else
            isOwner = false;
    });
    if (mailFound && isOwner)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_MARK_AS_READ);
        stmt->setUInt32(0, mailId);
        CharacterDatabase.Execute(stmt);
    }
    return mailFound && isOwner;
}

bool MailMgr::MarkAsCopied(uint32 mailId, ObjectGuid playerGuid)
{
    bool isOwner;
    bool mailFound = MailMgr::Update(mailId, [&](Mail& m)
    {
        if (m.receiver == playerGuid)
        {
            m.checked = m.checked | MAIL_CHECK_MASK_COPIED;
            isOwner = true;
        }
        else
            isOwner = false;
    });
    if (mailFound && isOwner)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_MARK_AS_COPIED);
        stmt->setUInt32(0, mailId);
        CharacterDatabase.Execute(stmt);
    }
    return mailFound && isOwner;
}

void MailMgr::Insert(Mail mail)
{
    WriteLock lock(mutex);
    auto& index = storage.get<id_tag>();
    auto itr = index.find(mail.messageID);
    if (itr != index.end())
    {
        const auto assign = [&mail](Mail& entry) { entry = std::move(mail); };
        index.modify(itr, assign);
    }
    else
        storage.insert(std::move(mail));
}

uint8 MailMgr::GetMailCount(ObjectGuid player)
{
    ReadLock lock(mutex);
    auto& index = storage.get<reciever_tag>();
    auto itr = index.find(player);
    uint8 count = 0;
    while (itr != index.end())
    {
        count++;
        itr++;
        if (itr == index.end() || itr->receiver != player)
            break;
    }
    return count;
}

bool MailMgr::Delete(uint32 mailId)
{
    WriteLock lock(mutex);
    auto& index = storage.get<id_tag>();
    auto itr = index.find(mailId);
    if (itr != index.end())
    {
        Delete(itr, nullptr);
        return true;
    }
    else
        return false;
}

template<typename iterator>
iterator MailMgr::Delete(iterator mailItr, SQLTransaction trans)
{
    bool commit = false;

    if (!trans)
    {
        trans = CharacterDatabase.BeginTransaction();
        commit = true;
    }

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
    stmt->setUInt32(0, mailItr->messageID);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM_BY_ID);
    stmt->setUInt32(0, mailItr->messageID);
    trans->Append(stmt);

    const std::vector<Item*>& items = mailItr->items;

    for (auto item : items)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
        stmt->setUInt32(0, item->GetGUID().GetCounter());
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_BOP_TRADE);
        stmt->setUInt32(0, item->GetGUID().GetCounter());
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_REFUND_INSTANCE);
        stmt->setUInt32(0, item->GetGUID().GetCounter());
        trans->Append(stmt);

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_MAIL_ITEM);
        stmt->setUInt32(0, item->GetGUID().GetCounter());
        trans->Append(stmt);
    }

    if (commit)
        CharacterDatabase.CommitTransaction(trans);

    return storage.erase(mailItr);
}

bool MailMgr::ReturnToSender(uint32 mailId, ObjectGuid playerGuid, uint32 accountId)
{
    std::unique_ptr<WriteLock> lock = std::make_unique<WriteLock>(mutex);
    auto& index = storage.get<id_tag>();
    auto itr = index.find(mailId);
    if (itr == index.end())
        return false;

    ReturnToSender(itr, playerGuid, accountId, std::move(lock));
    return true;
}

MailMgr::Store::iterator MailMgr::ReturnToSender(MailMgr::Store::iterator itr, ObjectGuid playerGuid, uint32 accountId, std::unique_ptr<WriteLock> lock)
{
    if (!lock)
        lock = std::make_unique<WriteLock>(mutex);
    auto& index = storage.get<id_tag>();

    const Mail &m = (*itr);
    if ((playerGuid && m.receiver != playerGuid) || m.deliver_time > time(NULL))
        return index.end();

    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    // only return mail if the player exists (and delete if not existing)
    if (m.messageType == MAIL_NORMAL && m.sender)
    {
        MailDraft draft(m.subject, m.body);
        if (m.mailTemplateId)
            draft = MailDraft(m.mailTemplateId, false);     // items already included

        if (m.HasItems())
        {
            for (auto itr2 = m.items.begin(); itr2 != m.items.end(); ++itr2)
                draft.AddItem(*itr2);
        }
        draft.AddMoney(m.money);

        ObjectGuid oldReceiver = m.receiver;
        uint32 oldSender = m.sender;

        // remove items from old mail
        index.modify(itr, [](Mail& m) { m.items.clear(); });
        itr = Delete(itr, trans);

        lock->unlock();
        draft.SendReturnToSender(accountId, oldReceiver, ObjectGuid(HighGuid::Player, oldSender), trans);
    }
    else
        itr = Delete(itr, trans);

    CharacterDatabase.CommitTransaction(trans);

    return itr;
}

bool MailMgr::TakeItem(uint32 mailId, ObjectGuid itemId, Player& player)
{
    ReadLock lock(mutex);
    auto& index = storage.get<id_tag>();
    auto m = index.find(mailId);
    if (m == index.end() || m->deliver_time > time(NULL))
    {
        player.SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return false;
    }

    auto itr = std::find_if(m->items.begin(), m->items.end(), [itemId](Item* item) { return item->GetGUID() == itemId; });

    if (itr == m->items.end())
    {
        player.SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return false;
    }

    // prevent cheating with skip client money check
    if (!player.HasEnoughMoney(m->COD))
    {
        player.SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_NOT_ENOUGH_MONEY);
        return false;
    }

    ObjectGuid sender_guid = ObjectGuid(HighGuid::Player, m->sender);
    std::string subject = m->subject;
    uint32 cod = m->COD;

    Item* it = *itr;

    ItemPosCountVec dest;
    uint8 msg = player.CanStoreItem(NULL_BAG, NULL_SLOT, dest, it, false);
    if (msg == EQUIP_ERR_OK)
    {
        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        index.modify(m, [itemId](Mail& mail) { mail.COD = 0; mail.RemoveItem(itemId); });

        // RG-Custom ItemLogging
        RG_LOG<ItemLogModule>(it, RG::Logging::ItemLogType::TAKE_MAIL, it->GetCount(), player.GetGUID().GetCounter(), m->sender);

        uint32 count = it->GetCount();                      // save counts before store and possible merge with deleting
        it->SetState(ITEM_UNCHANGED);                       // need to set this state, otherwise item cannot be removed later, if neccessary
        player.MoveItemToInventory(dest, it, true);

        player.SaveInventoryAndGoldToDB(trans);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM);
        stmt->setUInt32(0, itemId.GetCounter());
        trans->Append(stmt);

        lock.unlock();

        if (cod > 0)                                        //if there is COD, take COD money from player and send them to sender by mail
        {
            Player* receiver = ObjectAccessor::FindConnectedPlayer(sender_guid);

            uint32 sender_accId = 0;

            if (player.GetSession()->HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE))
            {
                std::string sender_name;
                if (receiver)
                {
                    sender_accId = receiver->GetSession()->GetAccountId();
                    sender_name = receiver->GetName();
                }
                else
                {
                    if (auto info = player::PlayerCache::Get(sender_guid))
                    {
                        sender_accId = info.account;
                        sender_name = info.name;
                    }
                    else
                    {
                        sender_accId = 0;
                        sender_name = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
                    }
                }
                sLog->outCommand(player.GetSession()->GetAccountId(), "GM %s (Account: %u) receiver mail item: %s (Entry: %u Count: %u) and send COD money: %u to player: %s (Account: %u)",
                    player.GetName().c_str(), player.GetSession()->GetAccountId(), it->GetTemplate()->Name1.c_str(), it->GetEntry(), it->GetCount(), cod, sender_name.c_str(), sender_accId);
            }
            else if (!receiver)
                sender_accId = player::PlayerCache::GetAccountId(sender_guid);

            // check player existence
            if (receiver || sender_accId)
            {
                MailDraft(subject, "")
                    .AddMoney(cod)
                    .SendMailTo(trans, MailReceiver(receiver, sender_guid), MailSender(MAIL_NORMAL, player.GetGUID().GetCounter()), MAIL_CHECK_MASK_COD_PAYMENT);
            }

            player.ModifyMoney(-int32(cod), ::RG::Logging::MoneyLogType::MAIL_COD, sender_guid.GetCounter());
        }

        CharacterDatabase.CommitTransaction(trans);

        player.SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_OK, 0, itemId.GetCounter(), count);

        return true;
    }
    else
        player.SendMailResult(mailId, MAIL_ITEM_TAKEN, MAIL_ERR_EQUIP_ERROR, msg);

    return false;
}

bool MailMgr::TakeMoney(uint32 mailId, Player& player)
{
    ReadLock lock(mutex);
    auto& index = storage.get<id_tag>();
    auto m = index.find(mailId);
    if (m == index.end() || m->deliver_time > time(NULL))
    {
        player.SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_INTERNAL_ERROR);
        return false;
    }

    if (!player.ModifyMoney(m->money, ::RG::Logging::MoneyLogType::MAIL_TAKE, m->sender))
    {
        player.SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_ERR_EQUIP_ERROR, EQUIP_ERR_TOO_MUCH_GOLD);
        return false;
    }

    index.modify(m, [](Mail& mail) { mail.money = 0; });

    player.SendMailResult(mailId, MAIL_MONEY_TAKEN, MAIL_OK);

    // save money and mail to prevent cheating
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    player.SaveGoldToDB(trans);
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_TAKE_MONEY);
    stmt->setUInt32(0, mailId);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    return true;
}

void MailMgr::ReturnOrDeleteOldMails()
{
    uint32 oldMSTime = getMSTime();

    time_t curTime = time(NULL);
    tm lt;
    localtime_r(&curTime, &lt);
    uint64 basetime(curTime);
    TC_LOG_INFO("misc", "Returning mails current time: hour: %d, minute: %d, second: %d ", lt.tm_hour, lt.tm_min, lt.tm_sec);

    for (auto itr = storage.begin(); itr != storage.end();)
    {
        if (itr->expire_time < curTime)
        {
            if (itr->messageType != MAIL_NORMAL || (itr->checked & (MAIL_CHECK_MASK_COD_PAYMENT | MAIL_CHECK_MASK_RETURNED)))
                itr = Delete(itr);
            else
                itr = ReturnToSender(itr, ObjectGuid::Empty, 0);
        }
        else
            ++itr;
    }
}

void MailMgr::GetPlayerMailData(ObjectGuid playerGuid, uint8 & unreadMailCount, time_t & mailDeliveryTime)
{
    // calculate next delivery time (min. from non-delivered mails)
    // and recalculate unReadMail
    time_t cTime = time(NULL);

    mailDeliveryTime = 0;
    unreadMailCount = 0;

    auto& index = storage.get<reciever_tag>();
    auto itr = index.find(playerGuid);
    uint8 count = 0;
    while (itr != index.end())
    {
        if (itr->deliver_time > cTime)
        {
            if (!mailDeliveryTime || mailDeliveryTime > itr->deliver_time)
                mailDeliveryTime = itr->deliver_time;
        }
        else if ((itr->checked & MAIL_CHECK_MASK_READ) == 0)
            ++unreadMailCount;
        itr++;
        if (itr == index.end() || itr->receiver != playerGuid)
            break;
    }
}
