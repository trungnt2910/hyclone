#ifndef __HYCLONE_AREA_H__
#define __HYCLONE_AREA_H__

#include "entry_ref.h"
#include "haiku_area.h"

class Area
{
    friend class System;
private:
    haiku_area_info _info;
    EntryRef _entryRef;
    size_t _offset = (size_t)-1;
public:
    Area(const haiku_area_info& info) : _info(info) {}
    ~Area() = default;

    const haiku_area_info& GetInfo() const { return _info; }
    haiku_area_info& GetInfo() { return _info; }

    int GetAreaId() const { return _info.area; }
    void* GetAddress() const { return _info.address; }
    size_t GetSize() const { return _info.size; }
};

#endif // __HYCLONE_AREA_H__