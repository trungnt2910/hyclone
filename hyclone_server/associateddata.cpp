#include <algorithm>

#include "associateddata.h"

bool AssociatedDataOwner::AddData(const std::shared_ptr<AssociatedData>& data)
{
    auto locker = std::unique_lock(_lock);

    if (data->Owner() != NULL)
    {
        return false;
    }

    _list.push_back(data);
    data->SetOwner(this);

    return true;
}

bool AssociatedDataOwner::RemoveData(const std::shared_ptr<AssociatedData>& data)
{
    auto locker = std::unique_lock(_lock);

    if (data->Owner() != this)
    {
        return false;
    }

    data->SetOwner(NULL);

    auto it = std::find(_list.begin(), _list.end(), data);
    _list.erase(it);

    return true;
}

void AssociatedDataOwner::PrepareForDeletion()
{
    {
        auto locker = std::unique_lock(_lock);

        // move all data to a temporary list and unset the owner
        std::list<std::shared_ptr<AssociatedData>> list = std::move(_list);

        for (auto& data : list)
        {
            data->SetOwner(NULL);
        }
    }

    // call the notification hooks and release our references
    for (auto& data : _list)
    {
        data->OwnerDeleted(this);
        data.reset();
    }
}