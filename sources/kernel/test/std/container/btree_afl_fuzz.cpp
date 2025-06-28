#include "absl/container/btree_map.h"
#ifdef NDEBUG
#   undef NDEBUG
#endif

#include <fstream>

#include "std/container/btree.hpp"

#include <assert.h>

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

int main(int argc, const char **argv) {
    assert(argc > 1 && "Usage: btree_afl_fuzz <source_file>");
    const char *source = argv[1];
    std::ifstream file(source);

    std::vector<Action> actions;
    size_t fileSize = getFileSize(file);
    size_t actionCount = fileSize / sizeof(Action);
    actions.resize(actionCount);
    file.read(reinterpret_cast<char*>(actions.data()), fileSize);

    sm::BTreeMap<uint32_t, uint32_t> tree;
    absl::btree_map<uint32_t, uint32_t> baseline;

    for (const auto &action : actions) {
        switch (action.type % 3) {
        case 0: { // Insert
            tree.insert(action.key, action.value);
            baseline.insert({action.key, action.value});

            assert(baseline.find(action.key) != baseline.end() && "Key not found in baseline after insert");
            assert(tree.find(action.key) != tree.end() && "Key not found in tree after insert");
            break;
        }
        case 1: { // Find
            auto baselineIt = baseline.find(action.key);
            auto it = tree.find(action.key);
            if (it != tree.end()) {
                assert(baselineIt != baseline.end() && "Key found in tree but not in baseline");
                auto [foundKey, foundValue] = *it;
                assert(foundKey == action.key && "Found key does not match action key");
                assert(foundValue == action.value && "Found value does not match action value");
            } else {
                assert(baselineIt == baseline.end() && "Key not found in tree but exists in baseline");
            }
            break;
        }
        case 2: { // Remove
            tree.remove(action.key);
            baseline.erase(action.key);

            assert(baseline.find(action.key) == baseline.end() && "Key not removed from baseline");
            assert(tree.find(action.key) == tree.end() && "Key not removed from tree");
            break;
        }
        }
    }
}
