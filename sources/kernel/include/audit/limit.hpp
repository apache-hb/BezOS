#pragma once

#include <bezos/status.h>

#include "std/std.hpp"
#include <stddef.h>

#include <atomic>
#include <string_view>

namespace km {
    class ResourceLimit {
        std::string_view mName;
        std::atomic<size_t> mHardLimit;
        std::atomic<size_t> mSoftLimit;
        std::atomic<size_t> mUsed;

    public:
        ResourceLimit(std::string_view name, size_t hardLimit, size_t softLimit) noexcept;

        [[nodiscard]]
        bool use(size_t amount, std::string_view reason) noexcept [[clang::nonallocating]];

        void release(size_t amount) noexcept [[clang::nonallocating]];

        [[nodiscard]]
        size_t getHardLimit() const noexcept [[clang::nonallocating]];

        [[nodiscard]]
        size_t getSoftLimit() const noexcept [[clang::nonallocating]];

        [[nodiscard]]
        size_t getUsed() const noexcept [[clang::nonallocating]];

        [[nodiscard]]
        size_t getAvailable() const noexcept [[clang::nonallocating]];

        [[nodiscard]]
        static OsStatus create(std::string_view name, size_t hardLimit, size_t softLimit, ResourceLimit *result [[outparam]]) noexcept [[clang::allocating]];
    };
}
