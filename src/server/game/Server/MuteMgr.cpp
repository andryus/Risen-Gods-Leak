#include "MuteMgr.hpp"

#include <chrono>

#include "DatabaseEnv.h"
#include "Player.h"
#include "WorldSession.h"

MuteMgr::MuteMgr(WorldSession& _session) : session(_session) {}

MuteMgr::~MuteMgr() {}

bool MuteMgr::IsMuted() const
{
    return (GetMuteExpirationTime() > Clock::now());
}

MuteMgr::Duration MuteMgr::GetMuteTimeLeft() const
{
    return (GetMuteExpirationTime() - Clock::now());
}

MuteMgr::TimePoint MuteMgr::GetMuteExpirationTime() const
{
    return TimePoint{ expirationTime };
}

void MuteMgr::Mute(const Duration& _duration, uint32 _authorAccountId, const std::string& _reason)
{
    Mute(_duration);

    SaveToDb();

    LogMute(
        session.GetAccountId(),
        session.GetPlayer() ? session.GetPlayer()->GetGUID().GetCounter() : 0,
        _duration,
        _authorAccountId,
        _reason
    );
}

void MuteMgr::MuteOffline(uint32 _accountId, const Duration& _duration, uint32 _authorAccountId, const std::string& _reason)
{
    int64 durationInSecs = std::chrono::duration_cast<std::chrono::seconds>(_duration).count();

    PreparedStatement* _stmt = LoginDatabase.GetPreparedStatement(LOGIN_INSUPD_MUTE);
    _stmt->setUInt32(0, _accountId);
    _stmt->setInt64(1, durationInSecs);
    _stmt->setInt64(2, durationInSecs);
    LoginDatabase.Execute(_stmt);

    LogMute(
        _accountId,
        0,
        _duration,
        _authorAccountId,
        _reason
    );
}

void MuteMgr::Mute(const Duration& _duration)
{
    Mute(Clock::now() + _duration);
}

void MuteMgr::Mute(const TimePoint& _expirationTime)
{
    if (GetMuteExpirationTime() < _expirationTime)
        SetMute(_expirationTime);
}

void MuteMgr::Unmute()
{
    SetMute({});

    SaveToDb();
}

void MuteMgr::UnmuteOffline(uint32 _accountId)
{
    PreparedStatement* _stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_MUTE);
    _stmt->setUInt32(0, _accountId);
    LoginDatabase.Execute(_stmt);
}

void MuteMgr::LoadFromDb()
{
    uint32 _accountId = session.GetAccountId();
    WorldSession* session = sWorld->FindSession(_accountId);
    if (!session)
        return;

    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_MUTE);
    stmt->setUInt32(0, _accountId);
    LoginDatabase.AsyncQuery(stmt)
            .via(session->GetExecutor()).then([session](PreparedQueryResult result)
            {
                if (result)
                    session->GetMuteMgr().Mute(std::chrono::seconds{ result->Fetch()[0].GetInt64() });
            });
}

void MuteMgr::SaveToDb()
{
    if (IsMuted())
    {
        PreparedStatement* _stmt = LoginDatabase.GetPreparedStatement(LOGIN_REP_MUTE);
        _stmt->setUInt32(0, session.GetAccountId());
        _stmt->setInt64(
            1,
            std::chrono::duration_cast<std::chrono::seconds>(GetMuteTimeLeft()).count()
        );
        LoginDatabase.Execute(_stmt);
    }
    else
    {
        PreparedStatement* _stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_MUTE);
        _stmt->setUInt32(0, session.GetAccountId());
        LoginDatabase.Execute(_stmt);
    }
}

void MuteMgr::SetMute(const TimePoint& _expirationTime)
{
    expirationTime = _expirationTime;
}

void MuteMgr::LogMute(uint32 _targetAccountId, uint32 _targetCharacterId, const Duration& _duration, uint32 _authorAccountId, const std::string& _reason)
{
    PreparedStatement* _stmt = RGDatabase.GetPreparedStatement(RG_INS_MUTE);
    _stmt->setUInt32(0, _targetAccountId);
    _stmt->setUInt32(1, _targetCharacterId);
    _stmt->setInt64(2, std::chrono::duration_cast<std::chrono::seconds>(_duration).count());
    _stmt->setUInt32(3, _authorAccountId);
    _stmt->setString(4, _reason.c_str());
    RGDatabase.Execute(_stmt);
}
