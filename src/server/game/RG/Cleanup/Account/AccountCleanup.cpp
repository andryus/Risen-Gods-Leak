#include "AccountCleanup.h"

#include <fstream>
#include "AccountMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Config.h"
#include "Player.h"
#include "PlayerDump.h"
#include "ScriptMgr.h"
#include "Threading/ThreadPool.hpp"

//! causes the regexp to be always false (end of line cannot be before beginning)
static const char* ALWAYS_FALSE_REGEXP = "$^";

namespace RG
{
namespace Cleanup
{
namespace Account
{
    typedef std::set<uint32> AccountSet;

    namespace Config
    {
        bool Active = false;
        std::string CharacterDatabaseName = "";
        std::string DumpPath = "";
        uint32 GMLevel = GMLEVEL_PREMIUM;
        uint32 DeleteLevel = 60;
        AccountSet Whitelist;
        uint32 MaxWaitDays = 90;
        uint32 KeepSafeDays = 365;

        namespace Inactive
        {
            uint32 Days = 2 * 365;
        }
        namespace Unused
        {
            uint32 Days = 30;
            uint32 MaxLevel = 1;
            uint32 Count = 1;
        }
        namespace Empty
        {
            uint32 Days = 14;
        }
        namespace Banned
        {
            uint32 Days = 182;
            std::set<std::string> Excluded = {};
            std::string ExcludedCompiled = ALWAYS_FALSE_REGEXP;
        }
    }

    bool _cancelCurrentCommand;
    util::thread::ThreadPool _executor;

    class Task
    {
    public:
        enum Type
        {
            FIND,
            STATE,
            DEL,
            ALL,
        };

        Task(Type type) : _type(type) {}

        void operator()()
        {
            _cancelCurrentCommand = false;

            switch (_type)
            {
            case FIND:
                UpdateList();
                break;
            case STATE:
                UpdateState();
                break;
            case DEL:
                ProcessList();
                break;
            case ALL:
                UpdateList();
                UpdateState();
                ProcessList();
                break;
            }

            if (_cancelCurrentCommand)
                _cancelCurrentCommand = false;
        }

    private:
        Type _type;
    };

    class CommandHandler
    {
    public:
        static bool HandleReload(ChatHandler* handler, const char* /*args*/);
        static bool HandleRun(ChatHandler* handler, const char* /*args*/);
        static bool HandleFind(ChatHandler* handler, const char* /*args*/);
        static bool HandleUpdate(ChatHandler* handler, const char* /*args*/);
        static bool HandleDelete(ChatHandler* handler, const char* /*args*/);
        static bool HandleCancel(ChatHandler* handler, const char* /*args*/);
    };

    bool ActivateIfNeeded();
}
}
}

bool RG::Cleanup::Account::ActivateIfNeeded()
{
    if (!Config::Active)
    {
        TC_LOG_INFO("rg.cleanup.account", "Account: Cleanup deactivated.");
        return false;
    }   

    if (!_executor.is_running())
    {
        _executor.start(1);
        _cancelCurrentCommand = false;
    }

    return true;
}

void RG::Cleanup::Account::Run()
{
    if (!ActivateIfNeeded())
        return;

    TC_LOG_INFO("rg.cleanup.account", "Account: Scheduled full cleanup run");
    _executor.schedule(Task(Task::ALL));
}

