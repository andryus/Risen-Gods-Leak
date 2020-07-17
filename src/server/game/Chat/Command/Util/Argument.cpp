#include "Chat/Command/Util/Argument.hpp"

#include <sstream>
#include <utility>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include "Errors.h"

#include "Accounts/AccountMgr.h"
#include "Entities/Player/PlayerCache.hpp"
#include "Globals/ObjectAccessor.h"
#include "World/World.h"


namespace chat { namespace command { namespace util
{
    Argument::Argument(std::string value) :
        value(std::move(value))
    {}

    std::string_view Argument::GetStringView() const
    {
        return std::string_view(value);
    }

    std::string Argument::GetString() const
    {
        return value;
    }

    boost::optional<int32> Argument::GetInt32() const
    {
        try { return boost::lexical_cast<int32>(value); }
        catch (const boost::bad_lexical_cast&) { return boost::none; }
    }

    boost::optional<uint32> Argument::GetUInt32() const
    {
        try { return boost::lexical_cast<uint32>(value); }
        catch (const boost::bad_lexical_cast&) { return boost::none; }
    }

    boost::optional<float> Argument::GetFloat() const
    {
        try { return boost::lexical_cast<float>(value); }
        catch (const boost::bad_lexical_cast&) { return boost::none; }
    }

    boost::optional<ObjectGuid> GetPlayerGUID(const Argument& arg)
    {
        if (const ObjectGuid guid = player::PlayerCache::GetGUID(arg.GetString()))
            return guid;

        return boost::none;
    }

    boost::optional<Player&> GetPlayer(const Argument& arg)
    {
        if (Player* player = sObjectAccessor->FindPlayerByName(arg.GetString()))
            return *player;

        return boost::none;
    }

    boost::optional<AccountId> GetAccountId(const Argument& arg)
    {
        if (const AccountId id = sAccountMgr->GetId(arg.GetString()))
            return id;

        return boost::none;
    }

    boost::optional<WorldSession&> GetSession(const Argument& arg)
    {
        if (auto id = GetAccountId(arg))
            if (WorldSession* session = sWorld->FindSession(*id))
                return *session;

        return boost::none;
    }


    ArgumentParser::ArgumentParser(CommandString argumentString):
        argString(argumentString),
        length(argumentString.size())
    {}

    ArgumentList ArgumentParser::Parse()
    {
        return ParseArguments();
    }

    ArgumentList ArgumentParser::ParseArguments()
    {
        ArgumentList args{};

        SkipSpaces();
        while (position < length)
        {
            args.push_back(ParseArgument());
            SkipSpaces();
        }

        return args;
    }

    Argument ArgumentParser::ParseArgument()
    {
        ASSERT(position < length);
        ASSERT(argString[position] != ' ');

        if (argString[position] == '"' || argString[position] == '\'')
        {
            // quote escaped argument -> parse until next unescaped occurence of the same quote

            const char quoteChar = argString[position];
            ++position;

            std::stringstream buffer{};
            bool escaped = false;

            while (position < length)
            {
                const char currentChar = argString[position];

                if (escaped)
                {
                    buffer << currentChar;
                    escaped = false;
                }
                else if (currentChar == '\\')
                {
                    escaped = true;
                }
                else if (currentChar == quoteChar)
                {
                    ++position;
                    break;
                }
                else
                {
                    buffer << currentChar;
                }

                ++position;
            }

            return Argument(buffer.str());
        }
        else if (boost::algorithm::starts_with(argString.substr(position), "|c"))
        {
            // chat link -> parse until |r

            const size_t start = position;
            const size_t end = argString.find("|r", position);

            if (end == CommandString::npos) // not found -> use the entire remaining string
            {
                position = length;
                return Argument(std::string(argString.substr(start)));
            }

            position = end + 2;
            return Argument(std::string(argString.substr(
                start,
                end - start + 2 // + 2 to include the |r
            )));
        }
        else
        {
            // normal, unescaped argument -> parse until next space

            const size_t start = position;

            while (position < length && argString[position] != ' ')
                ++position;

            return Argument(std::string(argString.substr(start, position - start)));
        }
    }

    void ArgumentParser::SkipSpaces()
    {
        while (position < length && argString[position] == ' ')
            ++position;
    }
}}}
