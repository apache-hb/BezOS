#pragma once

#include "std/string.hpp"
#include "std/string_view.hpp"

#include "std/vector.hpp"
#include "fs/types.hpp"

#include <utility>

namespace km {
    class VfsNode;

    struct VfsFolder {
        using Container = BTreeMap<stdx::String, std::unique_ptr<VfsNode>, std::less<>>;
        using ConstIterator = Container::const_iterator;
        Container nodes;

        void insert(stdx::String name, std::unique_ptr<VfsNode> node);

        ConstIterator find(stdx::StringView name) const {
            return nodes.find(name);
        }

        ConstIterator begin() const { return nodes.begin(); }
        ConstIterator end() const { return nodes.end(); }
    };

    struct VfsFile {
        VfsLocalNodeId nodeId;

        size_t offset;
        stdx::Vector2<uint8_t> data;
    };

    union VfsNodeData {
        ~VfsNodeData() { }

        VfsFolder folder;
        VfsFile file;
    };

    class VfsNode {
        VfsNodeId mId;
        VfsNodeType mType;
        VfsNodeData mData;

    public:
        ~VfsNode() {
            switch (mType) {
            case km::VfsNodeType::eFolder:
                std::destroy_at(&mData.folder);
                break;
            case km::VfsNodeType::eFile:
                std::destroy_at(&mData.file);
                break;

            default:
                break;
            }
        }

        VfsNode()
            : mId(VfsNodeId::eInvalid)
            , mType(VfsNodeType::eNone)
            , mData(VfsNodeData{})
        { }

        VfsNode(VfsNodeId id, VfsFile file)
            : mId(id)
            , mType(VfsNodeType::eFile)
            , mData(VfsNodeData{ .file = std::move(file) })
        { }

        VfsNode(VfsNodeId id, VfsFolder folder)
            : mId(id)
            , mType(VfsNodeType::eFolder)
            , mData(VfsNodeData{ .folder = std::move(folder) })
        { }

        VfsNodeId id() const { return mId; }
        VfsNodeType type() const { return mType; }

        VfsFolder& folder() { return mData.folder; }
        VfsFile& file() { return mData.file; }
    };
}
