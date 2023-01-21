/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_TIME_H__
#define __HAIKU_TIME_H__

#include "BeDefs.h"

struct haiku_timeval
{
    haiku_time_t      tv_sec;  /* seconds */
    haiku_suseconds_t tv_usec; /* microseconds */
};

struct haiku_timezone
{
    int tz_minuteswest;
    int tz_dsttime;
};

struct haiku_itimerval
{
    struct haiku_timeval it_interval;
    struct haiku_timeval it_value;
};

#define HAIKU_ITIMER_REAL 1    /* real time */
#define HAIKU_ITIMER_VIRTUAL 2 /* process virtual time */
#define HAIKU_ITIMER_PROF 3    /* both */

/* BSDish macros operating on timeval structs */
#define haiku_timeradd(a, b, res)                     \
    do                                                \
    {                                                 \
        (res)->tv_sec = (a)->tv_sec + (b)->tv_sec;    \
        (res)->tv_usec = (a)->tv_usec + (b)->tv_usec; \
        if ((res)->tv_usec >= 1000000)                \
        {                                             \
            (res)->tv_usec -= 1000000;                \
            (res)->tv_sec++;                          \
        }                                             \
    } while (0)
#define haiku_timersub(a, b, res)                     \
    do                                                \
    {                                                 \
        (res)->tv_sec = (a)->tv_sec - (b)->tv_sec;    \
        (res)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((res)->tv_usec < 0)                       \
        {                                             \
            (res)->tv_usec += 1000000;                \
            (res)->tv_sec--;                          \
        }                                             \
    } while (0)
#define haiku_timerclear(a) ((a)->tv_sec = (a)->tv_usec = 0)
#define haiku_timerisset(a) ((a)->tv_sec != 0 || (a)->tv_usec != 0)
#define haiku_timercmp(a, b, cmp) ((a)->tv_sec == (b)->tv_sec   \
                                 ? (a)->tv_usec cmp(b)->tv_usec \
                                 : (a)->tv_sec cmp(b)->tv_sec)

#endif /* __HAIKU_TIME_H__ */