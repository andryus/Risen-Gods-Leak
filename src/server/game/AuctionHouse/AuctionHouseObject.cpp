#include "AuctionHouseObject.hpp"

#include "AuctionHouseMgr.hpp"

#include "DatabaseEnv.h"
#include "DBCStructure.h"
#include "ScriptMgr.h"
#include "World.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "DBCStores.h"
#include "Player/PlayerCache.hpp"
#include "Monitoring/Auctionhouse.hpp"
#include "util/meta/TypeChecks.hpp"
#include "GameTime.h"

void AuctionHouseObject::AddAuction(AuctionEntry* auction)
{
    ASSERT(auction);

    AuctionsMap[auction->Id] = auction;
    sScriptMgr->OnAuctionAdd(this, auction);
}

AuctionHouseObject::AuctionEntryMap::iterator AuctionHouseObject::RemoveAuction(AuctionHouseObject::AuctionEntryMap::iterator itr)
{
    ASSERT(itr != AuctionsMap.end());

    auto auction = itr->second;

    itr = AuctionsMap.erase(itr);

    sScriptMgr->OnAuctionRemove(this, auction);

    // we need to delete the entry, it is not referenced any more
    delete auction;

    return itr;
}

void AuctionHouseObject::Update()
{
    ///- Handle expired auctions

    // If storage is empty, no need to update. next == NULL in this case.
    Mut.lock();
    bool empty = AuctionsMap.empty();
    Mut.unlock();
    if (empty)
        return;

    time_t curTime = game::time::GetGameTime();
    SQLTransaction trans = CharacterDatabase.BeginTransaction();

    Mut.lock();

    for (auto itr = AuctionsMap.begin(); itr != AuctionsMap.end();)
    {
        AuctionEntry* auction = itr->second;

        ///- filter auctions expired on next update
        if (auction->expire_time > curTime + 60)
        {
            ++itr;
            continue;
        }

        ///- Either cancel the auction if there was no bidder
        if (!auction->bidder)
        {
            sAuctionMgr->SendAuctionExpiredMail(auction, trans);
            sScriptMgr->OnAuctionExpire(this, auction);
            MONITOR_AUCTIONHOUSE(ReportAuctionExpired());
        }
        ///- Or perform the transaction
        else
        {
            //we should send an "item sold" message if the seller is online
            //we send the item to the winner
            //we send the money to the seller
            AddPendingSale(*auction, trans);
            sAuctionMgr->SendAuctionSuccessfulMail(auction, trans);
            sAuctionMgr->SendAuctionWonMail(auction, trans);
            sScriptMgr->OnAuctionSuccessful(this, auction);
            MONITOR_AUCTIONHOUSE(ReportAuctionWon(false));
        }

        ///- In any case clear the auction
        auction->DeleteFromDB(trans);

        sAuctionMgr->RemoveAItem(auction->itemGUIDLow);
        itr = RemoveAuction(itr);
    }

    Mut.unlock();

    CharacterDatabase.CommitTransaction(trans);
}

