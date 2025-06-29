#include <absl/container/btree_map.h>
#include "std/container/btree.hpp"
#include <random>

int main() {
    sm::BTreeMap<int, int> tree;
    absl::btree_map<int, int> baseline;

    std::mt19937 mtValues(0x1234);
    std::mt19937 mtActions(0x5678);
    std::uniform_int_distribution<int> distValues(INT_MIN, INT_MAX);
    std::uniform_int_distribution<int> distActions(0, 3); // 0 = insert, 1 = insert, 2 = find, 3 = remove

    static constexpr size_t kFuzzIterations = 100000;
    for (size_t i = 0; i < kFuzzIterations; i++) {
        int key = distValues(mtValues);
        int value = distValues(mtValues);
        int action = distActions(mtActions);
        if (action == 0 || action == 1) { // Insert
            tree.insert(key, value);
            baseline[key] = value;
            assert(tree.contains(key));
        } else if (action == 2) { // Find
            auto it = tree.find(key);
            if (it != tree.end()) {
                auto [foundKey, foundValue] = *it;
                assert(foundKey == key);
                assert(baseline.contains(key));
            } else {
                assert(!tree.contains(key));
                assert(!baseline.contains(key));
            }
        } else if (action == 3) { // Remove
            tree.remove(key);
            baseline.erase(key);
            assert(!tree.contains(key));
            assert(!baseline.contains(key));
        }
    }
}
