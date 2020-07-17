#pragma once

#include <string>
#include <string_view>

#include "Common.h"

#include "Chat/Command/CommandDefines.hpp"
#include "Entities/Object/ObjectGuid.h"

class Player;
class WorldSession;

namespace chat::command::util
{

    class GAME_API Argument
    {
    public:
        explicit Argument(std::string value);

        std::string_view GetStringView() const;
        // Note: Use GetStringView() if possible
        std::string GetString() const;
        boost::optional<int32> GetInt32() const;
        boost::optional<uint32> GetUInt32() const;
        boost::optional<float> GetFloat() const;

    private:
        const std::string value;
    };

    inline bool operator==(const Argument& arg1, const Argument& arg2) { return arg1.GetStringView() == arg2.GetStringView(); }
    inline bool operator!=(const Argument& arg1, const Argument& arg2) { return !(arg1 == arg2); }

    inline bool operator==(const Argument& arg, const char* str) { return arg.GetStringView().compare(str) == 0; }
    inline bool operator==(const char* str, const Argument& arg) { return arg == str; }
    inline bool operator!=(const Argument& arg, const char* str) { return !(arg == str); }
    inline bool operator!=(const char* str, const Argument& arg) { return arg != str; }

    inline bool operator==(const Argument& arg, const std::string& str) { return arg == str.c_str(); }
    inline bool operator==(const std::string& str, const Argument& arg) { return arg == str; }
    inline bool operator!=(const Argument& arg, const std::string& str) { return !(arg == str); }
    inline bool operator!=(const std::string& str, const Argument& arg) { return arg != str; }


    // returns the player id of the player with the given name
    boost::optional<ObjectGuid> GetPlayerGUID(const Argument& arg);

    // returns the player with the given name if online
    boost::optional<Player&> GetPlayer(const Argument& arg);

    // returns the account id of the account with the given name
    boost::optional<AccountId> GetAccountId(const Argument& arg);

    // returns the session of the account with the given name if online
    boost::optional<WorldSession&> GetSession(const Argument& arg);


    using ArgumentList = std::vector<Argument>;


    class ArgumentParser
    {
    private:
        CommandString argString;
        size_t position = 0; // current position in the argument string
        size_t length;

    public:
        explicit ArgumentParser(CommandString argumentString);

        ArgumentList Parse();

    private:
        ArgumentList ParseArguments();
        Argument ParseArgument();
        void SkipSpaces();
    };

}
