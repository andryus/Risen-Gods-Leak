/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
* @file cs_mmaps.cpp
* @brief .mmap related commands
*
* This file contains the CommandScripts for all
* mmap sub-commands
*/

#include "ScriptMgr.h"
#include "Chat.h"
#include "Chat/Command/Commands/LegacyCommand.hpp"
#include "DisableMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PointMovementGenerator.h"
#include "PathGenerator.h"
#include "MMapFactory.h"
#include "Map.h"
#include "TargetedMovementGenerator.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

namespace chat { namespace command { namespace handler {

    bool HandleMmapPathCommand(ChatHandler* handler, char const* args)
    {
        if (!MMAP::MMapFactory::createOrGetMMapManager()->GetNavMesh(handler->GetSession()->GetPlayer()->GetMapId()))
        {
            handler->PSendSysMessage("NavMesh not loaded for current map.");
            return true;
        }

        handler->PSendSysMessage("mmap path:");

        // units
        Player* player = handler->GetSession()->GetPlayer();
        Unit* target = handler->getSelectedUnit();
        if (!player || !target)
        {
            handler->PSendSysMessage("Invalid target/source selection.");
            return true;
        }

        // unit locations
        float x, y, z;
        player->GetPosition(x, y, z);

        // path
        PathGenerator path(target);
        bool result = path.CalculatePath(x, y, z);

        Movement::PointsArray const& pointPath = path.GetPath();
        handler->PSendSysMessage("%s's path to %s:", target->GetName().c_str(), player->GetName().c_str());
        handler->PSendSysMessage("Building: %s", "StraightPath");
        handler->PSendSysMessage("Result: %s - Length: %zu - Type: %u", (result ? "true" : "false"), pointPath.size(), path.GetPathType());

        G3D::Vector3 const &start = path.GetStartPosition();
        G3D::Vector3 const &end = path.GetEndPosition();
        G3D::Vector3 const &actualEnd = path.GetActualEndPosition();

        handler->PSendSysMessage("StartPosition     (%.3f, %.3f, %.3f)", start.x, start.y, start.z);
        handler->PSendSysMessage("EndPosition       (%.3f, %.3f, %.3f)", end.x, end.y, end.z);
        handler->PSendSysMessage("ActualEndPosition (%.3f, %.3f, %.3f)", actualEnd.x, actualEnd.y, actualEnd.z);

        if (!player->IsGameMaster())
            handler->PSendSysMessage("Enable GM mode to see the path points.");
        
        for (uint32 i = 0; i < pointPath.size(); ++i)
        {
            TempSummon* summon = player->SummonCreature(VISUAL_WAYPOINT, pointPath[i].x, pointPath[i].y, pointPath[i].z, 0, TEMPSUMMON_TIMED_DESPAWN, 9000);
            if (summon)
                summon->SetLevel(i + 1);
        }

        return true;
    }

    bool HandleMmapLocCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage("mmap tileloc:");

        // grid tile location
        Player* player = handler->GetSession()->GetPlayer();

        int32 gx = 32 - player->GetPositionX() / SIZE_OF_GRIDS;
        int32 gy = 32 - player->GetPositionY() / SIZE_OF_GRIDS;

        handler->PSendSysMessage("%03u%02i%02i.mmtile", player->GetMapId(), gx, gy);
        handler->PSendSysMessage("gridloc [%i, %i]", gy, gx);

        // calculate navmesh tile location
        dtNavMesh const* navmesh = MMAP::MMapFactory::createOrGetMMapManager()->GetNavMesh(player->GetMapId());
        dtNavMeshQuery const* navmeshquery = MMAP::MMapFactory::createOrGetMMapManager()->GetNavMeshQuery(player->GetMapId());
        if (!navmesh || !navmeshquery)
        {
            handler->PSendSysMessage("NavMesh not loaded for current map.");
            return true;
        }

        float const* min = navmesh->getParams()->orig;
        float x, y, z;
        player->GetPosition(x, y, z);
        float location[VERTEX_SIZE] = { y, z, x };
        float extents[VERTEX_SIZE] = { 3.0f, 5.0f, 3.0f };

