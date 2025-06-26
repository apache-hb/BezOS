#include <gtest/gtest.h>

#include <absl/container/btree_map.h>
#include "std/container/btree.hpp"
#include <random>

TEST(BTreeFuzzTest, Fuzz) {
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
            ASSERT_TRUE(tree.contains(key)) << "Key " << key << " not found in BTreeMap after insertion";
        } else if (action == 2) { // Find
            auto it = tree.find(key);
            if (it != tree.end()) {
                auto [foundKey, foundValue] = *it;
                ASSERT_EQ(foundKey, key);
                ASSERT_TRUE(baseline.contains(key)) << "Key " << key << " not found in baseline map";
            } else {
                ASSERT_FALSE(tree.contains(key));
                ASSERT_FALSE(baseline.contains(key)) << "Key " << key << " found in baseline map but not in BTreeMap";
            }
        } else if (action == 3) { // Remove
            tree.remove(key);
            baseline.erase(key);
            ASSERT_FALSE(tree.contains(key));
            ASSERT_FALSE(baseline.contains(key)) << "Key " << key << " found in baseline map after removal";
        }
    }
}
