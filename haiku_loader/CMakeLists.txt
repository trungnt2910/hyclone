project(haiku_loader)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})

string(TOLOWER ${HYCLONE_HOST} host)

file(GLOB_RECURSE LOADER_SYSTEM_SPECIFIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/${host}/*.cpp)

add_executable(haiku_loader
    main.cpp
    loader_debugger.cpp
    loader_elf.cpp
    loader_idmap.cpp
    loader_lock.cpp
    loader_mutex.cpp
    loader_requests.cpp
    loader_reservedrange.cpp
    loader_tls.cpp

    ${LOADER_SYSTEM_SPECIFIC_SOURCES})

set_property(TARGET haiku_loader PROPERTY CXX_STANDARD 20)
target_compile_options(haiku_loader PRIVATE
    -Werror -Wall
    -Wno-multichar
    -Werror=return-type -Werror=shadow)

if(HYCLONE_USE_SANITIZERS)
    target_compile_options(haiku_loader PRIVATE -fsanitize=address)
    target_link_options(haiku_loader PRIVATE -fsanitize=address)
endif()

install(TARGETS haiku_loader DESTINATION bin)