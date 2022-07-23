project(libhpkg_tests)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_executable(hpkg_test_dumpfiles test_dumpfiles.cpp)
target_link_libraries(hpkg_test_dumpfiles hpkg)
file(
    COPY ${CMAKE_CURRENT_SOURCE_DIR}/tipster-1.1.1-1-x86_64.hpkg
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})