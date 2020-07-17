#pragma once

#include "Common.h"
#include "GridDefines.h"

namespace Grids
{
    class GAME_API CellCoord
    {
        public:
            CellCoord() : grid_x(0), grid_y(0), cell_x(0), cell_y(0) {}
            static CellCoord FromPosition(float x, float y) { return {x, y}; }
            static CellCoord FromCoords(uint16 grid_x, uint16 grid_y, uint16 cell_x, uint16 cell_y) { return {grid_x, grid_y, cell_x, cell_y}; }
            static CellCoord FromGridCoord(uint16 x, uint16 y) { return {x, y, 0u, 0u}; }
            static CellCoord FromGlobalCoord(uint16 x, uint16 y) { return { std::make_pair(x, y) }; }

            bool DiffGrid(const CellCoord& other) const { return grid_x != other.grid_x || grid_y != other.grid_y; }
            bool DiffCell(const CellCoord& other) const { return cell_x != other.cell_x || cell_y != other.cell_y; }

            bool IsValid() const
            {
                return cell_x < MAX_NUMBER_OF_CELLS && cell_y < MAX_NUMBER_OF_CELLS && grid_x < MAX_NUMBER_OF_GRIDS && grid_y < MAX_NUMBER_OF_GRIDS;
            }

            static uint16 CalculateCellCoordFromPosition(float position)
            {
                return ComputeGlobalCellCoord(position, 0.f).first;
            }

            CellCoord Offset(int16 x, int16 y) const
            {
                const auto globalX = int16(GetGlobalX());
                const auto globalY = int16(GetGlobalY());
                int16 newX = globalX + x;
                int16 newY = globalY + y;
                return CellCoord(std::make_pair(newX, newY));
            }

            uint16 GetGridX() const { return grid_x; }
            uint16 GetGridY() const { return grid_y; }
            uint16 GetCellX() const { return cell_x; }
            uint16 GetCellY() const { return cell_y; }
            uint16 GetGlobalX() const { return grid_x * MAX_NUMBER_OF_CELLS + cell_x; }
            uint16 GetGlobalY() const { return grid_y * MAX_NUMBER_OF_CELLS + cell_y; }

            float GetLowX() const { return -MAP_HALFSIZE + GetGlobalX() * SIZE_OF_GRID_CELL; }
            float GetLowY() const { return -MAP_HALFSIZE + GetGlobalY() * SIZE_OF_GRID_CELL; }
            float GetHighX() const { return -MAP_HALFSIZE + (GetGlobalX() + 1) * SIZE_OF_GRID_CELL; }
            float GetHighY() const { return -MAP_HALFSIZE + (GetGlobalY() + 1) * SIZE_OF_GRID_CELL; }

            /// Calculate inverted x grid position for map and vmap lookups.
            void Inverse() { grid_x = GetInversedGridX(); grid_y = GetInversedGridY(); }
            uint16 GetInversedGridX() const { return (MAX_NUMBER_OF_GRIDS - 1) - GetGridX(); }
            uint16 GetInversedGridY() const { return (MAX_NUMBER_OF_GRIDS - 1) - GetGridY(); }
            
            uint32 GetId() const { return uint32(GetGlobalY()) * TOTAL_NUMBER_OF_CELLS_PER_MAP + uint32(GetGlobalX()); }
            uint32 GetCellId() const { return uint32(GetCellY()) * MAX_NUMBER_OF_CELLS + uint32(GetCellX()); }
            uint32 GetGridId() const { return uint32(GetGridY()) * MAX_NUMBER_OF_GRIDS + uint32(GetGridX()); }

            bool operator == (CellCoord const& other) const { return !DiffGrid(other) && !DiffCell(other); }
            bool operator != (CellCoord const& other) const { return DiffGrid(other) || DiffCell(other); }
        private:
            static std::pair<uint16, uint16> ComputeGlobalCellCoord(float x, float y)
            {
                const float x_offset = (x - CENTER_GRID_CELL_OFFSET) / SIZE_OF_GRID_CELL;
                const float y_offset = (y - CENTER_GRID_CELL_OFFSET) / SIZE_OF_GRID_CELL;

                uint16 x_val = uint16(CENTER_GRID_CELL_ID + x_offset + 0.5f);
                uint16 y_val = uint16(CENTER_GRID_CELL_ID + y_offset + 0.5f);
                return std::pair<uint16, uint16>(x_val, y_val);
            }

