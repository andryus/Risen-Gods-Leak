#include "SharedDefines.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Group.h"
#include "WorldSession.h"


/*###
### polly
###*/

enum polly
{
    SPELL_VOTE          = 14867,      // Visual Spell (Untalent Visual Effect)
};

enum pollyGossip
{
    GOSSIP_MENU_MAIN            = 60057, //Willkommen beim NPC für repäsentative Umfragen.
    GOSSIP_MENU_NO_POLLS        = 60058, //Zur Zeit sind keine Abstimmungen verfügbar.
    GOSSIP_MENU_CHO_POLL        = 60059, //Wähle eine Abstimmung um an ihr teilzunehmen.
    GOSSIP_MENU_CHO_POLL_TO_CON = 60060, //Wähle eine Abstimmung um Details zu sehen, ihre Laufzeit zu verändern oder sie zu löschen.
    GOSSIP_MENU_VOTE            = 60061, //Deine Stimme wurde gezählt.\n\nVielen Dank!
    GOSSIP_MENU_NO_ELI          = 60062, //Du hast an dieser Abstimmung bereits teilgenommen oder erfüllst die Level-Voraussetzung nicht!
    GOSSIP_MENU_POLL_CONFIG     = 60063, //Diese Abstimmung bearbeiten
    GOSSIP_MENU_CHANGE_SETED    = 60064, //Änderung übernommen
    GOSSIP_MENU_ERROR           = 60065, //Tables in RG-DB nicht vorhanden!

    GOSSIP_MENU_NOT_PVP         = 60066, //Du musst aktiv Arena in der letzten Season gespielt haben um an dieser Abfrage teilzunehmen.

    GOSSIP_ITEM_MAIN        = 0, //"Zum Hauptmenu"
    GOSSIP_ITEM_BYE         = 1, //"Lebt wohl"
    GOSSIP_ITEM_VERIFY      = 2, //"Vorgang wirklich durchführen?"
    GOSSIP_ITEM_CANCEL      = 3, //"Abstimmung beenden"
    GOSSIP_ITEM_DELETE      = 4, //"Abstimmung aus der DB löschen"
    GOSSIP_ITEM_CHANGE_ON   = 5, //"Änderungen zulassen [zZt ist die Stimmabgabe endgültig]"
    GOSSIP_ITEM_CHANGE_OFF  = 6, //"Endgültig abstimmen [zZt sind Änderungen zugelassen]"
    GOSSIP_ITEM_ANN_FIRST   = 7, //"Ersten Announce senden"
    GOSSIP_ITEM_ANN_MORE    = 8, //"Announce erneut senden"
    GOSSIP_ITEM_NO_RESULT   = 9, //"Es liegen noch keine Ergebnisse vor"
    GOSSIP_ITEM_EDIT        = 10, //"Abstimmungen bearbeiten"
    GOSSIP_ITEM_SHOW        = 11, //"Abstimmungen anzeigen"
};

#define WHISPER_VOTE        0 //"Aus meinen Aufzeichnungen geht hervor, dass ihr noch nicht an allen Abstimmungen teilgenommen habt. Es wäre schön, wenn ihr euch für mich eine Minute Zeit nehmen könntet."

#define ACTION(a) (a<<8)
#define ENTRY(a) (a<<16)
#define ITEM_TEXT(a) player->GetOptionTextWithEntry(GOSSIP_MENU_MAIN, a)

std::string staticText[] = 
{
    "Willkommen beim NPC für repäsentative Umfragen.",
    "Zur Zeit sind keine Abstimmungen verfügbar.",
    "Wähle eine Abstimmung um an ihr teilzunehmen.",
    "Wähle eine Abstimmung um Details zu sehen, ihre Laufzeit zu verändern oder sie zu löschen.",
    "Deine Stimme wurde gezählt.\n\nVielen Dank!",
    "Du hast an dieser Abstimmung bereits teilgenommen oder erfüllst die Level-Voraussetzung nicht!",
    "Diese Abstimmung bearbeiten",
    "Änderung übernommen",
    "Tables in RG-DB nicht vorhanden!",
    "Du musst aktiv Arena in der letzten Season gespielt haben um an dieser Abfrage teilzunehmen."
};
uint8 staticTextCount = sizeof(staticText) / sizeof(std::string);

