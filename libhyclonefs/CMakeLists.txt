project(libhyclonefs)

add_subdirectory(libroot_host)
add_subdirectory(fs_shell)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/haiku_include/fs_shell)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/haiku_include/private/fs_shell)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(hyclonefs_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/hyclonefs.cpp
)

set(devfs_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/devfs/kernel_interface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/devfs/RandomVNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/devfs/RootVNode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/devfs/Volume.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/devfs/VNode.cpp
)

add_library(hyclonefs STATIC ${hyclonefs_sources} ${devfs_sources})
target_link_libraries(hyclonefs PUBLIC fs_shell)
