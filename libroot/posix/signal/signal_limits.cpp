#include <algorithm>

#include "extended_signal.h"
#include "haiku_signal.h"
#include "signal_defs.h"

extern "C"
{

int
__signal_get_sigrtmin()
{
	return SIGNAL_REALTIME_MIN;
}


int
__signal_get_sigrtmax()
{
	return std::min(__HAIKU_MAX_SIGNO,
		SIGNAL_REALTIME_MIN + HYCLONE_AVAILABLE_REALTIME_SIGNALS - 1);
}

}
