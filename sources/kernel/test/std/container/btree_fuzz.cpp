#include "absl/container/btree_map.h"
#ifdef NDEBUG
#   undef NDEBUG
#endif

#include <fstream>

#include "std/container/btree.hpp"

#include <assert.h>

struct alignas(256) BigKey {
    uint32_t key;

    BigKey(uint32_t k = 0) noexcept
        : key(k)
    { }

    constexpr operator int() const noexcept {
        return key;
    }

    constexpr auto operator<=>(const BigKey&) const noexcept = default;

    constexpr bool operator==(const BigKey& other) const noexcept {
        return key == other.key;
    }

    constexpr bool operator==(uint32_t other) const noexcept {
        return key == other;
    }
};

struct [[gnu::packed]] Action {
    uint8_t type;
    uint32_t key;
    uint32_t value;
};

size_t getFileSize(std::ifstream &file) {
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    return size;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < sizeof(Action)) {
        return 0; // Not enough data to process any actions
    }

    std::vector<Action> actions;
    size_t actionCount = size / sizeof(Action);
    actions.resize(actionCount + 1);
    std::memcpy(actions.data(), data, size);

    sm::BTreeMap<BigKey, uint32_t> tree;
    absl::btree_map<BigKey, uint32_t> baseline;

    for (const auto &action : actions) {
        uint32_t key = action.key;
        uint32_t value = action.value;
        switch (action.type % 3) {
        case 0: { // Insert
            tree.insert(key, value);
            baseline[key] = value;

            assert(baseline.find(key) != baseline.end() && "Key not found in baseline after insert");
            assert(tree.find(key) != tree.end() && "Key not found in tree after insert");
            break;
        }
        case 1: { // Find
            auto baselineIt = baseline.find(key);
            auto it = tree.find(key);
            if (it != tree.end()) {
                assert(baselineIt != baseline.end() && "Key found in tree but not in baseline");
                auto [foundKey, foundValue] = *it;
                assert(foundKey == key && "Found key does not match action key");
                assert(foundValue == baselineIt->second && "Found value does not match action value");
            } else {
                assert(baselineIt == baseline.end() && "Key not found in tree but exists in baseline");
            }
            break;
        }
        case 2: { // Remove
            tree.remove(key);
            baseline.erase(key);

            assert(baseline.find(key) == baseline.end() && "Key not removed from baseline");
            assert(tree.find(key) == tree.end() && "Key not removed from tree");
            break;
        }
        }
    }

    return 0;
}
