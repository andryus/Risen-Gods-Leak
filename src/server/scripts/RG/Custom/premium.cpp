#include "PremiumCodeMgr.hpp"

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "WorldSession.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Chat.h"
#include "Language.h"

#include "RG/Logging/LogManager.hpp"

struct CodeCooldown
{
    void OnPlayerEnterWrongCode(ObjectGuid guid)
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player)
            return;

        uint32 accountId = player->GetSession()->GetAccountId();
        if (auto itr = _playerWrongCodeAttempts.find(accountId); itr != _playerWrongCodeAttempts.end())
        {
            itr->second.attempts++;
            itr->second.lastTime = time(nullptr);
        }
        else
            _playerWrongCodeAttempts[accountId] = { 0, time(nullptr) };
    }

    bool IsPlayerLocked(uint32 accountId)
    {
        constexpr uint32 HOURS_24 = 24 * HOUR;
        constexpr uint32 MAX_WRONG_ATTEMNTS = 3;

        if (auto itr = _playerWrongCodeAttempts.find(accountId); itr != _playerWrongCodeAttempts.end())
        {
            if ((itr->second.lastTime + HOURS_24) > time(nullptr))
                return itr->second.attempts >= MAX_WRONG_ATTEMNTS;
            else
            {
                _playerWrongCodeAttempts.erase(itr);
                return false;
            }
        }
        else
            return false;
    }

    bool ReleasePlayerLock(uint32 accountId)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (auto itr = _playerWrongCodeAttempts.find(accountId); itr != _playerWrongCodeAttempts.end())
        {
            _playerWrongCodeAttempts.erase(itr);
            return true;
        }
        return false;
    }

private:
    struct AttemptTime
    {
        uint8 attempts;
        time_t lastTime;
    };

    std::mutex _mutex;
    std::unordered_map<uint32 /*accountId*/, AttemptTime> _playerWrongCodeAttempts;
};

enum Gossip
{
    // premium code handle gossips
    GOSSIP_ACTION_ENTER_CODE    = 100,
    GOSSIP_ACTION_CONFIRM       = 101,
    GOSSIP_ACTION_ABORT         = 102,
    GOSSIP_ACTION_GM_UNLOCK     = 103,

    GOSSIP_ACTION_PET_START     = 1000,
    // reserved 1000 - 1019
    GOSSIP_ACTION_PET_END       = 1019,

    GOSSIP_ITEM_HELLO           = 60250,
    GOSSIP_ITEM_GOODBEY         = 60251,
    GOSSIP_ITEM_CONFIRM         = 60252,
    GOSSIP_ITEM_ABORT           = 60253,
    GOSSIP_ITEM_GM_UNLOCK       = 60254,

    GOSSIP_ITEM_CONFIRM_PET     = 60244
};

enum NpcText
{
    NPC_TEXT_HELLO              = 2002197,
    NPC_TEXT_HELLO_CODE_ACTIVE  = 2002198,
    NPC_TEXT_HELLO_COOLDOWN     = 2002199,
    NPC_TEXT_HELLO_UNAVAILABLE  = 2002200,

    NPC_TEXT_CHAR_CHANGE_NAME   = 2002201,
    NPC_TEXT_CHAR_CUSTOMIZE     = 2002202,
    NPC_TEXT_CHAR_RACECHANGE    = 2002203,
    NPC_TEXT_CHAR_FACTIONCHANGE = 2002204,

    NPC_TEXT_PET                = 2002205,
    NPC_TEXT_TRADING_CARD       = 2002206,

    NPC_TEXT_CHAR_MOVE_ON_REALM = 2002207,
    NPC_TEXT_GUILD_RENAME       = 2002224,

    NPC_TEXT_CODE_NOT_FOUND     = 2002208,
    NPC_TEXT_CODE_INVALID       = 2002209,
    NPC_TEXT_INTERNAL_ERROR     = 2002210,

    NPC_TEXT_PET_SELECT         = 2002223,
    NPC_TEXT_PET_SELECT_FAIL    = 2002225
};

