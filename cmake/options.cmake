# Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# binaries
option(BUILD_LOGON "Build logonserver" 1)
option(BUILD_WORLD "Build worldserver" 1)
option(BUILD_TOOLS "Build dbc/map/vmap/mmap extrators/generators" 0)
option(BUILD_TESTS "Build unit-tests" 0)

set(BUILD_SCRIPTS "rg" CACHE STRING "Build core with scripts included.")
set_property(CACHE BUILD_SCRIPTS PROPERTY STRINGS none trinity rg)

set(BUILD_SCRIPTS_NEW "default" CACHE STRING "Use Scripts from the new Script System.")
set_property(CACHE BUILD_SCRIPTS_NEW PROPERTY STRINGS default none reloadable)


# PCH usage
option(USE_SCRIPTPCH "Use precompiled headers when compiling scripts" 1)
option(USE_COREPCH "Use precompiled headers when compiling servers" 1)
option(USE_TESTPCH "Use precompiled headers when compiling tests" 1)


# warnings
option(WITH_WARNINGS "Show all warnings during compile" 0)
option(WITH_WARNINGS_PENDANTIC "Show pendantic warnings during compile" 0)


# debug & optimization
option(WITH_LINK_TIME_OPTIMIZATION "Enable link time optimization for gcc/clang" 0)
option(WITH_COREDEBUG "Include additional debug-code in core" 0)
option(WITH_FULLDEBUG "Compile sources with debug level 3 (gcc-only)" 0)

option(THREADSAFETY_DEBUG_ENABLE "Enable debugging of threadsafety" 1)
option(THREADSAFETY_DEBUG_ASSERT "Enable assert on threadsafety violations" 0)
option(SANITIZE_ASSERT "Enable assert on sanizize asserts" 0)


# linking
option(USE_STATIC_DEPENDENCIES "Link dependend libraries statically" 0)


# IDEs & visual
option(WITHOUT_GIT      "Disable the GIT testing routines"                            0)
set(WITH_SOURCE_TREE    "hierarchical-folders" CACHE STRING "Build the source tree for IDE's.")
set_property(CACHE WITH_SOURCE_TREE PROPERTY STRINGS no flat hierarchical hierarchical-folders)


# external libraries
option(WITH_PROMETHEUS_MONITORING "Include prometheus monitoring support" 0)
option(CPR_AND_CURL  "Enable build of cpr/curl submodule. Only needed if you need the premium code npc" 0)
option(WITH_SOCKET_ACTIVATION  "Build with socket activation, only supported on linux" 0)
option(WITH_GRAYLOG_SUPPORT "Include graylog appender" 0)

option(WITH_RESTRICTED_LOGON_ACCESS "Restrict access paths to logon database" 0)
mark_as_advanced(WITH_RESTRICTED_LOGON_ACCESS)

# legacy
option(WITH_MESHEXTRACTOR "Build meshextractor (alpha)" 0)


# backward compat
if (SERVERS)
  set(BUILD_LOGON 1)
  set(BUILD_WORLD 1)
endif()
