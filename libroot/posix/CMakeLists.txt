project(libroot_posix)

add_subdirectory(signal)
add_subdirectory(stdlib)

set(root_posix_objects
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_arch_${HYCLONE_ARCH}.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_crypt.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_locale.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_main.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_pthread.o
        #${CMAKE_CURRENT_SOURCE_DIR}/posix_signal.o
        ${root_posix_signal_objects}
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_stdio.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_musl.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_arch_${HYCLONE_ARCH}.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_ctype.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_ext.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_iconv.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_libio.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_locale.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_math.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_misc.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_regex.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_stdio.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_stdlib.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_gnu_wcsmbs.o
        #${CMAKE_CURRENT_SOURCE_DIR}/posix_stdlib.o
        ${root_posix_stdlib_objects}
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_string.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_string_arch_${HYCLONE_ARCH}.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_sys.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_time.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_unistd.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_wchar.o
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_malloc.o)

add_library(root_posix STATIC ${root_posix_objects})
add_dependencies(root_posix CopyHaikuObjects)