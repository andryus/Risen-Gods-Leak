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
#include "Vehicle.h"
#include "InstanceScript.h"
#include "eye_of_eternity.h"
#include "Player.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Group.h"

class instance_eye_of_eternity : public InstanceMapScript
{
public:
    instance_eye_of_eternity() : InstanceMapScript("instance_eye_of_eternity", 616) {}

    InstanceScript* GetInstanceScript(InstanceMap* map) const
    {
        return new instance_eye_of_eternity_InstanceMapScript(map);
    }

    struct instance_eye_of_eternity_InstanceMapScript : public InstanceScript
    {
        instance_eye_of_eternity_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_ENCOUNTER);

            vortexTriggers.clear();
            portalTriggers.clear();

            proxyGUID.Clear();
            surgeTriggerGUID.Clear();
            malygosGUID.Clear();
            platformGUID.Clear();
            exitPortalGUID.Clear();
            raidLeaderGUID.Clear();
        };

        bool SetBossState(uint32 type, EncounterState state)
        {
            if (!InstanceScript::SetBossState(type, state))
                return false;

            if (type != DATA_MALYGOS_EVENT)
                return false;

            switch (state)
            {
                case FAIL:
                    for (std::list<ObjectGuid>::const_iterator itr_trigger = portalTriggers.begin(); itr_trigger != portalTriggers.end(); ++itr_trigger)
                    {
                        if (Creature* trigger = instance->GetCreature(*itr_trigger))
                        {
                            // just in case
                            trigger->RemoveAllAuras();
                            trigger->AI()->Reset();
                        }
                    }

                    if (instance->GetDifficulty() == RAID_DIFFICULTY_10MAN_NORMAL)
                        SpawnGameObject(GO_FOCUSING_IRIS_10, focusingIrisPosition);
                    else
                        SpawnGameObject(GO_FOCUSING_IRIS_25, focusingIrisPosition);

                    SpawnGameObject(GO_EXIT_PORTAL, exitPortalPosition);

                    if (GameObject* platform = instance->GetGameObject(platformGUID))
                        platform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);

                    break;
                case DONE:
                    if (Creature* malygos = instance->GetCreature(malygosGUID))
                        malygos->SummonCreature(NPC_ALEXSTRASZA, 718.544f, 1358.71f, 304.7453f, 2.32f);

                    SpawnGameObject(GO_EXIT_PORTAL, exitPortalPosition);

                    // HACKFIX: we make the platform appear again because at the moment we don't support looting using a vehicle
                    if (GameObject* platform = instance->GetGameObject(platformGUID))
                        platform->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    break;
                default:
                    break;
            }

            return true;
        }

        // There is no other way afaik...
        void SpawnGameObject(uint32 entry, Position& pos)
        {
            GameObject* go = GameObject::Create(entry, *instance, PHASEMASK_NORMAL, pos, G3D::Quat(), 120);
            if (!go)
                return;

            instance->AddToMap(go);
        }

        void OnGameObjectCreate(GameObject* go)
        {
            switch (go->GetEntry())
            {
                case GO_NEXUS_RAID_PLATFORM:
                    if (GetBossState(DATA_MALYGOS_EVENT) == DONE)
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    platformGUID = go->GetGUID();
                    break;
                case GO_FOCUSING_IRIS_10:
                case GO_FOCUSING_IRIS_25:
                    if (GetBossState(DATA_MALYGOS_EVENT) == DONE)  // we dont want te iris to appear after malygos is dead
                        go->SetPhaseMask(0, true);
                    go->GetPosition(&focusingIrisPosition);
                    break;
                case GO_EXIT_PORTAL:
                    exitPortalGUID = go->GetGUID();
                    go->GetPosition(&exitPortalPosition);
                    break;
            }
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch (creature->GetEntry())
            {
                case NPC_SURGE_OF_POWER:
                    surgeTriggerGUID = creature->GetGUID();
                    break;
                case NPC_PROXY:
                    proxyGUID = creature->GetGUID();
                    break;
                case NPC_VORTEX_TRIGGER:
                    vortexTriggers.push_back(creature->GetGUID());
                    break;
                case NPC_MALYGOS:
                    malygosGUID = creature->GetGUID();
                    break;
                case NPC_PORTAL_TRIGGER:
                    portalTriggers.push_back(creature->GetGUID());
                    break;
            }
        }

        void UpdateLighting(uint32 light, Player* player)
        {
            uint32 transition = light & 0xFFFF;
            uint32 newLight = light >> 16;

            WorldPacket data(SMSG_OVERRIDE_LIGHT, 12);
            data << uint32(LIGHT_NATIVE);
            data << newLight;
            data << transition;

            if (player)
                player->GetSession()->SendPacket(std::move(data));
            else
                instance->SendToPlayers(data);
        }

        void OnPlayerRemove(Player* player)
        {
            player->ClearUnitState(UNIT_STATE_ROOT);
        }

        void OnPlayerEnter(Player* player)
        {
            uint32 data = GetBossState(DATA_MALYGOS_EVENT) != DONE ? LIGHT_NATIVE << 16 : LIGHT_CLOUDS << 16;
            UpdateLighting(data, player);

            if (GetBossState(DATA_MALYGOS_EVENT) == DONE) {

                /*
                // HACKFIX
                if (player)
                    player->CastSpell(player, SPELL_SUMMON_RED_DRAGON, true);

                if (GameObject* go = instance->GetGameObject(platformGUID))
                {
                    go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_DESTROYED);
                    go->DestroyForNearbyPlayers();
                }
                */
            } else {
                // store GUID of the raid leader for loot
                if (player && !player->IsGameMaster() && player->GetGroup())
                    if (ObjectGuid leaderguid = player->GetGroup()->GetLeaderGUID())
                        if (ObjectAccessor::FindPlayer(leaderguid))
                            raidLeaderGUID = leaderguid;
            }

            if (GetBossState(DATA_MALYGOS_EVENT) == DONE)
                player->CastSpell(player, SPELL_SUMMON_RED_DRAGON, true);
        }

        void OnUnitDeath(Unit* unit)
        {
            if (unit->GetTypeId() != TYPEID_PLAYER)
                return;

            // Player continues to be moving after death no matter if spline will be cleared along with all movements,
            // so on next world tick was all about delay if box will pop or not (when new movement will be registered)
            // since in EoE you never stop falling. However root at this precise* moment works
            unit->SetControlled(true, UNIT_STATE_ROOT);
            unit->StopMoving();
        }

        void ProcessEvent(WorldObject* obj, uint32 eventId)
        {
            if (eventId != EVENT_FOCUSING_IRIS)
                return;

            if (GameObject* go = obj->ToGameObject())
                go->Delete(); // this is not the best way.

            if (Creature* malygos = instance->GetCreature(malygosGUID))
                malygos->AI()->DoAction(0 /* ACTION_INIT_PHASE_ONE */);

            if (GameObject* exitPortal = instance->GetGameObject(exitPortalGUID))
                exitPortal->Delete();
        }

        void VortexHandling()
        {
            if (Creature* malygos = instance->GetCreature(malygosGUID))
            {
                std::list<HostileReference*> m_threatlist = malygos->GetThreatManager().getThreatList();
                for (std::list<ObjectGuid>::const_iterator itr_vortex = vortexTriggers.begin(); itr_vortex != vortexTriggers.end(); ++itr_vortex)
                {
                    if (m_threatlist.empty())
                        return;

                    uint8 counter = 0;
                    if (Creature* trigger = instance->GetCreature(*itr_vortex))
                    {
                        // each trigger have to cast the spell to 5 players.
                        for (std::list<HostileReference*>::const_iterator itr = m_threatlist.begin(); itr!= m_threatlist.end(); ++itr)
                        {
                            if (counter >= 5)
                                break;

                            if (Unit* target = (*itr)->getTarget())
                            {
                                Player* player = target->ToPlayer();

                                if (!player || player->IsGameMaster() || player->HasAura(SPELL_VORTEX_4))
                                    continue;

                                player->CastSpell(trigger, SPELL_VORTEX_4, true);
                                counter++;
                            }
                        }
                    }
                }
            }
        }

        void PowerSparksHandling()
        {
            if (Creature* trigger = instance->GetCreature(Trinity::Containers::SelectRandomContainerElement(portalTriggers)))
            {
                trigger->CastSpell(trigger, SPELL_PORTAL_OPENED, true);
                return;
            }
        }

        void SetData(uint32 data, uint32 value)
        {
            switch (data)
            {
                case DATA_VORTEX_HANDLING:
                    VortexHandling();
                    break;
                case DATA_POWER_SPARKS_HANDLING:
                    PowerSparksHandling();
                    break;
                case DATA_LIGHTING:
                    UpdateLighting(value, NULL);
                    break;
            }
        }

        ObjectGuid GetGuidData(uint32 data) const
        {
            switch (data)
            {
                case DATA_SURGE_TRIGGER:
                    return surgeTriggerGUID;
                case DATA_PROXY:
                    return proxyGUID;
                case DATA_VORTEX_TRIGGER:
                    return vortexTriggers.front();
                case DATA_MALYGOS:
                    return malygosGUID;
                case DATA_PLATFORM:
                    return platformGUID;
                case DATA_RAID_LEADER_GUID:
                    return raidLeaderGUID;
            }

            return ObjectGuid::Empty;
        }

        private:
            std::list<ObjectGuid> vortexTriggers;
            std::list<ObjectGuid> portalTriggers;
            ObjectGuid surgeTriggerGUID;
            ObjectGuid proxyGUID;
            ObjectGuid malygosGUID;
            ObjectGuid platformGUID;
            ObjectGuid exitPortalGUID;
//            uint64 chestGUID;
            ObjectGuid raidLeaderGUID;
            Position focusingIrisPosition;
            Position exitPortalPosition;
    };
};

void AddSC_instance_eye_of_eternity()
{
    new instance_eye_of_eternity();
}
