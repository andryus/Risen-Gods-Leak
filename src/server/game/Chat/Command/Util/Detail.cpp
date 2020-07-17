#include "Chat/Command/Util/Detail.hpp"

namespace chat {
    namespace command {
        namespace util {

            boost::optional<std::pair<CommandString, CommandString>> SplitCommandString(const CommandString& commandString)
            {
                if (commandString.empty())
                    return boost::none;

                auto pos = commandString.find(' ');

                if (pos == 0) // string starts with space -> invalid
                    return boost::none;

                if (pos == CommandString::npos) // no space in commandString -> no args and commandString is the full command
                    return boost::optional<std::pair<CommandString, CommandString>>{ std::make_pair(commandString, "") };

                auto command = commandString.substr(0, pos);
                auto length = commandString.length();

                while (pos < length && commandString[pos] == ' ') { pos++; } // skip spaces

                if (pos < length)
                    return boost::optional<std::pair<CommandString, CommandString>>{ std::make_pair(command, commandString.substr(pos)) };
                else
                    return boost::optional<std::pair<CommandString, CommandString>>{ std::make_pair(command, "") };
            }

        }
    }
}
