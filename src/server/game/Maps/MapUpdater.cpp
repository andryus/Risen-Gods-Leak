/*
* Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#include <mutex>
#include <condition_variable>

#include "Threading/ThreadName.hpp"
#include "MapUpdater.h"
#include "Map.h"
#include "ThreadsafetyDebugger.h"
#include "MapInstanced.h"
#include "World.h"
#include "Monitoring/Maps.hpp"
#include "GameTime.h"

const uint32 MAP_DELAY = 50;

float MapUpdater::UpdateEntry::GetEstimatedUpdateTime() const
{
    return std::min(map->GetDiffTimer().GetDurationAvg(), float(sWorld->getIntConfig(CONFIG_INTERVAL_MAPUPDATE)));
}

MapUpdater::MapUpdater(size_t num_threads)
{
    stopTime = 0;
    ASSERT(num_threads > 0);

    expectedThreads = num_threads;
    runningThreads = num_threads;

    std::unique_lock<std::mutex> lock(this->syncMutex);

    for (size_t i = 0; i < num_threads; ++i)
        workerThreads.push_back(new std::thread(&MapUpdater::WorkerThread, this));
}

MapUpdater::~MapUpdater()
{
    expectedThreads = 0;

    startCondition.notify_all();
    for (auto& t : workerThreads)
        t->join();

    for (auto kvp : updateEntries)
        delete kvp.second;
}

void MapUpdater::SetThreadCount(size_t num_threads)
{
    ASSERT(num_threads > 0);

    std::unique_lock<std::mutex> lock(this->syncMutex);

    if (num_threads == expectedThreads)
        return;

    expectedThreads = num_threads;

    // wake some threads they will die by themself
    if (expectedThreads < workerThreads.size())
        startCondition.notify_all();
    else
    {
        size_t threadsToSpawn = expectedThreads - workerThreads.size();
        runningThreads += threadsToSpawn;
        for (size_t i = 0; i < threadsToSpawn; ++i)
            workerThreads.push_back(new std::thread(&MapUpdater::WorkerThread, this));
    }
}

void MapUpdater::AddMap(Map* map)
{
    uint32 now = getMSTime();

    std::lock_guard<std::mutex> guard(entryMutex);

    auto entry = new UpdateEntry(now, map);
    updateEntries.emplace(now + MAP_DELAY, entry);
    maps[map] = entry;
}

void MapUpdater::RemoveMap(Map* map)
{
    std::lock_guard<std::mutex> guard(entryMutex);
    auto itr = maps.find(map);

    if (itr != maps.end())
        itr->second->toBeRemoved = true;
}

void MapUpdater::Cleanup()
{
    std::lock_guard<std::mutex> guard(entryMutex);
    ASSERT(runningThreads == 0);

    for(auto itr = maps.begin(); itr != maps.end();)
    {
        auto currItr = itr++;
        UpdateEntry* entry = currItr->second;

        if (entry->toBeRemoved)
        {
            for (auto entriesItr = updateEntries.begin(); entriesItr != updateEntries.end(); ++entriesItr)
            {
                if (entriesItr->second == entry)
                {
                    updateEntries.erase(entriesItr);
                    break;
                }
            }

            maps.erase(currItr);

            delete entry;
        }
    }
}

void MapUpdater::StartUpdates(uint32 timespan)
{
    std::lock_guard<std::mutex> guard(entryMutex);

    stopTime = getMSTime() + timespan;

    std::unique_lock<std::mutex> lock(this->syncMutex);
    runningThreads = this->workerThreads.size();
    startCondition.notify_all();
}

void MapUpdater::Wait()
{
    std::unique_lock<std::mutex> lock(this->syncMutex);
    while(runningThreads > 0)
        finishCondition.wait(lock);
}

float MapUpdater::GetMaximumEstimatedUpdateTime() const
{
    float max = -std::numeric_limits<float>::infinity();
    for (const auto itr : updateEntries)
    {
        const float updateTime = itr.second->GetEstimatedUpdateTime();
        if (max < updateTime)
            max = updateTime;
    }
    return max;
}

void MapUpdater::WorkerThread()
{
    util::thread::nameing::SetName("MapUpdater");

    while (!CheckForShutdown())
    {
        uint32 now = getMSTime();

        auto updateEntry = FindNextUpdateMap(now);
        if (!updateEntry)
        {
            std::unique_lock<std::mutex> lock(this->syncMutex);
            --runningThreads;
            finishCondition.notify_all();
            startCondition.wait(lock);
        }
        else
        {
            const uint32 diff = now - updateEntry->lastUpdated;
            updateEntry->lastUpdated = now;
            game::time::UpdateGameTimers();

            ThreadsafetyDebugger::Allow(ThreadsafetyDebugger::Context::NEEDS_MAP, updateEntry->map->GetId());
            auto latency = updateEntry->map->GetDiffTimer().StartTimer();
            updateEntry->map->Update(diff);
            auto duration = updateEntry->map->GetDiffTimer().StopTimer();
            ThreadsafetyDebugger::Disallow(ThreadsafetyDebugger::Context::NEEDS_MAP, updateEntry->map->GetId());

            MONITOR_MAPS(ReportUpdate(updateEntry->map, latency, duration));

            entryMutex.lock();
            updateEntries.emplace(now + MAP_DELAY, updateEntry);
            entryMutex.unlock();
        }
    }

    --runningThreads;
    finishCondition.notify_all();
}

MapUpdater::UpdateEntry* MapUpdater::FindNextUpdateMap(uint32& now)
{
    entryMutex.lock();
    auto itr = updateEntries.begin();
    while (itr != updateEntries.end())
    {
        auto updateEntry = itr->second;

        if (!updateEntry->toBeRemoved && stopTime >= std::max(itr->first, now) + uint32(updateEntry->GetEstimatedUpdateTime()))
        {
            uint32 sleepTime = 0;
            if (now < itr->first)
                sleepTime = itr->first - now;
            updateEntries.erase(itr);
            entryMutex.unlock();
            if (sleepTime)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
                now = getMSTime();
            }
            return updateEntry;
        }

        ++itr;
    }

    entryMutex.unlock();
    return nullptr;
}

bool MapUpdater::CheckForShutdown()
{
    std::unique_lock<std::mutex> lock(this->syncMutex);

    if (expectedThreads >= workerThreads.size())
        return false;

    auto itr = std::find_if(workerThreads.begin(), workerThreads.end(), [](const std::thread* thread) {return thread->get_id() == std::this_thread::get_id();});
    ASSERT(itr != workerThreads.end());
    workerThreads.erase(itr);

    return true;
}
