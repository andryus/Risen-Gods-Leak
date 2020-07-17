#include "RGSuspiciousLog.h"

#include "Player.h"
#include "ScriptMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "Config.h"
#include "Support/TicketMgr.hpp"
#include "AuctionHouseObject.hpp"
#include "Packets/WorldPacket.h"

namespace RG
{
namespace Suspicious
{
    bool        ACTIVE              = false;
    std::string LOG_PACKET_BASE_DIR = "";
    uint32      LOG_PACKET_TIME     = 5 * IN_MILLISECONDS * MINUTE;

    uint32      CONDITION_ITEM_SELL_FACTOR = 1000;
    bool        CONDITION_TICKET_CREATE_ACTIVE = false;
    bool        CONDITION_AUCTION_CANCEL_ACTIVE = false;
    uint32      CONDITION_AUCTION_CANCEL_COUNT = 5;
    uint32      CONDITION_AUCTION_CANCEL_TIME = 10 * IN_MILLISECONDS;
    bool        CONDITION_NETWORK_EXCEPTION_ACTIVE = false;

    namespace
    {
#pragma pack(push, 1)
        // Packet logging structures in PKT 3.1 format
        struct LogHeader
        {
            char Signature[3];
            uint16 FormatVersion;
            uint8 SnifferId;
            uint32 Build;
            char Locale[4];
            uint8 SessionKey[40];
            uint32 SniffStartUnixtime;
            uint32 SniffStartTicks;
            uint32 OptionalDataSize;
        };

        struct PacketHeader
        {
            uint32 Direction;
            uint32 ConnectionId;
            uint32 ArrivalTicks;
            uint32 OptionalDataSize;
            uint32 Length;
            uint32 Opcode;
        };
#pragma pack(pop)
    }
}
}

bool RG::Suspicious::IsActive()
{
    return ACTIVE;
}

RG::Suspicious::PacketLogger::PacketLogger(uint32 id, bool account) :
    _file(NULL)
{
    Initialize(id, account);
}

RG::Suspicious::PacketLogger::~PacketLogger()
{
    if (_file)
        fclose(_file);

    _file = NULL;
}

std::string GetTimeString()
{
    time_t tmt = time(NULL);
    tm* aTm = localtime(&tmt);
    char buf[20];
    snprintf(buf, 20, "%04d-%02d-%02d-%02d-%02d-%02d", aTm->tm_year+1900, aTm->tm_mon+1, aTm->tm_mday, aTm->tm_hour, aTm->tm_min, aTm->tm_sec);
    return std::string(buf);
}

void RG::Suspicious::PacketLogger::Initialize(uint32 id, bool account)
{
    std::string baseDir = LOG_PACKET_BASE_DIR;

    if (!baseDir.empty())
        if ((baseDir.at(baseDir.length() - 1) != '/') && (baseDir.at(baseDir.length() - 1) != '\\'))
            baseDir.push_back('/');

    std::ostringstream os;
    os << baseDir << GetTimeString() << "-";
    if (account)
        os << "acc-";
    os << id << ".pkt";
    _file = fopen(os.str().c_str(), "wb");

    LogHeader header;
    header.Signature[0] = 'P'; header.Signature[1] = 'K'; header.Signature[2] = 'T';
    header.FormatVersion = 0x0301;
    header.SnifferId = 'T';
    header.Build = 12340;
    header.Locale[0] = 'e'; header.Locale[1] = 'n'; header.Locale[2] = 'U'; header.Locale[3] = 'S';
    std::memset(header.SessionKey, 0, sizeof(header.SessionKey));
    header.SniffStartUnixtime = time(NULL);
    header.SniffStartTicks = getMSTime();
    header.OptionalDataSize = 0;

    fwrite(&header, sizeof(header), 1, _file);
}

void RG::Suspicious::PacketLogger::LogPacket(Direction direction, WorldPacket const& packet)
{
    if (!_file)
        return;

    PacketHeader header;
    header.Direction = direction == CLIENT_TO_SERVER ? 0x47534d43 : 0x47534d53;
    header.ConnectionId = 0;
    header.ArrivalTicks = getMSTime();
    header.OptionalDataSize = 0;
    header.Length = packet.size() + 4;
    header.Opcode = packet.GetOpcode();

    fwrite(&header, sizeof(header), 1, _file);
    if (!packet.empty())
        fwrite(packet.contents(), 1, packet.size(), _file);

    fflush(_file);
}


