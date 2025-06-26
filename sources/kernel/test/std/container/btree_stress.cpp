#ifdef NDEBUG
#   undef NDEBUG
#endif

#include "std/container/btree.hpp"

#include <absl/container/btree_map.h>
#include <random>

int main() {
    static constexpr size_t kElementCount = 1000'000;
    sm::BTreeMap<int, int> tree;
    absl::btree_map<int, int> baseline;

    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);

    for (size_t i = 0; i < kElementCount; i++) {
        int key = dist(mt);
        tree.insert(key, i);
        baseline[key] = i;
    }

    for (auto& [key, value] : baseline) {
        auto it = tree.find(key);
        if (it == tree.end()) {
            tree.dump();
            assert(false && "Key not found in BTreeMap");
        }

        auto [foundKey, foundValue] = *it;
        if (foundKey != key || foundValue != value) {
            tree.dump();
            assert(false && "Key or value mismatch in BTreeMap");
        }

        assert(tree.contains(key) && "Key not found in BTreeMap after insertion");
        tree.erase(it);
        assert(!tree.contains(key) && "Key found in BTreeMap after removal");
    }
}
