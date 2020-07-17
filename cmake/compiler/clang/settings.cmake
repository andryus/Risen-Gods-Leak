include(CheckCXXCompilerFlag)

# Set build-directive (used in core to tell which buildtype we used)
add_definitions(-D_BUILD_DIRECTIVE='"${CMAKE_BUILD_TYPE}"')

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
message(STATUS "Clang: Enabled c++17 support")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
message(STATUS "Clang: Force enabled colored diagnostics")

if(WITH_WARNINGS)
  set(WARNING_FLAGS "-W -Wall -Wextra -Winit-self")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} -Woverloaded-virtual")
  if ( NOT WITH_WARNINGS_PENDANTIC )
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unused-variable -Wno-reorder")
    message(STATUS "Clang: Disabled pendantic warnings: unused-parameter, unused-variable, no-reorder")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
    message(STATUS "Clang: Silence inconsistent-missing-override (temporary)")
  endif()

  check_cxx_compiler_flag("-Wundefined-var-template" HAS_UNDEFINED_VAR_TEMPLATE_WARNING)
  if (HAS_UNDEFINED_VAR_TEMPLATE_WARNING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")
    message(STATUS "Clang: Silence undefined-var-template (false positive)")
  endif()

  message(STATUS "Clang: All warnings enabled")
endif()

if(WITH_FULLDEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
  message(STATUS "Clang: Debug-flags set (-g3)")
endif()

if (WITH_LINK_TIME_OPTIMIZATION)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto=thin")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=thin -fuse-ld=lld")
  message(STATUS "Clang: Enabled Link-Time-Optimitation (-flto)")
endif()
