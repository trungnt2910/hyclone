project(hyclonefs_analyze)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libhyclonefs/haiku_include/fs_shell)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libhyclonefs/haiku_include/private/fs_shell)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libhyclonefs/include)

add_executable(hyclonefs_analyze
    main.cpp
)

target_link_libraries(hyclonefs_analyze
    hyclonefs
)