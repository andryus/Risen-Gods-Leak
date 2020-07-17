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
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "onyxias_lair.h"
#include "TemporarySummon.h"

class instance_onyxias_lair : public InstanceMapScript
{
    public:
        instance_onyxias_lair() : InstanceMapScript("instance_onyxias_lair", 249) { }
    
        struct instance_onyxias_lair_InstanceMapScript : public InstanceScript
        {
            instance_onyxias_lair_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(MAX_ENCOUNTER);

                OnyxiaGUID.Clear();

                OnyxiaLiftoffTimer = 0;
                ManyWhelpsCounter = 0;
                AchievManyWhelpsHandleIt = false;
                AchievSheDeepBreathMore = true;

                EruptTimer = 0;
            }
    
            //Eruption is a BFS graph problem
            //One map to remember all floor, one map to keep floor that still need to erupt and one queue to know what needs to be removed
            std::map<ObjectGuid, uint32> FloorEruptionGUID[2];
            std::queue<ObjectGuid> FloorEruptionGUIDQueue;
    
            ObjectGuid OnyxiaGUID;
    
    
            uint32 OnyxiaLiftoffTimer;
            uint32 ManyWhelpsCounter;
            uint32 EruptTimer;
    
            bool   AchievManyWhelpsHandleIt;
            bool   AchievSheDeepBreathMore;
    
            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_ONYXIA:
                        OnyxiaGUID = creature->GetGUID();
                        break;
                }
            }
    
            void OnGameObjectCreate(GameObject* go) override
            {
                if ((go->GetGOInfo()->displayId == 4392 || go->GetGOInfo()->displayId == 4472) && go->GetGOInfo()->trap.spellId == 17731)
                {
                    FloorEruptionGUID[0].insert(std::make_pair(go->GetGUID(), 0));
                    return;
                }
    
                switch (go->GetEntry())
                {
                    case GO_WHELP_SPAWNER:
                        if (Creature* onyxia = ObjectAccessor::GetCreature(*go, OnyxiaGUID))
                        {
                            if (onyxia->IsAlive())
                            {
                                Position goPos;
                                go->GetPosition(&goPos);
                                if (Creature* temp = go->SummonCreature(NPC_WHELP, goPos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10*IN_MILLISECONDS))
                                {
                                    temp->SetInCombatWithZone();
                                    ManyWhelpsCounter++;
                                }
                            }
                        }
                        break;
                }
            }
    
            void OnGameObjectRemove(GameObject* go) override
            {
                if ((go->GetGOInfo()->displayId == 4392 || go->GetGOInfo()->displayId == 4472) && go->GetGOInfo()->trap.spellId == 17731)
                {
                    FloorEruptionGUID[0].erase(go->GetGUID());
                    return;
                }
            }
    
            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;
    
                switch (state)
                {
                    case NOT_STARTED:
                        AchievSheDeepBreathMore = true;
                        DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT,  ACHIEV_TIMED_MORE_DOTS);
                        break;
                    case IN_PROGRESS:
                        DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT,  ACHIEV_TIMED_MORE_DOTS);
                        break;
                    case DONE:
                        if (CheckAchievementCriteriaMeet(instance->Is25ManRaid() ? ACHIEV_CRITERIA_DEEP_BREATH_25_PLAYER : ACHIEV_CRITERIA_DEEP_BREATH_10_PLAYER, NULL, NULL, 0))
                            DoCompleteAchievement(instance->Is25ManRaid() ? ACHIEVEMENT_DEEP_BREATH_25_PLAYER : ACHIEVEMENT_DEEP_BREATH_10_PLAYER);
    
                        if (CheckAchievementCriteriaMeet(instance->Is25ManRaid() ? ACHIEV_CRITERIA_MANY_WHELPS_25_PLAYER : ACHIEV_CRITERIA_MANY_WHELPS_10_PLAYER, NULL, NULL, 0))
                            DoCompleteAchievement(instance->Is25ManRaid() ? ACHIEVEMENT_MANY_WHELPS_25_PLAYER : ACHIEVEMENT_MANY_WHELPS_10_PLAYER);
                        break;
                    default:
                        break;
                }
                return true;
            }
    
            void SetData(uint32 type, uint32 data) override
            {
                switch (type)
                {
                    case DATA_ONYXIA_PHASE_2:
                        AchievManyWhelpsHandleIt = false;
                        ManyWhelpsCounter = 0;
                        OnyxiaLiftoffTimer = 10*IN_MILLISECONDS;
                        break;
                    case DATA_SHE_DEEP_BREATH_MORE:
                        if (data == FAIL)
                        {
                            AchievSheDeepBreathMore = false;
                        }
                        break;
                }
            }
    
            void SetGuidData(uint32 type, ObjectGuid data) override
            {
                switch (type)
                {
                    case DATA_FLOOR_ERUPTION:
                        FloorEruptionGUID[1] = FloorEruptionGUID[0];
                        FloorEruptionGUIDQueue.push(data);
                        EruptTimer = 2.5  * IN_MILLISECONDS;
                        break;
                }
            }
    
            void FloorEruption(ObjectGuid floorEruptedGUID)
            {
                if (GameObject* pFloorEruption = instance->GetGameObject(floorEruptedGUID))
                {
                    //THIS GOB IS A TRAP - What shall i do? =(
                    //Cast it spell? Copyed Heigan method
                    pFloorEruption->SendCustomAnim(pFloorEruption->GetGoAnimProgress());
                    pFloorEruption->CastSpell(NULL, Difficulty(instance->GetSpawnMode()) == RAID_DIFFICULTY_10MAN_NORMAL ? 17731 : 69294); //pFloorEruption->GetGOInfo()->trap.spellId
    
                    //Get all immediatly nearby floors
                    std::list<GameObject*> nearFloorList;
                    Trinity::GameObjectInRangeCheck check(pFloorEruption->GetPositionX(), pFloorEruption->GetPositionY(), pFloorEruption->GetPositionZ(), 15);

                    pFloorEruption->VisitAnyNearbyObject<GameObject, Trinity::ContainerAction>(999, nearFloorList, check);
                    //remove all that are not present on FloorEruptionGUID[1] and update treeLen on each GUID
                    for (std::list<GameObject*>::const_iterator itr = nearFloorList.begin(); itr != nearFloorList.end(); ++itr)
                    {
                        if (((*itr)->GetGOInfo()->displayId == 4392 || (*itr)->GetGOInfo()->displayId == 4472) && (*itr)->GetGOInfo()->trap.spellId == 17731)
                        {
                            ObjectGuid nearFloorGUID = (*itr)->GetGUID();
                            if (FloorEruptionGUID[1].find(nearFloorGUID) != FloorEruptionGUID[1].end() && (*FloorEruptionGUID[1].find(nearFloorGUID)).second == 0)
                            {
                                (*FloorEruptionGUID[1].find(nearFloorGUID)).second = (*FloorEruptionGUID[1].find(floorEruptedGUID)).second+1;
                                FloorEruptionGUIDQueue.push(nearFloorGUID);
                            }
                        }
                    }
                }
                FloorEruptionGUID[1].erase(floorEruptedGUID);
            }
    
            void Update(uint32 diff) override
            {
                if (GetBossState(BOSS_ONYXIA) == IN_PROGRESS)
                {
                    if (OnyxiaLiftoffTimer && OnyxiaLiftoffTimer <= diff)
                    {
                        OnyxiaLiftoffTimer = 0;
                        if (ManyWhelpsCounter >= 50)
                            AchievManyWhelpsHandleIt = true;
                    } else OnyxiaLiftoffTimer -= diff;
                }
    
                if (!FloorEruptionGUIDQueue.empty())
                {
                    if (EruptTimer <= diff)
                    {
                        uint32 treeHeight = 0;
                        do
                        {
                            treeHeight = (*FloorEruptionGUID[1].find(FloorEruptionGUIDQueue.front())).second;
                            FloorEruption(FloorEruptionGUIDQueue.front());
                            FloorEruptionGUIDQueue.pop();
                        } while (!FloorEruptionGUIDQueue.empty() && (*FloorEruptionGUID[1].find(FloorEruptionGUIDQueue.front())).second == treeHeight);
                        EruptTimer = 1 * IN_MILLISECONDS;
                    }
                    else
                        EruptTimer -= diff;
                }
            }
    
            bool CheckAchievementCriteriaMeet(uint32 criteriaId, Player const* /*source*/, Unit const* /*target = NULL*/, uint32 /*miscValue1 = 0*/)
            {
                switch (criteriaId)
                {
                    case ACHIEV_CRITERIA_MANY_WHELPS_10_PLAYER:  // Criteria for achievement 4403: Many Whelps! Handle It! (10 player) Hatch 50 eggs in 10s
                    case ACHIEV_CRITERIA_MANY_WHELPS_25_PLAYER:  // Criteria for achievement 4406: Many Whelps! Handle It! (25 player) Hatch 50 eggs in 10s
                        return AchievManyWhelpsHandleIt;
                    case ACHIEV_CRITERIA_DEEP_BREATH_10_PLAYER:  // Criteria for achievement 4404: She Deep Breaths More (10 player) Everybody evade Deep Breath
                    case ACHIEV_CRITERIA_DEEP_BREATH_25_PLAYER:  // Criteria for achievement 4407: She Deep Breaths More (25 player) Everybody evade Deep Breath
                        return AchievSheDeepBreathMore;
                }
                return false;
            }
        };
    
        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_onyxias_lair_InstanceMapScript(map);
        }
};

void AddSC_instance_onyxias_lair()
{
    new instance_onyxias_lair();
}
