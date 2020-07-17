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
#include "steam_vault.h"

class go_main_chambers_access_panel : public GameObjectScript
{
    public:
        go_main_chambers_access_panel() : GameObjectScript("go_main_chambers_access_panel") { }

        bool OnGossipHello(Player* /*player*/, GameObject* go) override
        {
            InstanceScript* instance = go->GetInstanceScript();
            if (!instance)
                return false;

            if (go->GetEntry() == GO_ACCESS_PANEL_HYDRO)
                if (instance->GetBossState(DATA_HYDROMANCER_THESPIA) == DONE)
                {
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    instance->SetBossState(DATA_HYDROMANCER_THESPIA, SPECIAL);
                }

            if (go->GetEntry() == GO_ACCESS_PANEL_MEK)
                if (instance->GetBossState(DATA_MEKGINEER_STEAMRIGGER) == DONE)
                {
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    instance->SetBossState(DATA_MEKGINEER_STEAMRIGGER, SPECIAL);
                }

            return true;
        }
};

class instance_steam_vault : public InstanceMapScript
{
    public:
        instance_steam_vault() : InstanceMapScript(SteamVaultScriptName, 545) { }

        struct instance_steam_vault_InstanceMapScript : public InstanceScript
        {
            instance_steam_vault_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);

                ThespiaGUID          .Clear();
                MekgineerGUID        .Clear();
                KalithreshGUID       .Clear();

                MainChambersDoorGUID .Clear();
                AccessPanelHydro     .Clear();
                AccessPanelMek       .Clear();
                DistillerState       = 0;
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_HYDROMANCER_THESPIA:
                        ThespiaGUID = creature->GetGUID();
                        if (!creature->IsAlive())
                            SetBossState(DATA_HYDROMANCER_THESPIA, SPECIAL);
                        break;
                    case NPC_MEKGINEER_STEAMRIGGER:
                        MekgineerGUID = creature->GetGUID();
                        if (!creature->IsAlive())
                            SetBossState(DATA_MEKGINEER_STEAMRIGGER, SPECIAL);
                        break;
                    case NPC_WARLORD_KALITHRESH:
                        KalithreshGUID = creature->GetGUID();
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_MAIN_CHAMBERS_DOOR:
                        MainChambersDoorGUID = go->GetGUID();
                        if (GetBossState(DATA_HYDROMANCER_THESPIA) == SPECIAL && GetBossState(DATA_MEKGINEER_STEAMRIGGER) == SPECIAL)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_ACCESS_PANEL_HYDRO:
                        AccessPanelHydro = go->GetGUID();
                        if (GetBossState(DATA_HYDROMANCER_THESPIA) == DONE)
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        else if (GetBossState(DATA_HYDROMANCER_THESPIA) == SPECIAL)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    case GO_ACCESS_PANEL_MEK:
                        AccessPanelMek = go->GetGUID();
                        if (GetBossState(DATA_MEKGINEER_STEAMRIGGER) == DONE)
                            go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        else if (GetBossState(DATA_MEKGINEER_STEAMRIGGER) == SPECIAL)
                            HandleGameObject(ObjectGuid::Empty, true, go);
                        break;
                    default:
                        break;
                }
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_HYDROMANCER_THESPIA:
                        return ThespiaGUID;
                    case DATA_MEKGINEER_STEAMRIGGER:
                        return MekgineerGUID;
                    case DATA_WARLORD_KALITHRESH:
                        return KalithreshGUID;
                    default:
                        break;
                }
                return ObjectGuid::Empty;
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type == DATA_DISTILLER)
                    DistillerState = data;
            }

            uint32 GetData(uint32 type) const override
            {
                if (type == DATA_DISTILLER)
                    return DistillerState;
                return 0;
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case DATA_HYDROMANCER_THESPIA:
                        if (state == SPECIAL)
                        {
                            if (GetBossState(DATA_MEKGINEER_STEAMRIGGER) == SPECIAL)
                                HandleGameObject(MainChambersDoorGUID, true);

                            TC_LOG_DEBUG("scripts", "Instance Steamvault: Access panel used.");
                        }
                        else if (state == DONE)
                        {
                            if (GameObject* panel = instance->GetGameObject(AccessPanelHydro))
                                panel->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        }
                        break;
                    case DATA_MEKGINEER_STEAMRIGGER:
                        if (state == SPECIAL)
                        {
                            if (GetBossState(DATA_HYDROMANCER_THESPIA) == SPECIAL)
                                HandleGameObject(MainChambersDoorGUID, true);

                            TC_LOG_DEBUG("scripts", "Instance Steamvault: Access panel used.");
                        }
                        else if (state == DONE)
                        {
                            if (GameObject* panel = instance->GetGameObject(AccessPanelMek))
                                panel->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                        }
                        break;
                    default:
                        break;
                }

                if (state == DONE || state == SPECIAL)
                    SaveToDB();

                return true;
            }

        protected:
            ObjectGuid ThespiaGUID;
            ObjectGuid MekgineerGUID;
            ObjectGuid KalithreshGUID;

            ObjectGuid MainChambersDoorGUID;
            ObjectGuid AccessPanelHydro;
            ObjectGuid AccessPanelMek;
            uint8 DistillerState;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_steam_vault_InstanceMapScript(map);
        }
};

void AddSC_instance_steam_vault()
{
    new go_main_chambers_access_panel();
    new instance_steam_vault();
}
