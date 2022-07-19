#include <string.h>

#include "debugbreak.h"
#include "export.h"
#include "linux_syscall.h"

extern "C"
{
    
void MONIKA_EXPORT _kern_debug_output(const char* userString)
{
    size_t len = strlen(userString);
    const char prefix[] = "kern_debug_output: ";
    LINUX_SYSCALL3(__NR_write, 2, prefix, sizeof(prefix));
    LINUX_SYSCALL3(__NR_write, 2, userString, len);
}

void MONIKA_EXPORT _kern_debugger(const char* userString)
{
    size_t len = strlen(userString);
    const char prefix[] = "_kern_debugger: ";
    LINUX_SYSCALL3(__NR_write, 2, prefix, sizeof(prefix));
    LINUX_SYSCALL3(__NR_write, 2, userString, len);
    debug_break();
}

}