#pragma once
#include <tuple>
#include "ScriptBindMacros.h"
#include "ScriptTarget.h"
#include "ScriptUndoHandler.h"
#include "ScriptGenerator.h"
#include "ScriptLogger.h"
#include "ScriptHook.h"

#include "Random.h"

namespace script
{
    /******************************
    * ValueComposite
    ******************************/

    template<class Value>
    struct ValueComposite
    {
        ValueComposite(Value value) :
            value(value)
        {
        }

        decltype(auto) Call(NoParam)
        {
            return value;
        }

    private:

        Value value;
    };

    template<class Element, size_t Size>
    auto makeArray(ArrayRef<Element, Size> array)
    {
        Array<Element, Size> stored;
        for (uint32 i = 0; i < Size; i++)
            stored[i] = array[i];

        return ValueComposite(stored);
    }

    template<class Element, class Value1, class Value2, class... Values>
    auto makeArray(Value1 value1, Value2 value2, Values&&... values)
    {
        const Element array[] = { Element(value1), Element(value2), Element(values)... };

        return makeArray(array);
    }

    /******************************
    * OneOfComposite
    ******************************/

    template<class Getter>
    struct OneOfComposite
    {
        OneOfComposite(Getter getter) :
            getter(getter)
        {
        }

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            auto array = getter.Call(params);
            const uint32 index = urand(0, array.size() - 1);

            return array[index];
        }

        USE_GENERATOR(Getter);

    private:

