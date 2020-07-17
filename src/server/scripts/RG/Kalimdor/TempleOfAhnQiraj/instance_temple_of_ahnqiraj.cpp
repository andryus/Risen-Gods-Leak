/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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
SDName: Instance_Temple_of_Ahnqiraj
SD%Complete: 80
SDComment:
SDCategory: Temple of Ahn'Qiraj
EndScriptData */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "temple_of_ahnqiraj.h"

class instance_temple_of_ahnqiraj : public InstanceMapScript
{
    public:
        instance_temple_of_ahnqiraj() : InstanceMapScript("instance_temple_of_ahnqiraj", 531) { }

    struct instance_temple_of_ahnqiraj_InstanceMapScript : public InstanceScript
    {
        instance_temple_of_ahnqiraj_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetHeaders(DataHeader);
            IsBossDied[0] = false;
            IsBossDied[1] = false;
            IsBossDied[2] = false;

            SkeramGUID.Clear();
            BugVemGUID.Clear();
            BugYaujGUID.Clear();
            BugKriGUID.Clear();
            SarturaGUID.Clear();
            FankrissGUID.Clear();
            ViscidusGUID.Clear();
            HuhuranGUID.Clear();
            OuroGUID.Clear();
            TwinVeklorGUID.Clear();
            TwinVeknilashGUID.Clear();
            OuroGUID.Clear();
            CthunEyeGUID.Clear();
            CthunGUID.Clear();

            BugTrioDeathCount = 0;

            CthunPhase = 0;
            go_skeram_gate.Clear();
            go_twins_gate.Clear();
        }

        //If Vem is dead...
        bool IsBossDied[3];

        ObjectGuid SkeramGUID;
        ObjectGuid BugVemGUID;
        ObjectGuid BugYaujGUID;
        ObjectGuid BugKriGUID;
        ObjectGuid SarturaGUID;
        ObjectGuid FankrissGUID;
        ObjectGuid ViscidusGUID;
        ObjectGuid HuhuranGUID;
        ObjectGuid TwinVeklorGUID;
        ObjectGuid TwinVeknilashGUID;
        ObjectGuid OuroGUID;
        ObjectGuid CthunEyeGUID;
        ObjectGuid CthunGUID;
        ObjectGuid go_skeram_gate;
        ObjectGuid go_twins_gate;

        uint8 BugTrioDeathCount;

        uint8 CthunPhase;

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_SKERAM:         
                    SkeramGUID = creature->GetGUID(); 
                    if (!creature->IsAlive())
                        HandleGameObject(go_skeram_gate, true);
                    break;
                case NPC_BUG_VEM:        BugVemGUID = creature->GetGUID(); break;
                case NPC_BUG_YAUJ:       BugYaujGUID = creature->GetGUID(); break;
                case NPC_BUG_KRI:        BugKriGUID = creature->GetGUID(); break;
                case NPC_SARTURA:        SarturaGUID = creature->GetGUID(); break;
                case NPC_FANKRISS:       FankrissGUID = creature->GetGUID(); break;
                case NPC_VISCIDUS:       ViscidusGUID = creature->GetGUID(); break;
                case NPC_HUHURAN:        HuhuranGUID = creature->GetGUID(); break;
                case NPC_TWIN_VEKLOR:    
                    TwinVeklorGUID = creature->GetGUID(); 
                    if (!creature->IsAlive())
                        HandleGameObject(go_twins_gate, true);
                    break;
                case NPC_TWIN_VEKLINASH: 
                    TwinVeknilashGUID = creature->GetGUID(); 
                    if (!creature->IsAlive())
                        HandleGameObject(go_twins_gate, true);
                    break;
                case NPC_OURO:           OuroGUID = creature->GetGUID(); break;
                case NPC_CTHUN_EYE:      CthunEyeGUID = creature->GetGUID(); break;
                case NPC_CTHUN:          CthunGUID = creature->GetGUID(); break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_SKERAM_GATE:
                    go_skeram_gate = go->GetGUID();
                    break;
                case GO_TWINS_GATE:
                    go_twins_gate = go->GetGUID();
                    break;
            }
        }

        ObjectGuid GetGuidData(uint32 identifier) const
        {
            switch (identifier)
            {
                case DATA_SKERAM:                 return SkeramGUID;
                case DATA_BUG_VEM:                return BugVemGUID;
                case DATA_BUG_KRI:                return BugKriGUID;
                case DATA_BUG_YAUJ:               return BugYaujGUID;
                case DATA_SARTURA:                return SarturaGUID;
                case DATA_FANKRISS:               return FankrissGUID;
                case DATA_VISCIDUS:               return ViscidusGUID;
                case DATA_HUHURAN:                return HuhuranGUID;
                case DATA_TWIN_VEKLOR:            return TwinVeklorGUID;
                case DATA_TWIN_VEKLINASH:         return TwinVeknilashGUID;
                case DATA_OURO:                   return OuroGUID;
                case DATA_CTHUN_EYE:              return CthunEyeGUID;
                case DATA_CTHUN:                  return CthunGUID;
            }
            return ObjectGuid::Empty;
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case BOSS_BUG_KRI:
                case BOSS_BUG_VEM:
                case BOSS_BUG_YAUJ:
                    break;
            }
            return true;
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_VEMISDEAD:
                    if (IsBossDied[0])
                        return 1;
                    break;
                case DATA_VEKLORISDEAD:
                    if (IsBossDied[1])
                        return 1;
                    break;
                case DATA_VEKNILASHISDEAD:
                    if (IsBossDied[2])
                        return 1;
                    break;
                case DATA_BUG_TRIO_DEATH:
                    return BugTrioDeathCount;
                case DATA_CTHUN_PHASE:
                    return CthunPhase;
            }
            return 0;
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_VEM_DEATH:
                    IsBossDied[0] = true;
                    break;
                case DATA_BUG_TRIO_DEATH:
                    ++BugTrioDeathCount;
                    break;
                case DATA_VEKLOR_DEATH:
                    IsBossDied[1] = true;
                    break;
                case DATA_VEKNILASH_DEATH:
                    IsBossDied[2] = true;
                    break;
                case DATA_CTHUN_PHASE:
                    CthunPhase = data;
                    break;
            }
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_temple_of_ahnqiraj_InstanceMapScript(map);
    }
};

void AddSC_instance_temple_of_ahnqiraj()
{
    new instance_temple_of_ahnqiraj();
}
