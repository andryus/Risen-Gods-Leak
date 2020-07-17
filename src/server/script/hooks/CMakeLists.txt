CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if (USE_SCRIPTPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/script_hooksPCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/script_hooksPCH.cpp)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(script_hooks
  ${PRIVATE_PCH_SOURCE}
  ${PRIVATE_SOURCES})

  
CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(script_hooks
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(script_hooks
  PUBLIC
    shared
	script-interface
  PRIVATE
	script_system
	game-interface)
	
set_property(TARGET script_hooks PROPERTY PROJECT_LABEL "hooks")

set_target_properties(script_hooks
    PROPERTIES
      FOLDER
        "server/script")

# Generate precompiled header
if (USE_SCRIPTPCH)
  add_cxx_pch(script_hooks ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif ()
