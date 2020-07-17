#pragma once

#include <boost/optional.hpp>

namespace util::transform
{

    const auto UnwrapReference = [](const auto& ref) -> auto&
    {
        return ref.get();
    };

    
    const auto UnwrapOptional = [](const auto& optional) -> auto&
    {
        return optional.value();
    };

}
