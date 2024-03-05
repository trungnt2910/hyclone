project(runtime_loader)

# Imported object library
add_library(HaikuRuntimeLoaderObjects OBJECT IMPORTED)

set_property(TARGET HaikuRuntimeLoaderObjects PROPERTY
    IMPORTED_OBJECTS
        ${CMAKE_CURRENT_SOURCE_DIR}/add_ons.o
        ${CMAKE_CURRENT_SOURCE_DIR}/elf.o
        ${CMAKE_CURRENT_SOURCE_DIR}/elf_haiku_version.o
        ${CMAKE_CURRENT_SOURCE_DIR}/elf_load_image.o
        ${CMAKE_CURRENT_SOURCE_DIR}/elf_symbol_lookup.o
        ${CMAKE_CURRENT_SOURCE_DIR}/elf_tls.o
        ${CMAKE_CURRENT_SOURCE_DIR}/elf_versioning.o
        ${CMAKE_CURRENT_SOURCE_DIR}/pe.o
        ${CMAKE_CURRENT_SOURCE_DIR}/errors.o
        ${CMAKE_CURRENT_SOURCE_DIR}/export.o
        ${CMAKE_CURRENT_SOURCE_DIR}/heap.o
        ${CMAKE_CURRENT_SOURCE_DIR}/images.o
        ${CMAKE_CURRENT_SOURCE_DIR}/runtime_loader.o
        ${CMAKE_CURRENT_SOURCE_DIR}/utility.o
)

add_custom_target(RemoveHaikuSyscalls
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libruntime_loader.a
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/libruntime_loader.a ${CMAKE_CURRENT_SOURCE_DIR}/libruntime_loader_nosyscalls.a
    COMMAND ar d ${CMAKE_CURRENT_SOURCE_DIR}/libruntime_loader_nosyscalls.a syscalls.o tls.o)

add_library(runtime_loader SHARED blank.c)
target_link_options(runtime_loader PRIVATE
    -nodefaultlibs
    -nostartfiles
    -Wl,--no-undefined,-shared,-Bsymbolic,-soname=runtime_loader,-entry=runtime_loader
)
set_target_properties(runtime_loader PROPERTIES
    OUTPUT_NAME "runtime_loader"
    SUFFIX ""
    PREFIX "")
add_dependencies(runtime_loader RemoveHaikuSyscalls)
target_link_libraries(runtime_loader PRIVATE HaikuRuntimeLoaderObjects)
target_link_libraries(runtime_loader PRIVATE monika)
target_link_libraries(runtime_loader PRIVATE root_os_tls)
target_link_libraries(runtime_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libgcc_eh-kernel.a)
target_link_libraries(runtime_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libgcc-kernel.a)
target_link_libraries(runtime_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libruntime_loader_${HYCLONE_ARCH}.a)
target_link_libraries(runtime_loader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libruntime_loader_nosyscalls.a)

install(TARGETS runtime_loader DESTINATION bin)
