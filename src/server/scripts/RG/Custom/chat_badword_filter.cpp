#include "Common.h"
#include "DatabaseEnv.h"
#include "Duration.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "WorldSession.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include <cwctype>

struct BlankCharCheck
{
    static std::string BLANK_CHARS;

    bool operator()(char c)
    {
        return BLANK_CHARS.find(c) != std::string::npos;
    }
};

std::string BlankCharCheck::BLANK_CHARS = ".-_ \t\n\r";
static bool ACTIVE = false;

enum SpamReaction
{
    ACTION_NONE,
    ACTION_KICK,
    ACTION_BAN_ACCOUNT,
    ACTION_BAN_IP
};

enum SpamChat
{
    CHAT_SAY,
    CHAT_CHANNEL,
    CHAT_PARTY,
    CHAT_GUILD,
    CHAT_WHISPER
};

struct SpamEntry
{
    std::string text;
    bool isRegexp;
    uint8 chats;
    SpamReaction reaction;
    uint32 time;
};

typedef std::list<SpamEntry> SpamEntryList;
static SpamEntryList ENTRIES;

class AntiSpamPlayerScript : public PlayerScript
{
public:
    AntiSpamPlayerScript() : PlayerScript("rg_anti_spam_player") {}


    inline void Check(Player* player, std::string& msg, SpamChat chat)
    {
        std::string ref;
        ref.resize(msg.size());
        std::transform(msg.begin(), msg.end(), ref.begin(), std::towlower);
        std::remove_if(ref.begin(), ref.end(), BlankCharCheck());

        for (SpamEntryList::const_iterator itr = ENTRIES.begin(); itr != ENTRIES.end(); ++itr)
        {
            const SpamEntry& entry = *itr;
            
            if (!(entry.chats & (1 << chat)))
                continue;

            bool passed = true;

            // NYI
            //if (entry.isRegexp)
            //    ; 
            //else
            {
                passed = ref.find(entry.text) == std::string::npos;
            }
            
            if (!passed)
            {
                msg = "";

                switch (entry.reaction)
                {
                default:
                case ACTION_NONE:
                    break;
                case ACTION_BAN_ACCOUNT:
                {
                    sWorld->BanAccountById(
                        player->GetSession()->GetAccountId(),
                        Seconds(entry.time),
                        "Antispam Autoban",
                        "Server: Antispam"
                    );
                    break;
                }
                case ACTION_BAN_IP:
                {
                    sWorld->BanAccountByIp(
                        player->GetSession()->GetRemoteAddress(),
                        Seconds(entry.time),
                        "Antispam Autoban",
                        "Server: Antispam"
                    );
                    break;
                }
                case ACTION_KICK:
                    player->GetSession()->KickPlayer();
                    break;
                }
            }
        }
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg)
    {
        if (lang == LANG_ADDON || type == CHAT_MSG_ADDON)
            return;

        Check(player, msg, CHAT_SAY);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* /*receiver*/) 
    {
        if (lang == LANG_ADDON || type == CHAT_MSG_ADDON)
            return;

        Check(player, msg, CHAT_WHISPER);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* /*group*/) 
    {
        if (lang == LANG_ADDON || type == CHAT_MSG_ADDON)
            return;

        Check(player, msg, CHAT_PARTY);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Guild* /*guild*/) 
    {
        if (lang == LANG_ADDON || type == CHAT_MSG_ADDON)
            return;

        Check(player, msg, CHAT_GUILD);
    }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Channel* /*channel*/) 
    {
        if (lang == LANG_ADDON || type == CHAT_MSG_ADDON)
            return;

        Check(player, msg, CHAT_CHANNEL);
    }
};

class AntiSpamWorldScript : public WorldScript
{
public:
    AntiSpamWorldScript() : WorldScript("rg_anti_spam_world") { }

    void OnLoad()
    {
        Load();
    }

    static void Load()
    {
        ENTRIES.clear();

        QueryResult result = WorldDatabase.Query("SELECT id, text, `regexp`, chats, reaction, time FROM rg_antispam");
        if (!result)
            return;

        do
        {
            Field* fields = result->Fetch();

            int32 id = fields[0].GetInt32();
            if (id < 0)
                HandleConfig(uint32(-id), fields);
            else
                HandleEntry(fields);

        } while (result->NextRow());
    }


    static void HandleConfig(uint32 id, Field* fields)
    {
        const char* value = fields[1].GetCString();

        switch (id)
        {
        case 1: // Active
            ACTIVE = atoi(value);
            break;
        case 2: // Blank Chars
            BlankCharCheck::BLANK_CHARS.assign(value);
            break;
        default:
            TC_LOG_ERROR("server.loading", "Anitspam: Unknown config option %u (%s)", id, value);
        }
    }

    static void HandleEntry(Field* fields)
    {
        SpamEntry entry;
        entry.text      = fields[1].GetString();
        entry.isRegexp  = fields[2].GetBool();
        entry.chats     = fields[3].GetUInt8();
        entry.reaction  = SpamReaction(fields[4].GetUInt8());
        entry.time      = fields[5].GetUInt32();

        ENTRIES.push_back(entry);
    }
};

bool HandleReloadAntiSpam(ChatHandler* handler, const char* /*args*/)
{
    AntiSpamWorldScript::Load();

    handler->PSendSysMessage("Reloaded antispam.");
    return true;
}

void AddSC_custom_antispam()
{
    new AntiSpamPlayerScript();
    new AntiSpamWorldScript();
    new chat::command::LegacyCommandScript("cmd_rg_antispam_reload", HandleReloadAntiSpam, true);
}
