#include <set>
#include <vector>

#include "loader_idmap.h"

struct loader_idmap
{
    std::vector<void*> data;
    std::set<int> freeIds;
};

void* loader_idmap_create()
{
    return new loader_idmap;
}

void loader_idmap_destroy(void* idmap)
{
    delete static_cast<loader_idmap*>(idmap);
}

int loader_idmap_add(void* idmap, void* data)
{
    loader_idmap* map = static_cast<loader_idmap*>(idmap);
    int id = 0;
    if (map->freeIds.empty())
    {
        id = map->data.size();
        map->data.push_back(data);
    }
    else
    {
        id = *map->freeIds.begin();
        map->freeIds.erase(map->freeIds.begin());
        map->data[id] = data;
    }
    return id;
}

void* loader_idmap_get(void* idmap, int id)
{
    loader_idmap* map = static_cast<loader_idmap*>(idmap);
    if (id < 0 || id >= map->data.size())
    {
        return nullptr;
    }
    return map->data[id];
}

void loader_idmap_remove(void* idmap, int id)
{
    loader_idmap* map = static_cast<loader_idmap*>(idmap);
    if (id < 0 || id >= map->data.size())
    {
        return;
    }
    if (map->freeIds.contains(id))
    {
        return;
    }
    if (id == map->data.size() - 1)
    {
        map->data.pop_back();
    }
    else
    {
        map->freeIds.insert(id);
        map->data[id] = nullptr;
    }
}