void RG::Cleanup::Account::UpdateList()
{
    TC_LOG_INFO("rg.cleanup.account", "Account: Starting search for new accounts...");
    uint32 startTime = getMSTime();

    AccountSet inactiveAccountIds;
    AccountSet unusedAccountIds;
    AccountSet emptyAccountIds;
    AccountSet bannedAccountIds;

    // Totally inactive accounts
    QueryResult result = LoginDatabase.PQuery("SELECT id FROM account WHERE last_login < NOW() - INTERVAL %u DAY", Config::Inactive::Days);
    if (result)
    {
        do
        {
            inactiveAccountIds.insert((*result)[0].GetUInt32());
        } while (result->NextRow());
    }

    // Account not actively played anymore
    result = LoginDatabase.PQuery("SELECT c.account FROM %s.characters AS c JOIN account AS a ON c.account = a.id "
                                  "WHERE a.last_login < NOW() - INTERVAL %u DAY GROUP BY c.account HAVING COUNT(*) <= %u AND MAX(c.level) <= %u",
                                  Config::CharacterDatabaseName.c_str(), Config::Unused::Days, Config::Unused::Count, Config::Unused::MaxLevel);
    if (result)
    {
        do
        {
            unusedAccountIds.insert((*result)[0].GetUInt32());
        } while (result->NextRow());
    }

    // Account has no characters
    result = LoginDatabase.PQuery("SELECT id FROM account WHERE id NOT IN (SELECT DISTINCT account FROM %s.characters) AND last_login < NOW() - INTERVAL %u DAY", Config::CharacterDatabaseName.c_str(), Config::Empty::Days);
    if (result)
    {
        do
        {
            emptyAccountIds.insert((*result)[0].GetUInt32());
        } while (result->NextRow());
    }

    // Account is banned
    if (Config::Banned::Days)
    {
        result = LoginDatabase.PQuery(
            "SELECT ab.id "
            "FROM account_banned AS ab "
            "JOIN account AS a ON a.id = ab.id "
            "WHERE ab.bandate = ab.unbandate AND ab.active = 1 AND "
            "   ab.unbandate < UNIX_TIMESTAMP(NOW() - INTERVAL %u DAY) AND "
            "   banreason NOT REGEXP '(%s)'", Config::Banned::Days, Config::Banned::ExcludedCompiled.c_str());
        if (result)
        {
            do
            {
                bannedAccountIds.insert((*result)[0].GetUInt32());
            } while (result->NextRow());
        }
    }

    // Exclude guild leaders
    result = CharacterDatabase.Query("SELECT DISTINCT c.account FROM guild AS g JOIN characters AS c ON c.guid = g.leaderguid");
    if (result)
    {
        do
        {
            uint32 id = (*result)[0].GetUInt32();
            inactiveAccountIds.erase(id);
            unusedAccountIds.erase(id);
            emptyAccountIds.erase(id);
            bannedAccountIds.erase(id);
        } while (result->NextRow());
    }

    // Exclude Premium and Team accounts
    result = LoginDatabase.PQuery("SELECT id FROM account_access WHERE gmlevel >= %u", Config::GMLevel);
    if (result)
    {
        do
        {
            uint32 id = (*result)[0].GetUInt32();
            inactiveAccountIds.erase(id);
            unusedAccountIds.erase(id);
            emptyAccountIds.erase(id);
            bannedAccountIds.erase(id);
        } while (result->NextRow());
    }

    // Exclude accounts from team members
    result = LoginDatabase.PQuery("SELECT id FROM account WHERE username REGEXP '^(PVE_)?((HP)?DEV|TESTER|GM|HP|MOD|[GV]FX)-'");
    if (result)
    {
        do
        {
            uint32 id = (*result)[0].GetUInt32();
            inactiveAccountIds.erase(id);
            unusedAccountIds.erase(id);
            emptyAccountIds.erase(id);
            bannedAccountIds.erase(id);
        } while (result->NextRow());
    }

    // Exclude accounts if last login has not happened yet but account is still new
    result = LoginDatabase.PQuery("SELECT id FROM account WHERE last_login = '0000-00-00 00:00:00' AND DATEDIFF(now(), joindate) < %u", Config::Inactive::Days);
    if (result)
    {
        do
        {
            uint32 id = (*result)[0].GetUInt32();
            inactiveAccountIds.erase(id);
            unusedAccountIds.erase(id);
            emptyAccountIds.erase(id);
            bannedAccountIds.erase(id);
        } while (result->NextRow());
    }

    // Exclude whitelisted accounts
    for (AccountSet::const_iterator itr = Config::Whitelist.begin(); itr != Config::Whitelist.end(); ++itr)
    {
        inactiveAccountIds.erase(*itr);
        unusedAccountIds.erase(*itr);
        emptyAccountIds.erase(*itr);
        bannedAccountIds.erase(*itr);
    }

    // Exclude already queued accounts
    result = LoginDatabase.Query("SELECT id FROM cleanup_account");
    if (result)
    {
        do
        {
            uint32 id = (*result)[0].GetUInt32();
            inactiveAccountIds.erase(id);
            unusedAccountIds.erase(id);
            emptyAccountIds.erase(id);
            bannedAccountIds.erase(id);
        } while (result->NextRow());
    }

    // REPLACE INTO so database handles duplicate entries and priority of deletion reason

    for (AccountSet::const_iterator itr = inactiveAccountIds.begin(); itr != inactiveAccountIds.end(); ++itr)
        LoginDatabase.PExecute("REPLACE INTO cleanup_account (id, username, email, token, reason) SELECT id, username, email, SHA1(UPPER(CONCAT(id, ':', username))), 'INACTIVE' FROM account WHERE id = %u", *itr);

    for (AccountSet::const_iterator itr = unusedAccountIds.begin(); itr != unusedAccountIds.end(); ++itr)
        LoginDatabase.PExecute("REPLACE INTO cleanup_account (id, username, email, token, reason) SELECT id, username, email, SHA1(UPPER(CONCAT(id, ':', username))), 'UNUSED' FROM account WHERE id = %u", *itr);

    for (AccountSet::const_iterator itr = emptyAccountIds.begin(); itr != emptyAccountIds.end(); ++itr)
        LoginDatabase.PExecute("REPLACE INTO cleanup_account (id, username, email, token, reason) SELECT id, username, email, SHA1(UPPER(CONCAT(id, ':', username))), 'EMPTY' FROM account WHERE id = %u", *itr);

    for (AccountSet::const_iterator itr = bannedAccountIds.begin(); itr != bannedAccountIds.end(); ++itr)
        LoginDatabase.PExecute("REPLACE INTO cleanup_account (id, username, email, token, reason, status) SELECT id, username, email, SHA1(UPPER(CONCAT(id, ':', username))), 'BANNED', 'READY' FROM account WHERE id = %u", *itr);

    TC_LOG_INFO("rg.cleanup.account", "Account: Found %zu inactive, %zu unused/discarded, %zu empty and %zu banned new accounts in %u ms", inactiveAccountIds.size(), unusedAccountIds.size(), emptyAccountIds.size(), bannedAccountIds.size(), GetMSTimeDiffToNow(startTime));
}

