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
    associateddata.cpp
    entry_ref.cpp
    hsemaphore.cpp
    image.cpp
    io_context.cpp
    kmessage.cpp
    port.cpp
    process.cpp
    server_apploadnotification.cpp
    server_debug.cpp
    server_dispatch.cpp
    server_errno.cpp
    server_filesystem.cpp
    server_memory.cpp
    server_messaging.cpp
    server_nodemonitor.cpp
    server_notificationimpl.cpp
    server_notifications.cpp
    server_prefix.cpp
    server_requests.cpp
    server_systemnotification.cpp
    server_usermap.cpp
    server_vfs.cpp
    server_workers.cpp
    shutdown.cpp
    system.cpp
    thread.cpp

    fs/bindfs.cpp
    fs/devfs.cpp
    fs/hostfs.cpp
    fs/packagefs.cpp
    fs/rootfs.cpp
    fs/systemfs.cpp

    ${SERVER_SYSTEM_SPECIFIC_SOURCES})

set_property(TARGET hyclone_server PROPERTY CXX_STANDARD 20)
target_link_libraries(hyclone_server hpkgvfs)
target_compile_options(hyclone_server PRIVATE
    -Werror -Wall
    -Wno-multichar -Wno-reorder
    -Werror=return-type -Werror=shadow)

if(HYCLONE_USE_SANITIZERS)
    target_compile_options(hyclone_server PRIVATE -fsanitize=address)
    target_link_options(hyclone_server PRIVATE -fsanitize=address)
endif()

install(TARGETS hyclone_server DESTINATION bin)
if(HYCLONE_HOST_LINUX)
    install(CODE "
        message(STATUS \"Setting capabilities for hyclone_server\")
        execute_process(COMMAND sudo setcap cap_sys_ptrace,cap_sys_nice,cap_sys_admin+eip ${CMAKE_INSTALL_PREFIX}/bin/hyclone_server)")
endif()
