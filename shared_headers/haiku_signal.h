/*
 * Copyright 2002-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU_SIGNAL_H_
#define _HAIKU_SIGNAL_H_

#ifdef _SIGNAL_H
#warning "Including haiku_signal.h after the host's <signal.h> may cause errors"
#endif

#include <cstddef>
#include "BeDefs.h"
#include "pthread_private.h"

typedef int haiku_sig_atomic_t;
#ifndef HAIKU_SIGSET_T_DEFINED
typedef uint64 haiku_sigset_t;
#define HAIKU_SIGSET_T_DEFINED
#endif

/* macros defining the standard signal handling behavior */
#define HAIKU_SIG_DFL ((__haiku_sighandler_t)0)  /* "default" signal behaviour */
#define HAIKU_SIG_IGN ((__haiku_sighandler_t)1)  /* ignore signal */
#define HAIKU_SIG_ERR ((__haiku_sighandler_t)-1) /* an error occurred during signal \
                                        processing */
#define HAIKU_SIG_HOLD ((__haiku_sighandler_t)3) /* the signal was hold */

/* macros specifying the event notification type (sigevent::sigev_notify) */
#define HAIKU_SIGEV_NONE 0   /* no notification */
#define HAIKU_SIGEV_SIGNAL 1 /* notify via queued signal */
#define HAIKU_SIGEV_THREAD 2 /* notify via function called in new thread */

union haiku_sigval
{
    int sival_int;
    void *sival_ptr;
};

struct haiku_sigevent
{
    int sigev_notify;         /* notification type */
    int sigev_signo;          /* signal number */
    union haiku_sigval sigev_value; /* user-defined signal value */
    void (*sigev_notify_function)(union haiku_sigval);
    /* notification function in case of
       SIGEV_THREAD */
    void *sigev_notify_attributes; /* pthread_attr_t */
    /* pthread creation attributes in case of
       SIGEV_THREAD */
};

typedef struct __siginfo_t
{
    int si_signo;          /* signal number */
    int si_code;           /* signal code */
    int si_errno;          /* if non zero, an error number associated with
                              this signal */
    haiku_pid_t si_pid;    /* sending process ID */
    haiku_uid_t si_uid;    /* real user ID of sending process */
    void *si_addr;         /* address of faulting instruction */
    int si_status;         /* exit value or signal */
    long si_band;          /* band event for SIGPOLL */
    union haiku_sigval si_value; /* signal value */
} haiku_siginfo_t;

/* signal handler function types */
typedef void (*__haiku_sighandler_t)(int);
typedef void (*__haiku_siginfo_handler_t)(int, haiku_siginfo_t *, void *);

#ifdef __USE_GNU
typedef __haiku_sighandler_t sighandler_t;
/* GNU-like signal handler typedef */
#endif

/* structure used by sigaction() */
struct haiku_sigaction
{
    union
    {
        __haiku_sighandler_t sa_handler;
        __haiku_siginfo_handler_t sa_sigaction;
    };
    haiku_sigset_t sa_mask;
    int sa_flags;
    void *sa_userdata; /* will be passed to the signal
                          handler, BeOS extension */
};

/* values for sa_flags */
#define HAIKU_SA_NOCLDSTOP 0x01
#define HAIKU_SA_NOCLDWAIT 0x02
#define HAIKU_SA_RESETHAND 0x04
#define HAIKU_SA_NODEFER 0x08
#define HAIKU_SA_RESTART 0x10
#define HAIKU_SA_ONSTACK 0x20
#define HAIKU_SA_SIGINFO 0x40
#define HAIKU_SA_NOMASK SA_NODEFER
#define HAIKU_SA_STACK SA_ONSTACK
#define HAIKU_SA_ONESHOT SA_RESETHAND

/* values for ss_flags */
#define HAIKU_SS_ONSTACK 0x1
#define HAIKU_SS_DISABLE 0x2

#define HAIKU_MINSIGSTKSZ 8192
#define HAIKU_SIGSTKSZ 16384

/* for signals using an alternate stack */
typedef struct haiku_stack_t
{
    void *ss_sp;
    size_t ss_size;
    int ss_flags;
} haiku_stack_t;

