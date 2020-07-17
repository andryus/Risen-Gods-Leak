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

#ifndef ObjectGuid_h__
#define ObjectGuid_h__

#include "Common.h"

#include <functional>
#include "Packets/BufferTraits.h"

enum class TypeID : uint8
{
    OBJECT = 0,
    ITEM = 1,
    CONTAINER = 2,
    UNIT = 3,
    PLAYER = 4,
    GAMEOBJECT = 5,
    DYNAMICOBJECT = 6,
    CORPSE = 7,
    INVALID = 255,
};

// backwards compatibility

constexpr TypeID TYPEID_OBJECT = TypeID::OBJECT;
constexpr TypeID TYPEID_ITEM = TypeID::ITEM;
constexpr TypeID TYPEID_CONTAINER = TypeID::CONTAINER;
constexpr TypeID TYPEID_UNIT = TypeID::UNIT;
constexpr TypeID TYPEID_PLAYER = TypeID::PLAYER;
constexpr TypeID TYPEID_GAMEOBJECT = TypeID::GAMEOBJECT;
constexpr TypeID TYPEID_DYNAMICOBJECT = TypeID::DYNAMICOBJECT;
constexpr TypeID TYPEID_CORPSE = TypeID::CORPSE;

constexpr uint8 NUM_CLIENT_OBJECT_TYPES = 8;

enum TypeMask
{
    TYPEMASK_OBJECT         = 0x0001,
    TYPEMASK_ITEM           = 0x0002,
    TYPEMASK_CONTAINER      = 0x0006,                       // TYPEMASK_ITEM | 0x0004
    TYPEMASK_UNIT           = 0x0008,                       // creature
    TYPEMASK_PLAYER         = 0x0010,
    TYPEMASK_GAMEOBJECT     = 0x0020,
    TYPEMASK_DYNAMICOBJECT  = 0x0040,
    TYPEMASK_CORPSE         = 0x0080,
    TYPEMASK_SEER           = TYPEMASK_PLAYER | TYPEMASK_UNIT | TYPEMASK_DYNAMICOBJECT
};

enum class HighGuid
{
    Item              = 0x4000, // blizz 4000
    Container         = 0x4000, // blizz 4000
    Player            = 0x0000, // blizz 0000
    Gameobject        = 0xF110, // blizz F110
    Transport         = 0xF120, // blizz F120 (for GAMEOBJECT_TYPE_TRANSPORT)
    Unit              = 0xF130, // blizz F130
    Pet               = 0xF140, // blizz F140
    Vehicle           = 0xF150, // blizz F550
    DynamicObject     = 0xF100, // blizz F100
    Corpse            = 0xF101, // blizz F100
    Mo_Transport      = 0x1FC0, // blizz 1FC0 (for GAMEOBJECT_TYPE_MO_TRANSPORT)
    Instance          = 0x1F40, // blizz 1F40
    Group             = 0x1F50
};

class ObjectGuid;

class GAME_API ObjectGuid
{
    public:

        // todo: constexpr, a bit complicated
        static const ObjectGuid Empty;

        constexpr ObjectGuid() : _guid(0) { }
        constexpr explicit ObjectGuid(uint64 guid) : _guid(guid) { }
        constexpr ObjectGuid(HighGuid hi, uint32 entry, uint32 counter) : _guid(counter ? uint64(counter) | (uint64(entry) << 24) | (uint64(hi) << 48) : 0) { }
        constexpr ObjectGuid(HighGuid hi, uint32 counter) : _guid(counter ? uint64(counter) | (uint64(hi) << 48) : 0) { }

        constexpr explicit operator uint64() const { return _guid; }
        auto ReadAsPacked();

        constexpr void Set(uint64 guid) { _guid = guid; }
        constexpr void Clear() { _guid = 0; }

        auto WriteAsPacked() const;

