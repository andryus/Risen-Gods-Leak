/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
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

#ifndef __GITREVISION_H__
#define __GITREVISION_H__

#include <string>
#include "API.h"

namespace GitRevision
{
    COMMON_API char const* GetHash();
    COMMON_API char const* GetDate();
    COMMON_API char const* GetBranch();
    COMMON_API char const* GetCMakeCommand();
    COMMON_API char const* GetBuildDirectory();
    COMMON_API char const* GetSourceDirectory();
    COMMON_API char const* GetMySQLExecutable();
    COMMON_API char const* GetFullDatabase();
    COMMON_API char const* GetFullVersion(bool wowChat = false);
    COMMON_API char const* GetCompanyNameStr();
    COMMON_API char const* GetLegalCopyrightStr();
    COMMON_API char const* GetFileVersionStr();
    COMMON_API char const* GetProductVersionStr();
    COMMON_API char const* GetBuildDirective();
}

#endif
