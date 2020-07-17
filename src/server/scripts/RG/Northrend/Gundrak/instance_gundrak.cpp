/*
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2010-2018 Rising Gods <http://www.rising-gods.de/>
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
#include "gundrak.h"
#include "Player.h"
#include "Vehicle.h"
#include "TemporarySummon.h"
#include "ScriptedCreature.h"
#include "SmartAI.h"

#define MAX_ENCOUNTER     5

const Position RhinoSpawn = { 1881.84f, 743.69f, 136.17f, 3.18f };

/* GunDrak encounters:
0 - Slad'Ran
1 - Moorabi
2 - Drakkari Colossus
3 - Gal'Darah
4 - Eck the Ferocious
*/

struct instance_gundrak_InstanceMapScript : public InstanceScript
{
    instance_gundrak_InstanceMapScript(Map* map) : InstanceScript(map), timer(0), phase(0)
    {
        SetHeaders(DataHeader);
        isHeroic = map->IsHeroic();

        sladranStatueState            = GO_STATE_ACTIVE;
        moorabiStatueState            = GO_STATE_ACTIVE;
        drakkaricolossusStatueState   = GO_STATE_ACTIVE;
        galdarahStatueState           = GO_STATE_READY;
        bridgeState                   = GO_STATE_ACTIVE;
        collisionState                = GO_STATE_READY;

        memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
    }

    bool isHeroic;

    uint32 timer;
    uint32 phase;
    uint32 spell;

    ObjectGuid toActivateGUID;

    ObjectGuid sladranGUID;
    ObjectGuid moorabiGUID;
    ObjectGuid drakkaricolossusGUID;
    ObjectGuid galdarahGUID;
    ObjectGuid ecktheferociousGUID;

    ObjectGuid sladranAltarGUID;
    ObjectGuid moorabiAltarGUID;
    ObjectGuid drakkaricolossusAltarGUID;
    ObjectGuid sladRanStatueGUID;
    ObjectGuid moorabiStatueGUID;
    ObjectGuid drakkarcolossusStatueGUID;
    ObjectGuid galdarahStatueGUID;
    ObjectGuid ecktheferociousDoorGUID;
    ObjectGuid ecktheferociousDoorBehindGUID;
    ObjectGuid galdarahDoor1GUID;
    ObjectGuid galdarahDoor2GUID;
    ObjectGuid bridgeGUID;
    ObjectGuid collisionGUID;
    ObjectGuid collisionSupportGUID;

    uint32 m_auiEncounter[MAX_ENCOUNTER];

    GOState sladranStatueState;
    GOState moorabiStatueState;
    GOState drakkaricolossusStatueState;
    GOState galdarahStatueState;
    GOState bridgeState;
    GOState collisionState;