            CellCoord(uint16 grid_x, uint16 grid_y, uint16 cell_x, uint16 cell_y) :
                grid_x(grid_x), grid_y(grid_y), cell_x(cell_x), cell_y(cell_y) {}
            CellCoord(float x, float y) : CellCoord(ComputeGlobalCellCoord(x, y)) {}
            CellCoord(std::pair<uint16, uint16> globalPosition) :
                grid_x(globalPosition.first / MAX_NUMBER_OF_CELLS),
                grid_y(globalPosition.second / MAX_NUMBER_OF_CELLS),
                cell_x(globalPosition.first % MAX_NUMBER_OF_CELLS),
                cell_y(globalPosition.second % MAX_NUMBER_OF_CELLS) {}

            uint16 grid_x;
            uint16 grid_y;
            uint16 cell_x;
            uint16 cell_y;
    };

    class CellArea
    {
        protected:
            bool _loadGrid;
            CellArea() : _loadGrid(true) {}
        public:
            bool ShouldLoadGrid() const { return _loadGrid; }
            void SetNoGridLoad() { _loadGrid = false; }
            virtual ~CellArea() = default;
            // virtual bool HasCoord() = 0;
            // virtual void Next() = 0;
            // virtual CellCoord Get() = 0;
            // virtual void Skip(CellCoord const& nextLoadedGrid) = 0;
    };

    class SquareCellArea : public CellArea
    {
            uint16 lowX;
            CellCoord high;
            CellCoord current;
        public:
            SquareCellArea(float centerX, float centerY, float range)
            {
                current = CellCoord::FromPosition(centerX - range, centerY - range);
                high = CellCoord::FromPosition(centerX + range, centerY + range);
                lowX = current.GetGlobalX();
            }

            CellCoord Get() const { return current; }
            bool HasCoord() const
            {
                return current.GetGlobalY() <= high.GetGlobalY();
            }

            void Next()
            {
                if (current.GetGlobalX() < high.GetGlobalX())
                    current = current.Offset(1, 0);
                else
                    current = CellCoord::FromGlobalCoord(lowX, current.GetGlobalY() + 1);
            }
            void Skip(CellCoord const& nextLoadedGrid) { Next(); }
    };

    class OctagonCellArea : public CellArea
    {
        int16 range;
        int16 halfRange;
        int16 row;
        int16 column;
        uint8 sym;
        CellCoord center;
        CellCoord current;
        bool hasNextCoord;
    public:
        OctagonCellArea(float centerX, float centerY, float _range) : row(0), column(0), sym(0), hasNextCoord(true)
        {
            range = uint16(std::ceil(std::min(_range, SIZE_OF_GRIDS) / SIZE_OF_GRID_CELL));
            halfRange = range / 2;
            center = CellCoord::FromPosition(centerX, centerY);
            Next();
        }

        CellCoord Get() const { return current; }
        bool HasCoord() const
        {
            return current.GetCellX() < MAX_NUMBER_OF_CELLS;
        }

        void Next()
        {
            if (!hasNextCoord)
            {
                current = CellCoord::FromCoords(0, 0, MAX_NUMBER_OF_CELLS + 1, 0);
                return;
            }

            const int16 xOffset = column - halfRange - std::min(row, halfRange);
            const int16 yOffset = range - row;

            switch (sym)
            {
            case 0:
            {
                current = center.Offset(xOffset, yOffset);
                if (xOffset)
                    sym = 1;
                else if (yOffset)
                    sym = 2;
                else
                    hasNextCoord = false;
                return;
            }
            case 1:
            {
                current = center.Offset(-xOffset, yOffset);
                if (yOffset)
                {
                    sym = 2;
                    return;
                }
                break;
            }
            case 2:
            {
                current = center.Offset(xOffset, -yOffset);
                if (xOffset)
                {
                    sym = 3;
                    return;
                }
                break;
            }
            default: current = center.Offset(-xOffset, -yOffset); break;
            }

            sym = 0;
            if (xOffset < 0)
                ++column;
            else if (yOffset > 0)
            {
                ++row;
                column = 0;
            }
            else
                assert(false);
        }
        void Skip(CellCoord const& nextLoadedGrid) { Next(); }
    };

    class CircleCellArea : public CellArea
    {
        CellCoord current, origin;
        uint16 max_x, max_y;

        float range, centerX, centerY;

        void _Next();
    public:
        CircleCellArea(float centerX, float centerY, float range);
        bool HasCoord() const;
        CellCoord Get() const;
        void Next();
        void Skip(CellCoord const& nextLoadedGrid);
    };
}
