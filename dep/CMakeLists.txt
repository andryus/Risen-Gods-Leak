# Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  string(REGEX REPLACE "/W[0-4] " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  string(REGEX REPLACE "/W[0-4] " "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  add_definitions(/W0)
else()
  add_definitions(-w)
endif()

add_subdirectory(threads)

if(BUILD_LOGON OR BUILD_WORLD OR BUILD_TOOLS)
  add_subdirectory(boost)
  add_subdirectory(zlib)
  add_subdirectory(g3dlite)
  add_subdirectory(recastnavigation)
  add_subdirectory(fmt)
  add_subdirectory(SFMT)
  add_subdirectory(utf8cpp)
  add_subdirectory(valgrind)
  add_subdirectory(openssl)
  add_subdirectory(jemalloc)
  add_subdirectory(gelfcpp)
  add_subdirectory(folly)
  if (CPR_AND_CURL)
    add_subdirectory(cpr)
  endif()
  add_subdirectory(json)
endif()

if(BUILD_LOGON OR BUILD_WORLD)
  add_subdirectory(mysql)
  add_subdirectory(readline)
  add_subdirectory(gsoap)
  if (WITH_PROMETHEUS_MONITORING)
    set(PROMETHEUS_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(prometheus-cpp)
  endif()
  add_subdirectory(efsw)
  add_subdirectory(libsystemd)
endif()

if(BUILD_TOOLS)
  add_subdirectory(bzip2)
  add_subdirectory(libmpq)
endif()
