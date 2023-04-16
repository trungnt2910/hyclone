project(libroot_posix_stdlib)

include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})

add_library(root_posix_stdlib_pty OBJECT pty.cpp)
target_compile_options(root_posix_stdlib_pty PRIVATE -fPIC)
target_compile_definitions(root_posix_stdlib_pty PRIVATE -DHYCLONE_LIBROOT)

set(root_posix_stdlib_objects
    ${CMAKE_CURRENT_SOURCE_DIR}/abs.o
    ${CMAKE_CURRENT_SOURCE_DIR}/atfork.o
    ${CMAKE_CURRENT_SOURCE_DIR}/atof.o
    ${CMAKE_CURRENT_SOURCE_DIR}/atoi.o
    ${CMAKE_CURRENT_SOURCE_DIR}/bsearch.o
    ${CMAKE_CURRENT_SOURCE_DIR}/div.o
    ${CMAKE_CURRENT_SOURCE_DIR}/env.o
    ${CMAKE_CURRENT_SOURCE_DIR}/exit.o
    ${CMAKE_CURRENT_SOURCE_DIR}/heapsort.o
    ${CMAKE_CURRENT_SOURCE_DIR}/merge.o
    ${CMAKE_CURRENT_SOURCE_DIR}/mktemp.o
    #${CMAKE_CURRENT_SOURCE_DIR}/pty.o
    ${CMAKE_CURRENT_SOURCE_DIR}/qsort.o
    ${CMAKE_CURRENT_SOURCE_DIR}/radixsort.o
    ${CMAKE_CURRENT_SOURCE_DIR}/random.o
    ${CMAKE_CURRENT_SOURCE_DIR}/realpath.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strfmon.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strtol.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strtoll.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strtoul.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strtoull.o

    $<TARGET_OBJECTS:root_posix_stdlib_pty>

    PARENT_SCOPE)
