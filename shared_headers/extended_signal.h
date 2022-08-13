#ifndef __HYCLONE_EXTENDED_SIGNALS_H__
#define __HYCLONE_EXTENDED_SIGNALS_H__

#include "extended_commpage.h"

#if defined(HYCLONE_HOST_LINUX)
// On Linux, we reserve:
// 3 for unmappable Haiku signals (SIGKILLTHR, SIGRESERVED1, SIGRESERVED2)
// 2 for suspend_thread's implementation.
#define HYCLONE_RESERVED_REALTIME_SIGNALS 5
#else
#warning "Define the number of reserved signal for this host."
#define HYCLONE_RESERVED_REALTIME_SIGNALS 0
#endif

#define HYCLONE_AVAILABLE_REALTIME_SIGNALS (GET_HOSTCALLS()->get_sigrtmax() - GET_HOSTCALLS()->get_sigrtmin() - HYCLONE_RESERVED_REALTIME_SIGNALS + 1)
#define HYCLONE_MIN_AVAILABLE_REALTIME_SIGNAL (GET_HOSTCALLS()->get_sigrtmin() + HYCLONE_RESERVED_REALTIME_SIGNALS)

#endif // __HYCLONE_EXTENDED_SIGNALS_H__
