#ifndef __HYCLONE_ASSOCIATED_DATA_H__
#define __HYCLONE_ASSOCIATED_DATA_H__

#include <list>
#include <memory>
#include <mutex>

class AssociatedDataOwner;

class AssociatedData : public std::enable_shared_from_this<AssociatedData>
{
private:
    AssociatedDataOwner* _owner = NULL;
public:
    AssociatedData() = default;
    virtual ~AssociatedData() = default;

    AssociatedDataOwner* Owner() const { return _owner; }
    void SetOwner(AssociatedDataOwner* owner) { _owner = owner; }

    virtual void OwnerDeleted(AssociatedDataOwner* owner) = 0;
};

class AssociatedDataOwner
{
private:
    std::mutex _lock;
    std::list<std::shared_ptr<AssociatedData>> _list;
public:
    AssociatedDataOwner() = default;
    ~AssociatedDataOwner() = default;

    bool AddData(const std::shared_ptr<AssociatedData>& data);
    bool RemoveData(const std::shared_ptr<AssociatedData>& data);

    void PrepareForDeletion();
};

#endif // __HYCLONE_ASSOCIATED_DATA_H__