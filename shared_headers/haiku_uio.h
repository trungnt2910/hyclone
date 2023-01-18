/*
 * Copyright 2002-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_SYS_UIO_H__
#define __HAIKU_SYS_UIO_H__

#include <cstddef>

#include "BeDefs.h"

typedef struct haiku_iovec
{
    void *iov_base;
    size_t iov_len;
} haiku_iovec;

#endif /* __HAIKU_SYS_UIO_H__ */