#pragma once
#include <type_traits>

#include "Define.h"
#include "util/meta/TypeChecks.hpp"

namespace script
{
    struct NoBase {};

    class ScriptLogger;
    class Scriptable;

    struct NoTarget
    {
        constexpr NoTarget& operator*() { return *this; }

        constexpr explicit operator bool() const { return true; }
    };

    template<class TargetT>
    constexpr bool HasTarget = !std::is_same_v<TargetT, NoTarget>;

    template<class Target>
    struct ScriptType : std::false_type {};

    template<>
    struct ScriptType<Scriptable> : std::true_type {};

    template<class Target>
    constexpr bool IsScriptObject = ScriptType<Target>::value;

    /*******************************************
    * Casting
    *******************************************/

    template<class Target>
    struct TargetBase { using type = NoBase; };

    template<class Target>
    using BaseOf = typename TargetBase<Target>::type;

    template<class Target>
    constexpr bool IsValidType = !std::is_same_v<NoBase, Target>;
    template<class Target>
    constexpr bool IsInvalidType = !IsValidType<Target>;
    template<class Target>
    constexpr bool HasBase = !IsInvalidType<BaseOf<Target>>;

    template<class Target, class Source, typename Enable = void>
    struct TargetCast {};

    namespace impl
    {
        template<class Target>
        auto* _calcBase()
        {
            if constexpr(!HasBase<Target> || std::is_same_v<BaseOf<Target>, script::Scriptable>)
                return (Target*)nullptr;
            else
                return _calcBase<BaseOf<Target>>();
        }

        template<class Target, class Type>
        constexpr bool _hasBase()
        {
            if constexpr(!HasBase<Type>)
                return false;
            else if constexpr(std::is_same_v<Target, Type>)
                return true;
            else
                return _hasBase<Target, BaseOf<Type>>();
        }

        template<class Type>
        constexpr uint8 getInheritanceLevel()
        {
            if constexpr (!HasBase<Type>)
                return 0;
            else
                return 1 + getInheritanceLevel<BaseOf<Type>>();
        }

        template<class Type, uint8 levels>
        auto* _getUpperBase()
        {
            if constexpr(levels == 0)
                return (Type*)nullptr;
            else
                return _getUpperBase<BaseOf<Type>, levels - 1>();
        }

        template<class Target, class Source>
        Target* upCast(Source& source)
        {
            return reinterpret_cast<Target*>(&source);
        }

        template<class Target, class Source>
        Target* downCast(Source& source)
        {
            if constexpr(std::is_same_v<Target, Source>)
                return &source;
            else
            {
                using Base = BaseOf<Target>;

                static_assert(IsValidType<Base>, "Internal error in the Script Type-Cast system.");

                if (Base* base = downCast<Base>(source))
                    return TargetCast<Target, Base>()(*base);
                else
                    return nullptr;
            }
        }
    }

    template<class Target, uint8 levels>
    using UpperBase = std::remove_pointer_t<decltype(impl::_getUpperBase<Target, levels>())>;

    template<class Target>
    using TopBaseOf = std::remove_pointer_t<decltype(impl::_calcBase<Target>())>;

    template<class Base, class Type>
    constexpr bool IsBaseOf = impl::_hasBase<Base, Type>();

    template<class Type>
    constexpr bool IsScriptable = std::is_same_v<Type, Scriptable> || std::is_same_v<BaseOf<TopBaseOf<Type>>, Scriptable>;

    template<class Target, class Source>
    constexpr int8 CalcCastDir()
    {
        constexpr uint8 targetLevel = impl::getInheritanceLevel<Target>();
        constexpr uint8 sourceLevel = impl::getInheritanceLevel<Source>();

        if constexpr(targetLevel <= sourceLevel)
        {
            constexpr uint8 levelDiff = sourceLevel - targetLevel;

            using Base = UpperBase<Source, levelDiff>;

            return std::is_same_v<Base, Target> ? 1 : 0;
        }
        else
        {
            constexpr uint8 levelDiff = targetLevel - sourceLevel;

            using Base = UpperBase<Target, levelDiff>;

            return std::is_same_v<Base, Source> ? -1 : 0;
        }
    }

    /*******************************
    * Module
    *********************************/

    template<class Module>
    struct ModuleOwner { using type = NoBase; };
    template<class Component>
    using ModuleOwnerType = typename ModuleOwner<Component>::type;

    template<class Module>
    constexpr bool IsModule = IsValidType<ModuleOwnerType<Module>>;

    template<class Component>
    struct ModuleAccess {};

    template<class Target, class Source>
    Target* castSourceTo(Source& source);

    template<class Target, class Source>
    Target* moduleAccess(Source& source)
    {
        if constexpr(IsModule<Target>)
        {
            if (auto* owner = castSourceTo<ModuleOwnerType<Target>>(source))
                return ModuleAccess<Target>()(*owner);
        }
        else if constexpr (IsModule<Source>)
        {
            if (auto* owner = ModuleAccess<Source>()(source))
                return castSourceTo<Target>(*owner);
        }
        else 
            static_assert(util::meta::False<Target>, "Neigther Target nor Source is a Module.");

        return nullptr;
    }

    /*******************************
    * Component
    *********************************/

    template<class Owner, class Component>
    struct ComponentAccess {};

