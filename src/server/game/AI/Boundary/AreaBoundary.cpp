#include "AreaBoundary.hpp"

namespace boundary
{

EllipseBoundary::EllipseBoundary(Position const& center, float radiusX, float radiusY) :
    _center(center), _radiusYSq(radiusY * radiusY), _scaleXSq(_radiusYSq / (radiusX * radiusX))
{
}

bool EllipseBoundary::ContainsPosition(Position const& pos) const
{
    float offX = _center.GetPositionX() - pos.GetPositionX();
    float offY = _center.GetPositionY() - pos.GetPositionY();
    return (offX*offX)*_scaleXSq + (offY*offY) <= _radiusYSq;
}

ZRangeBoundary::ZRangeBoundary(float minZ, float maxZ) :
    _minZ(minZ), _maxZ(maxZ)
{
}

bool ZRangeBoundary::ContainsPosition(Position const& pos) const
{
    return (_minZ <= pos.GetPositionZ() && pos.GetPositionZ() <= _maxZ);
}

PolygonBoundary::PolygonBoundary(const std::vector<Point_xy>& verticies)
{
    assign_points(_polygon, verticies);
}

bool PolygonBoundary::ContainsPosition(Position const& pos) const
{
    return boost::geometry::within(Point_xy(pos.m_positionX, pos.m_positionY), _polygon);
}

UnionBoundary::UnionBoundary(std::vector<AreaBoundary const*>&& boundaries) :
    _boundaries(std::move(boundaries))
{
}

bool UnionBoundary::ContainsPosition(Position const& pos) const
{
    for (auto boundary : _boundaries)
        if (!boundary->ContainsPosition(pos))
            return false;
    return true;
}

}