RG::Suspicious::Monitor::Monitor(WorldSession* session) :
    _session(session),
    _permanent(false),
    _player(nullptr),
    _timer(0),
    _logger(nullptr),
    auctiontracker()
{
    auctiontracker.timer.Reset(CONDITION_AUCTION_CANCEL_TIME);
}

RG::Suspicious::Monitor::~Monitor()
{
    StopLogging();
}

std::string RG::Suspicious::Monitor::GetPlayerName() const
{
    return _player ? _player->GetName() : "Account";
}
uint32 RG::Suspicious::Monitor::GetPlayerGUID() const
{
    return _player ? _player->GetGUID().GetCounter() : _session->GetAccountId();
}

void RG::Suspicious::Monitor::Reload()
{
    _permanent = _session->HasPermission(rbac::RBAC_PERM_RG_LOGGING_TRACK_PACKETS);
    if (_permanent && !IsLoggingActive())
        StartLogging();
}

void RG::Suspicious::Monitor::StartLogging() const
{
    _timer.Reset(LOG_PACKET_TIME);

    if (!_logger)
        _logger = new PacketLogger(_player ? _player->GetGUID().GetCounter() : _session->GetAccountId(), !_player);
}

void RG::Suspicious::Monitor::StopLogging()
{
    if (_logger)
        delete _logger;
    _logger = nullptr;

    _timer.Reset(0);
}

void RG::Suspicious::Monitor::Update(uint32 diff)
{
    if (_logger && !_permanent)
    {
        _timer.Update(diff);

        if (_timer.Passed())
            StopLogging();
    }

    if (CONDITION_AUCTION_CANCEL_ACTIVE)
    {
        auctiontracker.timer.Update(diff);
        if (auctiontracker.timer.Passed())
            auctiontracker.counter = 0;
    }
}

bool RG::Suspicious::Monitor::CheckConditionMeets(Condition condition, Item* item, uint32 money) const
{
    bool isSuspicious = false;

    switch (condition)
    {
    case CONDITION_UNREGULAR_ITEM_SELL:
        // attempt to sell item beyond normal price
        if (money > item->GetTemplate()->SellPrice * CONDITION_ITEM_SELL_FACTOR)
        {
            isSuspicious = true;

            TC_LOG_INFO("rg.log.suspicious", "Item: Player [%u] %s sold item %u (count: %u) for %u copper",
                        GetPlayerGUID(), GetPlayerName().c_str(), item->GetEntry(), item->GetCount(), money);
        }
        break;
    default:
        break;
    }

    if (isSuspicious)
        StartLogging();

    return isSuspicious;
}

bool RG::Suspicious::Monitor::CheckConditionMeets(Condition condition, const support::Ticket& ticket) const
{
    bool isSuspicious = false;

    switch (condition)
    {
    case CONDITION_TICKET_CREATE:
        if (CONDITION_TICKET_CREATE_ACTIVE)
        {
            isSuspicious = true;

            TC_LOG_INFO("rg.log.suspicious", "Ticket: Player [%u] %s created ticket %u with message %s",
                        GetPlayerGUID(), GetPlayerName().c_str(), ticket.GetId(), ticket.GetMessage().c_str());
        }
        break;
    default:
        break;
    }

    if (isSuspicious)
        StartLogging();

    return isSuspicious;
}

bool RG::Suspicious::Monitor::CheckConditionMeets(Condition condition, const AuctionEntry* auction) const
{
    bool isSuspicious = false;

    if (!auction) // prevent crash if auction is null
        return false;

    switch (condition)
    {
    case CONDITION_AUCTION_CANCEL:
        if (CONDITION_AUCTION_CANCEL_ACTIVE)
        {
            if (++auctiontracker.counter >= CONDITION_AUCTION_CANCEL_COUNT)
            {
                isSuspicious = true;

                TC_LOG_INFO("rg.log.suspicious", "Auction: Player [%u] %s canceld auction %u Item %u (%u x) (GUID %u)",
                            GetPlayerGUID(), GetPlayerName().c_str(), auction->Id, auction->itemEntry, auction->itemCount, auction->itemGUIDLow);
            }

            auctiontracker.timer.Reset(CONDITION_AUCTION_CANCEL_TIME);
        }
        break;
    default:
        break;
    }

    if (isSuspicious)
        StartLogging();

    return isSuspicious;
}

