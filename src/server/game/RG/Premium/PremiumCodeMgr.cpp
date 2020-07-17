#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#ifdef CONFIG_WITH_CPR_AND_CURL
  #include <cpr/api.h>
#endif // CONFIG_WITH_CPR_AND_CURL
#include <folly/futures/Future.h>

#include "Chat.h"
#include "Config.h"
#include "Language.h"
#include "Player.h"
#include "ObjectAccessor.h"

#include "RG/Logging/LogManager.hpp"

#include "PremiumCodeMgr.hpp"

enum TCGItemIds
{
    ITEM_TCG_GIFT_H             = 66812,
    ITEM_TCG_GIFT_A             = 66813
};

class PlayerLogoutEvent : public BasicEvent
{
public:
    PlayerLogoutEvent(Player& player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/)
    {
        _player.GetSession()->LogoutPlayer(true);
        return true;
    }
private:
    Player& _player;
};

PremiumCodeMgr::PremiumCodeMgr()
{
    LoadConfig();
    new WorldReloadScript();
}

bool PremiumCodeMgr::IsActive()
{
#ifdef CONFIG_WITH_CPR_AND_CURL
    return _active;
#else 
    return false;
#endif // CONFIG_WITH_CPR_AND_CURL
}

folly::Future<PremiumCode> PremiumCodeMgr::GetCodeInfo(std::string rawCode)
{
#ifdef CONFIG_WITH_CPR_AND_CURL
    folly::Promise<PremiumCode> promise;
    auto future = promise.getFuture();

    sWorld->httpRequestThreadPool.add([this, rawCode, p = std::move(promise)]() mutable {
        auto response = cpr::Get(
            cpr::Url{ _apiUrl + rawCode },
            cpr::VerifySsl{ false },
            cpr::Header{ { "Authorization", _apiHeaderAuthValue } },
            cpr::Timeout{ 3s });

        PremiumCode premium = ParseCode(rawCode, response);
        p.setValue(premium);
    });

    return future;
#else
    return PremiumCode(CodeType::Unknown, CodeState::InternalError, 0, rawCode);
#endif // CONFIG_WITH_CPR_AND_CURL
}

folly::Future<bool> PremiumCodeMgr::RedeemCode(Player& player, PremiumCode& premium)
{
    // we can do some checks right here
    switch (premium.GetType())
    {
        case CodeType::CharChangeFaction:
            return CheckFactionChangeCode(player, premium)
                .via(player.GetExecutor())
                .then([this, premium, guid = player.GetGUID()](bool ok) mutable {
                        Player* player = ObjectAccessor::FindPlayer(guid);
                        if (ok && player)
                            return ContinueRedeemingAfterChecks(*player, premium).via(player->GetExecutor());
                        else
                            return folly::Future<bool>(false);
                    });
        case CodeType::CharChangeRace:
            if (!CheckRaceChangeCode(player, premium))
                return folly::Future<bool>(false);
            break;
        case CodeType::TradingCard:
            premium.SetPetItemId(player.GetTeamId() == TEAM_HORDE ? ITEM_TCG_GIFT_H : ITEM_TCG_GIFT_A);
        case CodeType::Pet:
            if (!CanStoreItemInInventory(player, premium))
                return folly::Future<bool>(false);
            break;
        default:
            break;
    }
    return ContinueRedeemingAfterChecks(player, premium).via(player.GetExecutor());
}

folly::Future<bool> PremiumCodeMgr::ContinueRedeemingAfterChecks(Player& player, PremiumCode& premium)
{
    return SetCodeStateInProgress(player, premium)
        .via(player.GetExecutor())
        .then([this, guid = player.GetGUID(), premium](bool ok) mutable {
                if (ok)
                {
                    if (HandleCode(guid, premium))
                        return folly::Future<bool>(true);
                    return folly::Future<bool>(false);
                }
                else
                    return folly::Future<bool>(false);
            });
}

