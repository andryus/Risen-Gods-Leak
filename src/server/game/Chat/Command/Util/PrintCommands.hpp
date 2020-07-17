#pragma once

#include "Chat/Command/CommandDefines.hpp"
#include "Chat/Command/ExecutionContext.hpp"

namespace chat { namespace command { namespace util
{

    template<class Range>
    void PrintCommands(Range& range, ExecutionContext& context, const std::string& format = "%s")
    {
        for (auto& command : range)
        {
            if (command.GetMask() & CommandFlag::HIDDEN)
                continue;

            auto meta = command.ToMetaCommand();
            if (meta && meta->HasAvailableSubCommands(context))
                context.SendSystemMessage(format, command.GetName() + " ...");
            else
                context.SendSystemMessage(format, command.GetName());
        }
    }

}}}
