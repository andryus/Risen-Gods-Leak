/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

<<<<<<< HEAD
    struct instance_temple_of_ahnqiraj_InstanceMapScript : public InstanceScript
    {
        instance_temple_of_ahnqiraj_InstanceMapScript(Map* map) : InstanceScript(map) { }

        //If Vem is dead...
        bool IsBossDied[3];

        uint64 SkeramGUID;
        uint64 BugVemGUID;
        uint64 BugYaujGUID;
        uint64 BugKriGUID;
        uint64 SarturaGUID;
        uint64 FankrissGUID;
        uint64 ViscidusGUID;
        uint64 HuhuranGUID;
        uint64 TwinVeklorGUID;
        uint64 TwinVeknilashGUID;
        uint64 OuroGUID;
        uint64 CthunEyeGUID;
        uint64 CthunGUID;
        uint64 go_skeram_gate;

        uint8 BugTrioDeathCount;

        uint8 CthunPhase;

        void Initialize() override
=======
        InstanceScript* GetInstanceScript(InstanceMap* map) const override
>>>>>>> 24ae6a6... Core/Misc: Remove obsolete C++11 backward compatibility macros
        {
            IsBossDied[0] = false;
            IsBossDied[1] = false;
            IsBossDied[2] = false;

            SkeramGUID = 0;
            BugVemGUID = 0;
            BugYaujGUID = 0,
            BugKriGUID = 0;
            SarturaGUID = 0,
            FankrissGUID = 0,
            ViscidusGUID = 0,
            HuhuranGUID = 0,
            OuroGUID = 0,
            TwinVeklorGUID = 0;
            TwinVeknilashGUID = 0;
            OuroGUID = 0,
            CthunEyeGUID = 0,
            CthunGUID = 0,

            BugTrioDeathCount = 0;

            CthunPhase = 0;
            go_skeram_gate = 0;
        }

        void OnCreatureCreate(Creature* creature) override
        {
<<<<<<< HEAD
            switch (creature->GetEntry())
=======
            instance_temple_of_ahnqiraj_InstanceMapScript(Map* map) : InstanceScript(map) { }

            //If Vem is dead...
            bool IsBossDied[3];

            //Storing Skeram, Vem and Kri.
            uint64 SkeramGUID;
            uint64 VemGUID;
            uint64 KriGUID;
            uint64 VeklorGUID;
            uint64 VeknilashGUID;
            uint64 ViscidusGUID;

            uint32 BugTrioDeathCount;

            uint32 CthunPhase;

            void Initialize() override
>>>>>>> 24ae6a6... Core/Misc: Remove obsolete C++11 backward compatibility macros
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
                case NPC_TWIN_VEKLOR:    TwinVeklorGUID = creature->GetGUID(); break;
                case NPC_TWIN_VEKLINASH: TwinVeknilashGUID = creature->GetGUID(); break;
                case NPC_OURO:           OuroGUID = creature->GetGUID(); break;
                case NPC_CTHUN_EYE:      CthunEyeGUID = creature->GetGUID(); break;
                case NPC_CTHUN:          CthunGUID = creature->GetGUID(); break;
            }
        }

<<<<<<< HEAD
        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
=======
            void OnCreatureCreate(Creature* creature) override
>>>>>>> 24ae6a6... Core/Misc: Remove obsolete C++11 backward compatibility macros
            {
                case GO_SKERAM_GATE:
                    go_skeram_gate = go->GetGUID();
                    break;
            }
        }

<<<<<<< HEAD
        uint64 GetData64(uint32 identifier) const
        {
            switch (identifier)
=======
            bool IsEncounterInProgress() const override
>>>>>>> 24ae6a6... Core/Misc: Remove obsolete C++11 backward compatibility macros
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
            return 0;
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

<<<<<<< HEAD
        uint32 GetData(uint32 type) const override
        {
            switch (type)
=======
            uint32 GetData(uint32 type) const override
>>>>>>> 24ae6a6... Core/Misc: Remove obsolete C++11 backward compatibility macros
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

<<<<<<< HEAD
        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
=======
            uint64 GetData64(uint32 identifier) const override
            {
                switch (identifier)
                {
                case DATA_SKERAM:
                    return SkeramGUID;
                case DATA_VEM:
                    return VemGUID;
                case DATA_KRI:
                    return KriGUID;
                case DATA_VEKLOR:
                    return VeklorGUID;
                case DATA_VEKNILASH:
                    return VeknilashGUID;
                case DATA_VISCIDUS:
                    return ViscidusGUID;
                }
                return 0;
            }                                                       // end GetData64

            void SetData(uint32 type, uint32 data) override
>>>>>>> 24ae6a6... Core/Misc: Remove obsolete C++11 backward compatibility macros
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
