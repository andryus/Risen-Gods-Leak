#include "GridSearchers.h"
#include "WorldObject.h"

Trinity::PhaseMaskCheck::PhaseMaskCheck(WorldObject const* searcher) :
    i_phaseMask(searcher->GetPhaseMask())
{
}