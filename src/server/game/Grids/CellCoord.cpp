#include "CellCoord.hpp"

using namespace Grids;

CircleCellArea::CircleCellArea(float centerX, float centerY, float maxRange) : range(std::min(maxRange, SIZE_OF_GRIDS)), centerX(centerX), centerY(centerY)
{
    origin = CellCoord::FromPosition(centerX, centerY);
    // Calculate low and upper bound for y-axis
    float lowerBound = centerY - range;
    float upperBound = centerY + range;
    Trinity::NormalizeMapCoord(lowerBound);
    Trinity::NormalizeMapCoord(upperBound);
    const uint16 min_y = CellCoord::CalculateCellCoordFromPosition(lowerBound);
    max_y = CellCoord::CalculateCellCoordFromPosition(upperBound);

    // x-axis will be calculated in _Next()
    max_x = 0;
    current = CellCoord::FromGlobalCoord(0, min_y - 1);
    _Next();
}

bool CircleCellArea::HasCoord() const
{
    return current.GetGlobalX() <= max_x && current.GetGlobalY() <= max_y;
}

CellCoord CircleCellArea::Get() const
{
    return current;
}

void CircleCellArea::_Next()
{
    // same row
    if (current.GetGlobalX() < max_x)
        current = current.Offset(1, 0);
    else
    {
        current = current.Offset(0, 1);

        if (current.GetGlobalY() > max_y)
        {
            current = CellCoord::FromGlobalCoord(TOTAL_NUMBER_OF_CELLS_PER_MAP + 1, TOTAL_NUMBER_OF_CELLS_PER_MAP + 1);
            return;
        }

        // calculate lower and upper bounds for next row
        float yDiff;
        uint16 yOrigin = origin.GetGlobalY();
        uint16 yCurrentRow = current.GetGlobalY();
        bool swapped = false;
        if (yOrigin < yCurrentRow)
        {
            swapped = true;
            const uint16 swap = yCurrentRow;
            yCurrentRow = yOrigin;
            yOrigin = swap;
        }

        if (yOrigin - yCurrentRow <= 1)
        {
            if (yOrigin == yCurrentRow)
                yDiff = 0;
            else if (swapped)
                yDiff = centerY - origin.GetLowY();
            else
                yDiff = origin.GetHighY() - centerY;
        }
        else
            yDiff = (yOrigin - yCurrentRow - 1) * SIZE_OF_GRID_CELL;

        const float xRangeSq = range * range - yDiff * yDiff;
        const float xRange = (xRangeSq > 0.f) ? std::sqrt(xRangeSq) : 0.f;

        float lowerBound = centerX - xRange;
        float upperBound = centerX + xRange;
        Trinity::NormalizeMapCoord(lowerBound);
        Trinity::NormalizeMapCoord(upperBound);
        const uint16 min_x = CellCoord::CalculateCellCoordFromPosition(lowerBound);
        max_x = CellCoord::CalculateCellCoordFromPosition(upperBound);

        current = CellCoord::FromGlobalCoord(min_x, current.GetGlobalY());
    }
}

void CircleCellArea::Next()
{
    _Next();
    while (HasCoord() && !Get().IsValid())
        _Next();
}

void CircleCellArea::Skip(CellCoord const& nextLoadedGrid)
{
    if (nextLoadedGrid.GetGlobalY() > max_y)
    {
        current = CellCoord::FromGlobalCoord(TOTAL_NUMBER_OF_CELLS_PER_MAP + 1, TOTAL_NUMBER_OF_CELLS_PER_MAP + 1);
        return;
    }

    // skip grid rows
    if (nextLoadedGrid.GetGlobalY() > current.GetGlobalY())
    {
        max_x = 0;
        current = CellCoord::FromGlobalCoord(0, nextLoadedGrid.GetGlobalY() - 1);
        _Next();
        if (!HasCoord())
            return;
    }

    if (current.GetGridY() == nextLoadedGrid.GetGridY())
    {
        const int16 xOffset = int16(nextLoadedGrid.GetGlobalX()) - int16(current.GetGlobalX());
        if (xOffset > 0)
            current = current.Offset(xOffset, 0); // Move forward in current cell row
        else
            current = current.Offset(xOffset, 1); // Move to next cell row
    }
}