void AuctionHouseObject::BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (auto&[id, entry] : AuctionsMap)
    {
        util::meta::Unreferenced(id);

        if (entry->bidder == player->GetGUID())
        {
            // Prevent to large packets
            if (count < MAX_AUCTIONS_SHOWN)
                if (entry->BuildAuctionInfo(data))
                    ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount)
{
    for (auto&[id, entry] : AuctionsMap)
    {
        util::meta::Unreferenced(id);

        if (entry->owner == player->GetGUID())
        {
            // Prevent to large packets
            if (count < MAX_AUCTIONS_SHOWN)
                if (entry->BuildAuctionInfo(data))
                    ++count;

            ++totalcount;
        }
    }
}

void AuctionHouseObject::BuildListPendingSales(WorldPacket& data, ObjectGuid player, uint32& count)
{
    auto const itr = PendingSalesMap.find(player);
    if (itr == PendingSalesMap.end())
        return;

    time_t now = time(nullptr);
    auto& pendingList = itr->second;

    for (auto pendingItr = pendingList.begin(); pendingItr != pendingList.end();)
    {
        AuctionPendingSale const& sale = *pendingItr;

        if (sale.IsExpired(now))
        {
            sale.DeleteFromDB();
            pendingItr = pendingList.erase(pendingItr);
        }
        else
        {
            const uint32 unk1 = 0;
            const uint32 unk2 = 0;

            ++pendingItr;
            data << sale.BuildAuctionPendingHeader();
            data << sale.BuildAuctionPendingBody();
            data << unk1;
            data << unk2;
            data << float(sale.GetExpireTime() - now) / float(60 * 60 * 24);
            ++count;
        }
    }
}

void AuctionHouseObject::BuildListAuctionItems(WorldPacket& data, Player* player,
    std::wstring const& wsearchedname, uint32 listfrom, uint8 levelmin, uint8 levelmax, uint8 usable,
    uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
    uint32& count, uint32& totalcount, bool isFull, AuctionSortType sorting, bool sortDesc)
{
    if (isFull)
        MONITOR_AUCTIONHOUSE(ReportFullscan());
    else
    {
        MONITOR_AUCTIONHOUSE(ReportQuery(itemClass != 0xffffffff
            || itemSubClass != 0xffffffff
            || inventoryType != 0xffffffff
            || quality != 0xffffffff
            || levelmin != 0x00
            || levelmax != 0x00
            || usable != 0x00
            || !wsearchedname.empty()));
    }

    int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    int locdbc_idx = player->GetSession()->GetSessionDbcLocale();

    time_t curTime = game::time::GetGameTime();

    std::multiset<AuctionEntry*, AuctionSorter> res = std::multiset<AuctionEntry*, AuctionSorter>(AuctionSorter(sorting, sortDesc));

    for (auto&[id, entry] : AuctionsMap)
    {
        util::meta::Unreferenced(id);

        // Skip expired auctions
        if (entry->expire_time < curTime)
            continue;

        Item* item = sAuctionMgr->GetAItem(entry->itemGUIDLow);
        if (!item)
            continue;

        if (isFull)
        {
            if (count < MAX_AUCTIONS_SHOWN)
                if (entry->BuildAuctionInfo(data))
                    ++count;

            ++totalcount;
            continue;
        }

        ItemTemplate const* proto = item->GetTemplate();

        if (itemClass != 0xffffffff && proto->Class != itemClass)
            continue;

        if (itemSubClass != 0xffffffff && proto->SubClass != itemSubClass)
            continue;

        if (inventoryType != 0xffffffff && proto->GetInventoryTypeForAuctionFilter() != inventoryType)
            continue;

        if (quality != 0xffffffff && proto->Quality != quality)
            continue;

        if (levelmin != 0x00 && (proto->RequiredLevel < levelmin || (levelmax != 0x00 && proto->RequiredLevel > levelmax)))
            continue;

        if (usable != 0x00)
        {
            if (player->CanUseItem(item) != EQUIP_ERR_OK)
                continue;

            // check already learded recipes and pets
            if (proto->Spells[1].SpellTrigger == ITEM_SPELLTRIGGER_LEARN_SPELL_ID && player->HasSpell(proto->Spells[1].SpellId))
                continue;
        }

        // Allow search by suffix (ie: of the Monkey) or partial name (ie: Monkey)
        // No need to do any of this if no search term was entered
        if (!wsearchedname.empty())
        {
            std::string name = proto->Name1;
            if (name.empty())
                continue;

            // local name
            if (loc_idx >= 0)
                if (ItemLocale const* il = sObjectMgr->GetItemLocale(proto->ItemId))
                    ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

            // DO NOT use GetItemEnchantMod(proto->RandomProperty) as it may return a result
            //  that matches the search but it may not equal item->GetItemRandomPropertyId()
            //  used in BuildAuctionInfo() which then causes wrong items to be listed
            int32 propRefID = item->GetItemRandomPropertyId();

            if (propRefID)
            {
                // Append the suffix to the name (ie: of the Monkey) if one exists
                // These are found in ItemRandomSuffix.dbc and ItemRandomProperties.dbc
                //  even though the DBC names seem misleading
                char* const* suffix = nullptr;
                
                if (propRefID < 0)
                {
                    const ItemRandomSuffixEntry* itemRandSuffix = sItemRandomSuffixStore.LookupEntry(-propRefID);
                    if (itemRandSuffix)
                        suffix = itemRandSuffix->nameSuffix;
                }
                else
                {
                    const ItemRandomPropertiesEntry* itemRandProp = sItemRandomPropertiesStore.LookupEntry(propRefID);
                    if (itemRandProp)
                        suffix = itemRandProp->nameSuffix;
                }

                // dbc local name
                if (suffix)
                {
                    // Append the suffix (ie: of the Monkey) to the name using localization
                    // or default enUS if localization is invalid
                    name += ' ';
                    name += suffix[locdbc_idx >= 0 ? locdbc_idx : LOCALE_enUS];
                }
            }

            // Perform the search (with or without suffix)
            if (!Utf8FitTo(name, wsearchedname))
                continue;
        }

        ++totalcount;

        res.insert(entry);

        if (res.size() > listfrom + 50)
            res.erase(std::prev(res.end())); // We don't need entries past that
    }

    if (!isFull)
        for (auto it = res.rbegin(); it != res.rend(); ++it) // Wrong order but will be sorted client-side
        {
            (*it)->BuildAuctionInfo(data);
            ++count;
            if (count >= 50 || count >= (totalcount - listfrom))
                break;
        }

    if (sLog->ShouldLog("rg.lagreport.auctionhouse", LOG_LEVEL_INFO))
    {
        std::string name;
        if (WStrToUtf8(wsearchedname, name))
        {
            TC_LOG_INFO("rg.lagreport.auctionhouse",
                "Auction List: Player %s - Matched: %u Total: %u - Name: %s Offset: %u Level (min): %u Level (max): %u Usable: %u Type: %u Class: %u Subclass: %u Quality: %u Full: %u",
                player->GetSession()->GetPlayerInfo().c_str(),
                count, totalcount,
                name.c_str(), listfrom,
                levelmin, levelmax, usable,
                inventoryType, itemClass, itemSubClass, quality,
                isFull ? 1 : 0);
        }
    }
}

//this function inserts to WorldPacket auction's data
bool AuctionEntry::BuildAuctionInfo(WorldPacket& data) const
{
    Item* item = sAuctionMgr->GetAItem(itemGUIDLow);
    if (!item)
    {
        TC_LOG_ERROR("misc", "AuctionEntry::BuildAuctionInfo: Auction %u has a non-existent item: %u", Id, itemGUIDLow);
        return false;
    }
    data << uint32(Id);
    data << uint32(item->GetEntry());

    for (uint8 i = 0; i < MAX_INSPECTED_ENCHANTMENT_SLOT; ++i)
    {
        data << uint32(item->GetEnchantmentId(EnchantmentSlot(i)));
        data << uint32(item->GetEnchantmentDuration(EnchantmentSlot(i)));
        data << uint32(item->GetEnchantmentCharges(EnchantmentSlot(i)));
    }

    data << int32(item->GetItemRandomPropertyId());                 // Random item property id
    data << uint32(item->GetItemSuffixFactor());                    // SuffixFactor
    data << uint32(item->GetCount());                               // item->count
    data << uint32(item->GetSpellCharges());                        // item->charge FFFFFFF
    data << uint32(0);                                              // Unknown
    data << uint64(owner);                                          // Auction->owner
    data << uint32(startbid);                                       // Auction->startbid (not sure if useful)
    data << uint32(bid ? GetAuctionOutBid() : 0);
    // Minimal outbid
    data << uint32(buyout);                                         // Auction->buyout
    data << uint32((expire_time - time(NULL)) * IN_MILLISECONDS);   // time left
    data << uint64(bidder);                                         // auction->bidder current
    data << uint32(bid);                                            // current bid
    return true;
}

uint32 AuctionEntry::GetHouseId() const
{
    return auctionHouseEntry->houseId;
}

uint32 AuctionEntry::GetHouseFaction() const
{
    return auctionHouseEntry->faction;
}

uint32 AuctionEntry::GetAuctionCut() const
{
    int32 cut = int32(CalculatePct(bid, auctionHouseEntry->cutPercent) * sWorld->getRate(RATE_AUCTION_CUT));
    return std::max(cut, 0);
}

/// the sum of outbid is (1% from current bid)*5, if bid is very small, it is 1c
uint32 AuctionEntry::GetAuctionOutBid() const
{
    uint32 outbid = CalculatePct(bid, 5);
    return outbid ? outbid : 1;
}

void AuctionEntry::DeleteFromDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_AUCTION);
    stmt->setUInt32(0, Id);
    trans->Append(stmt);
}

