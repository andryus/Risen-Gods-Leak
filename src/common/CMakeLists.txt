# Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/Debugging
  ${CMAKE_CURRENT_SOURCE_DIR}/Platform
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders
  ${CMAKE_CURRENT_SOURCE_DIR}/Graylog
  ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring)

# Manually set sources for Debugging directory as we don't want to include WheatyExceptionReport in common project
# It needs to be included both in authserver and worldserver for the static global variable to be properly initialized
# and to handle crash logs on windows
list(APPEND PRIVATE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/Debugging/Errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Debugging/Errors.h)
  
add_definitions(-DEXPORT_COMMON)

if (USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/commonPCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/commonPCH.cpp)
endif (USE_COREPCH)

if (WITH_GRAYLOG_SUPPORT)
  CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR}/Graylog graylog_sources)
  list(APPEND PRIVATE_SOURCES ${graylog_sources})
endif()

if (WITH_PROMETHEUS_MONITORING)
  CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR}/Monitoring monitoring_sources)
  list(APPEND PRIVATE_SOURCES ${monitoring_sources})
endif()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_SCRIPT_RELOAD)
	add_library(common SHARED
	  ${PRIVATE_PCH_SOURCE}
	  ${PRIVATE_SOURCES}
	)
else()
    add_library(common
	  ${PRIVATE_PCH_SOURCE}
	  ${PRIVATE_SOURCES}
	)
endif()

# Do NOT add any extra include directory here, as we don't want the common
# library to depend on anything else than TC deps, and itself.
# This way we ensure that if either a PR does that without modifying this file,
# a compile error will be generated, either this file will be modified so it
# is detected more easily.
# While it is OK to include files from other libs as long as they don't require
# linkage (enums, defines...) it is discouraged to do so unless necessary, as it will pullute
# include_directories leading to further unnoticed dependency aditions
# Linker Depencency requirements: none
CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders
  ${CMAKE_CURRENT_SOURCE_DIR}/Graylog
  ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring)

target_include_directories(common
  PUBLIC
    # Provide the binary dir for all child targets
    ${CMAKE_BINARY_DIR}
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(common
  PUBLIC
    boost
    fmt
    g3dlib
    Detour
    sfmt
    utf8cpp
    openssl
    valgrind
    threads
    jemalloc
    gelfcpp
    folly
    json)

if (CPR_AND_CURL)
  target_link_libraries(common PUBLIC ${CPR_LIBRARIES})
  target_include_directories(common PUBLIC ${CPR_INCLUDE_DIRS})
  target_compile_definitions(common PUBLIC CONFIG_WITH_CPR_AND_CURL)
endif()
  
if (WITH_PROMETHEUS_MONITORING)
  target_link_libraries(common PUBLIC prometheus-cpp)
  target_compile_definitions(common PUBLIC CONFIG_WITH_PROMETHEUS_MONITORING)
endif()

add_dependencies(common revision_data.h)

set_target_properties(common
    PROPERTIES
      FOLDER
        "server")

# Generate precompiled header
if (USE_COREPCH)
  add_cxx_pch(common ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif ()
