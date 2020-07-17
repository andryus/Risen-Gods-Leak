#include "Grid.hpp"
#include "World.h"
#include "Map.h"

namespace Grids
{
    Grid::Grid() : initialized(false), inactiveTimer(sWorld->getIntConfig(CONFIG_INTERVAL_GRIDCLEAN)) { }

    void Grid::Initialize(const CellCoord& position, uint16 mapid, uint8 spawnMode, Map& map)
    {
        ASSERT(!initialized);
        initialized = true;
        for (uint16 cell_x = 0; cell_x < MAX_NUMBER_OF_CELLS; cell_x++)
            for (uint16 cell_y = 0; cell_y < MAX_NUMBER_OF_CELLS; cell_y++)
            {
                const CellCoord cellPos = CellCoord::FromCoords(position.GetGridX(), position.GetGridY(), cell_x, cell_y);
                cells[cell_x][cell_y].Initialize(cellPos, mapid, spawnMode, map);
            }
    }

    void Grid::ResetLastActiveTime()
    {
        inactiveTimer.Reset(sWorld->getIntConfig(CONFIG_INTERVAL_GRIDCLEAN));
    }

    void Grid::Update(uint32 diff)
    {
        inactiveTimer.Update(diff);
    }

    bool Grid::CanUnload() const
    {
        if (!sWorld->getBoolConfig(CONFIG_GRID_UNLOAD))
            return false;

        return inactiveTimer.Passed();
    }

    bool Grid::IsEmpty() const
    {
        for (uint16 cell_x = 0; cell_x < MAX_NUMBER_OF_CELLS; cell_x++)
            for (uint16 cell_y = 0; cell_y < MAX_NUMBER_OF_CELLS; cell_y++)
                if (!cells[cell_x][cell_y].IsEmpty())
                    return false;
        return true;
    }

    void Grid::CleanupBeforeDelete(Map& map)
    {
        for (uint16 cell_x = 0; cell_x < MAX_NUMBER_OF_CELLS; cell_x++)
            for (uint16 cell_y = 0; cell_y < MAX_NUMBER_OF_CELLS; cell_y++)
            {
                cells[cell_x][cell_y].CleanupBeforeDelete(map);
                // we may have to remove objects (e.g. minions totems) from other cells as well
                map.RemoveAllObjectsInRemoveList();
            }
    }
}
