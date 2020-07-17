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

#include "AuctionHouseMgr.hpp"

#include "Common.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "ScriptMgr.h"
#include "AccountMgr.h"
#include "Item.h"
#include "Language.h"
#include "Log.h"
#include "GameTime.h"

#include "PlayerCache.hpp"
#include "AuctionHouseObject.hpp"
#include "RG/Logging/LogManager.hpp"
#include "Opcodes.h"

constexpr uint32 AH_MINIMUM_DEPOSIT = 100;

constexpr uint8 NEUTRAL_AUCTIONS = 0;
constexpr uint8 ALLIANCE_AUCTIONS = 1;
constexpr uint8 HORDE_AUCTIONS = 2;

AuctionHouseMgr::AuctionHouseMgr() 
{
    for (auto& auction : auctions)
        auction = std::make_unique<AuctionHouseObject>();
}

AuctionHouseMgr::~AuctionHouseMgr()
{
    for (auto itr : mAitems)
        delete itr.second;
}

AuctionHouseMgr* AuctionHouseMgr::instance()
{
    static AuctionHouseMgr instance;
    return &instance;
}

AuctionHouseObject* AuctionHouseMgr::GetAuctionsMap(uint32 factionTemplateId)
{
    if (sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
        return auctions[NEUTRAL_AUCTIONS].get();

    // teams have linked auction houses
    FactionTemplateEntry const* uEntry = sFactionTemplateStore.LookupEntry(factionTemplateId);
    if (!uEntry)
        return auctions[NEUTRAL_AUCTIONS].get();
    else if (uEntry->ourMask & FACTION_MASK_ALLIANCE)
        return auctions[ALLIANCE_AUCTIONS].get();
    else if (uEntry->ourMask & FACTION_MASK_HORDE)
        return auctions[HORDE_AUCTIONS].get();
    else
        return auctions[NEUTRAL_AUCTIONS].get();
}

uint32 AuctionHouseMgr::GetAuctionDeposit(AuctionHouseEntry const* entry, uint32 time, Item* pItem, uint32 count)
{
    uint32 MSV = pItem->GetTemplate()->SellPrice;

    if (MSV <= 0)
        return AH_MINIMUM_DEPOSIT;

    float multiplier = CalculatePct(float(entry->depositPercent), 3);
    uint32 timeHr = (((time / 60) / 60) / 12);
    uint32 deposit = uint32(((multiplier * MSV * count / 3) * timeHr * 3) * sWorld->getRate(RATE_AUCTION_DEPOSIT));

    TC_LOG_DEBUG("auctionHouse", "MSV:        %u", MSV);
    TC_LOG_DEBUG("auctionHouse", "Items:      %u", count);
    TC_LOG_DEBUG("auctionHouse", "Multiplier: %f", multiplier);
    TC_LOG_DEBUG("auctionHouse", "Deposit:    %u", deposit);

    if (deposit < AH_MINIMUM_DEPOSIT)
        return AH_MINIMUM_DEPOSIT;
    else
        return deposit;
}

//does not clear ram
void AuctionHouseMgr::SendAuctionWonMail(AuctionEntry* auction, SQLTransaction& trans)
{
    Item* pItem = GetAItem(auction->itemGUIDLow);
    if (!pItem)
        return;

    uint32 bidderAccId = 0;
    Player* bidder = ObjectAccessor::FindConnectedPlayer(auction->bidder);
    // data for gm.log
    std::string bidderName;
    bool logGmTrade = false;

    if (bidder)
    {
        bidderAccId = bidder->GetSession()->GetAccountId();
        bidderName = bidder->GetName();
        logGmTrade = bidder->GetSession()->HasPermission(rbac::RBAC_PERM_LOG_GM_TRADE);
    }
    else
    {
        if (auto bidder_info = player::PlayerCache::Get(auction->bidder))
        {
            bidderAccId = bidder_info.account;
            bidderName  = bidder_info.name;
        }
        else
        {
            bidderAccId = 0;
            bidderName = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
        }
        logGmTrade = AccountMgr::HasPermission(bidderAccId, rbac::RBAC_PERM_LOG_GM_TRADE, realm.Id.Realm);
    }

    if (logGmTrade)
    {
        std::string ownerName;
        uint32 ownerAccount;
        if (auto owner_info = player::PlayerCache::Get(auction->owner))
        {
            ownerName = owner_info.name;
            ownerAccount = owner_info.account;
        }
        else
        {
            ownerName = sObjectMgr->GetTrinityStringForDBCLocale(LANG_UNKNOWN);
            ownerAccount = 0;
        }
        sLog->outCommand(bidderAccId, "GM %s (Account: %u) won item in auction: %s (Entry: %u Count: %u) and pay money: %u. Original owner %s (Account: %u)",
                         bidderName.c_str(), bidderAccId, pItem->GetTemplate()->Name1.c_str(), pItem->GetEntry(), pItem->GetCount(), auction->bid, ownerName.c_str(), ownerAccount);
    }

    // receiver exist
    if (bidder || bidderAccId)
    {
        // set owner to bidder (to prevent delete item with sender char deleting)
        // owner in `data` will set at mail receive and item extracting
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_OWNER);
        stmt->setUInt32(0, auction->bidder.GetCounter());
        stmt->setUInt32(1, pItem->GetGUID().GetCounter());
        trans->Append(stmt);

        if (bidder)
        {
            bidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, auction->bidder, 0, 0, auction->itemEntry);
            // FIXME: for offline player need also
            bidder->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS, 1);
        }

        MailDraft(auction->BuildAuctionMailSubject(MailAuctionAnswers::WON), AuctionEntry::BuildAuctionMailBody(auction->owner.GetCounter(), auction->bid, auction->buyout, 0, 0))
            .AddItem(pItem)
            .SendMailTo(trans, MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
    else
    {
        // bidder doesn't exist, delete the item
        sAuctionMgr->RemoveAItem(auction->itemGUIDLow, true);
    }
}