/* for the 'how' arg of sigprocmask() */
#define HAIKU_SIG_BLOCK 1
#define HAIKU_SIG_UNBLOCK 2
#define HAIKU_SIG_SETMASK 3

/*
 * The list of all defined signals:
 *
 * The numbering of signals for Haiku attempts to maintain
 * some consistency with UN*X conventions so that things
 * like "kill -9" do what you expect.
 */
#define HAIKU_SIGHUP 1        /* hangup -- tty is gone! */
#define HAIKU_SIGINT 2        /* interrupt */
#define HAIKU_SIGQUIT 3       /* `quit' special character typed in tty  */
#define HAIKU_SIGILL 4        /* illegal instruction */
#define HAIKU_SIGCHLD 5       /* child process exited */
#define HAIKU_SIGABRT 6       /* abort() called, dont' catch */
#define HAIKU_SIGPIPE 7       /* write to a pipe w/no readers */
#define HAIKU_SIGFPE 8        /* floating point exception */
#define HAIKU_SIGKILL 9       /* kill a team (not catchable) */
#define HAIKU_SIGSTOP 10      /* suspend a thread (not catchable) */
#define HAIKU_SIGSEGV 11      /* segmentation violation (read: invalid pointer) */
#define HAIKU_SIGCONT 12      /* continue execution if suspended */
#define HAIKU_SIGTSTP 13      /* `stop' special character typed in tty */
#define HAIKU_SIGALRM 14      /* an alarm has gone off (see alarm()) */
#define HAIKU_SIGTERM 15      /* termination requested */
#define HAIKU_SIGTTIN 16      /* read of tty from bg process */
#define HAIKU_SIGTTOU 17      /* write to tty from bg process */
#define HAIKU_SIGUSR1 18      /* app defined signal 1 */
#define HAIKU_SIGUSR2 19      /* app defined signal 2 */
#define HAIKU_SIGWINCH 20     /* tty window size changed */
#define HAIKU_SIGKILLTHR 21   /* be specific: kill just the thread, not team */
#define HAIKU_SIGTRAP 22      /* Trace/breakpoint trap */
#define HAIKU_SIGPOLL 23      /* Pollable event */
#define HAIKU_SIGPROF 24      /* Profiling timer expired */
#define HAIKU_SIGSYS 25       /* Bad system call */
#define HAIKU_SIGURG 26       /* High bandwidth data is available at socket */
#define HAIKU_SIGVTALRM 27    /* Virtual timer expired */
#define HAIKU_SIGXCPU 28      /* CPU time limit exceeded */
#define HAIKU_SIGXFSZ 29      /* File size limit exceeded */
#define HAIKU_SIGBUS 30       /* access to undefined portion of a memory object */
#define HAIKU_SIGRESERVED1 31 /* reserved for future use */
#define HAIKU_SIGRESERVED2 32 /* reserved for future use */

#define HAIKU_SIGRTMIN (__signal_get_sigrtmin())
/* lowest realtime signal number */
#define HAIKU_SIGRTMAX (__signal_get_sigrtmax())
/* greatest realtime signal number */

#define __HAIKU_MAX_SIGNO 64 /* greatest possible signal number, can be used (+1) \
                          as size of static arrays */
#define HAIKU_NSIG (__HAIKU_MAX_SIGNO + 1)
/* BSD extension, size of the sys_siglist table,
   obsolete */

/* Signal code values appropriate for siginfo_t::si_code: */
/* any signal */
#define HAIKU_SI_USER 0    /* signal sent by user */
#define HAIKU_SI_QUEUE 1   /* signal sent by sigqueue() */
#define HAIKU_SI_TIMER 2   /* signal sent on timer_settime() timeout */
#define HAIKU_SI_ASYNCIO 3 /* signal sent on asynchronous I/O completion */
#define HAIKU_SI_MESGQ 4   /* signal sent on arrival of message on empty \
                        message queue */
