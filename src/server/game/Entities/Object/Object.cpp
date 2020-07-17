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

#include "boost/function_output_iterator.hpp"

#include "Object.h"
#include "Common.h"
#include "SharedDefines.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "World.h"
#include "Creature.h"
#include "Player.h"
#include "Vehicle.h"
#include "ObjectMgr.h"
#include "UpdateData.h"
#include "Util.h"
#include "UpdateFieldFlags.h"
#include "Battlefield.h"
#include "MovementPacketBuilder.h"
#include "Transport.h"
#include "GameTime.h"

void packet::BufferIO<UpdateMask>::write(ByteBuffer& buffer, const UpdateMask& mask)
{
    buffer << uint8(mask.num_blocks());

    // boost output-iterator adaptor can't use reference (needs to be assignable)
    auto writeToBuffer  = [&buffer](uint32 value) { buffer << value; };

    boost::to_block_range(mask, boost::make_function_output_iterator(std::ref(writeToBuffer)));
}

 Player* Object::ToPlayer() { return To<Player>(); }
 const Player* Object::ToPlayer() const { return To<Player>(); }

 Creature* Object::ToCreature() { return To<Creature>(); }
 const Creature* Object::ToCreature() const { return To<Creature>(); }

 Unit* Object::ToUnit() { return To<Unit>(); }
 const Unit* Object::ToUnit() const { return To<Unit>(); }

 GameObject* Object::ToGameObject() { return To<GameObject>(); }
 const GameObject* Object::ToGameObject() const { return To<GameObject>(); }

 Corpse* Object::ToCorpse() { return To<Corpse>(); }
 const Corpse* Object::ToCorpse() const { return To<Corpse>(); }

 DynamicObject* Object::ToDynObject() { return To<DynamicObject>(); }
 DynamicObject const* Object::ToDynObject() const { return To<DynamicObject>(); }

 Object::Object() : m_PackGUID(sizeof(uint64)+1)
{
    m_objectTypeId      = TYPEID_OBJECT;
    m_objectType        = TYPEMASK_OBJECT;

    m_uint32Values      = NULL;
    m_valuesCount       = 0;
    _fieldNotifyFlags   = UF_FLAG_DYNAMIC;

    m_inWorld           = false;
    m_objectUpdated     = false;
}

Object::~Object()
{
    if (IsInWorld())
    {
        TC_LOG_FATAL("misc", "Object::~Object - guid=" UI64FMTD ", typeid=%hhu, entry=%u deleted but still in world!!", GetGUID().GetRawValue(), GetTypeId(), GetEntry());
        if (isType(TYPEMASK_ITEM))
            TC_LOG_FATAL("misc", "Item slot %u", ((Item*)this)->GetSlot());
        ABORT();
        RemoveFromWorld();
    }

    if (m_objectUpdated)
    {
        TC_LOG_FATAL("misc", "Object::~Object - guid=" UI64FMTD ", typeid=%hhu, entry=%u deleted but still in update list!!", GetGUID().GetRawValue(), GetTypeId(), GetEntry());
        ABORT();
    }

    delete [] m_uint32Values;
    m_uint32Values = nullptr;
}

void Object::_InitValues()
{
    m_uint32Values = new uint32[m_valuesCount];
    memset(m_uint32Values, 0, m_valuesCount*sizeof(uint32));

    _changesMask.resize(m_valuesCount);

    m_objectUpdated = false;
}

void Object::_Create(uint32 guidlow, uint32 entry, HighGuid guidhigh)
{
    if (!m_uint32Values) _InitValues();

    ObjectGuid guid(guidhigh, entry, guidlow);
    SetGuidValue(OBJECT_FIELD_GUID, guid);
    SetUInt32Value(OBJECT_FIELD_TYPE, m_objectType);
    m_PackGUID = guid;
}

std::string Object::_ConcatFields(uint16 startIndex, uint16 size) const
{
    std::ostringstream ss;
    for (uint16 index = 0; index < size; ++index)
        ss << GetUInt32Value(index + startIndex) << ' ';
    return ss.str();
}

void Object::AddToWorld()
{
    if (m_inWorld)
        return;

    ASSERT(m_uint32Values);

    m_inWorld = true;

    // synchronize values mirror with values array (changes will send in updatecreate opcode any way
    ClearUpdateMask(true);

    if (auto worldObject = dynamic_cast<WorldObject*>(this))
        worldObject->UpdateVisibilityGroup();
}

