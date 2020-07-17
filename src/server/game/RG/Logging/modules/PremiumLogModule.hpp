#pragma once

#include <atomic>

#include "RG/Logging/LogModule.hpp"
#include "RG/Premium/PremiumCode.hpp"

class ObjectGuid;

namespace RG { namespace Logging {

class GAME_API PremiumLogModule : public LogModule
{
public:
    enum class Action : uint8
    {
        Enter   = 1,
        Confirm = 2,
        Apply   = 3
    };

    PremiumLogModule();

    void Log(ObjectGuid guid, Action action, PremiumCode& premium);
    void Log(ObjectGuid guid, Action action, CodeType type);
};

} } // namespace RG::Logging

using RG::Logging::PremiumLogModule;
