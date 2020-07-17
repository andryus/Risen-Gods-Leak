/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PathGenerator.h"
#include "Map.h"
#include "Creature.h"
#include "MMapFactory.h"
#include "MMapManager.h"
#include "Log.h"
#include "DisableMgr.h"
#include "DetourCommon.h"
#include "MoveSpline.h"
#include "Transport.h"
#include "ObjectMgr.h"
#include "World.h"

PathQueryFilter::PathQueryFilter(const Unit* owner) : owner(owner), mapId(owner->GetMapId())
{
    if (DisableMgr::IsPathfindingEnabled(mapId))
    {
        bool pathfinding = true;
        if (const Creature* creature = owner->ToCreature())
        {
            if (!creature->IsPathfindingEnabled())
                pathfinding = false;
        }
        if (pathfinding)
        {
            MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
            if (owner->GetTransport() == NULL)
                navMesh = mmap->GetNavMesh(mapId);
        }
    }
}

bool PathQueryFilter::passFilter(const dtPolyRef ref, const dtMeshTile* tile, const dtPoly* poly) const
{
    if (dtQueryFilter::passFilter(ref, tile, poly))
    {
        if (navMesh && (poly->flags & NAV_DYNAMIC))
        {
            unsigned int salt, it, ip;
            navMesh->decodePolyId(ref, salt, it, ip);
            PolyIdentifier pId = std::make_tuple(mapId, tile->header->x, tile->header->y, ip);
            bool pass = true;
            if (std::vector<uint32> const* spawnIds = sObjectMgr->GetPolygonAssignments(pId))
            {
                for (uint32 spawnId : *spawnIds)
                {
                    auto optionalGO = owner->GetMap()->GetGameObjectBySpawnId(spawnId);
                    if (optionalGO)
                    {
                        GameObject& go = *optionalGO;
                        if (go.IsInWorld() && go.isSpawned() && (go.GetPhaseMask() & owner->GetPhaseMask()) && go.m_model)
                        {
                            if (go.m_model->isEnabled())
                            {
                                pass = false;
                            }
                            else
                            {
                                pass = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        TC_LOG_DEBUG("maps", "PathQueryFilter::passFilter: Could not find game object %u assigned to polygon [%u,%d,%d,%d]",
                            spawnId, mapId, tile->header->x, tile->header->y, ip);
                    }
                }
            }
            else
            {
                TC_LOG_DEBUG("maps", "PathQueryFilter::passFilter: Dynamic polygon [%u,%d,%d,%d] has no assigned game objects",
                    mapId, tile->header->x, tile->header->y, ip);
            }
            return pass;
        }
        else
            return true;
    }
    else
        return false;
}

////////////////// PathGenerator //////////////////
PathGenerator::PathGenerator(const Unit* owner) :
    _polyLength(0), _type(PATHFIND_BLANK), _useStraightPath(false),
    _forceDestination(false), _pointPathLimit(MAX_POINT_PATH_LENGTH), _straightLine(false),
    _endPosition(G3D::Vector3::zero()), _sourceUnit(owner), _navMesh(NULL),
    _navMeshQuery(NULL), _transport(owner->GetTransport()), _filter(owner)
{
    memset(_pathPolyRefs, 0, sizeof(_pathPolyRefs));

    TC_LOG_DEBUG("maps", "++ PathGenerator::PathGenerator for %u \n", _sourceUnit->GetGUID().GetCounter());

    uint32 mapId = _sourceUnit->GetMapId();

    if (DisableMgr::IsPathfindingEnabled(mapId))
    {
        bool pathfinding = true;
        if (const Creature* creature = _sourceUnit->ToCreature())
        {
            if (!creature->IsPathfindingEnabled())
                pathfinding = false;
        }
        if (pathfinding)
        {
            MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
            if (_transport)
            {
                _navMesh = mmap->GetModelNavMesh(_transport->GetDisplayId());
                _navMeshQuery = mmap->GetModelNavMeshQuery(_transport->GetDisplayId());
            }
            else
            {
                _navMesh = mmap->GetNavMesh(mapId);
                _navMeshQuery = mmap->GetNavMeshQuery(mapId);
            }
        }
    }

    CreateFilter();
}

PathGenerator::~PathGenerator()
{
    TC_LOG_DEBUG("maps", "++ PathGenerator::~PathGenerator() for %u \n", _sourceUnit->GetGUID().GetCounter());
}

bool PathGenerator::CalculatePath(float destX, float destY, float destZ, bool forceDest, bool straightLine)
{
    float x, y, z;
    if (_sourceUnit->movespline->Finalized())
    {
        if (_transport)
            _sourceUnit->m_movementInfo.transport.pos.GetPosition(x, y, z);
        else
            _sourceUnit->GetPosition(x, y, z);
    }
    else
    {
        auto reallocation = _sourceUnit->movespline->ComputePosition();
        x = reallocation.x;
        y = reallocation.y;
        z = reallocation.z;
    }

    if (!Trinity::IsValidMapCoord(destX, destY, destZ) || !Trinity::IsValidMapCoord(x, y, z))
        return false;

    G3D::Vector3 dest(destX, destY, destZ);
    SetEndPosition(dest);

    G3D::Vector3 start(x, y, z);
    SetStartPosition(start);

    _forceDestination = forceDest;
    _straightLine = straightLine;

    TC_LOG_DEBUG("maps", "++ PathGenerator::CalculatePath() for %u \n", _sourceUnit->GetGUID().GetCounter());

    // make sure navMesh works - we can run on map w/o mmap
    // check if the start and end point have a .mmtile loaded (can we pass via not loaded tile on the way?)
    if (!_navMesh || !_navMeshQuery || _sourceUnit->HasUnitState(UNIT_STATE_IGNORE_PATHFINDING) ||
        !HaveTile(start) || !HaveTile(dest))
    {
        BuildShortcut(GetStartPosition(), GetActualEndPosition());
        _type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
        _flags = PATH_EXTRA_CLEAR;
        return true;
    }

    MMAP::MMapManager* mmap = MMAP::MMapFactory::createOrGetMMapManager();
    if (_transport)
        _navMeshQuery = mmap->GetModelNavMeshQuery(_transport->GetDisplayId());
    else
        _navMeshQuery = mmap->GetNavMeshQuery(_sourceUnit->GetMapId());

    UpdateFilter();

    UpdateExtraFlags();
    
    BuildPolyPath(start, dest);
    return true;
}

dtPolyRef PathGenerator::GetPathPolyByPosition(dtPolyRef const* polyPath, uint32 polyPathSize, float const* point, float* distance) const
{
    if (!polyPath || !polyPathSize)
        return INVALID_POLYREF;

    dtPolyRef nearestPoly = INVALID_POLYREF;
    float minDist2d = FLT_MAX;
    float minDist3d = 0.0f;

    for (uint32 i = 0; i < polyPathSize; ++i)
    {
        float closestPoint[VERTEX_SIZE];
        if (dtStatusFailed(_navMeshQuery->closestPointOnPoly(polyPath[i], point, closestPoint, NULL)))
            continue;

        float d = dtVdist2DSqr(point, closestPoint);
        if (d < minDist2d)
        {
            minDist2d = d;
            nearestPoly = polyPath[i];
            minDist3d = dtVdistSqr(point, closestPoint);
        }

        if (minDist2d < 1.0f) // shortcut out - close enough for us
            break;
    }

    if (distance)
        *distance = dtMathSqrtf(minDist3d);

    return (minDist2d < 3.0f) ? nearestPoly : INVALID_POLYREF;
}

dtPolyRef PathGenerator::GetPolyByLocation(float const* point, float* distance) const
{
    // first we check the current path
    // if the current path doesn't contain the current poly,
    // we need to use the expensive navMesh.findNearestPoly
    dtPolyRef polyRef = GetPathPolyByPosition(_pathPolyRefs, _polyLength, point, distance);
    if (polyRef != INVALID_POLYREF)
        return polyRef;

    // we don't have it in our old path
    // try to get it by findNearestPoly()
    // first try with low search box
    float extents[VERTEX_SIZE] = {3.0f, 5.0f, 3.0f};    // bounds of poly search area
    float closestPoint[VERTEX_SIZE] = {0.0f, 0.0f, 0.0f};
    if (dtStatusSucceed(_navMeshQuery->findNearestPoly(point, extents, &_filter, &polyRef, closestPoint)) && polyRef != INVALID_POLYREF)
    {
        *distance = dtVdist(closestPoint, point);
        return polyRef;
    }

    // still nothing ..
    // try with bigger search box
    // Note that the extent should not overlap more than 128 polygons in the navmesh (see dtNavMeshQuery::findNearestPoly)
    extents[1] = 50.0f;

    if (dtStatusSucceed(_navMeshQuery->findNearestPoly(point, extents, &_filter, &polyRef, closestPoint)) && polyRef != INVALID_POLYREF)
    {
        *distance = dtVdist(closestPoint, point);
        return polyRef;
    }

    return INVALID_POLYREF;
}

void PathGenerator::BuildPolyPath(G3D::Vector3 startPos, G3D::Vector3 endPos)
{
    const auto UpdateEndPosition = [this]()
    {
        ASSERT(!_pathPoints.empty());

        // first point is always our current location - we need the next one
        SetActualEndPosition(*_pathPoints.rbegin());

        // force the given destination, if needed
        if (_forceDestination &&
            (!(_type & PATHFIND_NORMAL) || !InRange(GetEndPosition(), GetActualEndPosition(), 1.0f, 1.0f)))
        {
            // we may want to keep partial subpath
            if (Dist3DSqr(GetActualEndPosition(), GetEndPosition()) < 0.3f * Dist3DSqr(GetStartPosition(), GetEndPosition()))
            {
                SetActualEndPosition(GetEndPosition());
                _pathPoints[_pathPoints.size() - 1] = GetEndPosition();
            }
            else
            {
                SetActualEndPosition(GetEndPosition());
                BuildShortcut(GetStartPosition(), GetActualEndPosition());
            }

            _type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
        }
    };

    Movement::PointsArray prefix, suffix;

    const bool canShortcutAtStart = HasPathFlag(PATH_EXTRA_ALLOW_WATER_SHORTCUTS) && HasPathFlag(PATH_EXTRA_START_IN_WATER);
    const bool canShortcutAtEnd = HasPathFlag(PATH_EXTRA_ALLOW_WATER_SHORTCUTS) && HasPathFlag(PATH_EXTRA_END_IN_WATER);

    if (HasPathFlag(PATH_EXTRA_ALLOW_AIR_SHORTCUTS) || canShortcutAtStart)
    {
        if (Find3dPath(startPos, endPos, prefix))
        {
            // We found a direct path to our target
            _pathPoints.swap(prefix);
            UpdateEndPosition();
            _type = PathType(PATHFIND_NORMAL);
            return;
        }

        if (HasPathFlag(PATH_EXTRA_ALLOW_AIR_SHORTCUTS))
        {
            BuildShortcut(startPos, endPos);
            _type = PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH);
            return;
        }
    }

    if (!prefix.empty())
        startPos = *prefix.rbegin();

    if (canShortcutAtEnd)
    {
        if (Find3dPath(endPos, startPos, suffix))
        {
            _pathPoints.swap(suffix);
            UpdateEndPosition();
            return;
        }
    }

    if (!suffix.empty())
        endPos = *suffix.rbegin();
    
    BuildDetourPath(startPos, endPos);

    _flags = PathExtraFlags(_flags | PATH_EXTRA_USING_GROUND_PATH);

    if (HasPathFlag(PATH_EXTRA_ALLOW_AIR_SHORTCUTS) || HasPathFlag(PATH_EXTRA_ALLOW_WATER_SHORTCUTS))
        FindShortcutsOnPointPath(_pathPoints);

    ConcatenatePath(prefix, suffix);

    UpdateEndPosition();
}

void PathGenerator::BuildDetourPath(G3D::Vector3 const& startPos, G3D::Vector3 const& endPos)
{
    // *** getting start/end poly logic ***

    float distToStartPoly, distToEndPoly;
    float startPoint[VERTEX_SIZE] = { startPos.y, startPos.z, startPos.x };
    float endPoint[VERTEX_SIZE] = { endPos.y, endPos.z, endPos.x };

    const dtPolyRef startPoly = GetPolyByLocation(startPoint, &distToStartPoly);
    const dtPolyRef endPoly = GetPolyByLocation(endPoint, &distToEndPoly);

    // we have a hole in our mesh
    // make shortcut path and mark it as NOPATH ( with flying and swimming exception )
    // its up to caller how he will use this info
    if (startPoly == INVALID_POLYREF || endPoly == INVALID_POLYREF)
    {
        TC_LOG_DEBUG("maps", "++ BuildPolyPath :: (startPoly == 0 || endPoly == 0)\n");
        BuildShortcut(startPos, endPos);
        const bool path = _sourceUnit->GetTypeId() == TYPEID_UNIT && _sourceUnit->ToCreature()->CanFly();

        bool waterPath = _sourceUnit->GetTypeId() == TYPEID_UNIT && _sourceUnit->ToCreature()->CanSwim();
        if (waterPath)
        {
            // Check both start and end points, if they're both in water, then we can *safely* let the creature move
            for (uint32 i = 0; i < _pathPoints.size(); ++i)
            {
                ZLiquidStatus status = _sourceUnit->GetBaseMap()->getLiquidStatus(_pathPoints[i].x, _pathPoints[i].y, _pathPoints[i].z, MAP_ALL_LIQUIDS, NULL);
                // One of the points is not in the water, cancel movement.
                if (status == LIQUID_MAP_NO_WATER)
                {
                    waterPath = false;
                    break;
                }
            }
        }

        _type = (path || waterPath) ? PathType(PATHFIND_NORMAL | PATHFIND_NOT_USING_PATH) : PATHFIND_NOPATH;
        return;
    }

    // we may need a better number here
    const bool farFromPoly = distToStartPoly > 7.0f;
    _type = farFromPoly ? PATHFIND_INCOMPLETE : PATHFIND_NORMAL;
    if (farFromPoly)
    {
        TC_LOG_DEBUG("maps", "++ BuildPolyPath :: farFromPoly distToStartPoly=%.3f distToEndPoly=%.3f\n", distToStartPoly, distToEndPoly);

        float closestPoint[VERTEX_SIZE];
        // we may want to use closestPointOnPolyBoundary instead
        if (dtStatusSucceed(_navMeshQuery->closestPointOnPoly(endPoly, endPoint, closestPoint, NULL)))
        {
            dtVcopy(endPoint, closestPoint);
            SetActualEndPosition(G3D::Vector3(endPoint[2], endPoint[0], endPoint[1]));
        }
    }

    // *** poly path generating logic ***

    // start and end are on same polygon
    // just need to move in straight line
    if (startPoly == endPoly)
    {
        TC_LOG_DEBUG("maps", "++ BuildPolyPath :: (startPoly == endPoly)\n");

        _pathPolyRefs[0] = startPoly;
        _polyLength = 1;

        BuildDetourPointPath(startPoint, endPoint);

        _actualEndPosition = *_pathPoints.rbegin();

        TC_LOG_DEBUG("maps", "++ BuildPolyPath :: path type %d\n", _type);
        return;
    }

    // look for startPoly/endPoly in current path
    /// @todo we can merge it with getPathPolyByPosition() loop
    bool startPolyFound = false;
    bool endPolyFound = false;
    uint32 pathStartIndex = 0;
    uint32 pathEndIndex = 0;

    if (_polyLength && sWorld->getFloatConfig(CONFIG_PERFORMANCE_PATHFINDING_REUSE))
    {
        for (; pathStartIndex < _polyLength; ++pathStartIndex)
        {
            // here to catch few bugs
            if (_pathPolyRefs[pathStartIndex] == INVALID_POLYREF)
            {
                TC_LOG_ERROR("maps", "Invalid poly ref in BuildPolyPath. _polyLength: %u, pathStartIndex: %u,"
                                     " startPos: %s, endPos: %s, mapid: %u",
                                     _polyLength, pathStartIndex, startPos.toString().c_str(), endPos.toString().c_str(),
                                     _sourceUnit->GetMapId());

                break;
            }

            if (_pathPolyRefs[pathStartIndex] == startPoly)
            {
                startPolyFound = true;
                break;
            }
        }

        for (pathEndIndex = _polyLength-1; pathEndIndex > pathStartIndex; --pathEndIndex)
            if (_pathPolyRefs[pathEndIndex] == endPoly)
            {
                endPolyFound = true;
                break;
            }
    }

    if (startPolyFound && endPolyFound)
    {
        TC_LOG_DEBUG("maps", "++ BuildPolyPath :: (startPolyFound && endPolyFound)\n");

        // we moved along the path and the target did not move out of our old poly-path
        // our path is a simple subpath case, we have all the data we need
        // just "cut" it out

        _polyLength = pathEndIndex - pathStartIndex + 1;
        memmove(_pathPolyRefs, _pathPolyRefs + pathStartIndex, _polyLength * sizeof(dtPolyRef));
    }
    else if (startPolyFound && !endPolyFound)
    {
        TC_LOG_DEBUG("maps", "++ BuildPolyPath :: (startPolyFound && !endPolyFound)\n");

        // we are moving on the old path but target moved out
        // so we have atleast part of poly-path ready

        _polyLength -= pathStartIndex;

        // try to adjust the suffix of the path instead of recalculating entire length
        // at given interval the target cannot get too far from its last location
        // thus we have less poly to cover
        // sub-path of optimal path is optimal

        // take ~80% of the original length
        /// @todo play with the values here
        uint32 prefixPolyLength = uint32(_polyLength * sWorld->getFloatConfig(CONFIG_PERFORMANCE_PATHFINDING_REUSE) + 0.5f);
        memmove(_pathPolyRefs, _pathPolyRefs+pathStartIndex, prefixPolyLength * sizeof(dtPolyRef));

        dtPolyRef suffixStartPoly = _pathPolyRefs[prefixPolyLength-1];

        // we need any point on our suffix start poly to generate poly-path, so we need last poly in prefix data
        float suffixEndPoint[VERTEX_SIZE];
        if (dtStatusFailed(_navMeshQuery->closestPointOnPoly(suffixStartPoly, endPoint, suffixEndPoint, NULL)))
        {
            // we can hit offmesh connection as last poly - closestPointOnPoly() don't like that
            // try to recover by using prev polyref
            --prefixPolyLength;
            suffixStartPoly = _pathPolyRefs[prefixPolyLength-1];
            if (dtStatusFailed(_navMeshQuery->closestPointOnPoly(suffixStartPoly, endPoint, suffixEndPoint, NULL)))
            {
                // suffixStartPoly is still invalid, error state
                BuildShortcut(startPos, endPos);
                _type = PATHFIND_NOPATH;
            }
        }

        // generate suffix
        uint32 suffixPolyLength = 0;

        dtStatus dtResult;
        if (_straightLine)
        {
            float hit = 0;
            float hitNormal[3];
            memset(hitNormal, 0, sizeof(hitNormal));

            dtResult = _navMeshQuery->raycast(
                            suffixStartPoly,
                            suffixEndPoint,
                            endPoint,
                            &_filter,
                            &hit,
                            hitNormal,
                            _pathPolyRefs + prefixPolyLength - 1,
                            (int*)&suffixPolyLength,
                            MAX_PATH_LENGTH - prefixPolyLength);

            // raycast() sets hit to FLT_MAX if there is a ray between start and end
            if (hit != FLT_MAX)
            {
                // the ray hit something, return no path instead of the incomplete one
                _type = PATHFIND_NOPATH;
                BuildShortcut(startPos, endPos);
                return;
            }
        }
        else
        {
            dtResult = _navMeshQuery->findPath(
                            suffixStartPoly,    // start polygon
                            endPoly,            // end polygon
                            suffixEndPoint,     // start position
                            endPoint,           // end position
                            &_filter,            // polygon search filter
                            _pathPolyRefs + prefixPolyLength - 1,    // [out] path
                            (int*)&suffixPolyLength,
                            MAX_PATH_LENGTH - prefixPolyLength);   // max number of polygons in output path
        }

        if (!suffixPolyLength || dtStatusFailed(dtResult))
        {
            // this is probably an error state, but we'll leave it
            // and hopefully recover on the next Update
            // we still need to copy our preffix
            TC_LOG_ERROR("maps", "%u's Path Build failed: 0 length path", _sourceUnit->GetGUID().GetCounter());
        }

        TC_LOG_DEBUG("maps", "++  m_polyLength=%u prefixPolyLength=%u suffixPolyLength=%u \n", _polyLength, prefixPolyLength, suffixPolyLength);

        // new path = prefix + suffix - overlap
        _polyLength = prefixPolyLength + suffixPolyLength - 1;
    }
    else
    {
        TC_LOG_DEBUG("maps", "++ BuildPolyPath :: (!startPolyFound && !endPolyFound)\n");

        // either we have no path at all -> first run
        // or something went really wrong -> we aren't moving along the path to the target
        // just generate new path

        // free and invalidate old path data
        Clear();

        dtStatus dtResult;
        if (_straightLine)
        {
            float hit = 0;
            float hitNormal[3];
            memset(hitNormal, 0, sizeof(hitNormal));

            dtResult = _navMeshQuery->raycast(
                            startPoly,
                            startPoint,
                            endPoint,
                            &_filter,
                            &hit,
                            hitNormal,
                            _pathPolyRefs,
                            (int*)&_polyLength,
                            MAX_PATH_LENGTH);

            // raycast() sets hit to FLT_MAX if there is a ray between start and end
            if (hit != FLT_MAX)
            {
                // the ray hit something, return no path instead of the incomplete one
                _type = PATHFIND_NOPATH;
                BuildShortcut(startPos, endPos);
                return;
            }
        }
        else
        {
            dtResult = _navMeshQuery->findPath(
                            startPoly,          // start polygon
                            endPoly,            // end polygon
                            startPoint,         // start position
                            endPoint,           // end position
                            &_filter,           // polygon search filter
                            _pathPolyRefs,     // [out] path
                            (int*)&_polyLength,
                            MAX_PATH_LENGTH);   // max number of polygons in output path
        }

        if (!_polyLength || dtStatusFailed(dtResult))
        {
            // only happens if we passed bad data to findPath(), or navmesh is messed up
            TC_LOG_ERROR("maps", "%u's Path Build failed: 0 length path", _sourceUnit->GetGUID().GetCounter());
            BuildShortcut(startPos, endPos);
            _type = PATHFIND_NOPATH;
        }
    }

    // by now we know what type of path we can get
    if (_pathPolyRefs[_polyLength - 1] == endPoly && !(_type & PATHFIND_INCOMPLETE))
        _type = PATHFIND_NORMAL;
    else
        _type = PATHFIND_INCOMPLETE;

    // generate the point-path out of our up-to-date poly-path
    BuildDetourPointPath(startPoint, endPoint);
}

void PathGenerator::ConcatenatePath(Movement::PointsArray const& prefix, Movement::PointsArray const& suffix)
{
    // now build final path
    if (!prefix.empty() || !suffix.empty())
    {
        ASSERT(_pathPoints.size() >= 2);
        Movement::PointsArray result;
        result.reserve(prefix.size() + _pathPoints.size() + suffix.size());
        std::copy(prefix.begin(), prefix.end(), std::back_inserter(result));
        auto startItr = _pathPoints.begin();
        if (!prefix.empty())
            ++startItr;
        auto endItr = _pathPoints.end();
        if (!suffix.empty())
            --endItr;
        std::copy(startItr, endItr, std::back_inserter(result));
        std::copy(suffix.rbegin(), suffix.rend(), std::back_inserter(result));
        _pathPoints.swap(result);
    }
}

void PathGenerator::BuildDetourPointPath(const float *startPoint, const float *endPoint)
{
    float pathPoints[MAX_POINT_PATH_LENGTH*VERTEX_SIZE];
    uint32 pointCount = 0;
    dtStatus dtResult = DT_FAILURE;
    if (_straightLine)
    {
        dtResult = DT_SUCCESS;
        pointCount = 1;
        memcpy(&pathPoints[VERTEX_SIZE * 0], startPoint, sizeof(float)* 3); // first point

        // path has to be split into polygons with dist SMOOTH_PATH_STEP_SIZE between them
        G3D::Vector3 startVec = G3D::Vector3(startPoint[0], startPoint[1], startPoint[2]);
        G3D::Vector3 endVec = G3D::Vector3(endPoint[0], endPoint[1], endPoint[2]);
        G3D::Vector3 diffVec = (endVec - startVec);
        G3D::Vector3 prevVec = startVec;
        float len = diffVec.length();
        diffVec *= SMOOTH_PATH_STEP_SIZE / len;
        while (len > SMOOTH_PATH_STEP_SIZE)
        {
            len -= SMOOTH_PATH_STEP_SIZE;
            prevVec += diffVec;
            pathPoints[VERTEX_SIZE * pointCount + 0] = prevVec.x;
            pathPoints[VERTEX_SIZE * pointCount + 1] = prevVec.y;
            pathPoints[VERTEX_SIZE * pointCount + 2] = prevVec.z;
            ++pointCount;
        }

        memcpy(&pathPoints[VERTEX_SIZE * pointCount], endPoint, sizeof(float)* 3); // last point
        ++pointCount;
    }
    else if (_useStraightPath)
    {
        dtResult = _navMeshQuery->findStraightPath(
                startPoint,         // start position
                endPoint,           // end position
                _pathPolyRefs,     // current path
                _polyLength,       // lenth of current path
                pathPoints,         // [out] path corner points
                NULL,               // [out] flags
                NULL,               // [out] shortened path
                (int*)&pointCount,
                _pointPathLimit);   // maximum number of points/polygons to use
    }
    else
    {
        dtResult = FindSmoothPath(
                startPoint,         // start position
                endPoint,           // end position
                _pathPolyRefs,     // current path
                _polyLength,       // length of current path
                pathPoints,         // [out] path corner points
                (int*)&pointCount,
                _pointPathLimit);    // maximum number of points
    }

    if (pointCount < 2 || dtStatusFailed(dtResult))
    {
        // only happens if pass bad data to findStraightPath or navmesh is broken
        // single point paths can be generated here
        /// @todo check the exact cases
        TC_LOG_DEBUG("maps", "++ PathGenerator::BuildPointPath FAILED! path sized %d returned\n", pointCount);
        BuildShortcut(GetStartPosition(), GetActualEndPosition());
        _type = PATHFIND_NOPATH;
        return;
    }
    else if (pointCount == _pointPathLimit)
    {
        TC_LOG_DEBUG("maps", "++ PathGenerator::BuildPointPath FAILED! path sized %d returned, lower than limit set to %d\n", pointCount, _pointPathLimit);
        BuildShortcut(GetStartPosition(), GetActualEndPosition());
        _type = PATHFIND_SHORT;
        return;
    }

    _pathPoints.resize(pointCount);
    for (uint32 i = 0; i < pointCount; ++i)
        _pathPoints[i] = G3D::Vector3(pathPoints[i*VERTEX_SIZE+2], pathPoints[i*VERTEX_SIZE], pathPoints[i*VERTEX_SIZE+1]);

    TC_LOG_DEBUG("maps", "++ PathGenerator::BuildPointPath path type %d size %d poly-size %d\n", _type, pointCount, _polyLength);
}

void PathGenerator::NormalizePath()
{
    if (_transport) // path needs to be transformed into global coordinates
        return;

    for (uint32 i = 0; i < _pathPoints.size(); ++i)
        _sourceUnit->UpdateAllowedPositionZ(_pathPoints[i].x, _pathPoints[i].y, _pathPoints[i].z);
}

void PathGenerator::BuildShortcut(G3D::Vector3 const& start, G3D::Vector3 const& end)
{
    TC_LOG_DEBUG("maps", "++ BuildShortcut :: making shortcut\n");

    Clear();

    // make two point path, our curr pos is the start, and dest is the end
    _pathPoints.resize(2);

    // set start and a default next position
    _pathPoints[0] = start; // GetStartPosition();
    _pathPoints[1] = end; // GetActualEndPosition();

    NormalizePath();

    _type = PATHFIND_SHORTCUT;
}

bool IsPointInAir(const Map* map, G3D::Vector3 point)
{
    return point.z > map->GetHeight(point.x, point.y, point.z) + 2.f;
}

void PathGenerator::FindShortcutsOnPointPath(Movement::PointsArray& path) const
{
    if (_transport || !sWorld->getFloatConfig(CONFIG_PERFORMANCE_PATHFINDING_SHORTCUT_STEP_SIZE))
        return;

    const Map* map = _sourceUnit->GetBaseMap();
    bool openShortcut = false;
    uint32 writePos = 0;

    for (uint32 i = 0; i < path.size(); ++i)
    {
        bool canShortcut = false;
        if (HasPathFlag(PATH_EXTRA_ALLOW_AIR_SHORTCUTS) && IsPointInAir(map, path[i]))
            canShortcut = true;
        if (!canShortcut && HasPathFlag(PATH_EXTRA_ALLOW_WATER_SHORTCUTS))
        {
            ZLiquidStatus liquidStatus = map->getLiquidStatus(path[i].x, path[i].y, path[i].z, MAP_ALL_LIQUIDS);
            if (liquidStatus == LIQUID_MAP_IN_WATER || liquidStatus == LIQUID_MAP_UNDER_WATER)
                canShortcut = true;
        }

        if (canShortcut && openShortcut && IsShortcutWithoutObstacles(path[writePos - 1], path[i]))
            continue;

        openShortcut = canShortcut;

        if (writePos != i)
            path[writePos] = path[i];

        writePos++;
    }

    if (openShortcut && (path[writePos - 1] != path[path.size() - 1] || writePos < 2) && writePos < path.size())
        path[writePos++] = path[path.size() - 1];

    path.resize(writePos);
}

bool PathGenerator::IsShortcutWithoutObstacles(G3D::Vector3 const& startPoint, G3D::Vector3 const& endPoint) const
{
    const float STEP_SIZE = sWorld->getFloatConfig(CONFIG_PERFORMANCE_PATHFINDING_SHORTCUT_STEP_SIZE);
    if (!STEP_SIZE)
        return true;

    if (!_sourceUnit->GetBaseMap()->isInLineOfSight(startPoint.x, startPoint.y, startPoint.z,
        endPoint.x, endPoint.y, endPoint.z, _sourceUnit->GetPhaseMask()))
        return false;

    if (startPoint == endPoint)
        return true;

    if (_sourceUnit->GetBaseMap()->GetHeight(_sourceUnit->GetPhaseMask(), startPoint.x, startPoint.y, startPoint.z) == VMAP_INVALID_HEIGHT_VALUE
        && _sourceUnit->GetBaseMap()->GetHeight(_sourceUnit->GetPhaseMask(), endPoint.x, endPoint.y, endPoint.z) == VMAP_INVALID_HEIGHT_VALUE)
        return true;

    const bool canFly = HasPathFlag(PATH_EXTRA_START_IN_AIR);

    G3D::Vector3 iter = startPoint;
    G3D::Vector3 direction = endPoint - startPoint;
    float length = direction.length();
    direction = STEP_SIZE * direction / length;

    uint32 stepCount = length / STEP_SIZE;

    const Map* map = _sourceUnit->GetBaseMap();

    bool underground = iter.z < map->GetHeight(iter.x, iter.y, MAX_HEIGHT, false);

    for (uint32 i = 0; i < stepCount; i++)
    {
        iter += direction;
        if (underground != (iter.z < map->GetHeight(iter.x, iter.y, MAX_HEIGHT, false)))
            return false;
        if (!canFly && iter.z > map->GetWaterLevel(iter.x, iter.y))
            return false;
    }

    return true;
}

void PathGenerator::UpdateExtraFlags()
{
    _flags = PATH_EXTRA_CLEAR;

    const Map* map = _sourceUnit->GetBaseMap();

    ZLiquidStatus liquidStatus = map->getLiquidStatus(_startPosition.x, _startPosition.y, _startPosition.z, MAP_ALL_LIQUIDS);
    if (liquidStatus == LIQUID_MAP_IN_WATER || liquidStatus == LIQUID_MAP_UNDER_WATER)
        _flags = PathExtraFlags(_flags | PATH_EXTRA_START_IN_WATER);
    else if (IsPointInAir(map, _startPosition))
        _flags = PathExtraFlags(_flags | PATH_EXTRA_START_IN_AIR);

    liquidStatus = map->getLiquidStatus(_endPosition.x, _endPosition.y, _endPosition.z, MAP_ALL_LIQUIDS);
    if (liquidStatus == LIQUID_MAP_IN_WATER || liquidStatus == LIQUID_MAP_UNDER_WATER)
        _flags = PathExtraFlags(_flags | PATH_EXTRA_END_IN_WATER);
    else if (IsPointInAir(map, _endPosition))
        _flags = PathExtraFlags(_flags | PATH_EXTRA_END_IN_AIR);

    const bool allowGround = (_sourceUnit->GetTypeId() == TYPEID_PLAYER ||
        (_sourceUnit->GetTypeId() == TYPEID_UNIT && _sourceUnit->ToCreature()->IsInhabitting(INHABIT_GROUND)));
    const bool canFly = (_sourceUnit->GetTypeId() == TYPEID_UNIT && _sourceUnit->ToCreature()->CanFly())
        || (_sourceUnit->GetTypeId() == TYPEID_PLAYER && _sourceUnit->HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY));
    const bool canSwim = (_sourceUnit->GetTypeId() == TYPEID_UNIT && _sourceUnit->ToCreature()->CanSwim()) || _sourceUnit->GetTypeId() == TYPEID_PLAYER;

    if (allowGround)
        _flags = PathExtraFlags(_flags | PATH_EXTRA_ALLOW_GROUND);

    if (canFly)
        _flags = PathExtraFlags(_flags | PATH_EXTRA_ALLOW_AIR_SHORTCUTS | PATH_EXTRA_FLYING);

    if (canSwim)
        _flags = PathExtraFlags(_flags | PATH_EXTRA_ALLOW_WATER_SHORTCUTS);

}

