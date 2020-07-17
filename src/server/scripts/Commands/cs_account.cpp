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
Name: account_commandscript
%Complete: 100
Comment: All account related commands
Category: commandscripts
EndScriptData */

#include "AccountMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "IpAddress.h"
#include "Language.h"
#include "Player.h"
#include "SHA1.h"

namespace chat { namespace command { namespace handler {

#ifndef RG_RESTRICTED_LOGON_ACCESS
    bool HandleAccountAddonCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* exp = strtok((char*)args, " ");

        uint32 account_id = handler->GetSession()->GetAccountId();

        int expansion = atoi(exp); //get int anyway (0 if error)
        if (expansion < 0 || uint8(expansion) > sWorld->getIntConfig(CONFIG_EXPANSION))
        {
            handler->SendSysMessage(LANG_IMPROPER_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->setUInt8(0, uint8(expansion));
        stmt->setUInt32(1, account_id);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_ADDON, expansion);
        return true;
    }

    /// Create an account
    bool HandleAccountCreateCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        std::string email;

        ///- %Parse the command line arguments
        char* accountName = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        char* possibleEmail = strtok(NULL, " ' ");
        if (possibleEmail)
            email = possibleEmail;

        if (!accountName || !password)
            return false;

        AccountOpResult result = sAccountMgr->CreateAccount(std::string(accountName), std::string(password), email);
        switch (result)
        {
        case AccountOpResult::AOR_OK:
            handler->PSendSysMessage(LANG_ACCOUNT_CREATED, accountName);
            if (handler->GetSession())
            {
                TC_LOG_INFO("entities.player.character", "Account: %d (IP: %s) Character:[%s] (GUID: %u) created Account %s (Email: '%s')",
                    handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                    handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter(),
                    accountName, email.c_str());
            }
            break;
        case AccountOpResult::AOR_NAME_TOO_LONG:
            handler->SendSysMessage(LANG_ACCOUNT_TOO_LONG);
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_NAME_ALREADY_EXIST:
            handler->SendSysMessage(LANG_ACCOUNT_ALREADY_EXIST);
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_DB_INTERNAL_ERROR:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED_SQL_ERROR, accountName);
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_CREATED, accountName);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    /// Delete a user account and all associated characters in this realm
    /// @todo This function has to be enhanced to respect the login/realm split (delete char, delete account chars in realm then delete account)
    bool HandleAccountDeleteCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the account name from the command line
        char *account_name_str = strtok((char*)args, " ");
        if (!account_name_str)
            return false;

        uint32 account_id = 0;
        std::string account_name = account_name_str;

        if (!isNumeric(account_name_str))
        {
            if (!Utf8ToUpperOnlyLatin(account_name))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
            account_id = AccountMgr::GetId(account_name);
        }
        else
            account_id = atoi(account_name_str);

        if (!account_id)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// Commands not recommended call from chat, but support anyway
        /// can delete only for account with less security
        /// This is also reject self apply in fact
        if (handler->HasLowerSecurityAccount(NULL, account_id, true))
            return false;

        AccountOpResult result = AccountMgr::DeleteAccount(account_id);
        switch (result)
        {
        case AccountOpResult::AOR_OK:
            handler->PSendSysMessage(LANG_ACCOUNT_DELETED, account_name.c_str());
            break;
        case AccountOpResult::AOR_NAME_NOT_EXIST:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_DB_INTERNAL_ERROR:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED_SQL_ERROR, account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_DELETED, account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }
#endif

    /// Display info on users currently in the realm
    bool HandleAccountOnlineListCommand(ChatHandler* handler, char const* /*args*/)
    {
        ///- Get the list of accounts ID logged to the realm
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ONLINE);

        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (!result)
        {
            handler->SendSysMessage(LANG_ACCOUNT_LIST_EMPTY);
            return true;
        }

