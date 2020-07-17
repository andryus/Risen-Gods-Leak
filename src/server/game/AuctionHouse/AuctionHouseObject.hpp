#pragma once

#include <mutex>
#include "Define.h"

#include "Database/Transaction.h" // todo: replace with forward declaration
#include "Entities/Object/ObjectGuid.h"
#include "Event/EventQueue.h"
#include "Packets/WorldPacket.h"

constexpr uint32 MAX_AUCTIONS_SHOWN = 50000;

enum class MailAuctionAnswers : uint8
{
    OUTBIDDED = 0,
    WON,
    SUCCESSFUL,
    EXPIRED,
    CANCELLED_TO_BIDDER,
    CANCELED,
    SALE_PENDING
};

struct AuctionHouseEntry;
class WorldPacket;
class Field;
class Player;


struct AuctionEntry
{
    uint32 Id;
    uint32 itemGUIDLow;
    uint32 itemEntry;
    uint32 itemCount;
    ObjectGuid owner;
    uint32 startbid;
    uint32 bid;
    uint32 buyout;
    time_t expire_time;
    ObjectGuid bidder;
    uint32 deposit;                                         // deposit can be calculated only when creating auction
    const AuctionHouseEntry* auctionHouseEntry;             // in AuctionHouse.dbc
    uint32 factionTemplateId;

    // helpers
    uint32 GetHouseId() const;
    uint32 GetHouseFaction() const;
    uint32 GetAuctionCut() const;
    uint32 GetAuctionOutBid() const;
    bool BuildAuctionInfo(WorldPacket& data) const;
    void DeleteFromDB(SQLTransaction& trans) const;
    void SaveToDB(SQLTransaction& trans) const;
    bool LoadFromDB(Field* fields);
    bool LoadFromFieldList(Field* fields);
    std::string BuildAuctionMailSubject(MailAuctionAnswers response) const;
    static std::string BuildAuctionMailBody(uint32 lowGuid, uint32 bid, uint32 buyout, uint32 deposit, uint32 cut, uint32 time = 0);

    // for sorting
    uint32 GetItemLevel() const;
    uint32 GetItemQuality() const;
    time_t GetExpireTime() const;
    std::string GetOwnerName() const;
    uint32 GetCurrentOrStartBid() const;

};

struct AuctionPendingSale : protected AuctionEntry
{
    bool LoadFromDB(Field* fields);

    AuctionPendingSale() = default;
    AuctionPendingSale(AuctionEntry const& auction, time_t expireTime);

    std::string BuildAuctionPendingHeader() const;
    std::string BuildAuctionPendingBody() const;

    uint32 GetId() const { return Id;  }

    uint32 GetFactionTemplateId() const { return factionTemplateId; }
    ObjectGuid GetSeller() const { return owner; }

    bool IsExpired(time_t now) const { return now > expire_time; }
    time_t GetExpireTime() const { return expire_time; }

    void SaveToDB(SQLTransaction& trans);
    void DeleteFromDB() const;
};

enum class AuctionSortType : uint8;

//this class is used as auctionhouse instance
class AuctionHouseObject
{
public:
    AuctionHouseObject()
    {
        EventQueue.start(1);
    }
    ~AuctionHouseObject()
    {
        for (AuctionEntryMap::iterator itr = AuctionsMap.begin(); itr != AuctionsMap.end(); ++itr)
            delete itr->second;
        EventQueue.shutdown();
    }

    typedef std::map<uint32, AuctionEntry*> AuctionEntryMap;
    using AuctionPendingSaleMap = std::unordered_map<ObjectGuid, std::vector<AuctionPendingSale>>;

    AuctionEntry* GetAuction(uint32 id)
    {
        Mut.lock();
        AuctionEntry* result = _GetAuction(id);
        Mut.unlock();
        return result;
    }

    void Update();

    void QueueEvent(Events::Event* op) { EventQueue.schedule(op); }

    class AddAuctionEvent : Events::Event
    {
    public:
        AddAuctionEvent(AuctionHouseObject* AH, AuctionEntry* Auction) : AH(AH), Auction(Auction) {}
        void execute() override;
    private:
        AuctionHouseObject * AH;
        AuctionEntry* Auction;
    };

