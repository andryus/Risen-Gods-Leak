macro(AddOrReplace script)
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/RG/" "" base_name ${script})
  set(tc_name "${CMAKE_CURRENT_SOURCE_DIR}/${base_name}")

  list(REMOVE_ITEM PRIVATE_SOURCES ${tc_name})
  list(APPEND PRIVATE_SOURCES ${script})
endmacro(AddOrReplace)

macro(CollectScripts out)
  file(GLOB_RECURSE found
    RG/*.h
    RG/*.hpp
    RG/*.cpp
    )
  list(APPEND ${out} ${found})
endmacro(CollectScripts)

CollectScripts(RG_SCRIPTS)

foreach(script ${RG_SCRIPTS})
  AddOrReplace(${script})
endforeach(script)


set(rg_trinity_remove
  Northrend/ChamberOfAspects/ObsidianSanctum/obsidian_sanctum.cpp
  Northrend/Ulduar/Ulduar/boss_yoggsaron.cpp
)

foreach(script ${rg_trinity_remove})
  list(REMOVE_ITEM PRIVATE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${script})
endforeach(script)

message(STATUS "  -> Prepared: RG Scripts")