void AuctionEntry::SaveToDB(SQLTransaction& trans) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_AUCTION);
    stmt->setUInt32(0, Id);
    stmt->setUInt32(1, GetHouseId());
    stmt->setUInt32(2, itemGUIDLow);
    stmt->setUInt32(3, owner.GetCounter());
    stmt->setUInt32(4, buyout);
    stmt->setUInt32(5, uint32(expire_time));
    stmt->setUInt32(6, bidder.GetCounter());
    stmt->setUInt32(7, bid);
    stmt->setUInt32(8, startbid);
    stmt->setUInt32(9, deposit);
    trans->Append(stmt);
}

bool AuctionEntry::LoadFromDB(Field* fields)
{
    uint32 auctionHouseId = 0;

    Id = fields[0].GetUInt32();
    auctionHouseId = fields[1].GetUInt32();
    itemGUIDLow = fields[2].GetUInt32();
    itemEntry = fields[3].GetUInt32();
    itemCount = fields[4].GetUInt32();
    owner = ObjectGuid(HighGuid::Player, fields[5].GetUInt32());
    buyout = fields[6].GetUInt32();
    expire_time = fields[7].GetUInt32();
    bidder = ObjectGuid(HighGuid::Player, fields[8].GetUInt32());
    bid = fields[9].GetUInt32();
    startbid = fields[10].GetUInt32();
    deposit = fields[11].GetUInt32();

    auctionHouseEntry = sAuctionHouseStore.LookupEntry(auctionHouseId);
    if (!auctionHouseEntry)
    {
        TC_LOG_ERROR("misc", "Auction %u has non existing auction house id %u", Id, auctionHouseId);
        return false;
    }

    // check if sold item exists for guid
    // and itemEntry in fact (GetAItem will fail if problematic in result check in AuctionHouseMgr::LoadAuctionItems)
    if (!sAuctionMgr->GetAItem(itemGUIDLow))
    {
        TC_LOG_ERROR("misc", "Auction %u has not a existing item : %u", Id, itemGUIDLow);
        return false;
    }
    return true;
}