void RG::Cleanup::Account::UpdateState()
{
    TC_LOG_INFO("rg.cleanup.account", "Account: Start updating states...");
    uint32 startTime = getMSTime();

    // Check if account aquired premium status
    LoginDatabase.DirectPExecute("DELETE FROM cleanup_account WHERE status NOT IN ('KEEP', 'DELETED') AND id IN (SELECT id FROM account_access WHERE gmlevel >= %u)", Config::GMLevel);
    // Check if account has logged in
    LoginDatabase.DirectExecute("DELETE c FROM cleanup_account AS c JOIN account AS a ON (c.id = a.id) WHERE status NOT IN ('KEEP', 'DELETED') AND a.last_login > c.found_time");

    // Mark accounts ready if email is not provided
    LoginDatabase.DirectExecute("UPDATE cleanup_account SET status = 'READY' WHERE status NOT IN ('KEEP', 'DELETED') AND email = ''");
    // Mark banned accounts as ready
    LoginDatabase.DirectExecute("UPDATE cleanup_account SET status = 'READY' WHERE status NOT IN ('DELETED') AND reason = 'BANNED'");
    // Mark stalled accounts ready if the where pending too long
    if (Config::MaxWaitDays)
        LoginDatabase.DirectPExecute("UPDATE cleanup_account SET status = 'READY' WHERE status NOT IN ('KEEP', 'DELETED') AND found_time < NOW() - INTERVAL %u DAY", Config::MaxWaitDays);
    
    // Remove accounts if keep safe expired
    if (Config::KeepSafeDays)
        LoginDatabase.DirectPExecute("DELETE FROM cleanup_account WHERE status = 'KEEP' AND found_time < NOW() - INTERVAL %u DAY", Config::KeepSafeDays);

    TC_LOG_INFO("rg.cleanup.account", "Account: Updated states in %u ms", GetMSTimeDiffToNow(startTime));
}

void RG::Cleanup::Account::ProcessList()
{
    TC_LOG_INFO("rg.cleanup.account", "Account: Start deleting pending accounts...");
    uint32 startTime = getMSTime();

    QueryResult result = LoginDatabase.Query("SELECT id, username, reason FROM cleanup_account WHERE status = 'READY'");
    if (!result)
    {
        TC_LOG_INFO("rg.cleanup.account", "Account: No accounts are to be deleted.");
        return;
    }

    uint32 total = 0;
    uint32 deleted = 0;
    uint32 chars = 0;
    do
    {
        Field* row = result->Fetch();
        uint32 id = row[0].GetUInt32();
        std::string username = row[1].GetString();
        std::string reason = row[2].GetString();

        uint32 removed = 0;
        if (ProcessAccount(id, username, reason, removed))
            ++deleted;

        chars += removed;

        ++total;
    } while (result->NextRow() && !_cancelCurrentCommand);

    if (_cancelCurrentCommand)
        TC_LOG_INFO("rg.cleanup.account", "Account: Processing canceled!");

    TC_LOG_INFO("rg.cleanup.account", "Account: Deleted %u out of %u pending accounts. Total %u characters were erased. Time: %u ms", deleted, total, chars, GetMSTimeDiffToNow(startTime));
}

