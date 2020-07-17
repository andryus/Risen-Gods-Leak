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

#include "ObjectGuid.h"

#include <sstream>
#include <iomanip>

#include "World.h"
#include "ByteBuffer.h"

ObjectGuid const ObjectGuid::Empty = ObjectGuid();

std::string_view ObjectGuid::GetTypeName(HighGuid high)
{
    switch (high)
    {
        case HighGuid::Item:             return "Item";
        case HighGuid::Player:           return "Player";
        case HighGuid::Gameobject:       return "Gameobject";
        case HighGuid::Transport:        return "Transport";
        case HighGuid::Unit:             return "Creature";
        case HighGuid::Pet:              return "Pet";
        case HighGuid::Vehicle:          return "Vehicle";
        case HighGuid::DynamicObject:   return "DynObject";
        case HighGuid::Corpse:           return "Corpse";
        case HighGuid::Mo_Transport: return "MoTransport";
        case HighGuid::Instance:         return "InstanceID";
        case HighGuid::Group:            return "Group";
        default:
            return "<unknown>";
    }
}

std::string ObjectGuid::ToString() const
{
    std::ostringstream str;
    str << "GUID Full: 0x" << std::hex << std::setw(16) << std::setfill('0') << _guid;
    str << " Type: " << GetTypeName().data();
    if (HasEntry())
        str << (IsPet() ? " Pet number: " : " Entry: ") << GetEntry() << " ";

    str << " Low: " << GetCounter();
    return str.str();
}

template<HighGuid high>
uint32 ObjectGuidGenerator<high>::Generate()
{
    if (_nextGuid >= ObjectGuid::GetMaxCounter(high) - 1)
    {
        TC_LOG_ERROR("", "%s guid overflow!! Can't continue, shutting down server. ", ObjectGuid::GetTypeName(high).data());
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _nextGuid++;
}

void PackedGuidTag::read(ObjectGuid& guid, ByteBuffer& buf)
{
    guid = ObjectGuid(readPackGUID(buf));
}

void PackedGuidTag::write(ObjectGuid guid, ByteBuffer& buf)
{
    appendPackGUID(buf, guid.GetRawValue());
}

void packet::BufferIO<PackedGuid>::read(ByteBuffer& buffer)
{
    ASSERT(false); // PackedGuid is not implemented to be read
}
void packet::BufferIO<PackedGuid>::write(ByteBuffer& buffer, const PackedGuid & guid)
{
    const auto guidBuffer = guid.Buffer();
    buffer.append(guidBuffer.data(), sizeof(uint8) * guidBuffer.size());
}

uint64 readPackGUID(ByteBuffer& buffer)
{
    uint64 guid = 0;

    const uint8 guidmark = buffer.read<uint8>();

    for (int i = 0; i < 8; ++i)
    {
        if (guidmark & (uint8(1) << i))
        {
            const uint8 bit = buffer.read<uint8>();
            guid |= (uint64(bit) << (i * 8));
        }
    }

    return guid;
}

template<typename Functor>
uint8 calculatePackGUID(uint64 guid, Functor& append)
{
    uint8 packGUID = 0;
    for (uint8 i = 0; guid != 0; ++i)
    {
        if (guid & 0xFF)
        {
            packGUID |= uint8(1 << i);
            append(uint8(guid & 0xFF));
        }

        guid >>= 8;
    }

    return packGUID;
}

void appendPackGUID(ByteBuffer& buffer, uint64 guid)
{
    constexpr uint8 zero = 0;

    const auto start = buffer.wpos();
    buffer.write<uint8>(0);

    const auto appendBuffer = [&buffer](uint8 guid)
    {
        //buffer.write<uint8>(guid);

        buffer.append(&guid, sizeof(uint8));
    };

    const uint8 front = calculatePackGUID(guid, appendBuffer);
    buffer[start] = front;
}

PackedGuid::PackedGuid(uint64 guid) : 
     PackedGuid(ObjectGuid(guid)) {}

PackedGuid::PackedGuid(ObjectGuid guid)
{
    buffer.reserve(PACKED_MAX_SIZE);
    buffer.push_back(0);

    const auto appendVector = [&buffer = buffer](uint8 guid)
    {
        buffer.push_back(guid);
    };

    buffer.front() = calculatePackGUID(guid.GetRawValue(), appendVector);
}


std::size_t hash_value(ObjectGuid const& guid)
{
    boost::hash<uint64> hasher;
    return hasher(guid.GetRawValue());
}

template uint32 ObjectGuidGenerator<HighGuid::Item>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Player>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Gameobject>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Transport>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Unit>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Pet>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Vehicle>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::DynamicObject>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Corpse>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Instance>::Generate();
template uint32 ObjectGuidGenerator<HighGuid::Group>::Generate();
