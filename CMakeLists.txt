project(HyClone)
cmake_minimum_required(VERSION 3.24)

add_custom_target(CopyHaikuObjects
    ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/copy_objects.sh)

if(NOT DEFINED HYCLONE_ARCH)
    set(HYCLONE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    if(HYCLONE_ARCH MATCHES "aarch64")
        set(HYCLONE_ARCH "arm64")
    endif()
endif()

add_compile_definitions(HYCLONE_ARCH=${HYCLONE_ARCH})

if(NOT DEFINED HYCLONE_HOST)
    set(HYCLONE_HOST "${CMAKE_SYSTEM_NAME}")
    add_compile_definitions(HYCLONE_HOST=${HYCLONE_HOST})
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(HYCLONE_HOST_LINUX true)
    add_compile_definitions(HYCLONE_HOST_LINUX)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_compile_definitions(HAIKU_HOST_PLATFORM_64_BIT)
endif()

add_subdirectory(monika)
add_subdirectory(libroot)
add_subdirectory(runtime_loader)
add_subdirectory(libhpkg)
add_subdirectory(hpkgvfs_analyze)
add_subdirectory(haiku_loader)
add_subdirectory(hyclone_server)