void PathGenerator::CreateFilter()
{
    uint16 includeFlags = 0;
    uint16 excludeFlags = 0;

    if (_sourceUnit->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = (Creature*)_sourceUnit;
        if (creature->CanWalk())
            includeFlags |= NAV_GROUND;          // walk

        // creatures don't take environmental damage
        if (creature->CanSwim())
            includeFlags |= (NAV_WATER | NAV_MAGMA | NAV_SLIME);           // swim
    }
    else // assume Player
    {
        // perfect support not possible, just stay 'safe'
        includeFlags |= (NAV_GROUND | NAV_WATER | NAV_MAGMA | NAV_SLIME);
    }

    _filter.setIncludeFlags(includeFlags);
    _filter.setExcludeFlags(excludeFlags);

    UpdateFilter();
}

void PathGenerator::UpdateFilter()
{
    // allow creatures to cheat and use different movement types if they are moved
    // forcefully into terrain they can't normally move in
    if (_sourceUnit->IsInWater() || _sourceUnit->IsUnderWater())
    {
        uint16 includedFlags = _filter.getIncludeFlags();
        includedFlags |= GetNavTerrain(_sourceUnit->GetPositionX(),
                                       _sourceUnit->GetPositionY(),
                                       _sourceUnit->GetPositionZ());

        _filter.setIncludeFlags(includedFlags);
    }
}

