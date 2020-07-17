#pragma once

#include "Grid.hpp"
#include "GridDefines.h"
#include "CellCoord.hpp"

#include <array>
#include <memory>

namespace Grids
{
    template<typename T>
    using GridArray = std::array<std::array<T, MAX_NUMBER_OF_GRIDS>, MAX_NUMBER_OF_GRIDS>;

    class GAME_API GridMap
    {
        public:
            GridArray<std::unique_ptr<Grid>> grids;

            uint16 mapId;
            uint8 spawnMode;

            Map& map;
            
            void LoadGrid(const CellCoord& coord);
            CellCoord FindNextLoadedGrid(const CellCoord& start) const;
        public:
            GridMap(uint16 mapId, uint8 spawnMode, Map& map) : mapId(mapId), spawnMode(spawnMode), map(map) {}
            ~GridMap();

            template<class Visitor, class CellAreaType>
            void Visit(CellAreaType& area, Visitor& visitor);
            template<class Visitor, class CellAreaType>
            void VisitOnce(CellAreaType& area, Visitor& visitor);
            template<class Visitor>
            void VisitAllCells(Visitor& visitor);
            bool IsGridLoaded(CellCoord const& coord) const;
            void EnsureGridLoaded(CellCoord const& coord);
            void PreloadGrid(CellCoord const& coord);

            void UnloadAll();

            void ResetVisitedCells();

            void Update(uint32 diff);

            template<class T>
            bool Insert(T* object, CellCoord const& coord, bool loadCell);

            template<class T>
            void Erase(T* object);
    };
    
    template<class T>
    bool GridMap::Insert(T* object, CellCoord const& coord, bool loadCell)
    {
        ASSERT(!object->IsInGrid());
        ASSERT(object->GetMap() == &map);
        ASSERT(coord.IsValid());

        if (!IsGridLoaded(coord))
        {
            if (loadCell)
                LoadGrid(coord);
            else
                return false;
        }
        object->AddToGrid(*this);
        grids[coord.GetGridX()][coord.GetGridY()]->Insert(object, coord);
        return true;
    }

    template <class T>
    void GridMap::Erase(T* object)
    {
        ASSERT(object->IsInGrid());
        ASSERT(object->GetCurrentCell().IsValid());

        grids[object->GetCurrentCell().GetGridX()][object->GetCurrentCell().GetGridY()]->Erase(object);
    }

    template<class Visitor, class CellAreaType>
    void GridMap::Visit(CellAreaType& area, Visitor& visitor)
    {
        while (area.HasCoord())
        {
            const auto coord = area.Get();

            if (!coord.IsValid())
            {
                area.Next();
                continue;
            }

            if (!IsGridLoaded(coord))
            {
                if (area.ShouldLoadGrid())
                    LoadGrid(coord);
                else
                {
                    area.Skip(FindNextLoadedGrid(coord));
                    continue;
                }
            }

            auto& grid = grids[coord.GetGridX()][coord.GetGridY()];
            if (area.ShouldLoadGrid())
                grid->ResetLastActiveTime();
            
            if(!grid->Visit(coord, area, visitor))
                break;
        }
    }

    template<class Visitor, class CellAreaType>
    void GridMap::VisitOnce(CellAreaType& area, Visitor& visitor)
    {
        while (area.HasCoord())
        {
            const auto coord = area.Get();

            if (!coord.IsValid())
            {
                area.Next();
                continue;
            }

            if (!IsGridLoaded(coord))
            {
                if (area.ShouldLoadGrid())
                    LoadGrid(coord);
                else
                {
                    area.Next();
                    continue;
                }
            }

            auto& grid = grids[coord.GetGridX()][coord.GetGridY()];
            if (area.ShouldLoadGrid())
                grid->ResetLastActiveTime();
            
             if(!grid->VisitOnce(coord, area, visitor))
                 break;
        }
    }

    template <class Visitor>
    void GridMap::VisitAllCells(Visitor& visitor)
    {
        for (uint16 gridX = 0; gridX < MAX_NUMBER_OF_GRIDS; ++gridX)
            for (uint16 gridY = 0; gridY < MAX_NUMBER_OF_GRIDS; ++gridY)
            {
                if (grids[gridX][gridY])
                    grids[gridX][gridY]->VisitAllCells(visitor);
            }
    }
}
