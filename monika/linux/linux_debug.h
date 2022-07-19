#ifndef __LINUX_SYSCALLS_DEBUG__
#define __LINUX_SYSCALLS_DEBUG__

__attribute__ ((noreturn))
void panic(const char* message);

void trace(const char* message);

#endif