NavTerrain PathGenerator::GetNavTerrain(float x, float y, float z)
{
    LiquidData data;
    ZLiquidStatus liquidStatus = _sourceUnit->GetBaseMap()->getLiquidStatus(x, y, z, MAP_ALL_LIQUIDS, &data);
    if (liquidStatus == LIQUID_MAP_NO_WATER)
        return NAV_GROUND;

    switch (data.type_flags)
    {
        case MAP_LIQUID_TYPE_WATER:
        case MAP_LIQUID_TYPE_OCEAN:
            return NAV_WATER;
        case MAP_LIQUID_TYPE_MAGMA:
            return NAV_MAGMA;
        case MAP_LIQUID_TYPE_SLIME:
            return NAV_SLIME;
        default:
            return NAV_GROUND;
    }
}

bool PathGenerator::HaveTile(const G3D::Vector3& p) const
{
    if (_transport)
        return true;

    int tx = -1, ty = -1;
    float point[VERTEX_SIZE] = {p.y, p.z, p.x};

    _navMesh->calcTileLoc(point, &tx, &ty);

    /// Workaround
    /// For some reason, often the tx and ty variables wont get a valid value
    /// Use this check to prevent getting negative tile coords and crashing on getTileAt
    if (tx < 0 || ty < 0)
        return false;

    return (_navMesh->getTileAt(tx, ty, 0) != NULL);
}