//call this method to send mail to auction owner, when auction is successful, it does not clear ram
void AuctionHouseMgr::SendAuctionSuccessfulMail(AuctionEntry* auction, SQLTransaction& trans)
{
    Player* owner = ObjectAccessor::FindConnectedPlayer(auction->owner);
    uint32 owner_accId = player::PlayerCache::GetAccountId(auction->owner);
    // owner exist
    if (owner || owner_accId)
    {
        uint32 profit = auction->bid + auction->deposit - auction->GetAuctionCut();

        //FIXME: what do if owner offline
        if (owner)
        {
            owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS, profit);
            owner->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD, auction->bid);
            //send auction owner notification, bidder must be current!
            owner->GetSession()->SendAuctionOwnerNotification(auction);
        }

        MailDraft(auction->BuildAuctionMailSubject(MailAuctionAnswers::SUCCESSFUL), AuctionEntry::BuildAuctionMailBody(auction->bidder.GetCounter(), auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .AddMoney(profit)
            .SendMailTo(trans, MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED, sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY));

        // RG-Custom ItemLogging
        RG_LOG<ItemLogModule>(GetAItem(auction->itemGUIDLow), RG::Logging::ItemLogType::AUCTION_SUCCESSFUL, auction->itemCount, auction->owner.GetCounter(), auction->bidder.GetCounter());
    }
}

//does not clear ram
void AuctionHouseMgr::SendAuctionExpiredMail(AuctionEntry* auction, SQLTransaction& trans)
{
    //return an item in auction to its owner by mail
    Item* pItem = GetAItem(auction->itemGUIDLow);
    if (!pItem)
        return;

    Player* owner = ObjectAccessor::FindConnectedPlayer(auction->owner);
    uint32 owner_accId = player::PlayerCache::GetAccountId(auction->owner);
    // owner exist
    if (owner || owner_accId)
    {
        if (owner)
            owner->GetSession()->SendAuctionOwnerNotification(auction);

        MailDraft(auction->BuildAuctionMailSubject(MailAuctionAnswers::EXPIRED), AuctionEntry::BuildAuctionMailBody(0, 0, auction->buyout, auction->deposit, 0))
            .AddItem(pItem)
            .SendMailTo(trans, MailReceiver(owner, auction->owner), auction, MAIL_CHECK_MASK_COPIED, 0);
    }
    else
    {
        // owner doesn't exist, delete the item
        sAuctionMgr->RemoveAItem(auction->itemGUIDLow, true);
    }
}

