#pragma once

#include "Recast.h"
#include <unordered_map>

struct PathGeneratorConfig
{
    float WalkableSlopeAngle;
    int WalkableRadius;
    int WalkableHeight;
    int WalkableClimb;
    int MinRegionArea;
    int MergeRegionArea;
    float MaxSimplificationError;
    float DetailSampleDist;
    float DetailSampleMaxError;

    rcConfig GenerateConfig(bool bigBaseUnit, float bmin[3], float bmax[3]) const;
};

class ConfigStorage
{
public:
    ConfigStorage(std::string configPath, float maxAngle, bool bigBaseUnit) { LoadFromFile(configPath, maxAngle, bigBaseUnit); }

    PathGeneratorConfig const& GetConfig(uint32_t mapId) const;
    PathGeneratorConfig const& GetGameObjectConfig(uint32_t displayId) const;
private:
    PathGeneratorConfig baseConfig;
    PathGeneratorConfig baseGameObjectConfig;
    std::unordered_map<uint32_t, PathGeneratorConfig> mapConfig;
    std::unordered_map<uint32_t, PathGeneratorConfig> gameObjectConfig;

    void LoadFromFile(std::string path, float maxAngle, bool bigBaseUnit);
};
