project(hyclone_server)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})
include_directories(${CMAKE_SOURCE_DIR}/hpkgvfs_analyze/include)

string(TOLOWER ${HYCLONE_HOST} host)

file(GLOB_RECURSE SERVER_SYSTEM_SPECIFIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/${host}/*.cpp)

add_executable(hyclone_server
    main.cpp

    area.cpp
    hsemaphore.cpp
    image.cpp
    port.cpp
    process.cpp
    server_dispatch.cpp
    server_filesystem.cpp
    server_prefix.cpp
    server_workers.cpp
    shutdown.cpp
    system.cpp
    thread.cpp

    ${SERVER_SYSTEM_SPECIFIC_SOURCES})

set_property(TARGET hyclone_server PROPERTY CXX_STANDARD 20)
target_link_libraries(hyclone_server hpkgvfs)
install(TARGETS hyclone_server DESTINATION bin)
if(HYCLONE_HOST_LINUX)
    install(CODE "
        message(STATUS \"Setting cap_sys_ptrace capability for hyclone_server\")
        execute_process(COMMAND sudo setcap cap_sys_ptrace+eip ${CMAKE_INSTALL_PREFIX}/bin/hyclone_server)")
endif()
