#pragma once

#include <memory>

#include "Scripting/ScriptMgr.h"

#include "Chat/Command/CommandDefines.hpp"

namespace chat {
    namespace command {

        template<class CommandClass>
        class GenericCommandScriptLoader : public CommandScriptLoader
        {
        public:
            explicit GenericCommandScriptLoader(const char* name) : CommandScriptLoader(name) {};
            std::unique_ptr<Command> CreateCommand(CommandDefinition definition) const override
            {
                return std::make_unique<CommandClass>(std::move(definition));
            }
        };

    }
}
