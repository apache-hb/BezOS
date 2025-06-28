#include <gtest/gtest.h>

#include "absl/container/btree_map.h"
#include "std/container/btree.hpp"
#include <random>

class BTreeStringTest : public testing::Test {
public:
    using Map = sm::BTreeMap<std::string, int>;
    std::mt19937 mt{0x1234};

    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
    }

    std::string GenerateString(size_t length) {
        static constexpr char kCharSet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<size_t> dist(0, sizeof(kCharSet) - 2);
        std::string str;
        str.reserve(length);
        for (size_t i = 0; i < length; i++) {
            char c = kCharSet[dist(mt)];
            str.push_back(c);
        }
        return str;
    }
};

TEST_F(BTreeStringTest, InsertAndFind) {
    Map map;
    absl::btree_map<std::string, int> baseline;

    for (size_t i = 0; i < 1000; i++) {
        std::string key = GenerateString(10);
        int value = i * 10;
        map.insert(key, value);
        baseline.insert({key, value});
    }

    for (const auto& [key, value] : baseline) {
        auto it = map.find(key);
        ASSERT_NE(it, map.end()) << "Key " << key << " not found in BTreeMap";
        auto [foundKey, foundValue] = *it;
        ASSERT_EQ(foundKey, key) << "Found key does not match expected key";
        ASSERT_EQ(foundValue, value) << "Found value does not match expected value";
    }

    auto takeRandomKey = [&] {
        auto it = baseline.begin();
        std::advance(it, mt() % baseline.size());
        auto key = it->first;
        baseline.erase(it);
        return key;
    };

    while (!baseline.empty()) {
        auto key = takeRandomKey();
        bool contains = map.contains(key);
        ASSERT_TRUE(contains) << "Key " << key << " not found in BTreeMap before removal";
        map.remove(key);
        ASSERT_FALSE(map.contains(key)) << "Key " << key << " found in BTreeMap after removal";
        ASSERT_TRUE(baseline.find(key) == baseline.end()) << "Key " << key << " found in baseline map after removal";
    }
}
