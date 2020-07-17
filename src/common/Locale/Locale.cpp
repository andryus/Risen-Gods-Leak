#include "Locale.hpp"

namespace locale {

    Locale GetLocale(uint8 index)
    {
        switch (index)
        {
            case 0: return Locale::enUS;
            case 1: return Locale::koKR;
            case 2: return Locale::frFR;
            case 3: return Locale::deDE;
            case 4: return Locale::zhCN;
            case 5: return Locale::zhTW;
            case 6: return Locale::esES;
            case 7: return Locale::esMX;
            case 8: return Locale::ruRU;
            default: return DEFAULT_LOCALE;
        }
    };

}
