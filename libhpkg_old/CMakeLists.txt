project(libhpkg)

include_directories(include)
include_directories(iostream3)

file(GLOB_RECURSE libhpkg_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

find_package(ZLIB REQUIRED)

add_library(iostream3 STATIC ${CMAKE_CURRENT_SOURCE_DIR}/iostream3/zfstream.cc)

add_library(libhpkg STATIC ${libhpkg_sources})
target_link_libraries(libhpkg PRIVATE STATIC ZLIB::ZLIB)
target_link_libraries(libhpkg PRIVATE STATIC iostream3)

set_property(TARGET libhpkg PROPERTY CXX_STANDARD 20)