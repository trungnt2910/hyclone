project(fs_shell)

set(fs_shell_objects
    ${CMAKE_CURRENT_SOURCE_DIR}/errors.o
    ${CMAKE_CURRENT_SOURCE_DIR}/fs_attr.o
    ${CMAKE_CURRENT_SOURCE_DIR}/fs_descriptors.o
    ${CMAKE_CURRENT_SOURCE_DIR}/fs_shell_kernel.o
    ${CMAKE_CURRENT_SOURCE_DIR}/fs.o
    ${CMAKE_CURRENT_SOURCE_DIR}/misc.o
    ${CMAKE_CURRENT_SOURCE_DIR}/sem.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strlcat.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strlcpy.o
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.o
)

add_library(fs_shell STATIC ${fs_shell_objects})
set_target_properties(fs_shell PROPERTIES LINKER_LANGUAGE CXX)