        constexpr uint64   GetRawValue() const { return _guid; }
        constexpr HighGuid GetHigh() const { return HighGuid((_guid >> 48) & 0x0000FFFF); }
        constexpr uint32   GetEntry() const { return HasEntry() ? uint32((_guid >> 24) & UI64LIT(0x0000000000FFFFFF)) : 0; }
        constexpr uint32   GetCounter()  const
        {
            return HasEntry()
                   ? uint32(_guid & UI64LIT(0x0000000000FFFFFF))
                   : uint32(_guid & UI64LIT(0x00000000FFFFFFFF));
        }

        static constexpr uint32 GetMaxCounter(HighGuid high)
        {
            return HasEntry(high)
                   ? uint32(0x00FFFFFF)
                   : uint32(0xFFFFFFFF);
        }

        constexpr uint32 GetMaxCounter() const { return GetMaxCounter(GetHigh()); }

        constexpr bool IsEmpty()             const { return _guid == 0; }
        constexpr bool IsCreature()          const { return GetHigh() == HighGuid::Unit; }
        constexpr bool IsPet()               const { return GetHigh() == HighGuid::Pet; }
        constexpr bool IsVehicle()           const { return GetHigh() == HighGuid::Vehicle; }
        constexpr bool IsCreatureOrPet()     const { return IsCreature() || IsPet(); }
        constexpr bool IsCreatureOrVehicle() const { return IsCreature() || IsVehicle(); }
        constexpr bool IsAnyTypeCreature()   const { return IsCreature() || IsPet() || IsVehicle(); }
        constexpr bool IsPlayer()            const { return !IsEmpty() && GetHigh() == HighGuid::Player; }
        constexpr bool IsUnit()              const { return IsAnyTypeCreature() || IsPlayer(); }
        constexpr bool IsItem()              const { return GetHigh() == HighGuid::Item; }
        constexpr bool IsGameObject()        const { return GetHigh() == HighGuid::Gameobject; }
        constexpr bool IsDynamicObject()     const { return GetHigh() == HighGuid::DynamicObject; }
        constexpr bool IsCorpse()            const { return GetHigh() == HighGuid::Corpse; }
        constexpr bool IsTransport()         const { return GetHigh() == HighGuid::Transport; }
        constexpr bool IsMOTransport()       const { return GetHigh() == HighGuid::Mo_Transport; }
        constexpr bool IsAnyTypeGameObject() const { return IsGameObject() || IsTransport() || IsMOTransport(); }
        constexpr bool IsInstance()          const { return GetHigh() == HighGuid::Instance; }
        constexpr bool IsGroup()             const { return GetHigh() == HighGuid::Group; }

        static constexpr TypeID GetTypeId(HighGuid high)
        {
            switch (high)
            {
                case HighGuid::Item:         return TypeID::ITEM;
                //case HighGuid::Container:    return TypeId::CONTAINER; HighGuid::Container==HighGuid::ITEM currently
                case HighGuid::Unit:         return TypeID::UNIT;
                case HighGuid::Pet:          return TypeID::UNIT;
                case HighGuid::Player:       return TypeID::PLAYER;
                case HighGuid::Gameobject:   return TypeID::GAMEOBJECT;
                case HighGuid::DynamicObject: return TypeID::DYNAMICOBJECT;
                case HighGuid::Corpse:       return TypeID::CORPSE;
                case HighGuid::Mo_Transport: return TypeID::GAMEOBJECT;
                case HighGuid::Vehicle:      return TypeID::UNIT;
                // unknown
                case HighGuid::Instance:
                case HighGuid::Group:
                default:                    return TypeID::OBJECT;
            }
        }

        TypeID GetTypeId() const { return GetTypeId(GetHigh()); }

        constexpr explicit operator bool() const { return !IsEmpty(); }
        constexpr bool operator!() const { return IsEmpty(); }
        constexpr bool operator== (ObjectGuid const& guid) const { return GetRawValue() == guid.GetRawValue(); }
        constexpr bool operator!= (ObjectGuid const& guid) const { return GetRawValue() != guid.GetRawValue(); }
        constexpr bool operator< (ObjectGuid const& guid) const { return GetRawValue() < guid.GetRawValue(); }

