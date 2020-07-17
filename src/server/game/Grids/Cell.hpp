#pragma once

#include "Common.h"
#include "CellCoord.hpp"
#include "GridSearchers.h"

class WorldObject;
class Map;
class Player;
class Creature;
class GameObject;
class Corpse;
class DynamicObject;

namespace Grids
{
    class CellCoord;

    // Conditionally compile only objects that visitor accepts => mostly so we don't need to specify empty cases for Visit

    class Cell
    {
    public:
        template<class Visitor>
        bool Visit(Visitor& visitor) const;
        template<class Visitor, template<class> class ContainerType, class ObjectType>
        static bool ExecuteVisit(Visitor& visitor, const ContainerType<ObjectType>& container)
        {
            if constexpr(Trinity::VisitorAcceptsObjectType<Visitor, ObjectType>)
                return container.Visit(visitor);
            else
                return true;
        }
        template<class ObjectType>
        void Insert(ObjectType* object, const CellCoord& position);
        template<class ObjectType>
        void Erase(ObjectType* object);

        void Initialize(const CellCoord& position, uint16 mapid, uint8 spawnMode, Map& map);

        void CleanupBeforeDelete(Map& map);

        bool IsEmpty() const;

        ~Cell();
    private:

        template<class ObjectType>
        class CellContainer
        {
            using ContainerType = std::vector<ObjectType*>;

        public:
            void Insert(ObjectType* object, const CellCoord& position);
            void Erase(ObjectType* object);
            template<class Visitor>
            auto Visit(Visitor& visitor) const
            {
                if(Trinity::CanVisitObjectType<ObjectType>(visitor))
                {                
                    // we use index over iterator here, because we have to allow adding or removing elements while visiting this container
                    for (uint32_t index = 0; index < storage.size(); index++)
                    {
                        visitor.Visit(*storage[index]);

                        if (Trinity::VisitorTrait<Visitor>::IsDone(visitor))
                            return false;
                    }
                }

                return true;
            }

            void CleanupBeforeDelete(Map& map);
            bool IsEmpty() const { return storage.empty(); }
        private:
            std::unordered_map<ObjectType*, uint32> objectIndex;
            ContainerType storage;
        };

        template<class ObjectType> using ContainerType = CellContainer<ObjectType>;

        template<class ObjectType>
        void InsertHelper(ObjectType* object, ContainerType<ObjectType>& container, const CellCoord& position);

        template<class ObjectType>
        void EraseHelper(ObjectType* object, ContainerType<ObjectType>& container);

        std::unordered_map<WorldObject*, uint32> objectIndex;
        ContainerType<Creature> creatures;
        ContainerType<Player> players;
        ContainerType<GameObject> gameObjects;
        ContainerType<Corpse> corpses;
        ContainerType<DynamicObject> dynObjects;
    };

    template<class Visitor>
    bool Cell::Visit(Visitor& visitor) const
    {
        static_assert(!std::is_const_v<Visitor>, "Visitor cannot be const.");

        Trinity::checkVisitorValidity<Visitor>();

        if (!ExecuteVisit(visitor, creatures))
            return false;
        if (!ExecuteVisit(visitor, players))
            return false;
        if (!ExecuteVisit(visitor, gameObjects))
            return false;
        if (!ExecuteVisit(visitor, corpses))
            return false;

        return ExecuteVisit(visitor, dynObjects);
    }

    template<> inline void Cell::Insert<Player>(Player* object, const CellCoord& position) { InsertHelper(object, players, position); }
    template<> inline void Cell::Insert<Creature>(Creature* object, const CellCoord& position) { InsertHelper(object, creatures, position); }
    template<> inline void Cell::Insert<GameObject>(GameObject* object, const CellCoord& position) { InsertHelper(object, gameObjects, position); }
    template<> inline void Cell::Insert<Corpse>(Corpse* object, const CellCoord& position) { InsertHelper(object, corpses, position); }
    template<> inline void Cell::Insert<DynamicObject>(DynamicObject* object, const CellCoord& position) { InsertHelper(object, dynObjects, position); }

    template<> inline void Cell::Erase<Player>(Player* object) { EraseHelper(object, players); }
    template<> inline void Cell::Erase<Creature>(Creature* object) { EraseHelper(object, creatures); }
    template<> inline void Cell::Erase<GameObject>(GameObject* object) { EraseHelper(object, gameObjects); }
    template<> inline void Cell::Erase<Corpse>(Corpse* object) { EraseHelper(object, corpses); }
    template<> inline void Cell::Erase<DynamicObject>(DynamicObject* object) { EraseHelper(object, dynObjects); }
}
