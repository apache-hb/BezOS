#pragma once

#include "std/vector.hpp"

#include "vfs_types.hpp"

namespace km {
    template<typename TKey, typename TValue, typename TCompare = std::less<TKey>, typename TAllocator = mem::GlobalAllocator<std::pair<const TKey, TValue>>>
    using BTreeMap = absl::btree_map<TKey, TValue, TCompare, TAllocator>;

    class VfsNode;

    struct VfsFileNode {

    };

    struct VfsFolderNode {
        BTreeMap<stdx::String, VfsNode*> children;
    };

    class VfsNode {

    };

    class VirtualFileSystem {
        stdx::Vector2<VfsMount> mMounts;
        stdx::Vector2<VfsHandle> mHandles;
    public:
    };
}
