#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <libhpkg/Compat/ByteSource.h>
#include <gmpxx.h>

#include <hpkgvfs/Entry.h>

#include <iostream>

namespace HpkgVfs
{
    using namespace LibHpkg;
    using namespace LibHpkg::Compat;
    using namespace LibHpkg::Model;

    namespace Platform
    {
        extern void SetDateModifed(const std::filesystem::path& path, const std::filesystem::file_time_type& time);
        extern void SetDateAccess(const std::filesystem::path& path, const std::filesystem::file_time_type& time);
        extern void SetDateCreate(const std::filesystem::path& path, const std::filesystem::file_time_type& time);
        extern void SetOwner(const std::filesystem::path& path, const std::string& user, const std::string& group);
        extern void WriteExtendedAttributes(const std::filesystem::path& path, const std::vector<ExtendedAttribute>& attributes);
    }

    void EntryWriter::SetDateModifed(const std::filesystem::path& path, const std::filesystem::file_time_type& time)
    {
        return Platform::SetDateModifed(path, time);
    }

    void EntryWriter::SetDateAccess(const std::filesystem::path& path, const std::filesystem::file_time_type& time)
    {
        return Platform::SetDateAccess(path, time);
    }

    void EntryWriter::SetDateCreate(const std::filesystem::path& path, const std::filesystem::file_time_type& time)
    {
        return Platform::SetDateCreate(path, time);
    }

    void EntryWriter::SetOwner(const std::filesystem::path& path, const std::string& user, const std::string& group)
    {
        return Platform::SetOwner(path, user, group);
    }

    void EntryWriter::WriteExtendedAttributes(const std::filesystem::path& path, const std::vector<ExtendedAttribute>& attributes)
    {
        return Platform::WriteExtendedAttributes(path, attributes);
    }

    enum HpkgFileType
    {
        FILE = 0,
        DIRECTORY = 1,
        SYMLINK = 2
    };

