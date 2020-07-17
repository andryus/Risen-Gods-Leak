#include "ByteBuffer.h"

constexpr size_t DEFAULT_SIZE = 0x1000;

ByteBuffer::ByteBuffer() :
    ByteBuffer(DEFAULT_SIZE)
{
}

// string

std::string_view packet::BufferIO<std::string_view>::read(ByteBuffer& buffer)
{
    const uint32 start = buffer.rpos();
    uint32 size = 0;
    while (buffer.rpos() < buffer.size()) // prevent crash at wrong string format in packet
    {
        if (buffer.read<char>() == '\0')
            break;
        size++; // exclude 0-terminator
    }

    const char* string = reinterpret_cast<const char*>(&buffer.data()[start]);

    return std::string_view(string, size);
}

void packet::BufferIO<std::string_view>::write(ByteBuffer& buffer, std::string_view string)
{
    if (const size_t len = string.length())
        buffer.append(string.data(), len);
    buffer.write<char>('\0');
}

// time


uint32 createPackedTime(time_t time)
{
    tm lt;
    localtime_r(&time, &lt);

    return (lt.tm_year - 100) << 24 | lt.tm_mon << 20 | (lt.tm_mday - 1) << 14 | lt.tm_wday << 11 | lt.tm_hour << 6 | lt.tm_min;
}

void PackedTimeTag::read(time_t& time, ByteBuffer& buf)
{
    uint32 packedDate = buf.read<uint32>();
    tm lt = tm();

    lt.tm_min = packedDate & 0x3F;
    lt.tm_hour = (packedDate >> 6) & 0x1F;
    //lt.tm_wday = (packedDate >> 11) & 7;
    lt.tm_mday = ((packedDate >> 14) & 0x3F) + 1;
    lt.tm_mon = (packedDate >> 20) & 0xF;
    lt.tm_year = ((packedDate >> 24) & 0x1F) + 100;

    time = mktime(&lt);
}

void PackedTimeTag::write(time_t time, ByteBuffer& buf)
{
    buf << createPackedTime(time);
}
