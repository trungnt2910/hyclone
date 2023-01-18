#include "haiku_signal.h"

#include <errno.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/ucontext.h>

#include "BeDefs.h"
#include "errno_conversion.h"
#include "export.h"
#include "extended_commpage.h"
#include "haiku_errors.h"
#include "linux_debug.h"
#include "linux_subsystemlock.h"
#include "linux_syscall.h"
#include "signal_conversion.h"
#include "signal_defs.h"

static void SigHandlerTrampoline(int signal);
static void SigActionTrampoline(int signal, siginfo_t *siginfo, void *context);

static int sSignalSubsystemLock = HYCLONE_SUBSYSTEM_LOCK_UNINITIALIZED;
haiku_sigaction sHaikuSignalHandlers[HAIKU_NSIG];

extern "C"
{

status_t MONIKA_EXPORT _kern_sigaction(int sig, const struct haiku_sigaction *action, struct haiku_sigaction *oldAction)
{
    CHECK_COMMPAGE();

    if (sig >= HAIKU_NSIG)
    {
        return B_BAD_VALUE;
    }

    auto systemLock = SubsystemLock(sSignalSubsystemLock);

    struct linux_sigaction linuxSigactionMemory;
    struct linux_sigaction oldLinuxSigactionMemory;

    struct linux_sigaction *linuxSigaction = NULL;
    struct linux_sigaction *oldLinuxSigaction = NULL;

    if (action != NULL)
    {
        linuxSigaction = &linuxSigactionMemory;
        SigactionBToLinux(*action, *linuxSigaction);

        if (linuxSigaction->handler != SIG_IGN && linuxSigaction->handler != SIG_DFL)
        {
            if (linuxSigaction->flags & SA_SIGINFO)
            {
                linuxSigaction->handler = (linux_sighandler_t)SigActionTrampoline;
            }
            else
            {
                linuxSigaction->handler = (linux_sighandler_t)SigHandlerTrampoline;
            }
        }
    }

    if (oldAction != NULL)
    {
        oldLinuxSigaction = &oldLinuxSigactionMemory;
    }

    status_t result = LINUX_SYSCALL4(__NR_rt_sigaction,
        SignalBToLinux(sig), linuxSigaction, oldLinuxSigaction, sizeof(linux_sigset_t));

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    if (oldAction != NULL)
    {
        SigactionLinuxToB(*oldLinuxSigaction, *oldAction);
        if (oldAction->sa_flags & HAIKU_SA_SIGINFO &&
            haiku_sigaction_get_sa_sigaction(*oldAction) == (haiku_sigaction_t)SigActionTrampoline)
        {
            haiku_sigaction_set_sa_sigaction(*oldAction,
                haiku_sigaction_get_sa_sigaction(sHaikuSignalHandlers[sig]));
        }
        else if (!(oldAction->sa_flags & HAIKU_SA_SIGINFO) &&
            haiku_sigaction_get_sa_handler(*oldAction) == (haiku_sighandler_t)SigHandlerTrampoline)
        {
            haiku_sigaction_set_sa_handler(*oldAction,
                haiku_sigaction_get_sa_handler(sHaikuSignalHandlers[sig]));
        }
    }

    // Only modify our handler array after the old signal has been retrieved.
    if (action != NULL)
    {
        sHaikuSignalHandlers[sig] = *action;
    }

    return B_OK;
}

status_t MONIKA_EXPORT _kern_set_signal_mask(int how, const haiku_sigset_t *set, haiku_sigset_t *oldSet)
{
    linux_sigset_t linuxSetMemory;
    linux_sigset_t oldLinuxSetMemory;

    linux_sigset_t *linuxSet = NULL;
    linux_sigset_t *oldLinuxSet = NULL;

    if (set != NULL)
    {
        linuxSetMemory = SigSetBToLinux(*set);
        linuxSet = &linuxSetMemory;
    }

    if (oldSet != NULL)
    {
        oldLinuxSet = &oldLinuxSetMemory;
    }

    int linuxHow = 0;
    switch (how)
    {
        case HAIKU_SIG_BLOCK:
            linuxHow = SIG_BLOCK;
            break;
        case HAIKU_SIG_UNBLOCK:
            linuxHow = SIG_UNBLOCK;
            break;
        case HAIKU_SIG_SETMASK:
            linuxHow = SIG_SETMASK;
            break;
    }

    long result = LINUX_SYSCALL4(__NR_rt_sigprocmask, linuxHow, linuxSet, oldLinuxSet, sizeof(linux_sigset_t));

    if (result < 0)
    {
        return LinuxToB(-result);
    }

    if (oldSet != NULL)
    {
        *oldSet = SigSetLinuxToB(*oldLinuxSet);
    }

    return B_OK;
}

status_t MONIKA_EXPORT _kern_send_signal(int32 id, uint32 signal, const union haiku_sigval* userValue, uint32 flags)
{
    int linuxSignal = SignalBToLinux(signal);

    union sigval linuxValue;

    if (userValue != NULL)
    {
        linuxValue = *(const union sigval*)userValue;
    }
    else
    {
        memset(&linuxValue, 0, sizeof(linuxValue));
    }

    siginfo_t siginfo;
    siginfo.si_signo = linuxSignal;
    siginfo.si_code = SI_QUEUE;
    siginfo.si_value = linuxValue;
    siginfo.si_pid = LINUX_SYSCALL0(__NR_getpid);
    siginfo.si_uid = LINUX_SYSCALL0(__NR_getuid);

    long status;

    if (flags & SIGNAL_FLAG_SEND_TO_THREAD)
    {
        if (flags & SIGNAL_FLAG_QUEUING_REQUIRED)
        {
            status = -ENOSYS;
            // To do: Get the PID from thread?
            // status = LINUX_SYSCALL4(__NR_rt_tgsigqueueinfo, ??, id, linuxSignal, &siginfo);
        }
        else
        {
            status = LINUX_SYSCALL2(__NR_tkill, id, linuxSignal);
        }
    }
    else
    {
        status = LINUX_SYSCALL3(__NR_rt_sigqueueinfo, id, linuxSignal, &siginfo);
    }

    if (status < 0)
    {
        return LinuxToB(-status);
    }

    return B_OK;
}

}

void SigHandlerTrampoline(int signal)
{
    typedef void (haiku_internal_sighandler_t)(int, void*, vregs*);

    signal = SignalLinuxToB(signal);

    haiku_internal_sighandler_t *handler = (haiku_internal_sighandler_t*)
        haiku_sigaction_get_sa_handler(sHaikuSignalHandlers[signal]);

    // Currently stubbed, but allocate enough memory to prevent crashes.
    vregs regs;

    //trace("SigHandlerTrampoline: Passing control to Haiku signal handler...");

    handler(signal, sHaikuSignalHandlers[signal].sa_userdata, &regs);
}

void SigActionTrampoline(int signal, siginfo_t *siginfo, void *context)
{
    signal = SignalLinuxToB(signal);

    haiku_sigaction_t handler = (haiku_sigaction_t)
        haiku_sigaction_get_sa_sigaction(sHaikuSignalHandlers[signal]);

    haiku_siginfo_t haikuSiginfo;
    SiginfoLinuxToB(*siginfo, haikuSiginfo);

    // TODO: Marshal context to Haiku's context type.

    handler(signal, &haikuSiginfo, context);
}