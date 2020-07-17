#include "MapArea.h"

#include "API/LoadScript.h"
#include "AreaHooks.h"


MapArea::MapArea(Map& map, uint32 areaId, std::string_view script) :
    map(map), areaId(areaId), script(script)
{
}

MapArea::~MapArea()
{
    unloadScript(*this);
}

void MapArea::Init()
{
    InitScript(script);
}

void MapArea::Update(uint32 diff)
{
    *this <<= Fire::ProcessEvents.Bind(GameTime(diff));
}

uint32 MapArea::GetAreaId() const
{
    return areaId;
}

Map& MapArea::GetMap() const
{
    return map;
}

void MapArea::OnWorldObjectAdded(WorldObject& object)
{
    *this <<= Fire::AddToArea.Bind(&object);
}

std::string MapArea::OnQueryOwnerName() const
{
    return "Area" + std::string(script);
}

bool MapArea::OnInitScript(std::string_view scriptName)
{
    return loadScriptType(*this, scriptName);
}
