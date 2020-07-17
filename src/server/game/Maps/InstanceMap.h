#pragma once
#include "Map.h"
#include "Scriptable.h"
#include "Encounter.h"

enum InstanceResetMethod
{
    INSTANCE_RESET_ALL,
    INSTANCE_RESET_CHANGE_DIFFICULTY,
    INSTANCE_RESET_GLOBAL,
    INSTANCE_RESET_GROUP_DISBAND,
    INSTANCE_RESET_GROUP_JOIN,
    INSTANCE_RESET_RESPAWN_DELAY
};


class GAME_API InstanceMap final :
    public Map
{
public:
    InstanceMap(uint32 id, time_t, uint32 InstanceId, uint8 SpawnMode, uint8 team, Map* _parent);
    ~InstanceMap();
    
    InstanceMap(const InstanceMap&) = delete;
    void operator=(const InstanceMap&) = delete;

    void LoadFromDB();
    void SaveToDB() const;

    void FinishDungeon();

    bool AddPlayerToMap(std::shared_ptr<Player>) override;
    void RemovePlayerFromMap(std::shared_ptr<Player>) override;
    void Update(const uint32) override;
    void CreateInstanceData(bool load);
    bool Reset(uint8 method);
    uint32 GetScriptId() const { return instanceInfo ? instanceInfo->ScriptId : 0; }
    uint8 GetTeamId() const { return activeTeam; }
    InstanceScript* GetInstanceScript() { return i_data.get(); }
    void PermBindAllPlayers(Player* source);
    void UnloadAll() override;
    bool CanEnter(Player* player) override;
    void SendResetWarnings(uint32 timeLeft) const;
    void SetResetSchedule(bool on);

    uint32 GetMaxPlayers() const;
    uint32 GetMaxResetDelay() const;

    virtual void InitVisibilityDistance() override;

    bool IsEncounterInProgress() const;
    uint32 GetCompletedEncounterMask() const;

private:

    bool CanPlayerEnter(const Player& player) const;
    Encounter* GetActiveEncounter() const;


    void OnWorldObjectAdded(WorldObject& object) override;

    bool OnInitScript(std::string_view scriptName) override;
    std::string OnQueryOwnerName() const override;

    uint32 dungeonId;
    bool m_resetAfterUnload;
    bool m_unloadWhenEmpty;
    uint8 activeTeam;
    std::unique_ptr<InstanceScript> i_data;
    const InstanceTemplate* instanceInfo;

    std::unordered_map<uint32, std::shared_ptr<Encounter>> encounters;
};
