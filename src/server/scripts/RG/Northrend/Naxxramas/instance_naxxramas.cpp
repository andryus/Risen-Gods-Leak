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
#include "naxxramas.h"
#include "Player.h"
#include "CreatureTextMgr.h"

const DoorData doorData[] =
{
    {181126,    BOSS_ANUBREKHAN, DOOR_TYPE_ROOM     },
    {181195,    BOSS_ANUBREKHAN, DOOR_TYPE_PASSAGE  },
    {194022,    BOSS_FAERLINA,   DOOR_TYPE_PASSAGE  },
    {181209,    BOSS_FAERLINA,   DOOR_TYPE_PASSAGE  },
    {181209,    BOSS_MAEXXNA,    DOOR_TYPE_ROOM     },
    {181200,    BOSS_NOTH,       DOOR_TYPE_ROOM     },
    {181201,    BOSS_NOTH,       DOOR_TYPE_PASSAGE  },
    {181202,    BOSS_NOTH,       DOOR_TYPE_PASSAGE  },
    {181202,    BOSS_HEIGAN,     DOOR_TYPE_ROOM     },
    {181203,    BOSS_HEIGAN,     DOOR_TYPE_PASSAGE  },
    {181241,    BOSS_HEIGAN,     DOOR_TYPE_PASSAGE  },
    {181241,    BOSS_LOATHEB,    DOOR_TYPE_ROOM     },
    {181123,    BOSS_PATCHWERK,  DOOR_TYPE_PASSAGE  },
    {181123,    BOSS_GROBBULUS,  DOOR_TYPE_ROOM     },
    {181120,    BOSS_GLUTH,      DOOR_TYPE_PASSAGE  },
    {181121,    BOSS_GLUTH,      DOOR_TYPE_PASSAGE  },
    {181121,    BOSS_THADDIUS,   DOOR_TYPE_ROOM     },
    {181124,    BOSS_RAZUVIOUS,  DOOR_TYPE_PASSAGE  },
    {181124,    BOSS_GOTHIK,     DOOR_TYPE_ROOM     },
    {181125,    BOSS_GOTHIK,     DOOR_TYPE_PASSAGE  },
    {181119,    BOSS_GOTHIK,     DOOR_TYPE_PASSAGE  },
    {181119,    BOSS_HORSEMEN,   DOOR_TYPE_ROOM     },
    {181225,    BOSS_SAPPHIRON,  DOOR_TYPE_PASSAGE  },
    {181228,    BOSS_KELTHUZAD,  DOOR_TYPE_ROOM     },
    {0,         0,               DOOR_TYPE_ROOM     }, // EOF
};

Position const YellTriggerSummonPos[4] =
{
    {2909.0f, -3997.4099f, 274.1879f, 0.0f},
    {2520.5f, -2955.375f, 245.634f, 0.0f},
    {3514.0f, -2930.024f, 302.902f, 0.0f},
    {3511.37f, -3921.580f, 299.513f, 0.0f},
};

const MinionData minionData[] =
{
    {16506,     BOSS_FAERLINA},
    {16803,     BOSS_RAZUVIOUS},
    {0,         0, }
};

enum eEnums
{
    GO_HORSEMEN_CHEST_HERO  = 193426,
    GO_HORSEMEN_CHEST       = 181366,                   //four horsemen event, DoRespawnGameObject() when event == DONE
    GO_GOTHIK_GATE          = 181170,
    GO_KELTHUZAD_PORTAL01   = 181402,
    GO_KELTHUZAD_PORTAL02   = 181403,
    GO_KELTHUZAD_PORTAL03   = 181404,
    GO_KELTHUZAD_PORTAL04   = 181405,
    GO_KELTHUZAD_TRIGGER    = 181444,

    GO_PORTAL_SPIDER        = 181575,
    GO_PORTAL_ABOM          = 181576,
    GO_PORTAL_PLAGUE        = 181577,
    GO_PORTAL_DEATHKNIGHT   = 181578,

    GO_PORTAL_SPIDER_SHINE = 181233,
    GO_PORTAL_ABOM_SHINE   = 181232,
    GO_PORTAL_PLAGUE_SHINE = 181231,
    GO_PORTAL_DEATHKNIGHT_SHINE = 181230,
    