uint32 RG::Cleanup::Account::ProcessAccount(uint32 accountId, const std::string& username, const std::string& reason, uint32& removed)
{
    bool fullDelete = (reason.compare("UNUSED") == 0) || (reason.compare("EMPTY") == 0) || (reason.compare("BANNED") == 0);

    TC_LOG_DEBUG("rg.cleanup.account", "Account: [%u] %s %s deleting for %s", accountId, username.c_str(), fullDelete ? "full" : "partial", reason.c_str());

    QueryResult result = CharacterDatabase.PQuery("SELECT guid, name, level FROM characters WHERE account = %u", accountId);

    uint32 total = 0;
    uint32 deleted = 0;
    if (result)
    {
        do
        {
            Field* row = result->Fetch();
            uint32 guid = row[0].GetUInt32();
            std::string name = row[1].GetString();
            uint8 level = row[2].GetUInt8();

            if (level < Config::DeleteLevel || fullDelete)
            {
                std::ostringstream os;
                os << Config::DumpPath << "/" << accountId << "-" << username << "_" << guid << "-" << name << ".dump";
                DumpReturn res = PlayerDumpWriter().WriteDump(os.str(), guid);
                if (res != DUMP_SUCCESS)
                {
                    TC_LOG_ERROR("rg.cleanup.account", "Account: [%u] %s Unable to dump character [%u] %s Reason: %u Account skipped", accountId, username.c_str(), guid, name.c_str(), res);
                    return false;
                }

                Player::DeleteFromDB(ObjectGuid(HighGuid::Player, guid), accountId, false, true);

                TC_LOG_TRACE("rg.cleanup.account", "Account: [%u] %s Deleted character: [%u] %s Level: %u", accountId, username.c_str(), guid, name.c_str(), level);
                ++deleted;
            }
            else
                TC_LOG_TRACE("rg.cleanup.account", "Account: [%u] %s Skipped character: [%u] %s Level: %u", accountId, username.c_str(), guid, name.c_str(), level);

            ++total;
        } while (result->NextRow());
    }
    else
        TC_LOG_DEBUG("rg.cleanup.account", "Account: [%u] %s has no characters.", accountId, username.c_str());


    if (deleted == total || fullDelete)
    {
        DumpAccountInfo(accountId, username);
#ifndef RG_RESTRICTED_LOGON_ACCESS
        AccountMgr::DeleteAccount(accountId);
        TC_LOG_INFO("rg.cleanup.account", "Account: [%u] %s Full delete %u of %u characters (Forced: %s)", accountId, username.c_str(), deleted, total, fullDelete ? "YES" : "NO");
#else
        TC_LOG_INFO("rg.cleanup.account", "Account: [%u] %s Full delete %u of %u characters (Forced: %s), account deletion disabled", accountId, username.c_str(), deleted, total, fullDelete ? "YES" : "NO");
#endif
    }
    else
        TC_LOG_INFO("rg.cleanup.account", "Account: [%u] %s Partial delete %u of %u characters.", accountId, username.c_str(), deleted, total);

    removed = deleted;

    LoginDatabase.DirectPExecute("UPDATE cleanup_account SET delete_time = NOW(), status = 'DELETED', characters = %u WHERE id = %u", deleted, accountId);

    return true;
}

void RG::Cleanup::Account::DumpAccountInfo(uint32 accountId, const std::string& username)
{
    std::ostringstream path;
    path << Config::DumpPath << "/" << accountId << "-" << username << ".info";

    std::ofstream os(path.str().c_str(), std::ios::out);
    os << "ID: " << accountId << "\n";
    os << "Name: " << username << "\n";

    QueryResult result = LoginDatabase.PQuery("SELECT email, UNIX_TIMESTAMP(joindate), UNIX_TIMESTAMP(last_login) FROM account WHERE id = %u", accountId);
    if (result)
    {
        os << "E-Mail: " << (*result)[0].GetString() << "\n";
        os << "Join Date: " << (*result)[1].GetUInt32() << "\n";
        os << "Last Login: " << (*result)[2].GetUInt32() << "\n";
    }
    result = LoginDatabase.PQuery("SELECT bandate, unbandate, bannedby, banreason, active FROM account_banned WHERE id = %u", accountId);
    if (result)
    {
        os << "Ban Info:\n";
        do
        {
            os << "Bandate: " << (*result)[0].GetUInt32() << "\n";
            os << "Unbandate: " << (*result)[1].GetUInt32() << "\n";
            os << "By: " << (*result)[2].GetString() << "\n";
            os << "Reason: " << (*result)[3].GetString() << "\n";
            os << "Active: " << (*result)[4].GetUInt8() << "\n";
            os << "\n";
        } while (result->NextRow());
    }
    os.flush();
    os.close();
}

