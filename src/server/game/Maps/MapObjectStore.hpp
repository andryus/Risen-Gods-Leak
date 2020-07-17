#pragma once

#include <functional>

#include <boost/optional.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/map.hpp>

#include "util/multi_type/UnorderedMap.hpp"
#include "util/Typelist.hpp"
#include "util/adaptor/Adaptors.hpp"

#include "ObjectGuid.h"

class Creature;
class GameObject;
class DynamicObject;
class Corpse;

class MapObjectStore
{
public:
    template<typename T>
    boost::optional<T&> Find(ObjectGuid const& guid)
    {
        if (auto opt = _container.Find<Wrapper<T>>(guid))
            return opt->get();
        else
            return boost::none;
    }

    template<typename T>
    auto GetAll()
    {
        return _container.GetValues<Wrapper<T>>() | ::util::adaptor::UnwrapReference;
    }

    boost::optional<Creature&> GetCreatureBySpawnId(uint32 const spawnId);
    boost::optional<GameObject&> GetGameObjectBySpawnId(uint32 const spawnId);

    auto GetCreaturesByEntry(uint32 const entry)
    {
        return boost::make_iterator_range(_creaturesByEntry.equal_range(entry))
            | boost::adaptors::map_values
            | ::util::adaptor::UnwrapReference;
    }

    auto GetGameObjectsByEntry(uint32 const entry)
    {
        return boost::make_iterator_range(_gameObjectsByEntry.equal_range(entry))
            | boost::adaptors::map_values
            | ::util::adaptor::UnwrapReference;
    }

    bool Insert(Creature& obj);
    bool Insert(GameObject& obj);
    bool Insert(DynamicObject& obj);
    bool Insert(Corpse& obj);

    bool Remove(Creature const& obj);
    bool Remove(GameObject const& obj);
    bool Remove(DynamicObject const& obj);
    bool Remove(Corpse const& obj);

private:
    template<typename T>
    bool InsertImpl(T& object);

    template<typename T>
    bool RemoveImpl(T const& object);

    // elements are wrapped within reference_wrapper for the moment. maybe change this to unique_ptr or sth in the future.
    template<typename T>
    using Wrapper = std::reference_wrapper<T>;

    using StoredTypes = ::util::WrapTypes<Wrapper, ::util::TypeList<Creature, GameObject, DynamicObject, Corpse>>;
    using ContainerType = ::util::multi_type::UnorderedMap<ObjectGuid, StoredTypes>;
    ContainerType _container{};

    std::unordered_map<uint32, std::reference_wrapper<Creature>> _creaturesBySpawnId{};
    std::unordered_map<uint32, std::reference_wrapper<GameObject>> _gameObjectsBySpawnId{};

    std::unordered_multimap<uint32, std::reference_wrapper<Creature>> _creaturesByEntry{};
    std::unordered_multimap<uint32, std::reference_wrapper<GameObject>> _gameObjectsByEntry{};
};
