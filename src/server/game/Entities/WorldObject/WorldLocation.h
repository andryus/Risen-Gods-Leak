#pragma once
#include "Game/Position.h"

constexpr uint32 MAPID_INVALID = 0xFFFFFFFF;

class GAME_API WorldLocation : public Position
{
public:
    WorldLocation() = default;

    constexpr WorldLocation(uint32 _mapid, float _x, float _y, float _z, float _o = 0.f)
        : Position(_x, _y, _z, _o), m_mapId(_mapid) { }

    constexpr void WorldRelocate(const WorldLocation &loc)
    {
        m_mapId = loc.GetMapId(); 
        Relocate(loc);
    }
    constexpr uint32 GetMapId() const { return m_mapId; }

    bool IsPositionValid() const;

    static bool IsPositionValid(Position pos);

    uint32 m_mapId = MAPID_INVALID;
};

template<>
struct packet::ByteBufferRemapped<WorldLocation> { using type = Position; };