void RG::Cleanup::Account::LoadFromConfig(bool reload)
{
    if (reload)
    {
        std::string error_like_i_just_dont_care;
        sConfigMgr->Reload(error_like_i_just_dont_care);
    }

    Config::Active           = sConfigMgr->GetBoolDefault  ("RG.Cleanup.Account.Active",        false);
    Config::DeleteLevel      = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.DeleteLevel",    60);
    Config::DumpPath         = sConfigMgr->GetStringDefault("RG.Cleanup.Account.DumpPath",       "");
    Config::GMLevel          = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.GMLevel",       GMLEVEL_PREMIUM);
    Config::MaxWaitDays      = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.MaxWaitDays",    90);
    Config::KeepSafeDays     = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.KeepSafeDays",  365);
    Config::Inactive::Days   = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.Inactive.Days", 365);
    Config::Empty::Days      = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.Empty.Days",     14);
    Config::Unused::Days     = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.Unused.Days",    30);
    Config::Unused::MaxLevel = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.Unused.Level",    1);
    Config::Unused::Count    = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.Unused.Count",    1);
    Config::Banned::Days     = sConfigMgr->GetIntDefault   ("RG.Cleanup.Account.Banned.Days",   365);

    std::string whitelistString = sConfigMgr->GetStringDefault("RG.Cleanup.Account.Whitelist", "");
    Tokenizer whitelistToken(whitelistString, ',');
    for (Tokenizer::const_iterator itr = whitelistToken.begin(); itr != whitelistToken.end(); ++itr)
        Config::Whitelist.insert(atol(*itr));

    std::string bannedExlcudedReasons = sConfigMgr->GetStringDefault("RG.Cleanup.Account.Banned.ExcludedReasons", "");
    Tokenizer bannedExcludedToken(bannedExlcudedReasons, ',');
    for (Tokenizer::const_iterator itr = bannedExcludedToken.begin(); itr != bannedExcludedToken.end(); ++itr)
    {
        std::string reason(*itr);
        size_t pos1 = reason.find_first_not_of(" ");
        size_t pos2 = reason.find_last_not_of(" ");
        reason = reason.substr(pos1, pos2 + 1);

        Config::Banned::Excluded.insert(reason);
    }

    if (!Config::Banned::Excluded.empty())
    {
        std::ostringstream os;
        std::ostream_iterator<std::string> os_itr(os, "|");

        std::copy(Config::Banned::Excluded.begin(), Config::Banned::Excluded.end(), os_itr);

        Config::Banned::ExcludedCompiled = os.str();
        Config::Banned::ExcludedCompiled.pop_back();
    }
    else
        Config::Banned::ExcludedCompiled = ALWAYS_FALSE_REGEXP;

    std::string dbString = sConfigMgr->GetStringDefault("CharacterDatabaseInfo", "a;b;c;d;wotlk_chars");
    Tokenizer dbToken(dbString, ';');
    Tokenizer::const_iterator itr = dbToken.begin();
    std::advance(itr, 4);
    Config::CharacterDatabaseName = *itr;

    TC_LOG_INFO("rg.cleanup.account", "Account: Loaded config");
    TC_LOG_INFO("rg.cleanup.account", "  Active: %s", Config::Active ? "yes" : "no");
    TC_LOG_INFO("rg.cleanup.account", "  DeleteLevel: %u, GMLevel: %u", Config::DeleteLevel, Config::GMLevel);
    TC_LOG_INFO("rg.cleanup.account", "  DumpPath: %s", Config::DumpPath.c_str());
    TC_LOG_INFO("rg.cleanup.account", "  Whitelist: %s", whitelistString.c_str());
    TC_LOG_INFO("rg.cleanup.account", "  MaxWaitDays: %u", Config::MaxWaitDays);
    TC_LOG_INFO("rg.cleanup.account", "  KeepSafeDays: %u", Config::KeepSafeDays);
    TC_LOG_INFO("rg.cleanup.account", "  Inactive Days: %u", Config::Inactive::Days);
    TC_LOG_INFO("rg.cleanup.account", "  Empty    Days: %u", Config::Empty::Days);
    TC_LOG_INFO("rg.cleanup.account", "  Unused   Days: %u", Config::Unused::Days);
    TC_LOG_INFO("rg.cleanup.account", "  Unused   Level: %u", Config::Unused::MaxLevel);
    TC_LOG_INFO("rg.cleanup.account", "  Unused   Count: %u", Config::Unused::Count);
    TC_LOG_INFO("rg.cleanup.account", "  Banned   Days: %u", Config::Banned::Days);
    TC_LOG_INFO("rg.cleanup.account", "  Banned   Excluded (RegExp): %s", Config::Banned::ExcludedCompiled.c_str());

    if (sConfigMgr->GetIntDefault("LoginDatabase.SynchThreads", 1) == 1)
        TC_LOG_WARN("rg.cleanup.account", "It is recommended to have more than one synch logon database thread to prevent stalling.");
    if (sConfigMgr->GetIntDefault("CharacterDatabase.SynchThreads", 1) == 1)
        TC_LOG_WARN("rg.cleanup.account", "It is recommended to have more than one synch character database thread to prevent stalling.");
}

