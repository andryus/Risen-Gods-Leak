#include "ItemLogModule.hpp"

#include "Player.h"
#include "Config.h"
#include "ObjectAccessor.h"
#include "Map.h"

RG::Logging::ItemLogModule::ItemLogModule() :
    LogModule("Item")
{ }

void RG::Logging::ItemLogModule::Log(Item* item, ItemLogType type, uint32 count, uint32 ownerGUID, uint32 targetGUID)
{
    if (!item)
        return;
    
    if (!IsEnabled())
        return;

    Map* map = NULL;
    if (Player* player = ObjectAccessor::FindPlayer(item->GetOwnerGUID()))
        map = player->GetMap();

    const ItemTemplate* proto = item->GetTemplate();
    if (proto &&
        !IsBlacklisted(item->GetEntry()) &&
        (   proto->Quality >= config.quality 
         || (map && map->IsDungeon() && proto->Class == ITEM_CLASS_QUEST)
         || IsWhitelisted(item->GetEntry())
       ))
    {
        PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_ADD_ITEM_LOG);

        stmt->setUInt32(0, ownerGUID > 0 ? ownerGUID : item->GetOwnerGUID().GetCounter());
        stmt->setUInt32(1, item->GetGUID().GetCounter());
        stmt->setUInt32(2, item->GetEntry());
        stmt->setUInt32(3, count > 0 ? count : item->GetCount());
        stmt->setUInt32(4, static_cast<uint32>(type));
        stmt->setUInt32(5, targetGUID);

        RGDatabase.Execute(stmt);
    }
}

void RG::Logging::ItemLogModule::LoadConfig()
{
    LogModule::LoadConfig();

    config.quality = static_cast<uint32>(sConfigMgr->GetIntDefault(GetOption("Quality").c_str(), ITEM_QUALITY_EPIC));

    auto readList = [](const std::string& option, std::unordered_set<uint32>& target) {
        target.clear();

        std::string whitelistString = sConfigMgr->GetStringDefault(option.c_str(), "");
        Tokenizer tokens(whitelistString, ',');
        for (Tokenizer::const_iterator itr = tokens.begin(); itr != tokens.end(); ++itr)
        {
            std::string str(*itr);
            size_t pos1 = str.find_first_not_of(" ");
            size_t pos2 = str.find_last_not_of(" ");
            target.insert(static_cast<uint32>(strtoul(str.c_str(), NULL, 10)));
        }
    };
    
    readList(GetOption("Whitelist"), config.whitelist);
    readList(GetOption("Blacklist"), config.blacklist);
}

inline bool RG::Logging::ItemLogModule::IsWhitelisted(uint32 entry) const
{
    return config.whitelist.find(entry) != config.whitelist.end();
}

inline bool RG::Logging::ItemLogModule::IsBlacklisted(uint32 entry) const
{
    return config.blacklist.find(entry) != config.blacklist.end();
}
