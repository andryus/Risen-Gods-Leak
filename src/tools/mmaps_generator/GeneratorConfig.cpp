#include "GeneratorConfig.hpp"
#include <DetourNavMesh.h>
#include "TerrainBuilder.h"

#include <fstream>

rcConfig PathGeneratorConfig::GenerateConfig(bool bigBaseUnit, float bmin[3], float bmax[3]) const
{
    const static float BASE_UNIT_DIM = bigBaseUnit ? 0.5333333f : 0.2666666f;

    // All are in UNIT metrics!
    const static int VERTEX_PER_TILE = bigBaseUnit ? 40 : 80; // must divide VERTEX_PER_MAP

    rcConfig config;
    memset(&config, 0, sizeof(rcConfig));

    rcVcopy(config.bmin, bmin);
    rcVcopy(config.bmax, bmax);

    config.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
    config.cs = BASE_UNIT_DIM;
    config.ch = BASE_UNIT_DIM;
    config.walkableSlopeAngle = WalkableSlopeAngle;
    config.tileSize = VERTEX_PER_TILE;
    config.walkableRadius = WalkableRadius;
    config.borderSize = config.walkableRadius + 3;
    config.maxEdgeLen = VERTEX_PER_TILE + 1;        // anything bigger than tileSize
    config.walkableHeight = WalkableHeight;
    // a value >= 3|6 allows npcs to walk over some fences
    // a value >= 4|8 allows npcs to walk over all fences
    config.walkableClimb = WalkableClimb;
    config.minRegionArea = MinRegionArea;
    config.mergeRegionArea = MergeRegionArea;
    config.maxSimplificationError = MaxSimplificationError;           // eliminates most jagged edges (tiny polygons)
    config.detailSampleDist = DetailSampleDist; // sampling distance to use when generating the detail mesh
    config.detailSampleMaxError = DetailSampleMaxError; // Vertical precision

    return config;
}

template <typename Type>
bool ParseValue(std::string_view view, char* key, Type& value)
{
    const auto speratorPos = view.find('=');
    if (speratorPos == view.npos)
        return false;

    if (view.substr(0, speratorPos).compare(key) == 0)
    {
        view.remove_prefix(speratorPos + 1);
        if constexpr (std::is_floating_point_v<Type>)
            value = std::atof(std::string(view).c_str());
        else
            value = std::atoi(std::string(view).c_str());
        return true;
    }

    return false;
}

PathGeneratorConfig const & ConfigStorage::GetConfig(uint32_t mapId) const
{
    const auto itr = mapConfig.find(mapId);
    if (itr == mapConfig.end())
        return baseConfig;
    else
        return itr->second;
}

PathGeneratorConfig const & ConfigStorage::GetGameObjectConfig(uint32_t displayId) const
{
    const auto itr = gameObjectConfig.find(displayId);
    if (itr == gameObjectConfig.end())
        return baseGameObjectConfig;
    else
        return itr->second;
}

void ConfigStorage::LoadFromFile(std::string path, float maxAngle, bool bigBaseUnit)
{
    std::ifstream configFile(path);

    PathGeneratorConfig* current = &baseConfig;

    if (!configFile.is_open())
    {
        baseConfig.WalkableSlopeAngle = maxAngle;
        baseConfig.WalkableRadius = bigBaseUnit ? 1 : 2;
        baseConfig.WalkableHeight = bigBaseUnit ? 3 : 6;
        baseConfig.WalkableClimb = bigBaseUnit ? 4 : 8;
        baseConfig.MinRegionArea = rcSqr(60);
        baseConfig.MergeRegionArea = rcSqr(50);
        baseConfig.MaxSimplificationError = 1.8f;
        baseConfig.DetailSampleDist = 2.0f;
        baseConfig.DetailSampleMaxError = 0.5f;
        return;
    }

    bool goSet = false;

    std::string line;
    while (std::getline(configFile, line))
    {
        if (line.size() == 0)
            continue;

        std::string_view view(line);
        
        if (*line.begin() == '[' && *line.rbegin() == ']')
        {
            view.remove_prefix(1);
            view.remove_suffix(1);

            uint32 mapId, gameObjectId;
            if (ParseValue(view, "Map", mapId))
            {
                current = &mapConfig[mapId];
                *current = baseConfig;
            }
			else if (ParseValue(view, "GameObject", gameObjectId))
			{
				current = &gameObjectConfig[gameObjectId];
				*current = baseGameObjectConfig;
			}
            else if (view.compare("GameObject") == 0)
            {
                current = &baseGameObjectConfig;
                *current = baseConfig;
                goSet = true;
            }

            continue;
        }

        ParseValue(view, "WalkableSlopeAngle", current->WalkableSlopeAngle);
        ParseValue(view, "WalkableRadius", current->WalkableRadius);
        ParseValue(view, "WalkableHeight", current->WalkableHeight);
        ParseValue(view, "WalkableClimb", current->WalkableClimb);
        ParseValue(view, "MinRegionArea", current->MinRegionArea);
        ParseValue(view, "MergeRegionArea", current->MergeRegionArea);
        ParseValue(view, "MaxSimplificationError", current->MaxSimplificationError);
        ParseValue(view, "DetailSampleDist", current->DetailSampleDist);
        ParseValue(view, "DetailSampleMaxError", current->DetailSampleMaxError);
    }

    if (!goSet)
        baseGameObjectConfig = baseConfig;
}
