#pragma once
#include "ScriptInclude.h"

SCRIPT_EVENT_PERM(KillPlants, Instance)

enum class OrbType : uint8
{
    Telestra,
    Anomalus,
    Ormorok,
};

SCRIPT_EVENT_EX_PERM(ActivateOrb, Instance, OrbType, orb)
SCRIPT_EVENT_EX_PERM(OrbDeactived, Instance, OrbType, orb)

SCRIPT_EVENT_PERM(UnfreezeKeristrasza, Instance)
