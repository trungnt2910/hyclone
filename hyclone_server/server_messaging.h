#ifndef __SERVER_MESSAGE_H__
#define __SERVER_MESSAGE_H__

#include <cstddef>
#include <memory>
#include <mutex>

#include "BeDefs.h"

class Process;
class Semaphore;

// Adapted from https://xref.landonf.org/source/raw/haiku/src/system/kernel/messaging/MessagingService.h

typedef struct messaging_target
{
    port_id port;
    int32 token;
} messaging_target;

enum
{
    MESSAGING_COMMAND_SEND_MESSAGE = 0,
};

struct messaging_area_header
{
    std::atomic<int32> lock_counter;
    int32 size; // set to 0, when area is discarded
    area_id kernel_area;
    area_id next_kernel_area;
    int32 command_count;
    int32 first_command;
    int32 last_command;
};

struct messaging_command
{
    int32 next_command;
    uint32 command;
    int32 size; // == sizeof(messaging_command) + dataSize
    char data[0];
};

struct messaging_command_send_message
{
    int32 message_size;
    int32 target_count;
    messaging_target targets[0]; // [target_count]
    //char message[message_size];
};

// MessagingArea
class MessagingArea
{
private:
    MessagingArea() = default;

    messaging_command* _CheckCommand(int32 offset, int32& size);

    messaging_area_header* _header;
    area_id _id;
    int32 _size;
    std::weak_ptr<Semaphore> _lockSem;
    std::weak_ptr<Semaphore> _counterSem;
    std::shared_ptr<MessagingArea> _nextArea;

public:
    ~MessagingArea();

    static std::shared_ptr<MessagingArea> Create(const std::shared_ptr<Semaphore>& lockSem,
        const std::shared_ptr<Semaphore>& counterSem);

    static bool CheckCommandSize(int32 dataSize);

    void InitHeader();

    bool Lock();
    void Unlock();

    area_id GetId() const { return _id; }
    int32 GetSize() const { return _size; }
    bool IsEmpty() const { return _header->command_count == 0; }

    void* AllocateCommand(uint32 commandWhat, int32 dataSize, bool& wasEmpty);
    void CommitCommand();

    void SetNextArea(std::shared_ptr<MessagingArea> area) { _nextArea = area; }
    std::shared_ptr<MessagingArea> NextArea() const { return _nextArea; }
};

// MessagingService
class MessagingService
{
private:
    status_t _AllocateCommand(int32 commandWhat, int32 size,
                              MessagingArea*& area, void*& data, bool& wasEmpty);

    std::mutex _lock;
    std::shared_ptr<MessagingArea> _firstArea;
    std::shared_ptr<MessagingArea> _lastArea;
    std::weak_ptr<Process> _serverTeam;
    std::weak_ptr<Semaphore> _lockSem;
    std::weak_ptr<Semaphore> _counterSem;
public:
    MessagingService() = default;
    ~MessagingService() = default;

    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }

    status_t RegisterService(const std::shared_ptr<Process>& team,
        const std::shared_ptr<Semaphore>& lockSem,
        const std::shared_ptr<Semaphore>& counterSem,
        area_id& areaID);
    status_t UnregisterService(const std::shared_ptr<Process>& team);

    status_t SendMessage(const void* message, int32 messageSize,
                         const messaging_target* targets, int32 targetCount);
};

#endif // __SERVER_MESSAGE_H__