        static std::string_view GetTypeName(HighGuid high);
        std::string_view GetTypeName() const { return !IsEmpty() ? GetTypeName(GetHigh()) : "None"; }
        std::string ToString() const;

    private:
        static constexpr bool HasEntry(HighGuid high)
        {
            switch (high)
            {
                case HighGuid::Item:
                case HighGuid::Player:
                case HighGuid::DynamicObject:
                case HighGuid::Corpse:
                case HighGuid::Mo_Transport:
                case HighGuid::Instance:
                case HighGuid::Group:
                    return false;
                case HighGuid::Gameobject:
                case HighGuid::Transport:
                case HighGuid::Unit:
                case HighGuid::Pet:
                case HighGuid::Vehicle:
                default:
                    return true;
            }
        }

        constexpr bool HasEntry() const { return HasEntry(GetHigh()); }

        explicit ObjectGuid(uint32 const&) = delete;                 // no implementation, used to catch wrong type assignment
        ObjectGuid(HighGuid, uint32, uint64 counter) = delete;       // no implementation, used to catch wrong type assignment
        ObjectGuid(HighGuid, uint64 counter) = delete;               // no implementation, used to catch wrong type assignment

        uint64 _guid;
};

namespace packet
{
    template<>
    struct ByteBufferRemapped<ObjectGuid> { using type = uint64; };
}

// Some Shared defines
using GuidSet = std::set<ObjectGuid>;
using GuidList = std::list<ObjectGuid>;
using GuidDeque = std::deque<ObjectGuid>;
using GuidVector = std::vector<ObjectGuid>;

/*******************************
* Packed GUID
********************************/

uint64 readPackGUID(ByteBuffer& buffer);
void appendPackGUID(ByteBuffer& buffer, uint64 guid);

// reader
struct PackedGuidTag
{
    static void read(ObjectGuid& guid, ByteBuffer& buf);
    static void write(ObjectGuid guid, ByteBuffer& buf);
};

// todo: only one function, requires changes in many files though
inline auto ObjectGuid::ReadAsPacked() { return packet::with_tag<PackedGuidTag>(*this); };
inline auto ObjectGuid::WriteAsPacked() const { return packet::with_tag<PackedGuidTag>(*this); }

// storage
struct PackedGuid
{
public:

    static constexpr uint8 PACKED_MAX_SIZE = 9;

    PackedGuid() = default;
    PackedGuid(uint64 guid);
    PackedGuid(ObjectGuid guid);

    span<uint8> Buffer() const { return buffer; }

    size_t size() const { return buffer.size(); }

private:

    std::vector<uint8> buffer;
};

namespace packet
{
    template<> // register IO to buffer, so Read/WriteAsPacked can be used
    constexpr bool IsByteBufferType<PackedGuid> = true;

    template<>
    struct GAME_API BufferIO<PackedGuid>
    {
        static void read(ByteBuffer& buffer);

        static void write(ByteBuffer& buffer, const PackedGuid& guid);
    };
}

//
// Generator, Hash
//

template<HighGuid high>
class ObjectGuidGenerator
{
    public:
        explicit ObjectGuidGenerator(uint32 start = 1) : _nextGuid(start) {}

        void Set(uint32 val) { _nextGuid = val; }
        uint32 Generate();
        uint32 GetNextAfterMaxUsed() const { return _nextGuid; }

    private:
        uint32 _nextGuid;
};

GAME_API std::size_t hash_value(const ObjectGuid& guid);

namespace std
{
    template<>
    struct hash<ObjectGuid>
    {
        public:
            size_t operator()(ObjectGuid const& key) const
            {
                return hash<uint64>()(key.GetRawValue());
            }
    };
}

#endif // ObjectGuid_h__
