#ifndef __HPKGVFS_ENTRY_H__
#define __HPKGVFS_ENTRY_H__

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <libhpkg/Model/Attribute.h>

namespace HpkgVfs
{
    struct ExtendedAttribute
    {
        std::string Name;
        uint32_t Type;
        std::vector<uint8_t> Data;
    };

    class EntryWriter
    {
    public:
        virtual void SetDateModifed(const std::filesystem::path& path, const std::filesystem::file_time_type& time);
        virtual void SetDateAccess(const std::filesystem::path& path, const std::filesystem::file_time_type& time);
        virtual void SetDateCreate(const std::filesystem::path& path, const std::filesystem::file_time_type& time);
        virtual void SetOwner(const std::filesystem::path& path, const std::string& user, const std::string& group);
        virtual void WriteExtendedAttributes(const std::filesystem::path& path, const std::vector<ExtendedAttribute>& attributes);
    };

    class Entry: public std::enable_shared_from_this<Entry>
    {
    private:
        std::string _name;
        union
        {
            std::unordered_map<std::string, std::vector<std::shared_ptr<Entry>>> _children;
            std::vector<uint8_t> _data;
            std::string _target;
        };
        std::weak_ptr<Entry> _parent;
        std::filesystem::file_type _type;
        std::filesystem::perms _permissions;
        std::string _user;
        std::string _group;
        std::filesystem::file_time_type _access;
        std::filesystem::file_time_type _modified;
        std::filesystem::file_time_type _create;
        std::vector<ExtendedAttribute> _extendedAttributes;
        std::unordered_map<std::string, bool> _owningPackages;
        // List of names where _children[name].front() has changed.
        std::unordered_set<std::string> _updatedChildren;
        // Children pending delete.
        std::vector<std::shared_ptr<Entry>> _deletedChildren;

        int _writablePackages = 0;
        bool _updated = false;
        bool _shineThrough = false;

        // This is intentionally made private.
        // See the comments at FromAttribute.
        Entry(const std::string& packageName,
            const std::shared_ptr<LibHpkg::Model::Attribute>& attribute,
            const std::shared_ptr<LibHpkg::AttributeContext>& context,
            bool dropData);
    protected:
        bool HasPrecedenceOver(const std::shared_ptr<Entry>& other) const;
    public:
        Entry(const std::string& name,
            std::filesystem::file_type type = std::filesystem::file_type::regular,
            std::filesystem::perms permissions = std::filesystem::perms::none,
            bool shineThrough = false,
            const std::string& user = "",
            const std::string& group = "",
            const std::filesystem::file_time_type& access = std::filesystem::file_time_type::min(),
            const std::filesystem::file_time_type& modified = std::filesystem::file_time_type::min(),
            const std::filesystem::file_time_type& create = std::filesystem::file_time_type::min(),
            // Empty package means this is a manually created entry.
            const std::string& packageName = "");
        Entry(Entry&& other);
        ~Entry();

        Entry& operator=(Entry&& other);

        void AddChild(std::shared_ptr<Entry>& child);
        std::vector<std::shared_ptr<Entry>> GetChildren() const;
        std::shared_ptr<Entry> GetChild(const std::string& name) const;
        std::shared_ptr<Entry> GetChild(const std::filesystem::path& path) const;
        std::shared_ptr<Entry> GetChild(const std::filesystem::path::const_iterator& begin,
            const std::filesystem::path::const_iterator& end) const;
        // Removes the active child.
        void RemoveChild(const std::string& name);
        // Removes the child from the specified packaged.
        // An empty string means a system created file (file without package).
        void RemoveChild(const std::string& name, const std::string& package);

        void RemovePackage(const std::string& package);

        void AddPermissions(std::filesystem::perms permission)
        {
            UnsetUpdateFlag();
            _permissions |= permission;
        }
        void RemovePermissions(std::filesystem::perms permission)
        {
            UnsetUpdateFlag();
            _permissions &= ~permission;
        }

        void SetData(const std::vector<uint8_t>& data);
        void SetTarget(const std::string& target);

        // Unsets the _updated flag for this node and parents.
        void UnsetUpdateFlag();

        // Moves all chilren of other to this.
        void Merge(const std::shared_ptr<Entry>& other);

        void WriteToDisk(const std::filesystem::path& rootPath);
        void WriteToDisk(const std::filesystem::path& rootPath, EntryWriter& writer);

        // Removes the data associated with this entry.
        void Drop(bool force = false);

        std::string ToString() const;

        static std::shared_ptr<Entry> FromAttribute(
            const std::string& packageName,
            const std::shared_ptr<LibHpkg::Model::Attribute>& attribute,
            const std::shared_ptr<LibHpkg::AttributeContext>& context,
            bool dropData = false);

        static std::shared_ptr<Entry> CreateHaikuBootEntry(std::shared_ptr<Entry>* system = nullptr);
        static std::shared_ptr<Entry> CreatePackageFsRootEntry(const std::string& name);
    };
}

#endif // __HPKGVFS_ENTRY_H__