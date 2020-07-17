#pragma once

#include <functional>

namespace util
{

class ScopedOperation final
{
public:
    template<typename OnExit>
    ScopedOperation(OnExit&& exit) :
            on_exit_(std::forward<OnExit>(exit))
    {}

    template<typename OnEnter, typename OnExit>
    ScopedOperation(OnEnter&& enter, OnExit&& exit) :
            ScopedOperation(std::forward<OnExit>(exit))
    {
        enter();
    }

    ~ScopedOperation()
    {
        on_exit_();
    }


private:
    std::function<void()> on_exit_;
};

}