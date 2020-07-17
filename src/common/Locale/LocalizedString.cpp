#include "LocalizedString.hpp"

namespace locale {

    const std::string& LocalizedString::In(Locale locale) const
    {
        size_t index = GetIndex(locale);

        if (strings[index].empty())
            index = GetIndex(DEFAULT_LOCALE);

        return strings[index];
    }

    void LocalizedString::Set(Locale locale, std::string string)
    {
        strings[GetIndex(locale)] = std::move(string);
    }

}
