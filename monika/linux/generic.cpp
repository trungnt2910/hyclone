#include <cstddef>

#include "BeDefs.h"
#include "haiku_errors.h"

extern "C"
{

extern void _moni_debug_output(const char* userString);

status_t _moni_generic_syscall(const char* subsystem, uint32 function,
    void* buffer, size_t bufferSize)
{
    // Add this as a non-fatal stub for now.
    // In the future, this should be emulated in hyclone_server, see
    // https://github.com/trungnt2910/hyclone/pull/17#discussion_r1931997407

    _moni_debug_output("stub: _moni_generic_syscall");
    _moni_debug_output(subsystem);

    return B_NAME_NOT_FOUND;
}

}
