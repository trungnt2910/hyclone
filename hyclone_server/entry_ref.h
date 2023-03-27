#ifndef __HYCLONE_ENTRY_REF_H__
#define __HYCLONE_ENTRY_REF_H__

#include <bit>
#include <bitset>
#include <cstdint>
#include <string>
#include <utility>

class EntryRef
{
private:
    uint64_t _device;
    uint64_t _inode;
public:
    EntryRef(uint64_t device, uint64_t inode): _device(device), _inode(inode) {}
    ~EntryRef() = default;

    uint64_t GetDevice() const { return _device; }
    uint64_t GetInode() const { return _inode; }

    bool operator==(const EntryRef& other) const = default;
};

namespace std
{
    template<>
    struct hash<EntryRef>
    {
        std::size_t operator()(const EntryRef& ref) const
        {
            using bitset_t = std::bitset<sizeof(EntryRef) * __CHAR_BIT__>;
            return std::hash<bitset_t>()(std::bit_cast<bitset_t, EntryRef>(ref));
        }
    };
}

#endif // __HYCLONE_ENTRY_REF_H__