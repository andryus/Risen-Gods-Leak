#pragma once
#include <string_view>
#include <unordered_map>
#include <memory>

namespace script
{

    struct ScriptNamePredicate
    {
        bool operator()(std::string_view str1, std::string_view str2) const
        {
            constexpr auto ignoreCase = [](char c1, char c2)
            {
                return tolower(c1) < tolower(c2);
            };

            return std::lexicographical_compare(str1.begin(), str1.end(), str2.begin(), str2.end(), ignoreCase);
        }
    };

    struct ScriptState;

    using StatePtr = std::unique_ptr<ScriptState>;
    using StateContainer = std::unordered_map<uint64, StatePtr>;

}
