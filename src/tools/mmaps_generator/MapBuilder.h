/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
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

#ifndef _MAP_BUILDER_H
#define _MAP_BUILDER_H

#include "TerrainBuilder.h"
#include "GeneratorConfig.hpp"

#include "Recast.h"
#include "DetourNavMesh.h"
#include "ProducerConsumerQueue.h"
#include "GameObjectModel.h"

#include <vector>
#include <set>
#include <list>
#include <atomic>
#include <thread>
#include <G3D/Vector3.h>
#include <G3D/AABox.h>
#include <unordered_map>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/index/rtree.hpp>

using namespace VMAP;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

BOOST_GEOMETRY_REGISTER_POINT_2D(G3D::Vector3, float, bg::cs::cartesian, x, z)
BOOST_GEOMETRY_REGISTER_BOX(G3D::AABox, G3D::Vector3, low(), high())

namespace MMAP
{
    struct MapTiles
    {
        MapTiles() : m_mapId(uint32(-1)), m_tiles(NULL) {}

        MapTiles(uint32 id, std::set<uint32>* tiles) : m_mapId(id), m_tiles(tiles) {}
        ~MapTiles() {}

        uint32 m_mapId;
        std::set<uint32>* m_tiles;

        bool operator==(uint32 id)
        {
            return m_mapId == id;
        }
    };

    struct GameObjectBounds
    {
        GameObjectBounds(uint32 guid, uint32 displayId, float height, float size, G3D::Vector3 iPos, G3D::Matrix3 rotation, bool isStatic)
            : guid(guid), displayId(displayId), height(height), size(size), iPos(iPos), rotation(rotation), isStatic(isStatic) { }

        uint32 guid;
        uint32 displayId;
        float height;
        float bounds[12];
        float size;
        G3D::Vector3 iPos;
        G3D::Matrix3 rotation;
        bool isStatic;
    };
    typedef bgi::rtree<std::pair<G3D::AABox, std::shared_ptr<GameObjectBounds>>, bgi::quadratic<16>> GameObjectTree;
    typedef std::pair<G3D::AABox, std::shared_ptr<GameObjectBounds>> GOTreeElement;

    typedef std::list<MapTiles> TileList;

    struct Tile
    {
        Tile() : chf(NULL), solid(NULL), cset(NULL), pmesh(NULL), dmesh(NULL) {}
        ~Tile()
        {
            rcFreeCompactHeightfield(chf);
            rcFreeContourSet(cset);
            rcFreeHeightField(solid);
            rcFreePolyMesh(pmesh);
            rcFreePolyMeshDetail(dmesh);
        }
        rcCompactHeightfield* chf;
        rcHeightfield* solid;
        rcContourSet* cset;
        rcPolyMesh* pmesh;
        rcPolyMeshDetail* dmesh;
    };

    class MapBuilder
    {
        public:
            MapBuilder(float maxWalkableAngle   = 70.f,
                bool skipLiquid                 = false,
                bool skipContinents             = false,
                bool skipJunkMaps               = true,
                bool skipBattlegrounds          = false,
                bool debugOutput                = false,
                bool bigBaseUnit                = false,
                const char* offMeshFilePath     = NULL,
                const char* mysqlHost           = "127.0.0.1",
                const char* mysqlUser           = "trinity",
                const char* mysqlPassword       = "trinity",
                const char* mysqlDB             = "world",
                unsigned int mysqlPort          = 0,
                std::string configPath          = "mmap.ini");

            ~MapBuilder();

            // builds all mmap tiles for the specified map id (ignores skip settings)
            void buildMap(uint32 mapID);
            void buildMeshFromFile(char* name);

            // builds an mmap tile for the specified map and its mesh
            void buildSingleTile(uint32 mapID, uint32 tileX, uint32 tileY);

            // builds list of maps, then builds all of mmap tiles (based on the skip settings)
            void buildAllMaps(int threads);
            void buildTransports();

            void WorkerThread();

            bool loadGameObjects();

        private:
            // detect maps and tiles
            void discoverTiles();
            std::set<uint32>* getTileList(uint32 mapID);

            void buildNavMesh(uint32 mapID, dtNavMesh* &navMesh);

            void buildTile(uint32 mapID, uint32 tileX, uint32 tileY, dtNavMesh* navMesh);

            // move map building
            void buildMoveMapTile(uint32 mapID,
                uint32 tileX,
                uint32 tileY,
                MeshData &meshData,
                float bmin[3],
                float bmax[3],
                dtNavMesh* navMesh);

            void getTileBounds(uint32 tileX, uint32 tileY,
                float* verts, int vertCount,
                float* bmin, float* bmax);
            void getGridBounds(uint32 mapID, uint32 &minX, uint32 &minY, uint32 &maxX, uint32 &maxY) const;

            bool shouldSkipMap(uint32 mapID);
            bool isTransportMap(uint32 mapID);
            bool shouldSkipTile(uint32 mapID, uint32 tileX, uint32 tileY);

            uint32 percentageDone(uint32 totalTiles, uint32 totalTilesDone);

            void buildGameObject(std::string model, uint32 displayId);

            void loadGameObjectModelList();

            TerrainBuilder* m_terrainBuilder;
            ConfigStorage configStore;
            TileList m_tiles;

            bool m_debugOutput;

            const char* m_offMeshFilePath;
            bool m_skipContinents;
            bool m_skipJunkMaps;
            bool m_skipBattlegrounds;

            float m_maxWalkableAngle;
            bool m_bigBaseUnit;

            std::atomic<uint32> m_totalTiles;
            std::atomic<uint32> m_totalTilesProcessed;

            // build performance - not really used for now
            rcContext* m_rcContext;

            std::vector<std::thread> _workerThreads;
            ProducerConsumerQueue<uint32> _queue;
            std::atomic<bool> _cancelationToken;

            std::unordered_map<uint32, GameObjectTree> gobjectTrees;
            ModelList modelList;

            const char* mysqlHost;
            const char* mysqlUser;
            const char* mysqlPassword;
            const char* mysqlDB;
            const unsigned int mysqlPort;
    };
}

#endif
