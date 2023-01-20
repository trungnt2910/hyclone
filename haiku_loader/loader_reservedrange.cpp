#include <cassert>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>

#ifdef HYCLONE_DEBUG_RESERVED_RANGE
#include <iostream>
#include <sstream>
#endif

#include "loader_reservedrange.h"

struct RangeInfo
{
    // For easy arithmetic.
    uint8_t* address;
    size_t size;

    bool operator<(const RangeInfo& other) const
    {
        return (address != other.address) ? (address < other.address) : (size < other.size);
    }
};

static std::mutex sReservedRangeMutex;
static std::map<RangeInfo, std::set<RangeInfo>> sReservedRanges;

static void loader_reserved_range_debug();

void loader_lock_reserved_range_data()
{
    sReservedRangeMutex.lock();
}

void loader_unlock_reserved_range_data()
{
    sReservedRangeMutex.unlock();
}

void loader_register_reserved_range(void* address, size_t size)
{
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    loader_reserved_range_debug();
#endif

    RangeInfo info { (uint8_t*)address, size };
    auto result = sReservedRanges.insert(std::make_pair(info, std::set<RangeInfo>()));

    assert(result.second);
    auto it = result.first;
    assert((it == sReservedRanges.begin()) || (it->first.address >= (std::prev(it)->first.address + std::prev(it)->first.size)));
    assert((it == std::prev(sReservedRanges.end())) || (it->first.address + it->first.size <= (std::next(it)->first.address)));
}

void loader_unregister_reserved_range(void* address, size_t size)
{
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    loader_reserved_range_debug();
#endif

    assert(loader_is_in_reserved_range(address, size));
    auto it = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    --it;
    auto oldRange = it->first;
    auto oldRangeMappings = std::move(it->second);

    sReservedRanges.erase(it);

    if (oldRange.address == (uint8_t*)address && oldRange.size == size)
    {
        return;
    }

    auto oldRangeMappingsIt = oldRangeMappings.begin();

    if (oldRange.address < (uint8_t*)address)
    {
        auto newRange = RangeInfo { oldRange.address, (size_t)((uint8_t*)address - oldRange.address) };
        std::set<RangeInfo> newRangeMappings;
        while ((oldRangeMappingsIt != oldRangeMappings.end()) &&
               (oldRangeMappingsIt->address < newRange.address + newRange.size))
        {
            newRangeMappings.insert(RangeInfo
            {
                oldRangeMappingsIt->address,
                std::min(oldRangeMappingsIt->size, (size_t)(newRange.address + newRange.size - oldRangeMappingsIt->address))
            });
            ++oldRangeMappingsIt;
        }
        sReservedRanges.emplace(std::make_pair(newRange, std::move(newRangeMappings)));
    }
    if (oldRange.address + oldRange.size > (uint8_t*)address + size)
    {
        auto newRange = RangeInfo { (uint8_t*)address + size, oldRange.address + oldRange.size - (uint8_t*)address - size };
        std::set<RangeInfo> newRangeMappings;
        while ((oldRangeMappingsIt != oldRangeMappings.end()) &&
               (oldRangeMappingsIt->address + oldRangeMappingsIt->size <= newRange.address))
        {
            ++oldRangeMappingsIt;
        }
        while (oldRangeMappingsIt != oldRangeMappings.end())
        {
            auto newRangeMappingAddress = std::max(oldRangeMappingsIt->address, newRange.address);
            auto newRangeMappingSize = (std::size_t)
                (oldRangeMappingsIt->address + oldRangeMappingsIt->size - newRangeMappingAddress);
            newRangeMappings.insert(RangeInfo
            {
                newRangeMappingAddress,
                newRangeMappingSize
            });
        }
        sReservedRanges.emplace(std::make_pair(newRange, std::move(newRangeMappings)));
    }
}

