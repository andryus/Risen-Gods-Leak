CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if (USE_SCRIPTPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/script_ownerPCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/script_ownerPCH.cpp)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
  
# Provide an interface target for the game project to allow
# dependent projects to build meanwhile.
add_library(script-interface INTERFACE)

target_include_directories(script-interface
  INTERFACE
    ${PUBLIC_INCLUDES})

target_link_libraries(script-interface
  INTERFACE
    shared)

add_library(script_owner
  ${PRIVATE_PCH_SOURCE}
  ${PRIVATE_SOURCES})

target_include_directories(script_owner
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(script_owner
  PUBLIC
	script-interface
  PRIVATE
  	script_system
	game-interface
)

set_property(TARGET script_owner PROPERTY PROJECT_LABEL "owner")

set_target_properties(script_owner
    PROPERTIES
      FOLDER
        "server/script")

# Generate precompiled header
if (USE_SCRIPTPCH)
  add_cxx_pch(script_owner ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif ()
