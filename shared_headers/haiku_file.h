/*
 * Copyright 2008-2012 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __HAIKU_FILE_H__
#define __HAIKU_FILE_H__

/* for use with flock() */
#define HAIKU_LOCK_SH 0x01 /* shared file lock */
#define HAIKU_LOCK_EX 0x02 /* exclusive file lock */
#define HAIKU_LOCK_NB 0x04 /* don't block when locking */
#define HAIKU_LOCK_UN 0x08 /* unlock file */

#endif /* __HAIKU_FILE_H__ */