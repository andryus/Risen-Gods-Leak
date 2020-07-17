#pragma once
#include "BaseOwner.h"

class WorldObject;
class Spell;
class MapArea;
class Map;
class Encounter;

struct SpellHandle
{
    using Stored = Spell;

    SpellHandle() = default;
    SpellHandle(const Spell& aura) {};
    SpellHandle(const Spell* aura) {};

    Spell* Load() const { return nullptr; }

    std::string Print() const { return "INVALID (Spell)"; }

    static Spell* FromString(std::string_view, script::Scriptable&) { return nullptr; };
};

SCRIPT_OWNER_HANDLE(Spell, script::Scriptable, SpellHandle)

struct SpellId
{
    SpellId() = default;
    constexpr SpellId(uint32 id) : id(id) {}

    explicit operator uint32() const { return id; }

    uint32 id = 0;
};

SCRIPT_PRINTER(SpellId);
SCRIPT_FROM_STR_BY(SpellId, uint32);


SCRIPT_COMPONENT(Spell, MapArea)
SCRIPT_COMPONENT(Spell, Map)
SCRIPT_COMPONENT(Spell, Encounter)
