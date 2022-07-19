#include <sys/types.h>

#include "extended_commpage.h"
#include "linux_debug.h"
#include "linux_syscall.h"

void trace(const char* message)
{
    const char* end = message;
    while (*end != '\0')
    {
        ++end;
    }

    size_t len = end - message;
    LINUX_SYSCALL3(__NR_write, 2, message, len);
    LINUX_SYSCALL3(__NR_write, 2, "\n", 1);
}

void panic(const char* message)
{
    const char panic[] = "panic: ";
    LINUX_SYSCALL3(__NR_write, 2, panic, sizeof(panic));
    trace(message);

    if (__gCommPageAddress != NULL)
    {
        GET_HOSTCALLS()->at_exit(1);
    }

    while (true)
    {
        LINUX_SYSCALL1(__NR_exit, 1);
    }
}