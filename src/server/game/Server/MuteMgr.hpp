#pragma once

#include <chrono>
#include <string>

#include "Define.h"

class WorldSession;

class GAME_API MuteMgr
{
// Definitions
public:
    using Clock = std::chrono::system_clock;
    using Duration = Clock::duration;
    using TimePoint = Clock::time_point;

// Attributes
public:

private:
    // Associated session.
    WorldSession& session;

    // Time point when the current mute expires. May be in the past, if there is no active mute.
    TimePoint expirationTime = {};

// Methods
public:

    MuteMgr(WorldSession& session);
    ~MuteMgr();

    /**
    * Returns whether the associated player is currently muted.
    *
    * @return
    *       {@code true} if the associated player is currently muted, {@code false} otherwise
    */
    bool IsMuted() const;

    /**
    * Returns the time left until the current mute expires. Returns a duration <= 0 if there is no active mute.
    *
    * @return
    *       The time left until the current mute expires or a duration <= 0 if there is no active mute
    */
    Duration GetMuteTimeLeft() const;

    /**
    * Returns the time point when the current mute expires. Returns a time point in the past if there is no active mute.
    *
    * @return
    *       The time point when the current mute expires or a time point in the past if there is no active mute
    */
    TimePoint GetMuteExpirationTime() const;

    /**
    * Mutes the associated player for the given duration and logs the mute to the db. If there is another mute active, the longer one remains.
    * 
    * @note Use static {@code MuteMgr::MuteOffline} to mute offline players.
    * 
    * @param    duration         The duration to mute
    * @param    authorAccountId  The account id of the author of the mute or {@code 0} for system mutes (used for logging)
    * @param    reason           The reason for the mute (used for logging)
    */
    void Mute(const Duration& duration, uint32 authorAccountId, const std::string& reason);

    /**
    * Mutes the offline player with the given accountId for the given duration and logs the mute to the db. If there is another mute stored, the longer one remains.
    *
    * @note Use non-static {@code MuteMgr::Mute} to mute online players.
    *
    * @param    accountId        The account id of the player to mute
    * @param    duration         The duration to mute
    * @param    authorAccountId  The account id of the author of the mute or {@code 0} for system mutes (used for logging)
    * @param    reason           The reason for the mute (used for logging)
    */
    static void MuteOffline(uint32 accountId, const Duration& duration, uint32 authorAccountId, const std::string& reason);

    /**
    * Mutes the associated player for the given duration. If there is another mute active, the longer one remains.
    *
    * @note This does not log anything to the Db, so prefer to use {@code void Mute(const Duration& duration, uint32 authorAccountId, const std::string& reason)} instead.
    *
    * @param    duration    The duration to mute
    */
    void Mute(const Duration& duration);

    /**
    * Mutes the associated player until the given time point. If there is another mute active, the longer one remains.
    *
    * @note This does not log anything to the Db, so prefer to use {@code void Mute(const Duration& duration, uint32 authorAccountId, const std::string& reason)} instead.
    *
    * @param    expirationTime  The expiration time of the mute
    */
    void Mute(const TimePoint& expirationTime);

    /**
    * Unmutes the associated player.
    */
    void Unmute();

    /**
    * Unmutes the offline player with the given accountId.
    * 
    * @note Use non-static {@code MuteMgr::Unmute} to unmute online players.
    * 
    * @param    accountId   The account id of the player to unmute
    */
    static void UnmuteOffline(uint32 accountId);

    /**
    * Load stored mutes for the associated player from the Db. (async)
    */
    void LoadFromDb();

    /**
    * Save the current status to the Db. If there is an active mute, it is stored in the Db, otherwise any existing mute is removed from the Db.
    */
    void SaveToDb();

private:
    /**
    * Mutes the associated player until the given time point. This overrides any previous mute.
    *
    * @param    expirationTime  The expiration time of the mute
    */
    void SetMute(const TimePoint& expirationTime);

    /**
    * Adds a new log entry to the log_mute Db.
    * 
    * @param    targetAccountId    The account id of the muted player
    * @param    targetCharacterId  The id of the character that the muted player was online on when the mute was authored or {@code 0} if the player was offline at that time
    * @param    duration           The duration of the mute
    * @param    authorAccountId    The account id of the author of the mute
    * @param    reason             The reason for the mute
    */
    static void LogMute(uint32 targetAccountId, uint32 targetCharacterId, const Duration& duration, uint32 authorAccountId, const std::string& reason);

};

