#pragma once

#include "Common.h"

namespace boundary
{

class AreaBoundary;

class GAME_API BoundaryMgr
{
public:
    BoundaryMgr();
    ~BoundaryMgr();

    static void LoadFromDB();

    static AreaBoundary const* FindBoundary(uint16 id);

private:
    static std::unordered_map<uint16 /*id*/, std::unique_ptr<AreaBoundary>> _boundaryStore;

};

}
