#ifndef SUPPORTED_SIGNAL
#define SUPPORTED_SIGNAL(name)
#define SUPPORTED_SIGNAL_NOT_PREDEFINED
#endif

#ifndef UNSUPPORTED_SIGNAL
#define UNSUPPORTED_SIGNAL(name)
#define UNSUPPORTED_SIGNAL_NOT_PREDEFINED
#endif

SUPPORTED_SIGNAL(SIGHUP)
SUPPORTED_SIGNAL(SIGINT)
SUPPORTED_SIGNAL(SIGQUIT)
SUPPORTED_SIGNAL(SIGILL)
SUPPORTED_SIGNAL(SIGCHLD)
SUPPORTED_SIGNAL(SIGABRT)
SUPPORTED_SIGNAL(SIGPIPE)
SUPPORTED_SIGNAL(SIGFPE)
SUPPORTED_SIGNAL(SIGKILL)
SUPPORTED_SIGNAL(SIGSTOP)
SUPPORTED_SIGNAL(SIGSEGV)
SUPPORTED_SIGNAL(SIGCONT)
SUPPORTED_SIGNAL(SIGTSTP)
SUPPORTED_SIGNAL(SIGALRM)
SUPPORTED_SIGNAL(SIGTERM)
SUPPORTED_SIGNAL(SIGTTIN)
SUPPORTED_SIGNAL(SIGTTOU)
SUPPORTED_SIGNAL(SIGUSR1)
SUPPORTED_SIGNAL(SIGUSR2)
SUPPORTED_SIGNAL(SIGWINCH)
SUPPORTED_SIGNAL(SIGTRAP)
SUPPORTED_SIGNAL(SIGPOLL)
SUPPORTED_SIGNAL(SIGPROF)
SUPPORTED_SIGNAL(SIGSYS)
SUPPORTED_SIGNAL(SIGURG)
SUPPORTED_SIGNAL(SIGVTALRM)
SUPPORTED_SIGNAL(SIGXCPU)
SUPPORTED_SIGNAL(SIGXFSZ)
SUPPORTED_SIGNAL(SIGBUS)

UNSUPPORTED_SIGNAL(SIGKILLTHR)
UNSUPPORTED_SIGNAL(SIGRESERVED1)
UNSUPPORTED_SIGNAL(SIGRESERVED2)

#ifdef SUPPORTED_SIGNAL_NOT_PREDEFINED
#undef SUPPORTED_SIGNAL_NOT_PREDEFINED
#undef SUPPORTED_SIGNAL
#endif

#ifdef UNSUPPORTED_SIGNAL_NOT_PREDEFINED
#undef UNSUPPORTED_SIGNAL_NOT_PREDEFINED
#undef UNSUPPORTED_SIGNAL
#endif

#ifndef SUPPORTED_SIGFLAG
#define SUPPORTED_SIGFLAG(name)
#define SUPPORTED_SIGFLAG_NOT_PREDEFINED
#endif

SUPPORTED_SIGFLAG(SA_NOCLDSTOP)
SUPPORTED_SIGFLAG(SA_NOCLDWAIT)
SUPPORTED_SIGFLAG(SA_RESETHAND)
SUPPORTED_SIGFLAG(SA_NODEFER)
SUPPORTED_SIGFLAG(SA_RESTART)
SUPPORTED_SIGFLAG(SA_ONSTACK)
SUPPORTED_SIGFLAG(SA_SIGINFO)
SUPPORTED_SIGFLAG(SA_NOMASK)
SUPPORTED_SIGFLAG(SA_STACK)
SUPPORTED_SIGFLAG(SA_ONESHOT)

#ifdef SUPPORTED_SIGFLAG_NOT_PREDEFINED
#undef SUPPORTED_SIGFLAG_NOT_PREDEFINED
#undef SUPPORTED_SIGFLAG
#endif

// Members of different categories
// may have the same value.
#ifndef SUPPORTED_SIGCODE_SI
#define SUPPORTED_SIGCODE_SI(name)
#define SUPPORTED_SIGCODE_SI_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_SI(SI_USER)
SUPPORTED_SIGCODE_SI(SI_QUEUE)
SUPPORTED_SIGCODE_SI(SI_TIMER)
SUPPORTED_SIGCODE_SI(SI_ASYNCIO)
SUPPORTED_SIGCODE_SI(SI_MESGQ)

#ifdef SUPPORTED_SIGCODE_SI_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_SI_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_SI
#endif