        int32 tilex = int32((y - min[0]) / SIZE_OF_GRIDS);
        int32 tiley = int32((x - min[2]) / SIZE_OF_GRIDS);

        handler->PSendSysMessage("Calc   [%02i, %02i]", tilex, tiley);

        // navmesh poly -> navmesh tile location
        dtQueryFilter filter = dtQueryFilter();
        dtPolyRef polyRef = INVALID_POLYREF;
        if (dtStatusFailed(navmeshquery->findNearestPoly(location, extents, &filter, &polyRef, NULL)))
        {
            handler->PSendSysMessage("Dt     [??,??] (invalid poly, probably no tile loaded)");
            return true;
        }

        if (polyRef == INVALID_POLYREF)
            handler->PSendSysMessage("Dt     [??, ??] (invalid poly, probably no tile loaded)");
        else
        {
            dtMeshTile const* tile;
            dtPoly const* poly;
            if (dtStatusSucceed(navmesh->getTileAndPolyByRef(polyRef, &tile, &poly)))
            {
                if (tile)
                {
                    handler->PSendSysMessage("Dt     [%02i,%02i]", tile->header->x, tile->header->y);
                    unsigned int salt, it, ip;
                    navmesh->decodePolyId(polyRef, salt, it, ip);
                    handler->PSendSysMessage("Index: %u, Flag: %u", ip, poly->flags);
                    if (poly->flags & NAV_DYNAMIC)
                    {
                        handler->PSendSysMessage("Assigned game objects (Core GUID, Spawn ID, collision):");
                        PolyIdentifier pId = std::make_tuple(player->GetMapId(), tile->header->x, tile->header->y, ip);
                        Map* map = player->GetMap();
                        if (map)
                        {
                            if (std::vector<uint32> const* guids = sObjectMgr->GetPolygonAssignments(pId))
                            {
                                for (uint32 i = 0; i < guids->size(); ++i)
                                {
                                    auto optionalGO = map->GetGameObjectBySpawnId(guids->at(i));
                                    if (optionalGO)
                                    {
                                        GameObject& go = *optionalGO;
                                        bool collision = (go.IsInWorld() && go.isSpawned() && (go.GetPhaseMask() & player->GetPhaseMask()) && go.m_model && go.m_model->isEnabled());
                                        handler->PSendSysMessage("     %u, %u, %b", go.GetGUID().GetCounter(), guids->at(i), collision);
                                    }
                                }
                            }
                        }
                    }
                    handler->PSendSysMessage("Vertices:");
                    for (uint32 i = 0; i < poly->vertCount; ++i)
                    {
                        float* v = &(tile->verts[poly->verts[i]*3]);
                        handler->PSendSysMessage("     [%f,%f,%f]", v[2], v[0], v[1]);
                        player->SummonCreature(VISUAL_WAYPOINT, v[2] - 0.4f, v[0], v[1], 0, TEMPSUMMON_TIMED_DESPAWN, 9000);
                    }
                    return true;
                }
            }

            handler->PSendSysMessage("Dt     [??,??] (no tile loaded)");
        }

        return true;
    }

    bool HandleMmapLoadedTilesCommand(ChatHandler* handler, char const* /*args*/)
    {
        uint32 mapid = handler->GetSession()->GetPlayer()->GetMapId();
        dtNavMesh const* navmesh = MMAP::MMapFactory::createOrGetMMapManager()->GetNavMesh(mapid);
        dtNavMeshQuery const* navmeshquery = MMAP::MMapFactory::createOrGetMMapManager()->GetNavMeshQuery(mapid);
        if (!navmesh || !navmeshquery)
        {
            handler->PSendSysMessage("NavMesh not loaded for current map.");
            return true;
        }

        handler->PSendSysMessage("mmap loadedtiles:");

        for (int32 i = 0; i < navmesh->getMaxTiles(); ++i)
        {
            dtMeshTile const* tile = navmesh->getTile(i);
            if (!tile || !tile->header)
                continue;

            handler->PSendSysMessage("[%02i, %02i]", tile->header->x, tile->header->y);
        }

        return true;
    }