struct CodeTypeMapping
{
    CodeType type;
    uint64 npcText;
    TrinityStrings response;
};

static const std::array<CodeTypeMapping, 9U> codeMappingByType
{ {
    { CodeType::Unknown, 0, LANG_UNKNOWN },
    { CodeType::CharMoveOnRealm, NPC_TEXT_CHAR_MOVE_ON_REALM, LANG_UNKNOWN },
    { CodeType::GuildRename, NPC_TEXT_GUILD_RENAME, LANG_UNKNOWN },
    { CodeType::CharChangeName, NPC_TEXT_CHAR_CHANGE_NAME, LANG_PREMIUM_RENAME_SUCCESS },
    { CodeType::CharCustomize, NPC_TEXT_CHAR_CUSTOMIZE, LANG_PREMIUM_CUSTOMIZE_SUCCESS },
    { CodeType::CharChangeRace, NPC_TEXT_CHAR_RACECHANGE, LANG_PREMIUM_RACECHANGE_SUCCESS },
    { CodeType::CharChangeFaction, NPC_TEXT_CHAR_FACTIONCHANGE, LANG_PREMIUM_FACTIONCHANGE_SUCCESS },
    { CodeType::Pet, NPC_TEXT_PET, LANG_PREMIUM_PET_SUCCESS },
    { CodeType::TradingCard, NPC_TEXT_TRADING_CARD, LANG_PREMIUM_TRADINGCARDGOODS_SUCCESS }
} };

struct PetMapping
{
    uint8 menuId;
    uint32 itemId;
    uint32 spellId;
    uint32 broadcastTextId;
};

constexpr PetMapping petMapping[] =
{
    { 0,  13582, 17709, 1000058 },
    { 1,  13583, 17707, 1000059 },
    { 2,  22114, 27241, 1000060 },
    { 3,  20371, 24696, 1000061 },
    { 4,  39286, 52615, 1000062 },
    { 5,  39656, 53082, 1000063 },
    { 6,  49663, 69536, 1000064 },
    { 7,  49662, 69535, 1000065 },
    { 8,  49693, 69677, 1000066 },
    { 9,  54847, 75906, 1000067 },
    { 10, 13584, 17708, 1000068 },
    { 11, 25535, 32298, 1000069 },
    { 12, 49362, 69002, 1000070 },
    { 13, 46802, 66030, 1000071 },
    { 14, 49646, 69452, 1000072 },
    { 15, 49665, 69541, 1000073 },
    { 16, 56806, 78381, 1000074 },
    { 17, 45180, 63318, 1000075 },
    { 18, 30360, 24988, 1000076 },
    { 19, 44819, 61855, 1000091 },
};

class npc_premium_redeem : public CreatureScript
{
    std::unordered_map<ObjectGuid, PremiumCode> playerCodes;
    std::mutex mutex;