//this function sends mail to old bidder
void AuctionHouseMgr::SendAuctionOutbiddedMail(AuctionEntry* auction, uint32 newPrice, Player* newBidder, SQLTransaction& trans)
{
    Player* oldBidder = ObjectAccessor::FindConnectedPlayer(auction->bidder);

    uint32 oldBidder_accId = 0;
    if (!oldBidder)
        oldBidder_accId = player::PlayerCache::GetAccountId(auction->bidder);

    // old bidder exist
    if (oldBidder || oldBidder_accId)
    {
        if (oldBidder && newBidder)
            oldBidder->GetSession()->SendAuctionBidderNotification(auction->GetHouseId(), auction->Id, newBidder->GetGUID(), newPrice, auction->GetAuctionOutBid(), auction->itemEntry);

        MailDraft(auction->BuildAuctionMailSubject(MailAuctionAnswers::OUTBIDDED), AuctionEntry::BuildAuctionMailBody(auction->owner.GetCounter(), auction->bid, auction->buyout, auction->deposit, auction->GetAuctionCut()))
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(oldBidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
    }
}

//this function sends mail, when auction is cancelled to old bidder
void AuctionHouseMgr::SendAuctionCancelledToBidderMail(AuctionEntry* auction, SQLTransaction& trans)
{
    Player* bidder = ObjectAccessor::FindConnectedPlayer(auction->bidder);

    uint32 bidder_accId = 0;
    if (!bidder)
        bidder_accId = player::PlayerCache::GetAccountId(auction->bidder);

    // bidder exist
    if (bidder || bidder_accId)
        MailDraft(auction->BuildAuctionMailSubject(MailAuctionAnswers::CANCELLED_TO_BIDDER), AuctionEntry::BuildAuctionMailBody(auction->owner.GetCounter(), auction->bid, auction->buyout, auction->deposit, 0))
            .AddMoney(auction->bid)
            .SendMailTo(trans, MailReceiver(bidder, auction->bidder), auction, MAIL_CHECK_MASK_COPIED);
}

void AuctionHouseMgr::LoadAuctionItems()
{
    uint32 oldMSTime = getMSTime();

    // need to clear in case we are reloading
    if (!mAitems.empty())
    {
        for (ItemMap::iterator itr = mAitems.begin(); itr != mAitems.end(); ++itr)
            delete itr->second;

        mAitems.clear();
    }

    // data needs to be at first place for Item::LoadFromDB
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_AUCTION_ITEMS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 auction items. DB table `auctionhouse` or `item_instance` is empty!");

        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 item_guid        = fields[11].GetUInt32();
        uint32 itemEntry    = fields[12].GetUInt32();

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
        if (!proto)
        {
            TC_LOG_ERROR("misc", "AuctionHouseMgr::LoadAuctionItems: Unknown item (GUID: %u id: #%u) in auction, skipped.", item_guid, itemEntry);
            continue;
        }

        Item* item = NewItemOrBag(proto);
        if (!item->LoadFromDB(item_guid, ObjectGuid::Empty, fields, itemEntry))
        {
            delete item;
            continue;
        }
        AddAItem(item);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u auction items in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void AuctionHouseMgr::LoadAuctions()
{
    uint32 oldMSTime = getMSTime();

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_AUCTIONS);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 auctions. DB table `auctionhouse` is empty.");

        return;
    }

    uint32 count = 0;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    do
    {
        Field* fields = result->Fetch();

        AuctionEntry* aItem = new AuctionEntry();
        if (!aItem->LoadFromDB(fields))
        {
            TC_LOG_ERROR("misc", "AuctionHouseMgr::LoadAuctions: Corrupt auction (ID: %u), skipped.", aItem->Id);

            delete aItem;
            continue;
        }

        AuctionHouseObject* AH = GetAuctionsMap(aItem->factionTemplateId);
        AH->QueueEvent((Events::Event*)new AuctionHouseObject::AddAuctionEvent(AH, aItem));

        ++count;
    } while (result->NextRow());

    CharacterDatabase.CommitTransaction(trans);

    auto pendingSales = CharacterDatabase.Query("SELECT id, auctionHouseId, itemEntry, count, seller, buyout, time, buyer, bid, deposit FROM auctionhouse_pending_sales");

    if (pendingSales)
    {
        do
        {
            Field* fields = pendingSales->Fetch();

            AuctionPendingSale sale;
            if (!sale.LoadFromDB(fields))
            {
                TC_LOG_ERROR("misc", "AuctionHouseMgr::LoadAuctions: Corrupt pending sale (ID: %u), skipped.", sale.GetId());
                continue;
            }

            AuctionHouseObject* AH = GetAuctionsMap(sale.GetFactionTemplateId());
            AH->GetPendingSaleList(sale.GetSeller()).push_back(sale);

            ++count;
        } while (pendingSales->NextRow());
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u auctions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

}

void AuctionHouseMgr::AddAItem(Item* it)
{
    ASSERT(it);
    ASSERT(mAitems.find(it->GetGUID().GetCounter()) == mAitems.end());
    mAitems[it->GetGUID().GetCounter()] = it;
}

bool AuctionHouseMgr::RemoveAItem(uint32 id, bool deleteItem)
{
    ItemMap::iterator i = mAitems.find(id);
    if (i == mAitems.end())
        return false;

    if (deleteItem)
    {
        SQLTransaction trans = SQLTransaction(nullptr);
        i->second->FSetState(ITEM_REMOVED);
        i->second->SaveToDB(trans);
    }

    mAitems.erase(i);
    return true;
}

void AuctionHouseMgr::Update()
{
    for (auto& object : auctions)
        object->Update();
}

const AuctionHouseEntry* AuctionHouseMgr::GetAuctionHouseEntry(uint32 factionTemplateId)
{
    uint32 houseid = 7; // goblin auction house

    if (!sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION))
    {
        // FIXME: found way for proper auctionhouse selection by another way
        // AuctionHouse.dbc have faction field with _player_ factions associated with auction house races.
        // but no easy way convert creature faction to player race faction for specific city
        switch (factionTemplateId)
        {
            case   12: houseid = 1; break; // human
            case   29: houseid = 6; break; // orc, and generic for horde
            case   55: houseid = 2; break; // dwarf, and generic for alliance
            case   68: houseid = 4; break; // undead
            case   80: houseid = 3; break; // n-elf
            case  104: houseid = 5; break; // trolls
            case  120: houseid = 7; break; // booty bay, neutral
            case  474: houseid = 7; break; // gadgetzan, neutral
            case  855: houseid = 7; break; // everlook, neutral
            case 1604: houseid = 6; break; // b-elfs,
            default:                       // for unknown case
            {
                FactionTemplateEntry const* u_entry = sFactionTemplateStore.LookupEntry(factionTemplateId);
                if (!u_entry)
                    houseid = 7; // goblin auction house
                else if (u_entry->ourMask & FACTION_MASK_ALLIANCE)
                    houseid = 1; // human auction house
                else if (u_entry->ourMask & FACTION_MASK_HORDE)
                    houseid = 6; // orc auction house
                else
                    houseid = 7; // goblin auction house
                break;
            }
        }
    }

    return sAuctionHouseStore.LookupEntry(houseid);
}

