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

/* ScriptData
SDName: Instance_Halls_of_Lightning
SD%Complete: 90%
SDComment: All ready.
SDCategory: Halls of Lightning
EndScriptData */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "halls_of_lightning.h"

/* Halls of Lightning encounters:
0 - General Bjarngrim
1 - Volkhan
2 - Ionar
3 - Loken
*/

class instance_halls_of_lightning : public InstanceMapScript
{
public:
    instance_halls_of_lightning() : InstanceMapScript("instance_halls_of_lightning", 602) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_halls_of_lightning_InstanceMapScript(pMap);
    }

    struct instance_halls_of_lightning_InstanceMapScript : public InstanceScript
    {
        instance_halls_of_lightning_InstanceMapScript(Map* pMap) : InstanceScript(pMap) 
        {
            SetHeaders(DataHeader);
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

            m_uiGeneralBjarngrimGUID.Clear();
            m_uiStormforgedLieutenant[0].Clear();
            m_uiStormforgedLieutenant[1].Clear();
            m_uiVolkhanGUID.Clear();
            m_uiIonarGUID.Clear();
            m_uiLokenGUID.Clear();

            m_uiBjarngrimDoorGUID.Clear();
            m_uiVolkhanDoorGUID.Clear();
            m_uiIonarDoorGUID.Clear();
            m_uiLokenDoorGUID.Clear();
            m_uiLokenGlobeGUID.Clear();
            uiCountStormforgedLieutenant = 0;
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];

        ObjectGuid m_uiGeneralBjarngrimGUID;
        ObjectGuid m_uiStormforgedLieutenant[2];
        ObjectGuid m_uiIonarGUID;
        ObjectGuid m_uiLokenGUID;
        ObjectGuid m_uiVolkhanGUID;

        ObjectGuid m_uiBjarngrimDoorGUID;
        ObjectGuid m_uiVolkhanDoorGUID;
        ObjectGuid m_uiIonarDoorGUID;
        ObjectGuid m_uiLokenDoorGUID;
        ObjectGuid m_uiLokenGlobeGUID;

        uint8 uiCountStormforgedLieutenant;

        void OnCreatureCreate(Creature* creature)
        {
            switch(creature->GetEntry())
            {
                case NPC_BJARNGRIM:
                    m_uiGeneralBjarngrimGUID = creature->GetGUID();
                    break;
                case NPC_STORMFORGED_LIEUTENANT:
                    if (uiCountStormforgedLieutenant < 2)
                    {
                        m_uiStormforgedLieutenant[uiCountStormforgedLieutenant++] = creature->GetGUID();
                    }
                    break;
                case NPC_VOLKHAN:
                    m_uiVolkhanGUID = creature->GetGUID();
                    break;
                case NPC_IONAR:
                    m_uiIonarGUID = creature->GetGUID();
                    break;
                case NPC_LOKEN:
                    m_uiLokenGUID = creature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch(go->GetEntry())
            {
                case GO_BJARNGRIM_DOOR:
                    m_uiBjarngrimDoorGUID = go->GetGUID();
                    if (m_auiEncounter[0] == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    else
                        go->SetGoState(GO_STATE_READY);
                    break;
                case GO_VOLKHAN_DOOR:
                    m_uiVolkhanDoorGUID = go->GetGUID();
                    if (m_auiEncounter[1] == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    else
                        go->SetGoState(GO_STATE_READY);
                    break;
                case GO_IONAR_DOOR:
                    m_uiIonarDoorGUID = go->GetGUID();
                    if (m_auiEncounter[2] == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    else
                        go->SetGoState(GO_STATE_READY);
                    break;
                case GO_LOKEN_DOOR:
                    m_uiLokenDoorGUID = go->GetGUID();
                    go->SetGoState(GO_STATE_ACTIVE);
                case GO_LOKEN_THRONE:
                    m_uiLokenGlobeGUID = go->GetGUID();
                    break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData)
        {
            switch(uiType)
            {
                case TYPE_BJARNGRIM:
                    if (uiData == DONE)
                        if (GameObject* pDoor = instance->GetGameObject(m_uiBjarngrimDoorGUID))
                            pDoor->SetGoState(GO_STATE_ACTIVE);
                    m_auiEncounter[0] = uiData;
                    break;
                case TYPE_VOLKHAN:
                    if (uiData == DONE)
                        if (GameObject* pDoor = instance->GetGameObject(m_uiVolkhanDoorGUID))
                            pDoor->SetGoState(GO_STATE_ACTIVE);
                    m_auiEncounter[1] = uiData;
                    break;
                case TYPE_IONAR:
                    if (uiData == DONE)
                        if (GameObject* pDoor = instance->GetGameObject(m_uiIonarDoorGUID))
                            pDoor->SetGoState(GO_STATE_ACTIVE);
                    m_auiEncounter[2] = uiData;
                    break;
                case TYPE_LOKEN:
                    if (uiData == DONE)
                    {
                        if (GameObject* pDoor = instance->GetGameObject(m_uiLokenDoorGUID))
                            pDoor->SetGoState(GO_STATE_ACTIVE);

                        // Appears to be type 5 GO with animation. Need to figure out how this work, code below only placeholder
                        if (GameObject* pGlobe = instance->GetGameObject(m_uiLokenGlobeGUID))
                            pGlobe->SetGoState(GO_STATE_ACTIVE);
                    }
                    m_auiEncounter[3] = uiData;
                    break;
            }

            if (uiData == DONE)
                SaveToDB();
        }

        uint32 GetData(uint32 uiType) const
        {
            switch(uiType)
            {
                case TYPE_BJARNGRIM:
                    return m_auiEncounter[0];
                case TYPE_VOLKHAN:
                    return m_auiEncounter[1];
                case TYPE_IONAR:
                    return m_auiEncounter[2];
                case TYPE_LOKEN:
                    return m_auiEncounter[3];
            }
            return 0;
        }

        ObjectGuid GetGuidData(uint32 uiData) const override
        {
            switch(uiData)
            {
                case DATA_BJARNGRIM:
                    return m_uiGeneralBjarngrimGUID;
                case DATA_STORMFORGED_LIEUTENANT_1:
                    return m_uiStormforgedLieutenant[0];
                case DATA_STORMFORGED_LIEUTENANT_2:
                    return m_uiStormforgedLieutenant[1];
                case DATA_VOLKHAN:
                    return m_uiVolkhanGUID;
                case DATA_IONAR:
                    return m_uiIonarGUID;
                case DATA_LOKEN:
                    return m_uiLokenGUID;
            }
            return ObjectGuid::Empty;
        }
    };

};

void AddSC_instance_halls_of_lightning()
{
    new instance_halls_of_lightning();
}
