/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
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
#include "pit_of_saron.h"
#include "Player.h"

// position for Jaina and Sylvanas
Position const EventLeaderPos2 = { 1066.864746f, 92.282631f, 631.283020f, 2.084293f };

DoorData const Doors[] =
{
    {GO_ICE_WALL,                         DATA_GARFROST,  DOOR_TYPE_PASSAGE },
    {GO_ICE_WALL,                         DATA_ICK,       DOOR_TYPE_PASSAGE },
    {GO_HALLS_OF_REFLECTION_PORTCULLIS,   DATA_TYRANNUS,  DOOR_TYPE_PASSAGE },
    { 0,                                  0,              DOOR_TYPE_ROOM    }  // END
};

Position const TrashPos1 = { 780.310974f, -27.701401f, 508.365997f, 0.f };
Position const TrashPos2 = { 849.631226f, 1.170173f, 510.153442f, 0.f };
Position const TrashPos3 = { 631.137024f, 111.806999f, 510.575012f, 0.f };

const uint32 SPELL_GREATER_INVISIBILITY = 34426;

enum TrashGroups
{
    GROUP_SPAWN_AFTER_GARFROST           = 0,
    GROUP_SPAWN_AFTER_ICK                = 1,
    GROUP_DESPAWN_AFTER_ANY              = 2,
    GROUP_DESPAWN_AFTER_ICK              = 3,
    GROUP_DESPAWN_AFTER_GARFROST         = 4,
    TRASH_GROUP_COUNT,
};

class instance_pit_of_saron : public InstanceMapScript
{
    public:
        instance_pit_of_saron() : InstanceMapScript(PoSScriptName, 658) { }

        struct instance_pit_of_saron_InstanceScript : public InstanceScript
        {
            instance_pit_of_saron_InstanceScript(Map* map) : InstanceScript(map)
            {
                SetBossNumber(EncounterCount);
                LoadDoorData(Doors);
                _garfrostGUID.Clear();
                _krickGUID.Clear();
                _ickGUID.Clear();
                _tyrannusGUID.Clear();
                _rimefangGUID.Clear();
                _jainaOrSylvanas1GUID.Clear();
                _jainaOrSylvanas2GUID.Clear();
                _teamInInstance = 0;
                DoNotLookUp = 0;
            }

            WeatherState GetWeatherStateInInstance(uint32 /*zoneID*/)
            {
                return WEATHER_STATE_SNOW_FLAKES;
            }

            float GetWeatherGradeInInstance(uint32 /*zoneID*/)
            {
                return 0.28f;
            }

            void Initalisize()
            {
                SlaveGUIDs.clear();
            }

            void OnPlayerEnter(Player* player)
            {
                if (!_teamInInstance)
                    _teamInInstance = player->GetTeam();
            }

            void OnCreatureCreate(Creature* creature)
            {
                if (!_teamInInstance)
                {
                    if (!instance->GetPlayers().empty())
                        _teamInInstance = instance->GetPlayers().begin()->GetTeam();
                }

                creature->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, true);
                
                switch (creature->GetEntry())
                {
                    case NPC_HORDE_SLAVE_1:
                    case NPC_HORDE_SLAVE_2:
                    case NPC_HORDE_SLAVE_3:
                    case NPC_HORDE_SLAVE_4:
                    case NPC_GORKUN_IRONSKULL_2:
                    case NPC_SYLVANAS_PART1:
                    case NPC_KORALEN:
                    case NPC_KILARA:
                    case NPC_WRATHBONE_LABORER:
                        if (creature->HasAura(SPELL_GREATER_INVISIBILITY) && creature->IsWithinDist2d(&TrashPos2, 50.f))
                        {
                            SpecialTrashGUIDs[GROUP_SPAWN_AFTER_ICK].push_back(creature->GetGUID());
                            SpecialTrashGUIDs[GROUP_DESPAWN_AFTER_GARFROST].push_back(creature->GetGUID());
                        }
                        else if (creature->GetEntry() == NPC_GORKUN_IRONSKULL_2 && creature->IsWithinDist2d(&TrashPos3, 50.f))
                            SpecialTrashGUIDs[GROUP_DESPAWN_AFTER_ANY].push_back(creature->GetGUID());
                        else if (creature->IsWithinDist2d(&TrashPos2, 50.f))
                            SpecialTrashGUIDs[GROUP_DESPAWN_AFTER_ICK].push_back(creature->GetGUID());
                        break;
                    default:
                        break;
                }

