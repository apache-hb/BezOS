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
            return mNode->stat(stat);
        }

        virtual OsStatus read(ReadRequest request, ReadResult *result) override {
            if constexpr (FileNodeRead<T>) {
                return mNode->read(request, result);
            } else {
                return OsStatusNotSupported;
            }
        }

        virtual OsStatus write(WriteRequest request, WriteResult *result) override {
            if constexpr (FileNodeWrite<T>) {
                return mNode->write(request, result);
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

    template<typename T>
    concept FolderRmNode = requires (T it) {
        { it.rmnode(std::declval<INode*>()) } -> std::same_as<OsStatus>;
    };

    class FolderMixin {
        using FolderContainer = sm::BTreeMap<VfsString, INode*, std::less<>>;

        FolderContainer mChildren;

    public:
        OsStatus lookup(VfsStringView name, INode **child);
        OsStatus mknode(VfsStringView name, INode *child);
        OsStatus rmnode(INode *child);
    };

    class IFolderHandle : public IHandle {
    public:
        virtual ~IFolderHandle() = default;

        virtual OsStatus lookup(VfsStringView name, INode **child) = 0;
        virtual OsStatus mknode(VfsStringView name, INode *child) = 0;
        virtual OsStatus rmnode(INode *child) = 0;
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
                return mNode->lookup(name, child);
            } else {
                return OsStatusFunctionNotSupported;
            }
        }

        virtual OsStatus mknode(VfsStringView name, INode *child) override {
            if constexpr (FolderMkNode<T>) {
                return mNode->mknode(name, child);
            } else {
                return OsStatusFunctionNotSupported;
            }
        }

        virtual OsStatus rmnode(INode *child) override {
            if constexpr (FolderRmNode<T>) {
                return mNode->rmnode(child);
            } else {
                return OsStatusFunctionNotSupported;
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
        NodeInfo mInfo;

    public:
        FolderNode(NodeInfo info)
            : mInfo(info)
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

            return OsStatusInterfaceNotSupported;
        }

        void init(INode *parent, VfsString name, Access access) override {
            mInfo = NodeInfo { name, mInfo.mount, parent, access };
        }

        NodeInfo info() override {
            return mInfo;
        }
    };
}
