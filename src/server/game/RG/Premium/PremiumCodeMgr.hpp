#pragma once

#include <mutex>

#include "ScriptMgr.h"
#include "PremiumCode.hpp"

namespace cpr
{
    class Response;
}

class GAME_API PremiumCodeMgr
{
public:
    PremiumCodeMgr();

    bool IsActive();

    folly::Future<PremiumCode> GetCodeInfo(std::string rawCode);
    folly::Future<bool> RedeemCode(Player& player, PremiumCode& premium);

private:
    folly::Future<bool> ContinueRedeemingAfterChecks(Player& player, PremiumCode& premium);
    folly::Future<bool> SetCodeStateInProgress(Player& player, PremiumCode& premium);
    void InvalidateCode(ObjectGuid guid, PremiumCode& premium);

    bool HandleCode(ObjectGuid guid, PremiumCode& premium);
    bool CheckRaceChangeCode(Player& player, PremiumCode& premium);
    folly::Future<bool> CheckFactionChangeCode(Player& player, PremiumCode& premium);
    bool CanStoreItemInInventory(Player& player, PremiumCode& premium);

    void ApplyCharModifyCode(Player& target, PremiumCode& premium);
    bool AddItem(Player& player, PremiumCode& premium);

    static bool _active;
    static std::string _apiUrl;
    static std::string _apiHeaderAuthValue;

protected:
    PremiumCode ParseCode(std::string code, cpr::Response& response) const;
    bool CheckStatusChange(cpr::Response& response) const;

public:
    static void LoadConfig();
};

class WorldReloadScript : public WorldScript
{
public:
    WorldReloadScript() : WorldScript("rg_premium_config") { }

    void OnConfigLoad(bool reload);
};
