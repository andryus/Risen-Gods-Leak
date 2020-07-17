/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2010-2015 Rising Gods <http://www.rising-gods.de/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "GameObjectAI.h"

enum Blastenheimer5000Misc
{
    // Spells
    SPELL_CANNON_PREP     = 24832,
    SPELL_MAGIC_WINGS     = 42867,
    
    // Creatures
    NPC_DARKMOON_BUMMY    = 15218,

    // Gameobjects
    GO_BLASTENHIMER_ONE   = 180452,
    GO_BLASTENHIMER_TWO   = 186560,
    GO_BLASTENHIMER_THREE = 180515
};

class go_blastenheimer_5000 : public GameObjectScript
{
    public:
        go_blastenheimer_5000() : GameObjectScript("go_blastenheimer_5000") { }

        struct go_blastenheimer_5000AI : public GameObjectAI
        {
            go_blastenheimer_5000AI(GameObject* gameObject) : GameObjectAI(gameObject) { }

            void OnStateChanged(uint32 state, Unit* /*unit*/) override
            {
                if (state == GO_ACTIVATED)
                {
                    if (Player* player = go->FindNearestPlayer(10.0f))
                    {
                        player->CastSpell(player, SPELL_CANNON_PREP, true);
                        player->SetMovement(MOVE_ROOT);
                        switch (go->GetEntry())
                        {
                            case GO_BLASTENHIMER_ONE:
                                player->NearTeleportTo(-1326.310181f, 85.720284f, 129.708099f, 0.768718f, true);
                                break;
                            case GO_BLASTENHIMER_TWO:
                                player->NearTeleportTo(-1742.5616f, 5455.239f, -12.427f, 4.7070f, true);
                                break;
                            case GO_BLASTENHIMER_THREE:
                                player->NearTeleportTo(-9570.940f, -20.881f, 62.665f, 4.921f, true);
                                break;
                        }
                        respawnInFlow = true;
                    }
                    _respawnTimer = 2.5 * IN_MILLISECONDS;
                }
            }

            void UpdateAI(uint32 diff) override
            {
                if (!respawnInFlow)
                    return;

                if (_respawnTimer <= diff)
                {
                    if (Player* player = go->FindNearestPlayer(10.0f))
                    {
                        player->SetMovement(MOVE_UNROOT);
                        go->CastSpell(player, SPELL_MAGIC_WINGS);
                        respawnInFlow = false;
                    }
                }
                else _respawnTimer -= diff;
            }
            
        private:
            uint32 _respawnTimer;
            bool respawnInFlow;
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return new go_blastenheimer_5000AI(go);
        }
};

void AddSC_mulgore_rg()
{
    new go_blastenheimer_5000();
}
