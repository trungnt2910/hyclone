#ifndef __HAIKU_AREA_H__
#define __HAIKU_AREA_H__

#include <cstddef>
#include "BeDefs.h"

/* area locking */
#define B_NO_LOCK 0
#define B_LAZY_LOCK 1
#define B_FULL_LOCK 2
#define B_CONTIGUOUS 3
#define B_LOMEM 4             /* B_CONTIGUOUS, < 16 MB physical address */
#define B_32_BIT_FULL_LOCK 5  /* B_FULL_LOCK, < 4 GB physical addresses */
#define B_32_BIT_CONTIGUOUS 6 /* B_CONTIGUOUS, < 4 GB physical address */

/* address spec for create_area(), and clone_area() */
#define B_ANY_ADDRESS 0
#define B_EXACT_ADDRESS 1
#define B_BASE_ADDRESS 2
#define B_CLONE_ADDRESS 3
#define B_ANY_KERNEL_ADDRESS 4
/* B_ANY_KERNEL_BLOCK_ADDRESS		5 */
#define B_RANDOMIZED_ANY_ADDRESS 6
#define B_RANDOMIZED_BASE_ADDRESS 7

/* area protection */
#define B_READ_AREA (1 << 0)
#define B_WRITE_AREA (1 << 1)
#define B_EXECUTE_AREA (1 << 2)
#define B_STACK_AREA (1 << 3)
/* "stack" protection is not available on most platforms - it's used
   to only commit memory as needed, and have guard pages at the
   bottom of the stack. */
#define B_CLONEABLE_AREA (1 << 8)

// mapping argument for several internal VM functions
enum
{
    REGION_NO_PRIVATE_MAP = 0,
    REGION_PRIVATE_MAP
};

typedef struct haiku_area_info
{
    area_id area;
    char name[B_OS_NAME_LENGTH];
    size_t size;
    uint32 lock;
    uint32 protection;
    team_id team;
    uint32 ram_size;
    uint32 copy_count;
    uint32 in_count;
    uint32 out_count;
    void *address;
} haiku_area_info;

#endif // __HAIKU_AREA_H__