    CodeCooldown codeCooldown;
    PremiumCodeMgr mgr;

public:
    npc_premium_redeem() : CreatureScript("npc_premium_redeem") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (mgr.IsActive())
        {
            if (player->HasAtLoginFlag(AtLoginFlags(AT_LOGIN_RENAME | AT_LOGIN_CUSTOMIZE | AT_LOGIN_CHANGE_FACTION | AT_LOGIN_CHANGE_RACE)))
            {
                player->PlayerTalkClass->SendGossipMenu(NPC_TEXT_HELLO_CODE_ACTIVE, creature->GetGUID());
                return true;
            }
            else if (codeCooldown.IsPlayerLocked(player->GetSession()->GetAccountId()))
            {
                player->PlayerTalkClass->SendGossipMenu(NPC_TEXT_HELLO_COOLDOWN, creature->GetGUID());
                return true;
            }
            player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_HELLO, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ENTER_CODE);
            player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_HELLO, 1, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_GOSSIP);
            player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_HELLO, 2, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_GOSSIP);
            player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_HELLO, 3, GOSSIP_SENDER_MAIN, GOSSIP_OPTION_GOSSIP);
            player->PlayerTalkClass->GetGossipMenu().AddGossipMenuItemData(1, 60233, 0);
            player->PlayerTalkClass->GetGossipMenu().AddGossipMenuItemData(2, 60234, 0);
            player->PlayerTalkClass->GetGossipMenu().AddGossipMenuItemData(3, 60235, 0);
            player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_HELLO, 4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ABORT);

            if (player->GetSession()->HasPermission(rbac::RBAC_PERM_RG_MANAGE_PREMIUM_NPC))
                player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_GM_UNLOCK, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_GM_UNLOCK);
            player->PlayerTalkClass->SendGossipMenu(NPC_TEXT_HELLO, creature->GetGUID());
        }
        else
            player->PlayerTalkClass->SendGossipMenu(NPC_TEXT_HELLO_UNAVAILABLE, creature->GetGUID());

        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        if (action == GOSSIP_ACTION_CONFIRM)
        {
            if (PremiumCode* premium = GetPremiumFor(*player))
            {
                if (premium->GetType() == CodeType::Pet)
                {
                    SendPetMenu(*player, *premium, creature->GetGUID());
                    return true;
                }

                player->PlayerTalkClass->SendCloseGossip();
                HandleRedeemCode(*player, *premium);
                return true;
            }
        }
        else if (action >= GOSSIP_ACTION_PET_START)
        {
            if (PremiumCode* premium = GetPremiumFor(*player); premium->GetType() == CodeType::Pet)
            {
                uint32 menuId = (action - GOSSIP_ACTION_PET_START);
                premium->SetPetItemId(petMapping[menuId].itemId);

                player->PlayerTalkClass->SendCloseGossip();
                HandleRedeemCode(*player, *premium);
                return true;
            }
            uint32 menuId = (action - GOSSIP_ACTION_PET_START);
        }
        else if (action == GOSSIP_ACTION_ABORT)
        {
            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
        return false;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 /*sender*/, uint32 action, const char* code) override
    {
        if (action == GOSSIP_ACTION_GM_UNLOCK)
        {
            player->PlayerTalkClass->SendCloseGossip();
            if (!player->GetSession()->HasPermission(rbac::RBAC_PERM_RG_MANAGE_PREMIUM_NPC))
                return true;

            uint32 accountId = std::atoi(code);
            if (codeCooldown.ReleasePlayerLock(accountId))
                ChatHandler(player->GetSession()).PSendSysMessage(LANG_PREMIUM_GM_RELEASE_PLAYER_LOCK_SUCCESS, accountId);
            else
                ChatHandler(player->GetSession()).PSendSysMessage(LANG_PREMIUM_GM_RELEASE_PLAYER_LOCK_FAIL, accountId);
            return true;
        }

        TC_LOG_INFO("entities.unit.premium", "OnGossipSelectCode: Player: %u entered code: %s", player->GetGUID().GetCounter(), code);

        mgr.GetCodeInfo(code)
            .via(player->GetExecutor())
            .then([this, playerGuid = player->GetGUID(), creatureGuid = creature->GetGUID()](PremiumCode& premium) {
                    RG_LOG<PremiumLogModule>(playerGuid, RG::Logging::PremiumLogModule::Action::Enter, premium);

                    if (premium.IsValid())
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        playerCodes[playerGuid] = premium;
                    }
                    else if (premium.CanApplyCooldown())
                    {
                        codeCooldown.OnPlayerEnterWrongCode(playerGuid);
                    }
                    SendConfirmMenu(playerGuid, premium, creatureGuid);
                });

        return true;
    }

