#ifndef __HYCLONE_AREA_H__
#define __HYCLONE_AREA_H__

#include <memory>
#include <vector>
#include <utility>

#include "entry_ref.h"
#include "haiku_area.h"

class Area
{
    friend class System;
private:
    haiku_area_info _info;
    EntryRef _entryRef;
    size_t _offset = (size_t)-1;
    uint32_t _mapping = REGION_NO_PRIVATE_MAP;
public:
    Area(const haiku_area_info& info) : _info(info) {}
    Area(const Area& other) = default;
    ~Area() = default;

    const haiku_area_info& GetInfo() const { return _info; }
    haiku_area_info& GetInfo() { return _info; }

    int GetAreaId() const { return _info.area; }
    void* GetAddress() const { return _info.address; }
    size_t GetSize() const { return _info.size; }
    const EntryRef& GetEntryRef() const { return _entryRef; }
    size_t GetOffset() const { return _offset; }
    uint32_t GetMapping() const { return _mapping; }

    void SetMapping(uint32_t mapping) { _mapping = mapping; }

    bool IsShared() const { return _offset != (size_t)-1; }
    void Share(const EntryRef& entryRef, size_t offset) { _entryRef = entryRef; _offset = offset; }
    void Unshare() { _entryRef = EntryRef(); _offset = (size_t)-1; }

    bool IsWritable() const { return _info.protection & B_WRITE_AREA; }

    // Split this area into multiple areas specified in ranges. The old area will be replaced with
    // the first area in the vector. The remaining areas will be returned in a vector.
    std::vector<std::shared_ptr<Area>> Split(const std::vector<std::pair<uint8_t*, uint8_t*>>& ranges);
};

#endif // __HYCLONE_AREA_H__