    template<class Owner, class Component>
    constexpr bool _isComponentOf = std::is_invocable_v<ComponentAccess<Owner, Component>, Owner&>;

    template<class Owner, class Component>
    constexpr auto* queryTypeWithComponent()
    {
        if constexpr(IsInvalidType<Owner> || IsInvalidType<Component>)
            return (NoBase*)nullptr;
        else if constexpr(_isComponentOf<Owner, Component>)
            return (Owner*)nullptr;
        else 
        {
            // resolve base-tree of Owner
            constexpr auto* owner = queryTypeWithComponent<BaseOf<Owner>, Component>();

            // otherwise, get component base and try again
            if constexpr(IsValidType<std::decay_t<decltype(*owner)>>)
                return owner;
            else
                return queryTypeWithComponent<Owner, BaseOf<Component>>();
        }
    }

    template<class Owner, class Component>
    using BaseOwnerOf = std::remove_pointer_t<decltype(queryTypeWithComponent<Owner, Component>())>;

    template<class Owner, class Component>
    constexpr bool IsComponentOf = IsValidType<BaseOwnerOf<Owner, Component>>;

    template<class Component, class Owner>
    Component* componentAccess(Owner& owner)
    {
        using ComponentBase = TopBaseOf<Component>;
        using OwnerBase = BaseOwnerOf<Owner, Component>;

        static_assert(IsValidType<OwnerBase>, "Component is not a part of this object.");

        if (OwnerBase* baseOwner = castSourceTo<OwnerBase>(owner))
            if (ComponentBase* component = ComponentAccess<OwnerBase, ComponentBase>()(*baseOwner))
                return castSourceTo<Component>(*component);

        return nullptr;
    }

    /*******************************************
    * Cast
    *******************************************/

    template<class Target, class Source>
    constexpr bool CanCastTo()
    {
        // "conversion" to NoTarget is always possible
        if constexpr (std::is_same_v<Target, Source>)
            return true;
        else if constexpr (!HasTarget<Target>)
            return true;
        else if constexpr(IsModule<Target>)
            return CanCastTo<ModuleOwnerType<Target>, Source>();
        else if constexpr(IsModule<Source>)
            return CanCastTo<Target, ModuleOwnerType<Source>>();
        else if constexpr(IsComponentOf<Source, Target>)
            return true;
        else if constexpr (IsScriptObject<Target> && IsScriptObject<Source>)
            return CalcCastDir<Target, Source>() != 0;
        else
            return false;
    }

    template<class Target, class Source>
    constexpr bool CanAccessModule = (IsModule<Target> && CanCastTo<ModuleOwnerType<Target>, Source>()) || (IsModule<Source> && CanCastTo<Target, ModuleOwnerType<Source>>());

    template<class Target, class Source>
    Target* castSourceTo(Source& source)
    {
        using RealSource = std::decay_t<Source>;

        if constexpr(std::is_same_v<Target, RealSource>)
            return &source;
        else
        {
            constexpr int8 castType = CalcCastDir<Target, RealSource>();

            if constexpr(castType == 1) // upcast
                return impl::upCast<Target>(source);
            else if constexpr(castType == -1) // downcast
                return impl::downCast<Target>(source);
            else if constexpr(CanAccessModule<Target, RealSource>)
                return moduleAccess<Target>(source);
            else if constexpr(IsComponentOf<RealSource, Target>)
                return componentAccess<Target>(source);
            else
                static_assert(util::meta::False<Target>, "No cast available for Source => Target.");
        }
    }

    template<class Target, class Source>
    Target* castSourceTo(Source* source)
    {
        if (source)
            return castSourceTo<Target>(*source);
        else
            return nullptr;
    }

    template<class Target>
    ScriptLogger* getLoggerFromTarget(Target* target)
    {
        if (target)
            if (auto base = castSourceTo<Scriptable>(*target))
                return requestCurrentLogger(base->GetScriptLogFile());

        return nullptr;
    }


    /*******************************************
    * Storage
    *******************************************/

    template<class Target>
    struct StoredTarget
    {
        static_assert(util::meta::False<Target>, "Cannot store target of this Type.");
    };

    /**********************************************
    * Binding
    ***********************************************/

#define SCRIPT_STORED_TARGET(Me, HANDLE)                        \
    template<>                                                  \
    struct script::StoredTarget<Me>                             \
    {                                                           \
        using Handle = HANDLE;                                  \
        using Stored = Me;                                      \
                                                                \
        using HandleStored = typename Handle::Stored;           \
                                                                \
        StoredTarget() {}                                       \
        StoredTarget(Me& me) :                                  \
            handle(castSourceTo<HandleStored>(me)) {}           \
        StoredTarget(Me* me) :                                  \
            handle(castSourceTo<HandleStored>(me)) {}           \
                                                                \
        Me* Load() const                                        \
        {                                                       \
            return castSourceTo<Me>(handle.Load());             \
        }                                                       \
                                                                \
        std::string Print() const                               \
           { return handle.Print(); }                           \
                                                                \
    private:                                                    \
                                                                \
        HANDLE handle;                                          \
    };                                                          \
                                                                       \
    template<>                                                         \
    struct script::FromString<Me>                                      \
    {                                                                  \
        Me* operator()                                                 \
            (std::string_view string, script::Scriptable& ctx) const;  \
    };                                                                 \

}
