#pragma once

#include <stddef.h>

namespace sm {
    size_t huffmanEncode(const char *input [[clang::noescape, gnu::nonnull]], char *output [[clang::noescape, gnu::nonnull]], size_t inputSize);
    size_t huffmanDecode(const char *input [[clang::noescape, gnu::nonnull]], char *output [[clang::noescape, gnu::nonnull]], size_t outputSize);
}
