/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITYCORE_COMMON_H
#define TRINITYCORE_COMMON_H

#include "Define.h"

#include <boost/dynamic_bitset.hpp>

#include <algorithm>
#include <array>
#include <exception>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <numeric>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <csignal>

#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/functional/hash.hpp>

#include <boost/functional/hash.hpp>

#include "Debugging/Errors.h"

#include "Threading/LockedQueue.h"

#if PLATFORM == PLATFORM_WINDOWS
#  include <ws2tcpip.h>

#  if defined(__INTEL_COMPILER)
#    if !defined(BOOST_ASIO_HAS_MOVE)
#      define BOOST_ASIO_HAS_MOVE
#    endif // !defined(BOOST_ASIO_HAS_MOVE)
#  endif // if defined(__INTEL_COMPILER)

#else
#  include <sys/types.h>
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#  include <netdb.h>
#endif

#if COMPILER == COMPILER_MICROSOFT

#include <float.h>

#define atoll _atoi64
#define llabs _abs64

#else

#define stricmp strcasecmp
#define strnicmp strncasecmp

#endif

inline float finiteAlways(float f) { return std::isfinite(f) ? f : 0.0f; }

inline unsigned long atoul(char const* str) { return strtoul(str, nullptr, 10); }
inline unsigned long long atoull(char const* str) { return strtoull(str, nullptr, 10); }

#define STRINGIZE(a) #a

enum TimeConstants
{
    SECOND   =  1,
    MINUTE   = 60 * SECOND,
    HOUR     = 60 * MINUTE,
    DAY      = 24 * HOUR,
    WEEK     =  7 * DAY,
    MONTH    = 30 * DAY,
    YEAR     = 12 * MONTH,

    IN_MILLISECONDS = 1000
};

using AccountId = uint32;

enum GMLevel : uint32
{
    GMLEVEL_PLAYER                   =   0,
    GMLEVEL_PREMIUM                  =  10,
    GMLEVEL_TEAMGOESDB               =  20,
    GMLEVEL_RG_TEAM                  =  30,
    GMLEVEL_TESTER                   =  40,
    GMLEVEL_DEVELOPER                =  50,
    GMLEVEL_VIDEOFX                  =  60,
    GMLEVEL_HP_GM_TRIAL              =  70,
    GMLEVEL_GM_TRIAL                 =  80,
    GMLEVEL_HP_GM_TRIAL_CHARTRANS    =  90,
    GMLEVEL_HP_GM                    = 100,
    GMLEVEL_GM                       = 110,
    GMLEVEL_BEREICHSLEITER           = 120,
    GMLEVEL_HP_GM_LEAD               = 130,
    GMLEVEL_GM_LEAD                  = 140,
    GMLEVEL_T_LEAD                   = 150,
    GMLEVEL_DEBUG                    = 160,
    GMLEVEL_ADMINISTRATOR            = 170,
    GMLEVEL_SERVER                   = 200,
};

enum AccountTypes : uint32
{
    SEC_PLAYER         = GMLEVEL_PLAYER,
    SEC_MODERATOR      = GMLEVEL_RG_TEAM,
    SEC_GAMEMASTER     = GMLEVEL_HP_GM_TRIAL,
    SEC_ADMINISTRATOR  = GMLEVEL_ADMINISTRATOR,
    SEC_CONSOLE        = GMLEVEL_SERVER                                  // must be always last in list, accounts must have less security level always also
};

enum LocaleConstant
{
    LOCALE_enUS = 0,
    LOCALE_koKR = 1,
    LOCALE_frFR = 2,
    LOCALE_deDE = 3,
    LOCALE_zhCN = 4,
    LOCALE_zhTW = 5,
    LOCALE_esES = 6,
    LOCALE_esMX = 7,
    LOCALE_ruRU = 8,

    TOTAL_LOCALES
};

#define DEFAULT_LOCALE LOCALE_enUS

#define MAX_LOCALES 8
#define MAX_ACCOUNT_TUTORIAL_VALUES 8

COMMON_API extern char const* localeNames[TOTAL_LOCALES];

COMMON_API LocaleConstant GetLocaleByName(const std::string& name);

typedef std::vector<std::string> StringVector;

// we always use stdlibc++ std::max/std::min, undefine some not C++ standard defines (Win API and some other platforms)
#ifdef max
#  undef max
#endif

#ifdef min
#  undef min
#endif

#ifndef M_PI
#  define M_PI          3.14159265358979323846
#endif

#ifndef M_PI_F
#  define M_PI_F        float(M_PI)
#endif

#define MAX_QUERY_LEN 32*1024

//! Optional helper class to wrap optional values within.
template <typename T>
using Optional = boost::optional<T>;

namespace Trinity
{
    //! std::make_unique implementation (TODO: remove this once C++14 is supported)
    template<typename T, typename ...Args>
    std::unique_ptr<T> make_unique(Args&& ...args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

//! Hash implementation for std::pair to allow using pairs in unordered_set or as key for unordered_map
//! Individual types used in pair must be hashable by boost::hash
namespace std
{
    template<class K, class V>
    struct hash<std::pair<K, V>>
    {
    public:
        size_t operator()(std::pair<K, V> const& key) const
        {
            return boost::hash_value(key);
        }
    };
}

#endif
