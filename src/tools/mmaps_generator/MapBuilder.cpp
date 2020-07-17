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

#include "PathCommon.h"
#include "MapBuilder.h"
#include "MapTree.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMesh.h"
#include "IntermediateValues.h"
#include "DetourCommon.h"
#include "VMapDefinitions.h"
#include "VMapManager2.h"

#include <limits.h>
#include <mysql.h>
#include <boost/function_output_iterator.hpp>
#include <G3D/Quat.h>

#define MMAP_MAGIC 0x4d4d4150   // 'MMAP'
#define MMAP_VERSION 6

struct MmapTileHeader
{
    uint32 mmapMagic;
    uint32 dtVersion;
    uint32 mmapVersion;
    uint32 size;
    char usesLiquids;
    char padding[3];

    MmapTileHeader() : mmapMagic(MMAP_MAGIC), dtVersion(DT_NAVMESH_VERSION),
        mmapVersion(MMAP_VERSION), size(0), usesLiquids(true), padding() {}
};

// All padding fields must be handled and initialized to ensure mmaps_generator will produce binary-identical *.mmtile files
static_assert(sizeof(MmapTileHeader) == 20, "MmapTileHeader size is not correct, adjust the padding field size");
static_assert(sizeof(MmapTileHeader) == (sizeof(MmapTileHeader::mmapMagic) +
                                         sizeof(MmapTileHeader::dtVersion) +
                                         sizeof(MmapTileHeader::mmapVersion) +
                                         sizeof(MmapTileHeader::size) +
                                         sizeof(MmapTileHeader::usesLiquids) +
                                         sizeof(MmapTileHeader::padding)), "MmapTileHeader has uninitialized padding fields");

namespace MMAP
{
    MapBuilder::MapBuilder(float maxWalkableAngle, bool skipLiquid,
        bool skipContinents, bool skipJunkMaps, bool skipBattlegrounds,
        bool debugOutput, bool bigBaseUnit, const char* offMeshFilePath,
        const char* mysqlHost, const char* mysqlUser, const char* mysqlPassword,
        const char* mysqlDB, unsigned int mysqlPort, std::string configPath) :
        m_terrainBuilder     (NULL),
        m_debugOutput        (debugOutput),
        m_offMeshFilePath    (offMeshFilePath),
        m_skipContinents     (skipContinents),
        m_skipJunkMaps       (skipJunkMaps),
        m_skipBattlegrounds  (skipBattlegrounds),
        m_maxWalkableAngle   (maxWalkableAngle),
        m_bigBaseUnit        (bigBaseUnit),
        m_totalTiles         (0u),
        m_totalTilesProcessed(0u),
        m_rcContext          (NULL),
        _cancelationToken    (false),
        mysqlHost            (mysqlHost),
        mysqlUser            (mysqlUser),
        mysqlPassword        (mysqlPassword),
        mysqlDB              (mysqlDB),
        mysqlPort            (mysqlPort),
        configStore          (configPath, maxWalkableAngle, bigBaseUnit)
    {
        m_terrainBuilder = new TerrainBuilder(skipLiquid);

        m_rcContext = new rcContext(false);

        discoverTiles();
    }

