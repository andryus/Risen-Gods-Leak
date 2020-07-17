#include "Encounter.h"
#include "InstanceMap.h"
#include "Player.h"
#include "LoadScript.h"
#include "EncounterHooks.h"
#include "LFGMgr.h"

Encounter::Encounter(InstanceMap& map, std::string_view scriptName, bool isLastEncounter) :
    map(map), scriptName(std::move(scriptName)), state(State::DEFAULT), isLastEncounter(isLastEncounter) {}

Encounter::~Encounter()
{
    unloadScript(*this);
}

void Encounter::Init()
{
    InitScript(scriptName);
}

void Encounter::Start()
{
    ASSERT(state != State::STARTED);
    ASSERT(activePlayers.empty());

    const Map::PlayerList players = map.GetPlayers();

    for (auto& player : players)
    {
    	player.SetEncounter(weak_from_this());
        *this <<= Fire::AddToEncounter.Bind(&player);

        activePlayers.push_back(player.GetSharedPtr());
    }

    state = State::STARTED;

    *this <<= Fire::EncounterStarted;
}

void Encounter::End()
{
    ASSERT(state == State::STARTED);

    *this <<= Fire::EncounterEnded;

    state = State::DEFAULT;
    Clear();
}

void Encounter::Finish(bool save)
{
    if (state == State::FINISHED)
        TC_LOG_ERROR("server.script", "Finish was called twice on encounter (Script: %s)", scriptName.data());
    else
    {
        if (IsStarted())
            End();

        state = State::FINISHED;

        if (save)
        {
            if (isLastEncounter)
                map.FinishDungeon();

            map.SaveToDB();
        }

        *this <<= Fire::EncounterFinished;
    }
}

void Encounter::Clear()
{
    for (auto& player : activePlayers)
    {
        if (auto ptr = player.lock())
            ptr->SetEncounter({});
    }
    
    activePlayers.clear();
}

void Encounter::Update(std::chrono::milliseconds diff)
{
    *this <<= Fire::ProcessEvents.Bind(diff);
}

InstanceMap& Encounter::GetInstance() const
{
    return map;
}

bool Encounter::IsStarted() const
{
    return state == State::STARTED;
}

bool Encounter::IsFinished() const
{
    return state == State::FINISHED;
}

bool Encounter::IsPlayerActive(const Player& player) const
{
    for (auto& activePlayer : activePlayers)
    {
        if (activePlayer.lock().get() == &player)
            return true;
    }

    return false;
}

constexpr auto FINISHED_DATA = "finished";

void Encounter::ApplyData(const DataMap& data)
{
    auto itr = data.find(FINISHED_DATA);
    if (itr != data.end() && itr->second != 0)
        Finish(false);
}

Encounter::DataVector Encounter::AquireData() const
{
    return { { FINISHED_DATA, state == State::FINISHED }};
}

std::string Encounter::OnQueryOwnerName() const
{
    return std::string(scriptName);
}

bool Encounter::OnInitScript(std::string_view scriptName)
{
    return loadScriptType(*this, scriptName);
}
