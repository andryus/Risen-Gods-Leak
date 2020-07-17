#pragma once

#include <type_traits>

#include "Define.h"

namespace locale {

    enum class Locale : uint8
    {
        enUS = 0,
        koKR = 1,
        frFR = 2,
        deDE = 3,
        zhCN = 4,
        zhTW = 5,
        esES = 6,
        esMX = 7,
        ruRU = 8,
    };
    const uint8 NUM_LOCALES = 9;

    const Locale DEFAULT_LOCALE = Locale::enUS;

    COMMON_API Locale GetLocale(uint8 index);

    constexpr auto GetIndex(Locale locale)
    {
        return static_cast<std::underlying_type_t<decltype(locale)>>(locale);
    };

}
