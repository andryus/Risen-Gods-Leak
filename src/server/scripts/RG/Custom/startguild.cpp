#include "Chat/Chat.h"
#include "Entities/Player/Player.h"
#include "Guilds/Guild.h"
#include "Guilds/GuildMgr.h"
#include "Miscellaneous/Language.h"
#include "Scripting/ScriptMgr.h"
#include "World/World.h"

class rg_startguild_playerscript : public PlayerScript
{
public:
    rg_startguild_playerscript() : PlayerScript("rg_startguild_playerscript") {}

    void OnLevelChanged(Player* player, uint8 oldLevel)
    {
        if (oldLevel == 1 && !player->IsGameMaster())
        {
            if (player->GetGuildId() == 0)
            {
                if (auto startGuild = sGuildMgr->GetGuildByName(sWorld->GetStartguildName(player->GetOTeamId())))
                {
                    auto trans = CharacterDatabase.BeginTransaction();
                    if (startGuild->AddMember(trans, player->GetGUID()))
                    {
                        ChatHandler(player->GetSession()).SendSysMessage(LANG_JOIN_START_GUILD_NOTICE);
                        CharacterDatabase.CommitTransaction(trans);
                    }
                }
            }
        }
    }
};

class rg_startguild_guildscript : public GuildScript
{
public:
    rg_startguild_guildscript() : GuildScript("rg_startguild_guildscript") {}

    void OnAddMember(Guild* guild, Player* player, uint8& /*plRank*/)
    {
        if (!player)
            return;

        if (guild->GetName() == sWorld->GetStartguildName(player->GetOTeamId()))
        {
            auto trans = CharacterDatabase.BeginTransaction();
            //check the max size of usefull members and kick long inactive member
            const uint32 maxMember = sWorld->getIntConfig(CONFIG_STARTGUILD_MAX_PLAYER);
            for (ObjectGuid inactiveMember = guild->GetMaxDaysOfflineMember(); guild->GetMemberCount() > maxMember && inactiveMember; inactiveMember = guild->GetMaxDaysOfflineMember())
                guild->DeleteMember(trans, inactiveMember, false, true);

            //to prevent ninja the guild bank on first day
            for (uint8 i = 0; i <= GUILD_BANK_MAX_TABS; i++)
                guild->UpdateBankWithdrawValueForMember(trans, player->GetGUID(), i, 0);
            CharacterDatabase.CommitTransaction(trans);
        }
    }
};

void AddSC_custom_startguild()
{
    new rg_startguild_playerscript();
    new rg_startguild_guildscript();
}
