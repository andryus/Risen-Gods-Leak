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
#include "ScriptedCreature.h"
#include "InstanceScript.h"
#include "ahnkahet.h"

/* Ahn'kahet encounters:
0 - Elder Nadox
1 - Prince Taldaram
2 - Jedoga Shadowseeker
3 - Herald Volazj
4 - Amanitar (Heroic only)
*/

#define MAX_ENCOUNTER           5

static Position VolunteerPositions[]=
{
    {391.897f,-709.394f,-16.0964f,2.976446f},
    {390.592f,-713.409f,-16.0964f,2.777740f},
    {388.481f,-717.066f,-16.0964f,2.577464f},
    {385.656f,-720.204f,-16.0964f,2.326136f},
    {382.240f,-722.685f,-16.0964f,2.098371f},
    {378.383f,-724.402f,-16.0964f,1.909875f},
    {374.254f,-725.280f,-16.0964f,1.658548f},
    {370.032f,-725.280f,-16.0964f,1.509321f},
    {365.902f,-724.402f,-16.0964f,1.475056f},
    {362.045f,-722.685f,-16.0964f,1.012458f},
    {385.656f,-690.187f,-16.0964f,3.999035f},
    {388.481f,-693.325f,-16.0964f,3.786978f},
    {390.592f,-696.981f,-16.0964f,3.610263f},
    {391.897f,-700.996f,-16.0964f,3.390351f},
    {392.338f,-705.195f,-16.0964f,3.162586f}
};

class instance_ahnkahet : public InstanceMapScript
{
public:
    instance_ahnkahet() : InstanceMapScript("instance_ahnkahet", 619) { }

    struct instance_ahnkahet_InstanceScript : public InstanceScript
    {
        instance_ahnkahet_InstanceScript(Map* map) : InstanceScript(map) 
        {
            SetHeaders(DataHeader);

            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            InitiateGUIDs.clear();
            VolunteerGUIDs.clear();

            Elder_Nadox.Clear();
            Prince_Taldaram.Clear();
            Jedoga_Shadowseeker.Clear();
            Herald_Volazj.Clear();
            Amanitar.Clear();

            spheres[0] = NOT_STARTED;
            spheres[1] = NOT_STARTED;
        }

        ObjectGuid Elder_Nadox;
        ObjectGuid Prince_Taldaram;
        ObjectGuid Jedoga_Shadowseeker;
        ObjectGuid Herald_Volazj;
        ObjectGuid Amanitar;

        ObjectGuid Prince_TaldaramSpheres[2];
        ObjectGuid Prince_TaldaramPlatform;
        ObjectGuid Prince_TaldaramGate;

        GuidSet InitiateGUIDs;
        GuidSet VolunteerGUIDs;

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        uint32 spheres[2];

        std::string str_data;

        bool IsEncounterInProgress() const
        {
            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS)
                    return true;

            return false;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case 29309: Elder_Nadox = creature->GetGUID();                      break;
                case 29308: 
                    Prince_Taldaram = creature->GetGUID();  
                    if (!creature->IsAlive())
                        HandleGameObject(Prince_TaldaramGate, true); // Web gate past Prince Taldaram
                    break;
                case 29310: Jedoga_Shadowseeker = creature->GetGUID();              break;
                case 29311: Herald_Volazj = creature->GetGUID();                    break;
                case 30258: Amanitar = creature->GetGUID();                         break;
                case 30114: InitiateGUIDs.insert(creature->GetGUID());              break;
                case 30385: VolunteerGUIDs.insert(creature->GetGUID());             break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case 193564:
                    Prince_TaldaramPlatform = go->GetGUID();
                    if (m_auiEncounter[1] == DONE)
                        HandleGameObject(ObjectGuid::Empty, true, go);
                    break;

