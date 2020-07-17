#include "MuteMgr.hpp"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

class PlayerMuteRestoreScript : public PlayerScript
{
public:
    PlayerMuteRestoreScript() : PlayerScript("player_mute_restore") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        player->GetSession()->GetMuteMgr().LoadFromDb();
    }

    void OnSave(Player* player) override // includes logout
    {
        player->GetSession()->GetMuteMgr().SaveToDb();
    }
};

GAME_API void AddSC_player_mute()
{
    new PlayerMuteRestoreScript();
}
