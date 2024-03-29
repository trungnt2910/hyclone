project(dispatcher)

include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

set(gensyscalls_sourrces
    gensyscalls.cpp
    syscalls.o
)

add_executable(gensyscalls ${gensyscalls_sourrces})

target_compile_options(gensyscalls PRIVATE)
set_property(TARGET gensyscalls PROPERTY CXX_STANDARD 20)

target_link_options(gensyscalls PRIVATE
    -no-pie
    -Wl,--unresolved-symbols=ignore-in-object-files)

add_custom_target(monika_implemented.txt ALL
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/monika_implemented.txt
    COMMENT "Enumerating implemented syscalls"
    COMMAND nm -a $<TARGET_FILE:monika_static> | grep \"_moni_\" | cut -f 3 -d \" \" | grep -v \"^$$\" | sort | uniq -u > ${CMAKE_CURRENT_BINARY_DIR}/monika_implemented.txt)

add_custom_target(monika_dispatcher.cpp ALL
    DEPENDS monika_implemented.txt
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/monika_dispatcher.cpp
    COMMENT "Generating monika_dispatcher.cpp"
    COMMAND $<TARGET_FILE:gensyscalls> ${CMAKE_CURRENT_BINARY_DIR}/monika_implemented.txt ${CMAKE_CURRENT_BINARY_DIR}/monika_dispatcher.cpp)

add_library(monika_dispatcher OBJECT ${CMAKE_CURRENT_BINARY_DIR}/monika_dispatcher.cpp)
add_dependencies(monika_dispatcher monika_dispatcher.cpp)

target_compile_options(monika_dispatcher PRIVATE
    -fno-stack-protector
    -fno-exceptions
    -fvisibility=hidden
    -fPIC)

target_link_options(monika_dispatcher PRIVATE
    -nodefaultlibs
    -nostartfiles)