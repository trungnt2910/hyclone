#include <signal.h>

#include "loader_signal.h"

int loader_get_sigrtmin()
{
    return SIGRTMIN;
}

int loader_get_sigrtmax()
{
    return SIGRTMAX;
}
