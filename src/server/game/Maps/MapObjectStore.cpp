#include "MapObjectStore.hpp"

#include "Errors.h"

#include "Corpse.h"
#include "Creature.h"
#include "DynamicObject.h"
#include "GameObject.h"

template <typename T>
bool MapObjectStore::InsertImpl(T& object)
{
    auto result = _container.Insert<Wrapper<T>>(object.GetGUID(), Wrapper<T>{object});

    if (!result.second) // if there was already an object with this object's guid
    {
        TC_LOG_FATAL("map", "object %s is already in map", object.GetGUID().ToString().c_str());

        //ASSERT(&result.first.get() == &object, "There is already an object with the given key, but the objects differ.");
    }

    return result.second;
}

template <typename T>
bool MapObjectStore::RemoveImpl(T const& object)
{
    return _container.Remove<Wrapper<T>>(object.GetGUID());
}

boost::optional<Creature&> MapObjectStore::GetCreatureBySpawnId(uint32 const spawnId)
{
    auto const it = _creaturesBySpawnId.find(spawnId);
    if (it != _creaturesBySpawnId.end())
        return it->second.get();
    else
        return boost::none;
}

boost::optional<GameObject&> MapObjectStore::GetGameObjectBySpawnId(uint32 const spawnId)
{
    auto const it = _gameObjectsBySpawnId.find(spawnId);
    if (it != _gameObjectsBySpawnId.end())
        return it->second.get();
    else
        return boost::none;
}

bool MapObjectStore::Insert(Creature& obj)
{
    bool const successful = InsertImpl(obj);

    if (successful)
    {
        _creaturesByEntry.emplace(obj.GetEntry(), std::ref(obj));

        if (obj.GetSpawnId() != 0)
        {
            auto result = _creaturesBySpawnId.emplace(obj.GetSpawnId(), std::ref(obj));
            ASSERT(result.second || &result.first->second.get() == &obj, "Tried to insert a creature with a spawn id that is already present.");
        }
    }

    return successful;
}

bool MapObjectStore::Insert(GameObject& obj)
{
    bool const successful = InsertImpl(obj);

    if (successful)
    {
        _gameObjectsByEntry.emplace(obj.GetEntry(), std::ref(obj));

        if (obj.GetSpawnId() != 0)
        {
            auto result = _gameObjectsBySpawnId.emplace(obj.GetSpawnId(), std::ref(obj));
            ASSERT(result.second || &result.first->second.get() == &obj, "Tried to insert a gameobject with a spawn id that is already present.");
        }
    }

    return successful;
}

bool MapObjectStore::Insert(DynamicObject& obj)
{
    return InsertImpl(obj);
}

bool MapObjectStore::Insert(Corpse& obj)
{
    return InsertImpl(obj);
}

bool MapObjectStore::Remove(Creature const& obj)
{
    bool const successful = RemoveImpl(obj);

    if (successful)
    {
        auto const range = _creaturesByEntry.equal_range(obj.GetEntry());
        for (auto it = range.first; it != range.second; ++it)
            if (&it->second.get() == &obj)
            {
                _creaturesByEntry.erase(it);
                break;
            }

        if (obj.GetSpawnId() != 0)
            _creaturesBySpawnId.erase(obj.GetSpawnId());
    }

    return successful;
}

bool MapObjectStore::Remove(GameObject const& obj)
{
    bool const successful = RemoveImpl(obj);

    if (successful)
    {
        auto const range = _gameObjectsByEntry.equal_range(obj.GetEntry());
        for (auto it = range.first; it != range.second; ++it)
            if (&it->second.get() == &obj)
            {
                _gameObjectsByEntry.erase(it);
                break;
            }

        if (obj.GetSpawnId() != 0)
            _gameObjectsBySpawnId.erase(obj.GetSpawnId());
    }

    return successful;
}

bool MapObjectStore::Remove(DynamicObject const& obj)
{
    return RemoveImpl(obj);
}

bool MapObjectStore::Remove(Corpse const& obj)
{
    return RemoveImpl(obj);
}