// Client tolerance for objects above terrain to be placed on the ground
const float SMOOTH_LOW_THRESHOLD = 0.2f;
// Client tolerance for objects under terrain to be placed on the ground (actual value ~0,5415)
const float SMOOTH_HIGH_THRESHOLD = 0.4f;

dtStatus PathGenerator::FindSmoothPath(float const* startPos, float const* endPos,
                                     dtPolyRef const* polyPath, uint32 polyPathSize,
                                     float* smoothPath, int* smoothPathSize, uint32 maxSmoothPathSize)
{
    const float stepSize = sWorld->getFloatConfig(CONFIG_PERFORMANCE_PATHFINDING_SMOOTH_PATH_STEP_SIZE);

    dtPolyRef polys[MAX_PATH_LENGTH];
    memcpy(polys, polyPath, sizeof(dtPolyRef)*polyPathSize);
    uint32 npolys = polyPathSize;

    uint32 pointCount;
    float pathPoints[MAX_POINT_PATH_LENGTH*VERTEX_SIZE];

    dtStatus dtResult = _navMeshQuery->findStraightPath(
        startPos,         // start position
        endPos,           // end position
        _pathPolyRefs,     // current path
        _polyLength,       // lenth of current path
        pathPoints,         // [out] path corner points
        NULL,               // [out] flags
        NULL,               // [out] shortened path
        (int*)&pointCount,
        _pointPathLimit);   // maximum number of points/polygons to use

    float iterPos[VERTEX_SIZE], targetPos[VERTEX_SIZE];
    if (dtStatusFailed(_navMeshQuery->closestPointOnPolyBoundary(polys[0], startPos, iterPos)))
        return DT_FAILURE;

    if (dtStatusFailed(_navMeshQuery->closestPointOnPolyBoundary(polys[npolys - 1], endPos, targetPos)))
        return DT_FAILURE;

    uint32 nsmoothPath = 0;
    dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], iterPos);
    nsmoothPath++;

    uint32 currentPoint = 1;

    dtPolyRef currentPoly = polys[0];

    while (currentPoint < pointCount && nsmoothPath < maxSmoothPathSize)
    {
        float delta[VERTEX_SIZE];
        dtVsub(delta, &pathPoints[currentPoint*VERTEX_SIZE], &pathPoints[(currentPoint - 1)*VERTEX_SIZE]);

        // float len = dtMathSqrtf(dtVdot(delta, delta));
        const float xyDelta = std::sqrt(delta[2] * delta[2] + delta[0] * delta[0]);
        const uint32 stepCount = uint32(std::floor(xyDelta / stepSize));
        dtVscale(delta, delta, stepSize / xyDelta);

        uint32 segmentLength = 0;
        uint32 maxSlopePos = 1;
        uint32 minSlopePos = 1;
        float maxSlope = -std::numeric_limits<float>::infinity();
        float maxSlopeHeight = -std::numeric_limits<float>::infinity();
        float minSlope = std::numeric_limits<float>::infinity();
        float minSlopeHeight = std::numeric_limits<float>::infinity();

        float lastTgt[VERTEX_SIZE];
        dtVcopy(lastTgt, iterPos);

        for (uint32 i = 0; i < stepCount && nsmoothPath < maxSmoothPathSize; i++)
        {
            segmentLength++;
            float moveTgt[VERTEX_SIZE];
            if (i < stepCount)
                dtVadd(moveTgt, lastTgt, delta);
            else
                dtVcopy(moveTgt, &pathPoints[currentPoint*VERTEX_SIZE]);

            float result[VERTEX_SIZE];
            const static uint32 MAX_VISIT_POLY = 16;
            dtPolyRef visited[MAX_VISIT_POLY];

            uint32 nvisited = 0;
            _navMeshQuery->moveAlongSurface(currentPoly, iterPos, moveTgt, &_filter, result, visited, (int*)&nvisited, MAX_VISIT_POLY);

            if (nvisited > 0)
                currentPoly = visited[nvisited - 1];

            _navMeshQuery->getPolyHeight(currentPoly, result, &result[1]);
            // add tolerance because of mmap height inaccuracy
            result[1] += 0.5f;
            dtVcopy(moveTgt, result);

            float mapHeight;
            if (_transport)
                mapHeight = _transport->GetModelCollisionHeight(moveTgt[2], moveTgt[0], moveTgt[1]).first;
            else
                mapHeight = _sourceUnit->GetBaseMap()->GetHeight(moveTgt[2], moveTgt[0], moveTgt[1]);
            // map/map height is more accurate in general, but we have to check if our height is on the same level as our detour
            if (mapHeight > VMAP_INVALID_HEIGHT && std::abs(moveTgt[1] - mapHeight) < 7.f)
                moveTgt[1] = mapHeight;
            else
                moveTgt[1] -= 0.5f;

            float slope = (moveTgt[1] - iterPos[1]) / (stepSize * segmentLength);

            const float heightAtMaxSlopePos = iterPos[1] + slope * stepSize * maxSlopePos;
            const float heightAtMinSlopePos = iterPos[1] + slope * stepSize * minSlopePos;

            if (heightAtMaxSlopePos <= maxSlopeHeight || heightAtMinSlopePos >= minSlopeHeight)
            {
                dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], lastTgt);
                nsmoothPath++;

                dtVcopy(iterPos, lastTgt);

                slope = (moveTgt[1] - iterPos[1]) / xyDelta;

                maxSlope = minSlope = slope;
                maxSlopeHeight = moveTgt[1] - SMOOTH_LOW_THRESHOLD;
                minSlopeHeight = moveTgt[1] + SMOOTH_HIGH_THRESHOLD;
                maxSlopePos = minSlopePos = segmentLength = 1;
            }
            else
            {
                if (maxSlope < slope)
                {
                    maxSlope = slope;
                    maxSlopeHeight = moveTgt[1] - SMOOTH_LOW_THRESHOLD;
                    maxSlopePos = segmentLength;
                }
                if (minSlope > slope)
                {
                    minSlope = slope;
                    minSlopeHeight = moveTgt[1] + SMOOTH_HIGH_THRESHOLD;
                    minSlopePos = segmentLength;
                }
            }

            dtVcopy(lastTgt, moveTgt);
        }

        if (nsmoothPath < maxSmoothPathSize)
        {
            if (dtVdist(&smoothPath[(nsmoothPath - 1)*VERTEX_SIZE], lastTgt) < 1.0 && nsmoothPath > 1)
                dtVcopy(&smoothPath[(nsmoothPath - 1)*VERTEX_SIZE], lastTgt);
            else
            {
                dtVcopy(&smoothPath[nsmoothPath*VERTEX_SIZE], lastTgt);
                nsmoothPath++;
            }
            dtVcopy(iterPos, &pathPoints[currentPoint*VERTEX_SIZE]);
        }
        currentPoint++;
    }

    *smoothPathSize = nsmoothPath;

    // this is most likely a loop
    return nsmoothPath < MAX_POINT_PATH_LENGTH ? DT_SUCCESS : DT_FAILURE;
}

