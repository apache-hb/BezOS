#pragma once

#include "log.hpp"
#include "panic.hpp"
#include "std/shared.hpp"
#include "std/string.hpp"
#include "std/string_view.hpp"

#include "fs/types.hpp"
#include "fs/mount.hpp"

namespace km {
    class VfsEntry;

    struct VfsFolder {
        using Container = BTreeMap<stdx::String, std::unique_ptr<VfsEntry>, std::less<>>;
        using ConstIterator = Container::const_iterator;
        Container nodes;

        km::VfsEntry *insert(stdx::String name, std::unique_ptr<VfsEntry> node);

        ConstIterator find(stdx::StringView name) const {
            return nodes.find(name);
        }

        void erase(ConstIterator it) {
            nodes.erase(it);
        }

        ConstIterator begin() const { return nodes.begin(); }
        ConstIterator end() const { return nodes.end(); }
    };

    class VfsEntry {
        sm::SharedPtr<vfs::INode> mNode;
        VfsEntryType mType;
        std::unique_ptr<VfsFolder> mData;
        vfs::IFileSystemMount *mMount;

    public:
        VfsEntry(vfs::IFileSystemMount *mount)
            : mNode(mount->root())
            , mType(VfsEntryType::eMount)
            , mMount(mount)
        {
            mData = std::unique_ptr<VfsFolder>(new VfsFolder());
        }

        VfsEntry(vfs::INode *node)
            : mNode(node)
            , mType(mNode->type())
            , mMount(mNode->owner())
        {
            if (mType == VfsEntryType::eFolder || mType == VfsEntryType::eMount) {
                mData = std::unique_ptr<VfsFolder>(new VfsFolder());
            }
        }

        VfsEntryType type() const { return mType; }
        vfs::INode *node() const { return mNode.get(); }
        vfs::IFileSystemMount *mount() const { return mMount; }

        VfsFolder& folder() {
            if (mData == nullptr) {
                KmDebugMessage("Entry ", std::to_underlying(mType), " is not a folder.");
                KM_PANIC("Entry is not a folder.");
            }
            return *mData.get();
        }
    };
}