void loader_map_reserved_range(void* address, size_t size)
{
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    loader_reserved_range_debug();
#endif

    assert(loader_reserved_range_longest_mappable_from(address, size) >= size);

    auto it = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    --it;

    auto& rangeMappings = it->second;

    RangeInfo newMapping = { (uint8_t*)address, size };

    if (rangeMappings.empty())
    {
        rangeMappings.insert(newMapping);
        return;
    }

    bool deleteNext = false;
    bool deletePrev = false;

    auto mappingsIt = rangeMappings.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (mappingsIt->address == (uint8_t*)address + size)
    {
        newMapping.size += mappingsIt->size;
        deleteNext = true;
    }
    if (mappingsIt != rangeMappings.begin())
    {
        --mappingsIt;
        if (mappingsIt->address + mappingsIt->size == (uint8_t*)address)
        {
            newMapping.address = mappingsIt->address;
            newMapping.size += mappingsIt->size;
            deletePrev = true;
        }
    }

    auto newMappingsIt = rangeMappings.insert(newMapping).first;
    // Avoid fragmentation.
    if (deleteNext)
    {
        rangeMappings.erase(std::next(newMappingsIt));
    }
    if (deletePrev)
    {
        rangeMappings.erase(std::prev(newMappingsIt));
    }
}

void loader_unmap_reserved_range(void* address, size_t size)
{
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    loader_reserved_range_debug();
    std::stringstream ss;
    ss << "unmap_reserved_range: " << address << " " << (void*)((uint8_t*)address + size) << std::endl;
    std::cerr << ss.str() << std::flush;
#endif

    assert(loader_is_in_reserved_range(address, size));
    auto it = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    --it;
    auto& rangeMappings = it->second;

    auto mappingsIt = rangeMappings.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });

    if (mappingsIt == rangeMappings.begin())
    {
        // The reserved range exists, but the current area is not mapped.
        // Similar to POSIX mmap, this should be a no-op.
        return;
    }

    assert(mappingsIt != rangeMappings.begin());
    --mappingsIt;

    auto oldRange = *mappingsIt;
    rangeMappings.erase(oldRange);

    if (oldRange.address < (uint8_t*)address)
    {
        auto newRange = RangeInfo { oldRange.address, (size_t)((uint8_t*)address - oldRange.address) };
        rangeMappings.insert(newRange);
    }
    if (oldRange.address + oldRange.size > (uint8_t*)address + size)
    {
        auto newRange =
            RangeInfo { (uint8_t*)address + size, oldRange.address + oldRange.size - (uint8_t*)address - size };
        rangeMappings.insert(newRange);
    }
}

bool loader_is_in_reserved_range(void* address, size_t size)
{
    auto it = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (it == sReservedRanges.begin())
    {
        return false;
    }

    --it;

    return (it->first.address <= (uint8_t*)address) && ((uint8_t*)address + size <= it->first.address + it->first.size);
}

bool loader_collides_with_reserved_range(void* address, size_t size)
{
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    RangeInfo collidingRange;
#endif

    bool collides = false;
    auto firstRangeThatStartsAfterThisRange = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (firstRangeThatStartsAfterThisRange != sReservedRanges.end())
    {
        if ((uint8_t*)address + size > firstRangeThatStartsAfterThisRange->first.address)
        {
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
            collidingRange = firstRangeThatStartsAfterThisRange->first;
#endif
            collides = true;
        }
    }

    if (firstRangeThatStartsAfterThisRange != sReservedRanges.begin())
    {
        auto rangeThatStartsBeforeOrAtThisRange = std::prev(firstRangeThatStartsAfterThisRange);
        if ((uint8_t*)address < rangeThatStartsBeforeOrAtThisRange->first.address + rangeThatStartsBeforeOrAtThisRange->first.size)
        {
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
            collidingRange = rangeThatStartsBeforeOrAtThisRange->first;
#endif
            collides = true;
        }
    }

#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    bool reallyCollides = false;
    for (const auto& range : sReservedRanges)
    {
        if (((range.first.address <= (uint8_t*)address) && (range.first.address + range.first.size > (uint8_t*)address))
            || (((uint8_t*)address <= range.first.address) && ((uint8_t*)address + size > range.first.address)))
        {
            reallyCollides = true;
            break;
        }
    }

    if (collides != reallyCollides)
    {
        std::stringstream ss;
        ss << "collides_with_reserved_range: " << address << " " << (void*)((uint8_t*)address + size) << std::endl;
        if (collides)
        {
            ss << "seems to collide with " << collidingRange.address << " " << (void*)(collidingRange.address + collidingRange.size);
            ss << " but it doesn't" << std::endl;
        }
        std::cerr << ss.str() << std::flush;

        loader_reserved_range_debug();
    }
    assert(collides == reallyCollides);
#endif

    return collides;
}

