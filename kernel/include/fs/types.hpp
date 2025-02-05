#pragma once

#include "absl/container/btree_map.h"
#include "allocator/allocator.hpp"

#include <utility>
#include <cstdint>

namespace km {
    template<typename TKey, typename TValue, typename TCompare = std::less<TKey>, typename TAllocator = mem::GlobalAllocator<std::pair<const TKey, TValue>>>
    using BTreeMap = absl::btree_map<TKey, TValue, TCompare, TAllocator>;

    class IFileSystem;

    enum class VfsNodeId : std::uint64_t {
        eInvalid = UINT64_MAX,
    };

    enum class VfsHandleId : std::uint64_t {
        eInvalid = UINT64_MAX,
    };

    enum class VfsEntryType {
        eNone,

        eFile,
        eFolder,
        eDevice,
        eMount,
    };

    class VfsIdAllocator {
        VfsNodeId mNextId = VfsNodeId(1);
    public:
        VfsNodeId allocateId() {
            VfsNodeId id = mNextId;
            mNextId = VfsNodeId(std::to_underlying(mNextId) + 1);
            return id;
        }
    };
}
