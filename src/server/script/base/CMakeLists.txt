
CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

if (USE_SCRIPTPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/script_basePCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/script_basePCH.cpp)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(script_base
  ${PRIVATE_PCH_SOURCE}
  ${PRIVATE_SOURCES}
)

if( NOT WIN32 )
  target_compile_options(script_base
  PRIVATE
    "-Wno-parentheses"
    "-Wno-bitwise-op-parentheses")
endif ()

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(script_base
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(script_base
  PUBLIC
	script_system
	script_hooks
  PRIVATE
    game
  	script-interface
	)
	
set_property(TARGET script_base PROPERTY PROJECT_LABEL "base")

set_target_properties(script_base
    PROPERTIES
      FOLDER
        "server/script")

# Generate precompiled header
if (USE_SCRIPTPCH)
  add_cxx_pch(script_base ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif()
