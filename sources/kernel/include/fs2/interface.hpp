#pragma once

#include "fs2/node.hpp"

namespace vfs2 {
    class IFileHandle : public IHandle {
    public:
        virtual ~IFileHandle() = default;

        virtual OsStatus stat(NodeStat *stat) = 0;
    };

    template<typename T>
    concept FileNodeStat = requires (T it) {
        { it.stat(std::declval<NodeStat*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FileNodeRead = requires (T it) {
        { it.read(std::declval<ReadRequest>(), std::declval<ReadResult*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FileNodeWrite = requires (T it) {
        { it.write(std::declval<WriteRequest>(), std::declval<WriteResult*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FileNode = FileNodeStat<T> && std::derived_from<T, INode>;

    template<FileNode T>
    class TFileHandle : public IFileHandle {
        T *mNode;

    public:
        TFileHandle(T *node)
            : mNode(node)
        { }

        virtual HandleInfo info() override {
            return HandleInfo { mNode };
        }

        virtual OsStatus stat(NodeStat *stat) override {
            return static_cast<T*>(mNode)->stat(stat);
        }

        virtual OsStatus read(ReadRequest request, ReadResult *result) override {
            if constexpr (FileNodeRead<T>) {
                return static_cast<T*>(mNode)->read(request, result);
            } else {
                return OsStatusNotSupported;
            }
        }

        virtual OsStatus write(WriteRequest request, WriteResult *result) override {
            if constexpr (FileNodeWrite<T>) {
                return static_cast<T*>(mNode)->write(request, result);
            } else {
                return OsStatusNotSupported;
            }
        }
    };

    template<typename T>
    concept FolderLookup = requires (T it) {
        { it.lookup(std::declval<VfsStringView>(), std::declval<INode**>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FolderMkNode = requires (T it) {
        { it.mknode(std::declval<VfsStringView>(), std::declval<INode*>()) } -> std::same_as<OsStatus>;
    };

    class FolderMixin {
        using FolderContainer = sm::BTreeMap<VfsString, INode*, std::less<>>;

        FolderContainer mChildren;

    public:
        OsStatus lookup(VfsStringView name, INode **child);
        OsStatus mknode(VfsStringView name, INode *child);
    };

    class IFolderHandle : public IHandle {
    public:
        virtual ~IFolderHandle() = default;

        virtual OsStatus lookup(VfsStringView name, INode **child) = 0;
        virtual OsStatus mknode(VfsStringView name, INode *child) = 0;
    };

    template<std::derived_from<INode> T>
    class TFolderHandle : public IFolderHandle {
        T *mNode;
    public:
        TFolderHandle(T *node)
            : mNode(node)
        { }

        virtual OsStatus lookup(VfsStringView name, INode **child) override {
            if constexpr (FolderLookup<T>) {
                return static_cast<T*>(mNode)->lookup(name, child);
            } else {
                return OsStatusNotSupported;
            }
        }

        virtual OsStatus mknode(VfsStringView name, INode *child) override {
            if constexpr (FolderMkNode<T>) {
                return static_cast<T*>(mNode)->mknode(name, child);
            } else {
                return OsStatusNotSupported;
            }
        }

        virtual HandleInfo info() override {
            return HandleInfo { mNode };
        }
    };

    class FolderIterator : public IHandle {
    public:
    };

    class FolderNode final : public INode, public FolderMixin {
        INode *mParent;
        IVfsMount *mMount;
        VfsString mName;

    public:
        FolderNode(INode *parent, IVfsMount *mount, VfsString name)
            : mParent(parent)
            , mMount(mount)
            , mName(name)
        { }

        virtual OsStatus query(sm::uuid uuid, const void *, size_t, IHandle **handle) override {
            if (uuid == kOsFolderGuid) {
                auto *folder = new (std::nothrow) TFolderHandle<FolderNode>(this);
                if (!folder) {
                    return OsStatusOutOfMemory;
                }

                *handle = folder;
                return OsStatusSuccess;
            }

            return OsStatusNotSupported;
        }

        NodeInfo info() override {
            return NodeInfo { .name = mName, .mount = mMount, .parent = mParent };
        }
    };
}
