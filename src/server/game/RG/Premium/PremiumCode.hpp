#pragma once

#include "Common.h"

enum class CodeType : uint8
{
    Unknown           = 0,
    CharMoveOnRealm   = 1,
    GuildRename       = 2,
    CharChangeName    = 3,
    CharCustomize     = 4,
    CharChangeRace    = 5,
    CharChangeFaction = 6,
    Pet               = 7,
    TradingCard       = 8
};

enum class CodeState : uint8
{
    Valid             = 0,
    Invalid           = 1,
    NotFound          = 2,
    InternalError     = 3
};

class PremiumCode
{
public:
    explicit PremiumCode() : PremiumCode(CodeType::Unknown, CodeState::NotFound, 0, "") { }
    explicit PremiumCode(CodeType type, CodeState state, uint16 amount, std::string code) :
        _type(type), _state(state), _amount(amount), _code(std::move(code)), _itemId(0) { }

    CodeType GetType() const { return _type; }
    CodeState GetState() const { return _state; }
    uint8 GetAmount() const { return _amount; }
    std::string GetRawCode() const { return _code; }

    void SetPetItemId(uint32 itemId) { _itemId = itemId; }
    uint32 GetItemId() const { return _itemId; }

    bool IsValid() const { return _state == CodeState::Valid; }
    bool CanBeRedeemedIngame() const { return _type != CodeType::CharMoveOnRealm && _type != CodeType::GuildRename; }
    bool CanApplyCooldown() const { return _state == CodeState::NotFound; }

private:
    CodeType _type;
    CodeState _state;
    uint16 _amount;
    uint32 _itemId;
    std::string _code;
};
