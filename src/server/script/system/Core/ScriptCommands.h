#pragma once
#include <string>
#include <string_view>
#include <map>
#include <optional>

#include "Define.h"
#include "ScriptUtil.h"
#include "Utilities/Span.h"

namespace script
{
    class Scriptable;

    class SCRIPT_API CommandRegistry
    {
    public:
        using ArgList = span<std::string_view>;
    private:
        struct SCRIPT_API StoredCommand
        {
            StoredCommand(std::string_view command);

            ~StoredCommand();

        private:

            std::string_view command;
        };

        using CallResult = std::optional<std::string>;

        using CommandFunction = CallResult(*)(Scriptable& script, ArgList data);
        using CommandMap = std::map<std::string_view, CommandFunction, ScriptNamePredicate>;
    public:

        StoredCommand Store(std::string_view command, CommandFunction function);

        static std::string FormatName(std::string_view module, std::string_view command);

        CallResult Call(std::string_view command, Scriptable& script, ArgList args);
        bool HasCommand(std::string_view command) const;

        std::string_view GetName(std::string_view command) const;

        std::vector<std::string_view> GetList(std::string_view command) const;

    private:

        void Remove(std::string_view command);

        CommandMap commands;
    };

#ifdef SCRIPT_DYNAMIC_RELOAD
    SCRIPT_API extern CommandRegistry _commandRegistry;
#endif
    
    SCRIPT_API CommandRegistry& commandRegistry();

    template<class Me, class Base>
    Me* castSourceTo(Base& base);
    template<class Data>
    auto print(Data&& data);

    struct NoParam;
    template<class ContextT>
    constexpr bool _HasParam = !std::is_same_v<std::decay_t<typename ContextT::Param>, NoParam>;

    template<class T>
    auto fromString(std::string_view string, Scriptable& ctx);

    template<class Ctx, class Binding, class HookT>
    auto callCmd(Binding binding, Scriptable& base, HookT hook, CommandRegistry::ArgList args)
    {
        using Me = typename Ctx::Me;

        Me* me = castSourceTo<Me>(base);

        if constexpr(_HasParam<Ctx>)
        {
            if (!args.empty())
            {
                if constexpr(std::is_reference_v<typename Ctx::Param>)
                {
                    if (auto param = fromString<std::decay_t<typename Ctx::Param>>(args.front(), base))
                        return me <<= binding.Bind(*param).Hook(hook);
                }
                else
                {
                    auto param = fromString<typename Ctx::Param>(args.front(), base);

                    return me <<= binding.Bind(param).Hook(hook);
                }
            }

            return decltype(me <<= binding.Bind(std::declval<typename Ctx::Param>()))();
        }
        else
            return me <<= binding.Hook(hook);
    }

    template<class Ctx, class Binding>
    std::optional<std::string> evalCmd(Binding binding, Scriptable& base, CommandRegistry::ArgList args)
    {
        bool success = false;
        const auto hook = [&success](const auto& binding) mutable { success = true; };

        auto result = callCmd<Ctx>(binding, base, hook, args);

        if (success)
            return std::make_optional(std::string(print(result)));
        else
            return std::nullopt;
    }

    template<class Ctx, class Binding, class Create>
    std::optional<std::string> evalCmdEx(Scriptable& base, CommandRegistry::ArgList args)
    {
        if (args.empty())
            return std::nullopt;                                              
        const auto result = fromString<typename Ctx::Data>(args.front(), base);
        args.pop_front();

        return evalCmd<Ctx>(Create(Binding(result)), base, args);
    }


#define _SCRIPT_STR(STRING) #STRING

#define SCRIPT_REGISTER_COMMAND(NAMESPACE, NAME)                                            \
    namespace NAMESPACE::Bind::Cmd                                                          \
    {                                                                                       \
        template<class CtxT>                                                                \
        std::optional<std::string> NAME(                                                    \
            script::Scriptable& base, script::CommandRegistry::ArgList args)                \
        {                                                                                   \
            using Binding = ::NAMESPACE::Bind::NAME;                                        \
                                                                                            \
            if constexpr(!script::HasData<CtxT>)                                            \
                return script::evalCmd<CtxT>(Create::NAME(Binding()), base, args);          \
            else                                                                            \
                return script::evalCmdEx<CtxT, Binding, Create::NAME>(base, args);          \
        }                                                                                   \
    }                                                                                       \
                                                                                            \
    namespace NAMESPACE::Cmd                                                                \
    {                                                                                       \
         auto NAME =                                                                        \
            script::commandRegistry().Store(_SCRIPT_STR(NAMESPACE) "." _SCRIPT_STR(NAME),   \
                Bind::Cmd::NAME<Bind::NAME>);                                               \
    }                                                                                       \

#define SCRIPT_FROM_STR(TYPE)                                                        \
    template<>                                                                       \
    struct script::FromString<TYPE>                                                  \
    {                                                                                \
        TYPE operator()(std::string_view string, script::Scriptable& ctx) const;     \
    };                                                                               \

#define SCRIPT_FROM_STR_IMPL(TYPE) \
    TYPE script::FromString<TYPE>::operator()(std::string_view string, script::Scriptable& ctx) const

#define SCRIPT_FROM_STR_BY(TYPE, BASE)                                   \
    SCRIPT_FROM_STR(TYPE)                                                \
    inline SCRIPT_FROM_STR_IMPL(TYPE)                                    \
    {                                                                    \
        return static_cast<TYPE>(script::fromString<BASE>(string, ctx)); \
    }                                                                    \
                                                                         \

}
