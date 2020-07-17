#include "Cell.hpp"
#include "CellCoord.hpp"
#include "Errors.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "Transport.h"
#include "Pet.h"

namespace Grids
{
    void Cell::Initialize(const CellCoord& position, uint16 mapid, uint8 spawnMode, Map& map)
    {
        CellObjectGuids const& cell_guids = sObjectMgr->GetCellObjectGuids(mapid, spawnMode, position.GetId());

        for (uint32 spawnId : cell_guids.creatures)
        {
            Creature::LoadFromDB(spawnId, &map, true);
        }

        for (uint32 spawnId : cell_guids.gameobjects)
        {
            GameObject::LoadFromDB(spawnId, map, true);
        }

        for (const auto itr : cell_guids.corpses)
        {
            if (itr.second != map.GetInstanceId())
                continue;
            
            Corpse* obj = sObjectAccessor->GetCorpseForPlayerGUID(ObjectGuid(HighGuid::Player, itr.first));
            if (!obj)
                continue;

            if (obj->FindMap() != nullptr)
            {
                TC_LOG_ERROR("corpse", "Corpse can't be added to map. Corpse is already in map. playerGuid=%u, mapId=%u, instanceId=%u", itr.first, map.GetId(), map.GetInstanceId());
                continue;
            }

            obj->SetMap(&map);
            map.AddToMap(obj);
        }
    }


    template <> void Cell::CellContainer<Creature>::CleanupBeforeDelete(Map& map) 
    {
        for (Creature* creature : storage)
        {
            if (TempSummon* summon = creature->ToTempSummon())
                summon->UnSummon();
        }

        map.RemoveAllObjectsInRemoveList();

        while (!storage.empty())
        {
            Creature* creature = *storage.begin();
            creature->GetMap()->OnCellCleanup(creature);
        }

        map.RemoveAllObjectsInRemoveList();
    }

    template <> void Cell::CellContainer<GameObject>::CleanupBeforeDelete(Map& map)
    {
        while (!storage.empty())
        {
            GameObject* gameObject = *storage.begin();
            gameObject->GetMap()->OnCellCleanup(gameObject);
        }

        map.RemoveAllObjectsInRemoveList();
    }

    template <> void Cell::CellContainer<Corpse>::CleanupBeforeDelete(Map& map)
    {
        while (!storage.empty())
        {
            Corpse* corpse = *storage.begin();
            corpse->GetMap()->OnCellCleanup(corpse);
        }
    }

    template <> void Cell::CellContainer<DynamicObject>::CleanupBeforeDelete(Map& map)
    {
        while (!storage.empty())
        {
            DynamicObject* dynObject = *storage.begin();
            dynObject->GetMap()->OnCellCleanup(dynObject);
        }
    }


    void Cell::CleanupBeforeDelete(Map& map)
    {
        ASSERT(players.IsEmpty());

        creatures.CleanupBeforeDelete(map);
        gameObjects.CleanupBeforeDelete(map);
        corpses.CleanupBeforeDelete(map);
        dynObjects.CleanupBeforeDelete(map);

        ASSERT(IsEmpty());
    }

    bool Cell::IsEmpty() const
    {
        return players.IsEmpty() && creatures.IsEmpty() && gameObjects.IsEmpty() && corpses.IsEmpty() && dynObjects.IsEmpty();
    }
    
    Cell::~Cell()
    {
        ASSERT(IsEmpty());
    }

    template <class ObjectType>
    void Cell::CellContainer<ObjectType>::Insert(ObjectType* object, const CellCoord& position)
    {
        objectIndex[object] = storage.size();
        storage.push_back(object);
        object->SetCurrentCell(position);
    }

    template <class ObjectType>
    void Cell::CellContainer<ObjectType>::Erase(ObjectType* object)
    {
        const auto itr = objectIndex.find(object);
        ASSERT(itr != objectIndex.end());
        if (itr->second != (storage.size() - 1))
        {
            storage[itr->second] = storage.back();
            objectIndex[storage[itr->second]] = itr->second;
        }
        storage.pop_back();
        objectIndex.erase(itr);
    }

    template <class ObjectType>
    void Cell::InsertHelper(ObjectType* object, ContainerType<ObjectType>& container, const CellCoord& position)
    {
        container.Insert(object, position);
    }

    template<class ObjectType>
    void Cell::EraseHelper(ObjectType* object, ContainerType<ObjectType>& container)
    {
        container.Erase(object);
    }

    template GAME_API void Cell::InsertHelper<Player>(Player* object, ContainerType<Player>& container, const CellCoord& position);
    template GAME_API void Cell::InsertHelper<Creature>(Creature* object, ContainerType<Creature>& container, const CellCoord& position);
    template GAME_API void Cell::InsertHelper<GameObject>(GameObject* object, ContainerType<GameObject>& container, const CellCoord& position);
    template GAME_API void Cell::InsertHelper<Corpse>(Corpse* object, ContainerType<Corpse>& container, const CellCoord& position);
    template GAME_API void Cell::InsertHelper<DynamicObject>(DynamicObject* object, ContainerType<DynamicObject>& container, const CellCoord& position);

    template GAME_API void Cell::EraseHelper<Player>(Player* object, ContainerType<Player>& container);
    template GAME_API void Cell::EraseHelper<Creature>(Creature* object, ContainerType<Creature>& container);
    template GAME_API void Cell::EraseHelper<GameObject>(GameObject* object, ContainerType<GameObject>& container);
    template GAME_API void Cell::EraseHelper<Corpse>(Corpse* object, ContainerType<Corpse>& container);
    template GAME_API void Cell::EraseHelper<DynamicObject>(DynamicObject* object, ContainerType<DynamicObject>& container);

}