void Object::RemoveFromWorld()
{
    if (!m_inWorld)
        return;

    // if we remove from world then sending changes not required
    ClearUpdateMask(true);

    m_inWorld = false;
}

void Object::BuildUpdateBlockForPlayer(UpdateData& data, Player* target) const
{
    ObjectUpdateType updateType = ObjectUpdateType::CREATE_OBJECT;
    uint16 flags = m_updateFlag;

    /** lower flag1 **/
    if (target == this)                                      // building packet for yourself
        flags |= UPDATEFLAG_SELF;

    if (flags & UPDATEFLAG_STATIONARY_POSITION)
    {
        switch (GetGUID().GetHigh())
        {
            case HighGuid::Player:
            case HighGuid::Pet:
            case HighGuid::Corpse:
            case HighGuid::DynamicObject:
                updateType = ObjectUpdateType::CREATE_OBJECT2;
                break;
            case HighGuid::Unit:
            case HighGuid::Vehicle:
            {
                if (ToUnit()->ToTempSummon())
                    updateType = ObjectUpdateType::CREATE_OBJECT2;
                break;
            }
            default:
                break;
        }

        // UPDATETYPE_CREATE_OBJECT2 for some gameobject types...
        if (isType(TYPEMASK_GAMEOBJECT))
        {
            switch (ToGameObject()->GetGoType())
            {
                case GAMEOBJECT_TYPE_TRAP:
                case GAMEOBJECT_TYPE_DUEL_ARBITER:
                case GAMEOBJECT_TYPE_FLAGSTAND:
                case GAMEOBJECT_TYPE_FLAGDROP:
                    updateType = ObjectUpdateType::CREATE_OBJECT2;
                    break;
                default:
                    break;
            }
        }

        if (isType(TYPEMASK_UNIT))
        {
            if (ToUnit()->GetVictim())
                flags |= UPDATEFLAG_HAS_TARGET;
        }
    }

    //TC_LOG_DEBUG("BuildCreateUpdate: update-type: %u, object-type: %u got flags: %X, flags2: %X", updateType, m_objectTypeId, flags, flags2);

    ByteBuffer& buffer = data.RequestBuffer();
    buffer << uint8(updateType);
    buffer << GetPackGUID();
    buffer << uint8(m_objectTypeId);

    BuildMovementUpdate(buffer, flags);
    BuildValuesUpdate(updateType, buffer, target);
}

WorldPacket Object::CreateUpdateBlockForPlayer(Player* target) const
{
    UpdateData data;
    BuildUpdateBlockForPlayer(data, target);

    return data.BuildPacket();
}

void Object::SendUpdateToPlayer(Player* player)
{
    // send create update to player
    WorldPacket packet = CreateUpdateBlockForPlayer(player);
    player->SendDirectMessage(std::move(packet));
}

WorldPacket Object::CreateValuesUpdateBlockForPlayer(Player* target) const
{
    UpdateData data(500);

    BuildValuesUpdateBlockForPlayer(data, target);

    return data.BuildPacket();
}

void Object::BuildValuesUpdateBlockForPlayer(UpdateData& data, Player* target) const
{
    ByteBuffer& buffer = data.RequestBuffer();
    buffer << ObjectUpdateType::VALUES;
    buffer << GetPackGUID();

    BuildValuesUpdate(ObjectUpdateType::VALUES, buffer, target);
}

void Object::DestroyForPlayer(Player* target, bool onDeath) const
{
    ASSERT(target);

    if (isType(TYPEMASK_UNIT) || isType(TYPEMASK_PLAYER))
    {
        if (Battleground* bg = target->GetBattleground())
        {
            if (bg->isArena())
            {
                WorldPacket data(SMSG_ARENA_UNIT_DESTROYED, 8);
                data << uint64(GetGUID());
                target->SendDirectMessage(std::move(data));
            }
        }
    }

    WorldPacket data(SMSG_DESTROY_OBJECT, 8 + 1);
    data << uint64(GetGUID());
    //! If the following bool is true, the client will call "void CGUnit_C::OnDeath()" for this object.
    //! OnDeath() does for eg trigger death animation and interrupts certain spells/missiles/auras/sounds...
    data << uint8(onDeath ? 1 : 0);
    target->SendDirectMessage(std::move(data));
}

int32 Object::GetInt32Value(uint16 index) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return m_int32Values[index];
}