bool PathGenerator::InRange(G3D::Vector3 const& p1, G3D::Vector3 const& p2, float r, float h) const
{
    G3D::Vector3 d = p1 - p2;
    return (d.x * d.x + d.y * d.y) < r * r && fabsf(d.z) < h;
}

float PathGenerator::Dist3DSqr(G3D::Vector3 const& p1, G3D::Vector3 const& p2) const
{
    return (p1 - p2).squaredLength();
}

void PathGenerator::ReducePathLenghtByDist(float dist)
{
    if (GetPathType() == PATHFIND_BLANK)
    {
        TC_LOG_ERROR("maps", "PathGenerator::ReducePathLenghtByDist called before path was built");
        return;
    }

    if (_pathPoints.size() < 2) // path building failure
        return;

    uint32 i = _pathPoints.size();
    G3D::Vector3 nextVec = _pathPoints[--i];
    while (i > 0)
    {
        G3D::Vector3 currVec = _pathPoints[--i];
        G3D::Vector3 diffVec = (nextVec - currVec);
        float len = diffVec.length();
        if (len > dist)
        {
            float step = dist / len;
            // same as nextVec
            _pathPoints[i + 1] -= diffVec * step;
            _sourceUnit->UpdateAllowedPositionZ(_pathPoints[i + 1].x, _pathPoints[i + 1].y, _pathPoints[i + 1].z);
            _pathPoints.resize(i + 2);
            break;
        }
        else if (i == 0) // at second point
        {
            _pathPoints[1] = _pathPoints[0];
            _pathPoints.resize(2);
            break;
        }

        dist -= len;
        nextVec = currVec; // we're going backwards
    }

    SetEndPosition(_pathPoints[_pathPoints.size() - 1]);
}

