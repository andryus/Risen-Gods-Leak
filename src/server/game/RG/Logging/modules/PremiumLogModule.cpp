#include "PremiumLogModule.hpp"

#include "DatabaseEnv.h"
#include "ObjectGuid.h"
#include "Config.h"

using namespace RG::Logging;

PremiumLogModule::PremiumLogModule() :
    LogModule("Premium")
{
}

void PremiumLogModule::Log(ObjectGuid guid, Action action, PremiumCode& premium)
{
    // sanitize check
    if (!IsEnabled() || action == Action::Apply)
        return;

    // only 1 log entry for 1 code
    // we log valid codes when player confirm
    // invalid codes can't be confirmed
    if (action == Action::Enter && premium.GetState() == CodeState::Valid)
        return;

    PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_INS_PREMIUM);
    stmt->setUInt32(0, guid.GetCounter());
    stmt->setUInt8(1, static_cast<uint8>(action));
    stmt->setUInt8(2, static_cast<uint8>(premium.GetState()) +1); // sql enum index starts with 1. We use 0 as first index for cpp enum
    stmt->setUInt8(3, static_cast<uint8>(premium.GetType()) +1); // sql enum index starts with 1. We use 0 as first index for cpp enum
    stmt->setUInt8(4, premium.GetAmount());
    stmt->setString(5, premium.GetRawCode());
    RGDatabase.Execute(stmt);
}

void PremiumLogModule::Log(ObjectGuid guid, Action action, CodeType type)
{
    // sanitize check
    if (!IsEnabled() || action != Action::Apply)
        return;

    PreparedStatement* stmt = RGDatabase.GetPreparedStatement(RG_INS_PREMIUM);
    stmt->setUInt32(0, guid.GetCounter());
    stmt->setUInt8(1, static_cast<uint8>(action));
    stmt->setUInt8(2, static_cast<uint8>(CodeState::Valid));
    stmt->setUInt8(3, static_cast<uint8>(type) +1); // sql enum index starts with 1. We use 0 as first index for cpp enum
    stmt->setUInt8(4, 1);
    stmt->setString(5, "");
    RGDatabase.Execute(stmt);
}

