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

#include "boost/dynamic_bitset.hpp"

#include <set>
#include <string>

#include "Common.h"
#include "UpdateFields.h"
#include "ObjectDefines.h"

constexpr float CONTACT_DISTANCE      = 0.5f;
constexpr float INTERACTION_DISTANCE  = 5.0f;

class Corpse;
class Creature;
class DynamicObject;
class GameObject;
class Player;
class Unit;
class UpdateData;
class WorldObject;
class Object;
class WorldPacket;

using UpdateMask = boost::dynamic_bitset<uint32>;

namespace packet
{
    template<>
    constexpr bool IsByteBufferType<UpdateMask> = true;

    template<>
    class BufferIO<UpdateMask>
    {
    public:

        static void write(ByteBuffer& buffer, const UpdateMask& mask);
    };
}

namespace impl
{
    // Transforms Type to related TYPEID
    template<class Type>
    constexpr TypeID TypeToId = TypeID::INVALID;
    template<>
    constexpr TypeID TypeToId<Object> = TypeID::OBJECT;
    template<>
    constexpr TypeID TypeToId<Creature> = TypeID::UNIT;
    template<>
    constexpr TypeID TypeToId<Player> = TypeID::PLAYER;
    template<>
    constexpr TypeID TypeToId<GameObject> = TypeID::GAMEOBJECT;
    template<>
    constexpr TypeID TypeToId<DynamicObject> = TypeID::DYNAMICOBJECT;
    template<>
    constexpr TypeID TypeToId<Corpse> = TypeID::CORPSE;

    // Compares dynamic ID to Type-related id
    template<class Type>
    bool isOfType(TypeID id)
    {
        if (std::is_same_v<WorldObject, Type>)
            return true;
        else
        {
            static_assert(TypeToId<Type> != TypeID::INVALID, "Invalid Type, or implementation failure (see TypeToId<>))");

            return id == TypeToId<Type>;
        }
    }

    template<>
    inline bool isOfType<Unit>(TypeID id) { return isOfType<Creature>(id) || isOfType<Player>(id); }
}

enum class ObjectUpdateType : uint8;

class GAME_API Object
{
    public:
        virtual ~Object();

        bool IsInWorld() const { return m_inWorld; }

        virtual void AddToWorld();
        virtual void RemoveFromWorld();

        ObjectGuid GetGUID() const { return GetGuidValue(OBJECT_FIELD_GUID); }
        PackedGuid const& GetPackGUID() const { return m_PackGUID; }
        uint32 GetEntry() const { return GetUInt32Value(OBJECT_FIELD_ENTRY); }
        void SetEntry(uint32 entry) { SetUInt32Value(OBJECT_FIELD_ENTRY, entry); }

        float GetObjectScale() const { return GetFloatValue(OBJECT_FIELD_SCALE_X); }
        virtual void SetObjectScale(float scale) { SetFloatValue(OBJECT_FIELD_SCALE_X, scale); }

        TypeID GetTypeId() const { return m_objectTypeId; }
        bool isType(uint16 mask) const { return (mask & m_objectType) != 0; }

        void SendUpdateToPlayer(Player* player);

        WorldPacket CreateValuesUpdateBlockForPlayer(Player* target) const;
        void BuildValuesUpdateBlockForPlayer(UpdateData& buffer, Player* target) const;
        WorldPacket CreateUpdateBlockForPlayer(Player* target) const;
        virtual void BuildUpdateBlockForPlayer(UpdateData& data, Player* target) const;

        virtual void DestroyForPlayer(Player* target, bool onDeath = false) const;

        int32 GetInt32Value(uint16 index) const;
        uint32 GetUInt32Value(uint16 index) const;
        uint64 GetUInt64Value(uint16 index) const;
        float GetFloatValue(uint16 index) const;
        uint8 GetByteValue(uint16 index, uint8 offset) const;
        uint16 GetUInt16Value(uint16 index, uint8 offset) const;
        ObjectGuid GetGuidValue(uint16 index) const;

        void SetInt32Value(uint16 index, int32 value);
        void SetUInt32Value(uint16 index, uint32 value);
        void UpdateUInt32Value(uint16 index, uint32 value);
        void SetUInt64Value(uint16 index, uint64 value);
        void SetFloatValue(uint16 index, float value);
        void SetByteValue(uint16 index, uint8 offset, uint8 value);
        void SetUInt16Value(uint16 index, uint8 offset, uint16 value);
        void SetInt16Value(uint16 index, uint8 offset, int16 value) { SetUInt16Value(index, offset, (uint16)value); }
        void SetGuidValue(uint16 index, ObjectGuid value);
        void SetStatFloatValue(uint16 index, float value);
        void SetStatInt32Value(uint16 index, int32 value);

