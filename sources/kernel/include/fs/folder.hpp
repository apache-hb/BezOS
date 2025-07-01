#pragma once

#include "fs/device.hpp"
#include "fs/interface.hpp"
#include "std/shared_spinlock.hpp"
#include "util/absl.hpp"

namespace vfs {
    template<typename T>
    concept FolderLookup = requires (T it) {
        { it.lookup(std::declval<VfsStringView>(), std::declval<sm::RcuSharedPtr<INode>*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FolderMkNode = requires (T it) {
        { it.mknode(std::declval<sm::RcuWeakPtr<INode>>(), std::declval<VfsStringView>(), std::declval<sm::RcuSharedPtr<INode>>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FolderRmNode = requires (T it) {
        { it.rmnode(std::declval<sm::RcuSharedPtr<INode>>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FolderNodeType = FolderLookup<T> && std::derived_from<T, INode>;

    class FolderMixin {
        using Container = sm::BTreeMap<VfsString, sm::RcuSharedPtr<INode>, std::less<>>;
        using MapIterator = typename Container::iterator;

        stdx::SharedSpinLock mLock;
        Container mChildren GUARDED_BY(mLock);
        uint32_t mGeneration GUARDED_BY(mLock) = 0;

    public:
        struct Iterator {
            VfsString name;
            MapIterator current;
            uint32_t generation;
        };

        OsStatus lookup(VfsStringView name, sm::RcuSharedPtr<INode> *child);
        OsStatus mknode(sm::RcuWeakPtr<INode> parent, VfsStringView name, sm::RcuSharedPtr<INode> child);
        OsStatus rmnode(sm::RcuSharedPtr<INode> child);

        OsStatus next(Iterator *iterator, sm::RcuSharedPtr<INode> *node);
    };

    template<FolderNodeType T>
    class TFolderHandle : public BaseHandle<T, IFolderHandle> {
        using BaseHandle<T, IFolderHandle>::mNode;

    public:
        TFolderHandle(sm::RcuSharedPtr<T> node, const void *, size_t)
            : BaseHandle<T, IFolderHandle>(node)
        { }

        virtual OsStatus lookup(VfsStringView name, sm::RcuSharedPtr<INode> *child) override {
            return mNode->lookup(name, child);
        }

        virtual OsStatus mknode(VfsStringView name, sm::RcuSharedPtr<INode> child) override {
            if constexpr (FolderMkNode<T>) {
                return mNode->mknode(mNode, name, child);
            } else {
                return OsStatusFunctionNotSupported;
            }
        }

        virtual OsStatus rmnode(sm::RcuSharedPtr<INode> child) override {
            if constexpr (FolderRmNode<T>) {
                return mNode->rmnode(child);
            } else {
                return OsStatusFunctionNotSupported;
            }
        }

        virtual HandleInfo info() override {
            return HandleInfo { mNode, kOsFolderGuid };
        }
    };
}