// helpers

void AuctionHouseMgr::DeleteExpiredAuctionsAtStartup()
{
    // Deletes expired auctions. Should be called at server start before loading auctions.

    // DO NOT USE after auctions are already loaded since this deletes from the DB
    //  and assumes the auctions HAVE NOT been loaded into a list or AuctionEntryMap yet

    uint32 oldMSTime = getMSTime();
    uint32 expirecount = 0;
    time_t curTime = game::time::GetGameTime();

    // Query the DB to see if there are any expired auctions
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_AUCTIONS);
    stmt->setUInt32(0, (uint32)curTime+60);
    PreparedQueryResult expAuctions = CharacterDatabase.Query(stmt);

    if (!expAuctions)
    {
        TC_LOG_INFO("server.loading", ">> No expired auctions to delete");

        return;
    }

    do
    {
        Field* fields = expAuctions->Fetch();

        AuctionEntry* auction = new AuctionEntry();

        // Can't use LoadFromDB() because it assumes the auction map is loaded
        if (!auction->LoadFromFieldList(fields))
        {
            // For some reason the record in the DB is broken (possibly corrupt
            //  faction info). Delete the object and move on.
            delete auction;
            continue;
        }

        SQLTransaction trans = CharacterDatabase.BeginTransaction();

        if (!auction->bidder)
        {
            // Cancel the auction, there was no bidder
            sAuctionMgr->SendAuctionExpiredMail(auction, trans);
        }
        else
        {
            // Send the item to the winner and money to seller
            GetAuctionsMap(auction->factionTemplateId)->AddPendingSale(*auction, trans);
            sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
            sAuctionMgr->SendAuctionWonMail(auction, trans);
        }

        // Call the appropriate AuctionHouseObject script
        //  ** Do we need to do this while core is still loading? **
        sScriptMgr->OnAuctionExpire(GetAuctionsMap(auction->factionTemplateId), auction);

        // Delete the auction from the DB
        auction->DeleteFromDB(trans);
        CharacterDatabase.CommitTransaction(trans);

        // Release memory
        delete auction;
        ++expirecount;

    }
    while (expAuctions->NextRow());

    TC_LOG_INFO("server.loading", ">> Deleted %u expired auctions in %u ms", expirecount, GetMSTimeDiffToNow(oldMSTime));
}

