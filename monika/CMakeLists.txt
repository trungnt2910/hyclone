project(monika)

if(HYCLONE_HOST_LINUX)
    add_subdirectory(linux)
else()
    message(FATAL_ERROR "No syscalls for this platform")
endif()

add_subdirectory(dispatcher)

add_library(monika STATIC)

target_link_libraries(monika PRIVATE
    monika_obj
    monika_dispatcher
)

target_compile_options(monika PRIVATE
    -fno-stack-protector
    -fno-exceptions
    -fvisibility=hidden
    -fPIC)

target_link_options(monika PRIVATE
    -nodefaultlibs
    -nostartfiles)