    GuidSet dwellerGUID;

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
            case NPC_SLAD_RAN:
                sladranGUID = creature->GetGUID(); 
                break;
            case NPC_MOORABI:
                moorabiGUID = creature->GetGUID(); 
                break;
            case NPC_GALDARAH:
                galdarahGUID = creature->GetGUID();
                break;
            case NPC_DRAKKARICOLOSSUS:
                drakkaricolossusGUID = creature->GetGUID(); 
                break;
            case NPC_ECK:
                ecktheferociousGUID = creature->GetGUID(); 
                break;
            case NPC_RUIN_DWELLER:
                if (creature->IsAlive())
                    dwellerGUID.insert(creature->GetGUID());
                break;
        }
    }

    void OnGameObjectCreate(GameObject* go)
    {
        switch (go->GetEntry())
        {
            case GO_SLADRAN_ALTAR:
                sladranAltarGUID = go->GetGUID();
                // Make sure that they start out as unusuable
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                if (m_auiEncounter[0] == DONE)
                {
                    if (sladranStatueState == GO_STATE_ACTIVE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    else
                    {
                        ++phase;
                        go->SetGoState(GO_STATE_ACTIVE);
                    }
                }
                break;
            case GO_MOORABI_ALTAR:
                moorabiAltarGUID = go->GetGUID();
                // Make sure that they start out as unusuable
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                if (m_auiEncounter[0] == DONE)
                {
                    if (moorabiStatueState == GO_STATE_ACTIVE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    else
                    {
                        ++phase;
                        go->SetGoState(GO_STATE_ACTIVE);
                    }
                }
                break;
            case GO_DRAKKARI_COLOSSUS_ALTAR:
                drakkaricolossusAltarGUID = go->GetGUID();
                // Make sure that they start out as unusuable
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                if (m_auiEncounter[0] == DONE)
                {
                    if (drakkaricolossusStatueState == GO_STATE_ACTIVE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    else
                    {
                        ++phase;
                        go->SetGoState(GO_STATE_ACTIVE);
                    }
                }
                break;
            case GO_SLADRAN_STATUE:
                sladRanStatueGUID = go->GetGUID();
                go->SetGoState(sladranStatueState);
                break;
            case GO_MOORABI_STATUE:
                moorabiStatueGUID = go->GetGUID();
                go->SetGoState(moorabiStatueState);
                break;
            case GO_GALDARAH_STATUE:
                galdarahStatueGUID = go->GetGUID();
                go->SetGoState(galdarahStatueState);
                break;
            case GO_DRAKKARI_COLOSSUS_STATUE:
                drakkarcolossusStatueGUID = go->GetGUID();
                go->SetGoState(drakkaricolossusStatueState);
                break;
            case GO_ECK_THE_FEROCIOUS_DOOR:
                ecktheferociousDoorGUID = go->GetGUID();
                if (isHeroic && m_auiEncounter[1] == DONE)
                    HandleGameObject(ObjectGuid::Empty, true, go);
                break;
            case GO_ECK_THE_FEROCIOUS_DOOR_BEHIND:
                ecktheferociousDoorBehindGUID = go->GetGUID();
                if (isHeroic && m_auiEncounter[4] == DONE)
                    HandleGameObject(ObjectGuid::Empty, true, go);
            case GO_GALDARAH_DOOR1:
                galdarahDoor1GUID = go->GetGUID();
                if (m_auiEncounter[3] == DONE)
                    HandleGameObject(ObjectGuid::Empty, true, go);
                break;
            case GO_GALDARAH_DOOR2:
                galdarahDoor2GUID = go->GetGUID();
                if (m_auiEncounter[3] == DONE)
                    HandleGameObject(ObjectGuid::Empty, true, go);
                break;
            case GO_BRIDGE:
                bridgeGUID = go->GetGUID();
                go->SetGoState(bridgeState);
                break;
            case GO_COLLISION:
                collisionGUID = go->GetGUID();
                go->SetGoState(collisionState);
                HandleBridgeSupport();
                break;
            case GO_COLLISION_SUPPORT:
                collisionSupportGUID = go->GetGUID();
                go->SetPhaseMask(0, true);
                break;
            default:
                break;
        }
    }

    void SetData(uint32 type, uint32 data)
    {
        switch (type)
        {
            case DATA_SLAD_RAN_EVENT:
                m_auiEncounter[0] = data;
                if (data == DONE)
                    if (GameObject* go = instance->GetGameObject(sladranAltarGUID))
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                break;
            case DATA_MOORABI_EVENT:
                m_auiEncounter[1] = data;
                if (data == DONE)
                {
                    if (GameObject* go = instance->GetGameObject(moorabiAltarGUID))
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    if (isHeroic)
                        HandleGameObject(ecktheferociousDoorGUID, true);
                }
                break;
            case DATA_DRAKKARI_COLOSSUS_EVENT:
                m_auiEncounter[2] = data;
                if (data == DONE)
                    if (GameObject* go = instance->GetGameObject(drakkaricolossusAltarGUID))
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                break;
            case DATA_GAL_DARAH_EVENT:
                m_auiEncounter[3] = data;
                if (data == DONE)
                {
                    HandleGameObject(galdarahDoor1GUID, true);
                    HandleGameObject(galdarahDoor2GUID, true);
                }
                break;
            case DATA_ECK_THE_FEROCIOUS_EVENT:
                m_auiEncounter[4] = data;
                if (isHeroic && data == DONE)
                    HandleGameObject(ecktheferociousDoorBehindGUID, true);
                break;
            default:
                break;
        }

        if (data == DONE)
            SaveToDB();
    }

    void SetGuidData(uint32 type, ObjectGuid data) override
    {
        if (type == DATA_RUIN_DWELLER_DIED)
            dwellerGUID.erase(data);
        else if (type == DATA_STATUE_ACTIVATE)
        {
            toActivateGUID = data;
            timer = 3500;
            ++phase;
        }
    }

    uint32 GetData(uint32 type) const
    {
        switch (type)
        {
            case DATA_SLAD_RAN_EVENT:           return m_auiEncounter[0];
            case DATA_MOORABI_EVENT:            return m_auiEncounter[1];
            case DATA_DRAKKARI_COLOSSUS_EVENT:  return m_auiEncounter[2];
            case DATA_GAL_DARAH_EVENT:          return m_auiEncounter[3];
            case DATA_ECK_THE_FEROCIOUS_EVENT:  return m_auiEncounter[4];
            case DATA_ALIVE_RUIN_DWELLERS:      return dwellerGUID.size();
            default:                            break;
        }

        return 0;
    }

    ObjectGuid GetGuidData(uint32 type) const override
    {
        switch (type)
        {
            case DATA_SLAD_RAN_ALTAR:           return sladranAltarGUID;
            case DATA_MOORABI_ALTAR:            return moorabiAltarGUID;
            case DATA_DRAKKARI_COLOSSUS_ALTAR:  return drakkaricolossusAltarGUID;
            case DATA_SLAD_RAN_STATUE:          return sladRanStatueGUID;
            case DATA_MOORABI_STATUE:           return moorabiStatueGUID;
            case DATA_DRAKKARI_COLOSSUS_STATUE: return drakkarcolossusStatueGUID;
            case DATA_DRAKKARI_COLOSSUS:        return drakkaricolossusGUID;
            case DATA_STATUE_ACTIVATE:          return toActivateGUID;
            default:                            break;
        }

        return ObjectGuid::Empty;
    }

    std::string GetSaveData()
    {
        OUT_SAVE_INST_DATA;

        std::ostringstream saveStream;
        saveStream << "G D " << m_auiEncounter[0] << ' ' << m_auiEncounter[1] << ' '
                             << m_auiEncounter[2] << ' ' << m_auiEncounter[3] << ' ' << m_auiEncounter[4] << ' '
                             << (uint32) (sladRanStatueGUID ? GetObjState(sladRanStatueGUID)                 : GO_STATE_ACTIVE) << ' '
                             << (uint32) (moorabiStatueGUID ? GetObjState(moorabiStatueGUID)                 : GO_STATE_ACTIVE) << ' '
                             << (uint32) (drakkarcolossusStatueGUID ? GetObjState(drakkarcolossusStatueGUID) : GO_STATE_ACTIVE) << ' '
                             << (uint32) (galdarahStatueGUID ? GetObjState(galdarahStatueGUID)               : GO_STATE_READY)  << ' '
                             << (uint32) (bridgeGUID ? GetObjState(bridgeGUID)                               : GO_STATE_ACTIVE) << ' '
                             << (uint32) (collisionGUID ? GetObjState(collisionGUID)                         : GO_STATE_READY);

        str_data = saveStream.str();

        OUT_SAVE_INST_DATA_COMPLETE;
        return str_data;
    }

    void Load(const char* in)
    {
        if (!in)
        {
            OUT_LOAD_INST_DATA_FAIL;
            return;
        }

        OUT_LOAD_INST_DATA(in);

        char dataHead1, dataHead2;
        uint16 data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10;

        std::istringstream loadStream(in);
        loadStream >> dataHead1 >> dataHead2 >> data0 >> data1 >> data2 >> data3 >> data4 >> data5 >> data6 >> data7 >> data8 >> data9 >> data10;

        if (dataHead1 == 'G' && dataHead2 == 'D')
        {
            m_auiEncounter[0]             = data0;
            m_auiEncounter[1]             = data1;
            m_auiEncounter[2]             = data2;
            m_auiEncounter[3]             = data3;
            m_auiEncounter[4]             = data4;
            sladranStatueState            = GOState(data5);
            moorabiStatueState            = GOState(data6);
            drakkaricolossusStatueState   = GOState(data7);
            galdarahStatueState           = GOState(data8);
            bridgeState                   = GOState(data9);
            collisionState                = GOState(data10);

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
                if (m_auiEncounter[i] == IN_PROGRESS)
                    m_auiEncounter[i] = NOT_STARTED;
        } else OUT_LOAD_INST_DATA_FAIL;

        OUT_LOAD_INST_DATA_COMPLETE;
    }

    void OnPlayerEnter(Player* /*player*/)
    {
        HandleBridgeSupport();
    }

    void HandleBridgeSupport()
    {
        if (collisionState == GO_STATE_ACTIVE_ALTERNATIVE)
        {
            if (GameObject* collision = instance->GetGameObject(collisionGUID))
                collision->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

            if (GameObject* support = instance->GetGameObject(collisionSupportGUID))
                support->SetPhaseMask(1, true);
        }
    }

    void Update(uint32 diff)
    {
        // If there is nothing to activate, then return
        if (!toActivateGUID)
            return;

        if (timer < diff)
        {
            timer = 0;
            if (toActivateGUID == bridgeGUID)
            {
                GameObject* bridge                 = instance->GetGameObject(bridgeGUID);
                GameObject* collision              = instance->GetGameObject(collisionGUID);
                GameObject* sladRanStatue          = instance->GetGameObject(sladRanStatueGUID);
                GameObject* moorabiStatue          = instance->GetGameObject(moorabiStatueGUID);
                GameObject* drakkariColossusStatue = instance->GetGameObject(drakkarcolossusStatueGUID);
                GameObject* galDarahStatue         = instance->GetGameObject(galdarahStatueGUID);

                toActivateGUID.Clear();

                if (bridge && collision && sladRanStatue && moorabiStatue && drakkariColossusStatue && galDarahStatue)
                {
                    bridge->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    collision->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    sladRanStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    moorabiStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    drakkariColossusStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    galDarahStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);
                    galDarahStatue->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

                    collisionState = GO_STATE_ACTIVE_ALTERNATIVE;
                    HandleBridgeSupport();
                         
                    if (Creature* rhino = instance->SummonCreature(NPC_RHINO, RhinoSpawn))
                        rhino->GetMotionMaster()->MovePath(NPC_RHINO * 10, false);
                    if (Creature* galhadar = instance->GetCreature(galdarahGUID))
                        galhadar->setFaction(FACTION_HOSTILE);
                    SaveToDB();
                }
            }
            else
            {
                GameObject* altar = NULL;
                if (toActivateGUID == sladRanStatueGUID)
                {
                    spell = SPELL_FIRE_BEAM_SNAKE;
                    altar = instance->GetGameObject(sladranAltarGUID);
                }
                else if (toActivateGUID == moorabiStatueGUID)
                {
                    spell = SPELL_FIRE_BEAM_MAMMOTH;
                    altar = instance->GetGameObject(moorabiAltarGUID);
                }
                else if (toActivateGUID == drakkarcolossusStatueGUID)
                {
                    spell = SPELL_FIRE_BEAM_ELEMENTAL;
                    altar = instance->GetGameObject(drakkaricolossusAltarGUID);
                }

                // This is a workaround to make the beam cast properly. The caster should be ID 30298 but since the spells
                // all are with scripted target for that same ID, it will hit itself.
                if (altar)
                    if (Creature* trigger = altar->SummonCreature(NPC_TRIGGER_ALTAR, altar->GetPositionX(), altar->GetPositionY(), altar->GetPositionZ() + 3, altar->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN, 5000))
                    {
                        // Set the trigger model to invisible
                        trigger->SetDisplayId(11686);
                        trigger->CastSpell(trigger, spell, false);
                    }

                if (GameObject* statueGO = instance->GetGameObject(toActivateGUID))
                    statueGO->SetGoState(GO_STATE_READY);

                toActivateGUID.Clear();

                if (phase == 3)
                    SetGuidData(DATA_STATUE_ACTIVATE, bridgeGUID);
                else
                    SaveToDB(); // Don't save in between last statue and bridge turning in case of crash leading to stuck instance
            }
        }
        else
            timer -= diff;
    }

    GOState GetObjState(ObjectGuid guid)
    {
        if (GameObject* go = instance->GetGameObject(guid))
            return go->GetGoState();
        return GO_STATE_ACTIVE;
    }
};

class go_gundrak_altar : public GameObjectScript
{
public:
    go_gundrak_altar() : GameObjectScript("go_gundrak_altar") { }

    bool OnGossipHello(Player* /*player*/, GameObject* go)
    {
        InstanceScript* instance = go->GetInstanceScript();
        ObjectGuid statueGUID;

        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
        go->SetGoState(GO_STATE_ACTIVE);

        if (instance)
        {
            switch (go->GetEntry())
            {
                case GO_SLADRAN_ALTAR:
                    statueGUID = instance->GetGuidData(DATA_SLAD_RAN_STATUE);
                    break;
                case GO_MOORABI_ALTAR:
                    statueGUID = instance->GetGuidData(DATA_MOORABI_STATUE);
                    break;
                case GO_DRAKKARI_COLOSSUS_ALTAR:
                    statueGUID = instance->GetGuidData(DATA_DRAKKARI_COLOSSUS_STATUE);
                    break;
            }

            if (!instance->GetGuidData(DATA_STATUE_ACTIVATE))
            {
                instance->SetGuidData(DATA_STATUE_ACTIVATE, statueGUID);
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                go->SetGoState(GO_STATE_ACTIVE);
            }

            return true;
        }
        return false;
    }
};

enum RhinoMisc
{
    SPELL_DEAFENING_ROAR    = 55663,
    SPELL_CHARGE            = 55530,

    EVENT_RHINO_ROAR        = 1,
};

Position const RhinoExits[] =
{
    {1776.019f, 739.07f, 119.44f, 3.24f},
    {1775.118f, 748.51f, 119.44f, 3.24f},
    {1769.025f, 743.31f, 119.44f, 3.24f}
};

struct npc_gundrak_drakkari_rhino_platformAI : public CreatureAI
{
    npc_gundrak_drakkari_rhino_platformAI(Creature* creature) : CreatureAI(creature) { }
       
    void UpdateAI(uint32 diff) override
    {
        if (UpdateVictim())
            return;

        _events.Update(diff);

        if (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_RHINO_ROAR:
                    DoCastVictim(SPELL_DEAFENING_ROAR);
                    _events.ScheduleEvent(eventId, Seconds(urand(14, 20)));
                    break;
                default:
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }

    void EnterCombat(Unit* /*who*/) override
    {
        for (int i = 0; i < 3; ++i)
            if (Unit* passenger = me->GetVehicleKit()->GetPassenger(i))
            {
                passenger->ExitVehicle(&RhinoExits[i]);

                if(me->GetVictim())
                    passenger->SetInCombatWith(me->GetVictim());
            }

        DoCast(me->GetVictim(), SPELL_CHARGE);

        _events.Reset();
        _events.ScheduleEvent(EVENT_RHINO_ROAR, Seconds(urand(5, 11)));
    }
private:
    EventMap _events;
};

enum CobraGUIDs
{
    COBRA_GUID_1 = 127027,
    COBRA_GUID_2 = 127026,
    COBRA_GUID_3 = 127016
};

struct npc_constrictorAI : public SmartAI
{
    npc_constrictorAI(Creature* creature) : SmartAI(creature) { }

    void MoveInLineOfSight(Unit* who) override
    {
        if (me->GetSpawnId() == COBRA_GUID_1 || me->GetSpawnId() == COBRA_GUID_2 || me->GetSpawnId() == COBRA_GUID_3)
            if (!who || me->GetDistance2d(who) > 12.0f)
                return;

        SmartAI::MoveInLineOfSight(who);
    }
};

void AddSC_instance_gundrak()
{
    new InstanceScriptLoaderEx<instance_gundrak_InstanceMapScript>("instance_gundrak", 604);
    new go_gundrak_altar;
    new CreatureScriptLoaderEx<npc_gundrak_drakkari_rhino_platformAI>("npc_gundrak_drakkari_rhino_platform");
    new CreatureScriptLoaderEx<npc_constrictorAI>("npc_constrictor");
}
