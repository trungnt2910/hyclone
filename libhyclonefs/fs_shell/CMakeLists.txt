project(fs_shell)

set(fs_shell_objects
    ${CMAKE_CURRENT_SOURCE_DIR}/fs_shell_kernel.o
)

add_library(fs_shell STATIC ${fs_shell_objects})
target_link_libraries(fs_shell PUBLIC libroot_host)
set_target_properties(fs_shell PROPERTIES LINKER_LANGUAGE CXX)