bool RG::Cleanup::Account::CommandHandler::HandleReload(ChatHandler* handler, const char* /*args*/)
{
    LoadFromConfig(true);

    handler->PSendSysMessage("Cleanup: Account: Reloaded config");
    return true;
}

bool RG::Cleanup::Account::CommandHandler::HandleRun(ChatHandler* handler, const char* /*args*/)
{
    Run();

    handler->PSendSysMessage("Cleanup: Account: Scheduled full run");
    return true;
}

bool RG::Cleanup::Account::CommandHandler::HandleFind(ChatHandler* handler, const char* /*args*/)
{
    if (ActivateIfNeeded())
    {
        _executor.schedule(Task(Task::FIND));
        handler->PSendSysMessage("Cleanup: Account: Scheduled search for new accounts");
    }
    else
        handler->PSendSysMessage("Cleanup: Account: Deactivated or unable to start process!");

    return true;
}

bool RG::Cleanup::Account::CommandHandler::HandleUpdate(ChatHandler* handler, const char* /*args*/)
{
    if (ActivateIfNeeded())
    {
        _executor.schedule(Task(Task::STATE));
        handler->PSendSysMessage("Cleanup: Account: Scheduled state update for pending accounts");
    }
    else
        handler->PSendSysMessage("Cleanup: Account: Deactivated or unable to start process!");

    return true;
}

bool RG::Cleanup::Account::CommandHandler::HandleDelete(ChatHandler* handler, const char* /*args*/)
{
    if (ActivateIfNeeded())
    {
        _executor.schedule(Task(Task::DEL));
        handler->PSendSysMessage("Cleanup: Account: Scheduled deletion of pending accounts");
    }
    else
        handler->PSendSysMessage("Cleanup: Account: Deactivated or unable to start process!");
    
    return true;
}

bool RG::Cleanup::Account::CommandHandler::HandleCancel(ChatHandler* handler, const char* /*args*/)
{
    if (_cancelCurrentCommand)
        handler->PSendSysMessage("Cleanup: Account: Already canceled.");
    else
    {
        handler->PSendSysMessage("Cleanup: Account: Canceling requested");
        _cancelCurrentCommand = true;
    }

    return true;
}

void AddSC_rg_accountcleanup_commandscript()
{
    using namespace chat::command;
    using namespace RG::Cleanup::Account;

    new LegacyCommandScript("cmd_accountcleanup_reload", CommandHandler::HandleReload, true);
    new LegacyCommandScript("cmd_accountcleanup_run", CommandHandler::HandleRun, true);
    new LegacyCommandScript("cmd_accountcleanup_find", CommandHandler::HandleFind, true);
    new LegacyCommandScript("cmd_accountcleanup_update", CommandHandler::HandleUpdate, true);
    new LegacyCommandScript("cmd_accountcleanup_delete", CommandHandler::HandleDelete, true);
    new LegacyCommandScript("cmd_accountcleanup_cancel", CommandHandler::HandleCancel, true);
}
