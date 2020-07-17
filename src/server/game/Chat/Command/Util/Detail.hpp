#pragma once

#include <utility>
#include <string_view>

#include <boost/optional.hpp>

#include "Chat/Command/CommandDefines.hpp"

/*
 * Contains utility method(s) internally used for the command system.
 */

namespace chat { namespace command { namespace util
{

    // Split a raw command into 2 parts. Example: "ban account Blub" -> "ban", "account Blub"
    // if the result is not boost::none, it guarantees that pair.first contains a valid (not empty, not containing whitespace) command name
    boost::optional<std::pair<CommandString, CommandString>> SplitCommandString(const CommandString& commandString);

}}}