        Getter getter;
    };

    template<class Element, size_t Size>
    auto makeOneOf(ArrayRef<Element, Size> array)
    {
        return OneOfComposite(makeArray(array));
    }

    template<class Element, class Value1, class Value2, class... Values>
    auto makeOneOf(Value1 value1, Value2 value2, Values&&... values)
    {
        return OneOfComposite(makeArray<Element>(value1, value2, values...));
    }

    /****************************************
    * PreparedBinding
    *****************************************/

    template<class Functor, class Value>
    struct PreparedBinding
    {
        using Param = typename Functor::Param;
        using Me = typename Functor::Me;

        PreparedBinding(Functor functor, Value value) :
            functor(functor), value(value) {}

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            auto storedParam = TransformElement(value.Call(params));

            const auto callWithParam = [&functor = functor, params](auto&& element)
            {
                const auto callParams = makeParams(params.target, TransformElement(element), params);

                return functor.Call(callParams);
            };

            return applyForReturnValue<typename Functor::Param>(storedParam, callWithParam);
        }

        template<class HookT>
        auto Hook(HookT hook) const
        {
            return script::makeHookedComposite(*this, hook);
        }

        USE_GENERATOR(Functor);

        bool canUndo = true;

    private:

        template<class Element>
        static auto TransformElement(Element&& element)
        {
            if constexpr(std::is_reference_v<Param> && !std::is_pointer_v<std::decay_t<Element>>)
                return &element;
            else
                return element;
        }

        Functor functor;
        Value value;
    };

    template<class Functor, class Value>
    auto makePreparedBinding(Functor functor, Value value)
    {
        return Functor::MakeGenerator(PreparedBinding(functor, value));
    }


    /****************************************
    * DynamicBinding
    *****************************************/

    template<class Functor, class Value>
    struct DynamicBinding
    {
        using Param = typename Functor::Param;
        using Me = typename Functor::Me;
        using Return = typename Functor::Return;

        DynamicBinding(Value value) :
            value(value) {}

        template<class Context>
        decltype(auto) Call(Params<Context> params)
        {
            const auto newParam = value.Call(params);

            const auto createAndCall = [params](auto element)
            {
                Functor composite(element);

                return composite.Call(params);
            };

            return applyForReturnValue<typename Functor::Data>(newParam, createAndCall);
        }

        USE_GENERATOR(Functor);

    private:

        Value value;
    };

    template<class Functor, class Value>
    auto makeDynamicBinding(Value value)
    {
        return Functor::MakeGenerator(DynamicBinding<Functor, Value>(value));
    }

    /****************************************
    * BindingMe
    *****************************************/

    template<class Me>
    struct BindingMe
    {
    public:

        template<class Target>
        static Me* EvalMe(Target* target)
        { 
            if (target)
            {
                decltype(auto) result = castSourceTo<Me>(*target);

                return result;
            }

            return nullptr;
        }

    };

    using Unbound = NoTarget;

    template<>
    struct BindingMe<Unbound>
    {
        template<class Target>
        static Unbound EvalMe(Target* target)
        {
            return {};
        }
    };

    /****************************************
    * BindingParam
    *****************************************/

    template<class Param, class Target>
    auto convertParam(Target&& target)
    {
        if constexpr(std::is_pointer_v<Param> || std::is_reference_v<Param>)
            return script::castSourceTo<std::remove_pointer_t<std::decay_t<Param>>>(target);
        else
            return target;
    }

    template<class Param, class Temp>
    bool checkParam(Temp&& temp)
    {
        if constexpr(std::is_reference_v<Param> && std::is_pointer_v<std::decay_t<Temp>>)
            return temp != nullptr;
        else
            return true;
    }

    template<class Param, class Temp>
    decltype(auto) convertParamBack(Temp&& temp)
    {
        if constexpr (std::is_reference_v<Param> && std::is_pointer_v<std::decay_t<Temp>>)
            return *temp;
        else
            return temp;
    }


    /****************************************
    * BindingPrint
    *****************************************/

    template<template<class> class Generator>
    struct GeneratorPrint
    {
        template<class Type>
        auto operator()(std::string_view in, const Type& type) { return in; }
    };

    template<class Type>
    void printParam(std::string_view name, const Type& value, ScriptLogger& logger)
    {
        const auto param = print(value);
        logger.AddParam(name, param);
    }

    template<class Binding, class Me, class Param>
    void printBinding(Binding& binding, Me&& me, Param&& param, script::ScriptLogger* logger)
    {
        using Ctx = typename Binding::CtxT;

        if (logger)
        {
            binding.Print(*logger);

            if constexpr (HasData<Binding>)
            {
                const auto data = print(binding.data);
                logger->AddData(data);
            }

            if constexpr (!std::is_same_v<std::decay_t<Me>, Unbound>)
                printParam("Me", me, *logger);
            if constexpr (!std::is_same_v<std::decay_t<Param>, script::NoParam>)
                printParam(Ctx::ParamName, param, *logger);

            if constexpr (HasState<Binding>)
                printParam(Ctx::StateName, binding.state, *logger);
        }
    }

    template<template<class> class Generator, class Composite>
    void printComposite(std::string_view nameSpace, std::string_view name, const Composite& composite, ScriptLogger& logger)
    {
        const auto prepend =
            script::GeneratorPrint<Generator>()(nameSpace, composite);

        logger.AddComposite(prepend, name);
    }                               

    /****************************************
    * BindingCall
    *****************************************/

    template<class State, class Binding>
    struct RestoreState
    {
        RestoreState(State oldState, Binding& binding) :
            oldState(oldState), binding(binding) { }

        bool Call(NoParam)
        {
            binding.state = oldState;

            return true;
        }

        bool canUndo = false;
    private:

        State oldState;
        Binding& binding; 
        // @todo: storing by ref unsafe. With regular undo-order, it shouldn't be a problem
        // but if we add more complex logic, needs to be fixed
    };

    template<class Param, class Me, class Composite, class Target>
    void addUndoableTo(Me& me, Composite&& composite, Target* target, Param param);

    template<class Binding, class Ctx, template<class> class GeneratorT, class Context>
    decltype(auto) doBindingCall(Binding& binding, script::Params<Context> params)
    {
        using Me = typename Ctx::Me;
        using Param = typename Ctx::Param;
        using Generator = GeneratorT<Binding>;

        auto me = BindingMe<Me>::EvalMe(params.target);
        decltype(auto) param = convertParam<Param>(params.param);

        printBinding(binding, me, param, params.logger);

        const bool isParamValid = checkParam<Param>(param);
        if (me && isParamValid)
        {
            decltype(auto) localParam = convertParamBack<Param>(param);
            Ctx ctx(*me, localParam, binding, params.logger);
            ctx._canUndo = binding.canUndo;

            const auto block = ScriptLogger::StoreLogger(params.logger);

            params.hook(ctx);

            const auto oldState = binding.state;
            uint64 stateId;
            if constexpr(HasState<Ctx>)
                stateId = UndoHandler::GetStateId(*me);
            else
                stateId = -1;

            decltype(auto) result = [](auto& ctx)
            {
                if constexpr(std::is_same_v<decltype(std::declval<Ctx>()()), void>)
                {
                    ctx();

                    return Generator::DefaultReturn;
                }
                else
                    return ctx();
            }(ctx);

            decltype(auto) output = Generator::ValidateResult(result, ctx, params);

            if constexpr(HasState<Ctx>)
                if (oldState != binding.state)
                    addUndoableTo(stateId, *me, RestoreState(oldState, binding), me, NoParam());

            return output;
        }
        else
        {
            std::string error = "Failed to call ";
            error += Binding::ModuleName;
            error += "::";
            error += Binding::CompositeName;
            error += '.';

            if (!me)
            {
                error += " Invalid #Me ";
                if (!params.target)
                    error += "(was NULL) ";
                else
                {
                    error += "(Failed to convert from ";
                    error += print(params.target);
                    error += ')';
                }

                error += ".";
            }

            if (!isParamValid)
            {
                error += " Invalid param #";
                error += Ctx::ParamName;
                error += "(is ";
                error += print(params.param);
                error += ").";
            }


            reportLoggerFailure(params.logger, binding.canUndo, error);

            return Generator::template ResultForFailure<Ctx>(params);
        }
    }

    template<class Binding, template<class> class GeneratorT>
    struct BindingCall
    {
        template<class Context>
        decltype(auto) Call(script::Params<Context> params)
        {
            Binding& binding = *static_cast<Binding*>(this);

            return doBindingCall<Binding, typename Binding::CtxT, GeneratorT>(binding, params);
        }
    };

}
