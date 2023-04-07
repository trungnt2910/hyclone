#ifndef __SERVER_REQUESTS_H__
#define __SERVER_REQUESTS_H__

#include <atomic>
#include <cstddef>
#include <future>

#include "BeDefs.h"
#include "haiku_errors.h"
#include "requests.h"

class Request
{
protected:
    std::promise<intptr_t> _promise;
    std::shared_future<intptr_t> _future;
    std::atomic<bool> _hasValue = false;
    void* _data;
public:
    Request();
    virtual ~Request();
    virtual size_t Size() = 0;
    virtual status_t Relocate(void* address) { return B_OK; }
    virtual status_t Reply(intptr_t result);
    std::shared_future<intptr_t> Result();

    void* Data() { return _data; }
};

class TransferAreaRequest : public Request
{
    TransferAreaRequestArgs _args;
public:
    TransferAreaRequest(const TransferAreaRequestArgs& args) 
        : _args(args) { _data = &_args; }
    virtual size_t Size() override { return sizeof(TransferAreaRequestArgs); }
};

#endif // __SERVER_REQUESTS_H__