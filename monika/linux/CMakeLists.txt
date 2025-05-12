enable_language(ASM)
enable_language(C)
enable_language(CXX)
project(syscalls_linux)

include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(monika_sources
    errno_conversion.cpp
    fcntl_conversion.cpp
    linux_syscall.S
    linux_debug.cpp
    linux_version.cpp
    realpath.cpp
    signal_conversion.cpp
    signal_restore.S
    socket_conversion.cpp
    stringutils.cpp

    debug.cpp
    disk.cpp
    fcntl.cpp
    generic.cpp
    image.cpp
    io.cpp
    ioctl.cpp
    ipc.cpp
    mman.cpp
    process.cpp
    rlimit.cpp
    shutdown.cpp
    signal.cpp
    socket.cpp
    sysinfo.cpp
    threading.cpp
    time.cpp
    timer.cpp
    userid.cpp)

add_library(monika_static STATIC ${monika_sources})

target_compile_definitions(monika_static PRIVATE
    -DHYCLONE_MONIKA)

add_library(monika_obj OBJECT ${monika_sources})

target_compile_definitions(monika_obj PRIVATE
    -DHYCLONE_MONIKA)

target_compile_options(monika_obj PRIVATE
    -Werror -Wall
    -Wno-multichar
    -Werror=return-type -Werror=shadow
    -fno-stack-protector
    -fno-exceptions
    -fvisibility=hidden
    -fPIC)

target_link_options(monika_obj PRIVATE
    -nodefaultlibs
    -nostartfiles)
