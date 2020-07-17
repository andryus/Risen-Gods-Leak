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
#include "azjol_nerub.h"

/* Azjol Nerub encounters:
0 - Krik'thir the Gatewatcher
1 - Hadronox
2 - Anub'arak
*/

class instance_azjol_nerub : public InstanceMapScript
{
public:
    instance_azjol_nerub() : InstanceMapScript("instance_azjol_nerub", 601) { }

    struct instance_azjol_nerub_InstanceScript : public InstanceScript
    {
        instance_azjol_nerub_InstanceScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);

            KrikthirGUID.Clear();
            HadronoxGUID.Clear();
            AnubarakGUID.Clear();
            WatcherGashraGUID.Clear();
            WatcherSilthikGUID.Clear();
            WatcherNarjilGUID.Clear();

            KrikthirDoorGUID.Clear();
            AnubarakDoorGUID[0].Clear();
            AnubarakDoorGUID[1].Clear();
            AnubarakDoorGUID[2].Clear();
        }

        ObjectGuid KrikthirGUID;
        ObjectGuid HadronoxGUID;
        ObjectGuid AnubarakGUID;
        ObjectGuid WatcherGashraGUID;
        ObjectGuid WatcherSilthikGUID;
        ObjectGuid WatcherNarjilGUID;

        ObjectGuid KrikthirDoorGUID;
        ObjectGuid AnubarakDoorGUID[3];

        uint32 Encounter[MAX_ENCOUNTER];

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_KRIKTHIR:        
                    KrikthirGUID = creature->GetGUID();        
                    break;
                case NPC_HADRONOX:        
                    HadronoxGUID = creature->GetGUID();        
                    break;
                case NPC_ANUBARAK:        
                    AnubarakGUID = creature->GetGUID();        
                    break;
                case NPC_WATCHER_GASHRA:  
                    WatcherGashraGUID = creature->GetGUID();   
                    break;
                case NPC_WATCHER_SILTHIK: 
                    WatcherSilthikGUID = creature->GetGUID();  
                    break;
                case NPC_WATCHER_NARJIL:  
                    WatcherNarjilGUID = creature->GetGUID();   
                    break;
                case NPC_WEB_WRAP:
                    creature->SetLevel(creature->GetMap()->IsHeroic() ? 80 : 74);
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case 192395:
                    KrikthirDoorGUID = go->GetGUID();
                    HandleGameObject(ObjectGuid::Empty, GetBossState(BOSS_KRIKTHIR_THE_GATEWATCHER) == DONE, go);
                    break;
                case 192396:
                    AnubarakDoorGUID[0] = go->GetGUID();
                    break;
                case 192397:
                    AnubarakDoorGUID[1] = go->GetGUID();
                    break;
                case 192398:
                    AnubarakDoorGUID[2] = go->GetGUID();
                    break;
            }
        }

        ObjectGuid GetGuidData(uint32 id) const override
        {
            switch (id)
            {
                case DATA_KRIKTHIR_THE_GATEWATCHER:     return KrikthirGUID;
                case DATA_HADRONOX:                     return HadronoxGUID;
                case DATA_ANUBARAK:                     return AnubarakGUID;
                case DATA_WATCHER_GASHRA:               return WatcherGashraGUID;
                case DATA_WATCHER_SILTHIK:              return WatcherSilthikGUID;
                case DATA_WATCHER_NARJIL:               return WatcherNarjilGUID;
            }

            return ObjectGuid::Empty;
        }

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            switch (type)
            {
                case BOSS_KRIKTHIR_THE_GATEWATCHER:
                    if (state == DONE)
                        HandleGameObject(KrikthirDoorGUID, true);
                    break;
                case BOSS_ANUBARAK:
                    if (state == IN_PROGRESS)
                        for (uint8 i = 0; i < 3; ++i)
                            HandleGameObject(AnubarakDoorGUID[i], false);
                    else if (state == NOT_STARTED || state == DONE)
                        for (uint8 i = 0; i < 3; ++i)
                            HandleGameObject(AnubarakDoorGUID[i], true);
                    break;
            }
            return true;
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_azjol_nerub_InstanceScript(map);
    }
};

void AddSC_instance_azjol_nerub()
{
   new instance_azjol_nerub();
}