bool AuctionEntry::LoadFromFieldList(Field* fields)
{
    // Loads an AuctionEntry item from a field list. Unlike "LoadFromDB()", this one
    //  does not require the AuctionEntryMap to have been loaded with items. It simply
    //  acts as a wrapper to fill out an AuctionEntry struct from a field list
    uint32 auctionHouseId = 0;

    Id = fields[0].GetUInt32();
    auctionHouseId = fields[1].GetUInt32();
    itemGUIDLow = fields[2].GetUInt32();
    itemEntry = fields[3].GetUInt32();
    itemCount = fields[4].GetUInt32();
    owner = ObjectGuid(HighGuid::Player, fields[5].GetUInt32());
    buyout = fields[6].GetUInt32();
    expire_time = fields[7].GetUInt32();
    bidder = ObjectGuid(HighGuid::Player, fields[8].GetUInt32());
    bid = fields[9].GetUInt32();
    startbid = fields[10].GetUInt32();
    deposit = fields[11].GetUInt32();

    auctionHouseEntry = sAuctionHouseStore.LookupEntry(auctionHouseId);
    if (!auctionHouseEntry)
    {
        TC_LOG_ERROR("misc", "AuctionEntry::LoadFromFieldList() - Auction %u has non existing auction house id %u", Id, auctionHouseId);
        return false;
    }

    return true;
}