                case 193093:
                    Prince_TaldaramSpheres[0] = go->GetGUID();
                    if (spheres[0] == IN_PROGRESS)
                    {
                        go->SetGoState(GO_STATE_ACTIVE);
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    }
                    else
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case 193094:
                    Prince_TaldaramSpheres[1] = go->GetGUID();
                    if (spheres[1] == IN_PROGRESS)
                    {
                        go->SetGoState(GO_STATE_ACTIVE);
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    }
                    else
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case 192236:
                    Prince_TaldaramGate = go->GetGUID(); // Web gate past Prince Taldaram
                    break;
            }
        }

        ObjectGuid GetGuidData(uint32 identifier) const override
        {
            switch (identifier)
            {
                case DATA_ELDER_NADOX:                return Elder_Nadox;
                case DATA_PRINCE_TALDARAM:            return Prince_Taldaram;
                case DATA_JEDOGA_SHADOWSEEKER:        return Jedoga_Shadowseeker;
                case DATA_HERALD_VOLAZJ:              return Herald_Volazj;
                case DATA_AMANITAR:                   return Amanitar;
                case DATA_SPHERE_1:                   return Prince_TaldaramSpheres[0];
                case DATA_SPHERE_2:                   return Prince_TaldaramSpheres[1];
                case DATA_PRINCE_TALDARAM_PLATFORM:   return Prince_TaldaramPlatform;
                case DATA_JEDOGA_VOLUNTEER:
                {
                    std::vector<ObjectGuid> volunteers;
                    volunteers.clear();
                    for (std::set<ObjectGuid>::const_iterator itr = VolunteerGUIDs.begin(); itr != VolunteerGUIDs.end(); ++itr)
                    {
                        Creature* cr = instance->GetCreature(*itr);
                        if (cr && cr->IsAlive())
                            volunteers.push_back(*itr);
                    }
                    if (volunteers.empty())
                        return ObjectGuid::Empty;
                    return volunteers[urand(1, volunteers.size()) - 1];
                }
            }
            return ObjectGuid::Empty;
        }

        void SetData(uint32 type, uint32 data)
        {
            switch (type)
            {
                case DATA_ELDER_NADOX_EVENT:
                    m_auiEncounter[0] = data;
                    break;
                case DATA_PRINCE_TALDARAM_EVENT:
                    if (data == DONE)
                        HandleGameObject(Prince_TaldaramGate, true);
                    m_auiEncounter[1] = data;
                    break;
                case DATA_JEDOGA_SHADOWSEEKER_EVENT:
                    m_auiEncounter[2] = data;
                    break;
                case DATA_HERALD_VOLAZJ_EVENT:
                    m_auiEncounter[3] = data;
                    break;
                case DATA_AMANITAR_EVENT:
                    m_auiEncounter[4] = data;
                    break;
                case DATA_SPHERE1_EVENT:
                    spheres[0] = data;
                    break;
                case DATA_SPHERE2_EVENT:
                    spheres[1] = data;
                    break;
                case DATA_JEDOGA_RESET_INITIATES:
                {
                    int count = 0;
                    for (std::set<ObjectGuid>::const_iterator itr = InitiateGUIDs.begin(); itr != InitiateGUIDs.end(); ++itr)
                    {
                        Creature* cr = instance->GetCreature(*itr);
                        if (cr)
                        {
                            cr->Respawn();
                            if (!cr->IsInEvadeMode())
                                cr->AI()->EnterEvadeMode();
                        }
                    }
                    break;
                }
                case DATA_JEDOGA_SPAWN_VOLUNTEERS:
                    VolunteerGUIDs.clear();
                    for (int i = 0; i < 15; i++)
                        instance->SummonCreature(30385, VolunteerPositions[i]);
                    break;
                case DATA_JEDOGA_RESET_VOLUNTEERS:
                    for (std::set<ObjectGuid>::const_iterator itr = VolunteerGUIDs.begin(); itr != VolunteerGUIDs.end(); ++itr)
                    {
                        Creature* cr = instance->GetCreature(*itr);
                        if (cr)
                            cr->DespawnOrUnsummon(1U);
                    }
                    break;
            }
            if (data == DONE)
                SaveToDB();
        }

        uint32 GetData(uint32 type) const
        {
            switch (type)
            {
                case DATA_ELDER_NADOX_EVENT:            return m_auiEncounter[0];
                case DATA_PRINCE_TALDARAM_EVENT:        return m_auiEncounter[1];
                case DATA_JEDOGA_SHADOWSEEKER_EVENT:    return m_auiEncounter[2];
                case DATA_HERALD_VOLAZJ:                return m_auiEncounter[3];
                case DATA_AMANITAR_EVENT:               return m_auiEncounter[4];
                case DATA_SPHERE1_EVENT:                return spheres[0];
                case DATA_SPHERE2_EVENT:                return spheres[1];
                case DATA_JEDOGA_ALL_INITIATES_DEAD:
                    for (std::set<ObjectGuid>::const_iterator itr = InitiateGUIDs.begin(); itr != InitiateGUIDs.end(); ++itr)
                    {
                        Creature* cr = instance->GetCreature(*itr);
                        if (cr && cr->IsAlive())
                            return 0;
                    }
                    return 1;
            }
            return 1;
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << spheres[0] << ' ' << spheres[1];
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            data >> spheres[0];
            data >> spheres[1];
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
       return new instance_ahnkahet_InstanceScript(map);
    }
};

void AddSC_instance_ahnkahet()
{
   new instance_ahnkahet();
}
