#ifndef __HAIKU_STRUCTS_H__
#define __HAIKU_STRUCTS_H__

#include <cstdint>

typedef unsigned char       uint8;
typedef int                 int32;
typedef unsigned int        uint32;
typedef int64_t             int64;
typedef uint64_t            uint64;

typedef int32 status_t;
typedef int32 haiku_pid_t;

typedef int32 haiku_dev_t;
typedef int64 haiku_ino_t;
typedef int32 haiku_nlink_t;
typedef int32 haiku_blksize_t;
typedef int64 haiku_blkcnt_t;

typedef int32 area_id;
typedef int32 image_id;
typedef int32 port_id;
typedef int32 sem_id;
typedef int32 team_id;
typedef int32 thread_id;

typedef uint32 haiku_mode_t;
typedef uint32 haiku_gid_t;
typedef uint32 haiku_uid_t;

typedef uintptr_t addr_t;

typedef uint64 haiku_sigset_t;
typedef uint64 bigtime_t;

typedef int32 haiku_clock_t;
typedef int32 haiku_suseconds_t;
typedef uint32 haiku_useconds_t;

#if defined(__i386__) && !defined(__x86_64__)
typedef int32 haiku_time_t;
#else
typedef int64 haiku_time_t;
#endif

typedef int64 haiku_off_t;

typedef long signed int     haiku_ssize_t;

#define B_OS_NAME_LENGTH    32
#define B_FILE_NAME_LENGTH  256
#define B_PATH_NAME_LENGTH  1024
#define B_INFINITE_TIMEOUT  (9223372036854775807LL)

#ifdef __x86_64__
#define B_PAGE_SIZE         4096
#define B_HAIKU_64_BIT      1
#endif

struct user_space_program_args
{
    char            program_name[B_OS_NAME_LENGTH];
    char            program_path[B_PATH_NAME_LENGTH];
    port_id         error_port;
    uint32          error_token;
    int             arg_count;
    int             env_count;
    char            **args;
    char            **env;
    haiku_mode_t    umask;    // (mode_t)-1 means not set
    bool            disable_user_addons;
};

typedef struct
{
    bigtime_t   user_time;
    bigtime_t   kernel_time;
} team_usage_info;

enum
{
    B_TIMEOUT           = 0x8,    /* relative timeout */
    B_RELATIVE_TIMEOUT  = 0x8,    /* fails after a relative timeout
                                                with B_TIMED_OUT */
    B_ABSOLUTE_TIMEOUT  = 0x10,   /* fails after an absolute timeout
                                                with B_TIMED_OUT */

    /* experimental Haiku only API */
    B_TIMEOUT_REAL_TIME_BASE        = 0x40,
    B_ABSOLUTE_REAL_TIME_TIMEOUT    = B_ABSOLUTE_TIMEOUT
                                        | B_TIMEOUT_REAL_TIME_BASE
};

#endif // __HAIKU_STRUCTS_H__