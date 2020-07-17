
message(STATUS "SCRIPT PREPARATIONS")

# Make the script module list available in the current scope
GetScriptModuleList(SCRIPT_MODULE_LIST)


macro(PrepareScripts name out)
  file(GLOB_RECURSE found
    ${name}/*.cpp
  )
  list(APPEND ${out} ${found})
  message(STATUS "  -> Prepared: ${name}")
endmacro(PrepareScripts)

PrepareScripts(Northend SCRIPT_SOURCES)
PrepareScripts(EasternKingdoms SCRIPT_SOURCES)
PrepareScripts(Outlands SCRIPT_SOURCES)
PrepareScripts(Spells SCRIPT_SOURCES)

# Configures the scriptloader with the given name and stores the output in the LOADER_OUT variable.
# It is possible to expose multiple subdirectories from the same scriptloader through passing
# it to the variable arguments
function(ConfigureScriptLoader SCRIPTLOADER_NAME LOADER_OUT IS_DYNAMIC)

  set(SCRIPT_DYNAMIC_RELOAD ${IS_DYNAMIC})
  # To generate forward declarations of the loading functions
  unset(SCRIPTS_FORWARD_DECL)
  unset(SCRIPTS_INVOKE)
  # The current script project which is built in
  set(CURRENT_SCRIPT_PROJECT ${SCRIPTLOADER_NAME})

  foreach(LOCAL_SCRIPT_FILE ${SCRIPT_SOURCES})
    # Generate the funciton call and the forward declarations
	get_filename_component(LOCAL_SCRIPT_MODULE ${LOCAL_SCRIPT_FILE} NAME_WE)

    set(SCRIPTS_FORWARD_DECL
      "${SCRIPTS_FORWARD_DECL}SCRIPT_DECL(${LOCAL_SCRIPT_MODULE});\n")
    set(SCRIPTS_INVOKE
      "${SCRIPTS_INVOKE}    SCRIPT_CALL(${LOCAL_SCRIPT_MODULE});\n")
  endforeach()
  set(GENERATED_LOADER ${CMAKE_CURRENT_BINARY_DIR}/gen_scriptloader/RegisteredScripts.cpp)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/RegisteredScripts.cpp.in.cmake ${GENERATED_LOADER})
  set(${LOADER_OUT} ${GENERATED_LOADER} PARENT_SCOPE)
endfunction()

if (ENABLE_SCRIPT_RELOAD)
  ConfigureScriptLoader("" SCRIPT_MODULE_PRIVATE_SCRIPTLOADER ON ${SCRIPT_MODULE})
else()
  ConfigureScriptLoader("" SCRIPT_MODULE_PRIVATE_SCRIPTLOADER OFF ${SCRIPT_MODULE})
endif()

message(STATUS "SCRIPT PREPARATION COMPLETE")
message("")

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)
  
if (USE_SCRIPTPCH)
  set(PRIVATE_PCH_HEADER PrecompiledHeaders/script_contentPCH.h)
  set(PRIVATE_PCH_SOURCE PrecompiledHeaders/script_contentPCH.cpp)
endif ()

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_SCRIPT_RELOAD)
  add_library(script_content MODULE
    ${PRIVATE_PCH_SOURCE}
    ${SCRIPT_MODULE_PRIVATE_SCRIPTLOADER}
    ${PRIVATE_SOURCES})
	  
  set_target_properties(script_content PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /PDBALTPATH:script_content_staged.pdb")
else() 
  add_library(script_content
    ${PRIVATE_PCH_SOURCE}
    ${SCRIPT_MODULE_PRIVATE_SCRIPTLOADER}
    ${PRIVATE_SOURCES})
endif()

if( NOT WIN32 )
  target_compile_options(script_content
  PRIVATE
    "-Wno-parentheses"
    "-Wno-bitwise-op-parentheses")
endif ()

target_include_directories(script_content
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(script_content
  PUBLIC
	script_base
  PRIVATE
	script_owner
    script_system
	efsw)
	
set_property(TARGET script_content PROPERTY PROJECT_LABEL "content")
set_property(TARGET script_content PROPERTY COMPILE_PDB_NAME "$(IntDir)$(TargetName)-$([System.DateTime]::Now.ToString(\"HH_mm_ss_fff\")).pdb")

set_target_properties(script_content
    PROPERTIES
      FOLDER
        "server/script")

# Generate precompiled header
if (USE_SCRIPTPCH)
  add_cxx_pch(script_content ${PRIVATE_PCH_HEADER} ${PRIVATE_PCH_SOURCE})
endif()
