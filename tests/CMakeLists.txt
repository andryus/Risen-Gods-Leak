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
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
  
if (USE_TESTPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/testPCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/testPCH.cpp)
endif (USE_TESTPCH)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tests
  ${PRIVATE_PCH_SOURCE}
  ${PRIVATE_SOURCES}
)

target_link_libraries(tests
  PUBLIC
    boost
    game
    )

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(tests
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(tests
    PROPERTIES
      FOLDER
        "tests")

if (USE_TESTPCH)
  add_cxx_pch(tests ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif()