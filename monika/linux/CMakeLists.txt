enable_language(ASM)
enable_language(C)
enable_language(CXX)
project(syscalls_linux)

include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})

set(monika_sources
    errno_conversion.cpp
    fcntl_conversion.cpp
    linux_syscall.S
    linux_debug.cpp
    linux_version.cpp
    realpath.cpp
    signal_conversion.cpp
    signal_restore.S
    stringutils.cpp

    debug.cpp
    fcntl.cpp
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
    stubs.cpp
    sysinfo.cpp
    threading.cpp
    time.cpp
    timer.cpp
    userid.cpp)

add_library(monika STATIC ${monika_sources})

target_compile_options(monika PRIVATE
    -fno-stack-protector
    -fno-exceptions
    -fvisibility=hidden
    -fPIC)

target_link_options(monika PRIVATE
    -nodefaultlibs
    -nostartfiles)