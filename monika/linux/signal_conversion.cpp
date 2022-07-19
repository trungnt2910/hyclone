#include "haiku_signal.h"

#include <climits>
#include <initializer_list>
#include <signal.h>
#include <string.h>

#include "errno_conversion.h"
#include "extended_commpage.h"
#include "linux_debug.h"
#include "signal_conversion.h"

// From Linux <asm/signal.h>
// Conflicts with various stuff from <sys/signal.h>
#define SA_RESTORER	0x04000000

int SignalBToLinux(int signal)
{
    switch (signal)
    {
#define UNSUPPORTED_SIGNAL(signal)                           \
    case HAIKU_##signal:                                     \
        trace("SignalBToLinux: unsupported signal: "#signal);\
        return SIGUSR1;
#define SUPPORTED_SIGNAL(signal)                             \
    case HAIKU_##signal:                                     \
        return signal;

#include "signal_values.h"

#undef UNSUPPORTED_SIGNAL
#undef SUPPORTED_SIGNAL
    default:
        trace("SignalBToLinux: unsupported signal");
        return SIGUSR1;
    }
}

int SignalLinuxToB(int signal)
{
    switch (signal)
    {
#define UNSUPPORTED_SIGNAL(signal)
#define SUPPORTED_SIGNAL(signal)                             \
    case signal:                                             \
        return HAIKU_##signal;

#include "signal_values.h"

#undef UNSUPPORTED_SIGNAL
#undef SUPPORTED_SIGNAL
    default:
        trace("SignalLinuxToB: unsupported signal");
        return HAIKU_SIGUSR1;
    }
}

void *SigHandlerBToLinux(void *handler)
{
    if (handler == (void *)HAIKU_SIG_DFL)
    {
        return (void *)SIG_DFL;
    }
    else if (handler == (void *)HAIKU_SIG_IGN)
    {
        return (void *)SIG_IGN;
    }
    return handler;
}

int SigFlagsBToLinux(int flags)
{
    int linuxFlags = 0;

#define SUPPORTED_SIGFLAG(flag)                              \
    if (flags & HAIKU_##flag)                                \
    {                                                        \
        linuxFlags |= flag;                                  \
        flags ^= HAIKU_##flag;                               \
    }

#include "signal_values.h"

#undef SUPPORTED_SIGFLAG

    if (flags != 0)
    {
        trace("SigFlagsBToLinux: Unsupported flags detected.");
    }

    return linuxFlags;
}

int SigFlagsLinuxToB(int flags)
{
    int haikuFlags = 0;

#define SUPPORTED_SIGFLAG(flag)                              \
    if (flags & flag)                                        \
    {                                                        \
        haikuFlags |= HAIKU_##flag;                          \
        flags ^= flag;                                       \
    }

#include "signal_values.h"

#undef SUPPORTED_SIGFLAG

    if (flags != 0)
    {
        trace("SigFlagsLinuxToB: Unsupported flags detected.");
        if (flags & SA_RESTORER)
        {
            panic("wtf");
        }
    }

    return haikuFlags;
}

inline static unsigned long linux_sigmask(int sig)
{
    return 1ul << ((sig - 1) % (CHAR_BIT * sizeof(unsigned long)));
}

inline static unsigned long linux_sigword(int sig)
{
    return (sig - 1) / (CHAR_BIT * sizeof(unsigned long));
}

inline static int linux_sigaddset(linux_sigset_t& set, int sig)
{
    set.__val[linux_sigword(sig)] |= linux_sigmask(sig);
    return 0;
}

inline static bool linux_sigismember(const linux_sigset_t& set, int sig)
{
    return set.__val[linux_sigword(sig)] & linux_sigmask(sig);
}

inline static int haiku_sigaddset(haiku_sigset_t& sigset, int sig)
{
    sigset |= (1ul << (haiku_sigset_t)(sig - 1));
    return 0;
}

inline static bool haiku_sigismember(haiku_sigset_t& sigset, int sig)
{
    return sigset & (1ul << (haiku_sigset_t)(sig - 1));
}

linux_sigset_t SigSetBToLinux(haiku_sigset_t sigset)
{
    linux_sigset_t linuxSigset;
    memset(&linuxSigset, 0, sizeof(linuxSigset));
    for (const auto i : 
    {
#define SUPPORTED_SIGNAL(signal) HAIKU_##signal,
#include "signal_values.h"
#undef SUPPORTED_SIGNAL
    })    
    {
        if (haiku_sigismember(sigset, i))
        {
            int linuxSignal = SignalBToLinux(i);
            linux_sigaddset(linuxSigset, linuxSignal);
        }
    }

    return linuxSigset;
}

haiku_sigset_t SigSetLinuxToB(linux_sigset_t sigset)
{
    haiku_sigset_t haikuSigset;
    memset(&haikuSigset, 0, sizeof(haikuSigset));
    for (const auto i : 
    {
#define SUPPORTED_SIGNAL(signal) signal,
#include "signal_values.h"
#undef SUPPORTED_SIGNAL
    })
    {
        if (linux_sigismember(sigset, i))
        {
            int haikuSignal = SignalLinuxToB(i);
            haiku_sigaddset(haikuSigset, haikuSignal);
        }
    }

    return haikuSigset;
}

void SigactionBToLinux(const struct haiku_sigaction &sigaction, struct linux_sigaction &linuxSigaction)
{
    if (sigaction.sa_flags & HAIKU_SA_SIGINFO)
    {
        linuxSigaction.handler = (linux_sighandler_t)
            SigHandlerBToLinux((void *)haiku_sigaction_get_sa_sigaction(sigaction));
        linuxSigaction.restorer = __restore_rt;
    }
    else
    {
        linuxSigaction.handler = (linux_sighandler_t)
            SigHandlerBToLinux((void *)haiku_sigaction_get_sa_handler(sigaction));
        linuxSigaction.restorer = __restore;
    }
    linuxSigaction.flags = SigFlagsBToLinux(sigaction.sa_flags);
    linuxSigaction.flags |= SA_RESTORER;
    linuxSigaction.mask = SigSetBToLinux(sigaction.sa_mask);
}

void SigactionLinuxToB(const struct linux_sigaction &linuxSigaction, struct haiku_sigaction &sigaction)
{
    sigaction.sa_flags = SigFlagsLinuxToB(linuxSigaction.flags & ~SA_RESTORER);
    sigaction.sa_mask = SigSetLinuxToB(linuxSigaction.mask);
    sigaction.sa_userdata = NULL;

    if (linuxSigaction.flags & SA_SIGINFO)
    {
        haiku_sigaction_set_sa_sigaction(sigaction, (haiku_sigaction_t)linuxSigaction.handler);
        sigaction.sa_flags |= HAIKU_SA_SIGINFO;
    }
    else
    {
        haiku_sigaction_set_sa_handler(sigaction, (haiku_sighandler_t)linuxSigaction.handler);
    }
}

int SigCodeLinuxToB(int signal, int siCode)
{
    switch (siCode)
    {
#define SUPPORTED_SIGCODE_SI(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_SI
    }

    switch (signal)
    {
        case SIGILL:
            switch (siCode)
            {
#define SUPPORTED_SIGCODE_ILL(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_ILL
            }
            break;
        case SIGFPE:
            switch (siCode)
            {
#define SUPPORTED_SIGCODE_FPE(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_FPE
            }
            break;
        case SIGSEGV:
            switch (siCode)
            {
#define SUPPORTED_SIGCODE_SEGV(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_SEGV
            }
            break;
        case SIGBUS:
            switch (siCode)
            {
#define SUPPORTED_SIGCODE_BUS(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_BUS
            }
            break;
        case SIGTRAP:
            switch (siCode)
            {
#define SUPPORTED_SIGCODE_TRAP(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_TRAP
            }
            break;
        case SIGCHLD:
            switch (siCode)
            {
#define SUPPORTED_SIGCODE_CLD(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_CLD
            }
            break;
        case SIGPOLL:
            switch (siCode)
            {
#define SUPPORTED_SIGCODE_POLL(code) case code: return HAIKU_##code;
#include "signal_values.h"
#undef SUPPORTED_SIGCODE_POLL
            }
            break;
    }

    trace("SigCodeLinuxToB: Unsupported sigcode detected.");
    return 0;
}

void SiginfoLinuxToB(const siginfo_t &linuxSiginfo, haiku_siginfo_t &siginfo)
{    
    siginfo.si_signo = SignalLinuxToB(linuxSiginfo.si_signo);
    siginfo.si_code = SigCodeLinuxToB(linuxSiginfo.si_signo, linuxSiginfo.si_code);
    siginfo.si_errno = LinuxToB(linuxSiginfo.si_errno);
    haiku_siginfo_set_si_pid(siginfo, linuxSiginfo.si_pid);
    haiku_siginfo_set_si_uid(siginfo, linuxSiginfo.si_uid);
    haiku_siginfo_set_si_addr(siginfo, (void *)linuxSiginfo.si_addr);
    haiku_siginfo_set_si_status(siginfo, linuxSiginfo.si_status);
    // We'll support this when we implement _kern_poll
    if ((linuxSiginfo.si_signo == SIGPOLL || linuxSiginfo.si_signo == SIGIO)
        && linuxSiginfo.si_band)
    {
        trace("SiginfoLinuxToB: si_band conversion unimplemented.");
    }
    // The union should be the same.
    haiku_siginfo_set_si_value(siginfo, (const haiku_sigval&)linuxSiginfo.si_value);
}