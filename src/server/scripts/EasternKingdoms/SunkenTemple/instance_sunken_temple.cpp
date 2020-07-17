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
SDName: Instance_Sunken_Temple
SD%Complete: 100
SDComment:Place Holder
SDCategory: Sunken Temple
EndScriptData */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "sunken_temple.h"
#include "CreatureTextMgr.h"

enum Gameobject
{
    GO_ATALAI_STATUE1           = 148830,
    GO_ATALAI_STATUE2           = 148831,
    GO_ATALAI_STATUE3           = 148832,
    GO_ATALAI_STATUE4           = 148833,
    GO_ATALAI_STATUE5           = 148834,
    GO_ATALAI_STATUE6           = 148835,
    GO_ATALAI_IDOL              = 148836,
    GO_ATALAI_LIGHT1            = 148883,
    GO_ATALAI_LIGHT2            = 148937,
    GO_FORCEFIELD               = 149431

};

enum CreatureIds
{
    NPC_MALFURION_STORMRAGE     = 15362
};

enum Texts
{
    SAY_JAMMAL_FORCEFIELD       = 0
};

class instance_sunken_temple : public InstanceMapScript
{
public:
    instance_sunken_temple() : InstanceMapScript("instance_sunken_temple", 109) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_sunken_temple_InstanceMapScript(map);
    }

    struct instance_sunken_temple_InstanceMapScript : public InstanceScript
    {
        instance_sunken_temple_InstanceMapScript(Map* map) : InstanceScript(map) 
        {
            SetHeaders(DataHeader);
            GOForcefield.Clear();
            JammalGUID.Clear();
            ShadeGUID.Clear();
            TrollDoneGUID.Clear();

            State = 0;
            TBdeaths = 0;

            s1 = false;
            s2 = false;
            s3 = false;
            s4 = false;
            s5 = false;
            s6 = false;
            FieldDown = false;
        }

        void OnCreatureCreate(Creature* creature)  override
        {
            switch (creature->GetEntry())
            {
                case NPC_ZULLOR:
                    TrollDoneGUID = creature->GetGUID();
                    if (creature->isDead())
                    {
                        if (Creature* jammal = instance->GetCreature(JammalGUID))
                        {
                            jammal->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
							jammal->SetImmuneToAll(false);
                        }
                        if (GameObject* pForcefield = instance->GetGameObject(GOForcefield))
                        {
                            pForcefield->SetUInt32Value(GAMEOBJECT_FLAGS, 33);
                            pForcefield->SetGoState(GO_STATE_ACTIVE);
                        }
                    }
                    break;
                case NPC_JAMMAL_AN:
                    JammalGUID = creature->GetGUID();
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					creature->SetImmuneToAll(true);
                    if (creature->isDead())
                    {
                        if (Creature* shade = instance->GetCreature(ShadeGUID))
                        {
                            shade->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
							shade->SetImmuneToAll(false);
                        }
                    }
                    break;
                case NPC_SHADE_OF_EANKIUS:
                    ShadeGUID = creature->GetGUID();
                    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					creature->SetImmuneToAll(true);
                    break;                
                default:
                    break;
            }
        }

        void OnUnitDeath(Unit* unit) override
        {
            Creature* creature = unit->ToCreature();
            if (!creature)
                return;

            switch (creature->GetEntry())
            {
                case NPC_JAMMAL_AN:                   
                    if (Creature* shade = instance->GetCreature(ShadeGUID))
                    {
                        shade->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
						shade->SetImmuneToAll(false);
                    }
                    break;
                default:
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_ATALAI_STATUE1: 
                    GOAtalaiStatue1 = go->GetGUID();   
                    break;
                case GO_ATALAI_STATUE2: 
                    GOAtalaiStatue2 = go->GetGUID();   
                    break;
                case GO_ATALAI_STATUE3: 
                    GOAtalaiStatue3 = go->GetGUID();   
                    break;
                case GO_ATALAI_STATUE4: 
                    GOAtalaiStatue4 = go->GetGUID();   
                    break;
                case GO_ATALAI_STATUE5: 
                    GOAtalaiStatue5 = go->GetGUID();   
                    break;
                case GO_ATALAI_STATUE6: 
                    GOAtalaiStatue6 = go->GetGUID();   
                    break;
                case GO_ATALAI_IDOL:    
                    GOAtalaiIdol    = go->GetGUID();   
                    break;
                case GO_FORCEFIELD:     
                    GOForcefield    = go->GetGUID(); 
                    break;
                default:
                    break;
            }
        }

         virtual void Update(uint32 /*diff*/) // correct order goes form 1-6
         {
             switch (State)
             {
                 case GO_ATALAI_STATUE1:
                     if (!s1 && !s2 && !s3 && !s4 && !s5 && !s6)
                     {
                         if (GameObject* pAtalaiStatue1 = instance->GetGameObject(GOAtalaiStatue1))
                             UseStatue(pAtalaiStatue1);
                         s1 = true;
                         State = 0;
                     };
                     break;
                 case GO_ATALAI_STATUE2:
                     if (s1 && !s2 && !s3 && !s4 && !s5 && !s6)
                     {
                         if (GameObject* pAtalaiStatue2 = instance->GetGameObject(GOAtalaiStatue2))
                             UseStatue(pAtalaiStatue2);
                         s2 = true;
                         State = 0;
                     };
                     break;
                 case GO_ATALAI_STATUE3:
                     if (s1 && s2 && !s3 && !s4 && !s5 && !s6)
                     {
                         if (GameObject* pAtalaiStatue3 = instance->GetGameObject(GOAtalaiStatue3))
                             UseStatue(pAtalaiStatue3);
                         s3 = true;
                         State = 0;
                     };
                     break;
                 case GO_ATALAI_STATUE4:
                     if (s1 && s2 && s3 && !s4 && !s5 && !s6)
                     {
                         if (GameObject* pAtalaiStatue4 = instance->GetGameObject(GOAtalaiStatue4))
                             UseStatue(pAtalaiStatue4);
                         s4 = true;
                         State = 0;
                     }
                     break;
                 case GO_ATALAI_STATUE5:
                     if (s1 && s2 && s3 && s4 && !s5 && !s6)
                     {
                         if (GameObject* pAtalaiStatue5 = instance->GetGameObject(GOAtalaiStatue5))
                             UseStatue(pAtalaiStatue5);
                         s5 = true;
                         State = 0;
                     }
                     break;
                 case GO_ATALAI_STATUE6:
                     if (s1 && s2 && s3 && s4 && s5 && !s6)
                     {
                         if (GameObject* pAtalaiStatue6 = instance->GetGameObject(GOAtalaiStatue6))
                         {    
                             UseStatue(pAtalaiStatue6);
                             UseLastStatue(pAtalaiStatue6);
                         }
                         s6 = true;
                         State = 0;
                     }
                     break;
             }

             if(TBdeaths==6) //All TrollBosses Dead
             {
                 if (!FieldDown)
                 {
                     FieldDown = true;
                     if (GameObject* pForcefield = instance->GetGameObject(GOForcefield))
                     {
                         pForcefield->SetUInt32Value(GAMEOBJECT_FLAGS, 33);
                         pForcefield->SetGoState(GO_STATE_ACTIVE);
                         if (Creature* jammal = instance->GetCreature(JammalGUID))
                         {
                             sCreatureTextMgr->SendChat(jammal, SAY_JAMMAL_FORCEFIELD, NULL, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE, 5861);
                             jammal->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
							 jammal->SetImmuneToAll(false);
                         }
                     }
                 }                 
             }
         };

        void UseStatue(GameObject* go)
        {
            go->SummonGameObject(GO_ATALAI_LIGHT1, go->GetPositionX(), go->GetPositionY(), go->GetPositionZ(), 0, 0, 0, 0, 0, 0);
            go->SetUInt32Value(GAMEOBJECT_FLAGS, 4);
        }

         void UseLastStatue(GameObject* go)
         {
             if (GameObject* AtalaiStatue1 = instance->GetGameObject(GOAtalaiStatue1))
                AtalaiStatue1->SummonGameObject(GO_ATALAI_LIGHT2, AtalaiStatue1->GetPositionX(), AtalaiStatue1->GetPositionY(), AtalaiStatue1->GetPositionZ(), 0, 0, 0, 0, 0, 100);

             if (GameObject* AtalaiStatue2 = instance->GetGameObject(GOAtalaiStatue2))
                AtalaiStatue2->SummonGameObject(GO_ATALAI_LIGHT2, AtalaiStatue2->GetPositionX(), AtalaiStatue2->GetPositionY(), AtalaiStatue2->GetPositionZ(), 0, 0, 0, 0, 0, 100);
             
             if (GameObject* AtalaiStatue3 = instance->GetGameObject(GOAtalaiStatue3))
                AtalaiStatue3->SummonGameObject(GO_ATALAI_LIGHT2, AtalaiStatue3->GetPositionX(), AtalaiStatue3->GetPositionY(), AtalaiStatue3->GetPositionZ(), 0, 0, 0, 0, 0, 100);
             
             if (GameObject* AtalaiStatue4 = instance->GetGameObject(GOAtalaiStatue4))
                AtalaiStatue4->SummonGameObject(GO_ATALAI_LIGHT2, AtalaiStatue4->GetPositionX(), AtalaiStatue4->GetPositionY(), AtalaiStatue4->GetPositionZ(), 0, 0, 0, 0, 0, 100);
             
             if (GameObject* AtalaiStatue5 = instance->GetGameObject(GOAtalaiStatue5))
                AtalaiStatue5->SummonGameObject(GO_ATALAI_LIGHT2, AtalaiStatue5->GetPositionX(), AtalaiStatue5->GetPositionY(), AtalaiStatue5->GetPositionZ(), 0, 0, 0, 0, 0, 100);
             
             if (GameObject* AtalaiStatue6 = instance->GetGameObject(GOAtalaiStatue6))
                AtalaiStatue6->SummonGameObject(GO_ATALAI_LIGHT2, AtalaiStatue6->GetPositionX(), AtalaiStatue6->GetPositionY(), AtalaiStatue6->GetPositionZ(), 0, 0, 0, 0, 0, 100);
             
             go->SummonGameObject(GO_ALTAR, -488.997f, 96.61f, -189.019f, -1.52f, 0, 0, 0, 0, DAY);
             go->SummonCreature(NPC_ATAL, -480.4f, 96.5663f, -189.73f, 6.19797f);
         }

         void SetData(uint32 type, uint32 data) override
         {
             switch(type)
             {   
                 case EVENT_STATE:               
                     State = data; 
                     break;
                 case TROLLBOSS_DEATH:           
                     if (data)
                         TBdeaths++; 
                     break;                 
             }
         }

         uint32 GetData(uint32 type) const override
         {
            if (type == EVENT_STATE)
                return State;
            return 0;
         }

         ObjectGuid GetGuidData(uint32 id) const override
         {
             switch (id)
             {
                 case DATA_JAMMAL_AN:
                     return JammalGUID;
                 case DATA_SHADE:
                     return ShadeGUID;
                 case DATA_TROLL_DONE:
                     return TrollDoneGUID;
             }

             return ObjectGuid::Empty;
         }

    protected:
        ObjectGuid GOAtalaiStatue1;
        ObjectGuid GOAtalaiStatue2;
        ObjectGuid GOAtalaiStatue3;
        ObjectGuid GOAtalaiStatue4;
        ObjectGuid GOAtalaiStatue5;
        ObjectGuid GOAtalaiStatue6;
        ObjectGuid GOAtalaiIdol;
        ObjectGuid GOForcefield;
        ObjectGuid JammalGUID;
        ObjectGuid ShadeGUID;
        ObjectGuid TrollDoneGUID;

        uint32 State;

        bool s1;
        bool s2;
        bool s3;
        bool s4;
        bool s5;
        bool s6;
        bool FieldDown;

        int  TBdeaths;
    };

};

void AddSC_instance_sunken_temple()
{
    new instance_sunken_temple();
}