        bool AddGuidValue(uint16 index, ObjectGuid value);
        bool RemoveGuidValue(uint16 index, ObjectGuid value);

        void ApplyModUInt32Value(uint16 index, int32 val, bool apply);
        void ApplyModInt32Value(uint16 index, int32 val, bool apply);
        void ApplyModUInt64Value(uint16 index, int32 val, bool apply);
        void ApplyModPositiveFloatValue(uint16 index, float val, bool apply);
        void ApplyModSignedFloatValue(uint16 index, float val, bool apply);
        void ApplyPercentModFloatValue(uint16 index, float val, bool apply);

        void SetFlag(uint16 index, uint32 newFlag);
        void RemoveFlag(uint16 index, uint32 oldFlag);
        void ToggleFlag(uint16 index, uint32 flag);
        bool HasFlag(uint16 index, uint32 flag) const;
        void ApplyModFlag(uint16 index, uint32 flag, bool apply);

        void SetByteFlag(uint16 index, uint8 offset, uint8 newFlag);
        void RemoveByteFlag(uint16 index, uint8 offset, uint8 newFlag);
        void ToggleByteFlag(uint16 index, uint8 offset, uint8 flag);
        bool HasByteFlag(uint16 index, uint8 offset, uint8 flag) const;

        void SetFlag64(uint16 index, uint64 newFlag);
        void RemoveFlag64(uint16 index, uint64 oldFlag);
        void ToggleFlag64(uint16 index, uint64 flag);
        bool HasFlag64(uint16 index, uint64 flag) const;
        void ApplyModFlag64(uint16 index, uint64 flag, bool apply);

        void ClearUpdateMask(bool remove);

        uint16 GetValuesCount() const { return m_valuesCount; }

        virtual bool hasQuest(uint32 /* quest_id */) const { return false; }
        virtual bool hasInvolvedQuest(uint32 /* quest_id */) const { return false; }
        virtual void BroadcastValuesUpdate() { }

        void SetFieldNotifyFlag(uint16 flag) { _fieldNotifyFlags |= flag; }
        void RemoveFieldNotifyFlag(uint16 flag) { _fieldNotifyFlags &= uint16(~flag); }

        void ForceValuesUpdateAtIndex(uint32);

        template<class Type> 
        bool IsOfType() const { return impl::isOfType<Type>(GetTypeId()); }

        template<class Type>
        Type* To() { return IsOfType<Type>() ? static_cast<Type*>(this) : nullptr; }
        template<class Type>
        const Type* To() const { return IsOfType<Type>() ? static_cast<const Type*>(this) : nullptr; }

        // specified functions - mostly for backwards-compatibility

        Player* ToPlayer();
        const Player* ToPlayer() const;

        Creature* ToCreature();
        const Creature* ToCreature() const;

        Unit* ToUnit();
        const Unit* ToUnit() const;

        GameObject* ToGameObject();
        const GameObject* ToGameObject() const;

        Corpse* ToCorpse();
        const Corpse* ToCorpse() const;

        DynamicObject* ToDynObject();
        DynamicObject const* ToDynObject() const;

    protected:
        Object();

        void _InitValues();
        void _Create(uint32 guidlow, uint32 entry, HighGuid guidhigh);
        std::string _ConcatFields(uint16 startIndex, uint16 size) const;
        void _LoadIntoDataField(std::string const& data, uint32 startOffset, uint32 count);

        uint32 GetUpdateFieldData(Player const* target, uint32*& flags) const;

        void BuildMovementUpdate(ByteBuffer& data, uint16 flags) const;
        virtual void BuildValuesUpdate(ObjectUpdateType updatetype, ByteBuffer& data, Player* target) const;

        /**
         * Prepares an object values update for this object. All players that can see this will receive a broadcast 
         * packet with updated object values.
         */
        virtual void ScheduleObjectUpdate() = 0;
        /**
        * Stops a pending object values update request.
        */
        virtual void AbortObjectUpdate() = 0;

        uint16 m_objectType;

        TypeID m_objectTypeId;
        uint16 m_updateFlag;

        union
        {
            int32  *m_int32Values;
            uint32 *m_uint32Values;
            float  *m_floatValues;
        };

        UpdateMask _changesMask;

        uint16 m_valuesCount;

        uint16 _fieldNotifyFlags;

        bool m_objectUpdated;

    private:
        bool m_inWorld;

        PackedGuid m_PackGUID;

        // for output helpfull error messages from asserts
        bool PrintIndexError(uint32 index, bool set) const;
        Object(Object const& right) = delete;
        Object& operator=(Object const& right) = delete;
};