enum PollFlags 
{
    POLL_ALREADY_NOTIFIED   = 0x000001,     // Announce already sent
    POLL_RECORDING          = 0x000002,     // Poll is currently active and running
    POLL_ALLOW_CHANGE       = 0x000004,     // Player may change his mind about vote
    POLL_ACTIVE             = 0x000008,     // Poll was recognized and incorporated by this script
    POLL_ONLY_PVP_PLAYER    = 0x010000      // Only PVP Player (with arena team) are allowed to parcipitate
};

enum PollCustGossipSender
{
    GOSSIP_SENDER_GLOBAL    = 2,
    GOSSIP_SENDER_OPTIONS   = 3,
    GOSSIP_SENDER_CONFIG    = 4,
    GOSSIP_SENDER_LOGIN     = 5,
};

struct PollStruct
{
    uint32 entry;
    uint32 flags;
    uint32 timeout;
    std::string title;
    std::string text;
    std::string option[5];
};

typedef std::list<PollStruct> PollList;
std::set<uint32> doneWhispers;

PollList pollList;
bool     hasActivePolls = false;
bool     started        = false;
uint32   Update_Timer   = 0;
uint32   startEntry     = GOSSIP_MENU_MAIN;
uint8    minLevel       = 19;

void LoadSingleText(uint32 entry, std::string text)
{
    GossipText &gText = sObjectMgr->_gossipTextStore[entry];
    for (int i = 0; i < 8; i++)
    {
        gText.Options[i].Text_0           = text;
        gText.Options[i].Text_1           = text;
        gText.Options[i].Language         = 0;
        gText.Options[i].Probability      = 0.0f;
        for (int j=0; j<3; ++j)
        {
            gText.Options[i].Emotes[j]._Delay  = 0;
            gText.Options[i].Emotes[j]._Emote  = 0;
        }
    }
}

void AppendPollText()
{
    /*for (UNORDERED_MAP<uint32,GossipText>::iterator itr = sObjectMgr->_gossipTextStore.begin(); itr != sObjectMgr->_gossipTextStore.end(); itr++)
        if ((*itr).first >= startEntry)
            startEntry = (*itr).first+1;*/

    for (int i = 0; i < staticTextCount; i++)
        LoadSingleText((i+startEntry), staticText[i]);

    for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
    {
        std::ostringstream ss;
        ss << itr->text;
        if (itr->flags & POLL_ALLOW_CHANGE)
            ss << "\n\nEs ist möglich die abgegebene Stimme zu ändern.";
        else
            ss << "\n\nDeine abgegebene Stimme ist endgültig!";

        LoadSingleText((itr->entry + startEntry + staticTextCount),  ss.str().c_str());
    }
}

void PopulatePollList(bool init)
{
    pollList.clear();

    QueryResult result = RGDatabase.PQuery("SELECT * FROM `custom_poll_polls`");

    if (!result)
        return;

    if (init)
        started = true;

    while (Field *fields = result->Fetch())
    {
        PollStruct pl;
        pl.entry = fields[0].GetUInt32();
        pl.flags = fields[1].GetUInt32();
        pl.timeout = fields[2].GetUInt32();
        pl.title = fields[3].GetString();
        pl.text = fields[4].GetString();
        for (int i = 0; i < 5; i++)
            pl.option[i] = fields[5+i].GetString();

        // tag outdated polls as such
        if (pl.flags & POLL_RECORDING && time(0) >= fields[2].GetUInt32())
        {
            pl.flags = (pl.flags & ~POLL_RECORDING);
            RGDatabase.PExecute("UPDATE custom_poll_polls SET `flags` = `flags` & ~%u WHERE entry = %u", POLL_RECORDING, fields[0].GetUInt32());
        }
        // set timer for next PopulatePollList()-call
        else if (pl.flags & POLL_RECORDING)
        {
            if ((fields[2].GetUInt32() - time(0)) < Update_Timer)
                Update_Timer = fields[2].GetUInt32() - time(0);
        }
        // auto-activate new polls
        else if ((pl.flags & POLL_ACTIVE) == 0)
        {
            RGDatabase.PExecute("UPDATE `custom_poll_polls` SET `flags` = `flags` | %u WHERE `entry` = %u",(POLL_ACTIVE|POLL_RECORDING), fields[0].GetUInt32());
            pl.flags = (pl.flags | POLL_ACTIVE);

            // add new desctiption to gossip map if not in init
            if (!init)
            {
                std::ostringstream ss;
                ss << pl.text;
                if (pl.flags & POLL_ALLOW_CHANGE)
                    ss << "\n\nEs ist möglich die abgegebene Stimme zu ändern.";
                else
                    ss << "\n\nDeine abgegebene Stimme ist endgültig!";
                LoadSingleText((fields[0].GetUInt32() + startEntry + staticTextCount),  ss.str().c_str());
            }
        }

        pollList.push_back(pl);
        result->NextRow();
    }
}

