#pragma once

#include <folly/futures/ManualExecutor.h>
#include <folly/executors/FutureExecutor.h>

template<typename T>
class ExecutionContext
{
public:
    std::shared_ptr<folly::FutureExecutor<folly::ManualExecutor>> GetExecutor() const
    {
        return _executor;
    }

    /**
     * Schedules a function call for this object. The call will be executed (possibly in another thread) in a context 
     * that allows any threadunsafe operation on this object.
     * @param f A function that may performs threadunsafe operation on this object.
     * @return a future containing the result of the function call.
     */
    template<typename F>
    auto Schedule(F&& f)
    {
        return _executor->addFuture([this, call = std::move(f)]() { return call(static_cast<T&>(*this)); });
    }

protected:
    void Run() // non-const as the queued functions may modify the inheriting object
    {
        _executor->run();
    }
    
private:
    std::shared_ptr<folly::FutureExecutor<folly::ManualExecutor>> _executor{ std::make_shared<folly::FutureExecutor<folly::ManualExecutor>>() };
};