private:
    PremiumCode* GetPremiumFor(Player& player)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto itr = playerCodes.find(player.GetGUID());
        return itr == playerCodes.end() ? nullptr : &itr->second;
    }

    void SendConfirmMenu(ObjectGuid guid, PremiumCode& premium, ObjectGuid creatureGuid)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
        {
            player->PlayerTalkClass->ClearMenus();
            if (premium.IsValid())
            {
                player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_ABORT, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ABORT);
                if (premium.CanBeRedeemedIngame())
                    player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_CONFIRM, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_CONFIRM);
                player->PlayerTalkClass->SendGossipMenu(codeMappingByType[static_cast<uint8>(premium.GetType())].npcText, creatureGuid);

                if (premium.GetAmount() > 1)
                    ChatHandler(player->GetSession()).PSendSysMessage(LANG_PREMIUM_TRADINGCARD_MULTI, premium.GetAmount());
            }
            else
            {
                player->PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_GOODBEY, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ABORT);

                WorldPacket data(SMSG_INVALID_PROMOTION_CODE);
                player->SendDirectMessage(std::move(data));

                constexpr std::pair<CodeState, uint32> resultGossip[] =
                {
                    { CodeState::Invalid,       NPC_TEXT_CODE_INVALID },
                    { CodeState::NotFound,      NPC_TEXT_CODE_NOT_FOUND },
                    { CodeState::InternalError, NPC_TEXT_INTERNAL_ERROR }
                };

                for (const auto[CodeState, textId] : resultGossip)
                    if (premium.GetState() == CodeState)
                    {
                        player->PlayerTalkClass->SendGossipMenu(textId, creatureGuid);
                        return;
                    }
            }
        }
    }

    void HandleRedeemCode(Player& player, PremiumCode& premium)
    {
        mgr.RedeemCode(player, premium)
            .via(player.GetExecutor())
            .then([premium = premium, guid = player.GetGUID()](bool ok) {
                    if (ok)
                    {
                        Player* player = ObjectAccessor::FindPlayer(guid);
                        if (!player)
                            return;

                        switch (premium.GetType())
                        {
                            case CodeType::CharChangeName:
                            case CodeType::CharCustomize:
                            case CodeType::CharChangeRace:
                            case CodeType::CharChangeFaction:
                                ChatHandler(player->GetSession()).SendSysMessage(codeMappingByType[static_cast<uint8>(premium.GetType())].response);
                                break;
                            case CodeType::Pet:
                                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(premium.GetItemId()))
                                {
                                    uint32 broadcastTextId = 0;
                                    for (auto petMap : petMapping)
                                        if (petMap.itemId == premium.GetItemId())
                                        {
                                            broadcastTextId = petMap.broadcastTextId;
                                            break;
                                        }
                                    ChatHandler(player->GetSession()).
                                        PSendSysMessage(codeMappingByType[static_cast<uint8>(premium.GetType())].response,
                                            sObjectMgr->GetBroadcastText(broadcastTextId)->GetText(player->GetSession()->GetSessionDbcLocale()).c_str());
                                }
                                return;
                            case CodeType::TradingCard:
                                ChatHandler(player->GetSession()).
                                    PSendSysMessage(codeMappingByType[static_cast<uint8>(premium.GetType())].response, premium.GetAmount());
                                break;
                            default:
                                return;
                        }
                    }
        });
    }

    void SendPetMenu(Player& player, PremiumCode& premium, ObjectGuid creatureGuid)
    {
        player.PlayerTalkClass->ClearMenus();

        bool noPets = true;
        for (auto[menuId, itemId, spellId, broadcastTextId] : petMapping)
        {
            if (!player.HasSpell(spellId) && !player.HasItemCount(itemId, 1, true))
            {
                noPets = false;
                player.PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_CONFIRM_PET, menuId, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_PET_START + menuId);
            }
        }

        if (noPets)
        {
            player.PlayerTalkClass->GetGossipMenu().AddMenuItem(GOSSIP_ITEM_GOODBEY, 0, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_ABORT);
            player.PlayerTalkClass->SendGossipMenu(NPC_TEXT_PET_SELECT_FAIL, creatureGuid);
        }
        else
            player.PlayerTalkClass->SendGossipMenu(NPC_TEXT_PET_SELECT, creatureGuid);
    }
};

void AddSC_custom_premium()
{
    new npc_premium_redeem();
}
