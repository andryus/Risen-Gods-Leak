#include "WorldObjectOwner.h"
#include "WorldObject.h"
#include "ObjectAccessor.h"
#include "MapManager.h"

SCRIPT_OWNER_IMPL(WorldObject, script::Scriptable)
{
    return dynamic_cast<WorldObject*>(&base);
}


WorldObjectHandle::WorldObjectHandle(const WorldObject& object) :
    WorldObjectHandle(&object) { }

WorldObjectHandle::WorldObjectHandle(const WorldObject* object)
{
    if (object)
    {
        map = object->GetMap();
        guid = object->GetGUID().GetRawValue();
    }
    else
    {
        map = nullptr;
        guid = 0;
    }
}

WorldObject* WorldObjectHandle::Load() const
{
    if (guid == 0)
        return nullptr;

    ASSERT(map);

    return ObjectAccessor::GetWorldObject(*map, ObjectGuid(guid));
}

std::string WorldObjectHandle::Print() const
{
    return WorldObject::PrintGUID("<UNK>", ObjectGuid(guid));
}

WorldObject* WorldObjectHandle::FromString(std::string_view guid, script::Scriptable& ctx)
{
    if (WorldObject* object = script::castSourceTo<WorldObject, script::Scriptable>(ctx))
    {
        const uint64 guidValue = strtoull(guid.data(), nullptr, 10);

        return ObjectAccessor::GetWorldObject(*object, ObjectGuid(guidValue));
    }

    return nullptr;
}


SCRIPT_COMPONENT_IMPL(WorldObject, Map)
{
    return owner.GetMap();
}


SCRIPT_COMPONENT_IMPL(WorldObject, MapArea)
{
    if (Map* map = owner.GetMap())
        return map->GetAreaForObject(owner);

    return nullptr;
}

SCRIPT_COMPONENT_IMPL(WorldObject, Encounter)
{
    return owner.GetEncounter();
}
