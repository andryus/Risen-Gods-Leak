#pragma once
#include "ScriptGenerator.h"
#include "ScriptUndoable.h"

namespace script
{

    /*************************************
    * Action
    **************************************/

    template<class ActionT>
    struct Action : public Generator<ActionT>
    {
        using Generator<ActionT>::Generator;

        static constexpr bool DefaultReturn = true; 

        template<class Ctx>
        static bool ValidateResult(bool result, const Ctx& action, NoParam params)
        {
            return result && action.HadSuccess();
        }

        template<class Ctx>
        static bool ResultForFailure(NoParam params)
        {
            return false;
        }
    };

    /*************************************
    * Action
    **************************************/

    template<class ActionT>
    struct ActionReturn : 
        public Action<ActionT>
    {
        using Action<ActionT>::Action;

        template<class Ctx>
        static auto ValidateResult(typename Ctx::Return result, const Ctx& action, NoParam params)
        {
            return result;
        }

        template<class Ctx>
        static auto ResultForFailure(NoParam params)
        {
            return typename Ctx::Return{};
        }

        USE_POSTFIX_MANIPULATORS(ActionT, ActionReturn);
    };

#define _SCRIPT_ACTION_EXT public: bool HadSuccess() const { return _success; } private: void SetSuccess(bool state) { _success = state; } bool _success = true; public:

/******************************************
* COMPOSITE
********************************************/

#define SCRIPT_ACTION(NAME, ME) \
    SCRIPT_COMPOSITE(Action, Do, void, NAME, ME, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)
#define SCRIPT_ACTION_INLINE(NAME, ME) \
    SCRIPT_COMPOSITE(Action, Do, void, NAME, ME, _SCRIPT_INLINE, _SCRIPT_ACTION_EXT)

#define SCRIPT_ACTION1(NAME, ME, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE1(Action, Do, void, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)
#define SCRIPT_ACTION1_INLINE(NAME, ME, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE1(Action, Do, void, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_INLINE, _SCRIPT_ACTION_EXT)

// return values
#define SCRIPT_ACTION_RET(NAME, ME, RETURN) \
    SCRIPT_COMPOSITE(ActionReturn, Do, RETURN, NAME, ME, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)
#define SCRIPT_ACTION1_RET(NAME, ME, RETURN, DATA_T, DATA_N, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE1(ActionReturn, Do, RETURN, NAME, ME, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)

/******************************************
* EX
********************************************/

#define SCRIPT_ACTION_EX(NAME, ME, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Action, Do, void, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)
#define SCRIPT_ACTION_EX_INLINE(NAME, ME, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX(Action, Do, void, NAME, ME, DATA_T, DATA_N, _SCRIPT_INLINE, _SCRIPT_ACTION_EXT)

#define SCRIPT_ACTION_EX1(NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE_EX1(Action, Do, void, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)
#define SCRIPT_ACTION_EX1_INLINE(NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE_EX1(Action, Do, void, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_INLINE, _SCRIPT_ACTION_EXT)

// return values
#define SCRIPT_ACTION_EX_RET(NAME, ME, RETURN, DATA_T, DATA_N) \
    SCRIPT_COMPOSITE_EX1(ActionReturn, Do, RETURN, NAME, ME, DATA_T, DATA_N, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)
#define SCRIPT_ACTION_EX1_RET(NAME, ME, RETURN, DATA_T, DATA_N, PARAM_T, PARAM_N) \
    SCRIPT_COMPOSITE_EX1(ActionReturn, Do, RETURN, NAME, ME, DATA_T, DATA_N, PARAM_T, PARAM_N, _SCRIPT_NO_INLINE, _SCRIPT_ACTION_EXT)


/******************************************
* IMPL
********************************************/

#define SCRIPT_ACTION_IMPL(NAME) \
    SCRIPT_REGISTER_COMMAND(Do, NAME) \
    SCRIPT_COMPOSITE_IMPL(Action, Do, NAME)

#define SCRIPT_ACTION_RET_IMPL(NAME) \
    SCRIPT_REGISTER_COMMAND(Do, NAME) \
    SCRIPT_COMPOSITE_IMPL(ActionReturn, Do, NAME)

}
