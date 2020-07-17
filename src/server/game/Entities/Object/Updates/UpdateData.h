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

#ifndef __UPDATEDATA_H
#define __UPDATEDATA_H

#include <set>

#include "BaseByteBuffer.h"
#include "ObjectGuid.h"

class ByteBuffer;
class WorldPacket;

class UpdateData
{
    public:
        UpdateData();
        UpdateData(uint32 startSize);
        UpdateData(UpdateData&& right) = default;

        // alawys returns ref to the same object, but increases block-count on each call
        ByteBuffer& RequestBuffer();
        // invalidates the buffers content, resuse MUST be set to true for resual
        WorldPacket BuildPacket(bool reuse = false);
        bool HasData() const;

    private:

        void InitBuffer();
        ByteBuffer& Buffer();

        packet::BaseByteBuffer m_data;
        uint32_t numBlocks = 0;

        UpdateData(UpdateData const& right) = delete;
        UpdateData& operator=(UpdateData const& right) = delete;
};
#endif

WorldPacket buildOutOfRangePacket(ObjectGuid guid);
WorldPacket buildOutOfRangePacket(span<ObjectGuid> guids);