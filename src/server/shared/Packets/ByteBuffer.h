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

#include "BufferTraits.h"
#include "Errors.h"
#include "Util.h"

#include <list>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>

class ByteBuffer :
    public packet::BaseByteBuffer
{
public:
    // constructor

    ByteBuffer();
    using BaseByteBuffer::BaseByteBuffer;

    // IO

    template <typename Type, typename = packet::EnableIfBufferType<Type>>
    auto read()
    {
        using Remapped = packet::ByteBufferRemappedType<Type>;
        using IO = packet::BufferIO<Remapped>;

        auto readValue = IO::read(*this);
        packet::check_type(readValue);

        return packet::RevertRemapping<Type, Remapped>::output(std::move(readValue));
    }

    template<typename Type, typename = packet::EnableIfBufferType<Type>>
    void read(Type& value)
    {
        using Remapped = packet::ByteBufferRemappedType<Type>;
        using IO = packet::BufferIO<Remapped>;

        // check if IO has read_modify specified
        if constexpr(packet::ReadByModify<ByteBuffer, Remapped>)
            IO::read_modify(*this, value);
        else
            value = read<Type>();
    }

    template<typename Type, typename = packet::EnableIfBufferType<Type>>
    void read_skip()
    {
        using Remapped = packet::ByteBufferRemappedType<Type>;

        packet::BufferIO<Remapped>::read(*this);
    }

    template<typename Type, typename = packet::EnableIfBufferType<Type>>
    void write(const Type& value)
    {
        using Remapped = packet::ByteBufferRemappedType<Type>;

        packet::BufferIO<Remapped>::write(*this, static_cast<Remapped>(value));
    }

    template<typename Type, typename = packet::EnableIfBufferType<Type>>
    void replace(size_t pos, const Type& value)
    {
        const size_t oldPos = wpos();
        set_wpos(pos);
        write(value);

        set_wpos(oldPos);
    }

    // operators

    template<typename Type, typename = packet::EnableIfBufferType<Type>>
    ByteBuffer& operator<<(const Type& value)
    {
        write<Type>(value);

        return *this;
    }

    template<typename Type, typename = packet::EnableIfBufferType<Type>>
    ByteBuffer& operator>>(Type& value)
    {
        read<Type>(value);

        return *this;
    }

    template<typename Tag, typename Type, typename = packet::EnableIfBufferType<Type>>
    ByteBuffer& operator>>(packet::BufferTag<Tag, Type> value) // todo: remove dependency on BufferTagValue-class (got problems with temporary-to-rvalue
    {
        read(value);

        return *this;
    }

    uint8& operator[](size_t const pos)
    {
        if (pos + sizeof(uint8) > size())
            throw ByteBufferPositionException(false, pos, sizeof(uint8), size());

        return data()[pos];
    }

    const uint8& operator[](size_t const pos) const
    {
        if (pos + sizeof(uint8) > size())
            throw ByteBufferPositionException(false, pos, sizeof(uint8), size());

        return data()[pos];
    }

    uint8* contents()
    {
        if (empty())
            throw ByteBufferException();
        return data();
    }

    const uint8* contents() const
    {
        if (empty())
            throw ByteBufferException();
        return data();
    }
};

/**********************************************
* Default type-registartion
* (those could/should be moved to another header, but need to change includes etc...
**********************************************/

// float/double has an extra check
template<>
inline void packet::check_type<float>(const float& value)
{
    if (!std::isfinite(value))
        throw ByteBufferException();
}
template<>
inline void packet::check_type<double>(const double& value)
{
    if (!std::isfinite(value))
        throw ByteBufferException();
}

//
// handle strings - internally use a string_view
// 

namespace packet
{
    template<>
    constexpr bool IsByteBufferType<std::string_view> = true;
    template<>
    struct ByteBufferRemapped<std::string> { using type = std::string_view; };
    template<>
    struct ByteBufferRemapped<const char*> { using type = std::string_view; };
    template<>
    struct ByteBufferRemapped<char*> { using type = std::string_view; };
    template<size_t Size>
    struct ByteBufferRemapped<char[Size]> { using type = std::string_view; };
    template<size_t Size>
    struct ByteBufferRemapped<const char[Size]> { using type = std::string_view; };

    template<>
    struct BufferIO<std::string_view>
    {
        static std::string_view read(ByteBuffer& buffer);

        static void write(ByteBuffer& buffer, std::string_view string);
    };

    template<>
    struct RevertRemapping<const char*, std::string_view>
    {
        static const char* output(std::string_view string) { return string.data(); };
    };
}

//
// handle container - internally using span
// 

namespace packet
{
    template<typename Type>
    struct IsByteBufferAcceptedType<span<Type>> : std::true_type {};

    template<typename Type>
    struct ByteBufferRemapped<std::list<Type>> { using type = span<Type>; };
    template<typename Type>
    struct ByteBufferRemapped<std::vector<Type>> { using type = span<Type>; };

    template<typename Type>
    struct BufferIO<span<Type>>
    {
        static auto read(ByteBuffer& buffer)
        {
            const uint32 size = buffer.read<uint32>();
            
            return buffer.read_data<Type>(size);
        }

        static void write(ByteBuffer& buffer, span<Type> container)
        {
            const uint32 size = container.size();

            buffer.append<Type>(container.data(), size);
        }
    };

    template<typename Type>
    struct RevertRemapping<std::vector<Type>, span<Type>>
    {
        static auto output(span<Type> span)
        {
            return std::vector<Type>(span.begin(), span.end());
        };
    };

    template<typename Type>
    struct RevertRemapping<std::list<Type>, span<Type>>
    {
        static auto output(span<Type> span)
        {
            return std::list<Type>(span.begin(), span.end());
        };
    };
}

//
// handle map
// 

namespace packet
{
    template<typename Key, typename Type>
    struct IsByteBufferAcceptedType<std::map<Key, Type>> : std::true_type {};

    template<typename Key, typename Type>
    struct BufferIO<std::map<Key, Type>>
    {
        static auto read(ByteBuffer& buffer)
        {
            std::map<Key, Type> map; 
            const uint32 size = buffer.read<uint32>();

            for (uint32 i = 0; i < size; i++)
            {
                auto key = buffer.read<Key>();
                auto value = buffer.read<Type>();

                map.emplace(std::move(key), std::move(value));
            };
        }

        static void write(ByteBuffer& buffer, std::map<Key, Type> map)
        {
            buffer << map.size();

            for (const auto [key, value] : map)
            {
                buffer << key;
                buffer << value;
            }
        }
    };
}

//
// Packed time
//

struct PackedTimeTag
{
    static void read(time_t& time, ByteBuffer& buf);
    static void write(time_t time, ByteBuffer& buf);
};

inline auto asPackedTime(const time_t& time)
{
    return packet::with_tag<PackedTimeTag>(time);
}

uint32 createPackedTime(time_t time);