    /**************************************************************************/
    MapBuilder::~MapBuilder()
    {
        for (TileList::iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
        {
            (*it).m_tiles->clear();
            delete (*it).m_tiles;
        }

        delete m_terrainBuilder;
        delete m_rcContext;
    }

    /**************************************************************************/
    void MapBuilder::discoverTiles()
    {
        std::vector<std::string> files;
        uint32 mapID, tileX, tileY, tileID, count = 0;
        char filter[12];

        printf("Discovering maps... ");
        getDirContents(files, "maps");
        for (uint32 i = 0; i < files.size(); ++i)
        {
            mapID = uint32(atoi(files[i].substr(0,3).c_str()));
            if (std::find(m_tiles.begin(), m_tiles.end(), mapID) == m_tiles.end())
            {
                m_tiles.emplace_back(MapTiles(mapID, new std::set<uint32>));
                count++;
            }
        }

        files.clear();
        getDirContents(files, "vmaps", "*.vmtree");
        for (uint32 i = 0; i < files.size(); ++i)
        {
            mapID = uint32(atoi(files[i].substr(0,3).c_str()));
            if (std::find(m_tiles.begin(), m_tiles.end(), mapID) == m_tiles.end())
            {
                m_tiles.emplace_back(MapTiles(mapID, new std::set<uint32>));
                count++;
            }
        }
        printf("found %u.\n", count);

        count = 0;
        printf("Discovering tiles... ");
        for (TileList::iterator itr = m_tiles.begin(); itr != m_tiles.end(); ++itr)
        {
            std::set<uint32>* tiles = (*itr).m_tiles;
            mapID = (*itr).m_mapId;

            sprintf(filter, "%03u*.vmtile", mapID);
            files.clear();
            getDirContents(files, "vmaps", filter);
            for (uint32 i = 0; i < files.size(); ++i)
            {
                tileX = uint32(atoi(files[i].substr(7,2).c_str()));
                tileY = uint32(atoi(files[i].substr(4,2).c_str()));
                tileID = StaticMapTree::packTileID(tileY, tileX);

                tiles->insert(tileID);
                count++;
            }

            sprintf(filter, "%03u*", mapID);
            files.clear();
            getDirContents(files, "maps", filter);
            for (uint32 i = 0; i < files.size(); ++i)
            {
                tileY = uint32(atoi(files[i].substr(3,2).c_str()));
                tileX = uint32(atoi(files[i].substr(5,2).c_str()));
                tileID = StaticMapTree::packTileID(tileX, tileY);

                if (tiles->insert(tileID).second)
                    count++;
            }
        }
        printf("found %u.\n", count);

        m_totalTiles.store(count, std::memory_order_relaxed);
    }

    /**************************************************************************/
    std::set<uint32>* MapBuilder::getTileList(uint32 mapID)
    {
        TileList::iterator itr = std::find(m_tiles.begin(), m_tiles.end(), mapID);
        if (itr != m_tiles.end())
            return (*itr).m_tiles;

        std::set<uint32>* tiles = new std::set<uint32>();
        m_tiles.emplace_back(MapTiles(mapID, tiles));
        return tiles;
    }

    /**************************************************************************/

    void MapBuilder::WorkerThread()
    {
        while (1)
        {
            uint32 mapId = 0;

            _queue.WaitAndPop(mapId);

            if (_cancelationToken)
                return;

            buildMap(mapId);
        }
    }

    void MapBuilder::buildAllMaps(int threads)
    {
        for (int i = 0; i < threads; ++i)
        {
            _workerThreads.push_back(std::thread(&MapBuilder::WorkerThread, this));
        }

        m_tiles.sort([](MapTiles a, MapTiles b)
        {
            return a.m_tiles->size() > b.m_tiles->size();
        });

        for (TileList::iterator it = m_tiles.begin(); it != m_tiles.end(); ++it)
        {
            uint32 mapId = it->m_mapId;
            if (!shouldSkipMap(mapId))
            {
                if (threads > 0)
                    _queue.Push(mapId);
                else
                    buildMap(mapId);
            }
        }

        while (!_queue.Empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        _cancelationToken = true;

        _queue.Cancel();

        for (auto& thread : _workerThreads)
        {
            thread.join();
        }
    }

    /**************************************************************************/
    void MapBuilder::getGridBounds(uint32 mapID, uint32 &minX, uint32 &minY, uint32 &maxX, uint32 &maxY) const
    {
        // min and max are initialized to invalid values so the caller iterating the [min, max] range
        // will never enter the loop unless valid min/max values are found
        maxX = 0;
        maxY = 0;
        minX = std::numeric_limits<uint32>::max();
        minY = std::numeric_limits<uint32>::max();

        float bmin[3] = { 0, 0, 0 };
        float bmax[3] = { 0, 0, 0 };
        float lmin[3] = { 0, 0, 0 };
        float lmax[3] = { 0, 0, 0 };
        MeshData meshData;

        // make sure we process maps which don't have tiles
        // initialize the static tree, which loads WDT models
        if (!m_terrainBuilder->loadVMap(mapID, 64, 64, meshData))
            return;

        // get the coord bounds of the model data
        if (meshData.solidVerts.size() + meshData.liquidVerts.size() == 0)
            return;

        // get the coord bounds of the model data
        if (meshData.solidVerts.size() && meshData.liquidVerts.size())
        {
            rcCalcBounds(meshData.solidVerts.getCArray(), meshData.solidVerts.size() / 3, bmin, bmax);
            rcCalcBounds(meshData.liquidVerts.getCArray(), meshData.liquidVerts.size() / 3, lmin, lmax);
            rcVmin(bmin, lmin);
            rcVmax(bmax, lmax);
        }
        else if (meshData.solidVerts.size())
            rcCalcBounds(meshData.solidVerts.getCArray(), meshData.solidVerts.size() / 3, bmin, bmax);
        else
            rcCalcBounds(meshData.liquidVerts.getCArray(), meshData.liquidVerts.size() / 3, lmin, lmax);

        // convert coord bounds to grid bounds
        maxX = 32 - bmin[0] / GRID_SIZE;
        maxY = 32 - bmin[2] / GRID_SIZE;
        minX = 32 - bmax[0] / GRID_SIZE;
        minY = 32 - bmax[2] / GRID_SIZE;
    }

    void MapBuilder::buildMeshFromFile(char* name)
    {
        FILE* file = fopen(name, "rb");
        if (!file)
            return;

        printf("Building mesh from file\n");
        int tileX, tileY, mapId;
        if (fread(&mapId, sizeof(int), 1, file) != 1)
        {
            fclose(file);
            return;
        }
        if (fread(&tileX, sizeof(int), 1, file) != 1)
        {
            fclose(file);
            return;
        }
        if (fread(&tileY, sizeof(int), 1, file) != 1)
        {
            fclose(file);
            return;
        }

        dtNavMesh* navMesh = NULL;
        buildNavMesh(mapId, navMesh);
        if (!navMesh)
        {
            printf("Failed creating navmesh!              \n");
            fclose(file);
            return;
        }

        uint32 verticesCount, indicesCount;
        if (fread(&verticesCount, sizeof(uint32), 1, file) != 1)
        {
            fclose(file);
            return;
        }

        if (fread(&indicesCount, sizeof(uint32), 1, file) != 1)
        {
            fclose(file);
            return;
        }

        float* verts = new float[verticesCount];
        int* inds = new int[indicesCount];

        if (fread(verts, sizeof(float), verticesCount, file) != verticesCount)
        {
            fclose(file);
            delete[] verts;
            delete[] inds;
            return;
        }

        if (fread(inds, sizeof(int), indicesCount, file) != indicesCount)
        {
            fclose(file);
            delete[] verts;
            delete[] inds;
            return;
        }

        MeshData data;

        for (uint32 i = 0; i < verticesCount; ++i)
            data.solidVerts.append(verts[i]);
        delete[] verts;

        for (uint32 i = 0; i < indicesCount; ++i)
            data.solidTris.append(inds[i]);
        delete[] inds;

        TerrainBuilder::cleanVertices(data.solidVerts, data.solidTris);
        // get bounds of current tile
        float bmin[3], bmax[3];
        getTileBounds(tileX, tileY, data.solidVerts.getCArray(), data.solidVerts.size() / 3, bmin, bmax);

        // build navmesh tile
        buildMoveMapTile(mapId, tileX, tileY, data, bmin, bmax, navMesh);
        fclose(file);
    }

    /**************************************************************************/
    void MapBuilder::buildSingleTile(uint32 mapID, uint32 tileX, uint32 tileY)
    {
        dtNavMesh* navMesh = NULL;
        buildNavMesh(mapID, navMesh);
        if (!navMesh)
        {
            printf("Failed creating navmesh!              \n");
            return;
        }

        buildTile(mapID, tileX, tileY, navMesh);
        dtFreeNavMesh(navMesh);
    }

    /**************************************************************************/
    void MapBuilder::buildMap(uint32 mapID)
    {
        std::set<uint32>* tiles = getTileList(mapID);

        // make sure we process maps which don't have tiles
        if (!tiles->size())
        {
            // convert coord bounds to grid bounds
            uint32 minX, minY, maxX, maxY;
            getGridBounds(mapID, minX, minY, maxX, maxY);

            // add all tiles within bounds to tile list.
            for (uint32 i = minX; i <= maxX; ++i)
                for (uint32 j = minY; j <= maxY; ++j)
                    if (tiles->insert(StaticMapTree::packTileID(i, j)).second)
                        ++m_totalTiles;
        }

        if (!tiles->empty())
        {
            // build navMesh
            dtNavMesh* navMesh = NULL;
            buildNavMesh(mapID, navMesh);
            if (!navMesh)
            {
                printf("[Map %03i] Failed creating navmesh!\n", mapID);
                m_totalTilesProcessed += tiles->size();
                return;
            }

            // now start building mmtiles for each tile
            printf("[Map %03i] We have %u tiles.                          \n", mapID, (unsigned int)tiles->size());
            for (std::set<uint32>::iterator it = tiles->begin(); it != tiles->end(); ++it)
            {
                uint32 tileX, tileY;

                // unpack tile coords
                StaticMapTree::unpackTileID((*it), tileX, tileY);

                ++m_totalTilesProcessed;
                if (shouldSkipTile(mapID, tileX, tileY))
                    continue;

                buildTile(mapID, tileX, tileY, navMesh);
            }

            dtFreeNavMesh(navMesh);
        }

        printf("[Map %03i] Complete!\n", mapID);
    }

    /**************************************************************************/
    void MapBuilder::buildTile(uint32 mapID, uint32 tileX, uint32 tileY, dtNavMesh* navMesh)
    {
        printf("%u%% [Map %03i] Building tile [%02u,%02u]\n", percentageDone(m_totalTiles, m_totalTilesProcessed), mapID, tileX, tileY);

        MeshData meshData;

        // get heightmap data
        m_terrainBuilder->loadMap(mapID, tileX, tileY, meshData);

        // get model data
        m_terrainBuilder->loadVMap(mapID, tileY, tileX, meshData);

        // add static game objects to mesh
        GameObjectTree& tree = gobjectTrees[mapID];
        float xh = (32 - int(tileX)) * GRID_SIZE;
        float zh = (32 - int(tileY)) * GRID_SIZE;
        VMapManager2 vmapManager;
        G3D::AABox tileBox(G3D::Vector3(xh - GRID_SIZE, 0.f, zh - GRID_SIZE), G3D::Vector3(xh, 1.f, zh));
        auto itr = boost::make_function_output_iterator([&](auto const& val)
        {
            std::shared_ptr<GameObjectBounds> data = val.second;
            if (data->isStatic)
            {
                auto it = modelList.find(data->displayId);
                if (it != modelList.end())
                {
                    WorldModel* model = vmapManager.acquireModelInstance("vmaps/", it->second.name);
                    bool isM2 = it->second.name.find(".m2") != std::string::npos || it->second.name.find(".M2") != std::string::npos;
                    m_terrainBuilder->loadStaticGameObject(*data, *model, meshData, isM2);
                }
            }
        });
        tree.query(bgi::intersects(tileBox), itr);

        // if there is no data, give up now
        if (!meshData.solidVerts.size() && !meshData.liquidVerts.size())
            return;

        // remove unused vertices
        TerrainBuilder::cleanVertices(meshData.solidVerts, meshData.solidTris);
        TerrainBuilder::cleanVertices(meshData.liquidVerts, meshData.liquidTris);

        // gather all mesh data for final data check, and bounds calculation
        G3D::Array<float> allVerts;
        allVerts.append(meshData.liquidVerts);
        allVerts.append(meshData.solidVerts);

        if (!allVerts.size())
            return;

        // get bounds of current tile
        float bmin[3], bmax[3];
        getTileBounds(tileX, tileY, allVerts.getCArray(), allVerts.size() / 3, bmin, bmax);

        m_terrainBuilder->loadOffMeshConnections(mapID, tileX, tileY, meshData, m_offMeshFilePath);

        // build navmesh tile
        buildMoveMapTile(mapID, tileX, tileY, meshData, bmin, bmax, navMesh);
    }

    /**************************************************************************/
    void MapBuilder::buildNavMesh(uint32 mapID, dtNavMesh* &navMesh)
    {
        std::set<uint32>* tiles = getTileList(mapID);

        // old code for non-statically assigned bitmask sizes:
        ///*** calculate number of bits needed to store tiles & polys ***/
        //int tileBits = dtIlog2(dtNextPow2(tiles->size()));
        //if (tileBits < 1) tileBits = 1;                                     // need at least one bit!
        //int polyBits = sizeof(dtPolyRef)*8 - SALT_MIN_BITS - tileBits;

        int polyBits = DT_POLY_BITS;

        int maxTiles = tiles->size();
        int maxPolysPerTile = 1 << polyBits;

        /***          calculate bounds of map         ***/

        uint32 tileXMin = 64, tileYMin = 64, tileXMax = 0, tileYMax = 0, tileX, tileY;
        for (std::set<uint32>::iterator it = tiles->begin(); it != tiles->end(); ++it)
        {
            StaticMapTree::unpackTileID(*it, tileX, tileY);

            if (tileX > tileXMax)
                tileXMax = tileX;
            else if (tileX < tileXMin)
                tileXMin = tileX;

            if (tileY > tileYMax)
                tileYMax = tileY;
            else if (tileY < tileYMin)
                tileYMin = tileY;
        }

        // use Max because '32 - tileX' is negative for values over 32
        float bmin[3], bmax[3];
        getTileBounds(tileXMax, tileYMax, NULL, 0, bmin, bmax);

        /***       now create the navmesh       ***/

        // navmesh creation params
        dtNavMeshParams navMeshParams;
        memset(&navMeshParams, 0, sizeof(dtNavMeshParams));
        navMeshParams.tileWidth = GRID_SIZE;
        navMeshParams.tileHeight = GRID_SIZE;
        rcVcopy(navMeshParams.orig, bmin);
        navMeshParams.maxTiles = maxTiles;
        navMeshParams.maxPolys = maxPolysPerTile;

        navMesh = dtAllocNavMesh();
        printf("[Map %03i] Creating navMesh...\n", mapID);
        if (!navMesh->init(&navMeshParams))
        {
            printf("[Map %03i] Failed creating navmesh!                \n", mapID);
            return;
        }

        char fileName[25];
        sprintf(fileName, "mmaps/%03u.mmap", mapID);

        FILE* file = fopen(fileName, "wb");
        if (!file)
        {
            dtFreeNavMesh(navMesh);
            char message[1024];
            sprintf(message, "[Map %03i] Failed to open %s for writing!\n", mapID, fileName);
            perror(message);
            return;
        }

        // now that we know navMesh params are valid, we can write them to file
        fwrite(&navMeshParams, sizeof(dtNavMeshParams), 1, file);
        fclose(file);
    }

    /**************************************************************************/
    void MapBuilder::buildMoveMapTile(uint32 mapID, uint32 tileX, uint32 tileY,
        MeshData &meshData, float bmin[3], float bmax[3],
        dtNavMesh* navMesh)
    {
        // console output
        char tileString[20];
        sprintf(tileString, "[Map %03i] [%02i,%02i]: ", mapID, tileX, tileY);
        printf("%s Building movemap tiles...\n", tileString);

        IntermediateValues iv;

        float* tVerts = meshData.solidVerts.getCArray();
        int tVertCount = meshData.solidVerts.size() / 3;
        int* tTris = meshData.solidTris.getCArray();
        int tTriCount = meshData.solidTris.size() / 3;

        float* lVerts = meshData.liquidVerts.getCArray();
        int lVertCount = meshData.liquidVerts.size() / 3;
        int* lTris = meshData.liquidTris.getCArray();
        int lTriCount = meshData.liquidTris.size() / 3;
        uint8* lTriFlags = meshData.liquidType.getCArray();

        // these are WORLD UNIT based metrics
        // this are basic unit dimentions
        // value have to divide GRID_SIZE(533.3333f) ( aka: 0.5333, 0.2666, 0.3333, 0.1333, etc )
        const static float BASE_UNIT_DIM = m_bigBaseUnit ? 0.5333333f : 0.2666666f;

        // All are in UNIT metrics!
        const static int VERTEX_PER_MAP = int(MMAP::GRID_SIZE / BASE_UNIT_DIM + 0.5f);
        const static int VERTEX_PER_TILE = m_bigBaseUnit ? 40 : 80; // must divide VERTEX_PER_MAP
        const static int TILES_PER_MAP = VERTEX_PER_MAP/VERTEX_PER_TILE;

        rcConfig config = configStore.GetConfig(mapID).GenerateConfig(m_bigBaseUnit, bmin, bmax);

        // this sets the dimensions of the heightfield - should maybe happen before border padding
        rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

        // allocate subregions : tiles
        Tile* tiles = new Tile[TILES_PER_MAP * TILES_PER_MAP];

        // Initialize per tile config.
        rcConfig tileCfg = config;
        tileCfg.width = config.tileSize + config.borderSize*2;
        tileCfg.height = config.tileSize + config.borderSize*2;

        // merge per tile poly and detail meshes
        rcPolyMesh** pmmerge = new rcPolyMesh*[TILES_PER_MAP * TILES_PER_MAP];
        rcPolyMeshDetail** dmmerge = new rcPolyMeshDetail*[TILES_PER_MAP * TILES_PER_MAP];
        int nmerge = 0;
        GameObjectTree& tree = gobjectTrees[mapID];
        // build all tiles
        for (int y = 0; y < TILES_PER_MAP; ++y)
        {
            for (int x = 0; x < TILES_PER_MAP; ++x)
            {
                Tile& tile = tiles[x + y * TILES_PER_MAP];

                // Calculate the per tile bounding box.
                tileCfg.bmin[0] = config.bmin[0] + float(x*config.tileSize - config.borderSize)*config.cs;
                tileCfg.bmin[2] = config.bmin[2] + float(y*config.tileSize - config.borderSize)*config.cs;
                tileCfg.bmax[0] = config.bmin[0] + float((x+1)*config.tileSize + config.borderSize)*config.cs;
                tileCfg.bmax[2] = config.bmin[2] + float((y+1)*config.tileSize + config.borderSize)*config.cs;

                // build heightfield
                tile.solid = rcAllocHeightfield();
                if (!tile.solid || !rcCreateHeightfield(m_rcContext, *tile.solid, tileCfg.width, tileCfg.height, tileCfg.bmin, tileCfg.bmax, tileCfg.cs, tileCfg.ch))
                {
                    printf("%s Failed building heightfield!            \n", tileString);
                    continue;
                }

                // mark all walkable tiles, both liquids and solids
                unsigned char* triFlags = new unsigned char[tTriCount];
                memset(triFlags, NAV_GROUND, tTriCount*sizeof(unsigned char));
                rcClearUnwalkableTriangles(m_rcContext, tileCfg.walkableSlopeAngle, tVerts, tVertCount, tTris, tTriCount, triFlags);

                // mark dynamic surface
                if (!tree.empty())
                {
                    for (int i = 0; i < tTriCount; ++i)
                    {
                        const int* triIndices = &tTris[i * 3];
                        float tri[3 * 3];
                        G3D::AABox box;
                        for (int p = 0; p < 3; ++p)
                        {
                            std::memcpy(&tri[p * 3], &tVerts[triIndices[p] * 3], 3 * sizeof(float));
                            box.merge(G3D::Vector3(tri[p * 3], tri[p * 3 + 1], tri[p * 3 + 2]));
                        }
                        float minY = std::min(std::min(tri[1], tri[3 + 1]), tri[2 * 3 + 1]);
                        float maxY = std::max(std::max(tri[1], tri[3 + 1]), tri[2 * 3 + 1]);
                        auto itr = boost::make_function_output_iterator([&](auto const& val)
                        {
                            std::shared_ptr<GameObjectBounds> data = val.second;
                            if ((triFlags[i] & NAV_DYNAMIC) || data->isStatic)
                                return;
                            bool overlapY = !(maxY < (data->bounds[1] - 0.5f) || minY >(data->bounds[1] + data->height - 1.0f));
                            if (overlapY && dtOverlapPolyPoly2D(tri, 3, data->bounds, 4))
                                triFlags[i] |= NAV_DYNAMIC;
                        });
                        tree.query(bgi::intersects(box), itr);
                    }
                }

                rcRasterizeTriangles(m_rcContext, tVerts, tVertCount, tTris, triFlags, tTriCount, *tile.solid, config.walkableClimb);
                delete[] triFlags;

                rcFilterLowHangingWalkableObstacles(m_rcContext, config.walkableClimb, *tile.solid);
                rcFilterLedgeSpans(m_rcContext, tileCfg.walkableHeight, tileCfg.walkableClimb, *tile.solid);
                rcFilterWalkableLowHeightSpans(m_rcContext, tileCfg.walkableHeight, *tile.solid);

                rcRasterizeTriangles(m_rcContext, lVerts, lVertCount, lTris, lTriFlags, lTriCount, *tile.solid, config.walkableClimb);

                // compact heightfield spans
                tile.chf = rcAllocCompactHeightfield();
                if (!tile.chf || !rcBuildCompactHeightfield(m_rcContext, tileCfg.walkableHeight, tileCfg.walkableClimb, *tile.solid, *tile.chf))
                {
                    printf("%s Failed compacting heightfield!            \n", tileString);
                    continue;
                }

                // build polymesh intermediates
                if (!rcErodeWalkableArea(m_rcContext, config.walkableRadius, *tile.chf))
                {
                    printf("%s Failed eroding area!                    \n", tileString);
                    continue;
                }

                if (!rcBuildDistanceField(m_rcContext, *tile.chf))
                {
                    printf("%s Failed building distance field!         \n", tileString);
                    continue;
                }

                if (!rcBuildRegions(m_rcContext, *tile.chf, tileCfg.borderSize, tileCfg.minRegionArea, tileCfg.mergeRegionArea))
                {
                    printf("%s Failed building regions!                \n", tileString);
                    continue;
                }

                tile.cset = rcAllocContourSet();
                if (!tile.cset || !rcBuildContours(m_rcContext, *tile.chf, tileCfg.maxSimplificationError, tileCfg.maxEdgeLen, *tile.cset))
                {
                    printf("%s Failed building contours!               \n", tileString);
                    continue;
                }

                // build polymesh
                tile.pmesh = rcAllocPolyMesh();
                if (!tile.pmesh || !rcBuildPolyMesh(m_rcContext, *tile.cset, tileCfg.maxVertsPerPoly, *tile.pmesh))
                {
                    printf("%s Failed building polymesh!               \n", tileString);
                    continue;
                }

                tile.dmesh = rcAllocPolyMeshDetail();
                if (!tile.dmesh || !rcBuildPolyMeshDetail(m_rcContext, *tile.pmesh, *tile.chf, tileCfg.detailSampleDist, tileCfg.detailSampleMaxError, *tile.dmesh))
                {
                    printf("%s Failed building polymesh detail!        \n", tileString);
                    continue;
                }

                // free those up
                // we may want to keep them in the future for debug
                // but right now, we don't have the code to merge them
                rcFreeHeightField(tile.solid);
                tile.solid = NULL;
                rcFreeCompactHeightfield(tile.chf);
                tile.chf = NULL;
                rcFreeContourSet(tile.cset);
                tile.cset = NULL;

                pmmerge[nmerge] = tile.pmesh;
                dmmerge[nmerge] = tile.dmesh;
                nmerge++;
            }
        }

        iv.polyMesh = rcAllocPolyMesh();
        if (!iv.polyMesh)
        {
            printf("%s alloc iv.polyMesh FAILED!\n", tileString);
            delete[] pmmerge;
            delete[] dmmerge;
            delete[] tiles;
            return;
        }
        rcMergePolyMeshes(m_rcContext, pmmerge, nmerge, *iv.polyMesh);

        iv.polyMeshDetail = rcAllocPolyMeshDetail();
        if (!iv.polyMeshDetail)
        {
            printf("%s alloc m_dmesh FAILED!\n", tileString);
            delete[] pmmerge;
            delete[] dmmerge;
            delete[] tiles;
            return;
        }
        rcMergePolyMeshDetails(m_rcContext, dmmerge, nmerge, *iv.polyMeshDetail);

        // free things up
        delete[] pmmerge;
        delete[] dmmerge;
        delete[] tiles;

        int tx = (((bmin[0] + bmax[0]) / 2) - navMesh->getParams()->orig[0]) / GRID_SIZE;
        int ty = (((bmin[2] + bmax[2]) / 2) - navMesh->getParams()->orig[2]) / GRID_SIZE;

        // set polygons as walkable
        float* poly = new float[iv.polyMesh->nvp * 3];
        FILE* sqlFile = fopen((std::string("gameobject_polygons_") + std::to_string(mapID) + ".sql").c_str(), "a");
        fprintf(sqlFile, "DELETE FROM `gameobject_polygons` WHERE `map` = %u AND `tileX` = %d AND `tileY` = %d;\n", mapID, tx, ty);
        bool first = true;
        for (int i = 0; i < iv.polyMesh->npolys; ++i)
        {
            if (iv.polyMesh->areas[i] & RC_WALKABLE_AREA)
                iv.polyMesh->flags[i] = iv.polyMesh->areas[i];
            if (iv.polyMesh->areas[i] & NAV_DYNAMIC)
            {
                int offset = i * iv.polyMesh->nvp * 2;
                int j = 0;
                float minY = std::numeric_limits<float>::max();
                float maxY = std::numeric_limits<float>::min();
                G3D::AABox box;
                for (; j < iv.polyMesh->nvp; ++j)
                {
                    unsigned short idx = iv.polyMesh->polys[offset + j];
                    if (idx == 0xffff)
                        break;
                    const unsigned short* v = &iv.polyMesh->verts[idx * 3];
                    poly[j * 3 + 0] = bmin[0] + v[0] * config.cs;
                    poly[j * 3 + 1] = bmin[1] + v[1] * config.ch;
                    poly[j * 3 + 2] = bmin[2] + v[2] * config.cs;
                    box.merge(G3D::Vector3(poly[j * 3 + 0], poly[j * 3 + 1], poly[j * 3 + 2]));
                    if (minY > poly[j * 3 + 1])
                        minY = poly[j * 3 + 1];
                    if (maxY < poly[j * 3 + 1])
                        maxY = poly[j * 3 + 1];
                }

                bool foundGameObject = false;
                auto itr = boost::make_function_output_iterator([&](auto const& val)
                {
                    std::shared_ptr<GameObjectBounds> data = val.second;
                    if (!data->isStatic)
                    {
                        bool overlapY = !(maxY < (data->bounds[1] - 0.5f) || minY >(data->bounds[1] + data->height - 1.0f));
                        if (overlapY && dtOverlapPolyPoly2D(poly, j, data->bounds, 4))
                        {
                            foundGameObject = true;
                            if (!first)
                            {
                                fprintf(sqlFile, ",\n");
                            }
                            else
                            {
                                fprintf(sqlFile, "INSERT INTO `gameobject_polygons` (`map`, `tileX`, `tileY`, `polyIndex`, `guid`) VALUES \n");
                                first = false;
                            }
                            fprintf(sqlFile, "(%u, %d, %d, %d, %u)", mapID, tx, ty, i, data->guid);
                        }
                    }
                });
                tree.query(bgi::intersects(box), itr);
                if (!foundGameObject)
                    iv.polyMesh->flags[i] &= ~NAV_DYNAMIC;
            }
        }
        if (!first)
            fprintf(sqlFile, ";\n");
        fclose(sqlFile);
        delete[] poly;

        // setup mesh parameters
        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        params.verts = iv.polyMesh->verts;
        params.vertCount = iv.polyMesh->nverts;
        params.polys = iv.polyMesh->polys;
        params.polyAreas = iv.polyMesh->areas;
        params.polyFlags = iv.polyMesh->flags;
        params.polyCount = iv.polyMesh->npolys;
        params.nvp = iv.polyMesh->nvp;
        params.detailMeshes = iv.polyMeshDetail->meshes;
        params.detailVerts = iv.polyMeshDetail->verts;
        params.detailVertsCount = iv.polyMeshDetail->nverts;
        params.detailTris = iv.polyMeshDetail->tris;
        params.detailTriCount = iv.polyMeshDetail->ntris;

        params.offMeshConVerts = meshData.offMeshConnections.getCArray();
        params.offMeshConCount = meshData.offMeshConnections.size()/6;
        params.offMeshConRad = meshData.offMeshConnectionRads.getCArray();
        params.offMeshConDir = meshData.offMeshConnectionDirs.getCArray();
        params.offMeshConAreas = meshData.offMeshConnectionsAreas.getCArray();
        params.offMeshConFlags = meshData.offMeshConnectionsFlags.getCArray();

        params.walkableHeight = BASE_UNIT_DIM*config.walkableHeight;    // agent height
        params.walkableRadius = BASE_UNIT_DIM*config.walkableRadius;    // agent radius
        params.walkableClimb = BASE_UNIT_DIM*config.walkableClimb;      // keep less that walkableHeight (aka agent height)!
        params.tileX = (((bmin[0] + bmax[0]) / 2) - navMesh->getParams()->orig[0]) / GRID_SIZE;
        params.tileY = (((bmin[2] + bmax[2]) / 2) - navMesh->getParams()->orig[2]) / GRID_SIZE;
        rcVcopy(params.bmin, bmin);
        rcVcopy(params.bmax, bmax);
        params.cs = config.cs;
        params.ch = config.ch;
        params.tileLayer = 0;
        params.buildBvTree = true;

        // will hold final navmesh
        unsigned char* navData = NULL;
        int navDataSize = 0;

        do
        {
            // these values are checked within dtCreateNavMeshData - handle them here
            // so we have a clear error message
            if (params.nvp > DT_VERTS_PER_POLYGON)
            {
                printf("%s Invalid verts-per-polygon value!        \n", tileString);
                break;
            }
            if (params.vertCount >= 0xffff)
            {
                printf("%s Too many vertices!                      \n", tileString);
                break;
            }
            if (!params.vertCount || !params.verts)
            {
                // occurs mostly when adjacent tiles have models
                // loaded but those models don't span into this tile

                // message is an annoyance
                //printf("%sNo vertices to build tile!              \n", tileString);
                break;
            }
            if (!params.polyCount || !params.polys ||
                TILES_PER_MAP*TILES_PER_MAP == params.polyCount)
            {
                // we have flat tiles with no actual geometry - don't build those, its useless
                // keep in mind that we do output those into debug info
                // drop tiles with only exact count - some tiles may have geometry while having less tiles
                printf("%s No polygons to build on tile!              \n", tileString);
                break;
            }
            if (!params.detailMeshes || !params.detailVerts || !params.detailTris)
            {
                printf("%s No detail mesh to build tile!           \n", tileString);
                break;
            }

            printf("%s Building navmesh tile...\n", tileString);
            if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
            {
                printf("%s Failed building navmesh tile!           \n", tileString);
                break;
            }

            dtTileRef tileRef = 0;
            printf("%s Adding tile to navmesh...\n", tileString);
            // DT_TILE_FREE_DATA tells detour to unallocate memory when the tile
            // is removed via removeTile()
            dtStatus dtResult = navMesh->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, &tileRef);
            if (!tileRef || dtResult != DT_SUCCESS)
            {
                printf("%s Failed adding tile to navmesh!           \n", tileString);
                break;
            }

            // file output
            char fileName[255];
            sprintf(fileName, "mmaps/%03u%02i%02i.mmtile", mapID, tileY, tileX);
            FILE* file = fopen(fileName, "wb");
            if (!file)
            {
                char message[1024];
                sprintf(message, "[Map %03i] Failed to open %s for writing!\n", mapID, fileName);
                perror(message);
                navMesh->removeTile(tileRef, NULL, NULL);
                break;
            }

            printf("%s Writing to file...\n", tileString);

            // write header
            MmapTileHeader header;
            header.usesLiquids = m_terrainBuilder->usesLiquids();
            header.size = uint32(navDataSize);
            fwrite(&header, sizeof(MmapTileHeader), 1, file);

            // write data
            fwrite(navData, sizeof(unsigned char), navDataSize, file);
            fclose(file);

            // now that tile is written to disk, we can unload it
            navMesh->removeTile(tileRef, NULL, NULL);
        }
        while (0);

        if (m_debugOutput)
        {
            // restore padding so that the debug visualization is correct
            for (int i = 0; i < iv.polyMesh->nverts; ++i)
            {
                unsigned short* v = &iv.polyMesh->verts[i*3];
                v[0] += (unsigned short)config.borderSize;
                v[2] += (unsigned short)config.borderSize;
            }

            iv.generateObjFile(mapID, tileX, tileY, meshData);
            iv.writeIV(mapID, tileX, tileY);
        }
    }

    /**************************************************************************/
    void MapBuilder::getTileBounds(uint32 tileX, uint32 tileY, float* verts, int vertCount, float* bmin, float* bmax)
    {
        // this is for elevation
        if (verts && vertCount)
            rcCalcBounds(verts, vertCount, bmin, bmax);
        else
        {
            bmin[1] = FLT_MIN;
            bmax[1] = FLT_MAX;
        }

        // this is for width and depth
        bmax[0] = (32 - int(tileX)) * GRID_SIZE;
        bmax[2] = (32 - int(tileY)) * GRID_SIZE;
        bmin[0] = bmax[0] - GRID_SIZE;
        bmin[2] = bmax[2] - GRID_SIZE;
    }

    /**************************************************************************/
    bool MapBuilder::shouldSkipMap(uint32 mapID)
    {
        if (m_skipContinents)
            switch (mapID)
            {
                case 0:
                case 1:
                case 530:
                case 571:
                    return true;
                default:
                    break;
            }

        if (m_skipJunkMaps)
            switch (mapID)
            {
                case 13:    // test.wdt
                case 25:    // ScottTest.wdt
                case 29:    // Test.wdt
                case 42:    // Colin.wdt
                case 169:   // EmeraldDream.wdt (unused, and very large)
                case 451:   // development.wdt
                case 573:   // ExteriorTest.wdt
                case 597:   // CraigTest.wdt
                case 605:   // development_nonweighted.wdt
                case 606:   // QA_DVD.wdt
                    return true;
                default:
                    if (isTransportMap(mapID))
                        return true;
                    break;
            }

        if (m_skipBattlegrounds)
            switch (mapID)
            {
                case 30:    // AV
                case 37:    // ?
                case 489:   // WSG
                case 529:   // AB
                case 566:   // EotS
                case 607:   // SotA
                case 628:   // IoC
                    return true;
                default:
                    break;
            }

        return false;
    }

    /**************************************************************************/
    bool MapBuilder::isTransportMap(uint32 mapID)
    {
        switch (mapID)
        {
            // transport maps
            case 582:
            case 584:
            case 586:
            case 587:
            case 588:
            case 589:
            case 590:
            case 591:
            case 592:
            case 593:
            case 594:
            case 596:
            case 610:
            case 612:
            case 613:
            case 614:
            case 620:
            case 621:
            case 622:
            case 623:
            case 641:
            case 642:
            case 647:
            case 672:
            case 673:
            case 712:
            case 713:
            case 718:
                return true;
            default:
                return false;
        }
    }

    /**************************************************************************/
    bool MapBuilder::shouldSkipTile(uint32 mapID, uint32 tileX, uint32 tileY)
    {
        char fileName[255];
        sprintf(fileName, "mmaps/%03u%02i%02i.mmtile", mapID, tileY, tileX);
        FILE* file = fopen(fileName, "rb");
        if (!file)
            return false;

        MmapTileHeader header;
        int count = fread(&header, sizeof(MmapTileHeader), 1, file);
        fclose(file);
        if (count != 1)
            return false;

        if (header.mmapMagic != MMAP_MAGIC || header.dtVersion != uint32(DT_NAVMESH_VERSION))
            return false;

        if (header.mmapVersion != MMAP_VERSION)
            return false;

        return true;
    }

    /**************************************************************************/
    uint32 MapBuilder::percentageDone(uint32 totalTiles, uint32 totalTilesBuilt)
    {
        if (totalTiles)
            return totalTilesBuilt * 100 / totalTiles;

        return 0;
    }

        /**
     * Build navmesh for GameObject model.
     * Yup, transports are GameObjects and we need pathfinding there.
     * This is basically a copy-paste of buildMoveMapTile with slightly diff parameters
     * .. and without worrying about model/terrain, undermap or liquids.
     */
    void MapBuilder::buildGameObject(std::string model, uint32 displayId)
    {
        printf("Building GameObject model %s\n", model.c_str());
        WorldModel m;
        MeshData meshData;
        if (!m.readFile("vmaps/" + model))
        {
            printf("* Unable to open file\n");
            return;
        }

        // Load model data into navmesh
        std::vector<GroupModel> groupModels;
        m.getGroupModels(groupModels);

        // all M2s need to have triangle indices reversed
        bool isM2 = model.find(".m2") != model.npos || model.find(".M2") != model.npos;

        for (std::vector<GroupModel>::iterator it = groupModels.begin(); it != groupModels.end(); ++it)
        {
            // transform data
            std::vector<G3D::Vector3> tempVertices;
            std::vector<MeshTriangle> tempTriangles;
            WmoLiquid* liquid = NULL;

            (*it).getMeshData(tempVertices, tempTriangles, liquid);

            int offset = meshData.solidVerts.size() / 3;

            TerrainBuilder::copyVertices(tempVertices, meshData.solidVerts);
            TerrainBuilder::copyIndices(tempTriangles, meshData.solidTris, offset, isM2);
        }
        // if there is no data, give up now
        if (!meshData.solidVerts.size())
        {
            printf("* no solid vertices found\n");
            return;
        }
        TerrainBuilder::cleanVertices(meshData.solidVerts, meshData.solidTris);

        // gather all mesh data for final data check, and bounds calculation
        G3D::Array<float> allVerts;
        allVerts.append(meshData.solidVerts);

        if (!allVerts.size())
            return;

        printf("* Model opened (%u vertices)\n", allVerts.size());

        float* verts = meshData.solidVerts.getCArray();
        int nverts = meshData.solidVerts.size() / 3;
        int* tris = meshData.solidTris.getCArray();
        int ntris = meshData.solidTris.size() / 3;

        const static float BASE_UNIT_DIM = 0.13f;
        float bmin[3], bmax[3];

        rcCalcBounds(verts, nverts, bmin, bmax);
        rcConfig config = configStore.GetGameObjectConfig(displayId).GenerateConfig(m_bigBaseUnit, bmin, bmax);
		
        // this sets the dimensions of the heightfield - should maybe happen before border padding
        rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

        Tile tile;
        tile.solid = rcAllocHeightfield();
        if (!tile.solid || !rcCreateHeightfield(m_rcContext, *tile.solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
        {
            printf("* Failed building heightfield!            \n");
            return;
        }
        unsigned char* m_triareas = new unsigned char[ntris];
        memset(m_triareas, 0, ntris*sizeof(unsigned char));
        rcMarkWalkableTriangles(m_rcContext, config.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
        rcRasterizeTriangles(m_rcContext, verts, nverts, tris, m_triareas, ntris, *tile.solid, config.walkableClimb);
        rcFilterLowHangingWalkableObstacles(m_rcContext, config.walkableClimb, *tile.solid);
        rcFilterLedgeSpans(m_rcContext, config.walkableHeight, config.walkableClimb, *tile.solid);
        rcFilterWalkableLowHeightSpans(m_rcContext, config.walkableHeight, *tile.solid);
        tile.chf = rcAllocCompactHeightfield();
        if (!tile.chf || !rcBuildCompactHeightfield(m_rcContext, config.walkableHeight, config.walkableClimb, *tile.solid, *tile.chf))
        {
            printf("Failed compacting heightfield!            \n");
            return;
        }
        // Erode the walkable area by agent radius.
        if (!rcErodeWalkableArea(m_rcContext, config.walkableRadius, *tile.chf))
        {
            printf("Failed eroding heightfield!            \n");
            return;
        }
        if (!rcBuildDistanceField(m_rcContext, *tile.chf))
        {
            printf("Failed building distance field!         \n");
            return;
        }

        if (!rcBuildRegions(m_rcContext, *tile.chf, 0, config.minRegionArea, config.mergeRegionArea))
        {
            printf("Failed building regions!                \n");
            return;
        }

        tile.cset = rcAllocContourSet();
        if (!tile.cset || !rcBuildContours(m_rcContext, *tile.chf, config.maxSimplificationError, config.maxEdgeLen, *tile.cset))
        {
            printf("Failed building contours!               \n");
            return;
        }

        // build polymesh
        tile.pmesh = rcAllocPolyMesh();
        if (!tile.pmesh || !rcBuildPolyMesh(m_rcContext, *tile.cset, config.maxVertsPerPoly, *tile.pmesh))
        {
            printf("Failed building polymesh!               \n");
            return;
        }

        tile.dmesh = rcAllocPolyMeshDetail();
        if (!tile.dmesh || !rcBuildPolyMeshDetail(m_rcContext, *tile.pmesh, *tile.chf, config.detailSampleDist, config.detailSampleMaxError, *tile.dmesh))
        {
            printf("Failed building polymesh detail!        \n");
            return;
        }
        rcFreeHeightField(tile.solid);
        tile.solid = NULL;
        rcFreeCompactHeightfield(tile.chf);
        tile.chf = NULL;
        rcFreeContourSet(tile.cset);
        tile.cset = NULL;

        IntermediateValues iv;
        iv.polyMesh = tile.pmesh;
        iv.polyMeshDetail = tile.dmesh;
        for (int i = 0; i < iv.polyMesh->npolys; ++i)
        {
            if (iv.polyMesh->areas[i] == RC_WALKABLE_AREA)
            {
                iv.polyMesh->areas[i] = 0; // =SAMPLE_POLYAREA_GROUND in RecastDemo
                iv.polyMesh->flags[i] = NAV_GROUND;
            }
            else
            {
                iv.polyMesh->areas[i] = 0;
                iv.polyMesh->flags[i] = 0;
            }
        }

        // Will be deleted by IntermediateValues
        tile.pmesh = NULL;
        tile.dmesh = NULL;
        // setup mesh parameters
        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        params.verts = iv.polyMesh->verts;
        params.vertCount = iv.polyMesh->nverts;
        params.polys = iv.polyMesh->polys;
        params.polyAreas = iv.polyMesh->areas;
        params.polyFlags = iv.polyMesh->flags;
        params.polyCount = iv.polyMesh->npolys;
        params.nvp = iv.polyMesh->nvp;
        params.detailMeshes = iv.polyMeshDetail->meshes;
        params.detailVerts = iv.polyMeshDetail->verts;
        params.detailVertsCount = iv.polyMeshDetail->nverts;
        params.detailTris = iv.polyMeshDetail->tris;
        params.detailTriCount = iv.polyMeshDetail->ntris;

        params.walkableHeight = config.walkableHeight;  // agent height
        params.walkableRadius = config.walkableRadius;  // agent radius
        params.walkableClimb = config.walkableClimb;    // keep less that walkableHeight (aka agent height)!
        rcVcopy(params.bmin, iv.polyMesh->bmin);
        rcVcopy(params.bmax, iv.polyMesh->bmax);
        params.cs = config.cs;
        params.ch = config.ch;
        params.buildBvTree = true;

        unsigned char* navData = NULL;
        int navDataSize = 0;
        printf("* Building navmesh tile [%f %f %f to %f %f %f]\n",
                params.bmin[0], params.bmin[1], params.bmin[2],
                params.bmax[0], params.bmax[1], params.bmax[2]);
        printf(" %u triangles (%u vertices)\n", params.polyCount, params.vertCount);
        printf(" %u polygons (%u vertices)\n", params.detailTriCount, params.detailVertsCount);
        if (params.nvp > DT_VERTS_PER_POLYGON)
        {
            printf("Invalid verts-per-polygon value!        \n");
            return;
        }
        if (params.vertCount >= 0xffff)
        {
            printf("Too many vertices! (0x%8x)        \n", params.vertCount);
            return;
        }
        if (!params.vertCount || !params.verts)
        {
            printf("No vertices to build tile!              \n");
            return;
        }
        if (!params.polyCount || !params.polys)
        {
            // we have flat tiles with no actual geometry - don't build those, its useless
            // keep in mind that we do output those into debug info
            // drop tiles with only exact count - some tiles may have geometry while having less tiles
            printf("No polygons to build on tile!              \n");
            return;
        }
        if (!params.detailMeshes || !params.detailVerts || !params.detailTris)
        {
            printf("No detail mesh to build tile!           \n");
            return;
        }
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            printf("Failed building navmesh tile!           \n");
            return;
        }
        char fileName[255];
        sprintf(fileName, "mmaps/go%04u.mmap", displayId);
        FILE* file = fopen(fileName, "wb");
        if (!file)
        {
            char message[1024];
            sprintf(message, "Failed to open %s for writing!\n", fileName);
            perror(message);
            return;
        }

        printf("* Writing to file \"%s\" [size=%u]\n", fileName, navDataSize);

        // write header
        MmapTileHeader header;
        header.usesLiquids = false;
        header.size = uint32(navDataSize);
        fwrite(&header, sizeof(MmapTileHeader), 1, file);

        // write data
        fwrite(navData, sizeof(unsigned char), navDataSize, file);
        fclose(file);
        if (m_debugOutput)
        {
            iv.generateObjFile(model, meshData);
            // Write navmesh data
            std::string fname = "meshes/" + model + ".nav";
            FILE* file = fopen(fname.c_str(), "wb");
            if (file)
            {
                fwrite(&navDataSize, sizeof(uint32), 1, file);
                fwrite(navData, sizeof(unsigned char), navDataSize, file);
                fclose(file);
            }
        }
    }

    void MapBuilder::buildTransports()
    {
        // List here Transport gameobjects you want to extract.
        buildGameObject("Elevatorcar.m2.vmo", 360);
        buildGameObject("Undeadelevator.m2.vmo", 455);
        buildGameObject("Undeadelevatordoor.m2.vmo", 462);
        // buildGameObject("Ironforgeelevator.m2.vmo", 561);
        // buildGameObject("Ironforgeelevatordoor.m2.vmo", 562);
        buildGameObject("Gnomeelevatorcar01.m2.vmo", 807);
        buildGameObject("Gnomeelevatorcar02.m2.vmo", 808);
        // buildGameObject("Gnomeelevatorcar05.m2.vmo", 827);
        buildGameObject("Gnomeelevatorcar03.m2.vmo", 852);
        buildGameObject("Gnomehutelevator.m2.vmo", 1587);
        buildGameObject("Burningsteppselevator.m2.vmo", 2454);
        buildGameObject("Transportship.wmo.vmo", 3015);
        buildGameObject("Transport_Zeppelin.wmo.vmo", 3031);
        buildGameObject("Subwaycar.m2.vmo", 3831);
        // buildGameObject("Blackcitadel.wmo.vmo", 6637);
        buildGameObject("Ancdrae_Elevatorpiece.m2.vmo", 7026);
        buildGameObject("Mushroombase_Elevator.m2.vmo", 7028);
        buildGameObject("Cf_Elevatorplatform.m2.vmo", 7043);
        buildGameObject("Cf_Elevatorplatform_Small.m2.vmo", 7060);
        buildGameObject("Factoryelevator.m2.vmo", 7077);
        // buildGameObject("Transportship_Ne.wmo.vmo", 7087);
        buildGameObject("Ancdrae_Elevatorpiece_Netherstorm.m2.vmo", 7163);
        // buildGameObject("Transport_Icebreaker_Ship.wmo.vmo", 7446);
        buildGameObject("Vr_Elevator_Gate.m2.vmo", 7451);
        buildGameObject("Vr_Elevator_Lift.m2.vmo", 7452);
        // buildGameObject("VR_Elevator_Pulley.m2.vmo", 7491);
        buildGameObject("Hf_Elevator_Gate.m2.vmo", 7519);
        buildGameObject("Hf_Elevator_Lift_02.m2.vmo", 7520);
        buildGameObject("Hf_Elevator_Lift.m2.vmo", 7521);
        buildGameObject("Transport_Horde_Zeppelin.wmo.vmo", 7546);
        buildGameObject("Transport_Pirate_Ship.wmo.vmo", 7570);
        buildGameObject("Transport_Tuskarr_Ship.wmo.vmo", 7636);
        buildGameObject("Vrykul_Gondola.wmo.vmo", 7642);
        buildGameObject("Logrun_Pumpelevator01.m2.vmo", 7648);
        buildGameObject("Vrykul_Gondola.m2.vmo", 7767);
        buildGameObject("Nexus_Elevator_Basestructure_01.m2.vmo", 7793);
        buildGameObject("Id_Elevator.m2.vmo", 7794);
        buildGameObject("Orc_Fortress_Elevator01.m2.vmo", 7797);
        buildGameObject("Org_Arena_Pillar.m2.vmo", 7966);
        buildGameObject("Org_Arena_Elevator.m2.vmo", 7973);
        buildGameObject("Logrun_Pumpelevator02.m2.vmo", 8079);
        buildGameObject("Logrun_Pumpelevator03.m2.vmo", 8080);
        buildGameObject("Nd_Hordegunship.wmo.vmo", 8253);
        buildGameObject("Nd_Alliancegunship.wmo.vmo", 8254);
        buildGameObject("Org_Arena_Yellow_Elevator.m2.vmo", 8258);
        buildGameObject("Org_Arena_Axe_Pillar.m2.vmo", 8259);
        buildGameObject("Org_Arena_Lightning_Pillar.m2.vmo", 8260);
        buildGameObject("Org_Arena_Ivory_Pillar.m2.vmo", 8261);
        buildGameObject("Gundrak_Elevator_01.m2.vmo", 8277);
        buildGameObject("Nd_Icebreaker_Ship_Bg_Transport.wmo.vmo", 8409);
        buildGameObject("Nd_Ship_Ud_Bg_Transport.wmo.vmo", 8410);
        buildGameObject("Ulduarraid_Gnomewing_Transport_Wmo.wmo.vmo", 8587);
        buildGameObject("Nd_Hordegunship_Bg.wmo.vmo", 9001);
        buildGameObject("Nd_Alliancegunship_Bg.wmo.vmo", 9002);
        buildGameObject("Icecrown_Elevator.m2.vmo", 9136);
        buildGameObject("Nd_Alliancegunship_Icecrown.wmo.vmo", 9150);
        buildGameObject("Nd_Hordegunship_Icecrown.wmo.vmo", 9151);
        buildGameObject("Icecrown_Elevator02.m2.vmo", 9248);
    }

    void MapBuilder::loadGameObjectModelList()
    {
        FILE* model_list_file = fopen((std::string("vmaps/") + VMAP::GAMEOBJECT_MODELS).c_str(), "rb");
        if (!model_list_file)
        {
            printf("Unable to open '%s' file.\n", VMAP::GAMEOBJECT_MODELS);
            return;
        }

        uint32 name_length, displayId;
        char buff[500];
        while (true)
        {
            G3D::Vector3 v1, v2;
            if (fread(&displayId, sizeof(uint32), 1, model_list_file) != 1)
                if (feof(model_list_file))  // EOF flag is only set after failed reading attempt
                    break;

            if (fread(&name_length, sizeof(uint32), 1, model_list_file) != 1
                || name_length >= sizeof(buff)
                || fread(&buff, sizeof(char), name_length, model_list_file) != name_length
                || fread(&v1, sizeof(G3D::Vector3), 1, model_list_file) != 1
                || fread(&v2, sizeof(G3D::Vector3), 1, model_list_file) != 1)
            {
                printf("File '%s' seems to be corrupted!\n", VMAP::GAMEOBJECT_MODELS);
                break;
            }

            if (v1.isNaN() || v2.isNaN())
            {
                printf("File '%s' Model '%s' has invalid v1%s v2%s values!\n", VMAP::GAMEOBJECT_MODELS, std::string(buff, name_length).c_str(), v1.toString().c_str(), v2.toString().c_str());
                continue;
            }

            G3D::AABox box(v1, v2);
            if (box == G3D::AABox::zero())
                continue;

            modelList.emplace(displayId, GameobjectModelData(std::string(buff, name_length), box));
        }

        fclose(model_list_file);
    }

    bool MapBuilder::loadGameObjects()
    {
        bool res = true;
        printf("Loading game objects... ");
        MYSQL con;
        if (!mysql_init(&con))
        {
            printf("\nCould not initialize database connection.\n");
            res = false;
        }
        if (res && mysql_real_connect(&con, mysqlHost, mysqlUser, mysqlPassword, mysqlDB, mysqlPort, 0, 0))
        {
            mysql_autocommit(&con, 1);
            mysql_set_character_set(&con, "utf8");

            const char* sql = "CALL getGameObjectMMapData();";

            if (!mysql_query(&con, sql))
            {
                loadGameObjectModelList();

                MYSQL_RES* result = mysql_store_result(&con);
                uint64 rowCount = mysql_affected_rows(&con);
                uint64 count = 0;

                if (rowCount && result)
                {
                    std::unordered_map<uint32, std::list<GOTreeElement>> treeElements;
                    MYSQL_ROW row = mysql_fetch_row(result);
                    while ((row = mysql_fetch_row(result)))
                    {
                        uint32 guid = std::stoul(row[0]);
                        uint32 displayId = std::stoul(row[1]);
                        float size = std::stof(row[2]);
                        uint32 mapId = std::stoul(row[3]);
                        float x = std::stof(row[4]);
                        float y = std::stof(row[5]);
                        float z = std::stof(row[6]);
                        float rot0 = std::stof(row[7]);
                        float rot1 = std::stof(row[8]);
                        float rot2 = std::stof(row[9]);
                        float rot3 = std::stof(row[10]);
                        bool isStatic = std::stoul(row[11]);

                        G3D::Quat quat(rot0, rot1, rot2, rot3);
                        if (isStatic)
                            quat = quat.toUnit().conj();

                        G3D::Vector3 iPos(x, y, z);
                        G3D::Matrix3 rotation(quat);
                        auto it = modelList.find(displayId);
                        if (it != modelList.end())
                        {
                            G3D::AABox box = it->second.bound;
                            float height = (box.high().z - box.low().z) * size;
                            if (isStatic || height > 1.5f)
                            {
                                std::shared_ptr<GameObjectBounds> data = std::make_shared<GameObjectBounds>(guid, displayId, height, size, iPos, rotation, isStatic);
                                G3D::AABox rTreeBox;
                                for (int i = 0; i < 4; ++i)
                                {
                                    G3D::Vector3 pos((rotation * box.corner(i + 4) * size) + iPos);
                                    data->bounds[i * 3 + 0] = pos.y;
                                    data->bounds[i * 3 + 1] = pos.z;
                                    data->bounds[i * 3 + 2] = pos.x;
                                    rTreeBox.merge(G3D::Vector3(pos.y, pos.z, pos.x));
                                }
                                treeElements[mapId].emplace_back(rTreeBox, data);
                                ++count;
                            }
                        }
                    }
                    for (auto itr = treeElements.cbegin(); itr != treeElements.cend(); ++itr)
                        gobjectTrees.emplace(itr->first, GameObjectTree(itr->second));
                }

                printf("loaded %llu spawned game objects.\n", count);

                if (result)
                    mysql_free_result(result);
            }
            else
            {
                uint32 err = mysql_errno(&con);
                printf("\nCould not query game objects: [%u] %s\n", err, mysql_error(&con));
                res = false;
            }
        }
        else
        {
            printf("\nCould not connect to database.\n");
            res = false;
        }
        mysql_close(&con);
        return res;
    }
}
