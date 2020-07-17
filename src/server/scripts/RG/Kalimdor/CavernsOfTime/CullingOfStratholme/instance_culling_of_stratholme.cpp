/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "InstanceScript.h"
#include "CreatureTextMgr.h"
#include "culling_of_stratholme.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "SpellInfo.h"
#include "Packets/WorldPacket.h"

#define MAX_ENCOUNTER 5

/* Culling of Stratholme encounters:
0 - Meathook
1 - Salramm the Fleshcrafter
2 - Chrono-Lord Epoch
3 - Mal'Ganis
4 - Infinite Corruptor (Heroic only)
*/

enum Texts
{
    SAY_CRATES_COMPLETED = 0,
};

const char* zombiefestWarnings[12] =
{
    "|cFFFFFC00 [Zombiefest!] |cFF00FFFF Gestartet.",
    "|cFFFFFC00 [Zombiefest!] |cFFFE6C00 Fortschritt: 10/100.",
    "|cFFFFFC00 [Zombiefest!] |cFFFF9000 Fortschritt: 20/100.",
    "|cFFFFFC00 [Zombiefest!] |cFFFDB900 Fortschritt: 30/100.",
    "|cFFFFFC00 [Zombiefest!] |cFFFFCC00 Fortschritt: 40/100.",
    "|cFFFFFC00 [Zombiefest!] |cFFFFF600 Fortschritt: 50/100.",
    "|cFFFFFC00 [Zombiefest!] |cFFDDFE00 Fortschritt: 60/100.",
    "|cFFFFFC00 [Zombiefest!] |cFFC6FF00 Fortschritt: 70/100.",
    "|cFFFFFC00 [Zombiefest!] |cFF96FF00 Fortschritt: 80/100.",
    "|cFFFFFC00 [Zombiefest!] |cFF6CFF00 Fortschritt: 90/100.",
    "|cFFFFFC00 [Zombiefest!] |cFF33FF00 Erfolgreich..",
    "|cFFFFFC00 [Zombiefest!] |cFFFF0000 Fehlgeschlagen.",
};

