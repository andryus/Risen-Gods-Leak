#pragma once

#include <tuple>

namespace util
{
    //############################
    //# TypeList

    template<typename ...Ts>
    using TypeList = std::tuple<Ts...>;


    //############################
    //# UnpackVariadicTypelist

    // tag for the fixed parameters
    template<typename ...Ts>
    struct Fixed;

    // tag for the variadic parameters
    template<typename ...Ts>
    struct Variadic;

    namespace detail
    {

        template<template<typename ...> class TemplateT, typename FixedTs, typename VariadicTs>
        struct UnpackVariadicTypelist {};

        template<template<typename ...> class TemplateT, typename ...FixedTs, typename ...VariadicTs>
        struct UnpackVariadicTypelist<TemplateT, Fixed<FixedTs...>, Variadic<VariadicTs...>>
        {
            using type = TemplateT<FixedTs..., VariadicTs...>;
        };

        template<template<typename ...> class TemplateT, typename ...FixedTs, typename ...VariadicTs>
        struct UnpackVariadicTypelist<TemplateT, Fixed<FixedTs...>, Variadic<TypeList<VariadicTs...>>>
        {
            using type = TemplateT<FixedTs..., VariadicTs...>;
        };

    }

    template<template<typename ...> class TemplateT, typename FixedTs, typename VariadicTs>
    using UnpackVariadicTypelist = typename detail::UnpackVariadicTypelist<TemplateT, FixedTs, VariadicTs>::type;


    //############################
    //# WrapTypes

    namespace detail
    {
        template<template<typename ...> class WrapperT, typename TypelistT>
        struct WrapTypes {};

        template<template<typename ...> class WrapperT, typename ...Ts>
        struct WrapTypes<WrapperT, TypeList<Ts...>>
        {
            using type = TypeList<WrapperT<Ts>...>;
        };
    }

    template<template<typename ...> class WrapperT, typename TypelistT>
    using WrapTypes = typename detail::WrapTypes<WrapperT, TypelistT>::type;

}
