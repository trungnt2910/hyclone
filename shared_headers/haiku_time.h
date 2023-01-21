/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_TIME_H__
#define __HAIKU_TIME_H__

#include "BeDefs.h"

#define HAIKU_CLOCKS_PER_SEC 1000000
#define HAIKU_CLK_TCK HAIKU_CLOCKS_PER_SEC
#define HAIKU_TIME_UTC 1

#define HAIKU_MAX_TIMESTR 70
/* maximum length of a string returned by asctime(), and ctime() */

#define HAIKU_CLOCK_MONOTONIC ((haiku_clockid_t)0)
/* system-wide monotonic clock (aka system time) */
#define HAIKU_CLOCK_REALTIME ((haiku_clockid_t)-1)
/* system-wide real time clock */
#define HAIKU_CLOCK_PROCESS_CPUTIME_ID ((haiku_clockid_t)-2)
/* clock measuring the used CPU time of the current process */
#define HAIKU_CLOCK_THREAD_CPUTIME_ID ((haiku_clockid_t)-3)
/* clock measuring the used CPU time of the current thread */

#define HAIKU_TIMER_ABSTIME 1 /* absolute timer flag */

struct haiku_timespec
{
    haiku_time_t tv_sec; /* seconds */
    long tv_nsec;        /* and nanoseconds */
};

struct haiku_itimerspec
{
    struct haiku_timespec it_interval;
    struct haiku_timespec it_value;
};

struct haiku_tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;   /* day of month (1 to 31) */
    int tm_mon;    /* months since January (0 to 11) */
    int tm_year;   /* years since 1900 */
    int tm_wday;   /* days since Sunday (0 to 6, Sunday = 0, ...) */
    int tm_yday;   /* days since January 1 (0 to 365) */
    int tm_isdst;  /* daylight savings time (0 == no, >0 == yes, <0 == has to be calculated */
    int tm_gmtoff; /* timezone offset to GMT */
    char *tm_zone; /* timezone name */
};

struct haiku_timeval
{
    haiku_time_t tv_sec;       /* seconds */
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
#define haiku_timercmp(a, b, cmp) ((a)->tv_sec == (b)->tv_sec         \
                                       ? (a)->tv_usec cmp(b)->tv_usec \
                                       : (a)->tv_sec cmp(b)->tv_sec)

#endif /* __HAIKU_TIME_H__ */