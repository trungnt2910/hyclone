project(libroot_host)

set(libroot_host_objects
    ${CMAKE_CURRENT_SOURCE_DIR}/errors.o
    ${CMAKE_CURRENT_SOURCE_DIR}/fs_attr.o
    ${CMAKE_CURRENT_SOURCE_DIR}/fs_descriptors.o
    ${CMAKE_CURRENT_SOURCE_DIR}/fs.o
    ${CMAKE_CURRENT_SOURCE_DIR}/misc.o
    ${CMAKE_CURRENT_SOURCE_DIR}/sem.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strlcat.o
    ${CMAKE_CURRENT_SOURCE_DIR}/strlcpy.o
    ${CMAKE_CURRENT_SOURCE_DIR}/thread.o
)

add_library(libroot_host STATIC ${libroot_host_objects})
set_target_properties(libroot_host PROPERTIES LINKER_LANGUAGE CXX)