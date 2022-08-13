project(libroot_posix_signal)

include_directories(${CMAKE_SOURCE_DIR}/shared_headers)
include_directories(${CMAKE_SOURCE_DIR}/shared_headers/${HYCLONE_ARCH})

add_library(root_posix_signal_signal_limits OBJECT signal_limits.cpp)
target_compile_options(root_posix_signal_signal_limits PRIVATE -fPIC)

set(root_posix_signal_objects
        ${CMAKE_CURRENT_SOURCE_DIR}/kill.o
        ${CMAKE_CURRENT_SOURCE_DIR}/killpg.o
        ${CMAKE_CURRENT_SOURCE_DIR}/psiginfo.o
        ${CMAKE_CURRENT_SOURCE_DIR}/psignal.o
        ${CMAKE_CURRENT_SOURCE_DIR}/raise.o
        ${CMAKE_CURRENT_SOURCE_DIR}/send_signal.o
        ${CMAKE_CURRENT_SOURCE_DIR}/set_signal_mask.o
        ${CMAKE_CURRENT_SOURCE_DIR}/set_signal_stack.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigaction.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigaltstack.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sighold.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigignore.o
        ${CMAKE_CURRENT_SOURCE_DIR}/siginterrupt.o
        ${CMAKE_CURRENT_SOURCE_DIR}/signal.o
        #${CMAKE_CURRENT_SOURCE_DIR}/signal_limits.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigpause.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigpending.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigqueue.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigrelse.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigset.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigset_accessors.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigsuspend.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigtimedwait.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigwait.o
        ${CMAKE_CURRENT_SOURCE_DIR}/sigwaitinfo.o
        ${CMAKE_CURRENT_SOURCE_DIR}/strsignal.o

        $<TARGET_OBJECTS:root_posix_signal_signal_limits>

        PARENT_SCOPE)