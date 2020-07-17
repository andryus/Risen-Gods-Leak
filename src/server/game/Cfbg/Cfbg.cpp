#include "Cfbg.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "Player.h"
#include "Chat.h"
#include "Containers.h"
#include "Opcodes.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"

uint8 Unit::getRace(bool forceoriginal) const
{
    if (GetTypeId() == TYPEID_PLAYER)
    {
        Player* pPlayer = ((Player*)this);

        if (forceoriginal)
            return pPlayer->getORace();

        if (pPlayer->InArena())
            return GetByteValue(UNIT_FIELD_BYTES_0, 0);

        if (!pPlayer->IsPlayingNative() && !pPlayer->IsLoading())
            return pPlayer->getFRace();
    }

    return GetByteValue(UNIT_FIELD_BYTES_0, 0);
}

bool Player::SendRealNameQuery() const
{
    if (IsPlayingNative())
        return false;

    WorldPacket data(SMSG_NAME_QUERY_RESPONSE, (8 + 1 + 1 + 1 + 1 + 1 + 10));
    data << GetGUID().WriteAsPacked();                             // player guid
    data << uint8(0);                                       // added in 3.1; if > 1, then end of packet
    data << GetName();                                   // played name
    data << uint8(0);                                       // realm name for cross realm BG usage
    data << uint8(getORace());
    data << uint8(getGender());
    data << uint8(getClass());
    data << uint8(0);                                   // is not declined
    SendDirectMessage(std::move(data));

    return true;
}

void Player::SetFakeRaceAndMorph()
{
    std::vector<std::pair<Races, PlayerInfo const*>> validRaces;

    for (uint8 race = 1; race < MAX_RACES; race++)
    {
        if ((1 << (race - 1)) & (GetOTeam() == ALLIANCE ? RACEMASK_ALLIANCE : RACEMASK_HORDE))
            continue;
        PlayerInfo const* info = sObjectMgr->GetPlayerInfo(race, getClass());
        if (!info)
            continue;
        validRaces.push_back(std::make_pair(static_cast<Races>(race), info));
    }

    if (validRaces.empty())
        return;

    auto info = Trinity::Containers::SelectRandomContainerElement(validRaces);

    m_FakeRace = info.first;
    m_FakeMorph = (getGender() == GENDER_MALE) ? info.second->displayId_m : info.second->displayId_f;
}

void Player::MorphFit(bool value)
{
    if (!IsPlayingNative() && value)
    {
        SetNativeDisplayId(m_FakeMorph);
        SetDisplayId(m_FakeMorph);
        AddAura((GetOTeam() == ALLIANCE) ? 100024 : 100026, this);
        AddAura((GetOTeam() == ALLIANCE) ? 100025 : 100027, this);
    }
    else
    {
        for (uint32 spellId = 100024; spellId < 100028; spellId++)
            RemoveAurasDueToSpell(spellId);
        InitDisplayIds();
    }
}

void Player::FitPlayerInTeam(bool action, Battleground* pBattleGround)
{
    if (!pBattleGround)
        pBattleGround = GetBattleground();

    if ((!pBattleGround || pBattleGround->isArena()) && action)
        return;

    if(!IsPlayingNative() && action)
        setFactionForRace(getRace());
    else
        setFactionForRace(getORace());

    if (pBattleGround)
    {
        for (Battleground::BattlegroundPlayerMap::const_iterator itr = pBattleGround->GetPlayers().begin(); itr != pBattleGround->GetPlayers().end(); ++itr)
            if (itr->first != GetGUID())
                InvalidatePlayer(itr->first);
    }

    MorphFit(action);

    if (pBattleGround && action)
        SendChatMessage("%sIhr spielt an der Seite der %s%s auf dem Schlachtfeld %s", MSG_COLOR_WHITE, GetTeam() == ALLIANCE ? MSG_COLOR_DARKBLUE"Allianz" : MSG_COLOR_RED"Horde", MSG_COLOR_WHITE, pBattleGround->GetName());
}

void Player::InvalidatePlayer(ObjectGuid playerGUID)
{
    Schedule([playerGUID](Player& me)
    {
        // Here we invalidate players in the bg to the added player
        WorldPacket data1(SMSG_INVALIDATE_PLAYER, 8);
        data1 << playerGUID;
        me.SendDirectMessage(std::move(data1));

        if (Player* otherPlayer = ObjectAccessor::FindConnectedPlayer(playerGUID))
        {
            me.GetSession()->SendNameQueryOpcode(otherPlayer->GetGUID()); // Send namequery answer instantly if player is available
                                                                          // Here we invalidate the player added to players in the bg
            WorldPacket data2(SMSG_INVALIDATE_PLAYER, 8);
            data2 << me.GetGUID();
            otherPlayer->SendDirectMessage(std::move(data2));
            otherPlayer->GetSession()->SendNameQueryOpcode(me.GetGUID());
        }
    });
}

void BattlegroundQueue::StartProposal(Battleground* bg, const Matchmaking::Proposal& proposal)
{
    auto itr = proposal.baseQueue->begin();
    for (size_t index = 0; index < proposal.assigment.size(); index++)
    {
        auto group = *itr;
        itr++;

        int32 team = proposal.assigment[index];
        if (proposal.assigment[index] == TEAM_ALLIANCE)
            InviteGroupToBG(group, bg, ALLIANCE);
        else if (proposal.assigment[index] == TEAM_HORDE)
            InviteGroupToBG(group, bg, HORDE);
    }
}

void Player::SendChatMessage(const char *format, ...)
{
    if (!IsInWorld())
        return;

    if (format)
    {
        va_list ap;
        char str [2048];
        va_start(ap, format);
        vsnprintf(str, 2048, format, ap);
        va_end(ap);

        ChatHandler(GetSession()).SendSysMessage(str);
    }
}
