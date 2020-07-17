#include "Instance/InstanceScript.h"
#include "InstanceMap.h"
#include "Player.h"

SCRIPT_MODULE_STATE_IMPL(Instance)
{
    SCRIPT_MODULE_PRINT_NONE
};

SCRIPT_MODULE_IMPL(Instance)
{
}

SCRIPT_FILTER_IMPL(InHeroicMode)
{
    return me.IsHeroic();
}

SCRIPT_GETTER_IMPL(ActiveFactionId)
{
    // @todo: find a cleaner solution
    const Map::PlayerList players = me.GetPlayers(); 

    if (!players.empty())
        return players.begin()->GetOTeamId();
    else
        return 0;
}
