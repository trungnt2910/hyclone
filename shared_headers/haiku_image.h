#ifndef __HAIKU_IMAGE_H__
#define __HAIKU_IMAGE_H__

#include <cstddef>
#include "BeDefs.h"

typedef enum
{
    B_APP_IMAGE = 1,
    B_LIBRARY_IMAGE,
    B_ADD_ON_IMAGE,
    B_SYSTEM_IMAGE
} haiku_image_type;

typedef struct
{
    image_id            id;
    haiku_image_type    type;
    int32               sequence;
    int32               init_order;
    void                (*init_routine)();
    void                (*term_routine)();
    haiku_dev_t         device;
    haiku_ino_t         node;
    char                name[B_PATH_NAME_LENGTH];
    void                *text;
    void                *data;
    int32               text_size;
    int32               data_size;

    /* Haiku R1 extensions */
    int32               api_version; /* the Haiku API version used by the image */
    int32               abi;         /* the Haiku ABI used by the image */
} haiku_image_info;

typedef struct haiku_extended_image_info
{
    haiku_image_info basic_info;
    ptrdiff_t text_delta;
    void *symbol_table;
    void *symbol_hash;
    void *string_table;
} haiku_extended_image_info;

#endif // __HAIKU_IMAGE_H__