std::string AuctionEntry::BuildAuctionMailSubject(MailAuctionAnswers response) const
{
    std::ostringstream strm;
    strm << itemEntry << ":0:" << (uint32)response << ':' << Id << ':' << itemCount;
    return strm.str();
}

std::string AuctionEntry::BuildAuctionMailBody(uint32 lowGuid, uint32 bid, uint32 buyout, uint32 deposit, uint32 cut, uint32 time)
{
    std::ostringstream strm;
    strm.width(16);
    strm << std::right << std::hex << ObjectGuid(HighGuid::Player, lowGuid).GetRawValue();   // HighGuid::Player always present, even for empty guids
    strm << std::dec << ':' << bid << ':' << buyout;
    strm << ':' << deposit << ':' << cut;
    if (time != 0)
        strm << ':' << 0 << ':' << time;
    return strm.str();
}

uint32 AuctionEntry::GetItemLevel() const
{
    if (const ItemTemplate* temp = sObjectMgr->GetItemTemplate(itemEntry))
        return temp->ItemLevel;
    return 0;
}

uint32 AuctionEntry::GetItemQuality() const
{
    if (const ItemTemplate* temp = sObjectMgr->GetItemTemplate(itemEntry))
        return temp->Quality;
    return 0;
}

time_t AuctionEntry::GetExpireTime() const
{
    return expire_time;
}

std::string AuctionEntry::GetOwnerName() const
{
    return player::PlayerCache::GetName(owner);
}

uint32 AuctionEntry::GetCurrentOrStartBid() const
{
    return bid ? bid : startbid;
}

bool AuctionPendingSale::LoadFromDB(Field* fields)
{
    // id, auctionHousId, itemEntry, count, seller, buyout, time, buyer, bid, deposit
    uint32 auctionHouseId = 0;

    Id = fields[0].GetUInt32();
    auctionHouseId = fields[1].GetUInt32();
    itemGUIDLow = 0;
    itemEntry = fields[2].GetUInt32();
    itemCount = fields[3].GetUInt32();
    owner = ObjectGuid(HighGuid::Player, fields[4].GetUInt32());
    buyout = fields[5].GetUInt32();
    expire_time = fields[6].GetUInt32();
    bidder = ObjectGuid(HighGuid::Player, fields[7].GetUInt32());
    bid = fields[8].GetUInt32();
    startbid = 0;
    deposit = fields[9].GetUInt32();

    auctionHouseEntry = sAuctionHouseStore.LookupEntry(auctionHouseId);
    if (!auctionHouseEntry)
    {
        TC_LOG_ERROR("misc", "Auction %u has non existing auction house id %u", Id, auctionHouseId);
        return false;
    }

    return true;
}

AuctionPendingSale::AuctionPendingSale(AuctionEntry const& auction, time_t expireTime) : AuctionEntry(auction)
{
    expire_time = expireTime;
}

std::string AuctionPendingSale::BuildAuctionPendingHeader() const
{
    return BuildAuctionMailSubject(MailAuctionAnswers::SUCCESSFUL);
}

std::string AuctionPendingSale::BuildAuctionPendingBody() const
{
    return BuildAuctionMailBody(bidder.GetCounter(), bid, buyout, deposit, GetAuctionCut(), expire_time);
}

