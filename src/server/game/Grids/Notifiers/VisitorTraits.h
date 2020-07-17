#pragma once

#include "MethodReflection.h"

class WorldObject;
class Creature;
class Player;
class DynamicObject;
class GameObject;
class Corpse;
class Unit;

DEFINE_METHOD_REFLECTION_SIGNATURE(IsDone, Arguments<>::template Templates<>, const);
DEFINE_METHOD_REFLECTION_SIGNATURE(Visit, Templates<>,);
DEFINE_METHOD_REFLECTION_SIGNATURE(AcceptsObjectType, Arguments<>,);
DEFINE_METHOD_REFLECTION_SIGNATURE(CanVisit, Arguments<>, const);
DEFINE_METHOD_REFLECTION_SIGNATURE_NAME(FunctorVisit, operator(), Templates<>,)

namespace Trinity
{

    template<class Type>
    constexpr bool isPlayer = std::is_same_v<Type, Player>;
    template<class Type>
    constexpr bool isCreature = std::is_same_v<Type, Creature>;
    template<class Type>
    constexpr bool isDynObject = std::is_same_v<Type, DynamicObject>;
    template<class Type>
    constexpr bool isGameObject = std::is_same_v<Type, GameObject>;
    template<class Type>
    constexpr bool isCorpse = std::is_same_v<Type, Corpse>;
    template<class Type>
    constexpr bool isUnit = std::is_base_of_v<Unit, Type>;
    template<class Type>
    constexpr bool isWorldObject = std::is_base_of_v<WorldObject, Type>;

    // template

    template<class Type>
    struct TypeDispatcher
    {
        template<class Function>
        static constexpr void Dispatch(Function& func)
        {
            func.template Exec<Type>();
        }
    };

    template<>
    struct TypeDispatcher<Unit>
    {
        template<class Function>
        static constexpr void Dispatch(Function& func)
        {
            func.template Exec<Unit>();
            func.template Exec<Creature>();
            func.template Exec<Player>();
        }
    };

    template<>
    struct TypeDispatcher<WorldObject>
    {
        template<class Function>
        static constexpr void Dispatch(Function& func)
        {
            func.template Exec<WorldObject>();
            func.template Exec<Creature>();
            func.template Exec<Player>();
            func.template Exec<Corpse>();
            func.template Exec<GameObject>();
            func.template Exec<DynamicObject>();
        }
    };

    template<class Object, class Target>
    constexpr bool areTypesCompatible = std::is_base_of_v<Object, Target> || std::is_convertible_v<Target*, Object*>;

    template<class Visitor>
    struct VisitorTrait
    {
        // this method checks for two conditions to see if it accepts an object
        // #1 see if it has a valid Visit(Object&) overload
        // #2 see if it eigther hase no AcceptsObjectType<Object> overload, or if it return true
        // this needs to perform dispatching of top-level types (WorldObject, Unit) into their respective sub-types
        template<class ObjectType, class ArgumentType, template<class> class Reflector>
        static constexpr bool CheckExplicitAcceptsObjectType(void)
        {
            using HasVisitSignature = typename Reflector<Visitor>::template DispatchArguments<TypeDispatcher, ArgumentType>;
            using AcceptsObjectTypeSignature = typename Reflect::AcceptsObjectType_::Signature<Visitor>::template DispatchTemplates<TypeDispatcher, ObjectType>;

            // todo: check if those both are true for the same types?
            constexpr bool HasVisitFunction = HasVisitSignature::Exists();
            constexpr bool AcceptsObjectType = AcceptsObjectTypeSignature::template CallStaticIfExists<Reflect::ResultMode::OR>(true);

            return AcceptsObjectType && HasVisitFunction;
        }

    public:

        static bool IsDone(const Visitor& visitor)
        {
            using Visit = typename Reflect::IsDone_::Signature<Visitor>;
            
            return Visit::CallIfExists(visitor, false);
        }

        template<typename Object>
        static bool CanVisitObjectType(const Visitor& visitor)
        {
            using CanVisit = typename Reflect::CanVisit_::Signature<Visitor>::template Templates<Object>;

            return CanVisit::CallIfExists(visitor, true);
        }

        template<typename Object>
        static constexpr bool AcceptsObjectType()
        {
            return CheckExplicitAcceptsObjectType<Object, Object&, Reflect::Visit_::Signature>();
        }

        template<typename Object>
        static constexpr bool AcceptsFunctorObjectType()
        {
            return CheckExplicitAcceptsObjectType<Object, Object*, Reflect::FunctorVisit_::Signature>();
        }

    };

    template<class Visitor, class Object>
    constexpr bool VisitorAcceptsObjectType = VisitorTrait<Visitor>::template AcceptsObjectType<Object>();
    template<class Visitor, class Object>
    constexpr bool FunctorAcceptsObjectType = VisitorTrait<Visitor>::template AcceptsFunctorObjectType<Object>();

    template<class Visitor>
    constexpr bool VisitorIsValid = VisitorAcceptsObjectType<Visitor, WorldObject>;
    template<class Visitor>
    constexpr bool FunctorIsValid = FunctorAcceptsObjectType<Visitor, WorldObject>;

    template<class Visitor>
    void checkVisitorValidity()
    {
        static_assert(VisitorIsValid<Visitor>, "Invalid Visitor (does not accept any possible object type). Signature must be void Visit(ObjectType& object).");
    }

    template<class Visitor>
    void checkFunctorValidity()
    {
        static_assert(FunctorIsValid<Visitor>, "Invalid Functor (it does not accept any possible object type). Signature must be bool operator()(ObjectType* object).");
    }

    template<class Object, class Visitor>
    bool CanVisitObjectType(const Visitor& visitor)
    {
        return VisitorTrait<Visitor>::template CanVisitObjectType<Object>(visitor);
    }

    template<class Visitor>
    bool IsVisitorDone(const Visitor& visitor)
    {
        return VisitorTrait<Visitor>::IsDone(visitor);
    }

    // map type mask

    template<class Object>
    constexpr uint32 buildMask(void)
    {
        uint32 mask = 0;
        if constexpr(std::is_base_of_v<Object, Player>)
            mask |= GRID_MAP_TYPE_MASK_PLAYER;
        if constexpr(std::is_base_of_v<Object, Creature>)
            mask |= GRID_MAP_TYPE_MASK_CREATURE;
        if constexpr(std::is_base_of_v<Object, GameObject>)
            mask |= GRID_MAP_TYPE_MASK_GAMEOBJECT;
        if constexpr(std::is_base_of_v<Object, Corpse>)
            mask |= GRID_MAP_TYPE_MASK_CORPSE;
        if constexpr(std::is_base_of_v<Object, DynamicObject>)
            mask |= GRID_MAP_TYPE_MASK_DYNAMICOBJECT;

        return mask;
    }

    template<class Object>
    constexpr bool visitorSupportByMask(uint32 mask)
    {
        constexpr uint32 _mask = buildMask<Object>();

        static_assert(_mask != 0, "Invalid Object template specified for visitor.");

        return (mask & _mask);
    }

    template<class Object>
    struct ExtractContainerType
    {
        using type = Object;
    };

    template<template<class, typename...> class Container, class Object, typename... Args>
    struct ExtractContainerType<Container<Object, Args...>>
    {
        using type = std::remove_pointer_t<Object>;
    };

}