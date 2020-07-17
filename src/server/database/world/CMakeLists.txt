CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES)

add_definitions(-DEXPORT_DB)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

if (ENABLE_SCRIPT_RELOAD)
  add_library(database-world SHARED
    ${PRIVATE_PCH_SOURCE}
    ${PRIVATE_SOURCES}
  )
else()
  add_library(database-world
    ${PRIVATE_PCH_SOURCE}
    ${PRIVATE_SOURCES}
  )
endif()

# Do NOT add any extra include directory unless it does not create unneeded extra dependencies,
# and specially, not add any dependency to neither of these: shared, game, scripts
# This way we ensure that if either a PR does that without modifying this file,
# a compile error will be generated, either this file will be modified so it
# is detected more easily.
# While it is OK to include files from other libs as long as they don't require
# linkage (enums, defines...) it is discouraged to do so unless necessary, as it will pullute
# include_directories leading to further unnoticed dependency aditions
# Linker Depencency requirements: common
CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES)

target_include_directories(database-world
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(database-world
  PUBLIC
    database)

set_target_properties(database-world
  PROPERTIES
    FOLDER
      "server")
