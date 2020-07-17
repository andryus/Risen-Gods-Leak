#pragma once

#include <string>

#include "Define.h"
#include "Utilities/Mask.hpp"

#include "Locale/LocalizedString.hpp"

namespace chat {
    namespace command {

        using CommandString = std::string_view;

        enum class CommandFlag : uint32
        {
            DISABLE_LOGGING = 0x0001,
            HIDDEN          = 0x0002,
        };
        using CommandMask = ::util::Mask<CommandFlag>;

        // struct for passing command definition on initialization
        struct CommandDefinition
        {
            uint32 id;
            int32 sortingKey;
            std::string name;
            locale::LocalizedString syntax;
            locale::LocalizedString description;
            uint32 permission;
            CommandMask mask;
        };

    }
}
