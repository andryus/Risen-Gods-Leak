#include "EventQueue.h"

Events::EventQueue::EventQueue() :
    queue_(),
    queueMutex_(),
    queueNotEmpty_(),
    shutdown_(false),
    workers_()
{}

void Events::EventQueue::start(uint8_t threads)
{    
    for (uint8_t i = 0; i < threads; ++i)
        workers_.emplace_back(&EventQueue::doWork, this);
}

void Events::EventQueue::shutdown(bool join)
{
    shutdown_ = true;

    {
        std::unique_lock<std::mutex> guard(queueMutex_);
        queueNotEmpty_.notify_all();
    }

    if (join)
        for (auto& thread : workers_)
            thread.join();
}

void Events::EventQueue::schedule(Event* event)
{
    if (shutdown_)
        return;

    std::unique_lock<std::mutex> guard(queueMutex_);
    
    queue_.emplace_back(event);

    queueNotEmpty_.notify_one();
}

void Events::EventQueue::schedule(std::list<Event*> events)
{
    if (shutdown_)
        return;

    std::unique_lock<std::mutex> guard(queueMutex_);

    for (Event* i : events)
        queue_.emplace_back(i);

    queueNotEmpty_.notify_one();
}

auto Events::EventQueue::getEvent() -> std::unique_ptr<Event>
{
    std::unique_lock<std::mutex> guard(queueMutex_);
    
    queueNotEmpty_.wait(guard, [this] { return !queue_.empty() || shutdown_; });

    if (shutdown_)
        return std::unique_ptr<Event>();

    std::unique_ptr<Event> event = std::move(queue_.front());
    queue_.pop_front();

    return event;
}

void Events::EventQueue::doWork()
{
    while (true)
    {
        std::unique_ptr<Event> event = getEvent();
        if (!event)
            break;

        event->execute();
    }
}