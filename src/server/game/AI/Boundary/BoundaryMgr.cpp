#include "DatabaseEnv.h"

#include "AreaBoundary.hpp"
#include "BoundaryMgr.hpp"
#include "Utilities/Timer.h"

namespace boundary
{

std::unordered_map<uint16 /*id*/, std::unique_ptr<AreaBoundary>> BoundaryMgr::_boundaryStore;

BoundaryMgr::BoundaryMgr()
{
}

BoundaryMgr::~BoundaryMgr()
{
}

void BoundaryMgr::LoadFromDB()
{
    const uint32 oldMSTime = getMSTime();

    //                                               0   1         2         3         4
    QueryResult result = WorldDatabase.Query("SELECT id, center_x, center_y, radius_x, radius_y FROM boundary_ellipse");
    if (result)
    {
        do
        {
            Field const* fields = result->Fetch();
            const uint16 id = fields[0].GetUInt16();
            const float centerX = fields[1].GetFloat();
            const float centerY = fields[2].GetFloat();
            const float radiusX = fields[3].GetFloat();
            const float radiusY = fields[4].GetFloat();

            _boundaryStore.emplace(id, std::make_unique<EllipseBoundary>(Position(centerX, centerY, 0.f), radiusX, radiusY));

        } while (result->NextRow());
    }

    //                                               0   1      2
    result = WorldDatabase.Query("SELECT id, min_z, max_z FROM boundary_z_range");
    if (result)
    {
        do
        {
            Field const* fields = result->Fetch();
            const uint16 id = fields[0].GetUInt16();
            const float minZ = fields[1].GetFloat();
            const float maxZ = fields[2].GetFloat();

            ASSERT(_boundaryStore.find(id) == _boundaryStore.end(), "BoundaryMgr::LoadFromDB Id must be uniqe over all boundary tables.");

            _boundaryStore.emplace(id, std::make_unique<ZRangeBoundary>(minZ, maxZ));

        } while (result->NextRow());
    }

    //                                   0   1         2        3
    result = WorldDatabase.Query("SELECT id, point_id, point_x, point_y FROM boundary_polygon ORDER BY id, point_id ASC");
    if (result)
    {
        std::unordered_map<uint16, std::vector<Point_xy>> tmpPolygonPoints;

        do
        {
            Field const* fields = result->Fetch();
            const uint16 id = fields[0].GetUInt16();
            const uint16 pointId = fields[1].GetUInt16();
            const float pointX = fields[2].GetFloat();
            const float pointY = fields[3].GetFloat();

            tmpPolygonPoints[id].emplace_back(pointX, pointY);

        } while (result->NextRow());

        for (auto& poly : tmpPolygonPoints)
        {
            ASSERT(_boundaryStore.find(poly.first) == _boundaryStore.end(), "BoundaryMgr::LoadFromDB Id must be uniqe over all boundary tables.");
            _boundaryStore.emplace(poly.first, std::make_unique<PolygonBoundary>(poly.second));
        }
    }

    //                                   0   1
    result = WorldDatabase.Query("SELECT id, ref_id FROM boundary_union ORDER BY id, ref_id ASC");
    if (result)
    {
        std::unordered_map<uint16, std::vector<AreaBoundary const*>> tmpUnionBoundary;

        do
        {
            Field const* fields = result->Fetch();
            const uint16 id = fields[0].GetUInt16();
            const uint16 refId = fields[1].GetUInt16();

            AreaBoundary const* boundary = FindBoundary(refId);
            ASSERT(boundary, "BoundaryMgr::LoadFromDB Tryed to creatre union boundary %u with non-existing boundary %u", id, refId);

            tmpUnionBoundary[id].push_back(boundary);

        } while (result->NextRow());

        for (auto& unions : tmpUnionBoundary)
        {
            ASSERT(_boundaryStore.find(unions.first) == _boundaryStore.end(), "BoundaryMgr::LoadFromDB Id must be uniqe over all boundary tables.");
            _boundaryStore.emplace(unions.first, std::make_unique<UnionBoundary>(std::move(unions.second)));
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " boundarys in %u ms", _boundaryStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

AreaBoundary const* BoundaryMgr::FindBoundary(uint16 id)
{
    if (id == 0)
        return nullptr;

    auto itr = _boundaryStore.find(id);
    return itr != _boundaryStore.end() ? itr->second.get() : nullptr;
}

}