void PathGenerator::OverrideEndPosition(Position const& endPosition)
{
    _pathPoints[_pathPoints.size() - 1] = G3D::Vector3(endPosition.GetPositionX(), endPosition.GetPositionY(), endPosition.GetPositionZ());
    SetEndPosition(_pathPoints[_pathPoints.size() - 1]);
}

bool PathGenerator::Find3dPath(G3D::Vector3 const &startPos, G3D::Vector3 const &endPos, Movement::PointsArray& outPath)
{
    const float stepSize = sWorld->getFloatConfig(CONFIG_PERFORMANCE_PATHFINDING_SMOOTH_PATH_STEP_SIZE);

    outPath.clear();
    outPath.push_back(startPos);

    G3D::Vector3 direction;
    int stepCount;

    // this flag will be set, when reaching water surface
    bool ignoreZDiff = false;

    const auto CheckNearEndPos = [this, &endPos](G3D::Vector3 const &actualEndPos) { return (actualEndPos - endPos).length() < _sourceUnit->GetCombatReach(); };

    const auto UpdateDirection = [&stepCount, &direction, &ignoreZDiff, &endPos, &outPath, &stepSize](G3D::Vector3 const &startPos)
    {
        G3D::Vector2 xyDirection(endPos.x - startPos.x, endPos.y - startPos.y);
        if (xyDirection.isZero())
        {
            outPath.push_back(startPos);
            return false;
        }

        float length = xyDirection.length();
        direction = G3D::Vector3(xyDirection, endPos.z - startPos.z);
        direction *= stepSize / length;
        stepCount = int(std::ceil(length / stepSize));

        if (ignoreZDiff)
            direction.z = 0;

        return true;
    };

    if (!UpdateDirection(startPos))
        return false;

    G3D::Vector3 currentPosition = startPos;
    G3D::Vector3 lastPosition;
    do
    {
        lastPosition = currentPosition;
        if (stepCount == 1)
            currentPosition = endPos;
        else
            currentPosition += direction;

        const float minHeight = _sourceUnit->GetMap()->GetHeight(currentPosition.x, currentPosition.y, currentPosition.z);
        float maxHeight = std::numeric_limits<float>::infinity();
        if (!HasPathFlag(PATH_EXTRA_ALLOW_AIR_SHORTCUTS))
            maxHeight = _sourceUnit->GetMap()->GetWaterLevel(currentPosition.x, currentPosition.y) - _sourceUnit->GetCombatReach();

        if (maxHeight < minHeight)
        {
            if (*outPath.rbegin() != lastPosition)
                outPath.push_back(lastPosition);
            return false;
        }

        bool insertPosition = false;
        if (maxHeight < currentPosition.z)
        {
            currentPosition.z = maxHeight;
            ignoreZDiff = true;
            insertPosition = true;
            if (!UpdateDirection(currentPosition))
                return CheckNearEndPos(currentPosition);
        }

        if (!insertPosition && minHeight > currentPosition.z)
        {
            currentPosition.z = minHeight;
            insertPosition = true;
            if (!UpdateDirection(currentPosition))
                return CheckNearEndPos(currentPosition);
        }

        if (insertPosition)
            outPath.push_back(currentPosition);
    } while (--stepCount > 0);

    outPath.push_back(endPos);

    return true;
}
