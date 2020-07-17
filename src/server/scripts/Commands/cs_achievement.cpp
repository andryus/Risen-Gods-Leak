/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: achievement_commandscript
%Complete: 100
Comment: All achievement related commands
Category: commandscripts
EndScriptData */

#include "AchievementMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/SimpleCommand.hpp"
#include "Chat/Command/GenericCommandScriptLoader.hpp"
#include "Language.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace chat { namespace command { namespace handler {

    class AchievementAddCommand : public SimpleCommand
    {
    public:
        explicit AchievementAddCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), false) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.empty())
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            const util::Argument& arg = args[0];

            uint32 achievementId;
            if (auto achievementIdOpt = arg.GetUInt32())
            {
                achievementId = *achievementIdOpt;
            }
            else
            {
                std::string achievementString = args[0].GetString();
                if (char* id = context.GetChatHandler().extractKeyFromLink(achievementString.data(), "Hachievement"))
                    achievementId = atoi(id);
                else
                    return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            }

            Player* target = context.GetChatHandler().getSelectedPlayerOrSelf();
            if (!target)
            {
                context.SendSystemMessage(LANG_NO_CHAR_SELECTED);
                return ExecutionResult::FAILURE;
            }

            if (AchievementEntry const* achievementEntry = sAchievementMgr->GetAchievement(achievementId))
                target->CompletedAchievement(achievementEntry);

            return ExecutionResult::SUCCESS;
        };
    };

    class AchievementDeleteCommand : public SimpleCommand
    {
    public:
        explicit AchievementDeleteCommand(CommandDefinition definition) : SimpleCommand(std::move(definition), false) {}

        ExecutionResult Execute(const util::ArgumentList& args, ExecutionContext& context) override
        {
            if (args.empty())
                return ExecutionResult::FAILURE_MISSING_ARGUMENT;

            const util::Argument& arg = args[0];

            uint32 achievementId;
            if (auto achievementIdOpt = arg.GetUInt32())
            {
                achievementId = *achievementIdOpt;
            }
            else
            {
                std::string achievementString = arg.GetString();
                if (char* id = context.GetChatHandler().extractKeyFromLink(achievementString.data(), "Hachievement"))
                    achievementId = atoi(id);
                else
                    return ExecutionResult::FAILURE_INVALID_ARGUMENT;
            }

            Player* target = context.GetChatHandler().getSelectedPlayerOrSelf();
            if (!target)
            {
                context.SendSystemMessage(LANG_NO_CHAR_SELECTED);
                return ExecutionResult::FAILURE;
            }

            target->RemoveAchievement(achievementId);

            return ExecutionResult::SUCCESS;
        };
    };

}}}

void AddSC_achievement_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new GenericCommandScriptLoader<AchievementAddCommand>("cmd_achievement_add");
    new GenericCommandScriptLoader<AchievementDeleteCommand>("cmd_achievement_delete");
}