uint32 Object::GetUInt32Value(uint16 index) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return m_uint32Values[index];
}

uint64 Object::GetUInt64Value(uint16 index) const
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, false));
    return *((uint64*)&(m_uint32Values[index]));
}

float Object::GetFloatValue(uint16 index) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return m_floatValues[index];
}

uint8 Object::GetByteValue(uint16 index, uint8 offset) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    ASSERT(offset < 4);
    return *(((uint8*)&m_uint32Values[index])+offset);
}

uint16 Object::GetUInt16Value(uint16 index, uint8 offset) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    ASSERT(offset < 2);
    return *(((uint16*)&m_uint32Values[index])+offset);
}

ObjectGuid Object::GetGuidValue(uint16 index) const
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, false));
    return *((ObjectGuid*)&(m_uint32Values[index]));
}

void Object::BuildMovementUpdate(ByteBuffer& data, uint16 flags) const
{
    Unit const* unit = NULL;
    WorldObject const* object = NULL;

    if (isType(TYPEMASK_UNIT))
        unit = ToUnit();
    else
        object = ((WorldObject*)this);

    data << uint16(flags);                                  // update flags

    // 0x20
    if (flags & UPDATEFLAG_LIVING)
    {
        ASSERT(unit);
        unit->BuildMovementPacket(data);

        data << unit->GetSpeed(MOVE_WALK)
              << unit->GetSpeed(MOVE_RUN)
              << unit->GetSpeed(MOVE_RUN_BACK)
              << unit->GetSpeed(MOVE_SWIM)
              << unit->GetSpeed(MOVE_SWIM_BACK)
              << unit->GetSpeed(MOVE_FLIGHT)
              << unit->GetSpeed(MOVE_FLIGHT_BACK)
              << unit->GetSpeed(MOVE_TURN_RATE)
              << unit->GetSpeed(MOVE_PITCH_RATE);

        // 0x08000000
        if (unit->m_movementInfo.GetMovementFlags() & MOVEMENTFLAG_SPLINE_ENABLED)
            Movement::PacketBuilder::WriteCreate(*unit->movespline, data);
    }
    else
    {
        if (flags & UPDATEFLAG_POSITION)
        {
            ASSERT(object);

            Transport* transport = object->GetTransport();

            if (transport)
                data << transport->GetPackGUID();
            else
                data << uint8(0);

            data << object->GetPositionX();
            data << object->GetPositionY();
            if (isType(TYPEMASK_UNIT))
                data << unit->GetPositionZMinusOffset();
            else
                data << object->GetPositionZ();

            if (transport)
            {
                data << object->GetTransOffsetX();
                data << object->GetTransOffsetY();
                data << object->GetTransOffsetZ();
            }
            else
            {
                data << object->GetPositionX();
                data << object->GetPositionY();
                if (isType(TYPEMASK_UNIT))
                    data << unit->GetPositionZMinusOffset();
                else
                    data << object->GetPositionZ();
            }

            data << object->GetOrientation();

            if (GetTypeId() == TYPEID_CORPSE)
                data << float(object->GetOrientation());
            else
                data << float(0);
        }
        else
        {
            // 0x40
            if (flags & UPDATEFLAG_STATIONARY_POSITION)
            {
                ASSERT(object);
                data << object->GetStationaryX();
                data << object->GetStationaryY();
                data << object->GetStationaryZ();
                data << object->GetStationaryO();
            }
        }
    }

    // 0x8
    if (flags & UPDATEFLAG_UNKNOWN)
    {
        data << uint32(0);
    }

    // 0x10
    if (flags & UPDATEFLAG_LOWGUID)
    {
        switch (GetTypeId())
        {
            case TYPEID_OBJECT:
            case TYPEID_ITEM:
            case TYPEID_CONTAINER:
            case TYPEID_GAMEOBJECT:
            case TYPEID_DYNAMICOBJECT:
            case TYPEID_CORPSE:
                data << uint32(GetGUID().GetCounter());              // GetGUID().GetCounter()
                break;
            //! Unit, Player and default here are sending wrong values.
            /// @todo Research the proper formula
            case TYPEID_UNIT:
                data << uint32(0x0000000B);                // unk
                break;
            case TYPEID_PLAYER:
                if (flags & UPDATEFLAG_SELF)
                    data << uint32(0x0000002F);            // unk
                else
                    data << uint32(0x00000008);            // unk
                break;
            default:
                data << uint32(0x00000000);                // unk
                break;
        }
    }

    // 0x4
    if (flags & UPDATEFLAG_HAS_TARGET)
    {
        ASSERT(unit);
        if (Unit* victim = unit->GetVictim())
            data << victim->GetPackGUID();
        else
            data << uint8(0);
    }

    // 0x2
    if (flags & UPDATEFLAG_TRANSPORT)
    {
        GameObject const* go = ToGameObject();
        /** @TODO Use IsTransport() to also handle type 11 (TRANSPORT)
            Currently grid objects are not updated if there are no nearby players,
            this causes clients to receive different PathProgress
            resulting in players seeing the object in a different position
        */
        if (go && go->ToTransport())
            data << uint32(go->GetGOValue()->Transport.PathProgress);
        else
            data << uint32(game::time::GetGameTimeMS());
    }

    // 0x80
    if (flags & UPDATEFLAG_VEHICLE)
    {
        /// @todo Allow players to aquire this updateflag.
        ASSERT(unit);
        ASSERT(unit->GetVehicleKit());
        ASSERT(unit->GetVehicleKit()->GetVehicleInfo());
        data << uint32(unit->GetVehicleKit()->GetVehicleInfo()->m_ID);
        if (unit->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT))
            data << float(unit->GetTransOffsetO());
        else
            data << float(unit->GetOrientation());
    }

    // 0x200
    if (flags & UPDATEFLAG_ROTATION)
        data << ToGameObject()->GetPackedWorldRotation();
}

