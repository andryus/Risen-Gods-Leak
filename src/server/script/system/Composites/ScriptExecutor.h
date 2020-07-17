#pragma once
#include <memory>

#include "ScriptGenerator.h"
#include "ScriptInvokable.h"
#include "ScriptBindMacros.h"
#include "Log.h"

namespace script
{

    template<class ExecutorT>
    struct Executor : public Generator<ExecutorT>
    {
        using Generator<ExecutorT>::Generator;
        static constexpr bool DefaultReturn = true;

        template<class Ctx, class Context>
        static bool ValidateResult(bool result, const Ctx& ctx, Params<Context> params)
        {
            return result && ctx._exec.WasExecuted();
        }

        template<class Ctx, class Context>
        static bool ResultForFailure(Params<Context>)
        {
            return false;
        }
    };

    struct ExecInvokable
    {
        using Invokation = std::unique_ptr<InvokationInterface>;

        void _setInvokation(InvokationInterface& invokation) { _invokable = &invokation; }

        bool WasExecuted() const { return _execResult; }

        void Execute(ScriptLogger* logger) 
        { 
            ASSERT(_invokable); 
            _execResult |= _invokable->Invoke(logger);
        }

        Invokation ExecuteDelayed() 
        { 
            ASSERT(_invokable);

            _execResult = true;

            return _invokable->Clone(); 
        }

    private: 
        bool _execResult = false;
        InvokationInterface* _invokable = nullptr;
    };

#define _EXEC_APPEND  script::ExecInvokable _exec; private: void Execute() { _exec.Execute(logger); } typename script::ExecInvokable::Invokation ExecuteDelayed() {return _exec.ExecuteDelayed(); } public:

#define SCRIPT_EXEC(NAME, ME) \
    SCRIPT_COMPOSITE(Executor, Exec, void, NAME, ME, _SCRIPT_NO_INLINE, _EXEC_APPEND)

#define SCRIPT_EXEC_STATE(NAME, ME, STATE_T, STATE_N) \
    SCRIPT_COMPOSITE2(Executor, Exec, void, NAME, ME, STATE_T, STATE_N, _SCRIPT_NO_INLINE, _EXEC_APPEND)

#define SCRIPT_EXEC_STATE_INLINE(NAME, ME, STATE_T, STATE_N) \
    SCRIPT_COMPOSITE2(Executor, Exec, void, NAME, ME, STATE_T, STATE_N, _SCRIPT_INLINE, _EXEC_APPEND)


#define SCRIPT_EXEC_EX(NAME, ME, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Executor, Exec, void, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_INLINE, _EXEC_APPEND)

#define SCRIPT_EXEC_EX_INLINE(NAME, ME, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Executor, Exec, void, NAME, ME, DATA_T, DATA_N, _SCRIPT_INLINE, _EXEC_APPEND)

#define SCRIPT_EXEC_EX_STATE_INLINE(NAME, ME, DATA_T, DATA_N, STATE_T, STATE_N) \
    SCRIPT_COMPOSITE_EX2(Executor, Exec, void, NAME, ME, DATA_T, DATA_N, STATE_T, STATE_N, _SCRIPT_INLINE, _EXEC_APPEND)

#define SCRIPT_EXEC_IMPL(NAME) \
    SCRIPT_COMPOSITE_IMPL(Executor, Exec, NAME)

}
