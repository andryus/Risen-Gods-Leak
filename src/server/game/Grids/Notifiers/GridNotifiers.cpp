/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Transport.h"
#include "ObjectAccessor.h"

#include "UnitHooks.h"

using namespace Trinity;

void MessageDistDeliverer::Visit(Player& target)
{
    if (!target.InSamePhase(i_phaseMask))
        return;

    if (target.GetExactDist2dSq(i_source) > i_distSq)
        return;

    // Send packet to all who are sharing the player's vision
    if (target.HasSharedVision())
    {
        SharedVisionList::const_iterator i = target.GetSharedVisionList().begin();
        for (; i != target.GetSharedVisionList().end(); ++i)
            if ((*i)->m_seer == &target)
                SendPacket(*i);
    }

    if (target.m_seer == &target || target.GetVehicle())
        SendPacket(&target);
}

void MessageDistDeliverer::Visit(Creature& target)
{
    if (!target.InSamePhase(i_phaseMask))
        return;

    if (target.GetExactDist2dSq(i_source) > i_distSq)
        return;

    // Send packet to all who are sharing the creature's vision
    if (target.HasSharedVision())
    {
        SharedVisionList::const_iterator i = target.GetSharedVisionList().begin();
        for (; i != target.GetSharedVisionList().end(); ++i)
            if ((*i)->m_seer == &target)
                SendPacket(*i);
    }
}

void MessageDistDeliverer::Visit(DynamicObject& target)
{
    if (!target.InSamePhase(i_phaseMask))
        return;

    if (target.GetExactDist2dSq(i_source) > i_distSq)
        return;

    if (Unit* caster = target.GetCaster())
    {
        // Send packet back to the caster if the caster has vision of dynamic object
        Player* player = caster->ToPlayer();
        if (player && player->m_seer == &target)
            SendPacket(player);
    }
}

void MessageDistDeliverer::SendPacket(Player* player)
{
    // never send packet to self
    if (player == i_source || (team && player->GetTeam() != team) || skipped_receiver == player)
        return;

    if (!player->HaveAtClient(i_source))
        return;

    if (WorldSession* session = player->GetSession())
        session->SendPacket(i_message);
}

bool AnyDeadUnitObjectInRangeCheck::operator()(Player* u)
{
    return !u->IsAlive() && !u->HasAuraType(SPELL_AURA_GHOST) && i_searchObj->IsWithinDistInMap(u, i_range);
}

bool AnyDeadUnitObjectInRangeCheck::operator()(Corpse* u)
{
    return u->GetType() != CORPSE_BONES && i_searchObj->IsWithinDistInMap(u, i_range);
}

bool AnyDeadUnitObjectInRangeCheck::operator()(Creature* u)
{
    return !u->IsAlive() && i_searchObj->IsWithinDistInMap(u, i_range);
}

bool AnyDeadUnitSpellTargetInRangeCheck::operator()(Player* u)
{
    return AnyDeadUnitObjectInRangeCheck::operator()(u) && i_check(u);
}

bool AnyDeadUnitSpellTargetInRangeCheck::operator()(Corpse* u)
{
    return AnyDeadUnitObjectInRangeCheck::operator()(u) && i_check(u);
}

bool AnyDeadUnitSpellTargetInRangeCheck::operator()(Creature* u)
{
    return AnyDeadUnitObjectInRangeCheck::operator()(u) && i_check(u);
}

bool Trinity::onNoticed(Unit& me, Unit& target)
{
    if (me.CanSeeOrDetect(&target, false, true))
    {
        if (Creature* creature = me.To<Creature>(); !creature || creature->IsWithinDist(&target, creature->m_SightDistance))
            return me <<= Fire::Noticed.Bind(&target);
    }

    return false;
}

WorldPacket* Trinity::_createPacket()
{
    return new WorldPacket();
}

void Trinity::_assertPacket(WorldPacket& packet)
{
    ASSERT(packet.GetOpcode() != MSG_NULL_ACTION);
}
