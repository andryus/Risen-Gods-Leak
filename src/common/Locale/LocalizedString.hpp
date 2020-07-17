#pragma once

#include <array>
#include <boost/optional.hpp>
#include <string>

#include "Locale.hpp"

namespace locale {

    class COMMON_API LocalizedString
    {
    private:
        std::array<std::string, NUM_LOCALES> strings;

    public:
        const std::string& In(Locale locale) const;
        void Set(Locale locale, std::string string);
    };

}
