#pragma once
#include "Scriptable.h"

class Map;
class MapArea;
class WorldObject;

class GAME_API MapArea : 
    public script::Scriptable, public std::enable_shared_from_this<MapArea>
{
public:

    MapArea(Map& map, uint32 areaId, std::string_view script);
    ~MapArea();

    void Init();
    void Update(uint32 diff);

    uint32 GetAreaId() const;
    Map& GetMap() const;

    void OnWorldObjectAdded(WorldObject& object);

private:

    std::string OnQueryOwnerName() const override;
    bool OnInitScript(std::string_view scriptName) override;

    Map& map;
    uint32 areaId;
    std::string_view script;
};
