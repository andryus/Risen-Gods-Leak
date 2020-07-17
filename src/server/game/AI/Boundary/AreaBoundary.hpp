#pragma once

#include "Object.h"
#include "Position.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace boundary
{

using namespace boost::geometry;
using Point_xy = model::d2::point_xy<float>;
using Polygon = model::polygon<Point_xy, true, false>;

class AreaBoundary
{
public:
    virtual ~AreaBoundary() = default;

    virtual bool ContainsPosition(Position const& pos) const = 0;
};

class EllipseBoundary : public AreaBoundary
{
public:
    EllipseBoundary(Position const& center, float radiusX, float radiusY);

    bool ContainsPosition(Position const& pos) const override;

private:
    const Position _center;
    const float _radiusYSq, _scaleXSq;
};

class ZRangeBoundary : public AreaBoundary
{
public:
    ZRangeBoundary(float minZ, float maxZ);

    bool ContainsPosition(Position const& pos) const override;

private:
    const float _minZ, _maxZ;
};

class PolygonBoundary : public AreaBoundary
{
public:
    PolygonBoundary(const std::vector<Point_xy>& verticies);

    bool ContainsPosition(Position const& pos) const;

private:
    Polygon _polygon;
};

class UnionBoundary : public AreaBoundary
{
public:
    UnionBoundary(std::vector<AreaBoundary const*>&& boundaries);

    bool ContainsPosition(Position const& pos) const override;

private:
    const std::vector<AreaBoundary const*> _boundaries;
};

}
