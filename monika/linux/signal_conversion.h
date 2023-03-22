#ifndef __SIGNAL_CONVERSION_H__
#define __SIGNAL_CONVERSION_H__

#include <signal.h>

extern "C" void __restore(), __restore_rt();

struct linux_sigset_t;

#ifndef HAIKU_SIGSET_T_DEFINED
#include "BeDefs.h"
typedef uint64 haiku_sigset_t;
#define HAIKU_SIGSET_T_DEFINED
#endif

int SignalBToLinux(int signal);
int SignalLinuxToB(int signal);
void SigactionBToLinux(const struct haiku_sigaction &sigaction, struct linux_sigaction &linuxSigaction);
void SigactionLinuxToB(const struct linux_sigaction &linuxSigaction, struct haiku_sigaction &sigaction);
void SiginfoLinuxToB(const siginfo_t &linuxSiginfo, haiku_siginfo_t &siginfo);
void StackBToLinux(const haiku_stack_t& stack, stack_t& linuxStack);
void StackLinuxToB(const stack_t& linuxStack, haiku_stack_t& stack);
linux_sigset_t SigSetBToLinux(haiku_sigset_t sigset);
haiku_sigset_t SigSetLinuxToB(linux_sigset_t sigset);
void ContextLinuxToB(const ucontext_t& linuxContext, haiku_ucontext_t& context);

struct linux_sigset_t
{
    union
    {
        // musl seems to use _NSIG / CHAR_BIT and it just works.
        char __buf[NSIG / CHAR_BIT];
        unsigned long __val[0];
    };
};

// According to musl.
struct linux_sigaction
{
    void (*handler)(int);
    unsigned long flags;
    void (*restorer)(void);
    linux_sigset_t mask;
};

typedef decltype(linux_sigaction::handler) linux_sighandler_t;

#endif // __SIGNAL_CONVERSION_H__