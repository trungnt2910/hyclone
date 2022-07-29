project(hpkgvfs_analyze)

include_directories(include)

file(GLOB_RECURSE libhpkgvfs_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/*.cpp)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    file(GLOB_RECURSE libhpkgvfs_sources ${libhpkgvfs_sources} ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/sys/linux/*.cpp)
endif()

add_library(hpkgvfs STATIC ${libhpkgvfs_sources})
target_link_libraries(hpkgvfs hpkg)
target_include_directories(hpkgvfs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../libhpkg/include)
set_property(TARGET hpkgvfs PROPERTY CXX_STANDARD 20)

add_executable(hpkgvfs_analyze ${CMAKE_CURRENT_SOURCE_DIR}/src/hpkgvfs_analyze.cpp)
target_link_libraries(hpkgvfs_analyze hpkgvfs)
set_property(TARGET hpkgvfs_analyze PROPERTY CXX_STANDARD 20)