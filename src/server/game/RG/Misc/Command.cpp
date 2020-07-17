#include "Command.hpp"

#include <set>
#include "Config.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Language.h"

namespace RG
{
    namespace Command
    {
        namespace
        {
            typedef std::set<uint32> QuestCompleteWhitelist;
            QuestCompleteWhitelist _quest_complete_whitelist;
        }

        void ReloadQuestCompleteWhitelist()
        {
            TC_LOG_INFO("misc", "RG: Loading command_quest_complete_whitelist...");

            if (!sConfigMgr->GetBoolDefault("RG.Database", false))
            {
                TC_LOG_INFO("misc", "RG: Skipped because RG database is unavailable");
                return;
            }

            _quest_complete_whitelist.clear();

            uint32 count = 0;
            if (QueryResult result = RGDatabase.Query("SELECT quest_id FROM command_quest_complete_whitelist"))
            {
                do
                {
                    _quest_complete_whitelist.insert((*result)[0].GetUInt32());
                    ++count;
                } while (result->NextRow());
            }
            TC_LOG_INFO("misc", "RG: Loaded %u command_quest_complete_whitelist entries", count);
        }

        bool IsQuestCompleteQuestWhitelisted(uint32 quest_id)
        {
            if (!sConfigMgr->GetBoolDefault("RG.Database", false))
                return true;

            return _quest_complete_whitelist.count(quest_id) > 0;
        }



        //
        // Script Loader and Hooks
        //
        namespace
        {
            class Loader : public WorldScript
            {
            public:
                Loader() : WorldScript("rg_misc_command_worldscript") {}

                void OnLoad() override
                {
                    ReloadQuestCompleteWhitelist();
                }
            };
        }


        bool HandleReloadQuestCompleteWhitelistCommand(ChatHandler* handler, const char* /*args*/)
        {
            ReloadQuestCompleteWhitelist();
            return true;
        }

        void AddSC_rg_misc_command()
        {
            new Loader();
            new chat::command::LegacyCommandScript("cmd_rg_reload_quest_complete_whitelist", HandleReloadQuestCompleteWhitelistCommand, true);
        }
    }
}
