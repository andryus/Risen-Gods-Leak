/*
* Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
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

#include "slave_pens.h"
#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "LFGMgr.h"
#include "Group.h"
#include "DBCStores.h"

class instance_slave_pens : public InstanceMapScript
{
public:
    instance_slave_pens() : InstanceMapScript("instance_slave_pens", 547) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_slave_pens_InstanceMapScript(map);
    }

    struct instance_slave_pens_InstanceMapScript : public InstanceScript
    {
        instance_slave_pens_InstanceMapScript(Map* map) : InstanceScript(map) {}

        void OnPlayerEnter(Player* player)
        {
            if(Group* group = player->GetGroup())
                if(group->isLFGGroup())
                    if(LFGDungeonEntry const* dungeon = sLFGDungeonStore.LookupEntry(sLFGMgr->GetDungeon(group->GetGUID())))
                    {
                        if(dungeon->ID == DUNGEON_ID_AHUNE && !player->HasAura(SPELL_PHASING_AHUNE))
                            player->AddAura(SPELL_PHASING_AHUNE, player);
                    }
        }

        void OnPlayerRemove(Player* player)
        {
            if(player->HasAura(SPELL_PHASING_AHUNE))
                player->RemoveAura(SPELL_PHASING_AHUNE);
        }
    };

};

void AddSC_instance_slave_pens()
{
    new instance_slave_pens();
}
