project(haiku_loader)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})

string(TOLOWER ${HYCLONE_HOST} host)

file(GLOB_RECURSE LOADER_SYSTEM_SPECIFIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/${host}/*.cpp)

add_executable(haiku_loader
    main.cpp
    loader_lock.cpp
    loader_mutex.cpp
    loader_reservedrange.cpp
    loader_tls.cpp

    ${LOADER_SYSTEM_SPECIFIC_SOURCES})

set_property(TARGET haiku_loader PROPERTY CXX_STANDARD 20)

install(TARGETS haiku_loader DESTINATION bin)