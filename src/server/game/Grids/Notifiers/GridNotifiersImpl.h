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
 * with this program. If not, see <http://www.gntoUpdate.org/licenses/>.
 */

#ifndef TRINITY_GRIDNOTIFIERSIMPL_H
#define TRINITY_GRIDNOTIFIERSIMPL_H

#include "GridNotifiers.h"
#include "Corpse.h"
#include "Player.h"
#include "CreatureAI.h"
#include "SpellAuras.h"
#include "Opcodes.h"

namespace Trinity
{

    // SEARCHERS & LIST SEARCHERS & WORKERS

    // Creature searchers

    template<class Builder>
    void LocalizedPacketDo<Builder>::operator()(Player* p)
    {
        LocaleConstant loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
        uint32 cache_idx = loc_idx + 1;
        WorldPacket* data;

        // create if not cached yet
        if (i_data_cache.size() < cache_idx + 1 || !i_data_cache[cache_idx])
        {
            if (i_data_cache.size() < cache_idx + 1)
                i_data_cache.resize(cache_idx + 1);

            data = _createPacket();

            i_builder(*data, loc_idx);

            _assertPacket(*data);

            i_data_cache[cache_idx] = data;
        }
        else
            data = i_data_cache[cache_idx];

        p->SendDirectMessage(*data);
    }

    template<class Builder>
    void LocalizedPacketListDo<Builder>::operator()(Player* p)
    {
        LocaleConstant loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
        uint32 cache_idx = loc_idx + 1;
        WorldPacketList* data_list;

        // create if not cached yet
        if (i_data_cache.size() < cache_idx + 1 || i_data_cache[cache_idx].empty())
        {
            if (i_data_cache.size() < cache_idx + 1)
                i_data_cache.resize(cache_idx + 1);

            data_list = &i_data_cache[cache_idx];

            i_builder(*data_list, loc_idx);
        }
        else
            data_list = &i_data_cache[cache_idx];

        for (size_t i = 0; i < data_list->size(); ++i)
            p->SendDirectMessage(*(*data_list)[i]);
    }
}

#endif                                                      // TRINITY_GRIDNOTIFIERSIMPL_H
