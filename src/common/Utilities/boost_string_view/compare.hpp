#pragma once

#include <algorithm>
#include <boost/algorithm/string/compare.hpp>
#include <cctype>

namespace Trinity {
    namespace string_view {

        template<typename Transform>
        struct hash
        {
            template<typename T>
            std::size_t operator()(const T& key, Transform trans = Transform()) const
            {
                std::string str{ key };
                std::transform(str.begin(), str.end(), str.begin(), trans);
                return std::hash<std::string>()(str);
            }
        };

        template<typename Compare>
        struct is_less
        {
            template<typename T1, typename T2>
            bool operator()(const T1& lhs, const T2& rhs, Compare comp = Compare()) const
            {
                return std::lexicographical_compare(
                    lhs.begin(), lhs.end(),
                    rhs.begin(), rhs.end(),
                    comp
                );
            }
        };

        template<typename Pred>
        struct is_equal
        {
            template<typename T1, typename T2>
            bool operator()(const T1& lhs, const T2& rhs, Pred pred = Pred()) const
            {
                return lhs.size() == rhs.size() && std::equal(
                    lhs.begin(), lhs.end(),
                    rhs.begin(),
                    pred
                );
            }
        };

        struct toupper
        {
            template<typename CharT>
            CharT operator()(CharT c, std::locale loc = std::locale())
            {
                return std::toupper(c, loc);
            }
        };

        using ihash = hash<toupper>;
        using is_iless = is_less<boost::is_iless>;
        using is_iequal = is_equal<boost::is_iequal>;

    }
}
