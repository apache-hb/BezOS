#pragma once

#include "fs2/interface.hpp"
#include "util/absl.hpp"

namespace vfs2 {
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

    template<typename T>
    concept FolderNodeType = FolderLookup<T> && std::derived_from<T, INode>;

    class FolderMixin {
        using Container = sm::BTreeMap<VfsString, std::unique_ptr<INode>, std::less<>>;

        Container mChildren;

    public:
        struct Iterator {
            uint64_t generation;
            VfsString name;
            Container::iterator current;
        };

        OsStatus lookup(VfsStringView name, INode **child);
        OsStatus mknode(VfsStringView name, INode *child);
        OsStatus rmnode(INode *child);

        OsStatus next(Iterator *iterator, INode **node);
    };

    template<FolderNodeType T>
    class TFolderHandle : public IFolderHandle {
        T *mNode;
    public:
        TFolderHandle(T *node)
            : mNode(node)
        { }

        virtual OsStatus lookup(VfsStringView name, INode **child) override {
            return mNode->lookup(name, child);
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
}
