#pragma once
#include "UnitOwner.h"

///////////////////////////////////////
/// Aura
///////////////////////////////////////

class Aura;

struct AuraHandle
{
    using Stored = Aura;

    AuraHandle() = default;
    AuraHandle(const Aura& aura);
    AuraHandle(const Aura* aura);

    Aura* Load() const;

    std::string Print() const;

    static Aura* FromString(std::string_view string, script::Scriptable& base);

private:

    WorldObjectHandle object;
    uint32 auraId = 0;
};

SCRIPT_OWNER_HANDLE(Aura, script::Scriptable, AuraHandle);

SCRIPT_COMPONENT(Aura, MapArea)
SCRIPT_COMPONENT(Aura, Map)
SCRIPT_COMPONENT(Aura, Encounter)