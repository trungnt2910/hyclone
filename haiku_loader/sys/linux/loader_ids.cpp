#include <unistd.h>

#include "loader_ids.h"

int loader_get_pid()
{
    return getpid();
}

int loader_get_tid()
{
    return gettid();
}