                switch (creature->GetEntry())
                {
                    case NPC_GARFROST:
                        _garfrostGUID = creature->GetGUID();
                        break;
                    case NPC_KRICK:
                        _krickGUID = creature->GetGUID();
                        break;
                    case NPC_ICK:
                        _ickGUID = creature->GetGUID();
                        break;
                    case NPC_TYRANNUS:
                        _tyrannusGUID = creature->GetGUID();
                        break;
                    case NPC_RIMEFANG:
                        _rimefangGUID = creature->GetGUID();
                        break;
                    case NPC_TYRANNUS_EVENTS:
                        _tyrannusEventGUID = creature->GetGUID();
                        break;
                    case NPC_SYLVANAS_PART1:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_JAINA_PART1);
                        if (!creature->HasAura(SPELL_GREATER_INVISIBILITY))
                            _jainaOrSylvanas1GUID = creature->GetGUID();
                        break;
                    case NPC_SYLVANAS_PART2:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_JAINA_PART2);
                        _jainaOrSylvanas2GUID = creature->GetGUID();
                        break;
                    case NPC_KILARA:
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_ELANDRA);
                        break;
                    case NPC_KORALEN:
                        if (_teamInInstance == ALLIANCE)
                        {
                            creature->UpdateEntry(NPC_KORLAEN);
                            creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                        }                           
                        break;
                    case NPC_CHAMPION_1_HORDE:
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_CHAMPION_1_ALLIANCE);
                        break;
                    case NPC_CHAMPION_2_HORDE:
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_CHAMPION_2_ALLIANCE);
                        break;
                    case NPC_CHAMPION_3_HORDE: // No 3rd set for Alliance?
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_CHAMPION_2_ALLIANCE);
                        break;
                    case NPC_HORDE_SLAVE_1:
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_ALLIANCE_SLAVE_1);
                        break;
                    case NPC_HORDE_SLAVE_2:
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_ALLIANCE_SLAVE_2);
                        break;
                    case NPC_HORDE_SLAVE_3:
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_ALLIANCE_SLAVE_3);
                        break;
                    case NPC_HORDE_SLAVE_4:
                        if (_teamInInstance == ALLIANCE)
                           creature->UpdateEntry(NPC_ALLIANCE_SLAVE_4);
                        break;
                    case NPC_FREED_SLAVE_1_HORDE:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_FREED_SLAVE_1_ALLIANCE);
                        break;
                    case NPC_FREED_SLAVE_2_HORDE:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_FREED_SLAVE_2_ALLIANCE);
                        break;
                    case NPC_FREED_SLAVE_3_HORDE:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_FREED_SLAVE_3_ALLIANCE);
                        break;
                    case NPC_RESCUED_SLAVE_HORDE:
                    case NPC_RESCUED_SLAVE_ALLIANCE:
                        creature->DespawnOrUnsummon(1);
                        SpecialTrashGUIDs[0].push_back(creature->GetGUID());
                        break;
                    case NPC_GORKUN_IRONSKULL_1:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_MARTIN_VICTUS_2);
                        break;
                    case NPC_GORKUN_IRONSKULL_2:
                        if (_teamInInstance == ALLIANCE)
                            creature->UpdateEntry(NPC_MARTIN_VICTUS_1);
                        break;
                    case NPC_GEIST_AMBUSHER:
                        if (creature->IsAlive())
                        {
                            SlaveGUIDs.insert(creature->GetGUID());
                            creature->DespawnOrUnsummon(1);
                            SpecialTrashGUIDs[GROUP_SPAWN_AFTER_GARFROST].push_back(creature->GetGUID());
                            SpecialTrashGUIDs[GROUP_DESPAWN_AFTER_ICK].push_back(creature->GetGUID());
                        }
                        break;
                    case NPC_DEATHWHISPER_NECROLYTE:
                    case NPC_SKELETAL_SLAVE:
                        if (creature->IsWithinDist2d(&TrashPos1, 50.f))
                            SpecialTrashGUIDs[GROUP_DESPAWN_AFTER_ANY].push_back(creature->GetGUID());
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GO_ICE_WALL:
                    case GO_HALLS_OF_REFLECTION_PORTCULLIS:
                        AddDoor(go, true);
                        break;
                    case GO_BALL_AND_CHAIN:
                        if (go->IsWithinDist2d(&TrashPos2, 50.f))
                            ChainGUIDs.push_back(go->GetGUID());
                        break;
                }
            }

            void OnGameObjectRemove(GameObject* go)
            {
                switch (go->GetEntry())
                {
                    case GO_ICE_WALL:
                    case GO_HALLS_OF_REFLECTION_PORTCULLIS:
                        AddDoor(go, false);
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state)
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;
            
                switch (type)
                {
                    case DATA_TYRANNUS:
                        if (state == DONE)
                        {
                            if (Creature* summoner = instance->GetCreature(_tyrannusGUID))
                            {
                                if (_teamInInstance == ALLIANCE)
                                    summoner->SummonCreature(NPC_JAINA_PART2, EventLeaderPos2, TEMPSUMMON_MANUAL_DESPAWN);
                                else
                                    summoner->SummonCreature(NPC_SYLVANAS_PART2, EventLeaderPos2, TEMPSUMMON_MANUAL_DESPAWN);
                            }
                        }
                        break;
                    case DATA_GARFROST:
                        if (state == DONE)
                        {
                            DoForTrashGroup(GROUP_SPAWN_AFTER_GARFROST, [](Creature* npc) { npc->Respawn(); });
                            DoForTrashGroup(GROUP_DESPAWN_AFTER_GARFROST, [](Creature* npc) { npc->AddAura(SPELL_GREATER_INVISIBILITY, npc); });
                            DoForTrashGroup(GROUP_DESPAWN_AFTER_ANY, [](Creature* npc) { npc->DespawnOrUnsummon(); });
                        }
                        break;
                    case DATA_ICK:
                        if (state == DONE)
                        {
                            for (ObjectGuid guid : ChainGUIDs)
                                if (GameObject* go = instance->GetGameObject(guid))
                                    go->DespawnOrUnsummon();
                            bool GarfrostDone = (GetBossState(DATA_GARFROST) == DONE);
                            DoForTrashGroup(GROUP_SPAWN_AFTER_ICK, [GarfrostDone](Creature* npc)
                            {
                                if (GarfrostDone)
                                {
                                    switch (npc->GetEntry())
                                    {
                                        case NPC_HORDE_SLAVE_1:
                                            npc->UpdateEntry(NPC_ARMORED_HORDE_SLAVE_1);
                                            break;
                                        case NPC_HORDE_SLAVE_2:
                                            npc->UpdateEntry(NPC_ARMORED_HORDE_SLAVE_2);
                                            break;
                                        case NPC_HORDE_SLAVE_4:
                                            npc->UpdateEntry(NPC_ARMORED_HORDE_SLAVE_4);
                                            break;
                                        case NPC_ALLIANCE_SLAVE_1:
                                            npc->UpdateEntry(NPC_ARMORED_ALLIANCE_SLAVE_1);
                                            break;
                                        case NPC_ALLIANCE_SLAVE_2:
                                            npc->UpdateEntry(NPC_ARMORED_ALLIANCE_SLAVE_2);
                                            break;
                                        case NPC_ALLIANCE_SLAVE_4:
                                            npc->UpdateEntry(NPC_ARMORED_ALLIANCE_SLAVE_4);
                                            break;
                                        case NPC_GORKUN_IRONSKULL_2:
                                            npc->UpdateEntry(NPC_GORKUN_IRONSKULL_1);
                                            break;
                                        case NPC_GORKUN_IRONSKULL_1:
                                            npc->UpdateEntry(NPC_MARTIN_VICTUS_2);
                                            break;
                                        default:
                                            break;
                                    }
                                }

                                npc->RemoveAurasDueToSpell(SPELL_GREATER_INVISIBILITY);
                                if (npc->GetUInt32Value(UNIT_NPC_EMOTESTATE) == EMOTE_STATE_WORK_MINING)
                                    npc->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                            });
                            DoForTrashGroup(GROUP_DESPAWN_AFTER_ICK, [](Creature* npc) { npc->DespawnOrUnsummon(); });
                            DoForTrashGroup(GROUP_DESPAWN_AFTER_ANY, [](Creature* npc) { npc->DespawnOrUnsummon(); });
                        }
                        break;
                    default:
                        break;
                }

                return true;
            }

            void SetGuidData(uint32 type, ObjectGuid data) override
            {
                if (type == DATA_SLAVE_DIED)
                    SlaveGUIDs.erase(data);
            }

            void SetData(uint32 type, uint32 data)
            {
                switch (type)
                {
                    case DATA_DO_NOT_LOOK_UP:
                        DoNotLookUp = data;
                        SaveToDB();
                        break;
                    default:
                        break;
                }
            }

            uint32 GetData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_TEAM_IN_INSTANCE:
                        return _teamInInstance;
                    case DATA_ALIVE_SLAVE:
                        return SlaveGUIDs.size();
                    case DATA_DO_NOT_LOOK_UP:
                        return DoNotLookUp;
                    default:
                        break;
                }

                return 0;
            }

            ObjectGuid GetGuidData(uint32 type) const
            {
                switch (type)
                {
                    case DATA_GARFROST:
                        return _garfrostGUID;
                    case DATA_KRICK:
                        return _krickGUID;
                    case DATA_ICK:
                        return _ickGUID;
                    case DATA_TYRANNUS:
                        return _tyrannusGUID;
                    case DATA_RIMEFANG:
                        return _rimefangGUID;
                    case DATA_TYRANNUS_EVENT:
                        return _tyrannusEventGUID;
                    case DATA_JAINA_SYLVANAS_1:
                        return _jainaOrSylvanas1GUID;
                    case DATA_JAINA_SYLVANAS_2:
                        return _jainaOrSylvanas2GUID;
                    default:
                        break;
                }

                return ObjectGuid::Empty;
            }

            void DoForTrashGroup(TrashGroups group, std::function<void(Creature*)> fnc)
            {
                for (ObjectGuid guid : SpecialTrashGUIDs[group])
                    if (Creature* creature = instance->GetCreature(guid))
                        fnc(creature);
            }

            void WriteSaveDataMore(std::ostringstream& data) override
            {
                data << DoNotLookUp;
            }

            void ReadSaveDataMore(std::istringstream& data) override
            {
                uint32 temp = 0;
                data >> temp;
                data >> DoNotLookUp;
            }

        private:
            ObjectGuid _garfrostGUID;
            ObjectGuid _krickGUID;
            ObjectGuid _ickGUID;
            ObjectGuid _tyrannusGUID;
            ObjectGuid _rimefangGUID;

            ObjectGuid _tyrannusEventGUID;
            ObjectGuid _jainaOrSylvanas1GUID;
            ObjectGuid _jainaOrSylvanas2GUID;

            uint32 _teamInInstance;
            std::set<ObjectGuid> SlaveGUIDs;
            std::vector<ObjectGuid> SpecialTrashGUIDs[TRASH_GROUP_COUNT];
            std::vector<ObjectGuid> ChainGUIDs;
            uint32 DoNotLookUp;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const
        {
            return new instance_pit_of_saron_InstanceScript(map);
        }
};

void AddSC_instance_pit_of_saron()
{
    new instance_pit_of_saron();
}
