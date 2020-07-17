/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
#include "InstanceScript.h"
#include "ruins_of_ahnqiraj.h"

/*
Ruins of Ahn'Qiraj Encounter:
1 - boss_kurinnaxx.cpp
2 - boss_rajaxx.cpp
3 - boss_moam.cpp
4 - boss_buru.cpp
5 - boss_ayamiss.cpp
6 - boss_ossirian.cpp
*/

class instance_ruins_of_ahnqiraj : public InstanceMapScript
{
    public:
        instance_ruins_of_ahnqiraj() : InstanceMapScript("instance_ruins_of_ahnqiraj", 509) { }

        struct instance_ruins_of_ahnqiraj_InstanceMapScript : public InstanceScript
        {
            instance_ruins_of_ahnqiraj_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(MAX_ENCOUNTER);

                _kurinaxxGUID  .Clear();
                _rajaxxGUID    .Clear();
                _moamGUID      .Clear();
                _buruGUID      .Clear();
                _ayamissGUID   .Clear();
                _ossirianGUID  .Clear();
                _andronovGUID  .Clear();
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_KURINAXX:
                        _kurinaxxGUID = creature->GetGUID();
                        break;
                    case NPC_RAJAXX:
                        _rajaxxGUID = creature->GetGUID();
                        break;
                    case NPC_MOAM:
                        _moamGUID = creature->GetGUID();
                        break;
                    case NPC_BURU:
                        _buruGUID = creature->GetGUID();
                        break;
                    case NPC_AYAMISS:
                        _ayamissGUID = creature->GetGUID();
                        break;
                    case NPC_OSSIRIAN:
                        _ossirianGUID = creature->GetGUID();
                        break;
                    case NPC_ANDRONOV:
                        _andronovGUID = creature->GetGUID();
                        break;
                }
            }

            bool SetBossState(uint32 bossId, EncounterState state)
            {
                if (!InstanceScript::SetBossState(bossId, state))
                    return false;

                return true;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_KURINNAXX:
                        return _kurinaxxGUID;
                    case DATA_RAJAXX:
                        return _rajaxxGUID;
                    case DATA_MOAM:
                        return _moamGUID;
                    case DATA_BURU:
                        return _buruGUID;
                    case DATA_AYAMISS:
                        return _ayamissGUID;
                    case DATA_OSSIRIAN:
                        return _ossirianGUID;
                    case DATA_ANDRONOV:
                        return _andronovGUID;
                }

                return ObjectGuid::Empty;
            }

            void OnPlayerEnter(Player* player) override
            {
                if (GetBossState(DATA_KURINNAXX) == DONE && GetBossState(DATA_RAJAXX) != DONE)
                if (!ObjectAccessor::GetCreature(*player, _andronovGUID))
                    player->SummonCreature(NPC_ANDRONOV, -8877.254883f, 1645.267578f, 21.386303f, 4.669808f, TEMPSUMMON_CORPSE_DESPAWN, 600000000);
            }            

        private:
            ObjectGuid _kurinaxxGUID;
            ObjectGuid _rajaxxGUID;
            ObjectGuid _moamGUID;
            ObjectGuid _buruGUID;
            ObjectGuid _ayamissGUID;
            ObjectGuid _ossirianGUID;
            ObjectGuid _andronovGUID;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_ruins_of_ahnqiraj_InstanceMapScript(map);
        }
};

void AddSC_instance_ruins_of_ahnqiraj()
{
    new instance_ruins_of_ahnqiraj();
}
