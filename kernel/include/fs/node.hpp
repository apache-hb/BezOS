#pragma once

#include "std/string.hpp"
#include "std/string_view.hpp"

#include "fs/types.hpp"
#include "fs/mount.hpp"

#include <utility>

namespace km {
    class VfsEntry;

    struct VfsFolder {
        using Container = BTreeMap<stdx::String, std::unique_ptr<VfsEntry>, std::less<>>;
        using ConstIterator = Container::const_iterator;
        Container nodes;

        void insert(stdx::String name, std::unique_ptr<VfsEntry> node);

        ConstIterator find(stdx::StringView name) const {
            return nodes.find(name);
        }

        ConstIterator begin() const { return nodes.begin(); }
        ConstIterator end() const { return nodes.end(); }
    };

    union VfsEntryData {
        ~VfsEntryData() { }

        VfsFolder folder;
    };

    class VfsEntry {
        VfsEntryType mType;
        VfsEntryData mData;
        vfs::INode *mNode;
        vfs::IFileSystemMount *mMount;

    public:
        ~VfsEntry() {
            switch (mType) {
            case km::VfsEntryType::eFolder:
                std::destroy_at(&mData.folder);
                break;

            default:
                break;
            }
        }

        VfsEntry(vfs::IFileSystemMount *mount)
            : mType(VfsEntryType::eMount)
            , mData(VfsEntryData{})
            , mNode(mount->root())
            , mMount(mount)
        { }

        VfsEntry(vfs::INode *node)
            : mType(node->type())
            , mData(VfsEntryData{})
            , mNode(node)
            , mMount(node->owner())
        {
            if (mType == VfsEntryType::eFolder) {
                new (&mData.folder) VfsFolder();
            }
        }

        VfsEntryType type() const { return mType; }
        vfs::INode *node() const { return mNode; }
        vfs::IFileSystemMount *mount() const { return mMount; }

        VfsFolder& folder() { return mData.folder; }
    };
}
