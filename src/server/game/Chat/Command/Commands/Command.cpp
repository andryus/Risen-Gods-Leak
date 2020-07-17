#include "Chat/Command/Commands/Command.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "Graylog/Graylog.hpp"
#include "Log.h"
#include "RG/Logging/LogManager.hpp"

#include "Miscellaneous/Language.h"
#include "Entities/Player/Player.h"
#include "Server/WorldSession.h"
#include "Maps/Map.h"
#include "DBCStores.h"

#include "Chat/Command/ExecutionContext.hpp"

namespace chat {
    namespace command {

        Command::Command(CommandDefinition definition) :
            id(          std::move( definition.id          )),
            sortingKey(  std::move( definition.sortingKey  )),
            name(        std::move( definition.name        )),
            syntax(      std::move( definition.syntax      )),
            description( std::move( definition.description )),
            permission(  std::move( definition.permission  )),
            mask(        std::move( definition.mask        ))
        {}

        void Command::Initialize(std::string _fullName)
        {
            fullName = std::move(_fullName);
        }

        bool Command::IsAvailable(const ExecutionContext& _context) const
        {
            return _context.HasPermission(permission);
        }

        void Command::HandleHelp(const CommandString& _command, ExecutionContext& _context) const
        {
            if (!IsAvailable(_context))
            {
                _context.SendSystemMessage(LANG_COMMAND_NOT_AVAILABLE);
                return;
            }

            PrintHelp(_context);
        }

        void Command::PrintHelp(ExecutionContext& _context) const
        {
            _context.SendSystemMessage(
                LANG_COMMAND_HELP_TEXT,
                GetFullName(),
                _context.GetString(GetSyntax()),
                _context.GetString(GetDescription())
            );
        }

        void Command::Log(const CommandString& args, const ExecutionContext& context) const
        {
            if (GetMask() & CommandFlag::DISABLE_LOGGING)
                return;

            const std::string argsStr{ args };

            switch (context.GetSource())
            {

                case ExecutionContext::Source::CHAT:
                {
                    const WorldSession& session = context.GetSession().get();
                    const ChatHandler& chatHandler = context.GetChatHandler();
                    Player* player = session.GetPlayer();

                    if (!AccountMgr::IsPlayerAccount(session.GetSecurity()))
                    {
                        ObjectGuid  guid = player->GetTarget();
                        std::string mapName = "Unknown";
                        if (auto* map = player->FindMap())
                            mapName = map->GetMapName();
                        uint32 areaId = player->GetAreaId();
                        std::string areaName = "Unknown";
                        std::string zoneName = "Unknown";
                        if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId))
                        {
                            int locale = chatHandler.GetSessionDbcLocale();
                            areaName = area->area_name[locale];
                            if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(area->zone))
                                zoneName = zone->area_name[locale];
                        }

                        sLog->outCommand(session.GetAccountId(), "Command: %s [Player: %s (%s) (Account: %u) X: %f Y: %f Z: %f Map: %u (%s) Area: %u (%s) Zone: %s Selected %s: %s (%s)]",
                            GetFullName() + (argsStr.size() > 0 ? " " + argsStr : ""), player->GetName().c_str(), player->GetGUID().ToString().c_str(),
                            session.GetAccountId(), player->GetPositionX(), player->GetPositionY(),
                            player->GetPositionZ(), player->GetMapId(), mapName,
                            areaId, areaName.c_str(), zoneName.c_str(), guid.GetTypeName().data(),
                            (player->GetSelectedUnit()) ? player->GetSelectedUnit()->GetName().c_str() : "",
                            guid.ToString().c_str());

                        RG_LOG<GMLogModule>(session.GetAccountId(), player, GetFullName(), argsStr);

#ifndef TRINITY_WITHOUT_GELFCPP
                        if (auto output = sGraylog->GetOutput())
                        {
                            gelfcpp::GelfMessageBuilder builder;
                            builder(sGraylog->GetDecorator());
                            builder("is_structured", true);
                            builder(Trinity::StringFormat("Chat Command: '%s %s' executed by '%s' (%u) %u",
                                GetFullName(), argsStr, player->GetName(), player->GetGUID().GetCounter(), session.GetAccountId()));
                            builder("log_type", "chat.command");
                            builder("chat_command_name", GetFullName());
                            builder("chat_command_param", argsStr);
                            builder("chat_command_invoker_name", player->GetName());
                            builder("chat_command_invoker_guid", player->GetGUID().GetCounter());
                            builder("chat_command_invoker_account_id", session.GetAccountId());
                            builder("chat_command_invoker_pos_x", player->GetPositionX());
                            builder("chat_command_invoker_pos_y", player->GetPositionY());
                            builder("chat_command_invoker_pos_z", player->GetPositionZ());
                            builder("chat_command_invoker_pos_map_id", player->GetMapId());
                            builder("chat_command_invoker_pos_map_name", mapName);
                            builder("chat_command_invoker_pos_area_id", areaId);
                            builder("chat_command_invoker_pos_area_name", areaName);
                            builder("chat_command_invoker_pos_zone_name", zoneName);
                            if (auto target = player->GetSelectedUnit())
                            {
                                builder("chat_command_target_type", guid.GetTypeName().data());
                                builder("chat_command_target_entry", target->GetGUID().GetEntry());
                                builder("chat_command_target_guid", guid.GetCounter());
                                builder("chat_command_target_name", target->GetName());
                                builder("chat_command_target_is_gm", target->GetTypeId() == TYPEID_PLAYER ?
                                    (AccountMgr::IsStaffAccount(target->ToPlayer()->GetSession()->GetSecurity())) :
                                    false);
                            }
                            output->Write(builder);
                        }
#endif
                    }

                    break;
                }

                case ExecutionContext::Source::CONSOLE:
                {
                    const CliHandler& cliHandler = dynamic_cast<const CliHandler&>(context.GetChatHandler());
                    const uint32 accountId = cliHandler.getAccountId();
                    const std::string& accountName = cliHandler.getAccountName();

#ifndef TRINITY_WITHOUT_GELFCPP
                    if (auto output = sGraylog->GetOutput())
                    {
                        using namespace std::string_literals;

                        gelfcpp::GelfMessageBuilder builder;
                        builder(sGraylog->GetDecorator());
                        builder("is_structured", true);
                        builder(Trinity::StringFormat("Chat Command: '%s %s' executed by console", GetFullName(), argsStr));
                        builder("log_type", "chat.command"s);
                        builder("chat_command_name", GetFullName());
                        builder("chat_command_param", argsStr);
                        builder("chat_command_invoker_name", accountId == 0 ? "<console>"s : accountName);
                        builder("chat_command_invoker_guid", 0);
                        builder("chat_command_invoker_account_id", accountId);
                        output->Write(builder);
                    }
#endif

                    if (!accountId) // ignore command line
                        return;

                    sLog->outCommand(
                        accountId,
                        "Cli Command: %s [Account: %s (%u)]",
                        GetFullName() + (argsStr.size() > 0 ? " " + argsStr : ""),
                        accountName,
                        accountId
                    );
                    
                    RG_LOG<GMLogModule>(accountId, GetFullName(), argsStr);

                    break;
                }

                default: ASSERT(false);
            }
        }

    }
}
