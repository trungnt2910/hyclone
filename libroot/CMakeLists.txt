project(libroot)

add_subdirectory(posix)
add_subdirectory(os)

add_library(root SHARED libroot_init.o)
target_link_options(root PRIVATE
    -nodefaultlibs
    -nostartfiles
    # Haiku does not support DT_GNU_HASH
    -Wl,--hash-style=both,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libroot_versions)
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/crti.o)
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/crtbeginS.o)
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/init_term_dyn.o)
target_link_libraries(root PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,monika>")
target_link_libraries(root PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,root_os>")
target_link_libraries(root PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,root_posix>")
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libsupc++.a)
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libgcc.a)
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libgcc_s.so.1)
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/crtendS.o)
target_link_libraries(root PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/crtn.o)

add_executable(determine_hyclone_revision determine_hyclone_revision.cpp)

add_custom_target(revision.txt ALL
    DEPENDS root determine_hyclone_revision
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}revision.txt
    COMMENT "Determining HyClone revision"
    COMMAND determine_hyclone_revision > ${CMAKE_CURRENT_BINARY_DIR}revision.txt)

add_custom_target(SetHycloneRevision ALL
    DEPENDS root revision.txt
    COMMAND objcopy ${CMAKE_CURRENT_BINARY_DIR}/libroot.so  
        --update-section _haiku_revision=${CMAKE_CURRENT_BINARY_DIR}revision.txt)

install(TARGETS root DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/libgcc_s.so.1 DESTINATION lib)