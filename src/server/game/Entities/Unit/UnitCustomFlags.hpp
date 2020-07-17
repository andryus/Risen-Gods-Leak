#pragma once

#include "Common.h"
#include "Mask.hpp"

enum class UnitCustomFlag : uint32
{
    /**
     * Allows a special movement flag for creatures with inhabit type water, which adds a swimming animation in water. 
     */
    WaterGroundTransitAnimation             = 0x00000001,
    /**
     * Prevents flying and swimming creatures in water or air environments to use ground pathfinding. 
     */
    UseDirectPath                           = 0x00000002,
};

using UnitCustomFlagMask = ::util::Mask<UnitCustomFlag>;
