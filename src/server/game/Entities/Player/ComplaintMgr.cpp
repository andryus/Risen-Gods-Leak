#include "ComplaintMgr.hpp"

#include "Common.h"
#include "Database/QueryResult.h"

#include "Entities/Player/Player.h"
#include "Logging/Log.h"
#include "Miscellaneous/Language.h"
#include "Server/WorldSession.h"
#include "World/World.h"

namespace player { namespace complaint {

ComplaintMgr::ComplaintMgr(Player& player) : player(player) {}

void ComplaintMgr::Load(PreparedQueryResult result)
{
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        AddComplaint(
            fields[0].GetUInt32(),
            Clock::from_time_t(static_cast<time_t>(fields[1].GetUInt64()))
        );
    } while (result->NextRow());
}

void ComplaintMgr::RegisterComplaint(uint32 complainerGuid)
{
    // can't complain about gms
    if (AccountMgr::IsStaffAccount(player.GetSession()->GetSecurity()))
        return;

    TimePoint now = Clock::now();
    TimePoint expirationTimeFromNow = now + Seconds(sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_TIME));

    bool alreadyComplained = false;
    for (auto itr = complaints.begin(); itr != complaints.end();)
    {
        // already complained -> update expiration time
        if (itr->complainerGuid == complainerGuid)
        {
            alreadyComplained = true;
            itr->expirationTime = expirationTimeFromNow;
        }
        
        // complaint expired -> delete
        if (itr->expirationTime <= now)
        {
            itr = complaints.erase(itr);
            continue;
        }

        ++itr;
    }

    if (!alreadyComplained)
    {
        AddComplaint(complainerGuid, expirationTimeFromNow);
        TC_LOG_TRACE("player.complaints", "Player::RegisterComplaint AccountId %u complained about player %u", complainerGuid, player.GetGUID().GetCounter());
    }

    HandlePunishment(complaints.size());

    SaveComplaintToDB(complainerGuid, expirationTimeFromNow);
}

void ComplaintMgr::AddComplaint(uint32 complainerGuid, TimePoint expirationTime)
{
    complaints.emplace_back(complainerGuid, expirationTime);
}

void ComplaintMgr::HandlePunishment(uint32 complaintCount)
{
    if (sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_GM_NOTIFY) != 0) // notification enabled
    {
        if (complaintCount < sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_GM_NOTIFY))
        {
            notified = false; // reset -> if the player comes over the threshold again, there should be a new notification
        }
        else if (!notified)
        {
            notified = true;
            NotifyGMs();
        }
    }

    if (sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_MUTE) != 0) // mute enabled
    {
        if (complaintCount >= sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_MUTE))
        {
            MutePlayer();
        }
    }

    if (sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_BAN) != 0) // ban enabled
    {
        if (complaintCount >= sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_BAN))
        {
            BanPlayer();
        }
    }
}

void ComplaintMgr::NotifyGMs() const
{
    std::string name = player.GetName();

    std::stringstream chatname{ "" }; // player name with link to the player
    chatname << "|Hplayer:" << name << "|h" << name << "|h";
    sWorld->SendGMText(LANG_COMPLAIN_NOTIFY, chatname.str().c_str(), player.getLevel());
}

void ComplaintMgr::MutePlayer() const
{
    player.GetSession()->GetMuteMgr().Mute(
        Seconds(sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_MUTE_TIME)),
        0,
        "Too many complaints for this player"
    );
}

void ComplaintMgr::BanPlayer() const
{
    sWorld->BanAccountById(
        player.GetSession()->GetAccountId(),
        Seconds(sWorld->getIntConfig(CONFIG_PLAYER_COMPLAINT_BAN_TIME)),
        "Too many complaints for this player",
        "Server: PlayerComplaints"
    );
}

void ComplaintMgr::SaveComplaintToDB(uint32 complainerGuid, TimePoint expirationTime) const
{
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_REP_ACCOUNT_COMPLAINTS);
    stmt->setUInt32(0, complainerGuid);
    stmt->setUInt32(1, player.GetSession()->GetAccountId());
    stmt->setUInt64(2, static_cast<uint64>(Clock::to_time_t(expirationTime)));
    CharacterDatabase.Execute(stmt);
}

}}
