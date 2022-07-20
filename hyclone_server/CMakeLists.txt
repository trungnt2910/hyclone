project(hyclone_server)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})

string(TOLOWER ${HYCLONE_HOST} host)

file(GLOB_RECURSE SERVER_SYSTEM_SPECIFIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/${host}/*.cpp)

add_executable(hyclone_server
    main.cpp

    area.cpp
    image.cpp
    process.cpp
    server_dispatch.cpp
    server_workers.cpp
    shutdown.cpp
    system.cpp

    ${SERVER_SYSTEM_SPECIFIC_SOURCES})

set_property(TARGET hyclone_server PROPERTY CXX_STANDARD 20)

install(TARGETS hyclone_server DESTINATION bin)
if(HYCLONE_HOST_LINUX)
    install(CODE "
        message(STATUS \"Setting cap_sys_ptrace capability for hyclone_server\")
        execute_process(COMMAND sudo setcap cap_sys_ptrace+eip ${CMAKE_INSTALL_PREFIX}/bin/hyclone_server)")
endif()