#pragma once

#include <type_traits>

#include <boost/optional.hpp>

namespace util::predicate
{
    
    const auto HasValue = [](const auto& optional) -> bool
    {
        return static_cast<bool>(optional);
    };

}
