#pragma once

#include "std/string.hpp"
#include "std/string_view.hpp"

#include "std/vector.hpp"

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

    struct VfsFile {
        size_t offset;
        stdx::Vector2<uint8_t> data;
    };

    union VfsEntryData {
        ~VfsEntryData() { }

        VfsFolder folder;
        VfsFile file;
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
            case km::VfsEntryType::eFile:
                std::destroy_at(&mData.file);
                break;

            default:
                break;
            }
        }

        VfsEntry()
            : mType(VfsEntryType::eNone)
            , mData(VfsEntryData{})
            , mMount(nullptr)
        { }

        VfsEntry(VfsFile file, vfs::IFileSystemMount *mount)
            : mType(VfsEntryType::eFile)
            , mData(VfsEntryData{ .file = std::move(file) })
            , mMount(mount)
        { }

        VfsEntry(VfsFolder folder, vfs::IFileSystemMount *mount)
            : mType(VfsEntryType::eFolder)
            , mData(VfsEntryData{ .folder = std::move(folder) })
            , mMount(mount)
        { }

        VfsEntry(vfs::IFileSystemMount *mount)
            : mType(VfsEntryType::eMount)
            , mData(VfsEntryData{})
            , mMount(mount)
        { }

        VfsEntry(vfs::INode *node, VfsEntryType type)
            : mType(type)
            , mData(VfsEntryData{})
            , mNode(node)
            , mMount(node->owner())
        { }

        VfsEntryType type() const { return mType; }
        vfs::INode *node() const { return mNode; }
        vfs::IFileSystemMount *mount() const { return mMount; }

        VfsFolder& folder() { return mData.folder; }
        VfsFile& file() { return mData.file; }
    };
}
