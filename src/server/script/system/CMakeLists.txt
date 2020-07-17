
CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if (USE_COREPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/script_systemPCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/script_systemPCH.cpp)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_SCRIPT_RELOAD)
  add_library(script_system SHARED
    ${PRIVATE_PCH_SOURCE}
    ${PRIVATE_SOURCES})
else()
  add_library(script_system
    ${PRIVATE_PCH_SOURCE}
    ${PRIVATE_SOURCES})
endif()

  
CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(script_system
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DEXPORT_SCRIPT)

target_link_libraries(script_system
  PUBLIC
    shared
  PRIVATE
    efsw)
	
set_property(TARGET script_system PROPERTY PROJECT_LABEL "system")

set_target_properties(script_system
    PROPERTIES
      FOLDER
        "server/script")

# Generate precompiled header
if (USE_COREPCH)
  add_cxx_pch(script_system ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif ()
