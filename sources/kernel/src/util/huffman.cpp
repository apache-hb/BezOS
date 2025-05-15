#include "util/huffman.hpp"

#include <array>

#include <limits.h>

namespace {
    class HuffTree;
    struct HuffNode;

    using HfWeight = uint16_t;

    constexpr size_t kNodeBits = CHAR_BIT;

    struct DecodeBuffer {

    };

    struct HuffNode {
        HuffNode *left;
        HuffNode *right;
        HuffNode *next;
        HfWeight weight;
        uint8_t value;

        void collect(uint8_t *buffer, size_t prefix);
        const uint8_t *construct(const uint8_t *value);
    };

    class HuffTree {
        HuffNode *mLow = new HuffNode { nullptr, nullptr, nullptr, 0, 0 };
        size_t mUnique{0};

        HuffNode *next(HuffNode *node) {
            return node->next;
        }

        void addNode(HuffNode *node) {
            HuffNode *current = mLow;
            while (current->next != nullptr && current->weight < node->weight) {
                current = current->next;
            }
            node->next = current->next;
            current->next = node;
        }

    public:
        void add(uint8_t value, HfWeight weight) {
            mUnique += 1;
            HuffNode *node = new HuffNode { nullptr, nullptr, nullptr, weight, value };
            addNode(node);
        }

        void combine();

        HuffNode *collect();
    };

}

size_t sm::huffmanEncode(const char *input [[clang::noescape, gnu::nonnull]], char *output [[clang::noescape, gnu::nonnull]], size_t inputSize) {
    std::array<HfWeight, UINT8_MAX> weights{};
    HuffTree tree;
    for (size_t i = 0; i < inputSize; i++) {
        weights[static_cast<uint8_t>(input[i])] += 1;
    }

    for (size_t i = 0; i < weights.size(); i++) {
        if (weights[i] > 0) {
            tree.add(i, weights[i]);
        }
    }
}

size_t sm::huffmanDecode(const char *input [[clang::noescape, gnu::nonnull]], char *output [[clang::noescape, gnu::nonnull]], size_t outputSize) {

}