namespace RG
{
    namespace Suspicious
    {
        std::string GetNetworkErrorInfo(NetworkExceptionType type)
        {
            switch (type)
            {
            case NetworkExceptionType::INVALID_HEADER:
                return "invalid header";
            case NetworkExceptionType::UNEXPECTED_OPCODE:
                return "opcode unexpected";
            case NetworkExceptionType::UNPROCESSED_TAIL:
                return "unprocessed tail";
            case NetworkExceptionType::EXCEPTION:
                return "parsing exception";
            case NetworkExceptionType::OPCODE_NEVER:
                return "opcode status never";
            case NetworkExceptionType::OPCODE_UNHANDLED:
                return "opcode unhandled";
            case NetworkExceptionType::OPCODE_UNKNOWN:
                return "opcode unkown";
            default:
                return "<unknown>";
            }
        }
    }
}

bool RG::Suspicious::Monitor::CheckConditionMeets(Condition condition, NetworkExceptionType type) const
{
    bool isSuspicios = false;

    switch (condition)
    {
    case CONDITION_NETWORK_EXCEPTION:
        if (CONDITION_NETWORK_EXCEPTION_ACTIVE)
        {
            switch (type)
            {
            case NetworkExceptionType::OPCODE_UNHANDLED:
            case NetworkExceptionType::UNPROCESSED_TAIL:
                return false;
            default:
                    break;
            }

            isSuspicios = true;

            TC_LOG_INFO("rg.log.suspicious", "Network: Player [%u] %s caused packet parsing error Type: %s",
                        GetPlayerGUID(), GetPlayerName().c_str(), GetNetworkErrorInfo(type).c_str());
        }
        break;
    default:
        break;
    }

    if (isSuspicios)
        StartLogging();

    return isSuspicios;
}

namespace RG
{
namespace Suspicious
{

    void LoadConfig()
    {
        ACTIVE                              = sConfigMgr->GetBoolDefault(   "RG.Suspicious.Enabled",                        false);
        LOG_PACKET_BASE_DIR                 = sConfigMgr->GetStringDefault( "RG.Suspicious.Log.Packet.Directory",           "");
        LOG_PACKET_TIME                     = sConfigMgr->GetIntDefault(    "RG.Suspicious.Log.Packet.Time",                5 * IN_MILLISECONDS * MINUTE);

        CONDITION_ITEM_SELL_FACTOR          = sConfigMgr->GetIntDefault(    "RG.Suspicious.Condition.ItemSellFactor",       1000);
        CONDITION_TICKET_CREATE_ACTIVE      = sConfigMgr->GetBoolDefault(   "RG.Suspicious.Condition.TicketCreate",         false);

        CONDITION_AUCTION_CANCEL_ACTIVE     = sConfigMgr->GetBoolDefault(   "RG.Suspicious.Condition.AuctionCancel",        false);
        CONDITION_AUCTION_CANCEL_COUNT      = sConfigMgr->GetIntDefault(    "RG.Suspicious.Condition.AuctionCancel.Count",  5);
        CONDITION_AUCTION_CANCEL_TIME       = sConfigMgr->GetIntDefault(    "RG.Suspicious.Condition.AuctionCancel.Time",   10) * IN_MILLISECONDS;

        CONDITION_NETWORK_EXCEPTION_ACTIVE  = sConfigMgr->GetBoolDefault(   "RG.Suspicious.Condition.NetworkException",     false);
    }

    bool HandleSuspiciousReload(ChatHandler* handler, char const* /*args*/)
    {
        LoadConfig();
        handler->PSendSysMessage("Reloaded rg suspicious.");
        return true;
    }

    class rg_suspicious_worldscript : public WorldScript
    {
    public:
        rg_suspicious_worldscript() : WorldScript("rg_suspicious_worldscript") { }

        void OnConfigLoad(bool reload)
        {
            if (reload)
                LoadConfig();
        }

        void OnLoad()
        {
            LoadConfig();
        }
    };

}
}

void AddSC_rg_suspicious()
{
    new RG::Suspicious::rg_suspicious_worldscript();
    new chat::command::LegacyCommandScript("cmd_rg_suspicious_reload", RG::Suspicious::HandleSuspiciousReload, true);
}
