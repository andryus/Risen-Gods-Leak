/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

#ifndef TRINITYCORE_STRING_FORMAT_H
#define TRINITYCORE_STRING_FORMAT_H

#include "fmt/format.h"

namespace Trinity
{
    // helper function for accepting enum-classes
    template<typename Arg>
    inline decltype(auto) convertFormatArg(Arg&& arg)
    {
        if constexpr(std::is_enum_v<Arg>)
        {
            using Type = std::underlying_type_t<Arg>;

            return (Type)arg;
        }
        else
            return std::forward<Arg>(arg);
    }

    template<>
    inline decltype(auto) convertFormatArg<std::string_view>(std::string_view&& arg)
    {
        return arg.data();
    }

    /// Default TC string format function.
    template<typename Format, typename... Args>
    inline std::string StringFormat(Format&& fmt, Args&&... args)
    {
        return fmt::sprintf(std::forward<Format>(fmt), convertFormatArg<Args>(std::forward<Args>(args))...);
    }

    /// Returns true if the given char pointer is null.
    inline bool IsFormatEmptyOrNull(const char* fmt)
    {
        return fmt == nullptr;
    }

    /// Returns true if the given std::string is empty.
    inline bool IsFormatEmptyOrNull(std::string const& fmt)
    {
        return fmt.empty();
    }
}

#endif
