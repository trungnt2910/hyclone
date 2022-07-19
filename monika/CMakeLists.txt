project(monika)

if(HYCLONE_HOST_LINUX)
    add_subdirectory(linux)
else()
    message(FATAL_ERROR "No syscalls for this platform")
endif()