void AuctionPendingSale::SaveToDB(SQLTransaction& trans)
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PENDING_SALE);

    stmt->setUInt32(0, Id);
    stmt->setUInt32(1, itemEntry);
    stmt->setUInt32(2, itemCount);
    stmt->setUInt32(3, owner.GetCounter());
    stmt->setUInt32(4, bidder.GetCounter());
    stmt->setUInt32(5, bid);
    stmt->setUInt32(6, buyout);
    stmt->setUInt32(7, expire_time);
    stmt->setUInt32(8, GetHouseId());
    stmt->setUInt32(9, deposit);

    trans->Append(stmt);
}

void AuctionPendingSale::DeleteFromDB() const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_PENDING_SALE);
    stmt->setUInt32(0, Id);
    CharacterDatabase.AsyncQuery(stmt);
}

void AuctionHouseObject::AddPendingSale(AuctionEntry& auction, SQLTransaction& trans)
{
    auto& pendingSales = PendingSalesMap[auction.owner];
    pendingSales.emplace_back(auction, time(nullptr) + sWorld->getIntConfig(CONFIG_MAIL_DELIVERY_DELAY));
    pendingSales[pendingSales.size() - 1].SaveToDB(trans);
}

void AuctionHouseObject::SendBidderItemsEvent::execute()
{
    Player* player = sObjectAccessor->FindPlayer(playerGuid);

    if (!player)
        return;

    AH->BuildListBidderItems(pck, player, count, totalcount);
    pck.put<uint32>(0, count);                           // add count to placeholder
    pck << totalcount;
    pck << (uint32)sWorld->getIntConfig(CONFIG_AUCTIONHOUSE_QUERY_DELAY_NORMAL);
    player->SendDirectMessage(std::move(pck));
}

void AuctionHouseObject::SendOwnerItemsEvent::execute()
{
    Player* player = sObjectAccessor->FindPlayer(playerGuid);

    if (!player)
        return;

    WorldPacket data(SMSG_AUCTION_OWNER_LIST_RESULT, (4 + 4 + 4));
    data << (uint32)0;                                     // amount place holder

    uint32 count = 0;
    uint32 totalcount = 0;

    AH->BuildListOwnerItems(data, player, count, totalcount);
    data.put<uint32>(0, count);
    data << (uint32)totalcount;
    data << (uint32)0;
    player->SendDirectMessage(std::move(data));
}

void AuctionHouseObject::SendOwnerPendingSalesEvent::execute()
{
    Player* player = sObjectAccessor->FindPlayer(playerGuid);

    if (!player)
        return;

    WorldPacket data(SMSG_AUCTION_LIST_PENDING_SALES, 4);
    data << static_cast<uint32>(0);                                     // amount place holder

    uint32 count = 0;

    AH.BuildListPendingSales(data, playerGuid, count);
    data.put<uint32>(0, count);
    player->SendDirectMessage(std::move(data));
}

void AuctionHouseObject::SendAuctionListEvent::execute()
{
    Player* player = sObjectAccessor->FindPlayer(playerGuid);

    if (!player)
        return;

    WorldPacket data(SMSG_AUCTION_LIST_RESULT, (4 + 4 + 4));
    uint32 count = 0;
    uint32 totalcount = 0;
    data << (uint32)0;

    AH->BuildListAuctionItems(data, player,
        searchedname, listfrom, levelmin, levelmax, usable,
        inventoryType, itemClass, itemSubClass, quality,
        count, totalcount, isFull, sorting, sortDesc);

    data.put<uint32>(0, count);
    data << (uint32)totalcount;
    data << (uint32)sWorld->getIntConfig(CONFIG_AUCTIONHOUSE_QUERY_DELAY_NORMAL);
    player->SendDirectMessage(std::move(data));
}

void AuctionHouseObject::AddAuctionEvent::execute()
{
    AH->Mut.lock();
    AH->AddAuction(Auction);
    AH->Mut.unlock();
}

void AuctionHouseObject::RemoveAuctionEvent::execute()
{
    AH->Mut.lock();
    AH->RemoveAuction(AH->AuctionsMap.find(Auction->Id));
    AH->Mut.unlock();
}