void Object::BuildValuesUpdate(ObjectUpdateType updateType, ByteBuffer& data, Player* target) const
{
    UpdateMask updateMask(m_valuesCount);

    const uint32 maskPos = data.wpos();
    data << updateMask; // write dummy mask, so we have enough space later

    uint32* flags = nullptr;
    const uint32 visibleFlag = GetUpdateFieldData(target, flags);

    for (const uint16 index : iterateTo(m_valuesCount))
    {
        if (_fieldNotifyFlags & flags[index] ||
            ((updateType == ObjectUpdateType::VALUES ? _changesMask.test(index) : m_uint32Values[index]) && (flags[index] & visibleFlag)))
        {
            updateMask.set(index);
            data << m_uint32Values[index];
        }
    }

    data.replace(maskPos, updateMask);
}

void Object::ClearUpdateMask(bool remove)
{
    _changesMask.reset();

    if (m_objectUpdated)
    {
        if (remove)
            AbortObjectUpdate();
        m_objectUpdated = false;
    }
}

uint32 Object::GetUpdateFieldData(Player const* target, uint32*& flags) const
{
    uint32 visibleFlag = UF_FLAG_PUBLIC;

    if (target == this)
        visibleFlag |= UF_FLAG_PRIVATE;

    switch (GetTypeId())
    {
        case TYPEID_ITEM:
        case TYPEID_CONTAINER:
            flags = ItemUpdateFieldFlags;
            if (target && ((Item*)this)->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER | UF_FLAG_ITEM_OWNER;
            break;
        case TYPEID_UNIT:
        case TYPEID_PLAYER:
        {
            Player* plr = ToUnit()->GetCharmerOrOwnerPlayerOrPlayerItself();
            flags = UnitUpdateFieldFlags;

            if (!target)
                break;

            if (ToUnit()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;

            if (HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO))
                if (ToUnit()->HasAuraTypeWithCaster(SPELL_AURA_EMPATHY, target->GetGUID()))
                    visibleFlag |= UF_FLAG_SPECIAL_INFO;

            if (plr && plr->IsInSameRaidWith(target))
                visibleFlag |= UF_FLAG_PARTY_MEMBER;
            break;
        }
        case TYPEID_GAMEOBJECT:
            flags = GameObjectUpdateFieldFlags;
            if (target && ToGameObject()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_DYNAMICOBJECT:
            flags = DynamicObjectUpdateFieldFlags;
            if (target && ((DynamicObject*)this)->GetCasterGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_CORPSE:
            flags = CorpseUpdateFieldFlags;
            if (target && ToCorpse()->GetOwnerGUID() == target->GetGUID())
                visibleFlag |= UF_FLAG_OWNER;
            break;
        case TYPEID_OBJECT:
            break;
        default:
            break;
    }

    return visibleFlag;
}

void Object::_LoadIntoDataField(std::string const& data, uint32 startOffset, uint32 count)
{
    if (data.empty())
        return;

    Tokenizer tokens(data, ' ', count);

    if (tokens.size() != count)
        return;

    for (uint32 index = 0; index < count; ++index)
    {
        m_uint32Values[startOffset + index] = atoul(tokens[index]);
        _changesMask.set(startOffset + index);
    }
}

void Object::SetInt32Value(uint16 index, int32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_int32Values[index] != value)
    {
        m_int32Values[index] = value;
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::SetUInt32Value(uint16 index, uint32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_uint32Values[index] != value)
    {
        m_uint32Values[index] = value;
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::UpdateUInt32Value(uint16 index, uint32 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    m_uint32Values[index] = value;
    _changesMask.set(index);
}

void Object::SetUInt64Value(uint16 index, uint64 value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (*((uint64*)&(m_uint32Values[index])) != value)
    {
        m_uint32Values[index] = PAIR64_LOPART(value);
        m_uint32Values[index + 1] = PAIR64_HIPART(value);
        _changesMask.set(index);
        _changesMask.set(index + 1);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

bool Object::AddGuidValue(uint16 index, ObjectGuid value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (value && !*((ObjectGuid*)&(m_uint32Values[index])))
    {
        *((ObjectGuid*)&(m_uint32Values[index])) = value;
        _changesMask.set(index);
        _changesMask.set(index + 1);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }

        return true;
    }

    return false;
}

bool Object::RemoveGuidValue(uint16 index, ObjectGuid value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (value && *((ObjectGuid*)&(m_uint32Values[index])) == value)
    {
        m_uint32Values[index] = 0;
        m_uint32Values[index + 1] = 0;
        _changesMask.set(index);
        _changesMask.set(index + 1);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }

        return true;
    }

    return false;
}

void Object::SetFloatValue(uint16 index, float value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (m_floatValues[index] != value)
    {
        m_floatValues[index] = value;
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::SetByteValue(uint16 index, uint8 offset, uint8 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 3)
    {
        TC_LOG_ERROR("misc", "Object::SetByteValue: wrong offset %u", offset);
        return;
    }

    if (uint8(m_uint32Values[index] >> (offset * 8)) != value)
    {
        m_uint32Values[index] &= ~uint32(uint32(0xFF) << (offset * 8));
        m_uint32Values[index] |= uint32(uint32(value) << (offset * 8));
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::SetUInt16Value(uint16 index, uint8 offset, uint16 value)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 1)
    {
        TC_LOG_ERROR("misc", "Object::SetUInt16Value: wrong offset %u", offset);
        return;
    }

    if (uint16(m_uint32Values[index] >> (offset * 16)) != value)
    {
        m_uint32Values[index] &= ~uint32(uint32(0xFFFF) << (offset * 16));
        m_uint32Values[index] |= uint32(uint32(value) << (offset * 16));
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::SetGuidValue(uint16 index, ObjectGuid value)
{
    ASSERT(index + 1 < m_valuesCount || PrintIndexError(index, true));
    if (*((ObjectGuid*)&(m_uint32Values[index])) != value)
    {
        *((ObjectGuid*)&(m_uint32Values[index])) = value;
        _changesMask.set(index);
        _changesMask.set(index + 1);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::SetStatFloatValue(uint16 index, float value)
{
    if (value < 0)
        value = 0.0f;

    SetFloatValue(index, value);
}

void Object::SetStatInt32Value(uint16 index, int32 value)
{
    if (value < 0)
        value = 0;

    SetUInt32Value(index, uint32(value));
}

void Object::ApplyModUInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetUInt32Value(index);
    cur += (apply ? val : -val);
    if (cur < 0)
        cur = 0;
    SetUInt32Value(index, cur);
}

void Object::ApplyModInt32Value(uint16 index, int32 val, bool apply)
{
    int32 cur = GetInt32Value(index);
    cur += (apply ? val : -val);
    SetInt32Value(index, cur);
}

void Object::ApplyModSignedFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    SetFloatValue(index, cur);
}

void Object::ApplyPercentModFloatValue(uint16 index, float val, bool apply)
{
    float value = GetFloatValue(index);
    ApplyPercentModFloatVar(value, val, apply);
    SetFloatValue(index, value);
}

void Object::ApplyModPositiveFloatValue(uint16 index, float  val, bool apply)
{
    float cur = GetFloatValue(index);
    cur += (apply ? val : -val);
    if (cur < 0)
        cur = 0;
    SetFloatValue(index, cur);
}

void Object::SetFlag(uint16 index, uint32 newFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    uint32 oldval = m_uint32Values[index];
    uint32 newval = oldval | newFlag;

    if (oldval != newval)
    {
        m_uint32Values[index] = newval;
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::RemoveFlag(uint16 index, uint32 oldFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));
    ASSERT(m_uint32Values);

    uint32 oldval = m_uint32Values[index];
    uint32 newval = oldval & ~oldFlag;

    if (oldval != newval)
    {
        m_uint32Values[index] = newval;
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::ToggleFlag(uint16 index, uint32 flag)
{
    if (HasFlag(index, flag))
        RemoveFlag(index, flag);
    else
        SetFlag(index, flag);
}

bool Object::HasFlag(uint16 index, uint32 flag) const
{
    if (index >= m_valuesCount && !PrintIndexError(index, false))
        return false;

    return (m_uint32Values[index] & flag) != 0;
}

void Object::ApplyModFlag(uint16 index, uint32 flag, bool apply)
{
    if (apply) SetFlag(index, flag); else RemoveFlag(index, flag);
}

void Object::SetByteFlag(uint16 index, uint8 offset, uint8 newFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 3)
    {
        TC_LOG_ERROR("misc", "Object::SetByteFlag: wrong offset %u", offset);
        return;
    }

    if (!(uint8(m_uint32Values[index] >> (offset * 8)) & newFlag))
    {
        m_uint32Values[index] |= uint32(uint32(newFlag) << (offset * 8));
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::RemoveByteFlag(uint16 index, uint8 offset, uint8 oldFlag)
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, true));

    if (offset > 3)
    {
        TC_LOG_ERROR("misc", "Object::RemoveByteFlag: wrong offset %u", offset);
        return;
    }

    if (uint8(m_uint32Values[index] >> (offset * 8)) & oldFlag)
    {
        m_uint32Values[index] &= ~uint32(uint32(oldFlag) << (offset * 8));
        _changesMask.set(index);

        if (m_inWorld && !m_objectUpdated)
        {
            ScheduleObjectUpdate();
            m_objectUpdated = true;
        }
    }
}

void Object::ToggleByteFlag(uint16 index, uint8 offset, uint8 flag)
{
    if (HasByteFlag(index, offset, flag))
        RemoveByteFlag(index, offset, flag);
    else
        SetByteFlag(index, offset, flag);
}

bool Object::HasByteFlag(uint16 index, uint8 offset, uint8 flag) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    ASSERT(offset < 4);
    return (((uint8*)&m_uint32Values[index])[offset] & flag) != 0;
}

void Object::SetFlag64(uint16 index, uint64 newFlag)
{
    uint64 oldval = GetUInt64Value(index);
    uint64 newval = oldval | newFlag;
    SetUInt64Value(index, newval);
}

void Object::RemoveFlag64(uint16 index, uint64 oldFlag)
{
    uint64 oldval = GetUInt64Value(index);
    uint64 newval = oldval & ~oldFlag;
    SetUInt64Value(index, newval);
}

void Object::ToggleFlag64(uint16 index, uint64 flag)
{
    if (HasFlag64(index, flag))
        RemoveFlag64(index, flag);
    else
        SetFlag64(index, flag);
}

bool Object::HasFlag64(uint16 index, uint64 flag) const
{
    ASSERT(index < m_valuesCount || PrintIndexError(index, false));
    return (GetUInt64Value(index) & flag) != 0;
}

void Object::ApplyModFlag64(uint16 index, uint64 flag, bool apply)
{
    if (apply) SetFlag64(index, flag); else RemoveFlag64(index, flag);
}

bool Object::PrintIndexError(uint32 index, bool set) const
{
    TC_LOG_ERROR("misc", "Attempt %s non-existed value field: %u (count: %u) for object typeid: %hhu type mask: %u", (set ? "set value to" : "get value from"), index, m_valuesCount, GetTypeId(), m_objectType);

    // ASSERT must fail after function call
    return false;
}

void Object::ForceValuesUpdateAtIndex(uint32 i)
{
    _changesMask.set(i);
    if (m_inWorld && !m_objectUpdated)
    {
        ScheduleObjectUpdate();
        m_objectUpdated = true;
    }
}
