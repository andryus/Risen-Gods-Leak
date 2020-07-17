#pragma once

#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <queue>
#include <atomic>

#include "Define.h"

namespace util
{
namespace thread
{
class ThreadPool final
{
public:
    explicit ThreadPool() :
            queue_(),
            queue_lock_(),
            queue_not_empty_(),
            stop_(true),
            worker_()
    {
    }

    ~ThreadPool()
    {
        stop();
    }

    template<typename F, typename... Args>
    auto schedule(F&& fun, Args&& ... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_t = typename std::result_of<F(Args...)>::type;

        if (stop_)
        {
            return std::async(std::launch::deferred, std::forward<F>(fun), std::forward<Args>(args)...);
        }
        else
        {
            auto task = std::make_shared<std::packaged_task<return_t()>>(std::bind(std::forward<F>(fun), std::forward<Args>(args)...));

            std::future<return_t> future = task->get_future();

            {
                std::unique_lock<std::mutex> lock(queue_lock_);
                queue_.emplace([task]() { (*task)(); });
                queue_not_empty_.notify_one();
            }

            return future;
        }
    }

    void stop(bool flush_queue = true)
    {
        if (stop_)
            return;

        stop_ = true;
        queue_not_empty_.notify_all();
        for (std::thread& thread : worker_)
            thread.join();

        if (flush_queue)
        {
            while (!queue_.empty())
            {
                auto task = queue_.front();
                queue_.pop();

                task();
            }
        }
        else
        {
            while (!queue_.empty())
                queue_.pop();
        }
    }

    void start(uint32 thread_count = 1)
    {
        if (!stop_)
            return;

        stop_ = false;
        for (uint32 i = 0; i < thread_count; ++i)
            worker_.emplace_back(std::bind(&ThreadPool::run, this));
    }

    bool is_running() const
    {
        return !stop_;
    }

private:
    void run()
    {
        std::function<void()> task;

        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(queue_lock_);

                queue_not_empty_.wait(lock, [this]() { return stop_ || !queue_.empty(); });

                if (stop_)
                    return;

                task = std::move(queue_.front());
                queue_.pop();
            }
            task();
        }
    }

private:
    std::queue<std::function<void()>> queue_;
    std::mutex                        queue_lock_;
    std::condition_variable           queue_not_empty_;

    std::atomic<bool>        stop_;
    std::vector<std::thread> worker_;

};
}
}