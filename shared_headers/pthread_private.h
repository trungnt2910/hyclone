#ifndef _PTHREAD_PRIVATE_H_
#define _PTHREAD_PRIVATE_H_

#include "BeDefs.h"

#define HAIKU_PTHREAD_KEYS_MAX              256

#define HAIKU_THREAD_DETACHED               0x01
#define HAIKU_THREAD_DEAD                   0x02
#define HAIKU_THREAD_CANCELED               0x04
#define HAIKU_THREAD_CANCEL_ENABLED         0x08
#define HAIKU_THREAD_CANCEL_ASYNCHRONOUS    0x10

struct haiku_pthread_cleanup_handler
{
    struct haiku_pthread_cleanup_handler *previous;
    void (*function)(void *argument);
    void *argument;
};

struct haiku_pthread_key_data
{
    int32 sequence;
    void  *value;
};

typedef struct haiku_pthread_thread
{
    thread_id id;
    int32 flags;
    void *(*entry)(void *);
    void *entry_argument;
    void *exit_value;
    struct haiku_pthread_key_data specific[HAIKU_PTHREAD_KEYS_MAX];
    struct haiku_pthread_cleanup_handler *cleanup_handlers;
} haiku_pthread_thread;

typedef struct haiku_pthread_thread *haiku_pthread_t;

#endif // _PTHREAD_PRIVATE_H_