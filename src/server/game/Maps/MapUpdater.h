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

#pragma once

#include "Define.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <map>
#include "ProducerConsumerQueue.h"

class MapUpdateRequest;
class Map;

class MapUpdater
{
    private:
        struct UpdateEntry
        {
            uint32 lastUpdated;
            Map* map;

            bool toBeRemoved;

            float GetEstimatedUpdateTime() const;

            UpdateEntry(uint32 lastUpdated, Map *map) : lastUpdated(lastUpdated), map(map), toBeRemoved(false) {}
        };

    public:
        MapUpdater(size_t threadCount);
        ~MapUpdater();

        void SetThreadCount(size_t num_threads);

        void AddMap(Map* map);
        void RemoveMap(Map* map);

        void Cleanup();

        void StartUpdates(uint32 timespan);
        void Wait();

        float GetMaximumEstimatedUpdateTime() const;
    private:
        std::mutex entryMutex;
        std::map<Map*, UpdateEntry*> maps;
        std::multimap<uint32, UpdateEntry*> updateEntries;
        uint32 stopTime;

        std::mutex syncMutex;
        std::vector<std::thread*> workerThreads;
        std::condition_variable startCondition;
        std::condition_variable finishCondition;
        std::atomic<uint32> runningThreads;
        std::atomic<size_t> expectedThreads;

        void WorkerThread();
        UpdateEntry* FindNextUpdateMap(uint32& now);
        bool CheckForShutdown();
};