size_t loader_reserved_range_longest_mappable_from(void* address, size_t maxSize)
{
    // Only address needs to be a valid address in a reserved range.
    assert(loader_is_in_reserved_range(address, 0));

    auto it = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (it == sReservedRanges.begin())
    {
        return 0;
    }

    --it;

    const auto& rangeMappings = it->second;

    // Nothing. Map freely.
    if (rangeMappings.empty())
    {
        return std::min(maxSize, (size_t)(it->first.address + it->first.size - (uint8_t*)address));
    }

    // First range with higher address.
    auto mappingsIt = rangeMappings.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (mappingsIt == rangeMappings.begin())
    {
        return std::min(maxSize, (size_t)(mappingsIt->address - (uint8_t*)address));
    }
    --mappingsIt;

    auto lastMappedAddress = mappingsIt->address + mappingsIt->size;
    if (lastMappedAddress > (uint8_t*)address)
    {
        return 0;
    }

    ++mappingsIt;
    return std::min(maxSize, (size_t)(mappingsIt->address - (uint8_t*)address));
}

size_t loader_next_reserved_range(void* address, void** nextAddress)
{
    auto it = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (it == sReservedRanges.begin())
    {
        return 0;
    }

    --it;

    if (it->first.address + it->first.size <= (uint8_t*)address)
    {
        ++it;
    }

    *nextAddress = it->first.address;
    return it->first.size;
}

size_t loader_next_reserved_range_mapping(void* address, void** nextAddress)
{
    assert(loader_is_in_reserved_range(address, 0));

    auto it = sReservedRanges.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (it == sReservedRanges.begin())
    {
        return 0;
    }

    --it;

    const auto& rangeMappings = it->second;
    auto mappingsIt = rangeMappings.upper_bound(RangeInfo { (uint8_t*)address, SIZE_MAX });
    if (mappingsIt == rangeMappings.begin())
    {
        return 0;
    }

    --mappingsIt;

    if (mappingsIt->address + mappingsIt->size <= (uint8_t*)address)
    {
        ++mappingsIt;
    }

#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    loader_reserved_range_debug();
    std::stringstream ss;
    ss << "next_reserved_range_mapping(" << address << "): " << (void*)mappingsIt->address << " "
       << (void*)(mappingsIt->address + mappingsIt->size) << std::endl;

    std::cerr << ss.str() << std::flush;
#endif

    *nextAddress = mappingsIt->address;
    return mappingsIt->size;
}

static void loader_reserved_range_debug()
{
#ifdef HYCLONE_DEBUG_RESERVED_RANGE
    for (auto& range : sReservedRanges)
    {
        std::stringstream ss;
        ss << "Reserved range: " << (void*)range.first.address << " " << (void*)(range.first.address + range.first.size)
           << std::endl;

        for (auto& mapping : range.second)
        {
            ss << "\tMapping: " << (void*)mapping.address << " " << (void*)(mapping.address + mapping.size) << std::endl;
        }

        std::cerr << ss.str() << std::flush;
    }
#endif
}