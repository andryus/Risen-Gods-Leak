#pragma once

#include <list>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "API.h"

namespace Events
{
    class COMMON_API Event
    {
    public:
        virtual ~Event() = default;
        virtual void execute() = 0;
    };

    class COMMON_API EventQueue
    {
        class Worker;
        friend class Worker;

    public:
        EventQueue();

        void start(uint8_t threads);
        void shutdown(bool join = true);

        void schedule(Event*);
        void schedule(std::list<Event*>);

    private:
        std::unique_ptr<Event> getEvent();

        void doWork();

    private:
        std::list<std::unique_ptr<Event>> queue_;
        std::mutex queueMutex_;
        std::condition_variable queueNotEmpty_;

        // Windows Bug: std::atomic_bool doesn't define a constructor -.-
        std::atomic<bool> shutdown_;
        std::vector<std::thread> workers_;
    };
}