folly::Future<bool> PremiumCodeMgr::SetCodeStateInProgress(Player& player, PremiumCode& premium)
{
#ifdef CONFIG_WITH_CPR_AND_CURL
    TC_LOG_INFO("entities.unit.premium", "SetCodeStateInProgress: Player: %u confirmed code: %s", player.GetGUID().GetCounter(), premium.GetRawCode().c_str());

    folly::Promise<bool> promise;
    auto future = promise.getFuture();
    sWorld->httpRequestThreadPool.add([this, guid = player.GetGUID(), &premium, p = std::move(promise)]() mutable {
        auto response = cpr::Post(
            cpr::Url{ _apiUrl + premium.GetRawCode() + "/" + std::to_string(guid.GetCounter()) },
            cpr::VerifySsl{ false },
            cpr::Header{ { "Authorization", _apiHeaderAuthValue } },
            cpr::Timeout{ 3s });

        if (CheckStatusChange(response))
            p.setValue(true);
        else
            p.setValue(false);
    });

    return future;
#else
    return folly::Future<bool>(false);
#endif // CONFIG_WITH_CPR_AND_CURL
}

void PremiumCodeMgr::InvalidateCode(ObjectGuid guid, PremiumCode& premium)
{
#ifdef CONFIG_WITH_CPR_AND_CURL
    TC_LOG_INFO("entities.unit.premium", "InvalidateCode: Code %s from Player: %u, is beeing invalidated", premium.GetRawCode().c_str(), guid.GetCounter());

    sWorld->httpRequestThreadPool.add([guid, premium]() mutable {
        auto response = cpr::Put(
            cpr::Url{ _apiUrl + premium.GetRawCode() + "/" + std::to_string(guid.GetCounter()) },
            cpr::VerifySsl{ false },
            cpr::Header{ { "Authorization", _apiHeaderAuthValue } },
            cpr::Timeout{ 5s });

        if (response.status_code == 200)
            TC_LOG_INFO("entities.unit.premium", "InvalidateCode: Successful invalidated code: %s", premium.GetRawCode().c_str());
        else
            TC_LOG_FATAL("entities.unit.premium", "InvalidateCode: Error in invalidation of code: %s status_code: %i header(custom field): %s",
                premium.GetRawCode().c_str(), response.status_code, response.header["x-invalidationproblem"].c_str());
    });
#endif // CONFIG_WITH_CPR_AND_CURL
}

bool PremiumCodeMgr::HandleCode(ObjectGuid guid, PremiumCode& premium)
{
    Player* player = ObjectAccessor::FindPlayer(guid);
    if (!player)
        return false;

    RG_LOG<PremiumLogModule>(guid, RG::Logging::PremiumLogModule::Action::Confirm, premium);
    switch (premium.GetType())
    {
        case CodeType::CharChangeName:
            player->m_Events.AddEvent(new PlayerLogoutEvent(*player), player->m_Events.CalculateTime(5 * IN_MILLISECONDS));
        case CodeType::CharCustomize:
        case CodeType::CharChangeRace:
        case CodeType::CharChangeFaction:
            ApplyCharModifyCode(*player, premium);
            break;
        case CodeType::Pet:
        case CodeType::TradingCard:
            if (!AddItem(*player, premium))
                return false;
            break;
        default:
            return false;
    }

    auto trans = CharacterDatabase.BeginTransaction();
    if (premium.GetItemId())
        player->SaveInventoryAndCurrenciesToDB(trans);
    else
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
        stmt->setUInt16(0, player->GetAtLoginFlags());
        stmt->setUInt32(1, guid.GetCounter());
        trans->Append(stmt);
    }
    CharacterDatabase.CommitTransaction(trans)
        .via(player->GetExecutor())
        .then([this, guid = player->GetGUID(), premium](bool ok) mutable {
                Player* player = ObjectAccessor::FindPlayer(guid);
                if (ok && player)
                    InvalidateCode(guid, premium);
                else
                    TC_LOG_FATAL("entities.unit.premium", "HandleCode: PlayerSave not completed for unknown reason. Code: %s is NOT invalidated!",
                        premium.GetRawCode().c_str());
    });
    return true;
}