    class RemoveAuctionEvent : Events::Event
    {
    public:
        RemoveAuctionEvent(AuctionHouseObject* AH, AuctionEntry* Auction) : AH(AH), Auction(Auction) {}
        void execute() override;
    private:
        AuctionHouseObject * AH;
        AuctionEntry* Auction;
    };

    class SendBidderItemsEvent : Events::Event
    {
    public:
        SendBidderItemsEvent(AuctionHouseObject* AH, ObjectGuid playerGuid, WorldPacket&& pck, uint32 count, uint32 totalcount) :
            AH(AH), playerGuid(playerGuid), pck(std::move(pck)), count(count), totalcount(totalcount)
        {
        }

        void execute() override;
    private:
        AuctionHouseObject * AH;
        ObjectGuid playerGuid;
        uint32 count, totalcount;
        WorldPacket pck;
    };

    class SendOwnerItemsEvent : Events::Event
    {
    public:
        SendOwnerItemsEvent(AuctionHouseObject* AH, ObjectGuid playerGuid) : AH(AH), playerGuid(playerGuid) {}
        void execute() override;
    private:
        AuctionHouseObject * AH;
        ObjectGuid playerGuid;
    };

    class SendOwnerPendingSalesEvent : Events::Event
    {
    public:
        SendOwnerPendingSalesEvent(AuctionHouseObject& AH, ObjectGuid playerGuid) : AH(AH), playerGuid(playerGuid) {}
        void execute() override;
    private:
        AuctionHouseObject & AH;
        ObjectGuid playerGuid;
    };

    class SendAuctionListEvent : Events::Event
    {
    public:
        SendAuctionListEvent(AuctionHouseObject* AH, ObjectGuid playerGuid, std::wstring const& searchedname, uint32 listfrom,
            uint8 levelmin, uint8 levelmax, uint8 usable, uint32 inventoryType, uint32 itemClass, uint32 itemSubClass,
            uint32 quality, bool isFull, AuctionSortType sorting, bool sortDesc) : AH(AH), playerGuid(playerGuid), searchedname(searchedname),
            listfrom(listfrom), levelmin(levelmin), levelmax(levelmax), usable(usable), inventoryType(inventoryType),
            itemClass(itemClass), itemSubClass(itemSubClass), quality(quality), isFull(isFull), sorting(sorting), sortDesc(sortDesc) {}
        void execute() override;
    private:
        AuctionHouseObject * AH;
        std::wstring searchedname;
        ObjectGuid playerGuid;
        uint32 listfrom, inventoryType, itemClass, itemSubClass, quality;
        uint8 levelmin, levelmax, usable;
        AuctionSortType sorting;
        bool isFull, sortDesc;
    };

    void AddPendingSale(AuctionEntry& auction, SQLTransaction& trans);
    std::vector<AuctionPendingSale>& GetPendingSaleList(ObjectGuid player) { return PendingSalesMap[player]; }

private:
    void AddAuction(AuctionEntry* auction);

    AuctionEntryMap::iterator RemoveAuction(AuctionEntryMap::iterator itr);

    void BuildListBidderItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount);
    void BuildListOwnerItems(WorldPacket& data, Player* player, uint32& count, uint32& totalcount);
    void BuildListPendingSales(WorldPacket& data, ObjectGuid player, uint32& count);
    void BuildListAuctionItems(WorldPacket& data, Player* player,
        std::wstring const& searchedname, uint32 listfrom, uint8 levelmin, uint8 levelmax, uint8 usable,
        uint32 inventoryType, uint32 itemClass, uint32 itemSubClass, uint32 quality,
        uint32& count, uint32& totalcount, bool isFull, AuctionSortType sorting, bool sortDesc);

    AuctionPendingSaleMap PendingSalesMap;
    AuctionEntryMap AuctionsMap;
    Events::EventQueue EventQueue;
    std::mutex Mut;

    AuctionEntry* _GetAuction(uint32 id)
    {
        auto itr = AuctionsMap.find(id);
        auto* result = itr != AuctionsMap.end() ? itr->second : NULL;
        return result;
    }
};