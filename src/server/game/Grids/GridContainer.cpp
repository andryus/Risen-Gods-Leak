#include "GridContainer.hpp"
#include "Log.h"
#include "Map.h"

namespace Grids
{
    void GridMap::LoadGrid(const CellCoord& coord)
    {
        TC_LOG_DEBUG("maps", "Creating grid[%u, %u] for map %u instance %u", coord.GetGridX(), coord.GetGridY(), map.GetId(), map.GetInstanceId());

        ASSERT(grids[coord.GetGridX()][coord.GetGridY()] == nullptr);

        grids[coord.GetGridX()][coord.GetGridY()] = std::make_unique<Grid>();
        grids[coord.GetGridX()][coord.GetGridY()]->Initialize(coord, mapId, spawnMode, map);
    }

    CellCoord GridMap::FindNextLoadedGrid(const CellCoord& start) const
    {
        // Find next grid in current grid row
        uint16 steps;
        if (start.GetCellY() + 1 < MAX_NUMBER_OF_CELLS)
            steps = MAX_NUMBER_OF_GRIDS;
        else
            steps = MAX_NUMBER_OF_GRIDS - start.GetGridY();
        for (uint16 x = 1; x < steps; ++x)
            if (IsGridLoaded(CellCoord::FromGridCoord((start.GetGridX() + x) % MAX_NUMBER_OF_GRIDS, start.GetGridY())))
                return CellCoord::FromGridCoord((start.GetGridX() + x) % MAX_NUMBER_OF_GRIDS, start.GetGridY());
        // Find loaded grid in next rows
        for (uint16 y = start.GetGridY() + 1; y < MAX_NUMBER_OF_GRIDS; ++y)
            for (uint16 x = 0; x < MAX_NUMBER_OF_GRIDS; ++x)
                if (IsGridLoaded(CellCoord::FromGridCoord(x, y)))
                    return CellCoord::FromGridCoord(x, y);
        // Not grid found, return invalid coords
        return CellCoord::FromGridCoord(MAX_NUMBER_OF_GRIDS + 1, MAX_NUMBER_OF_GRIDS + 1);
    }

    bool GridMap::IsGridLoaded(CellCoord const& coord) const
    {
        ASSERT(coord.IsValid());
        return grids[coord.GetGridX()][coord.GetGridY()] != nullptr;
    }

    void GridMap::EnsureGridLoaded(CellCoord const& coord)
    {
        if (!IsGridLoaded(coord))
            LoadGrid(coord);
        grids[coord.GetGridX()][coord.GetGridY()]->ResetLastActiveTime();
    }

    void GridMap::PreloadGrid(CellCoord const& coord)
    {
        if (!IsGridLoaded(coord))
        {
            LoadGrid(coord);
            if (grids[coord.GetGridX()][coord.GetGridY()]->IsEmpty())
                grids[coord.GetGridX()][coord.GetGridY()] = nullptr;
        }
    }

    void GridMap::Update(uint32 diff)
    {
        if (map.IsBattlegroundOrArena())
            return;

        for (uint32 x = 0; x < MAX_NUMBER_OF_GRIDS; ++x)
            for (uint32 y = 0; y < MAX_NUMBER_OF_GRIDS; ++y)
            {
                auto& grid = grids[x][y];
                if (grid)
                {
                    if (grid->CanUnload())
                    {
                        TC_LOG_DEBUG("maps", "Unload grid[%u, %u] for map %u instance %u", x, y, map.GetId(), map.GetInstanceId());
                        grid->CleanupBeforeDelete(map);
                        grid = nullptr;
                    }
                    else
                        grid->Update(diff);
                }
            }
    }

    GridMap::~GridMap()
    {
        UnloadAll();
    }

    void GridMap::UnloadAll()
    {
        for (uint32 x = 0; x < MAX_NUMBER_OF_GRIDS; ++x)
            for (uint32 y = 0; y < MAX_NUMBER_OF_GRIDS; ++y)
            {
                auto& grid = grids[x][y];
                if (grid)
                {
                    grid->CleanupBeforeDelete(map);
                    grid = nullptr;
                }
            }
    }

    void GridMap::ResetVisitedCells()
    {
        for (uint16 gridX = 0; gridX < MAX_NUMBER_OF_GRIDS; ++gridX)
            for (uint16 gridY = 0; gridY < MAX_NUMBER_OF_GRIDS; ++gridY)
            {
                if (grids[gridX][gridY])
                    grids[gridX][gridY]->ResetVisitedCells();
            }
    }
}