        ///- Display the list of account/characters online
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_HEADER);
        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);

        ///- Cycle through accounts
        do
        {
            Field* fieldsDB = result->Fetch();
            std::string name = fieldsDB[0].GetString();
            uint32 account = fieldsDB[1].GetUInt32();

            ///- Get the username, last IP and GM level of each account
            // No SQL injection. account is uint32.
            stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO);
            stmt->setUInt32(0, account);
            PreparedQueryResult resultLogin = LoginDatabase.Query(stmt);

            if (resultLogin)
            {
                Field* fieldsLogin = resultLogin->Fetch();
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_LINE,
                    fieldsLogin[0].GetCString(), name.c_str(), fieldsLogin[1].GetCString(),
                    fieldsDB[2].GetUInt16(), fieldsDB[3].GetUInt16(), fieldsLogin[3].GetUInt8(),
                    fieldsLogin[2].GetUInt8());
            }
            else
                handler->PSendSysMessage(LANG_ACCOUNT_LIST_ERROR, name.c_str());
        } while (result->NextRow());

        handler->SendSysMessage(LANG_ACCOUNT_LIST_BAR);
        return true;
    }

#ifndef RG_RESTRICTED_LOGON_ACCESS
    bool HandleAccountLockCountryCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_USE_BOL);
            handler->SetSentErrorMessage(true);
            return false;
        }
        std::string param = (char*)args;

        if (!param.empty())
        {
            if (param == "on")
            {
                PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_LOGON_COUNTRY);
                uint32 ip = inet_addr(handler->GetSession()->GetRemoteAddress().c_str());
                EndianConvertReverse(ip);
                stmt->setUInt32(0, ip);
                PreparedQueryResult result = LoginDatabase.Query(stmt);
                if (result)
                {
                    Field* fields = result->Fetch();
                    std::string country = fields[0].GetString();
                    stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK_CONTRY);
                    stmt->setString(0, country);
                    stmt->setUInt32(1, handler->GetSession()->GetAccountId());
                    LoginDatabase.Execute(stmt);
                    handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
                }
                else
                {
                    handler->PSendSysMessage("[IP2NATION] Table empty");
                    TC_LOG_DEBUG("server.authserver", "[IP2NATION] Table empty");
                }
            }
            else if (param == "off")
            {
                PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK_CONTRY);
                stmt->setString(0, "00");
                stmt->setUInt32(1, handler->GetSession()->GetAccountId());
                LoginDatabase.Execute(stmt);
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            }
            return true;
        }
        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    bool HandleAccountLockIpCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_USE_BOL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string param = (char*)args;

        if (!param.empty())
        {
            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK);

            if (param == "on")
            {
                stmt->setBool(0, true);                                     // locked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
            }
            else if (param == "off")
            {
                stmt->setBool(0, false);                                    // unlocked
                handler->PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
            }

            stmt->setUInt32(1, handler->GetSession()->GetAccountId());

            LoginDatabase.Execute(stmt);
            return true;
        }

        handler->SendSysMessage(LANG_USE_BOL);
        handler->SetSentErrorMessage(true);
        return false;
    }

    bool HandleAccountEmailCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char* oldEmail = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        char* email = strtok(NULL, " ");
        char* emailConfirmation = strtok(NULL, " ");

        if (!oldEmail || !password || !email || !emailConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!AccountMgr::CheckEmail(handler->GetSession()->GetAccountId(), std::string(oldEmail)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGEMAIL);
            sScriptMgr->OnFailedEmailChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change email, but the provided email [%s] is not equal to registration email [%s].",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter(),
                email, oldEmail);
            return false;
        }

        if (!AccountMgr::CheckPassword(handler->GetSession()->GetAccountId(), std::string(password)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
            sScriptMgr->OnFailedEmailChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change email, but the provided password is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
            return false;
        }

        if (strcmp(email, oldEmail) == 0)
        {
            handler->SendSysMessage(LANG_OLD_EMAIL_IS_NEW_EMAIL);
            sScriptMgr->OnFailedEmailChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (strcmp(email, emailConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_EMAILS_NOT_MATCH);
            sScriptMgr->OnFailedEmailChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change email, but the provided password is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
            return false;
        }


        AccountOpResult result = AccountMgr::ChangeEmail(handler->GetSession()->GetAccountId(), std::string(email));
        switch (result)
        {
        case AccountOpResult::AOR_OK:
            handler->SendSysMessage(LANG_COMMAND_EMAIL);
            sScriptMgr->OnEmailChange(handler->GetSession()->GetAccountId());
            TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Changed Email from [%s] to [%s].",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter(),
                oldEmail, email);
            break;
        case AccountOpResult::AOR_EMAIL_TOO_LONG:
            handler->SendSysMessage(LANG_EMAIL_TOO_LONG);
            sScriptMgr->OnFailedEmailChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->SendSysMessage(LANG_COMMAND_NOTCHANGEEMAIL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    bool HandleAccountPasswordCommand(ChatHandler* handler, char const* args)
    {
        // If no args are given at all, we can return false right away.
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // First, we check config. What security type (sec type) is it ? Depending on it, the command branches out
        uint32 pwConfig = sWorld->getIntConfig(CONFIG_ACC_PASSCHANGESEC); // 0 - PW_NONE, 1 - PW_EMAIL, 2 - PW_RBAC

                                                                            // Command is supposed to be: .account password [$oldpassword] [$newpassword] [$newpasswordconfirmation] [$emailconfirmation]
        char* oldPassword = strtok((char*)args, " ");       // This extracts [$oldpassword]
        char* newPassword = strtok(NULL, " ");              // This extracts [$newpassword]
        char* passwordConfirmation = strtok(NULL, " ");     // This extracts [$newpasswordconfirmation]
        char const* emailConfirmation = strtok(NULL, " ");  // This defines the emailConfirmation variable, which is optional depending on sec type.
        if (!emailConfirmation)                             // This extracts [$emailconfirmation]. If it doesn't exist, however...
            emailConfirmation = "";                         // ... it's simply "" for emailConfirmation.

                                                            //Is any of those variables missing for any reason ? We return false.
        if (!oldPassword || !newPassword || !passwordConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // We compare the old, saved password to the entered old password - no chance for the unauthorized.
        if (!AccountMgr::CheckPassword(handler->GetSession()->GetAccountId(), std::string(oldPassword)))
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
            sScriptMgr->OnFailedPasswordChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change password, but the provided old password is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
            return false;
        }

        // This compares the old, current email to the entered email - however, only...
        if ((pwConfig == PW_EMAIL || (pwConfig == PW_RBAC && handler->HasPermission(rbac::RBAC_PERM_EMAIL_CONFIRM_FOR_PASS_CHANGE))) // ...if either PW_EMAIL or PW_RBAC with the Permission is active...
            && !AccountMgr::CheckEmail(handler->GetSession()->GetAccountId(), std::string(emailConfirmation))) // ... and returns false if the comparison fails.
        {
            handler->SendSysMessage(LANG_COMMAND_WRONGEMAIL);
            sScriptMgr->OnFailedPasswordChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Tried to change password, but the entered email [%s] is wrong.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter(),
                emailConfirmation);
            return false;
        }

        // Making sure that newly entered password is correctly entered.
        if (strcmp(newPassword, passwordConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            sScriptMgr->OnFailedPasswordChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Changes password and prints result.
        AccountOpResult result = AccountMgr::ChangePassword(handler->GetSession()->GetAccountId(), std::string(newPassword));
        switch (result)
        {
        case AccountOpResult::AOR_OK:
            handler->SendSysMessage(LANG_COMMAND_PASSWORD);
            sScriptMgr->OnPasswordChange(handler->GetSession()->GetAccountId());
            TC_LOG_INFO("entities.player.character", "Account: %u (IP: %s) Character:[%s] (GUID: %u) Changed Password.",
                handler->GetSession()->GetAccountId(), handler->GetSession()->GetRemoteAddress().c_str(),
                handler->GetSession()->GetPlayer()->GetName().c_str(), handler->GetSession()->GetPlayer()->GetGUID().GetCounter());
            break;
        case AccountOpResult::AOR_PASS_TOO_LONG:
            handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
            sScriptMgr->OnFailedPasswordChange(handler->GetSession()->GetAccountId());
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }
#endif

    bool HandleAccountCommand(ChatHandler* handler, char const* /*args*/)
    {
        // GM Level
        uint32 gmLevel = handler->GetSession()->GetSecurity();
        handler->PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmLevel));

        // Security level required
        bool hasRBAC = (handler->HasPermission(rbac::RBAC_PERM_EMAIL_CONFIRM_FOR_PASS_CHANGE) ? true : false);
        uint32 pwConfig = sWorld->getIntConfig(CONFIG_ACC_PASSCHANGESEC); // 0 - PW_NONE, 1 - PW_EMAIL, 2 - PW_RBAC

        handler->PSendSysMessage(LANG_ACCOUNT_SEC_TYPE, (pwConfig == PW_NONE ? "Lowest level: No Email input required." :
            pwConfig == PW_EMAIL ? "Highest level: Email input required." :
            pwConfig == PW_RBAC ? "Special level: Your account may require email input depending on settings. That is the case if another lien is printed." :
            "Unknown security level: Notify technician for details."));

        // RBAC required display - is not displayed for console
        if (pwConfig == PW_RBAC && handler->GetSession() && hasRBAC)
            handler->PSendSysMessage(LANG_RBAC_EMAIL_REQUIRED);

        // Email display if sufficient rights
        if (handler->HasPermission(rbac::RBAC_PERM_MAY_CHECK_OWN_EMAIL))
        {
            std::string emailoutput;
            uint32 accountId = handler->GetSession()->GetAccountId();

            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_GET_EMAIL_BY_ID);
            stmt->setUInt32(0, accountId);
            PreparedQueryResult result = LoginDatabase.Query(stmt);

            if (result)
            {
                emailoutput = (*result)[0].GetString();
                handler->PSendSysMessage(LANG_COMMAND_EMAIL_OUTPUT, emailoutput.c_str());
            }
        }

        return true;
    }

#ifndef RG_RESTRICTED_LOGON_ACCESS
    /// Set/Unset the expansion level for an account
    bool HandleAccountSetAddonCommand(ChatHandler* handler, char const* args)
    {
        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* exp = strtok(NULL, " ");

        if (!account)
            return false;

        std::string account_name;
        uint32 account_id;

        if (!exp)
        {
            Player* player = handler->getSelectedPlayer();
            if (!player)
                return false;

            account_id = player->GetSession()->GetAccountId();
            AccountMgr::GetName(account_id, account_name);
            exp = account;
        }
        else
        {
            ///- Convert Account name to Upper Format
            account_name = account;
            if (!Utf8ToUpperOnlyLatin(account_name))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }

            account_id = AccountMgr::GetId(account_name);
            if (!account_id)
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Let set addon state only for lesser (strong) security level
        // or to self account
        if (handler->GetSession() && handler->GetSession()->GetAccountId() != account_id &&
            handler->HasLowerSecurityAccount(NULL, account_id, true))
            return false;

        int expansion = atoi(exp); //get int anyway (0 if error)
        if (expansion < 0 || uint8(expansion) > sWorld->getIntConfig(CONFIG_EXPANSION))
            return false;

        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

        stmt->setUInt8(0, expansion);
        stmt->setUInt32(1, account_id);

        LoginDatabase.Execute(stmt);

        handler->PSendSysMessage(LANG_ACCOUNT_SETADDON, account_name.c_str(), account_id, expansion);
        return true;
    }
#endif

    bool HandleAccountSetGmLevelCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string targetAccountName;
        uint32 targetAccountId = 0;
        uint32 targetSecurity = 0;
        uint32 gm = 0;
        char* arg1 = strtok((char*)args, " ");
        char* arg2 = strtok(NULL, " ");
        char* arg3 = strtok(NULL, " ");
        bool isAccountNameGiven = true;

        if (!arg3)
        {
            if (!handler->getSelectedPlayer())
                return false;
            isAccountNameGiven = false;
        }

        // Check for second parameter
        if (!isAccountNameGiven && !arg2)
            return false;

        // Check for account
        if (isAccountNameGiven)
        {
            targetAccountName = arg1;
            if (!Utf8ToUpperOnlyLatin(targetAccountName) || !AccountMgr::GetId(targetAccountName))
            {
                handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, targetAccountName.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Check for invalid specified GM level.
        gm = (isAccountNameGiven) ? atoi(arg2) : atoi(arg1);
        if (gm > SEC_CONSOLE)
        {
            handler->SendSysMessage(LANG_BAD_VALUE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // handler->getSession() == NULL only for console
        targetAccountId = (isAccountNameGiven) ? AccountMgr::GetId(targetAccountName) : handler->getSelectedPlayer()->GetSession()->GetAccountId();
        int32 gmRealmID = (isAccountNameGiven) ? atoi(arg3) : atoi(arg2);
        uint32 playerSecurity;
        if (handler->GetSession())
            playerSecurity = AccountMgr::GetSecurity(handler->GetSession()->GetAccountId(), gmRealmID);
        else
            playerSecurity = SEC_CONSOLE;

        // can set security level only for target with less security and to less security that we have
        // This also restricts setting handler's own security.
        targetSecurity = AccountMgr::GetSecurity(targetAccountId, gmRealmID);
        if (targetSecurity >= playerSecurity || gm >= playerSecurity)
        {
            handler->SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // Check and abort if the target gm has a higher rank on one of the realms and the new realm is -1
        if (gmRealmID == -1 && !AccountMgr::IsConsoleAccount(playerSecurity))
        {
            PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_ACCESS_GMLEVEL_TEST);

            stmt->setUInt32(0, targetAccountId);
            stmt->setUInt8(1, uint8(gm));

            PreparedQueryResult result = LoginDatabase.Query(stmt);

            if (result)
            {
                handler->SendSysMessage(LANG_YOURS_SECURITY_IS_LOW);
                handler->SetSentErrorMessage(true);
                return false;
            }
        }

        // Check if provided realmID has a negative value other than -1
        if (gmRealmID < -1)
        {
            handler->SendSysMessage(LANG_INVALID_REALMID);
            handler->SetSentErrorMessage(true);
            return false;
        }

        rbac::RBACData* rbac = isAccountNameGiven ? NULL : handler->getSelectedPlayer()->GetSession()->GetRBACData();
        sAccountMgr->UpdateAccountAccess(rbac, targetAccountId, uint8(gm), gmRealmID);

        handler->PSendSysMessage(LANG_YOU_CHANGE_SECURITY, targetAccountName.c_str(), gm);
        return true;
    }

#ifndef RG_RESTRICTED_LOGON_ACCESS
    /// Set password for account
    bool HandleAccountSetPasswordCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* password = strtok(NULL, " ");
        char* passwordConfirmation = strtok(NULL, " ");

        if (!account || !password || !passwordConfirmation)
            return false;

        std::string account_name = account;
        if (!Utf8ToUpperOnlyLatin(account_name))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(account_name);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set password only for target with less security
        /// This also restricts setting handler's own password
        if (handler->HasLowerSecurityAccount(NULL, targetAccountId, true))
            return false;

        if (strcmp(password, passwordConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangePassword(targetAccountId, password);

        switch (result)
        {
        case AccountOpResult::AOR_OK:
            handler->SendSysMessage(LANG_COMMAND_PASSWORD);
            break;
        case AccountOpResult::AOR_NAME_NOT_EXIST:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_PASS_TOO_LONG:
            handler->SendSysMessage(LANG_PASSWORD_TOO_LONG);
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
            handler->SetSentErrorMessage(true);
            return false;
        }
        return true;
    }

    /// Set normal email for account
    bool HandleAccountSetEmailCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* email = strtok(NULL, " ");
        char* emailConfirmation = strtok(NULL, " ");

        if (!account || !email || !emailConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string accountName = account;
        if (!Utf8ToUpperOnlyLatin(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set email only for target with less security
        /// This also restricts setting handler's own email.
        if (handler->HasLowerSecurityAccount(NULL, targetAccountId, true))
            return false;

        if (strcmp(email, emailConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_EMAILS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangeEmail(targetAccountId, email);
        switch (result)
        {
        case AccountOpResult::AOR_OK:
            handler->SendSysMessage(LANG_COMMAND_EMAIL);
            TC_LOG_INFO("entities.player.character", "ChangeEmail: Account %s [Id: %u] had it's email changed to %s.",
                accountName.c_str(), targetAccountId, email);
            break;
        case AccountOpResult::AOR_NAME_NOT_EXIST:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_EMAIL_TOO_LONG:
            handler->SendSysMessage(LANG_EMAIL_TOO_LONG);
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->SendSysMessage(LANG_COMMAND_NOTCHANGEEMAIL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

    /// Change registration email for account
    bool HandleAccountSetRegEmailCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        //- We do not want anything short of console to use this by default.
        //- So we force that.
        if (handler->GetSession())
            return false;

        ///- Get the command line arguments
        char* account = strtok((char*)args, " ");
        char* email = strtok(NULL, " ");
        char* emailConfirmation = strtok(NULL, " ");

        if (!account || !email || !emailConfirmation)
        {
            handler->SendSysMessage(LANG_CMD_SYNTAX);
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string accountName = account;
        if (!Utf8ToUpperOnlyLatin(accountName))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 targetAccountId = AccountMgr::GetId(accountName);
        if (!targetAccountId)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        /// can set email only for target with less security
        /// This also restricts setting handler's own email.
        if (handler->HasLowerSecurityAccount(NULL, targetAccountId, true))
            return false;

        if (strcmp(email, emailConfirmation) != 0)
        {
            handler->SendSysMessage(LANG_NEW_EMAILS_NOT_MATCH);
            handler->SetSentErrorMessage(true);
            return false;
        }

        AccountOpResult result = AccountMgr::ChangeRegEmail(targetAccountId, email);
        switch (result)
        {
        case AccountOpResult::AOR_OK:
            handler->SendSysMessage(LANG_COMMAND_EMAIL);
            TC_LOG_INFO("entities.player.character", "ChangeRegEmail: Account %s [Id: %u] had it's Registration Email changed to %s.",
                accountName.c_str(), targetAccountId, email);
            break;
        case AccountOpResult::AOR_NAME_NOT_EXIST:
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        case AccountOpResult::AOR_EMAIL_TOO_LONG:
            handler->SendSysMessage(LANG_EMAIL_TOO_LONG);
            handler->SetSentErrorMessage(true);
            return false;
        default:
            handler->SendSysMessage(LANG_COMMAND_NOTCHANGEEMAIL);
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }
#endif

    /// generate account verify code with SHA1
    bool HandleAccountVerifyCommand(ChatHandler* handler, char const* /*args*/)
    {
        std::string name;
        sAccountMgr->GetName(handler->GetSession()->GetAccountId(), name);

        time_t t = time(0);
        struct tm * now = localtime(&t);
        std::ostringstream os;
        os << now->tm_mday << '/'
            << (now->tm_mon + 1) << '/'
            << (now->tm_year + 1900);

        SHA1Hash sha;
        sha.Initialize();
        sha.UpdateData(name);
        sha.UpdateData(":");
        sha.UpdateData(os.str());
        sha.UpdateData(":");
        sha.UpdateData(sWorld->GetAccountVerifySecret());
        sha.Finalize();

        std::string hash = ByteArrayToHexStr(sha.GetDigest(), sha.GetLength());

        std::string shortHash = hash.substr(0, sWorld->getIntConfig(CONFIG_ACCOUNT_VERIFY_LENGTH));
        handler->PSendSysMessage(LANG_ACCOUNT_VERIFY_CODE, shortHash.c_str());

        return true;
    }

}}}

void AddSC_account_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_account",                  HandleAccountCommand,               false);

    new LegacyCommandScript("cmd_account_onlinelist",       HandleAccountOnlineListCommand,     true);
    new LegacyCommandScript("cmd_account_verify",           HandleAccountVerifyCommand,         false);

    new LegacyCommandScript("cmd_account_set_gmlevel",      HandleAccountSetGmLevelCommand,     true);

#ifndef RG_RESTRICTED_LOGON_ACCESS
    new LegacyCommandScript("cmd_account_addon",            HandleAccountAddonCommand,          false);
    new LegacyCommandScript("cmd_account_create",           HandleAccountCreateCommand,         true);
    new LegacyCommandScript("cmd_account_delete",           HandleAccountDeleteCommand,         true);
    new LegacyCommandScript("cmd_account_email",            HandleAccountEmailCommand,          false);
    new LegacyCommandScript("cmd_account_lock_country",     HandleAccountLockCountryCommand,    false);
    new LegacyCommandScript("cmd_account_lock_ip",          HandleAccountLockIpCommand,         false);
    new LegacyCommandScript("cmd_account_password",         HandleAccountPasswordCommand,       false);
    new LegacyCommandScript("cmd_account_set_addon",        HandleAccountSetAddonCommand,       true);
    new LegacyCommandScript("cmd_account_set_password",     HandleAccountSetPasswordCommand,    true);
    new LegacyCommandScript("cmd_account_set_sec_regmail",  HandleAccountSetRegEmailCommand,    true);
    new LegacyCommandScript("cmd_account_set_sec_email",    HandleAccountSetEmailCommand,       true);
#endif
}