    template <typename T>
    T GetValueFromChildAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const AttributeId& id,
        const std::shared_ptr<AttributeContext>& context,
        const T& defaultValue)
    {
        auto child = attribute->TryGetChildAttribute(id);
        if (child == nullptr)
        {
            return defaultValue;
        }
        return child->GetValue<T>(context);
    }

    static std::filesystem::file_type GetFileTypeFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const std::shared_ptr<AttributeContext>& context);

    static std::filesystem::perms GetFilePermissionsFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const std::shared_ptr<AttributeContext>& context,
        std::filesystem::perms defaultValue);

    static std::filesystem::file_time_type GetTimeFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const AttributeId& id,
        const AttributeId& idNanos,
        const std::shared_ptr<AttributeContext>& context);

    static std::vector<ExtendedAttribute> GetExtendedFileAttributesFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const std::shared_ptr<AttributeContext>& context);

    inline bool PermissionsWritable(std::filesystem::perms permissions)
    {
        return (permissions & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
    }

    Entry::Entry(const std::string& name,
            std::filesystem::file_type type,
            std::filesystem::perms permissions,
            bool shineThrough,
            const std::string& user,
            const std::string& group,
            const std::filesystem::file_time_type& access,
            const std::filesystem::file_time_type& modified,
            const std::filesystem::file_time_type& create,
            const std::string& packageName)
        : _name(name), _type(type), _permissions(permissions), _shineThrough(shineThrough),
        _user(user), _group(group), _access(access), _modified(modified), _create(create)
    {
        switch (_type)
        {
            case std::filesystem::file_type::regular:
                new (&_data) std::vector<uint8_t>();
            break;
            case std::filesystem::file_type::directory:
                new (&_children) std::unordered_map<std::string, std::vector<std::shared_ptr<Entry>>>();
            break;
            case std::filesystem::file_type::symlink:
                new (&_target) std::string();
            break;
        }
        bool writable = PermissionsWritable(_permissions);
        _owningPackages.emplace(packageName, writable);
        _writablePackages += writable;
    }

    Entry::Entry(const std::string& packageName,
            const std::shared_ptr<Attribute>& attribute,
            const std::shared_ptr<AttributeContext>& context,
            bool dropData)
    {
        if (attribute->GetAttributeId() != AttributeId::DIRECTORY_ENTRY)
        {
            throw std::invalid_argument("Needs to be a directory entry.");
        }

        _name = attribute->GetValue<std::string>(context);
        _type = GetFileTypeFromAttribute(attribute, context);

        std::filesystem::perms defaultPerms =
            std::filesystem::perms::owner_read |
            std::filesystem::perms::group_read |
            std::filesystem::perms::others_read;

        if (_type == std::filesystem::file_type::directory)
        {
            defaultPerms |=
                std::filesystem::perms::owner_exec |
                std::filesystem::perms::group_exec |
                std::filesystem::perms::others_exec;
        }

        _permissions = GetFilePermissionsFromAttribute(attribute, context, defaultPerms);
        _user = GetValueFromChildAttribute<std::string>(attribute, AttributeId::FILE_USER, context, "");
        _group = GetValueFromChildAttribute<std::string>(attribute, AttributeId::FILE_GROUP, context, "");
        _access = GetTimeFromAttribute(attribute, AttributeId::FILE_ATIME, AttributeId::FILE_ATIME_NANOS, context);
        _modified = GetTimeFromAttribute(attribute, AttributeId::FILE_MTIME, AttributeId::FILE_MTIME_NANOS, context);
        _create = GetTimeFromAttribute(attribute, AttributeId::FILE_CRTIME, AttributeId::FILE_CRTIM_NANOS, context);
        _extendedAttributes = GetExtendedFileAttributesFromAttribute(attribute, context);

        bool writable = PermissionsWritable(_permissions);
        _owningPackages.emplace(packageName, writable);
        _writablePackages += writable;

        if (!dropData)
        {
            switch (_type)
            {
                case std::filesystem::file_type::regular:
                    new (&_data) std::vector<uint8_t>(GetValueFromChildAttribute<std::shared_ptr<ByteSource>>
                        (attribute, AttributeId::DATA, context, ByteSource::Wrap({}))->Read());
                break;
                case std::filesystem::file_type::directory:
                {
                    new (&_children) std::unordered_map<std::string, std::vector<std::shared_ptr<Entry>>>();
                }
                break;
                case std::filesystem::file_type::symlink:
                    new (&_target) std::string(
                        GetValueFromChildAttribute<std::string>(attribute, AttributeId::SYMLINK_PATH, context, ""));
                break;
            }
            UnsetUpdateFlag();
        }
        else
        {
            switch (_type)
            {
                case std::filesystem::file_type::regular:
                    new (&_data) std::vector<uint8_t>();
                break;
                case std::filesystem::file_type::directory:
                    new (&_children) std::unordered_map<std::string, std::vector<std::shared_ptr<Entry>>>();
                break;
                case std::filesystem::file_type::symlink:
                    new (&_target) std::string();
                break;
            }
            _updated = true;
        }
    }

    Entry::Entry(Entry&& other)
    {
        _name = std::move(other._name);
        _type = std::move(other._type);
        _permissions = std::move(other._permissions);
        _shineThrough = std::move(other._shineThrough);
        _user = std::move(other._user);
        _group = std::move(other._group);
        _access = std::move(other._access);
        _modified = std::move(other._modified);
        _create = std::move(other._create);
        _extendedAttributes = std::move(other._extendedAttributes);
        _owningPackages = std::move(other._owningPackages);
        _updatedChildren = std::move(other._updatedChildren);

        switch (_type)
        {
            case std::filesystem::file_type::regular:
                new (&_data) std::vector<uint8_t>(std::move(other._data));
            break;
            case std::filesystem::file_type::directory:
                new (&_children)
                    std::unordered_map<std::string, std::vector<std::shared_ptr<Entry>>>(std::move(other._children));
            break;
            case std::filesystem::file_type::symlink:
                new (&_target) std::string(std::move(other._target));
            break;
        }
    }

    Entry::~Entry()
    {
        switch (_type)
        {
            case std::filesystem::file_type::regular:
                _data.~vector();
            break;
            case std::filesystem::file_type::directory:
                _children.~unordered_map();
            break;
            case std::filesystem::file_type::symlink:
                _target.~basic_string();
            break;
        }
    }

    Entry& Entry::operator=(Entry&& other)
    {
        if (this != &other)
        {
            this->~Entry();
            new (this) Entry(std::move(other));
        }
        return *this;
    }

    void Entry::AddChild(std::shared_ptr<Entry>& child)
    {
        if (_type != std::filesystem::file_type::directory)
        {
            throw std::invalid_argument("Parent needs to be a directory.");
        }

        if (child->_parent.lock())
        {
            throw std::invalid_argument("Child already has a parent.");
        }
        child->_parent = shared_from_this();
        child->_shineThrough = child->_shineThrough || _shineThrough;
        auto& vec = _children[child->_name];
        if (child->_type == std::filesystem::file_type::directory)
        {
            for (const auto& c: vec)
            {
                if (c->_type == std::filesystem::file_type::directory)
                {
                    c->Merge(child);
                    child = c;
                    return;
                }
            }
            vec.push_back(child);
        }
        else
        {
            for (const auto& c: vec)
            {
                if (*c->_owningPackages.begin() == *child->_owningPackages.begin())
                {
                    throw std::invalid_argument("Child already exists.");
                }
            }

            auto it = std::find_if(vec.begin(), vec.end(), [&](const std::shared_ptr<Entry>& entry)
            {
                return !entry->HasPrecedenceOver(child);
            });

            vec.insert(it, child);
        }

    }

    std::shared_ptr<Entry> Entry::GetChild(const std::string& name) const
    {
        if (_type != std::filesystem::file_type::directory)
        {
            throw std::invalid_argument("Parent needs to be a directory.");
        }

        if (name.find('/') != std::string::npos)
        {
            return GetChild(std::filesystem::path(name));
        }

        auto it = _children.find(name);
        if (it == _children.end())
        {
            return nullptr;
        }
        return it->second.front();
    }

    std::shared_ptr<Entry> Entry::GetChild(const std::filesystem::path& path) const
    {
        if (_type != std::filesystem::file_type::directory)
        {
            throw std::invalid_argument("Parent needs to be a directory.");
        }

        assert(path.is_relative());

        return GetChild(path.begin(), path.end());
    }

    std::shared_ptr<Entry> Entry::GetChild(
        const std::filesystem::path::const_iterator& begin,
        const std::filesystem::path::const_iterator& end) const
    {
        if (begin == end || begin->empty())
        {
            return std::const_pointer_cast<Entry>(shared_from_this());
        }

        if (_type != std::filesystem::file_type::directory)
        {
            throw std::invalid_argument("Parent needs to be a directory.");
        }

        if (begin->string() == ".")
        {
            return GetChild(std::next(begin), end);
        }

        if (begin->string() == "..")
        {
            auto parent = _parent.lock();
            if (!parent)
            {
                return GetChild(std::next(begin), end);
            }
            return parent->GetChild(std::next(begin), end);
        }

        auto it = _children.find(*begin);

        if (it == _children.end())
        {
            throw std::invalid_argument("Child does not exist.");
        }
        return it->second.front()->GetChild(std::next(begin), end);
    }

    std::vector<std::shared_ptr<Entry>> Entry::GetChildren() const
    {
        std::vector<std::shared_ptr<Entry>> result;
        if (_type != std::filesystem::file_type::directory)
        {
            return result;
        }
        for (const auto& kvp: _children)
        {
            result.push_back(kvp.second.front());
        }
        return result;
    }

    void Entry::RemoveChild(const std::string& name)
    {
        UnsetUpdateFlag();
        if (_type != std::filesystem::file_type::directory)
        {
            throw std::invalid_argument("Parent needs to be a directory.");
        }

        auto it = _children.find(name);
        if (it == _children.end())
        {
            throw std::invalid_argument("Child does not exist.");
        }

        _updatedChildren.insert(name);
        _deletedChildren.push_back(*(it->second.begin()));
        it->second.erase(it->second.begin());

        if (it->second.empty())
        {
            _children.erase(it);
        }
    }

    void Entry::RemoveChild(const std::string& name, const std::string& package)
    {
        UnsetUpdateFlag();
        if (_type != std::filesystem::file_type::directory)
        {
            throw std::invalid_argument("Parent needs to be a directory.");
        }

        auto it = _children.find(name);
        if (it == _children.end())
        {
            throw std::invalid_argument("Child does not exist.");
        }

        auto oldFront = it->second.front();

        auto it2 = std::find_if(it->second.begin(), it->second.end(), [&](const std::shared_ptr<Entry>& entry)
        {
            return entry->_owningPackages.contains(package);
        });
        if (it2 == it->second.end())
        {
            throw std::invalid_argument("Child does not exist.");
        }

        if ((*it2)->_type == std::filesystem::file_type::directory && !package.empty())
        {
            (*it2)->RemovePackage(package);
            if ((*it2)->_owningPackages.empty())
            {
                _deletedChildren.push_back(*it2);
                it->second.erase(it2);
            }
        }
        else
        {
            _deletedChildren.push_back(*it2);
            it->second.erase(it2);
        }

        if (it->second.empty() || it->second.front() != oldFront)
        {
            _updatedChildren.insert(name);
        }

        if (it->second.empty())
        {
            _children.erase(it);
        }
    }

    void Entry::RemovePackage(const std::string& package)
    {
        if (package.empty())
        {
            throw std::invalid_argument("Package name cannot be empty.");
        }

        auto ownerIt = _owningPackages.find(package);

        if (ownerIt != _owningPackages.end())
        {
            UnsetUpdateFlag();

            if (ownerIt->second)
            {
                --_writablePackages;
                if (_writablePackages == 0)
                {
                    _permissions &= ~std::filesystem::perms::owner_write;
                }
            }

            _owningPackages.erase(ownerIt);

            if (_type == std::filesystem::file_type::directory)
            {
                for (auto it = _children.begin(); it != _children.end();)
                {
                    auto& kvp = *it;

                    auto oldFront = kvp.second.front();

                    std::vector<decltype(kvp.second.begin())> childToRemove;

                    for (auto it2 = kvp.second.begin(); it2 != kvp.second.end(); ++it2)
                    {
                        auto& child = *it2;
                        child->RemovePackage(package);
                        if (child->_owningPackages.empty())
                        {
                            childToRemove.push_back(it2);
                        }
                    }

                    for (const auto& it2: childToRemove)
                    {
                        _deletedChildren.push_back(*it2);
                        kvp.second.erase(it2);
                    }

                    if (kvp.second.empty() || kvp.second.front() != oldFront)
                    {
                        _updatedChildren.insert(kvp.first);
                    }

                    if (kvp.second.empty())
                    {
                        auto temp = it;
                        ++it;
                        _children.erase(temp);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }
    }

    bool Entry::HasPrecedenceOver(const std::shared_ptr<Entry>& other) const
    {
        assert(_name == other->_name);

        if (_type == std::filesystem::file_type::directory)
        {
            if (other->_type == std::filesystem::file_type::directory)
            {
                // Behavior on Haiku.
                return
                    _extendedAttributes.size() != other->_extendedAttributes.size() ?
                    _extendedAttributes.size() > other->_extendedAttributes.size() :
                    _modified > other->_modified;
            }
            else
            {
                return _modified > other->_modified;
            }
        }

        return _modified > other->_modified;
    }

    void Entry::SetData(const std::vector<uint8_t>& data)
    {
        if (_type != std::filesystem::file_type::regular)
        {
            throw std::invalid_argument("Entry is not a regular file.");
        }
        UnsetUpdateFlag();
        _data = data;
    }

    void Entry::SetTarget(const std::string& target)
    {
        if (_type != std::filesystem::file_type::symlink)
        {
            throw std::invalid_argument("Entry is not a symbolic link.");
        }
        UnsetUpdateFlag();
        _target = target;
    }

    void Entry::UnsetUpdateFlag()
    {
        _updated = false;
        std::shared_ptr<Entry> parent = _parent.lock();
        while (parent && parent->_updated)
        {
            parent->_updated = false;
            parent = parent->_parent.lock();
        }
    }

    void Entry::Merge(const std::shared_ptr<Entry>& other)
    {
        if (_updated && !other->_updated)
        {
            UnsetUpdateFlag();
        }
        if (other.get() == this)
        {
            return;
        }

        if (_name != other->_name)
        {
            throw std::invalid_argument("Cannot merge entries with different names.");
        }

        if (!_shineThrough && other->_shineThrough)
        {
            throw std::invalid_argument("Cannot merge a shine-through entry to a read-only entry.");
        }

        if (_type == std::filesystem::file_type::directory &&
            other->_type == std::filesystem::file_type::directory)
        {
            for (auto& kvp: other->_children)
            {
                for (auto& child: kvp.second)
                {
                    // No parent.
                    child->_parent = std::shared_ptr<Entry>();
                    // Re-register this node as a parent.
                    AddChild(child);
                }
            }
            other->_children.clear();

            _updatedChildren.insert(other->_updatedChildren.begin(), other->_updatedChildren.end());
            _deletedChildren.insert(_deletedChildren.end(), other->_deletedChildren.begin(), other->_deletedChildren.end());

            _owningPackages.insert(other->_owningPackages.begin(), other->_owningPackages.end());

            auto parent = _parent.lock();
            while (parent)
            {
                bool anythingInserted = false;
                for (const auto& p : other->_owningPackages)
                {
                    anythingInserted = parent->_owningPackages.insert(p).second || anythingInserted;
                }
                if (!anythingInserted)
                {
                    break;
                }
                parent = parent->_parent.lock();
            }

            other->_owningPackages.clear();

            _permissions |= other->_permissions;

            // other now holds nothing.
            other->_updated = true;
        }

        if (_type != other->_type && other->HasPrecedenceOver(shared_from_this()))
        {
            *this = std::move(*other);
        }
    }

    void Entry::WriteToDisk(const std::filesystem::path& rootPath)
    {
        EntryWriter writer;
        WriteToDisk(rootPath, writer);
    }

    void Entry::WriteToDisk(const std::filesystem::path& rootPath, EntryWriter& writer)
    {
        if (_updated)
        {
            return;
        }

        if (!_shineThrough)
        {
            auto parent = _parent.lock();
            if (parent)
            {
                _shineThrough = _shineThrough || parent->_shineThrough;
            }
        }

        std::filesystem::path path = rootPath / _name;

        bool exists = std::filesystem::exists(std::filesystem::symlink_status(path));

        if (exists)
        {
            std::error_code e;
            std::filesystem::permissions(path,
                std::filesystem::perms::owner_write,
                std::filesystem::perm_options::add |
                std::filesystem::perm_options::nofollow,
                e);
            if (e.value() == (int)std::errc::operation_not_supported && _type == std::filesystem::file_type::symlink)
            {
                // On many systems (Linux), permissions are not supported for symlinks.
            }
            else if (e)
            {
                throw std::filesystem::filesystem_error(e.message(), e);
            }
        }

        if (_type == std::filesystem::file_type::directory)
        {
            std::filesystem::create_directories(path);

            for (const auto& c : _deletedChildren)
            {
                if (c->_type == std::filesystem::file_type::directory)
                {
                    c->WriteToDisk(path);
                }
            }

            for (const auto& name: _updatedChildren)
            {
                auto childPath = path / name;
                std::error_code _;
                auto oldStatus = std::filesystem::symlink_status(childPath, _);
                auto oldPerms = oldStatus.permissions();
                auto oldType = oldStatus.type();

                // Ignore writable files on shine-through directories.
                // This is the same behavior as on Haiku.
                // It also ensures that user setting files are not removed
                // during package upgrades.
                // Symlinks are always removed as many systems don't support
                // permissions for symlinks.
                if ((oldType == std::filesystem::file_type::symlink)
                    || !_shineThrough
                    || (oldPerms == std::filesystem::perms::unknown)
                    || (oldPerms & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
                {
                    std::filesystem::permissions(path,
                        std::filesystem::perms::owner_write,
                        std::filesystem::perm_options::add |
                        std::filesystem::perm_options::nofollow, _);
                    std::filesystem::remove(childPath, _);
                }
            }

            for (const auto& kvp: _children)
            {
                kvp.second.front()->WriteToDisk(path);
            }

            _updatedChildren.clear();
            _deletedChildren.clear();
        }
        else if (_type == std::filesystem::file_type::regular)
        {
            if (((_permissions & std::filesystem::perms::owner_write) !=
                std::filesystem::perms::none) && exists)
            {
                // Do nothing, keep the file.
            }
            else if (exists && _modified <= std::filesystem::last_write_time(path))
            {
                // Do nothing, keep the file.
            }
            else
            {
                // Delete the existing file and replace it.
                // If the file is read only, temporarily allow us to write on it.
                if (exists)
                {
                    std::error_code _;
                    std::filesystem::permissions(path,
                        std::filesystem::perms::owner_write,
                        std::filesystem::perm_options::add |
                        std::filesystem::perm_options::nofollow, _);
                    std::filesystem::remove(path);
                }
                std::ofstream fout(path, std::ios::binary);
                fout.write(reinterpret_cast<const char*>(_data.data()), _data.size());
                fout.close();
            }
        }
        else if (_type == std::filesystem::file_type::symlink)
        {
            std::filesystem::path targetPath = _target;
            std::error_code _;
            if (exists)
            {
                std::filesystem::permissions(path,
                    std::filesystem::perms::owner_write,
                    std::filesystem::perm_options::add |
                    std::filesystem::perm_options::nofollow, _);
                std::filesystem::remove(path);
            }
            if (std::filesystem::is_directory(targetPath, _))
            {
                std::filesystem::create_directory_symlink(targetPath, path);
            }
            else
            {
                std::filesystem::create_symlink(targetPath, path);
            }
        }

        if (_permissions != std::filesystem::perms::none)
        {
            std::filesystem::perm_options options = std::filesystem::perm_options::replace;
            if (_type == std::filesystem::file_type::symlink)
            {
                options |= std::filesystem::perm_options::nofollow;
            }
            auto effectivePermissions = _permissions;
            if (!_shineThrough)
            {
                effectivePermissions &= ~std::filesystem::perms::owner_write;
                effectivePermissions &= ~std::filesystem::perms::group_write;
                effectivePermissions &= ~std::filesystem::perms::others_write;
            }
            std::error_code e;
            std::filesystem::permissions(path, effectivePermissions, options, e);
            if (e.value() == (int)std::errc::operation_not_supported && _type == std::filesystem::file_type::symlink)
            {
            }
            else if (e)
            {
                throw std::filesystem::filesystem_error(e.message(), e);
            }
        }

        writer.SetDateModifed(path, _modified);
        writer.SetDateAccess(path, _access);
        writer.SetDateCreate(path, _create);

        writer.SetOwner(path, _user, _group);
        writer.WriteExtendedAttributes(path, _extendedAttributes);

        _updated = true;
    }

    void Entry::Drop(bool force)
    {
        if (!force && !_updated)
        {
            throw std::runtime_error("Cannot drop an entry (" + _name + ") that has not been updated to disk.");
        }

        if (_type == std::filesystem::file_type::directory)
        {
            for (auto& kvp: _children)
            {
                kvp.second.front()->Drop(force);
                for (auto it = std::next(kvp.second.begin()); it != kvp.second.end(); ++it)
                {
                    if (force)
                    {
                        (*it)->Drop(true);
                    }
                    // Can't drop non-primary child whose data hasn't been written to disk.
                    // This ensures that this child has data when the other package becomes
                    // primary.
                    else if ((*it)->_updated)
                    {
                        (*it)->Drop(false);
                    }
                }
            }
        }
        else if (_type == std::filesystem::file_type::regular)
        {
            _data.clear();
        }
        else if (_type == std::filesystem::file_type::symlink)
        {
            _target.clear();
        }

        _updated = true;
    }

    std::string Entry::ToString() const
    {
        std::string result;

        switch (_type)
        {
            case std::filesystem::file_type::regular:
                result = "FILE:";
            break;
            case std::filesystem::file_type::directory:
                result = "DIR :";
            break;
            case std::filesystem::file_type::symlink:
                result = "SYML:";
            break;
        }

        result += " ";

        const auto none = std::filesystem::perms::none;

        if ((_permissions & std::filesystem::perms::owner_exec) != none)
        {
            result += "x";
        }
        if ((_permissions & std::filesystem::perms::owner_write) != none)
        {
            result += "w";
        }
        if ((_permissions & std::filesystem::perms::owner_read) != none)
        {
            result += "r";
        }
        result += "-";
        if ((_permissions & std::filesystem::perms::group_exec) != none)
        {
            result += "x";
        }
        if ((_permissions & std::filesystem::perms::group_write) != none)
        {
            result += "w";
        }
        if ((_permissions & std::filesystem::perms::group_read) != none)
        {
            result += "r";
        }
        result += "-";
        if ((_permissions & std::filesystem::perms::others_exec) != none)
        {
            result += "x";
        }
        if ((_permissions & std::filesystem::perms::others_write) != none)
        {
            result += "w";
        }
        if ((_permissions & std::filesystem::perms::others_read) != none)
        {
            result += "r";
        }

        result += " " + _name;

        return result;
    }

    std::filesystem::file_type GetFileTypeFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const std::shared_ptr<AttributeContext>& context)
    {
        int type = GetValueFromChildAttribute<mpz_class>(attribute, AttributeId::FILE_TYPE, context, mpz_class(0))
            .get_si();
        switch (type)
        {
            case (int)HpkgFileType::FILE:
                return std::filesystem::file_type::regular;
            case (int)HpkgFileType::DIRECTORY:
                return std::filesystem::file_type::directory;
            case (int)HpkgFileType::SYMLINK:
                return std::filesystem::file_type::symlink;
            default:
                throw std::invalid_argument("Invalid file type.");
        }
    }

    std::filesystem::perms GetFilePermissionsFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const std::shared_ptr<AttributeContext>& context,
        std::filesystem::perms defaultPermissions)
    {
        int permissions = GetValueFromChildAttribute<mpz_class>(
            attribute,
            AttributeId::FILE_PERMISSIONS, context,
            mpz_class((int)defaultPermissions))
                .get_si();
        // cppreference says std::filesystem::perms are the same as POSIX permissions.
        return (std::filesystem::perms)permissions;
    }

    std::filesystem::file_time_type GetTimeFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const AttributeId& id,
        const AttributeId& idNanos,
        const std::shared_ptr<AttributeContext>& context)
    {
        mpz_class time = GetValueFromChildAttribute<mpz_class>(attribute, id, context, 0);
        mpz_class nanos = GetValueFromChildAttribute<mpz_class>(attribute, idNanos, context, 0);

        time = (time * 1000) + (nanos / 1000000);

        return std::filesystem::file_time_type(std::chrono::seconds(time.get_si())) + std::chrono::nanoseconds(nanos.get_si());
    }

    static std::vector<ExtendedAttribute> GetExtendedFileAttributesFromAttribute(
        const std::shared_ptr<Attribute>& attribute,
        const std::shared_ptr<AttributeContext>& context)
    {
        auto result = std::vector<ExtendedAttribute>();
        auto attributes = attribute->GetChildAttributes(AttributeId::FILE_ATTRIBUTE);
        for (const auto& fileAttribute: attributes)
        {
            auto entry = ExtendedAttribute();
            entry.Name = fileAttribute->GetValue<std::string>(context);
            entry.Type = GetValueFromChildAttribute<mpz_class>(fileAttribute, AttributeId::FILE_ATTRIBUTE_TYPE, context, mpz_class(0))
                .get_ui();
            entry.Data =
                GetValueFromChildAttribute<std::shared_ptr<ByteSource>>(fileAttribute, AttributeId::DATA, context, ByteSource::Wrap({}))
                    ->Read();
            result.emplace_back(std::move(entry));
        }
        return result;
    }

    std::shared_ptr<Entry> Entry::FromAttribute(
        const std::string& packageName,
        const std::shared_ptr<Attribute>& attribute,
        const std::shared_ptr<AttributeContext>& context,
        bool dropData)
    {
        // AddChild uses shared_from_this, which is not callable
        // in a constructor. Therefore, we can only populate child
        // entries after the entry has been constructed.
        Entry* raw_ptr;
        raw_ptr = new Entry(packageName, attribute, context, dropData);
        auto result = std::shared_ptr<Entry>(raw_ptr);
        auto childList = attribute->GetChildAttributes(AttributeId::DIRECTORY_ENTRY);

        for (const auto& child : childList)
        {
            auto ptr = Entry::FromAttribute(packageName, child, context, dropData);
            result->AddChild(ptr);
        }

        return result;
    }

    std::shared_ptr<Entry> Entry::CreateHaikuBootEntry(std::shared_ptr<Entry>* system)
    {
        std::shared_ptr<Entry> bootDir = std::make_shared<Entry>("boot",
            std::filesystem::file_type::directory,
            std::filesystem::perms::owner_all   |
            std::filesystem::perms::group_read  |
            std::filesystem::perms::group_exec  |
            std::filesystem::perms::others_exec);

        std::shared_ptr<Entry> systemDir = CreatePackageFsRootEntry("system");
        bootDir->AddChild(systemDir);

        if (system)
        {
            *system = systemDir;
        }

        return bootDir;
    }

    std::shared_ptr<Entry> Entry::CreatePackageFsRootEntry(const std::string& name)
    {
        const auto defaultWritableDirectoryPerms =
            std::filesystem::perms::owner_all   |
            std::filesystem::perms::group_read  |
            std::filesystem::perms::group_exec  |
            std::filesystem::perms::others_exec;

        std::shared_ptr<Entry> systemDir = std::make_shared<Entry>(name,
            std::filesystem::file_type::directory,
            defaultWritableDirectoryPerms & ~std::filesystem::perms::owner_write);

        std::shared_ptr<Entry> non_packagedDir = std::make_shared<Entry>("non-packaged",
            std::filesystem::file_type::directory,
            defaultWritableDirectoryPerms,
            true);

        std::shared_ptr<Entry> settingsDir = std::make_shared<Entry>("settings",
            std::filesystem::file_type::directory,
            defaultWritableDirectoryPerms,
            true);

        std::shared_ptr<Entry> cacheDir = std::make_shared<Entry>("cache",
            std::filesystem::file_type::directory,
            defaultWritableDirectoryPerms,
            true);

        std::shared_ptr<Entry> packagesDir = std::make_shared<Entry>("packages",
            std::filesystem::file_type::directory,
            defaultWritableDirectoryPerms,
            true);

        std::shared_ptr<Entry> varDir = std::make_shared<Entry>("var",
            std::filesystem::file_type::directory,
            defaultWritableDirectoryPerms,
            true);

        std::shared_ptr<Entry> hpkgvfsPackagesDir = std::make_shared<Entry>(".hpkgvfsPackages",
            std::filesystem::file_type::directory,
            defaultWritableDirectoryPerms & ~std::filesystem::perms::owner_write);

        systemDir->AddChild(non_packagedDir);
        systemDir->AddChild(settingsDir);
        systemDir->AddChild(cacheDir);
        systemDir->AddChild(packagesDir);
        systemDir->AddChild(varDir);
        systemDir->AddChild(hpkgvfsPackagesDir);

        return systemDir;
    }
}