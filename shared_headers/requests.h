#ifndef __HYCLONE_REQUESTS_H__
#define __HYCLONE_REQUESTS_H__

#include "haiku_area.h"

enum request_id
{
    REQUEST_ID_transfer_area,
};

struct RequestArgs
{
    request_id id;
};

struct TransferAreaRequestArgs : public RequestArgs
{
    area_id transferredArea;
    area_id baseArea;
    uint32 addressSpec;
    void* address;
};

#endif // __HYCLONE_REQUESTS_H__