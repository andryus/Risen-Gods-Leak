#pragma once

#include "Define.h"
#include "DatabaseEnv.h"

#include <vector>

class Player;

namespace player { namespace complaint {

using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;

struct Complaint
{
    uint32 complainerGuid;
    TimePoint expirationTime;

    Complaint(uint32 complainerGuid, TimePoint expirationTime) : complainerGuid(complainerGuid), expirationTime(expirationTime) {}
};


/**
* Class to store complaints for a player.
*/
class ComplaintMgr
{
public:
    explicit ComplaintMgr(Player& player);

    /**
     * Adds a new complaint from the given complainer and handles punishment (like mute, ban, gm notify).
     * 
     * \param  complainerGuid  The guid of the complaining player
     */
    void RegisterComplaint(uint32 complainerGuid);

    /**
     * Load stored complaints from the given query result.
     * 
     * \param  result  The query result
     */
    void Load(PreparedQueryResult result);

private:
    /// Directly adds a new complaint to the internal data structure without handling mute, ban, etc.
    void AddComplaint(uint32 complainerGuid, TimePoint expirationTime);

    /// Handles punishment (mute, ban, gm notify) if necessary
    void HandlePunishment(uint32 complaintCount);

    void NotifyGMs() const; ///< Notifies GMs (without further checks)
    void MutePlayer() const; ///< Mutes the player (without further checks)
    void BanPlayer() const; ///< Bans the player (without further checks)

    /// stores the given complaint in the database
    void SaveComplaintToDB(uint32 complainerGuid, TimePoint expirationTime) const;

    Player& player;
    std::vector<Complaint> complaints;
    bool notified = false; ///< stores whether the gms have already been notified
};

}}