class instance_culling_of_stratholme : public InstanceMapScript
{
    public:
        instance_culling_of_stratholme() : InstanceMapScript("instance_culling_of_stratholme", 595) { }

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_culling_of_stratholme_InstanceMapScript(map);
        }

        struct instance_culling_of_stratholme_InstanceMapScript : public InstanceScript
        {
            instance_culling_of_stratholme_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                _arthasGUID.Clear();
                _meathookGUID.Clear();
                _salrammGUID.Clear();
                _epochGUID.Clear();
                _malGanisGUID.Clear();
                _infiniteGUID.Clear();
                _shkafGateGUID.Clear();
                _malGanisGate1GUID.Clear();
                _malGanisGate2GUID.Clear();
                _exitGateGUID.Clear();
                _malGanisChestGUID.Clear();
                _genericBunnyGUID.Clear();
                _lordaeronCrierGUID.Clear();
                _artasStepUi = 0;
                memset(&_encounterState[0], 0, sizeof(uint32) * MAX_ENCOUNTER);
                _crateCount = 0;
                _zombieFest = 0;
                _sendZombiefestData = 0;
                _killedZombieCount = 0;
                _zombieTimer = 60000;
                _eventTimer = 1500000;
                _lastTimer = 1500000;
                _zombiePhaseTimer = 5000;
            }

            bool IsEncounterInProgress() const
            {
                for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                    if (_encounterState[i] == IN_PROGRESS)
                        return true;

                return false;
            }

            void FillInitialWorldStates(WorldPacket& data)
            {
                data << uint32(WORLDSTATE_SHOW_CRATES) << uint32(1);
                data << uint32(WORLDSTATE_CRATES_REVEALED) << uint32(_crateCount);
                data << uint32(WORLDSTATE_WAVE_COUNT) << uint32(0);
                data << uint32(WORLDSTATE_TIME_GUARDIAN) << uint32(25);
                data << uint32(WORLDSTATE_TIME_GUARDIAN_SHOW) << uint32(0);
            }

            void OnCreatureCreate(Creature* creature)
            {
                switch (creature->GetEntry())
                {
                    case NPC_ARTHAS:
                        _arthasGUID = creature->GetGUID();
                        break;
                    case NPC_MEATHOOK:
                        _meathookGUID = creature->GetGUID();
                        break;
                    case NPC_SALRAMM:
                        _salrammGUID = creature->GetGUID();
                        break;
                    case NPC_EPOCH:
                        _epochGUID = creature->GetGUID();
                        break;
                    case NPC_MAL_GANIS:
                        _malGanisGUID = creature->GetGUID();
                        break;
                    case NPC_INFINITE:
                        _infiniteGUID = creature->GetGUID();
                        break;
                    case NPC_GENERIC_BUNNY:
                        _genericBunnyGUID = creature->GetGUID();
                        break;
                    case NPC_CITY_MAN:
                    case NPC_CITY_MAN2:
                    case NPC_CITY_MAN3:
                    case NPC_CITY_MAN4:
                    case 30570: case 31020: case 31021: case 31022: case 31023: // Some individual npcs...
                    case 31025: case 31018: case 31027: case 31028: case 30994: case 31019:
                        _citizensList.push_back(creature->GetGUID());
                        break;
                    case NPC_LORDAERON_CRIER:
                        _lordaeronCrierGUID = creature->GetGUID();
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GO_SHKAF_GATE:
                        _shkafGateGUID = go->GetGUID();
                        break;
                    case GO_MALGANIS_GATE_1:
                        _malGanisGate1GUID = go->GetGUID();
                        break;
                    case GO_MALGANIS_GATE_2:
                        _malGanisGate2GUID = go->GetGUID();
                        break;
                    case GO_EXIT_GATE:
                        _exitGateGUID = go->GetGUID();
                        if (_encounterState[3] == DONE)
                            HandleGameObject(_exitGateGUID, true);
                        break;
                    case GO_MALGANIS_CHEST_N:
                    case GO_MALGANIS_CHEST_H:
                        _malGanisChestGUID = go->GetGUID();
                        if (_encounterState[3] == DONE)
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_INTERACT_COND);                        
                        break;
                }
            }

            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_MEATHOOK_EVENT:
                        _encounterState[0] = data;
                        break;
                    case DATA_SALRAMM_EVENT:
                        if(data == DONE)
                        {
                            DoUpdateWorldState(WORLDSTATE_WAVE_COUNT, 0);
                            if(ArthasNeedsTeleport())
                                if(Creature* arthas = instance->GetCreature(_arthasGUID))
                                    arthas->AI()->SetData(1, 0);
                        }
                        _encounterState[1] = data;
                        break;
                    case DATA_EPOCH_EVENT:
                        _encounterState[2] = data;
                        break;
                    case DATA_MAL_GANIS_EVENT:
                        _encounterState[3] = data;
                        switch (_encounterState[3])
                        {
                            case NOT_STARTED:
                                HandleGameObject(_malGanisGate2GUID, true);
                                break;
                            case IN_PROGRESS:
                                HandleGameObject(_malGanisGate2GUID, false);
                                break;
                            case DONE:
                                HandleGameObject(_exitGateGUID, true);
                                HandleGameObject(_malGanisGate2GUID, true);
                                if (GameObject* go = instance->GetGameObject(_malGanisChestGUID))
                                    go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);

                                instance->SummonCreature(NPC_CHROMIE_3, ChromieExitFlyPos);

                                for (Player& player : instance->GetPlayers())
                                    player.KilledMonsterCredit(31006);
                                break;
                        }
                        break;
                    case DATA_INFINITE_EVENT:
                        _encounterState[4] = data;
                        switch (data)
                        {
                            case DONE:
                                DoCompleteAchievement(ACHIEVEMENT_CULLING_TIME);
                                DoUpdateWorldState(WORLDSTATE_TIME_GUARDIAN_SHOW, 0);
                                break;
                            case FAIL:
                                DoUpdateWorldState(WORLDSTATE_TIME_GUARDIAN_SHOW, 0);
                                if(Creature* infinite = instance->GetCreature(_infiniteGUID))
                                    infinite->AI()->DoAction(ACTION_DESPAWN);
                                break;
                            case IN_PROGRESS:
                                DoUpdateWorldState(WORLDSTATE_TIME_GUARDIAN_SHOW, 1);
                                DoUpdateWorldState(WORLDSTATE_TIME_GUARDIAN, 25);
                                instance->SummonCreature(NPC_TIME_GUARDIAN, TimeGuardianSummonPos);
                                instance->SummonCreature(NPC_INFINITE, InfiniteSummonPos);
                                break;
                        }
                        break;
                    case DATA_ARTHAS_EVENT:
                        if (data == FAIL)
                        {
                            if (Creature* deadArthas = instance->GetCreature(_arthasGUID))
                            {
                                deadArthas->DespawnOrUnsummon(10000);
                                int index;

                                if(_artasStepUi >= 83) // Before last run
                                    index = 2;
                                else if (_artasStepUi >= 60) // Before the council
                                    index = 1;
                                else // entrance of city
                                    index = 0;

                                if(Creature* newArthas = instance->SummonCreature(NPC_ARTHAS, ArthasSpawnPositions[index]))
                                    newArthas->AI()->SetData(0, pow(2.0, index));
                            }
                        }
                        break;
                    case DATA_CRATE_COUNT:
                        _crateCount = data;
                        DoUpdateWorldState(WORLDSTATE_CRATES_REVEALED, _crateCount);
                        if (_crateCount >= 5)
                        {
                            // Summon Chromie and global whisper
                            if (Creature* chromie = instance->SummonCreature(NPC_CHROMIE_2, ChromieEntranceSummonPos))
                            {
                                for (Player& player : instance->GetPlayers())
                                {
                                    player.KilledMonsterCredit(30996);
                                    chromie->AI()->Talk(SAY_CRATES_COMPLETED, &player);
                                }
                            }
                            DoUpdateWorldState(WORLDSTATE_SHOW_CRATES, 0);
                            DoUpdateWorldState(WORLDSTATE_SHOW_CRATES, 0);
                        }
                        break;
                    case DATA_TRANSFORM_CITIZENS:
                        switch(data)
                        {
                            case SPECIAL: // Respawn Zombies
                                while (!_zombiesList.empty())
                                {
                                    Creature *summon = instance->GetCreature(*_zombiesList.begin());
                                    if (!summon)
                                        _zombiesList.erase(_zombiesList.begin());
                                    else
                                    {
                                        _zombiesList.erase(_zombiesList.begin());
                                        if (TempSummon* summ = summon->ToTempSummon())
                                            summ->UnSummon();
                                        else
                                            summon->DisappearAndDie();
                                    }
                                }
                            case IN_PROGRESS: // Transform Citizens
                                for (std::list<ObjectGuid>::iterator itr = _citizensList.begin(); itr != _citizensList.end(); ++itr)
                                    if(Creature* citizen = instance->GetCreature((*itr)))
                                    {
                                        citizen->SetPhaseMask(2, true);
                                        if (Creature* risenZombie = citizen->FindNearestCreature(NPC_ZOMBIE, 2.0f, true))
                                        {
                                            risenZombie->SetPhaseMask(1, true);
                                            _zombiesList.push_back(risenZombie->GetGUID());
                                        }
                                    }
                                break;
                        }
                        break;
                    case DATA_ZOMBIEFEST:
                        if (!instance->IsHeroic() || GetData(DATA_ZOMBIEFEST) == DONE)
                            break;

                        switch(data)
                        {
                            case DONE:
                                DoCompleteAchievement(ACHIEVEMENT_ZOMBIEFEST);
                                if (_sendZombiefestData)
                                    DoSendNotifyToInstance(zombiefestWarnings[10]);
                                _zombieFest = data;
                                break;
                            case IN_PROGRESS:
                                if (_sendZombiefestData)
                                    DoSendNotifyToInstance(zombiefestWarnings[0]);
                                _zombieFest = data;
                                break;
                            case FAIL:
                                if (_sendZombiefestData)
                                    DoSendNotifyToInstance(zombiefestWarnings[11]);
                                _killedZombieCount = 0;
                                _zombieTimer = 60000;
                                _zombieFest = data;
                                break;
                            case SPECIAL:
                                _killedZombieCount++;
                                if(_killedZombieCount == 1)
                                    SetData(DATA_ZOMBIEFEST, IN_PROGRESS);
                                else if(_killedZombieCount >= 100 && GetData(DATA_ZOMBIEFEST) == IN_PROGRESS)
                                    SetData(DATA_ZOMBIEFEST, DONE);
                                else
                                {
                                    if(_killedZombieCount%10 == 0 && _sendZombiefestData)
                                        DoSendNotifyToInstance(zombiefestWarnings[_killedZombieCount/10]);
                                }
                                break;
                        }
                        break;
                    case DATA_ZOMBIEFEST_STATUS:
                        _sendZombiefestData = data;
                        break;
                    case DATA_ARTHAS_STEP:
                        _artasStepUi = data;
                        return;
                }

                if (data == DONE || data == FAIL)
                    SaveToDB();
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_MEATHOOK_EVENT:
                        return _encounterState[0];
                    case DATA_SALRAMM_EVENT:
                        return _encounterState[1];
                    case DATA_EPOCH_EVENT:
                        return _encounterState[2];
                    case DATA_MAL_GANIS_EVENT:
                        return _encounterState[3];
                    case DATA_INFINITE_EVENT:
                        return _encounterState[4];
                    case DATA_CRATE_COUNT:
                        return _crateCount;
                    case DATA_ZOMBIEFEST:
                        return _zombieFest;
                    case DATA_ZOMBIEFEST_STATUS:
                        return _sendZombiefestData;

                }
                return 0;
            }

            ObjectGuid GetGuidData(uint32 identifier) const override
            {
                switch (identifier)
                {
                    case DATA_ARTHAS:
                        return _arthasGUID;
                    case DATA_MEATHOOK:
                        return _meathookGUID;
                    case DATA_SALRAMM:
                        return _salrammGUID;
                    case DATA_EPOCH:
                        return _epochGUID;
                    case DATA_MAL_GANIS:
                        return _malGanisGUID;
                    case DATA_INFINITE:
                        return _infiniteGUID;
                    case DATA_CRIER:
                        return _lordaeronCrierGUID;
                    case DATA_SHKAF_GATE:
                        return _shkafGateGUID;
                    case DATA_MAL_GANIS_GATE_1:
                        return _malGanisGate1GUID;
                    case DATA_MAL_GANIS_GATE_2:
                        return _malGanisGate2GUID;
                    case DATA_EXIT_GATE:
                        return _exitGateGUID;
                    case DATA_MAL_GANIS_CHEST:
                        return _malGanisChestGUID;
                }
                return ObjectGuid::Empty;
            }

            void Update(uint32 diff)
            {
                if(GetData(DATA_ZOMBIEFEST) == IN_PROGRESS)
                {
                    if (_zombieTimer <= diff)
                        SetData(DATA_ZOMBIEFEST, FAIL);
                    else _zombieTimer -= diff;
                }

                if(GetData(DATA_INFINITE_EVENT) == IN_PROGRESS)
                {
                    if(_eventTimer <= diff)
                        SetData(DATA_INFINITE_EVENT, FAIL);
                    else _eventTimer -= diff;

                    if(_eventTimer < _lastTimer - 60000)
                    {
                        _lastTimer = _eventTimer;
                        uint32 tMinutes = _eventTimer / 60000;
                        DoUpdateWorldState(WORLDSTATE_TIME_GUARDIAN, tMinutes);
                    }
                }

                if (GetData(DATA_EPOCH_EVENT) != DONE)
                {
                    if (_zombiePhaseTimer <= diff)
                    {
                        for (std::list<ObjectGuid>::iterator itr = _zombiesList.begin(); itr != _zombiesList.end(); ++itr)
                        {
                            if(Creature* zombie = instance->GetCreature((*itr)))
                                zombie->SetPhaseMask(1, true);
                        }
                    }
                    else _zombiePhaseTimer -= diff;
                }
            }

            std::string GetSaveData()
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "C S " << _encounterState[0] << ' ' << _encounterState[1] << ' '
                    << _encounterState[2] << ' ' << _encounterState[3] << ' ' << _encounterState[4];

                OUT_SAVE_INST_DATA_COMPLETE;
                return saveStream.str();
            }

            void Load(const char* in)
            {
                if (!in)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(in);

                char dataHead1, dataHead2;
                uint16 data0, data1, data2, data3, data4;

                std::istringstream loadStream(in);
                loadStream >> dataHead1 >> dataHead2 >> data0 >> data1 >> data2 >> data3 >> data4;

                if (dataHead1 == 'C' && dataHead2 == 'S')
                {
                    _encounterState[0] = data0;
                    _encounterState[1] = data1;
                    _encounterState[2] = data2;
                    _encounterState[3] = data3;
                    _encounterState[4] = data4;

                    for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                        if (_encounterState[i] == IN_PROGRESS)
                            _encounterState[i] = NOT_STARTED;

                }
                else
                    OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }

            void OnPlayerRemove(Player* player)
            {
                if (player->GetMapId() == 595 && player->GetPhaseMask() == 2)
                    player->SetPhaseMask(PHASEMASK_NORMAL, true);
            }

        private:
            ObjectGuid _arthasGUID;
            ObjectGuid _meathookGUID;
            ObjectGuid _salrammGUID;
            ObjectGuid _epochGUID;
            ObjectGuid _malGanisGUID;
            ObjectGuid _infiniteGUID;
            ObjectGuid _shkafGateGUID;
            ObjectGuid _malGanisGate1GUID;
            ObjectGuid _malGanisGate2GUID;
            ObjectGuid _exitGateGUID;
            ObjectGuid _malGanisChestGUID;
            ObjectGuid _genericBunnyGUID;
            ObjectGuid _lordaeronCrierGUID;
            uint32 _artasStepUi;
            uint32 _encounterState[MAX_ENCOUNTER];
            uint32 _crateCount;
            uint32 _zombieFest;
            uint32 _sendZombiefestData;
            uint8 _killedZombieCount;
            uint32 _zombieTimer;
            std::list<Position> _citizensPosList;
            std::list<ObjectGuid> _citizensList;
            std::list<ObjectGuid> _zombiesList;
            uint32 _eventTimer;
            uint32 _lastTimer;
            uint32 _zombiePhaseTimer;

            bool ArthasNeedsTeleport()
            {
                if(Creature* arthas = instance->GetCreature(_arthasGUID))
                {
                    for (Player& player : instance->GetPlayers())
                        if(player.GetDistance(arthas) >= 210.0f)
                            return true;
                }
                return false;
            }
        };
};

void AddSC_instance_culling_of_stratholme()
{
    new instance_culling_of_stratholme();
}
