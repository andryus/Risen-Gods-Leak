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

#include "Common.h"
#include "WorldPacket.h"
#include "UpdateData.h"
#include "Log.h"
#include "Opcodes.h"
#include "World.h"
#include "UpdateFieldFlags.h"
#include "zlib.h"

// we prematurely allocate space for the number of blocks
// saves us from copying the whole buffer
// todo: allocate for compressed-size as well - requires in-place compression
constexpr uint32 HeaderSize = sizeof(uint32);

WorldPacket buildCompressedPacket(ByteBuffer&& buf);

UpdateData::UpdateData() : m_data(WorldPacket::ResourceSize + HeaderSize)
{
    InitBuffer();
}

UpdateData::UpdateData(uint32 startSize) : m_data(startSize + HeaderSize)
{
    InitBuffer();
}

ByteBuffer& UpdateData::RequestBuffer()
{
    // make sure buffer is initialized
    ASSERT(m_data.size() >= HeaderSize);

    numBlocks++;

    return Buffer();
}

WorldPacket UpdateData::BuildPacket(bool reuse)
{    
    // make sure buffer is at least initialized
    ASSERT(m_data.size() >= HeaderSize);

    ByteBuffer& buffer = Buffer();
    // write num blocks
    buffer.replace(0, numBlocks);
    numBlocks = 0;

    const size_t oldSize = buffer.size(); // store old size for reusal
    WorldPacket packet = buildCompressedPacket(std::move(buffer));

    if (reuse)
    {
        buffer.reserve(oldSize);
        InitBuffer();
    }
    
    return packet;
}

bool UpdateData::HasData() const 
{ 
    return numBlocks && m_data.size() > HeaderSize;
}

void UpdateData::InitBuffer()
{
    // num blocks
    Buffer() << uint32(0);
}

ByteBuffer& UpdateData::Buffer()
{    
    // byte-buffer is just a functional interface on top of BaseByteBuffer
    return static_cast<ByteBuffer&>(m_data);
}

uint32 compress(uint8* dst, uint32 dst_size, uint8* src, uint32 src_size)
{
    z_stream c_stream;

    c_stream.zalloc = (alloc_func)nullptr;
    c_stream.zfree = (free_func)nullptr;
    c_stream.opaque = (voidpf)nullptr;

    // default Z_BEST_SPEED (1)
    int z_res = deflateInit(&c_stream, sWorld->getIntConfig(CONFIG_COMPRESSION));
    if (z_res != Z_OK)
    {
        TC_LOG_ERROR("misc", "Can't compress update packet (zlib: deflateInit) Error code: %i (%s)", z_res, zError(z_res));
        return 0;
    }

    c_stream.next_out = (Bytef*)dst;
    c_stream.avail_out = dst_size;
    c_stream.next_in = (Bytef*)src;
    c_stream.avail_in = (uInt)src_size;

    z_res = deflate(&c_stream, Z_NO_FLUSH);
    if (z_res != Z_OK)
    {
        TC_LOG_ERROR("misc", "Can't compress update packet (zlib: deflate) Error code: %i (%s)", z_res, zError(z_res));
        return 0;
    }

    if (c_stream.avail_in != 0)
    {
        TC_LOG_ERROR("misc", "Can't compress update packet (zlib: deflate not greedy)");
        return 0;
    }

    z_res = deflate(&c_stream, Z_FINISH);
    if (z_res != Z_STREAM_END)
    {
        TC_LOG_ERROR("misc", "Can't compress update packet (zlib: deflate should report Z_STREAM_END instead %i (%s)", z_res, zError(z_res));
        return 0;
    }

    z_res = deflateEnd(&c_stream);
    if (z_res != Z_OK)
    {
        TC_LOG_ERROR("misc", "Can't compress update packet (zlib: deflateEnd) Error code: %i (%s)", z_res, zError(z_res));
        return 0;
    }

    return c_stream.total_out;
}

WorldPacket buildCompressedPacket(ByteBuffer&& buf)
{
    const uint32 packetSize = buf.size(); // use real used data size

    if (packetSize > 100) // compress large packets
    {
        const uint32 destSize = compressBound(packetSize);

        WorldPacket data(SMSG_COMPRESSED_UPDATE_OBJECT, destSize + sizeof(uint32));
        data.resize(destSize + sizeof(uint32));
        data.replace(0, destSize);

        const uint32 compressedSize = compress(data.contents() + sizeof(uint32), destSize, buf.contents(), packetSize);
        buf.clear();
        if (compressedSize)
            data.resize(compressedSize + sizeof(uint32));

        return data;
    }
    else // send small packets without compression
    {
        WorldPacket data(SMSG_UPDATE_OBJECT, std::move(buf));
        return data;
    }
}

WorldPacket buildOutOfRangePacket(ObjectGuid guid)
{
    return buildOutOfRangePacket(make_span(guid));
}

WorldPacket buildOutOfRangePacket(span<ObjectGuid> outOfRangeGuids)
{
    ByteBuffer buf(4 + 1 + 4 + 9 * outOfRangeGuids.size());

    buf << (uint32)(!outOfRangeGuids.empty() ? 1 : 0);

    if (!outOfRangeGuids.empty())
    {
        buf << ObjectUpdateType::OUT_OF_RANGE_OBJECTS;
        buf << uint32(outOfRangeGuids.size());

        for (const ObjectGuid guid : outOfRangeGuids)
            buf << guid.WriteAsPacked();
    }

    return buildCompressedPacket(std::move(buf));
}
