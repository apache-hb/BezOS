#pragma once

#include "log.hpp"
#include "panic.hpp"

#include "std/string.hpp"
#include "std/string_view.hpp"

#include "std/vector.hpp"
#include "vfs_types.hpp"

#include <new>
#include <utility>

namespace km {
    class VfsNode;
    class VfsPath;

    class VfsPathIterator {
        const VfsPath *mPath;
        uint16_t mSegment;
        uint16_t mOffset;

        stdx::StringView segment() const;

    public:
        VfsPathIterator(const VfsPath *path, uint16_t segment, uint16_t offset)
            : mPath(path)
            , mSegment(segment)
            , mOffset(offset)
        { }

        bool operator!=(const VfsPathIterator& other) const;

        VfsPathIterator& operator++();
        stdx::StringView operator*() const;
    };

    class VfsPath {
        stdx::String mPath;
        uint16_t mSegments;

    public:
        static constexpr char kSeperator = '/';

        VfsPath() = default;
        
        VfsPath(stdx::StringView path) 
            : VfsPath(stdx::String(path)) 
        { }

        VfsPath(stdx::String path) 
            : mPath(std::move(path)) 
            , mSegments(std::count_if(mPath.begin(), mPath.end(), [](char c) { return c == kSeperator; }))
        { 
            KM_CHECK(!mPath.isEmpty(), "Path cannot be empty.");
            KM_CHECK(mPath.front() == kSeperator, "Path must start with '/'.");
            
            if (mPath.count() > 1)
                KM_CHECK(mPath.back() != kSeperator, "Path must not end with '/'.");
        }

        stdx::StringView path() const { return mPath; }
        uint16_t segments() const { return mSegments; }

        VfsPathIterator begin() const;
        VfsPathIterator end() const;

        VfsPath parent() const;
        stdx::StringView name() const;
    };

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

    class VirtualFileSystem {
        struct VfsCacheEntry {
            VfsNode *node;
            uint32_t refCount;
        };

        BTreeMap<VfsNodeId, VfsCacheEntry> mNodeCache;

        VfsFolder mRoot;
        VfsNodeId mNextId = VfsNodeId(1);

        VfsNodeId allocateId();
        VfsFolder *getParentFolder(const VfsPath& path);
        VfsFile *findFileInCache(VfsNodeId id);
        void cacheNode(VfsNodeId id, VfsNode *node);

    public:
        VirtualFileSystem() = default;

        VfsNodeId mkdir(const VfsPath& path);

        void rmdir(const VfsPath& path);

        VfsNodeId open(const VfsPath& path);

        void close(VfsNodeId id);

        size_t read(VfsNodeId id, void *buffer, size_t size);
        size_t write(VfsNodeId id, const void *buffer, size_t size);
    };
}
