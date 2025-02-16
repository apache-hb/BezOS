#pragma once

#include "allocator/allocator.hpp"

#include <absl/container/btree_map.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

namespace sm {
    template<typename TKey, typename TValue, typename TCompare = std::less<TKey>, typename TAllocator = mem::GlobalAllocator<std::pair<const TKey, TValue>>>
    using BTreeMap = absl::btree_map<TKey, TValue, TCompare, TAllocator>;

    template<typename TKey, typename TValue, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>, typename TAllocator = mem::GlobalAllocator<std::pair<const TKey, TValue>>>
    using FlatHashMap = absl::flat_hash_map<TKey, TValue, THash, TEqual, TAllocator>;

    template<typename TKey, typename THash = std::hash<TKey>, typename TEqual = std::equal_to<TKey>, typename TAllocator = mem::GlobalAllocator<TKey>>
    using FlatHashSet = absl::flat_hash_set<TKey, THash, TEqual, TAllocator>;

    template<typename T, typename... A> requires (std::constructible_from<T, A...>)
    std::unique_ptr<T> makeUnique(A&&... args) {
        return std::unique_ptr<T>(new T(std::forward<A>(args)...));
    }
}
