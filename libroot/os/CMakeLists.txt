project(libroot_os)

include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})

add_library(root_os_system_time OBJECT system_time.cpp)
target_compile_options(root_os_system_time PRIVATE -fPIC)

add_library(root_os_tls OBJECT tls.cpp)
target_compile_options(root_os_tls PRIVATE -fPIC)

add_library(root_os_objects OBJECT IMPORTED)

set(root_os_arch_objects)

if(HYCLONE_ARCH STREQUAL "x86_64")
    set(root_os_arch_objects
        # Architecture-specific stuff
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/byteorder.o
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/get_stack_frame.o
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/system_info.o
        #${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/system_time.o
        #${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/thread.o
        ${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/time.o
        #${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/tls.o
        
        # sources from generic
		${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/generic_atomic.o
		${CMAKE_CURRENT_SOURCE_DIR}/arch/${HYCLONE_ARCH}/generic_stack_trace.o)
endif()

set_property(TARGET root_os_objects PROPERTY
    IMPORTED_OBJECTS
        ${CMAKE_CURRENT_SOURCE_DIR}/Architecture.o
        ${CMAKE_CURRENT_SOURCE_DIR}/area.o
        ${CMAKE_CURRENT_SOURCE_DIR}/atomic.o
        ${CMAKE_CURRENT_SOURCE_DIR}/debug.o
        ${CMAKE_CURRENT_SOURCE_DIR}/driver_settings.o
        ${CMAKE_CURRENT_SOURCE_DIR}/extended_system_info.o
        ${CMAKE_CURRENT_SOURCE_DIR}/find_directory.o
        ${CMAKE_CURRENT_SOURCE_DIR}/find_paths.o
        ${CMAKE_CURRENT_SOURCE_DIR}/fs_attr.o
        ${CMAKE_CURRENT_SOURCE_DIR}/fs_index.o
        ${CMAKE_CURRENT_SOURCE_DIR}/fs_info.o
        ${CMAKE_CURRENT_SOURCE_DIR}/fs_query.o
        ${CMAKE_CURRENT_SOURCE_DIR}/fs_volume.o
        ${CMAKE_CURRENT_SOURCE_DIR}/image.o
        ${CMAKE_CURRENT_SOURCE_DIR}/launch.o
        ${CMAKE_CURRENT_SOURCE_DIR}/memory.o
        ${CMAKE_CURRENT_SOURCE_DIR}/parsedate.o
        ${CMAKE_CURRENT_SOURCE_DIR}/port.o
        ${CMAKE_CURRENT_SOURCE_DIR}/scheduler.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sem.o
        ${CMAKE_CURRENT_SOURCE_DIR}/stack_protector.o
        ${CMAKE_CURRENT_SOURCE_DIR}/system_info.o
        ${CMAKE_CURRENT_SOURCE_DIR}/system_revision.o
        ${CMAKE_CURRENT_SOURCE_DIR}/team.o
        ${CMAKE_CURRENT_SOURCE_DIR}/thread.o
        ${CMAKE_CURRENT_SOURCE_DIR}/time.o
        # Do NOT add this one.
        # syscalls.o
        ${CMAKE_CURRENT_SOURCE_DIR}/wait_for_objects.o

        ${CMAKE_CURRENT_SOURCE_DIR}/init_once.o
        ${CMAKE_CURRENT_SOURCE_DIR}/mutex.o
        ${CMAKE_CURRENT_SOURCE_DIR}/recursive_lock.o
        ${CMAKE_CURRENT_SOURCE_DIR}/rw_lock.o

        ${CMAKE_CURRENT_SOURCE_DIR}/KMessage.o

        ${root_os_arch_objects}
        )

add_library(root_os STATIC blank.c)
target_link_libraries(root_os PRIVATE root_os_objects)
target_link_libraries(root_os PRIVATE root_os_system_time)
target_link_libraries(root_os PRIVATE root_os_tls)
target_compile_options(root_os PRIVATE -fPIC)