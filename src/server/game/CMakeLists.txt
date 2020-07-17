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
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders
  ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring)

if (USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/gamePCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/gamePCH.cpp)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders
  ${CMAKE_CURRENT_SOURCE_DIR}/Monitoring)

if (WITH_PROMETHEUS_MONITORING)
  CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR}/Monitoring monitoring_sources)
  list(APPEND PRIVATE_SOURCES ${monitoring_sources})
endif()

# Provide an interface target for the game project to allow
# dependent projects to build meanwhile.
add_library(game-interface INTERFACE)

target_include_directories(game-interface
  INTERFACE
    ${PUBLIC_INCLUDES})

target_link_libraries(game-interface
  INTERFACE
    shared
    Detour
		script_system
		database-world)

if (ENABLE_SCRIPT_RELOAD)
	add_library(game SHARED
	  ${PRIVATE_PCH_SOURCE}
	  ${PRIVATE_SOURCES})
else()
	add_library(game
	  ${PRIVATE_PCH_SOURCE}
	  ${PRIVATE_SOURCES})
endif()

target_include_directories(game
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})
	
add_definitions(-DEXPORT_GAME)

target_link_libraries(game
  PUBLIC
    game-interface
  PRIVATE
    script_owner
	script_hooks)

set_target_properties(game
    PROPERTIES
      FOLDER
        "server")

# Generate precompiled header
if (USE_COREPCH)
  add_cxx_pch(game ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif ()
