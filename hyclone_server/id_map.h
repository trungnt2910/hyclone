#ifndef __HYCLONE_ID_MAP_H__
#define __HYCLONE_ID_MAP_H__

#include <cstddef>
#include <set>
#include <vector>

template<typename T, typename id_t>
class IdMapIter;

template <typename T, typename id_t = int>
class IdMap
{
    friend class IdMapIter<T, id_t>;
private:
    std::vector<T> _items;
    std::set<id_t> _freeIds;
public:
    IdMap() = default;
    ~IdMap() = default;

    id_t Add(const T& item)
    {
        if (_freeIds.empty())
        {
            _items.push_back(item);
            return (id_t)(_items.size() - 1);
        }
        else
        {
            id_t id = *_freeIds.begin();
            _freeIds.erase(id);
            _items[id] = item;
            return id;
        }
    }

    T& Get(id_t id)
    {
        return _items[id];
    }

    id_t MaxId() const
    {
        return (id_t)(_items.size() - 1);
    }

    id_t NextId(id_t id)
    {
        do
        {
            if (id >= MaxId())
            {
                return (id_t)-1;
            }
            ++id;
        }
        while (_freeIds.count(id));

        return id;
    }

    bool IsValidId(id_t id)
    {
        return id >= 0 && id <= MaxId() && !_freeIds.count(id);
    }

    void Remove(id_t id)
    {
        _freeIds.insert(id);
        _items[id] = T();
    }

    size_t Size() const
    {
        return _items.size() - _freeIds.size();
    }

    size_t size() const { return Size(); }
    IdMapIter<T, id_t> begin();
    IdMapIter<T, id_t> end();
};

template<typename T, typename id_t = int>
class IdMapIter
{
    friend class IdMap<T, id_t>;
private:
    std::vector<T>& _items;
    const std::set<id_t>& _freeIds;
    std::size_t _index;
    IdMapIter(IdMap<T, id_t>& map, std::size_t index) : _items(map._items), _freeIds(map._freeIds), _index(index) { }
public:
    using pointer         = T*;
    using difference_type = std::ptrdiff_t;

    IdMapIter(const IdMapIter& other) : _items(other._items), _freeIds(other._freeIds), _index(other._index) { };

    IdMapIter operator++(int)
    {
        IdMapIter tmp(*this);
        ++(*this);
        return tmp;
    }
    IdMapIter& operator++()
    {
        do
        {
            ++_index;
        }
        while (_freeIds.count(_index));
        return *this;
    }
    const T& operator*() const
    {
        return _items[_index];
    }
    const T& operator->() const
    {
        return _items[_index];
    }
    T& operator*()
    {
        return _items[_index];
    }
    T& operator->()
    {
        return _items[_index];
    }
    bool operator==(const IdMapIter& other) const
    {
        return _index == other._index;
    }
    bool operator!=(const IdMapIter& other) const
    {
        return _index != other._index;
    }
};

template <typename T, typename id_t>
inline IdMapIter<T, id_t> IdMap<T, id_t>::begin()
{
    return IdMapIter<T, id_t>(*this, 0);
}

template <typename T, typename id_t>
inline IdMapIter<T, id_t> IdMap<T, id_t>::end()
{
    return IdMapIter<T, id_t>(*this, _items.size());
}

#endif