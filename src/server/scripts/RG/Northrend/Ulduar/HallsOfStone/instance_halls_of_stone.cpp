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
#include "halls_of_stone.h"
#include "ScriptedCreature.h"
#include "Player.h"

#define MAX_ENCOUNTER 4

/* Halls of Stone encounters:
0- Krystallus
1- Maiden of Grief
2- Escort Event
3- Sjonnir The Ironshaper
*/

class instance_halls_of_stone : public InstanceMapScript
{
public:
    instance_halls_of_stone() : InstanceMapScript("instance_halls_of_stone", 599) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_halls_of_stone_InstanceMapScript(pMap);
    }

    struct instance_halls_of_stone_InstanceMapScript : public InstanceScript
    {
        instance_halls_of_stone_InstanceMapScript(Map* pMap) : InstanceScript(pMap) 
        {
            SetHeaders(DataHeader);
            uiMaidenOfGrief.Clear();
            uiKrystallus.Clear();
            uiSjonnir.Clear();

            uiKaddrak.Clear();
            uiMarnak.Clear();
            uiAbedneum.Clear();
            uiBrann.Clear();

            uiMaidenOfGriefDoor.Clear();
            uiSjonnirDoor.Clear();
            uiBrannDoor.Clear();
            uiKaddrakGo.Clear();
            uiMarnakGo.Clear();
            uiAbedneumGo.Clear();
            uiTribunalConsole.Clear();
            uiSjonnirConsole.Clear();
            uiTribunalChest.Clear();
            uiTribunalSkyFloor.Clear();

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                m_auiEncounter[i] = NOT_STARTED;
        }

        ObjectGuid uiMaidenOfGrief;
        ObjectGuid uiKrystallus;
        ObjectGuid uiSjonnir;

        ObjectGuid uiKaddrak;
        ObjectGuid uiAbedneum;
        ObjectGuid uiMarnak;
        ObjectGuid uiBrann;

        ObjectGuid uiMaidenOfGriefDoor;
        ObjectGuid uiSjonnirDoor;
        ObjectGuid uiBrannDoor;
        ObjectGuid uiTribunalConsole;
        ObjectGuid uiSjonnirConsole;
        ObjectGuid uiTribunalChest;
        ObjectGuid uiTribunalSkyFloor;
        ObjectGuid uiKaddrakGo;
        ObjectGuid uiAbedneumGo;
        ObjectGuid uiMarnakGo;

        uint32 m_auiEncounter[MAX_ENCOUNTER];

        std::string str_data;

        void OnCreatureCreate(Creature* creature)
        {
            switch(creature->GetEntry())
            {
                case CREATURE_MAIDEN: uiMaidenOfGrief = creature->GetGUID(); break;
                case CREATURE_KRYSTALLUS: uiKrystallus = creature->GetGUID(); break;
                case CREATURE_SJONNIR: 
                    uiSjonnir = creature->GetGUID(); 
                    if (GetData(DATA_BRANN_EVENT) == DONE)
                        creature->setFaction(FACTION_HOSTILE);
                    else
                        creature->setFaction(FACTION_FRIENDLY_TO_ALL);
                    break;
                case CREATURE_MARNAK: uiMarnak = creature->GetGUID(); break;
                case CREATURE_KADDRAK: uiKaddrak = creature->GetGUID(); break;
                case CREATURE_ABEDNEUM: uiAbedneum = creature->GetGUID(); break;
                case CREATURE_BRANN: uiBrann = creature->GetGUID(); break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch(go->GetEntry())
            {
                case GO_ABEDNEUM:
                    uiAbedneumGo = go->GetGUID();
                    break;
                case GO_MARNAK:
                    uiMarnakGo = go->GetGUID();
                    break;
                case GO_KADDRAK:
                    uiKaddrakGo = go->GetGUID();
                    break;
                case GO_MAIDEN_DOOR:
                    uiMaidenOfGriefDoor = go->GetGUID();
                    if (m_auiEncounter[0] == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    else
                        go->SetGoState(GO_STATE_READY);
                    break;
                case GO_BRANN_DOOR:
                    uiBrannDoor = go->GetGUID();
                    if (m_auiEncounter[1] == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    else
                        go->SetGoState(GO_STATE_READY);
                    break;
                case GO_SJONNIR_DOOR:
                    uiSjonnirDoor = go->GetGUID();
                    if (m_auiEncounter[2] == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    else
                        go->SetGoState(GO_STATE_READY);
                    break;
                case GO_TRIBUNAL_CONSOLE:
                    uiTribunalConsole = go->GetGUID();
                    break;
                case GO_SJONNIR_CONSOLE:
                    uiTribunalConsole = go->GetGUID();
                    break;
                case GO_TRIBUNAL_CHEST:
                case GO_TRIBUNAL_CHEST_HERO:
                    uiTribunalChest = go->GetGUID();
                    if (m_auiEncounter[2] == DONE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND | GO_FLAG_NOT_SELECTABLE);
                    break;
                case 191527:
                    uiTribunalSkyFloor = go->GetGUID();
                    break;
            }
        }

        void SetData(uint32 type, uint32 data)
        {
            switch(type)
            {
                case DATA_MAIDEN_OF_GRIEF_EVENT:
                    m_auiEncounter[1] = data;
                    if (m_auiEncounter[1] == DONE)
                        HandleGameObject(uiBrannDoor, true);
                    break;
                case DATA_KRYSTALLUS_EVENT:
                    m_auiEncounter[0] = data;
                    if (m_auiEncounter[0] == DONE)
                        HandleGameObject(uiMaidenOfGriefDoor, true);
                    break;
                case DATA_SJONNIR_EVENT:
                    m_auiEncounter[3] = data;
                    break;
                case DATA_BRANN_EVENT:
                    m_auiEncounter[2] = data;
                    if (m_auiEncounter[2] == DONE)
                    {
                        GameObject* go = instance->GetGameObject(uiTribunalChest);
                        if (go)
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND | GO_FLAG_NOT_SELECTABLE);
                    }
                    break;
            }

            if (data == DONE)
                SaveToDB();
        }

        uint32 GetData(uint32 type) const
        {
            switch(type)
            {
                case DATA_KRYSTALLUS_EVENT:                return m_auiEncounter[0];
                case DATA_MAIDEN_OF_GRIEF_EVENT:           return m_auiEncounter[1];
                case DATA_SJONNIR_EVENT:                   return m_auiEncounter[2];
                case DATA_BRANN_EVENT:                     return m_auiEncounter[3];
            }

            return 0;
        }

        ObjectGuid GetGuidData(uint32 identifier) const
        {
            switch(identifier)
            {
                case DATA_MAIDEN_OF_GRIEF:                 return uiMaidenOfGrief;
                case DATA_KRYSTALLUS:                      return uiKrystallus;
                case DATA_SJONNIR:                         return uiSjonnir;
                case DATA_KADDRAK:                         return uiKaddrak;
                case DATA_MARNAK:                          return uiMarnak;
                case DATA_ABEDNEUM:                        return uiAbedneum;
                case DATA_GO_TRIBUNAL_CONSOLE:             return uiTribunalConsole;
                case DATA_GO_SJONNIR_CONSOLE:              return uiSjonnirConsole;
                case DATA_GO_KADDRAK:                      return uiKaddrakGo;
                case DATA_GO_ABEDNEUM:                     return uiAbedneumGo;
                case DATA_GO_MARNAK:                       return uiMarnakGo;
                case DATA_GO_SKY_FLOOR:                    return uiTribunalSkyFloor;
                case DATA_SJONNIR_DOOR:                    return uiSjonnirDoor;
                case DATA_MAIDEN_DOOR:                     return uiMaidenOfGriefDoor;
            }

            return ObjectGuid::Empty;
        }
    };

};

enum Spells
{
    SPELL_RUNIC_INTELLECT                         = 51799,
    SPELL_STATIC_ARREST                           = 51612,
    SPELL_STATIC_ARREST_H                         = 59033
};

class npc_dark_rune_scholar : public CreatureScript
{
    public:
        npc_dark_rune_scholar() : CreatureScript("npc_dark_rune_scholar") { }

        struct npc_dark_rune_scholarAI : public ScriptedAI
        {
            npc_dark_rune_scholarAI(Creature* creature) : ScriptedAI(creature)
            {}

            uint32 uiArrestTimer;

            void Reset()
            {
                uiArrestTimer = urand(1000, 5000);
            }

            void EnterCombat(Unit* /*who*/)
            {
                DoCast(me,SPELL_RUNIC_INTELLECT);
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (uiArrestTimer <= diff)
                {
                    std::list<Player*> Player_List;
                    me->GetPlayerListInGrid(Player_List, 35.f);
                    for (std::list<Player*>::iterator itr = Player_List.begin(); itr != Player_List.end(); ++itr)
                        if (!(*itr)->HasUnitState(UNIT_STATE_CASTING))
                        {
                            DoCast((*itr),SPELL_STATIC_ARREST);
                            uiArrestTimer = urand(10000, 15000);
                            break;
                         }
                }
                else
                    uiArrestTimer -= diff;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_dark_rune_scholarAI(creature);
        }
};

void AddSC_instance_halls_of_stone()
{
    new instance_halls_of_stone();
    new npc_dark_rune_scholar();
}