bool PremiumCodeMgr::CheckRaceChangeCode(Player& player, PremiumCode& premium)
{
    bool noError = true;
    if (player.GetTeamId() == TEAM_ALLIANCE)
    {
        if (player.getClass() == CLASS_SHAMAN || player.getClass() == CLASS_DRUID)
            noError = false;
    }
    else // if (player.GetTeamId() == TEAM_HORDE)
    {
        if (player.getClass() == CLASS_PALADIN || player.getClass() == CLASS_DRUID)
            noError = false;
    }

    if (!noError)
        ChatHandler(player.GetSession()).PSendSysMessage(LANG_PREMIUM_RACECHANGE_FAIL, premium.GetAmount());

    return noError;
}

folly::Future<bool> PremiumCodeMgr::CheckFactionChangeCode(Player& player, PremiumCode& premium)
{
    auto errorMessage = [](PreparedQueryResult result, ObjectGuid guid) -> void
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player)
            return;

        auto handler = ChatHandler(player->GetSession());
        uint8 resultFlag = 0;
        do
        {
            Field const* fields = result->Fetch();
            resultFlag |= fields[0].GetUInt8();
        } while (result->NextRow());

        constexpr std::pair<uint8, TrinityStrings> resultMsg[] =
        { 
            { 1, LANG_PREMIUM_FACTIONCHANGE_FAIL_AUCTIONS },
            { 2, LANG_PREMIUM_FACTIONCHANGE_FAIL_POST },
            { 4, LANG_PREMIUM_FACTIONCHANGE_FAIL_GUILD },
            { 8, LANG_PREMIUM_FACTIONCHANGE_FAIL_ARENA }
        };

        for (const auto[flag, msg] : resultMsg)
        {
            if ((resultFlag & flag) != 0)
            {
                handler.PSendSysMessage(msg);
                break;
            }
        }
    };

    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHECK_FACTIONCHANGE);
    for (uint8 i = 0; i < 4; ++i)
        stmt->setUInt32(i, player.GetGUID().GetCounter());

    ObjectGuid guid = player.GetGUID();
    return CharacterDatabase.AsyncQuery(stmt)
            .via(player.GetExecutor())
            .then([errorMessage, guid](PreparedQueryResult result) {
                    if (result)
                    {
                        errorMessage(std::move(result), guid);
                        return folly::Future<bool>(false);
                    }
                    else
                        return folly::Future<bool>(true);
            });
}

bool PremiumCodeMgr::CanStoreItemInInventory(Player& player, PremiumCode& premium)
{
    ItemPosCountVec dest;
    const InventoryResult msg = player.CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, premium.GetItemId(), premium.GetAmount());
    if (msg != EQUIP_ERR_OK)
        player.SendEquipError(msg, nullptr, nullptr, premium.GetItemId());
    return msg == EQUIP_ERR_OK;
}

void PremiumCodeMgr::ApplyCharModifyCode(Player& player, PremiumCode& premium)
{
    AtLoginFlags flag = AT_LOGIN_NONE;
    switch (premium.GetType())
    {
        case CodeType::CharChangeName:
            flag = AT_LOGIN_RENAME;
            break;
        case CodeType::CharCustomize:
            flag = AT_LOGIN_CUSTOMIZE;
            break;
        case CodeType::CharChangeRace:
            flag = AT_LOGIN_CHANGE_RACE;
            break;
        case CodeType::CharChangeFaction:
            flag = AT_LOGIN_CHANGE_FACTION;
            break;
        default:
            ASSERT(false);
            return;
    }
    player.SetAtLoginFlag(flag);
}

bool PremiumCodeMgr::AddItem(Player& player, PremiumCode& premium)
{
    ItemPosCountVec dest;
    const InventoryResult msg = player.CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, premium.GetItemId(), premium.GetAmount());
    if (msg == EQUIP_ERR_OK)
    {
        Item* item = player.StoreNewItem(dest, premium.GetItemId(), true);
        ASSERT(item);
        player.SendNewItem(item, premium.GetAmount(), true, false, true);
        // RG-Custom ItemLogging
        RG_LOG<ItemLogModule>(item, RG::Logging::ItemLogType::ADDED, premium.GetAmount());
        return true;
    }
    return false;
}

