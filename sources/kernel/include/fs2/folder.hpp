#pragma once

#include "fs2/fsptr.hpp"
#include "fs2/interface.hpp"
#include "util/absl.hpp"

namespace vfs2 {
    template<typename T>
    concept FolderLookup = requires (T it) {
        { it.lookup(std::declval<VfsStringView>(), std::declval<INode**>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FolderMkNode = requires (T it) {
        { it.mknode(std::declval<INode*>(), std::declval<VfsStringView>(), std::declval<INode*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FolderRmNode = requires (T it) {
        { it.rmnode(std::declval<INode*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept FolderNodeType = FolderLookup<T> && std::derived_from<T, INode>;

    class FolderMixin {
        using Container = sm::BTreeMap<VfsString, AutoRelease<INode>, std::less<>>;

        Container mChildren;

    public:
        struct Iterator {
            VfsString name;
            Container::iterator current;
            uint32_t generation;
        };

        OsStatus lookup(VfsStringView name, INode **child);
        OsStatus mknode(INode *parent, VfsStringView name, INode *child);
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
                return mNode->mknode(mNode, name, child);
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
            return HandleInfo { mNode, kOsFolderGuid };
        }
    };
}
