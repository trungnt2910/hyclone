#include "server_requests.h"

Request::Request()
{
    _future = _promise.get_future().share();
}

Request::~Request()
{
    Reply(B_BAD_VALUE);
}

status_t Request::Reply(intptr_t result)
{
    if (_hasValue.exchange(true))
    {
        return B_BAD_VALUE;
    }
    _promise.set_value(result);
    return B_OK;
}

std::shared_future<intptr_t> Request::Result()
{
    return _future;
}