    GO_DEATHKNIGHT_WING_EYE = 181210,
    GO_PLAGUE_WING_EYE      = 181211,
    GO_SPIDER_WING_EYE      = 181212,
    GO_ABOM_WING_EYE        = 181213,

    SPELL_ERUPTION          = 29371,

    NPC_POISEN              = 16027,
};

const float HeiganPos[2] = {2796, -3707};
const float HeiganEruptionSlope[3] =
{
    (-3685 - HeiganPos[1]) /(2724 - HeiganPos[0]),
    (-3647 - HeiganPos[1]) /(2749 - HeiganPos[0]),
    (-3637 - HeiganPos[1]) /(2771 - HeiganPos[0]),
};

// 0  H      x
//  1        ^
//   2       |
//    3  y<--o
inline uint32 GetEruptionSection(float x, float y)
{
    y -= HeiganPos[1];
    if (y < 1.0f)
        return 0;

    x -= HeiganPos[0];
    if (x > -1.0f)
        return 3;

    float slope = y/x;
    for (uint32 i = 0; i < 3; ++i)
        if (slope > HeiganEruptionSlope[i])
            return i;
    return 3;
}

class instance_naxxramas : public InstanceMapScript
{
public:
    instance_naxxramas() : InstanceMapScript("instance_naxxramas", 533) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_naxxramas_InstanceMapScript(pMap);
    }

    struct instance_naxxramas_InstanceMapScript : public InstanceScript
    {
        instance_naxxramas_InstanceMapScript(Map* pMap) : InstanceScript(pMap)
        {
            SetHeaders(DataHeader);
            SetBossNumber(MAX_BOSS_NUMBER);
            LoadDoorData(doorData);
            LoadMinionData(minionData);

            horsemenBeserk = 0;
            playerDied = 0;
        }

        std::set<ObjectGuid> HeiganEruptionGUID[4];
        ObjectGuid GothikGateGUID;
        ObjectGuid HorsemenChestGUID;
        ObjectGuid SapphironGUID;
        ObjectGuid anubrekanGUID;
        ObjectGuid uiFaerlina;
        ObjectGuid uiThane;
        ObjectGuid uiLady;
        ObjectGuid uiBaron;
        ObjectGuid uiSir;

        ObjectGuid uiGluth;
        ObjectGuid uiThaddius;
        ObjectGuid uiHeigan;
        ObjectGuid uiLoatheb;
        ObjectGuid uiFeugen;
        ObjectGuid uiStalagg;

        ObjectGuid uiKelthuzad;
        ObjectGuid uiKelthuzadTrigger;
        ObjectGuid uiPortals[4];

        GOState gothikDoorState;

        uint32 horsemenBeserk;

        uint32 AbominationCount;
        uint32 SchockingCount;

        ObjectGuid PortalMaexxnaGUID;
        ObjectGuid PortalThaddiusGUID;
        ObjectGuid PortalLoathebGUID;
        ObjectGuid Portal4HorseGUID;

        ObjectGuid DeathknightWingEyeGUID[2];
        ObjectGuid SpiderWingEyeGUID[2];
        ObjectGuid AbomWingEyeGUID[2];
        ObjectGuid PlagueWingEyeGUID[2];
        uint32 playerDied;
        EventMap events;

        void OnCreatureCreate(Creature* creature)
        {
            switch(creature->GetEntry())
            {
                case 15956: anubrekanGUID = creature->GetGUID(); return;
                case 15989: SapphironGUID = creature->GetGUID(); return;
                case 15953: uiFaerlina = creature->GetGUID(); return;
                case 16064: uiThane = creature->GetGUID(); return;
                case 16065: uiLady = creature->GetGUID(); return;
                case 30549: uiBaron = creature->GetGUID(); return;
                case 16063: uiSir = creature->GetGUID(); return;
                case 15932: uiGluth = creature->GetGUID(); return;
                case 15928: uiThaddius = creature->GetGUID(); return;
                case 16011: uiLoatheb = creature->GetGUID(); return;
                case 15936: uiHeigan = creature->GetGUID(); return;
                case 15930: uiFeugen = creature->GetGUID(); return;
                case 15929: uiStalagg = creature->GetGUID(); return;
                case 15990: uiKelthuzad = creature->GetGUID(); return;
            }

            AddMinion(creature, true);
        }

        void OnCreatureRemove(Creature* creature)
        {
            AddMinion(creature, false);
        }

        void OnGameObjectCreate(GameObject* go)
        {
            if (go->GetGOInfo()->displayId == 6785 || go->GetGOInfo()->displayId == 1287)
            {
                uint32 section = GetEruptionSection(go->GetPositionX(), go->GetPositionY());
                HeiganEruptionGUID[section].insert(go->GetGUID());

                return;
            }

            switch (go->GetEntry())
            {
                case GO_GOTHIK_GATE:
                    GothikGateGUID = go->GetGUID();
                    go->SetGoState(gothikDoorState);
                    break;
                case GO_HORSEMEN_CHEST:
                    HorsemenChestGUID = go->GetGUID();
                    break;
                case GO_HORSEMEN_CHEST_HERO:
                    HorsemenChestGUID = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL01:
                    uiPortals[0] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL02:
                    uiPortals[1] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL03:
                    uiPortals[2] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL04:
                    uiPortals[3] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_TRIGGER:
                    uiKelthuzadTrigger = go->GetGUID();
                    break;
                case GO_PORTAL_SPIDER:
                    PortalMaexxnaGUID = go->GetGUID();
                    if (GetBossState(BOSS_MAEXXNA) == DONE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    else
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_PORTAL_ABOM:
                    PortalThaddiusGUID = go->GetGUID();
                    if (GetBossState(BOSS_THADDIUS) == DONE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    else
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_PORTAL_PLAGUE:
                    PortalLoathebGUID = go->GetGUID();
                    if (GetBossState(BOSS_LOATHEB) == DONE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    else
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_PORTAL_DEATHKNIGHT:
                    Portal4HorseGUID = go->GetGUID();
                    if (GetBossState(BOSS_HORSEMEN) == DONE)
                        go->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    else
                        go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                    break;
                case GO_DEATHKNIGHT_WING_EYE:
                    DeathknightWingEyeGUID[1] = go->GetGUID();
                    if (GetBossState(BOSS_HORSEMEN) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_SPIDER_WING_EYE:
                    SpiderWingEyeGUID[1] = go->GetGUID();
                    if (GetBossState(BOSS_MAEXXNA) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_PLAGUE_WING_EYE:
                    PlagueWingEyeGUID[1] = go->GetGUID();
                    if (GetBossState(BOSS_LOATHEB) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_ABOM_WING_EYE:
                    AbomWingEyeGUID[1] = go->GetGUID();
                    if (GetBossState(BOSS_THADDIUS) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_PORTAL_SPIDER_SHINE:
                    SpiderWingEyeGUID[0] = go->GetGUID();
                    if (GetBossState(BOSS_MAEXXNA) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_PORTAL_ABOM_SHINE:
                    AbomWingEyeGUID[0] = go->GetGUID();
                    if (GetBossState(BOSS_THADDIUS) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_PORTAL_PLAGUE_SHINE:
                    PlagueWingEyeGUID[0] = go->GetGUID();
                    if (GetBossState(BOSS_LOATHEB) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;
                case GO_PORTAL_DEATHKNIGHT_SHINE:
                    DeathknightWingEyeGUID[0] = go->GetGUID();
                    if (GetBossState(BOSS_HORSEMEN) == DONE)
                        go->SetGoState(GO_STATE_ACTIVE);
                    break;

                default:
                    break;
            }

            AddDoor(go, true);
        }

        void OnGameObjectRemove(GameObject* go)
        {
            if (go->GetGOInfo()->displayId == 6785 || go->GetGOInfo()->displayId == 1287)
            {
                uint32 section = GetEruptionSection(go->GetPositionX(), go->GetPositionY());

                HeiganEruptionGUID[section].erase(go->GetGUID());
                return;
            }

            switch (go->GetEntry())
            {
                case GO_BIRTH:
                    if (SapphironGUID)
                    {
                        if (Creature* pSapphiron = instance->GetCreature(SapphironGUID))
                        {
                            /* Buggy because of unknown reason -> temporary disabled
                            for (uint32 i = BOSS_ANUBREKHAN; i < BOSS_SAPPHIRON; ++i)
                            {
                                if (GetBossState(i) != DONE)
                                    return;
                            }
                            */
                            pSapphiron->AI()->DoAction(DATA_SAPPHIRON_BIRTH);
                        }
                        return;
                    }
                    break;
                default:
                    break;
            }

            AddDoor(go, false);
        }

        void SetData(uint32 id, uint32 value)
        {
            switch(id)
            {
                case DATA_HEIGAN_ERUPT:
                    HeiganErupt(value);
                    break;
                case DATA_GOTHIK_GATE:
                    if (GameObject* gothikGate = instance->GetGameObject(GothikGateGUID))
                        gothikGate->SetGoState(GOState(value));
                    gothikDoorState = GOState(value);
                    break;
                case DATA_ABOMINATION_KILLED:
                    AbominationCount = value;
                    break;
                case DATA_SHOCKING:
                    SchockingCount = value;
                    break;
                default:
                    break;

            }
        }

        ObjectGuid GetGuidData(uint32 id) const override
        {
            switch(id)
            {
            case DATA_ANUBREKHAN:
                return anubrekanGUID;
            case DATA_FAERLINA:
                return uiFaerlina;
            case DATA_THANE:
                return uiThane;
            case DATA_LADY:
                return uiLady;
            case DATA_BARON:
                return uiBaron;
            case DATA_SIR:
                return uiSir;
            case DATA_GLUTH:
                return uiGluth;
            case DATA_THADDIUS:
                return uiThaddius;
            case DATA_HEIGAN:
                return uiHeigan;
            case DATA_LOATHEB:
                return uiLoatheb;
            case DATA_FEUGEN:
                return uiFeugen;
            case DATA_STALAGG:
                return uiStalagg;
            case DATA_KELTHUZAD:
                return uiKelthuzad;
            case DATA_KELTHUZAD_PORTAL01:
                return uiPortals[0];
            case DATA_KELTHUZAD_PORTAL02:
                return uiPortals[1];
            case DATA_KELTHUZAD_PORTAL03:
                return uiPortals[2];
            case DATA_KELTHUZAD_PORTAL04:
                return uiPortals[3];
            case DATA_KELTHUZAD_TRIGGER:
                return uiKelthuzadTrigger;
            }
            return ObjectGuid::Empty;
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            if (id == BOSS_HORSEMEN && state == DONE)
            {
                if (GameObject* pHorsemenChest = instance->GetGameObject(HorsemenChestGUID))
                    pHorsemenChest->SetRespawnTime(pHorsemenChest->GetRespawnDelay());

                if (GameObject* pHorsePortal = instance->GetGameObject(Portal4HorseGUID))
                    pHorsePortal->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                if (GameObject* deathknightWingEye = instance->GetGameObject(DeathknightWingEyeGUID[0]))
                    deathknightWingEye->SetGoState(GO_STATE_ACTIVE);
                if (GameObject* deathknightWingEye = instance->GetGameObject(DeathknightWingEyeGUID[1]))
                    deathknightWingEye->SetGoState(GO_STATE_ACTIVE);

                if (Unit* proxy = instance->SummonCreature(NPC_KELTHUZAD_PROXY, YellTriggerSummonPos[1]))
                {
                    proxy->ToCreature()->AI()->Talk(SAY_TAUNT1);
                }
            }

            if (id == BOSS_THADDIUS && state == DONE)
            {
                if (GameObject* pThaddiusPortal = instance->GetGameObject(PortalThaddiusGUID))
                    pThaddiusPortal->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                if (GameObject* abomWingEye = instance->GetGameObject(AbomWingEyeGUID[0]))
                    abomWingEye->SetGoState(GO_STATE_ACTIVE);
                if (GameObject* abomWingEye = instance->GetGameObject(AbomWingEyeGUID[1]))
                    abomWingEye->SetGoState(GO_STATE_ACTIVE);

                if (Unit* proxy = instance->SummonCreature(NPC_KELTHUZAD_PROXY, YellTriggerSummonPos[2]))
                {
                    proxy->ToCreature()->AI()->Talk(SAY_TAUNT2);
                }
            }

            if (id == BOSS_MAEXXNA && state == DONE)
            {
                if (GameObject* pMaexxnaPortal = instance->GetGameObject(PortalMaexxnaGUID))
                    pMaexxnaPortal->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                if (GameObject* spiderWingEye = instance->GetGameObject(SpiderWingEyeGUID[0]))
                    spiderWingEye->SetGoState(GO_STATE_ACTIVE);
                if (GameObject* spiderWingEye = instance->GetGameObject(SpiderWingEyeGUID[1]))
                    spiderWingEye->SetGoState(GO_STATE_ACTIVE);

                if (Unit* proxy = instance->SummonCreature(NPC_KELTHUZAD_PROXY, YellTriggerSummonPos[3]))
                {
                    proxy->ToCreature()->AI()->Talk(SAY_TAUNT3);
                }
            }

            if (id == BOSS_LOATHEB && state == DONE)
            {
                if (GameObject* pLoathebPortal = instance->GetGameObject(PortalLoathebGUID))
                    pLoathebPortal->RemoveFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE);
                if (GameObject* plagueWingEye = instance->GetGameObject(PlagueWingEyeGUID[0]))
                    plagueWingEye->SetGoState(GO_STATE_ACTIVE);
                if (GameObject* plagueWingEye = instance->GetGameObject(PlagueWingEyeGUID[1]))
                    plagueWingEye->SetGoState(GO_STATE_ACTIVE);

                if (Unit* proxy = instance->SummonCreature(NPC_KELTHUZAD_PROXY, YellTriggerSummonPos[0]))
                {
                    proxy->ToCreature()->AI()->Talk(SAY_TAUNT4);
                }
            }

            if (id == BOSS_GOTHIK && state == DONE)
            {
                events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_KORTHAZZ, 10000);
            }
            return true;
        }

        void HeiganErupt(uint32 section)
        {
            for (uint32 i = 0; i < 4; ++i)
            {
                if (i == section)
                    continue;

                for (ObjectGuid guid : HeiganEruptionGUID[i])
                {
                    if (GameObject* pHeiganEruption = instance->GetGameObject(guid))
                    {
                        pHeiganEruption->SendCustomAnim(pHeiganEruption->GetGoAnimProgress());
                        pHeiganEruption->CastSpell(NULL, SPELL_ERUPTION);
                    }
                }
            }
        }

        uint32 GetData(uint32 id) const
        {
            bool allDead = true;
            switch (id)
            {
                case DATA_HORSEMEN_BESERK:
                    return horsemenBeserk;
                    break;
                case DATA_ALL_ENCOUNTERS_DEAD:
                    for (int i = BOSS_ANUBREKHAN; i < MAX_BOSS_NUMBER; ++i )
                    {
                        if (GetBossState(i) != DONE)
                            allDead = false;
                    }

                    if (allDead)
                        return 1;
                    else
                        return 0;
                    break;
                case DATA_ABOMINATION_KILLED:
                    return AbominationCount;
                    break;
                case DATA_SHOCKING:
                    return SchockingCount;
                    break;
                default:
                    return 0;
            }
        }

        void Update(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_DIALOGUE_GOTHIK_KORTHAZZ:
                        if (Creature* korthazz = instance->GetCreature(uiThane))
                            sCreatureTextMgr->SendChat(korthazz, SAY_DIALOGUE_GOTHIK_HORSEMAN, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_ZELIEK, 5 * IN_MILLISECONDS);
                        break;
                    case EVENT_DIALOGUE_GOTHIK_ZELIEK:
                        if (Creature* zeliek = instance->GetCreature(uiSir))
                            sCreatureTextMgr->SendChat(zeliek, SAY_DIALOGUE_GOTHIK_HORSEMAN, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_BLAUMEUX, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_DIALOGUE_GOTHIK_BLAUMEUX:
                        if (Creature* blaumeux = instance->GetCreature(uiLady))
                            sCreatureTextMgr->SendChat(blaumeux, SAY_DIALOGUE_GOTHIK_HORSEMAN, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_RIVENDARE, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_DIALOGUE_GOTHIK_RIVENDARE:
                        if (Creature* rivendare = instance->GetCreature(uiBaron))
                            sCreatureTextMgr->SendChat(rivendare, SAY_DIALOGUE_GOTHIK_HORSEMAN, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_BLAUMEUX2, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_DIALOGUE_GOTHIK_BLAUMEUX2:
                        if (Creature* blaumeux = instance->GetCreature(uiLady))
                            sCreatureTextMgr->SendChat(blaumeux, SAY_DIALOGUE_GOTHIK_HORSEMAN2, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_ZELIEK2, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_DIALOGUE_GOTHIK_ZELIEK2:
                        if (Creature* zeliek = instance->GetCreature(uiSir))
                            sCreatureTextMgr->SendChat(zeliek, SAY_DIALOGUE_GOTHIK_HORSEMAN2, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_KORTHAZZ2, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_DIALOGUE_GOTHIK_KORTHAZZ2:
                        if (Creature* korthazz = instance->GetCreature(uiThane))
                            sCreatureTextMgr->SendChat(korthazz, SAY_DIALOGUE_GOTHIK_HORSEMAN2, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        events.ScheduleEvent(EVENT_DIALOGUE_GOTHIK_RIVENDARE2, 6 * IN_MILLISECONDS);
                        break;
                    case EVENT_DIALOGUE_GOTHIK_RIVENDARE2:
                        if (Creature* rivendare = instance->GetCreature(uiBaron))
                            sCreatureTextMgr->SendChat(rivendare, SAY_DIALOGUE_GOTHIK_HORSEMAN2, 0, CHAT_MSG_ADDON, LANG_ADDON, TEXT_RANGE_ZONE);
                        break;
                    default:
                        break;
                }
            }
        }

        // This Function is called in CheckAchievementCriteriaMeet and CheckAchievementCriteriaMeet is called before SetBossState(bossId, DONE),
        // so to check if all bosses are done the checker must exclude 1 boss, the last done, if there is at most 1 encouter in progress when is
        // called this function then all bosses are done. The one boss that check is the boss that calls this function, so it is dead.
        bool AreAllEncoutersDone()
        {
            uint32 numBossAlive = 0;
            for (uint32 i = 0; i < MAX_BOSS_NUMBER; ++i)
                if (GetBossState(i) != DONE)
                    numBossAlive++;
            if (numBossAlive > 1)
                return false;
            return true;
        }

        bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target = NULL*/, uint32 /*miscvalue1 = 0*/)
        {
            switch(criteria_id)
            {
                // And They Would All Go Down Together (kill 4HM within 15sec of each other)
                case 7600: // 25-man
                case 7601: // 10-man
                    if (criteria_id + instance->GetSpawnMode() == 7601)
                        return false;
                    if (Creature* baron = instance->GetCreature(uiBaron)) // it doesn't matter which one we use, really
                        return (baron->AI()->GetData(DATA_HORSEMEN_CHECK_ACHIEVEMENT_CREDIT) == 1u);
                return false;
                    // Difficulty checks are done on DB.
                    // Criteria for achievement 2186: The Immortal (25-man)
                case 13233: // The Four Horsemen
                case 13234: // Maexxna
                case 13235: // Thaddius
                case 13236: // Loatheb
                case 7616: // Kel'Thuzad
                    // Criteria for achievement 2187: The Undying (10-man)
                case 13237: // The Four Horsemen
                case 13238: // Maexxna
                case 13239: // Loatheb
                case 13240: // Thaddius
                case 7617: // Kel'Thuzad
                    if (AreAllEncoutersDone() && !playerDied)
                        return true;
                    return false;
            }
            return false;
        }

        void ResetInstance()
        {
            const ObjectGuid bossGUID[13] = { SapphironGUID, uiFaerlina, uiThane, uiLady, uiBaron, uiSir, uiGluth, uiThaddius,
                uiLoatheb, uiHeigan, uiFeugen, uiStalagg, uiKelthuzad};

            uint8 counter = 0;
            for (Player& player : instance->GetPlayers())
            {
                if (player.IsAlive() && !player.IsGameMaster())
                    ++counter;
            }

            // Called when player leaves the instance, executed before actual removal
            // So when the last player leaves the counter is 1 and becomes 0 later
            if (counter > 1)
                return;

            for (uint8 i = 0; i < MAX_BOSS_NUMBER; ++i)
            {
                uint32 tmpState = GetBossState(i);
                if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                    SetBossState(i, NOT_STARTED);
            }

            for (uint8 i = 0; i < 13; ++i)
            {
                if (Creature* boss = instance->GetCreature(bossGUID[i]))
                    boss->AI()->EnterEvadeMode();
            }
        }

        void OnPlayerRemove(Player* /*player*/)
        {
            ResetInstance();
        }

        void OnUnitDeath(Unit* unit)
        {
            if (unit->GetTypeId() == TYPEID_PLAYER && IsEncounterInProgress())
            {
                playerDied = 1;
                SaveToDB();
            }
        }

        void WriteSaveDataMore(std::ostringstream& data) override
        {
            data << gothikDoorState << ' '// Gothicdoor
                << playerDied;
        }

        void ReadSaveDataMore(std::istringstream& data) override
        {
            uint32 buff, buff2;

            data >> buff;
            gothikDoorState = GOState(buff);
            data >> buff2;
            playerDied = buff2;
        }
    };
};


class at_naxxramas_frostwyrm_wing : public AreaTriggerScript
{
    public:
        at_naxxramas_frostwyrm_wing() : AreaTriggerScript("at_naxxramas_frostwyrm_wing") { }

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*areaTrigger*/)
    {
        /* This check doesn't apply because the script is called AFTER the players are portet
           Therefore: Remove the teleport from the database and hardcode the port.
        if (player->IsGameMaster())
            return false;

        InstanceScript* instance = player->GetInstanceScript();
        if (instance)
        {
            for (uint32 i = BOSS_ANUBREKHAN; i < BOSS_SAPPHIRON; ++i)
            {
                if (instance->GetBossState(i) != DONE)
                    return true;
            }
            return false;
        }

        return true;
        */

        // DT Version: Replace areatrigger_teleport by areatrigger_script in SQL and hardcode the port.
        InstanceScript* pInstance = pPlayer->GetInstanceScript();
        if (!pPlayer || pPlayer->isDead() || !pInstance || pInstance->IsEncounterInProgress())
            return false;


        if (((pInstance->GetBossState(BOSS_HORSEMEN) == DONE) &&
             (pInstance->GetBossState(BOSS_MAEXXNA) == DONE) &&
             (pInstance->GetBossState(BOSS_LOATHEB) == DONE) &&
             (pInstance->GetBossState(BOSS_THADDIUS) == DONE)) ||
             pPlayer->IsGameMaster())
        {
            pPlayer->TeleportTo(533, 3498.28f, -5349.9f, 144.968f, 1.31324f);
        } else
            return false;

        return true;
    }
};

#define    SPELL_EMBALMING_CLOUD       28322

class naxx_npc_slime : public CreatureScript
{
    public:
        naxx_npc_slime() : CreatureScript("naxx_npc_slime") { }

        struct naxx_npc_slimeAI : public ScriptedAI
        {
            naxx_npc_slimeAI(Creature* c) : ScriptedAI(c) {}

            uint32 embalmingCloud;

            void Reset()
            {
                embalmingCloud = 10000;
            }

            void EnterCombat(Unit* /*who*/)
            {
                me->CallForHelp(100.0f);
            }

            void UpdateAI(uint32 uiDiff)
            {
                if (!UpdateVictim())
                    return;

                if (embalmingCloud < uiDiff)
                {
                    DoCastAOE(SPELL_EMBALMING_CLOUD);
                    embalmingCloud = 10000;
                }
                else embalmingCloud -= uiDiff;

                DoMeleeAttackIfReady();
            }
        };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new naxx_npc_slimeAI(pCreature);
    }
};

class npc_toxic_trigger : public CreatureScript
{
    public:
        npc_toxic_trigger() : CreatureScript("npc_toxic_trigger") { }

        struct npc_toxic_triggerAI : public ScriptedAI
        {
            npc_toxic_triggerAI(Creature *pCreature) : ScriptedAI(pCreature) { }

            void AttackStart(Unit* /*pWho*/)
            {
                EnterEvadeMode(EVADE_REASON_OTHER);
            }

            void EnterEvadeMode(EvadeReason /*why*/) override
            {
                me->RemoveAllAuras();
                me->GetThreatManager().ClearAllThreat();
                me->CombatStop(true);
                me->LoadCreaturesAddon();
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
    return new npc_toxic_triggerAI(creature);
    }
};

class npc_mr_bigglesworth : public CreatureScript
{
    public:
        npc_mr_bigglesworth() : CreatureScript("npc_mr_bigglesworth") { }

        struct npc_mr_bigglesworthAI : public ScriptedAI
        {
            npc_mr_bigglesworthAI(Creature *Creature) : ScriptedAI(Creature)  { }

            void JustDied(Unit* /*Who*/)
            {
                if (Creature* proxy = me->SummonCreature(NPC_KELTHUZAD_PROXY, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 3000))
                {
                    proxy->AI()->Talk(0);
                }
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_mr_bigglesworthAI(creature);
    }
};

class npc_lichking_proxy : public CreatureScript
{
    public:
        npc_lichking_proxy() : CreatureScript("npc_lichking_proxy") { }

        struct npc_lichking_proxyAI : public ScriptedAI
        {
            npc_lichking_proxyAI(Creature *Creature) : ScriptedAI(Creature)  { }

            uint8 Step;
            uint32 PhaseTimer;
            bool SpeechStart;

            void JumpToNextStep(uint32 timer)
            {
                PhaseTimer = timer;
                ++Step;
            }

            void Reset()
            {
                Step = 0;
                PhaseTimer = 1000;
                SpeechStart = false;
            }

            void DoAction(int32 action)
            {
                switch(action)
                {
                    case ACTION_SAPP_DIALOG:
                        SpeechStart = true;
                        break;
                }
            }

            void UpdateAI(uint32 diff)
            {
                if (SpeechStart)
                {
                    if (PhaseTimer <= diff)
                    {
                        switch (Step)
                        {
                            case 0:
                                JumpToNextStep(5000);
                                break;
                            case 1:
                                if (Creature* Kel = me->SummonCreature(NPC_KELTHUZAD_PROXY, Speech, TEMPSUMMON_TIMED_DESPAWN, 3000))
                                    Kel->AI()->Talk(SAY_SAPP_DIALOG1);
                                JumpToNextStep(7000);
                                break;
                            case 2:
                                Talk(SAY_SAPP_DIALOG2_LICH);
                                JumpToNextStep(15000);
                                break;
                            case 3:
                                if (Creature* Kel = me->SummonCreature(NPC_KELTHUZAD_PROXY, Speech, TEMPSUMMON_TIMED_DESPAWN, 3000))
                                    Kel->AI()->Talk(SAY_SAPP_DIALOG3);
                                JumpToNextStep(8000);
                                break;
                            case 4:
                                Talk(SAY_SAPP_DIALOG4_LICH);
                                JumpToNextStep(8500);
                                break;
                            case 5:
                                if (Creature* Kel = me->SummonCreature(NPC_KELTHUZAD_PROXY, Speech, TEMPSUMMON_TIMED_DESPAWN, 3000))
                                    Kel->AI()->Talk(SAY_SAPP_DIALOG5);
                                me->DespawnOrUnsummon();
                                break;
                        }
                    } else PhaseTimer -= diff;
                }
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lichking_proxyAI(creature);
    }
};

class npc_naxxramas_maggot : public CreatureScript
{
    public:
        npc_naxxramas_maggot() : CreatureScript("npc_naxxramas_maggot") { }

        struct npc_naxxramas_maggotAI : public ScriptedAI
        {
            npc_naxxramas_maggotAI(Creature *Creature) : ScriptedAI(Creature) { }

            void Reset()
            {
                me->SetCorpseDelay(0);
            }

            void UpdateAI(uint32 /*uiDiff*/)
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_naxxramas_maggotAI(creature);
    }
};

class npc_eye_stalk : public CreatureScript
{
    public:
        npc_eye_stalk() : CreatureScript("npc_eye_stalk") { }

        struct npc_eye_stalkAI : public ScriptedAI
        {
            npc_eye_stalkAI(Creature *Creature) : ScriptedAI(Creature) { }

            uint32 mindflay_timer;

            void Reset()
            {
                me->SetCorpseDelay(0);
                SetCombatMovement(false);
                mindflay_timer = 0;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (!me->IsInRange(me->GetVictim(), 0, RAID_MODE(45, 35)))
                {
                    EnterEvadeMode();
                }

                if (mindflay_timer <= diff)
                {
                    DoCastVictim(RAID_MODE(29407, 54805), true);
                    mindflay_timer = urand(14000, 17000);
                }
                else
                {
                    mindflay_timer -= diff;
                }
            }
        };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eye_stalkAI(creature);
    }
};

void AddSC_instance_naxxramas()
{
    new instance_naxxramas();
    new at_naxxramas_frostwyrm_wing();
    new naxx_npc_slime();
    new npc_toxic_trigger();
    new npc_mr_bigglesworth();
    new npc_lichking_proxy();
    new npc_naxxramas_maggot();
    new npc_eye_stalk();
}
