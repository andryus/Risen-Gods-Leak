#pragma once

#include "Cell.hpp"
#include "CellCoord.hpp"
#include "Timer.h"
#include "GridDefines.h"

#include <bitset>
#include <array>

namespace Grids
{
    class GAME_API Grid
    {
        bool initialized;

        TimeTrackerSmall inactiveTimer;
        std::array<std::array<Cell, MAX_NUMBER_OF_CELLS>, MAX_NUMBER_OF_CELLS> cells;
        std::bitset<MAX_NUMBER_OF_CELLS*MAX_NUMBER_OF_CELLS> visited_cells;
    public:
        Grid();

        void Initialize(const CellCoord& position, uint16 mapid, uint8 spawnMode, Map& map);

        template<class Visitor, class CellAreaType>
        bool Visit(const CellCoord& position, CellAreaType& area, Visitor& visitor);
        template<class Visitor, class CellAreaType>
        bool VisitOnce(const CellCoord& position, CellAreaType& area, Visitor& visitor);
        template<class Visitor>
        bool VisitAllCells(Visitor& visitor);

        template<class ObjectType>
        void Insert(ObjectType* object, const CellCoord& position);
        template<class ObjectType>
        void Erase(ObjectType* object);

        void ResetLastActiveTime();
        void Update(uint32 diff);
        bool CanUnload() const;
        bool IsEmpty() const;

        void ResetVisitedCells() { visited_cells.reset(); }
        void CleanupBeforeDelete(Map& map);
    };

    template<class Visitor, class CellAreaType>
    bool Grid::Visit(const CellCoord& position, CellAreaType& area, Visitor& visitor)
    {
        while (area.HasCoord())
        {
            const auto coord = area.Get();
            if (position.DiffGrid(coord))
                break;

            if (!cells[coord.GetCellX()][coord.GetCellY()].Visit(visitor))
                return false;

            area.Next();
        }

        return true;
    }

    template<class Visitor, class CellAreaType>
    bool Grid::VisitOnce(const CellCoord& position, CellAreaType& area, Visitor& visitor)
    {
        while (area.HasCoord())
        {
            const auto coord = area.Get();
            
            if (position.DiffGrid(coord))
                break;

            if (!visited_cells.test(coord.GetCellId()))
                visited_cells.set(coord.GetCellId());
            else
            {
                area.Next();
                continue;
            }

            const auto continueSearch = cells[coord.GetCellX()][coord.GetCellY()].Visit(visitor);
            assert(continueSearch); // VisitOnce must not use a visitor with a break condition

            area.Next();
        }

        return true;
    }

    template <class Visitor>
    bool Grid::VisitAllCells(Visitor& visitor)
    {
        for (uint16 cellX = 0; cellX < MAX_NUMBER_OF_CELLS; ++cellX)
            for (uint16 cellY = 0; cellY < MAX_NUMBER_OF_CELLS; ++cellY)
                if(!cells[cellX][cellY].Visit(visitor))
                    return false;

        return true;
    }

    template <class ObjectType>
    void Grid::Insert(ObjectType* object, const CellCoord& position)
    {
        cells[position.GetCellX()][position.GetCellY()].Insert(object, position);
    }

    template<class ObjectType>
    void Grid::Erase(ObjectType* object)
    {
        cells[object->GetCurrentCell().GetCellX()][object->GetCurrentCell().GetCellY()].Erase(object);
    }
}
