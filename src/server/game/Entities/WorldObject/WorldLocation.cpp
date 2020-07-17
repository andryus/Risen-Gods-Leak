#include "WorldLocation.h"
#include "Grids/GridDefines.h"

bool WorldLocation::IsPositionValid() const
{
    return IsPositionValid(*this);
}

bool WorldLocation::IsPositionValid(Position pos)
{
    return Trinity::IsValidMapCoord(pos.m_positionX, pos.m_positionY, pos.m_positionZ, pos.m_orientation);
}
