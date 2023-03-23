/*
 * Copyright 2008-2015 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_SEMAPHORE_H__
#define __HAIKU_SEMAPHORE_H__

#include <cstdint>

typedef struct _haiku_sem_t
{
    int32_t type;
    union
    {
        int32_t named_sem_id;
        int32_t unnamed_sem;
    } u;
    int32_t padding[2];
} haiku_sem_t;

#define HAIKU_SEM_FAILED ((haiku_sem_t*)(long)-1)

#endif    /* __HAIKU_SEMAPHORE_H__ */