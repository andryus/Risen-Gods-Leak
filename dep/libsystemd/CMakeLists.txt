if (WITH_SOCKET_ACTIVATION)
    find_package(PkgConfig)

    pkg_check_modules(SYSTEMD REQUIRED "libsystemd")

    add_library(systemd-import INTERFACE)

    target_link_libraries(systemd-import
      INTERFACE
        ${SYSTEMD_LIBRARIES})

    target_include_directories(systemd-import
      INTERFACE
        ${SYSTEMD_INCLUDE_DIR})

    target_compile_definitions(systemd-import
      INTERFACE
        -DSYSTEMD_SOCKET_ACTIVATION)
else()
    add_library(systemd-import INTERFACE)
endif()
