#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "Accounts/AccountMgr.h"
#include "Entities/Player/PlayerCache.hpp"
#include "Globals/ObjectAccessor.h"
#include "World/World.h"

#include "Chat/Command/CommandDefines.hpp"
#include "Chat/Command/ExecutionContext.hpp"

namespace chat {
    namespace command {
        namespace util {

            // Split a raw command into 2 parts. Example: "ban account Blub" -> "ban", "account Blub"
            // if the result is not boost::none, it guarantees that pair.first contains a valid (not empty, not containing whitespace) command name
            boost::optional<std::pair<CommandString, CommandString>> SplitCommandString(const CommandString& commandString);
            
            template<class Range>
            void PrintCommands(Range& range, ExecutionContext& context, const std::string& format = "%s")
            {
                for (auto& entry : range)
                {
                    if (entry.GetMask() & CommandFlag::HIDDEN)
                        continue;

                    auto meta = entry.ToMetaCommand();
                    if (meta && meta->HasAvailableSubCommands(context))
                        context.SendSystemMessage(format, entry.GetName() + " ...");
                    else
                        context.SendSystemMessage(format, entry.GetName());
                }
            }

            class GAME_API Argument
            {
            public:
                Argument(const CommandString& _value) : value(_value) {};

                CommandString GetStringView() const { return value; };

                // Note: Use GetStringView() if possible
                std::string GetString() const { return std::string(value); };

                boost::optional<int32> GetInt32() const
                {
                    try { return boost::lexical_cast<int32>(value); }
                    catch (const boost::bad_lexical_cast&) { return boost::none; }
                };

                boost::optional<uint32> GetUInt32() const
                {
                    try { return boost::lexical_cast<uint32>(value); }
                    catch (const boost::bad_lexical_cast&) { return boost::none; }
                }

                boost::optional<float> GetFloat() const
                {
                    try { return boost::lexical_cast<float>(value); }
                    catch (const boost::bad_lexical_cast&) { return boost::none; }
                };

                // returns the player id of the player with the given name
                boost::optional<ObjectGuid> GetPlayerId() const
                {
                    if (ObjectGuid id = player::PlayerCache::GetGUID(std::string(value)))
                        return id;

                    return boost::none;
                }

                // returns the player with the given name if online
                boost::optional<Player&> GetPlayer() const
                {
                    if (auto player = sObjectAccessor->FindPlayerByName(GetString()))
                        return *player;

                    return boost::none;
                }

                // returns the account id of the account with the given name
                boost::optional<uint32> GetAccountId() const
                {
                    if (uint32 id = sAccountMgr->GetId(std::string(value)))
                        return id;

                    return boost::none;
                }

                // returns the session of the account with the given name if online
                boost::optional<WorldSession&> GetSession() const
                {
                    if (auto id = GetAccountId())
                        if (auto session = sWorld->FindSession(*id))
                            return *session;

                    return boost::none;
                }

            private:
                const CommandString value;
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

            using ArgumentList = std::vector<Argument>;

            ArgumentList ParseArguments(const CommandString& argumentString);

        }
    }
}