AuctionSorter::AuctionSorter(AuctionSortType sorting, bool sortDesc)
{
    static const std::map<std::pair<AuctionSortType, bool>, bool(*)(const AuctionEntry*, const AuctionEntry*)> SORTERS =
    {
        { { AuctionSortType::LEVEL,      false }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return first->GetItemLevel()   < second->GetItemLevel();
        } },
        { { AuctionSortType::LEVEL,      true  }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return first->GetItemLevel()   > second->GetItemLevel();
        } },
        { { AuctionSortType::RARITY,     false }, [](const AuctionEntry* first, const AuctionEntry* second) { // Client reverses order for rarity
            return first->GetItemQuality() > second->GetItemQuality();
        } },
        { { AuctionSortType::RARITY,     true  }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return first->GetItemQuality() < second->GetItemQuality();
        } },
        { { AuctionSortType::TIMELEFT,   false }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return first->GetExpireTime()  < second->GetExpireTime();
        } },
        { { AuctionSortType::TIMELEFT,   true  }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return first->GetExpireTime()  > second->GetExpireTime();
        } },
        { { AuctionSortType::SELLER,     false }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return std::strcmp(first->GetOwnerName().c_str(), second->GetOwnerName().c_str()) < 0;
        } },
        { { AuctionSortType::SELLER,     true  }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return std::strcmp(first->GetOwnerName().c_str(), second->GetOwnerName().c_str()) > 0;
        } },
        { { AuctionSortType::CURRENTBID, false }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return first->GetCurrentOrStartBid() < second->GetCurrentOrStartBid();
        } },
        { { AuctionSortType::CURRENTBID, true  }, [](const AuctionEntry* first, const AuctionEntry* second) {
            return first->GetCurrentOrStartBid() > second->GetCurrentOrStartBid();
        } }
    };

    auto it = SORTERS.find({ sorting, sortDesc });

    if (it == SORTERS.end())
        _sorter = SORTERS.at({ AuctionSortType::RARITY, false });
    else
        _sorter = it->second;
}
