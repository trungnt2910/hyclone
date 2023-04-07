#ifndef __HYCLONE_EXTENDED_SIGNALS_H__
#define __HYCLONE_EXTENDED_SIGNALS_H__

#include "extended_commpage.h"

#if defined(HYCLONE_HOST_LINUX)
// On Linux, we reserve:
// 3 for unmappable Haiku signals (SIGKILLTHR, SIGRESERVED1, SIGRESERVED2)
// 1 for handling requests from hyclone_server
#define HYCLONE_RESERVED_REALTIME_SIGNALS 4
#else
#warning "Define the number of reserved signal for this host."
#define HYCLONE_RESERVED_REALTIME_SIGNALS 0
#endif

#if defined(HYCLONE_MONIKA) || defined(HYCLONE_LIBROOT)
#ifdef SIGRTMAX
#undef SIGRTMAX
#endif
#define SIGRTMAX (GET_HOSTCALLS()->get_sigrtmax())
#ifdef SIGRTMIN
#undef SIGRTMIN
#endif
#define SIGRTMIN (GET_HOSTCALLS()->get_sigrtmin())
#endif

#define HYCLONE_AVAILABLE_REALTIME_SIGNALS (SIGRTMAX - SIGRTMIN - HYCLONE_RESERVED_REALTIME_SIGNALS + 1)
#define HYCLONE_MIN_AVAILABLE_REALTIME_SIGNAL (SIGRTMIN + HYCLONE_RESERVED_REALTIME_SIGNALS)

#if defined(HYCLONE_HOST_LINUX)
#define SIGKILLTHR      (SIGRTMIN + 0)
#define SIGRESERVED1    (SIGRTMIN + 1)
#define SIGRESERVED2    (SIGRTMIN + 2)
#define SIGREQUEST      (SIGRTMIN + 3)
#else
#warning "Define the reserved signals for this host."
#endif

#endif // __HYCLONE_EXTENDED_SIGNALS_H__