#ifndef SUPPORTED_SIGCODE_ILL
#define SUPPORTED_SIGCODE_ILL(name)
#define SUPPORTED_SIGCODE_ILL_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_ILL(ILL_ILLOPC)
SUPPORTED_SIGCODE_ILL(ILL_ILLOPN)
SUPPORTED_SIGCODE_ILL(ILL_ILLADR)
SUPPORTED_SIGCODE_ILL(ILL_ILLTRP)
SUPPORTED_SIGCODE_ILL(ILL_PRVOPC)
SUPPORTED_SIGCODE_ILL(ILL_PRVREG)
SUPPORTED_SIGCODE_ILL(ILL_COPROC)
SUPPORTED_SIGCODE_ILL(ILL_BADSTK)

#ifdef SUPPORTED_SIGCODE_ILL_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_ILL_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_ILL
#endif

#ifndef SUPPORTED_SIGCODE_FPE
#define SUPPORTED_SIGCODE_FPE(name)
#define SUPPORTED_SIGCODE_FPE_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_FPE(FPE_INTDIV)
SUPPORTED_SIGCODE_FPE(FPE_INTOVF)
SUPPORTED_SIGCODE_FPE(FPE_FLTDIV)
SUPPORTED_SIGCODE_FPE(FPE_FLTOVF)
SUPPORTED_SIGCODE_FPE(FPE_FLTUND)
SUPPORTED_SIGCODE_FPE(FPE_FLTRES)
SUPPORTED_SIGCODE_FPE(FPE_FLTINV)
SUPPORTED_SIGCODE_FPE(FPE_FLTSUB)

#ifdef SUPPORTED_SIGCODE_FPE_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_FPE_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_FPE
#endif

#ifndef SUPPORTED_SIGCODE_SEGV
#define SUPPORTED_SIGCODE_SEGV(name)
#define SUPPORTED_SIGCODE_SEGV_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_SEGV(SEGV_MAPERR)
SUPPORTED_SIGCODE_SEGV(SEGV_ACCERR)

#ifdef SUPPORTED_SIGCODE_SEGV_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_SEGV_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_SEGV
#endif

#ifndef SUPPORTED_SIGCODE_BUS
#define SUPPORTED_SIGCODE_BUS(name)
#define SUPPORTED_SIGCODE_BUS_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_BUS(BUS_ADRALN)
SUPPORTED_SIGCODE_BUS(BUS_ADRERR)
SUPPORTED_SIGCODE_BUS(BUS_OBJERR)

#ifdef SUPPORTED_SIGCODE_BUS_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_BUS_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_BUS
#endif

#ifndef SUPPORTED_SIGCODE_TRAP
#define SUPPORTED_SIGCODE_TRAP(name)
#define SUPPORTED_SIGCODE_TRAP_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_TRAP(TRAP_BRKPT)
SUPPORTED_SIGCODE_TRAP(TRAP_TRACE)

#ifdef SUPPORTED_SIGCODE_TRAP_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_TRAP_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_TRAP
#endif

#ifndef SUPPORTED_SIGCODE_CLD
#define SUPPORTED_SIGCODE_CLD(name)
#define SUPPORTED_SIGCODE_CLD_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_CLD(CLD_EXITED)
SUPPORTED_SIGCODE_CLD(CLD_KILLED)
SUPPORTED_SIGCODE_CLD(CLD_DUMPED)
SUPPORTED_SIGCODE_CLD(CLD_TRAPPED)
SUPPORTED_SIGCODE_CLD(CLD_STOPPED)
SUPPORTED_SIGCODE_CLD(CLD_CONTINUED)

#ifdef SUPPORTED_SIGCODE_CLD_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_CLD_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_CLD
#endif

#ifndef SUPPORTED_SIGCODE_POLL
#define SUPPORTED_SIGCODE_POLL(name)
#define SUPPORTED_SIGCODE_POLL_NOT_PREDEFINED
#endif

SUPPORTED_SIGCODE_POLL(POLL_IN)
SUPPORTED_SIGCODE_POLL(POLL_OUT)
SUPPORTED_SIGCODE_POLL(POLL_MSG)
SUPPORTED_SIGCODE_POLL(POLL_ERR)
SUPPORTED_SIGCODE_POLL(POLL_PRI)
SUPPORTED_SIGCODE_POLL(POLL_HUP)

#ifdef SUPPORTED_SIGCODE_POLL_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_POLL_NOT_PREDEFINED
#undef SUPPORTED_SIGCODE_POLL
#endif