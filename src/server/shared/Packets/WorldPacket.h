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

#pragma once

#include "Common.h"
#include "ByteBuffer.h"

class MessageBuffer;

class WorldPacket : 
    public ByteBuffer
{
public:
    static constexpr size_t ResourceSize = 192;

    WorldPacket();

    explicit WorldPacket(uint16 opcode, size_t res = ResourceSize);
    WorldPacket(WorldPacket&& packet);

    WorldPacket(const WorldPacket& right) = default;
    WorldPacket& operator=(const WorldPacket& right) = default;

    WorldPacket(uint16 opcode, MessageBuffer&& buffer);
    WorldPacket(uint16 opcode, ByteBuffer&& buffer);

    void Initialize(uint16 opcode, size_t newres = ResourceSize);

    uint16 GetOpcode() const { return m_opcode; }
    void SetOpcode(uint16 opcode) { m_opcode = opcode; }

private:
    uint16 m_opcode = 0;
};