class npc_polly : public CreatureScript
{
public:
/***********
 * Std Creature Functions
***********/
    npc_polly() : CreatureScript("npc_polly") {}

    struct npc_pollyAI : public ScriptedAI
    {
        npc_pollyAI(Creature* creature) : ScriptedAI(creature) 
        { //constructor

            if (!sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
                return;

            if (!started)
            {
                // populate list with players, that voted on every active poll
                std::ostringstream ss;
                ss << "SELECT DISTINCT `accountID` FROM `custom_poll_results` toptable WHERE NOT EXISTS (";
                ss << "SELECT `entry` FROM `custom_poll_polls` middletable WHERE flags & " << POLL_RECORDING << " AND NOT EXISTS (";
                ss << "SELECT `accountID` FROM `custom_poll_results` WHERE `accountID` = toptable.`accountID` AND middletable.`entry` = `pollEntry` ))";
                if (QueryResult result = RGDatabase.PQuery(ss.str().c_str()))
                {
                    while (Field *fields = result->Fetch())
                    {
                        doneWhispers.insert(fields[0].GetUInt32());
                        result->NextRow();
                    }
                }
                PopulatePollList(true);
                AppendPollText();
            }
            Reset();
        }//constructor end

        Creature* Me()
        {
            return me;
        }

        void Reset() {}

        void Aggro(Unit*) 
        {
            EnterEvadeMode(EVADE_REASON_OTHER);
        }

        void EnterEvadeMode(EvadeReason /*why*/) override
        {
            me->GetThreatManager().ClearAllThreat();
            me->CombatStop();
        }

        void UpdateAI(uint32 diff)
        {
            if (!sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
                me->DespawnOrUnsummon();

            // Invisible if no poll in progress
            if (Update_Timer <= diff)
            {
//                PopulatePollList(false);
                
                hasActivePolls  = false;
                for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
                {
                    if (itr->flags & POLL_RECORDING)
                    {
                        hasActivePolls = true;
                        break;
                    }
                }

                me->SetVisible(hasActivePolls);
                Update_Timer = 30000;
            }
            else
                Update_Timer -= diff;
        }

        void MoveInLineOfSight(Unit* unit)
        {
            if (!unit || unit->GetTypeId() != TYPEID_PLAYER)
                return;
            
            if (!sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
                return;

            Player* player = unit->ToPlayer();

            // has active polls
            if (!me->IsVisible())
                return;
        
            // don't whisper GMs
            if (player->IsGameMaster())
                return;
        
            // check done whispers
            uint32 accId = player->GetSession()->GetAccountId();
            std::set<uint32>::iterator itr = doneWhispers.find(accId);
            if (itr == doneWhispers.end())
            {
                Talk(WHISPER_VOTE, unit);
                doneWhispers.insert(player->GetSession()->GetAccountId());
            }
        }
    }; //end struct
    
    /***********
     * Pollys Helper
     ***********/
    bool IsAdministrator(Player *player)
    {
        if (!player || !player->IsGameMaster())
            return false;
        
        return (player->GetSession()->GetSecurity() >= GMLEVEL_BEREICHSLEITER);
    }

    bool IsEligibleForVote(Player *player, PollStruct poll, uint32& error)
    {
        if (!player || !poll.entry)
            return false;

        if (player->getLevel() < minLevel)
            return false;

        if (poll.flags & POLL_ONLY_PVP_PLAYER)
            if (!player->GetArenaTeamId(0))
            {
                error = POLL_ONLY_PVP_PLAYER;
                return false;
            }

        if (poll.flags & POLL_ALLOW_CHANGE)
            return true;

        QueryResult cprResult = RGDatabase.PQuery("SELECT `selectedOpt` FROM `custom_poll_results` WHERE `pollEntry` = %u AND `accountID` = %u", poll.entry, player->GetSession()->GetAccountId());
        if (!cprResult)
            return true;

        return false;
    }

    void SendAnnounce(Player *player, PollStruct poll)
    {
        if (!player || !IsAdministrator(player) || !poll.entry || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        uint32 d = uint32((poll.timeout - time(0)) / (24 * 60 * 60));
        uint32 h = uint32((poll.timeout - time(0)) / (60 * 60) - d * 24);

        std::ostringstream ss;
        ss << "Hinweis: Bitte nehmt an der Umfrage zum Thema";
        ss << " '" << poll.title << "' teil. ";
        ss << "Die Umfrage ist noch für " << d << " Tage und " << h << " Stunden verfügbar. ";
        ss << "Helft uns euch zu helfen!";

        // 4 = LANG_EVENTMESSAGE
        sWorld->SendWorldText(4, ss.str().c_str()); 
        // flag poll, that announce was sent
        DBModPollFlag(player, poll.entry, POLL_ALREADY_NOTIFIED, true);
    }

    void DBRemovePoll(Player *player, uint32 entry)
    {
        if (!player || !IsAdministrator(player) || !entry || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        // delete from list
        for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
            if (itr->entry == entry)
            {
                pollList.erase(itr);
                break;
            }

        // delete from db
        RGDatabase.PExecute("DELETE FROM `custom_poll_polls` WHERE `entry` = %u", entry);
        RGDatabase.PExecute("DELETE FROM `custom_poll_results` WHERE `pollEntry` = %u", entry);

        // request list refresh
        Update_Timer = 500;
    }

    void DBDeleteResults(Player *player, uint32 entry)
    {
        if (!player || !IsAdministrator(player) || !entry || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        RGDatabase.PExecute("DELETE FROM `custom_poll_results` WHERE `pollEntry` = %u", entry);
    }


    void DBwriteVote(PollStruct poll, uint32 accID, uint8 opt)
    {
        if (!poll.entry || !accID || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        // option_x empty -> not a valid vote option
        if (!poll.option[opt].length())
            return;


        RGDatabase.PExecute("REPLACE INTO custom_poll_results VALUES (%u, %u, %u)", poll.entry, accID, opt);
    }

    void DBModPollFlag(Player *player, uint32 entry, uint8 flag, bool add)
    {
        if (!player || !entry || !flag || !IsAdministrator(player) || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        // modify in list
        for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
            if (itr->entry == entry)
            {
                (*itr).flags = add ? (itr->flags | flag) : (itr->flags & ~flag);
                break;
            }

        // modify in DB
        if (add)
            RGDatabase.PExecute("UPDATE `custom_poll_polls` SET `flags` = `flags` | %u WHERE `entry` = %u", flag, entry);
        else
            RGDatabase.PExecute("UPDATE `custom_poll_polls` SET `flags` = `flags` & ~%u WHERE `entry` = %u", flag, entry);

        // request list refresh
        if (!add && (flag & POLL_RECORDING))
            Update_Timer = 500;
        // update GossiptText-Map
        else if (flag & POLL_ALLOW_CHANGE)
        {
            std::ostringstream ss;
            for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
                if (itr->entry == entry)
                    ss << itr->text;

            if (add)
                ss << "\n\nEs ist möglich die abgegebene Stimme zu ändern.";
            else
                ss << "\n\nDeine abgegebene Stimme ist endgültig!";

            LoadSingleText((entry + startEntry + staticTextCount),  ss.str().c_str());
        }
    }

    void DBUpdateRuntime(Player *player, uint32 entry, int32 diff)
    {
        if (!player || !entry || !diff || !IsAdministrator(player) || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        // update runtime in list
        for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
            if (itr->entry == entry)
            {
                (*itr).timeout = itr->timeout + diff;
            }

        // update runtime in DB        
        RGDatabase.PExecute("UPDATE `custom_poll_polls` SET `timeout` = `timeout` + %i WHERE `entry` = %u", diff, entry);
    }

    /***********
     * Pollys Gossip
     ***********/
    void AddBasicGossip(Creature* me, Player *player, uint32 textID)
    {
        if (!player || !textID || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_MAIN, GOSSIP_SENDER_GLOBAL, ACTION(2));
        player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_BYE, GOSSIP_SENDER_GLOBAL, ACTION(1));
        player->SEND_GOSSIP_MENU(textID, me->GetGUID());
    }

    void GossipPollList(Creature* me, Player *player, bool admin)
    {
        if (!player || (!IsAdministrator(player) && admin) || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;
        
        if (!pollList.empty())
        {
            // (entry << 16) | (action << 8) | option)
            for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
            {
                if (admin)
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, itr->title, GOSSIP_SENDER_MAIN, (ENTRY(itr->entry) | ACTION(1)));
                else if (itr->flags & POLL_RECORDING) //TODO: rausfinden, warum das mit dem abchecken vom Flag nicht geht
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, itr->title, GOSSIP_SENDER_MAIN, (ENTRY(itr->entry) | ACTION(2)));
            }
            AddBasicGossip(me, player, admin ? GOSSIP_MENU_CHO_POLL_TO_CON : GOSSIP_MENU_CHO_POLL);
        }
        else
            AddBasicGossip(me, player, GOSSIP_MENU_NO_POLLS);
    }

    void GossipPollOptionList(Creature* me, Player *player, PollStruct poll)
    {
        if (!player || !poll.entry || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        uint32 error = 0;
        if (!IsEligibleForVote(player, poll, error))
        {
            switch (error)
            {
                case POLL_ONLY_PVP_PLAYER:
                    AddBasicGossip(me, player, GOSSIP_MENU_NOT_PVP);
                    break;
                default:
                    AddBasicGossip(me, player, GOSSIP_MENU_NO_ELI);
                    break;
            }

            return;
        }

        Field *cprFields;
        uint8 selection = 10;
        // 10 hab ich mir ausm Arsch gezogen.. es gibt zeigt nur, das es keine valide Option ist
        if (QueryResult cprResult = RGDatabase.PQuery("SELECT `selectedOpt` FROM `custom_poll_results` WHERE `accountID` = %u AND `pollEntry` = %u LIMIT 1", player->GetSession()->GetAccountId(), poll.entry))
            if ((cprFields = cprResult->Fetch()))
            {
                selection = cprFields[0].GetUInt32();
            }

        std::ostringstream ss;
        for (uint8 i = 0; i < 5; ++i)
        {
            // only defined options
            if (!poll.option[i].length())
                continue;
            
            // if selection present mark selected options
            if (selection == 10)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, poll.option[i], GOSSIP_SENDER_OPTIONS, (ENTRY(poll.entry) | ACTION(2) | i));
            else
            {
                ss.str("");
                if (selection == i)
                    ss << "[x] ";
                else
                    ss << "[    ] ";
                ss << poll.option[i];
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ss.str().c_str(), GOSSIP_SENDER_OPTIONS, (ENTRY(poll.entry) | ACTION(2) | i));
            }
        }
        AddBasicGossip(me, player, poll.entry+staticTextCount+startEntry);
    }

    void GossipPollConfigList(Creature* me, Player *player, PollStruct poll)
    {
        if (!player || !poll.entry || !IsAdministrator(player) || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        if (poll.flags & POLL_ACTIVE)
        {
            std::ostringstream ss;
            if (QueryResult cprResult = RGDatabase.PQuery("SELECT `selectedOpt` FROM `custom_poll_results` WHERE `pollEntry` = %u", poll.entry))
            {
                uint32 votes[5] = {0, 0, 0, 0, 0};
                while (Field *cprFields = cprResult->Fetch())
                {
                    votes[cprFields[0].GetUInt32()]++;
                    cprResult->NextRow();
                }
                if (poll.flags & POLL_RECORDING)
                {
                    ss << "Aktueller Stand: " << votes[0] << " : " << votes[1] << " : " << votes[2] << " : " << votes[3] << " : " << votes[4] << " [Klicken zum resetten]";
                    player->PlayerTalkClass->GetGossipMenu().AddMenuItem(-1, GOSSIP_ICON_CHAT, ss.str().c_str(), GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(7)), 
                        ITEM_TEXT(GOSSIP_ITEM_VERIFY), 0, false);
                }
                else
                {
                    ss << "Stand: " << votes[0] << " : " << votes[1] << " : " << votes[2] << " : " << votes[3] << " : " << votes[4];
                    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ss.str().c_str(), GOSSIP_SENDER_GLOBAL, ACTION(2));
                }
            }
            else
                player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_NO_RESULT, GOSSIP_SENDER_GLOBAL, ACTION(2));

            ss.str("");
            int32 tdiff = poll.timeout - time(0);
            ss << "Verbleibende Laufzeit: ";
            tdiff > 0 ? ss << tdiff << " Sekunden [Klicken zum ändern]" : ss << "Zeit abgelaufen";
            // Currently its only allowed to change time during runtime. else send back to main menu
            if (tdiff > 0)
                player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, ss.str().c_str(), GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(1)), 
                    "Zeit in Sekunden angeben. Nur die Differenz angeben! Positive Zahl um Zeit zu erhöhen, negative um zu verringern. ", 0, true);
            else
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, ss.str().c_str(), GOSSIP_SENDER_GLOBAL, ACTION(2));

            if (poll.flags & POLL_RECORDING)
            {
                player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, ITEM_TEXT(GOSSIP_ITEM_CANCEL), 
                    GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(6) | POLL_RECORDING), ITEM_TEXT(GOSSIP_ITEM_VERIFY), 0, false);

                if (poll.flags & POLL_ALREADY_NOTIFIED)
                    player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, ITEM_TEXT(GOSSIP_ITEM_ANN_MORE),
                        GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(5)), ITEM_TEXT(GOSSIP_ITEM_VERIFY), 0, false);
                else
                    player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_ANN_FIRST, GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(5)));
            }

            if (poll.flags & POLL_ALLOW_CHANGE)
                player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_CHANGE_OFF, GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(2) | POLL_ALLOW_CHANGE));
            else
                player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_CHANGE_ON, GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(1) | POLL_ALLOW_CHANGE));
        }
            player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, ITEM_TEXT(GOSSIP_ITEM_DELETE),
                GOSSIP_SENDER_CONFIG, (ENTRY(poll.entry) | ACTION(4)), ITEM_TEXT(GOSSIP_ITEM_VERIFY), 0, false);
        AddBasicGossip(me, player, startEntry+6);
    }

    void GossipDoVote(Creature* me, Player *player, PollStruct poll, uint8 optionID)
    {
        if (!player || !poll.entry || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return;

        uint32 error = 0;
        if (!IsEligibleForVote(player, poll, error))
        {
            switch (error)
            {
                case POLL_ONLY_PVP_PLAYER:
                    AddBasicGossip(me, player, GOSSIP_MENU_NOT_PVP);
                    break;
                default:
                    AddBasicGossip(me, player, GOSSIP_MENU_NO_ELI);
                    break;
            }

            return;
        }

        // Visual Pleasure
        switch (urand(0,3))
        {
            case 0: me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE); break;
            case 1: me->HandleEmoteCommand(EMOTE_ONESHOT_BOW); break;
            case 2: me->HandleEmoteCommand(EMOTE_ONESHOT_WAVE); break;
            case 3: me->HandleEmoteCommand(EMOTE_ONESHOT_YES); break;
        }
        player->CastSpell(player, SPELL_VOTE, false);

        DBwriteVote(poll, player->GetSession()->GetAccountId(), optionID);
        AddBasicGossip(me, player, startEntry+4);
    }

    bool OnGossipHello(Player *player, Creature* creature)
    {
        if (!player || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return false;
        
        // Check for installation
        if (QueryResult result = RGDatabase.PQuery("SHOW TABLES LIKE 'custom_poll_polls'"))
        {
            if (Field *fields = result->Fetch())
            {
                if (fields[0].GetString() == "custom_poll_polls")
                {
                    if (player && IsAdministrator(player))
                        player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_EDIT, GOSSIP_SENDER_LOGIN, ACTION(1));

                    player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_SHOW, GOSSIP_SENDER_LOGIN, ACTION(2));
                    player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_BYE, GOSSIP_SENDER_GLOBAL, ACTION(1));
                    player->SEND_GOSSIP_MENU(GOSSIP_MENU_MAIN, creature->GetGUID());
                    return true;
                }
            }
        }

        player->AddGossipItem(GOSSIP_MENU_MAIN, GOSSIP_ITEM_BYE, GOSSIP_SENDER_GLOBAL, ACTION(1));
        player->PlayerTalkClass->SendGossipMenu(GOSSIP_MENU_ERROR, creature->GetGUID());
        return true;
    }

    /***********
     * Pollys Gossip Mgr
     ***********/
    bool OnGossipSelect(Player *player, Creature* creature, uint32 sender, uint32 valueMask)
    {
        if (!sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return false;

        player->PlayerTalkClass->ClearMenus();

        uint16 entry = ((valueMask & 0xFFFF0000) >> 16);
        uint8 action = ((valueMask & 0xFF00) >> 8);
        uint8 option = (valueMask & 0xFF);

        PollStruct poll;
        for (PollList::iterator itr = pollList.begin(); itr != pollList.end(); itr++)
            if (entry == itr->entry)
            {
                poll = *itr;
                break;
            }

        switch(sender)
        {
            case GOSSIP_SENDER_LOGIN:
            {
                switch(action)
                {
                    case 1: GossipPollList(creature, player, true);                return true;
                    case 2: GossipPollList(creature, player, false);               return true;
                }
                break;
            }
            case GOSSIP_SENDER_MAIN:
            {
                switch(action)
                {
                    case 1: GossipPollConfigList(creature, player, poll);          return true;
                    case 2: GossipPollOptionList(creature, player, poll);          return true;
                }
                break;
            }
            case GOSSIP_SENDER_GLOBAL:
            {
                switch(action)
                {
                    case 1: player->PlayerTalkClass->SendCloseGossip();  return true;
                    case 2: OnGossipHello(player, creature);             return true;
                }
                break;
            }
            case GOSSIP_SENDER_OPTIONS:
            {
                switch(action)
                {
                    case 2: GossipDoVote(creature, player, poll, option);          return true;
                }
            }
            case GOSSIP_SENDER_CONFIG:
            {
                switch(action)
                {
                    case 1: DBModPollFlag(player, entry, option, true);  break;
                    case 2:
                    case 6: DBModPollFlag(player, entry, option, false); break;
                    case 4: DBRemovePoll(player, entry);                 break;
                    case 5: SendAnnounce(player, poll);                  break;
                    case 7: DBDeleteResults(player, entry);              break;
                }
                AddBasicGossip(creature, player, GOSSIP_MENU_CHANGE_SETED);
                return true;
            }
        }
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 valueMask, const char* sCode)
    {
        if (sender != GOSSIP_SENDER_CONFIG || !sWorld->getBoolConfig(CONFIG_RG_DATABASE_AVAILABLE))
            return false;

        if (((valueMask & 0xFF00) >> 8) == 1)     // action == 1
            DBUpdateRuntime(player, ((valueMask & 0xFFFF0000) >> 16), atoi(sCode));

        AddBasicGossip(creature, player, GOSSIP_MENU_CHANGE_SETED);
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_pollyAI(creature);
    }
}; //end class


void AddSC_custom_poll() 
{
    new npc_polly();
}
