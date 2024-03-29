project(libhpkg)

include_directories(include)
include_directories(external)

file(GLOB_RECURSE libhpkg_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

find_package(ZLIB REQUIRED)

find_path(GMP_INCLUDE_DIR REQUIRED NAMES gmp.h gmpxx.h)
find_library(GMP_LIBRARY REQUIRED NAMES libgmp.a libgmp.lib gmp libgmp)
find_library(ZLIB_LIBRARY REQURIED NAMES libz.a libz.lib zlib zlib)
find_library(ZSTD_LIBRARY REQURIED NAMES libzstd.a libzstd.lib zstd zstd)

include_directories(${GMP_INCLUDE_DIR})

add_library(hpkg STATIC ${libhpkg_sources})
target_link_libraries(hpkg PRIVATE ${ZLIB_LIBRARY})
target_link_libraries(hpkg PRIVATE ${ZSTD_LIBRARY})
target_link_libraries(hpkg PRIVATE ${GMP_LIBRARY})

set_property(TARGET hpkg PROPERTY CXX_STANDARD 20)

add_subdirectory(tests)