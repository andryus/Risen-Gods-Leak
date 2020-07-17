/*
* Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2010-2014 Rising Gods <http://www.rising-gods.de/>
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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ObjectDefines.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "dire_maul.h"

class instance_dire_maul : public InstanceMapScript
{
public:
    instance_dire_maul() : InstanceMapScript("instance_dire_maul", 429) { }

    struct instance_dire_maul_InstanceMapScript : public InstanceScript
    {
        instance_dire_maul_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            go_forcefield.Clear();
            go_warpwood_door.Clear();
            ForcefieldCounter = 0;
            _events.ScheduleEvent(EVENT_GO_SEARCH, 2 * IN_MILLISECONDS);
        }
      
        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
            case GO_FORCEFIELD:
                go_forcefield = go->GetGUID();
                if (GetBossState(DATA_CRYSTAL_ACTIVATED) == DONE)
                    go->UseDoorOrButton();
                break;
            case GO_CRYSTAL_GENERATOR_1:
                go_generators[0] = go->GetGUID();
                if (GetBossState(DATA_CRYSTAL_GENERATOR_1) == DONE)
                    go->UseDoorOrButton();
                break;
            case GO_CRYSTAL_GENERATOR_2:
                go_generators[1] = go->GetGUID();
                if (GetBossState(DATA_CRYSTAL_GENERATOR_2) == DONE)
                    go->UseDoorOrButton();
                break;
            case GO_CRYSTAL_GENERATOR_3:
                go_generators[2] = go->GetGUID();
                if (GetBossState(DATA_CRYSTAL_GENERATOR_3) == DONE)
                    go->UseDoorOrButton();
                break;
            case GO_CRYSTAL_GENERATOR_4:
                go_generators[3] = go->GetGUID();
                if (GetBossState(DATA_CRYSTAL_GENERATOR_4) == DONE)
                    go->UseDoorOrButton();
                break;
            case GO_CRYSTAL_GENERATOR_5:
                go_generators[4] = go->GetGUID();
                if (GetBossState(DATA_CRYSTAL_GENERATOR_5) == DONE)
                    go->UseDoorOrButton();
                break;
            case GO_WARPWOOD_DOOR:
                go_warpwood_door = go->GetGUID();
                if (GetBossState(DATA_TENDRIS_WARPWOOD) == DONE)
                    go->UseDoorOrButton();
                break;
            default:
                break;
            }
        }

        bool SetBossState(uint32 type, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(type, state))
            return false;

            switch (type)
            {
            case DATA_CRYSTAL_ACTIVATED:
            case DATA_TENDRIS_WARPWOOD:
                break;
            default:
                break;
            }
            return true;
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
            case GO_FORCEFIELD:
                return go_forcefield;
                break;
            case GO_CRYSTAL_GENERATOR_1:
                return go_generators[0];
                break;
            case GO_CRYSTAL_GENERATOR_2:
                return go_generators[1];
                break;
            case GO_CRYSTAL_GENERATOR_3:
                return go_generators[2];
                break;
            case GO_CRYSTAL_GENERATOR_4:
                return go_generators[3];
                break;
            case GO_CRYSTAL_GENERATOR_5:
                return go_generators[4];
                break;
            case GO_WARPWOOD_DOOR:
                return go_warpwood_door;
                break;
            }
            return ObjectGuid::Empty;
        }

        void Update(uint32 diff)
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_GO_SEARCH:
                    Dragonspireroomcheck();
                    _events.ScheduleEvent(EVENT_GO_SEARCH, 2 * IN_MILLISECONDS);
                    break;
                default:
                    break;
                }
            }
        }

        void OnPlayerRemove(Player* player)
        {
            player->RemoveAurasDueToSpell(SPELL_KING_OF_GORDOK);
        }

        void Dragonspireroomcheck()
        {
            GameObject* rune = NULL;

            for (uint8 i = 0; i < 5; ++i)
            {
                rune = instance->GetGameObject(go_generators[i]);
                if (!rune)
                    continue;

                if (!rune->FindNearestCreature(NPC_ARCANE_ABERRATION, 80.0f) && rune->GetGoState() == GO_STATE_READY)
                {
                    rune->UseDoorOrButton();

                    switch (rune->GetEntry())
                    {
                    case GO_CRYSTAL_GENERATOR_1:
                        SetBossState(DATA_CRYSTAL_GENERATOR_1, DONE);
                        ++ForcefieldCounter;
                        break;
                    case GO_CRYSTAL_GENERATOR_2:
                        SetBossState(DATA_CRYSTAL_GENERATOR_2, DONE);
                        ++ForcefieldCounter;
                        break;
                    case GO_CRYSTAL_GENERATOR_3:
                        SetBossState(DATA_CRYSTAL_GENERATOR_3, DONE);
                        ++ForcefieldCounter;
                        break;
                    case GO_CRYSTAL_GENERATOR_4:
                        SetBossState(DATA_CRYSTAL_GENERATOR_4, DONE);
                        ++ForcefieldCounter;
                        break;
                    case GO_CRYSTAL_GENERATOR_5:
                        SetBossState(DATA_CRYSTAL_GENERATOR_5, DONE);
                        ++ForcefieldCounter;
                        break;
                    default:
                        break;
                    }
                }
            }

            if (ForcefieldCounter == 5)
            {
                if (GameObject* forcefield = instance->GetGameObject(go_forcefield))
                    forcefield->UseDoorOrButton();
                SetBossState(DATA_CRYSTAL_ACTIVATED, DONE);
            }
        }

    protected:
        EventMap _events;
        ObjectGuid go_forcefield;
        ObjectGuid go_warpwood_door;
        ObjectGuid go_generators[5];
        uint8 ForcefieldCounter;
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_dire_maul_InstanceMapScript(map);
    }
};

void AddSC_instance_dire_maul()
{
    new instance_dire_maul();
}