/* SIGILL */
#define HAIKU_ILL_ILLOPC 10 /* illegal opcode */
#define HAIKU_ILL_ILLOPN 11 /* illegal operand */
#define HAIKU_ILL_ILLADR 12 /* illegal addressing mode */
#define HAIKU_ILL_ILLTRP 13 /* illegal trap */
#define HAIKU_ILL_PRVOPC 14 /* privileged opcode */
#define HAIKU_ILL_PRVREG 15 /* privileged register */
#define HAIKU_ILL_COPROC 16 /* coprocessor error */
#define HAIKU_ILL_BADSTK 17 /* internal stack error */
/* SIGFPE */
#define HAIKU_FPE_INTDIV 20 /* integer division by zero */
#define HAIKU_FPE_INTOVF 21 /* integer overflow */
#define HAIKU_FPE_FLTDIV 22 /* floating-point division by zero */
#define HAIKU_FPE_FLTOVF 23 /* floating-point overflow */
#define HAIKU_FPE_FLTUND 24 /* floating-point underflow */
#define HAIKU_FPE_FLTRES 25 /* floating-point inexact result */
#define HAIKU_FPE_FLTINV 26 /* invalid floating-point operation */
#define HAIKU_FPE_FLTSUB 27 /* subscript out of range */
/* SIGSEGV */
#define HAIKU_SEGV_MAPERR 30 /* address not mapped to object */
#define HAIKU_SEGV_ACCERR 31 /* invalid permissions for mapped object */
/* SIGBUS */
#define HAIKU_BUS_ADRALN 40 /* invalid address alignment */
#define HAIKU_BUS_ADRERR 41 /* nonexistent physical address */
#define HAIKU_BUS_OBJERR 42 /* object-specific hardware error */
/* SIGTRAP */
#define HAIKU_TRAP_BRKPT 50 /* process breakpoint */
#define HAIKU_TRAP_TRACE 51 /* process trace trap. */
/* SIGCHLD */
#define HAIKU_CLD_EXITED 60    /* child exited */
#define HAIKU_CLD_KILLED 61    /* child terminated abnormally without core dump */
#define HAIKU_CLD_DUMPED 62    /* child terminated abnormally with core dump */
#define HAIKU_CLD_TRAPPED 63   /* traced child trapped */
#define HAIKU_CLD_STOPPED 64   /* child stopped */
#define HAIKU_CLD_CONTINUED 65 /* stopped child continued */
/* SIGPOLL */
#define HAIKU_POLL_IN 70  /* input available */
#define HAIKU_POLL_OUT 71 /* output available */
#define HAIKU_POLL_MSG 72 /* input message available */
#define HAIKU_POLL_ERR 73 /* I/O error */
#define HAIKU_POLL_PRI 74 /* high priority input available */
#define HAIKU_POLL_HUP 75 /* device disconnected */

#include <arch_signal.h>

typedef struct vregs vregs;

typedef decltype(haiku_sigaction::sa_handler) haiku_sighandler_t;
typedef decltype(haiku_sigaction::sa_sigaction) haiku_sigaction_t;

// Crap from sys/signal.h is interfering our Haiku stuff.
inline auto haiku_sigaction_get_sa_sigaction(const struct haiku_sigaction &sa)
{
    return sa.sa_sigaction;
}
inline auto haiku_sigaction_get_sa_handler(const struct haiku_sigaction &sa)
{
    return sa.sa_handler;
}
inline auto haiku_sigaction_set_sa_sigaction(struct haiku_sigaction &sa, haiku_sigaction_t sa_sigaction)
{
    sa.sa_sigaction = sa_sigaction;
}
inline auto haiku_sigaction_set_sa_handler(struct haiku_sigaction &sa, haiku_sighandler_t sa_handler)
{
    sa.sa_handler = sa_handler;
}

#define DECLARE_HAIKU_SIGINFO_SET(name)                 \
    inline auto haiku_siginfo_set_##name                \
        (haiku_siginfo_t &si, decltype(si.name) name)   \
    {                                                   \
        si.name = name;                                 \
    }

DECLARE_HAIKU_SIGINFO_SET(si_pid)
DECLARE_HAIKU_SIGINFO_SET(si_uid)
DECLARE_HAIKU_SIGINFO_SET(si_addr)
DECLARE_HAIKU_SIGINFO_SET(si_status)
DECLARE_HAIKU_SIGINFO_SET(si_band)
DECLARE_HAIKU_SIGINFO_SET(si_value)

#endif /* _SIGNAL_H_ */