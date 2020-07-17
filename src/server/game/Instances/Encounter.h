#pragma once
#include <memory>
#include <vector>
#include <chrono>
#include <unordered_map>
#include "Scriptable.h"

class Player;
class InstanceMap;

class GAME_API Encounter :
    public script::Scriptable, public std::enable_shared_from_this<Encounter>
{
public:
    enum class State : uint8
    {
        DEFAULT,
        STARTED,
        FINISHED
    };

    using DataVector = std::vector<std::pair<std::string_view, uint32>>;
    using DataMap = std::unordered_map<std::string_view, uint32>;

    Encounter(InstanceMap& map, std::string_view scriptName, bool isLastEncounter);
    ~Encounter();

    void Init();
    void Start();
    void End();
    void Finish(bool save = true);
    void Clear();
    void Update(std::chrono::milliseconds diff);

    InstanceMap& GetInstance() const;
    bool IsStarted() const;
    bool IsFinished() const;
    bool IsPlayerActive(const Player& player) const;

    void ApplyData(const DataMap& data);
    DataVector AquireData() const;

private:

    std::string OnQueryOwnerName() const override;
    bool OnInitScript(std::string_view scriptName) override;

    State state;
    bool isLastEncounter;
    InstanceMap& map;
    std::string_view scriptName;
    std::vector<std::weak_ptr<Player>> activePlayers;
};