PremiumCode PremiumCodeMgr::ParseCode(std::string code, cpr::Response& response) const
{
#ifdef CONFIG_WITH_CPR_AND_CURL
    if (response.status_code != 200 || response.error.code != cpr::ErrorCode::OK)
    {
        TC_LOG_FATAL("entities.unit.premium", "ParseCode: Request failed. HTTP status code: %i, error code: %u, error message: %s",
            response.status_code, static_cast<uint32>(response.error.code), response.error.message.c_str());
        return PremiumCode(CodeType::Unknown, CodeState::InternalError, 0, code);
    }

    namespace pt = boost::property_tree;
    pt::ptree jsonTree;
    try
    {
        auto jsonString = std::stringstream(response.text);
        pt::read_json(jsonString, jsonTree);
        if (jsonTree.get<uint8>("not_found", 0))
            return PremiumCode(CodeType::Unknown, CodeState::NotFound, 0, code);

        static const std::pair<CodeType, std::string> dbCodeNames[] =
        {
            { CodeType::Unknown, "UNKNOWN" },
            { CodeType::CharMoveOnRealm, "CHARMOVEONREALM" },
            { CodeType::GuildRename, "GUILDRENAME" },
            { CodeType::CharChangeName, "RENAME" },
            { CodeType::CharCustomize, "CUSTOMIZE" },
            { CodeType::CharChangeRace, "RACECHANGE" },
            { CodeType::CharChangeFaction, "FACTIONCHANGE" },
            { CodeType::Pet, "PET" },
            { CodeType::TradingCard, "TRADINGCARDGOODS" }
        };

        std::string dbType = jsonTree.get<std::string>("type");
        CodeType type;
        for (auto codeMap : dbCodeNames)
        {
            if (dbType == codeMap.second)
            {
                type = codeMap.first;
                break;
            }
        }

        uint16 amount = jsonTree.get<uint16>("amount", 1);

        uint8 used = jsonTree.get<uint8>("used");
        if (used == 2)
            return PremiumCode(type, CodeState::Invalid, amount, code);
        else if (used == 1)
            return PremiumCode(type, CodeState::InternalError, amount, code);

        return PremiumCode(type, CodeState::Valid, amount, code);
    }
    catch (std::exception const& e)
    {
        TC_LOG_FATAL("entities.unit.premium", "ParseCode: Failed to parse json: %s", e.what());
        return PremiumCode(CodeType::Unknown, CodeState::InternalError, 0, code);
    }
#else
    return PremiumCode(CodeType::Unknown, CodeState::InternalError, 0, code);
#endif // CONFIG_WITH_CPR_AND_CURL
}

bool PremiumCodeMgr::CheckStatusChange(cpr::Response& response) const
{
#ifdef CONFIG_WITH_CPR_AND_CURL
    if (response.status_code != 200 || response.error.code != cpr::ErrorCode::OK)
    {
        TC_LOG_FATAL("entities.unit.premium", "CheckStatusChange: Request failed. HTTP status code: %i, error code: %u, error message: %s",
            response.status_code, static_cast<uint32>(response.error.code), response.error.message.c_str());
        return false;
    }

    namespace pt = boost::property_tree;
    pt::ptree jsonTree;
    try
    {
        auto jsonString = std::stringstream(response.text);
        pt::read_json(jsonString, jsonTree);
        return static_cast<bool>(jsonTree.get<uint8>("successful"));
    }
    catch (std::exception const& e)
    {
        TC_LOG_FATAL("entities.unit.premium", "CheckStatusChange: Failed to parse json: %s", e.what());
        return false;
    }
#else
    return false;
#endif // CONFIG_WITH_CPR_AND_CURL
}

bool PremiumCodeMgr::_active;
std::string PremiumCodeMgr::_apiUrl;
std::string PremiumCodeMgr::_apiHeaderAuthValue;

void PremiumCodeMgr::LoadConfig()
{
    _active = sWorld->getBoolConfig(CONFIG_PREMIUM_NPC_ACTIVE);
    _apiUrl = sConfigMgr->GetStringDefault("RG.Premium.Api.Url", "");
    _apiHeaderAuthValue = "Bearer " + sConfigMgr->GetStringDefault("RG.Premium.Api.key", "");
}

void WorldReloadScript::OnConfigLoad(bool reload)
{
    if (reload)
        PremiumCodeMgr::LoadConfig();
}