    bool HandleMmapStatsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Map* map = handler->GetSession()->GetPlayer()->GetMap();
        handler->PSendSysMessage("mmap stats:");
        handler->PSendSysMessage("  global mmap pathfinding is %sabled", DisableMgr::IsPathfindingEnabled(map->GetId()) ? "en" : "dis");

        MMAP::MMapManager* manager = MMAP::MMapFactory::createOrGetMMapManager();
        handler->PSendSysMessage(" %u maps loaded with %u tiles overall", manager->getLoadedMapsCount(), manager->getLoadedTilesCount());

        dtNavMesh const* navmesh = manager->GetNavMesh(map->GetId());
        if (!navmesh)
        {
            handler->PSendSysMessage("NavMesh not loaded for current map.");
            return true;
        }

        uint32 tileCount = 0;
        uint32 nodeCount = 0;
        uint32 polyCount = 0;
        uint32 vertCount = 0;
        uint32 triCount = 0;
        uint32 triVertCount = 0;
        uint32 dataSize = 0;
        for (int32 i = 0; i < navmesh->getMaxTiles(); ++i)
        {
            dtMeshTile const* tile = navmesh->getTile(i);
            if (!tile || !tile->header)
                continue;

            tileCount++;
            nodeCount += tile->header->bvNodeCount;
            polyCount += tile->header->polyCount;
            vertCount += tile->header->vertCount;
            triCount += tile->header->detailTriCount;
            triVertCount += tile->header->detailVertCount;
            dataSize += tile->dataSize;
        }

        handler->PSendSysMessage("Navmesh stats:");
        handler->PSendSysMessage(" %u tiles loaded", tileCount);
        handler->PSendSysMessage(" %u BVTree nodes", nodeCount);
        handler->PSendSysMessage(" %u polygons (%u vertices)", polyCount, vertCount);
        handler->PSendSysMessage(" %u triangles (%u vertices)", triCount, triVertCount);
        handler->PSendSysMessage(" %.2f MB of data (not including pointers)", ((float)dataSize / sizeof(unsigned char)) / 1048576);

        return true;
    }

    bool HandleMmapTestArea(ChatHandler* handler, char const* /*args*/)
    {
        float radius = 40.0f;
        WorldObject* object = handler->GetSession()->GetPlayer();
        
        std::list<Creature*> creatureList;

        Trinity::AnyUnitInObjectRangeCheck go_check(object, radius);

        // Get Creatures
        object->VisitAnyNearbyObject<Creature, Trinity::ContainerAction>(radius, creatureList, go_check);

        if (!creatureList.empty())
        {
            handler->PSendSysMessage("Found %zu Creatures.", creatureList.size());

            uint32 paths = 0;
            uint32 uStartTime = getMSTime();

            float gx, gy, gz;
            object->GetPosition(gx, gy, gz);
            for (std::list<Creature*>::iterator itr = creatureList.begin(); itr != creatureList.end(); ++itr)
            {
                PathGenerator path(*itr);
                path.CalculatePath(gx, gy, gz);
                ++paths;
            }

            uint32 uPathLoadTime = getMSTimeDiff(uStartTime, getMSTime());
            handler->PSendSysMessage("Generated %i paths in %i ms", paths, uPathLoadTime);
        }
        else
            handler->PSendSysMessage("No creatures in %f yard range.", radius);

        return true;
    }

}}}

void AddSC_mmaps_commandscript()
{
    using namespace chat::command;
    using namespace chat::command::handler;

    new LegacyCommandScript("cmd_mmap_loadedtiles",     HandleMmapLoadedTilesCommand,   false);
    new LegacyCommandScript("cmd_mmap_loc",             HandleMmapLocCommand,           false);
    new LegacyCommandScript("cmd_mmap_path",            HandleMmapPathCommand,          false);
    new LegacyCommandScript("cmd_mmap_stats",           HandleMmapStatsCommand,         false);
    new LegacyCommandScript("cmd_mmap_testarea",        HandleMmapTestArea,             false);
}
