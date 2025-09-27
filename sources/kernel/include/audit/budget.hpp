#pragma once

#include <atomic>
#include <stddef.h>
#include <string_view>

namespace km {
    class ResourceLimit {
        std::string_view mName;
        std::atomic<size_t> mHardLimit;
        std::atomic<size_t> mSoftLimit;
        std::atomic<size_t> mUsed;

    public:
        ResourceLimit(std::string_view name, size_t hardLimit, size_t softLimit) noexcept
            : mName(name)
            , mHardLimit(hardLimit)
            , mSoftLimit(softLimit)
        { }

        [[nodiscard]]
        bool use(size_t amount, std::string_view reason) noexcept [[clang::nonallocating]];

        void release(size_t amount) noexcept [[clang::nonallocating]];

        size_t getHardLimit() const noexcept [[clang::nonallocating]] {
            return mHardLimit.load(std::memory_order_relaxed);
        }

        size_t getSoftLimit() const noexcept [[clang::nonallocating]] {
            return mSoftLimit.load(std::memory_order_relaxed);
        }

        size_t getUsed() const noexcept [[clang::nonallocating]] {
            return mUsed.load(std::memory_order_relaxed);
        }

        size_t getAvailable() const noexcept [[clang::nonallocating]] {
            size_t used = mUsed.load(std::memory_order_relaxed);
            size_t limit = mHardLimit.load(std::memory_order_relaxed);
            return (used >= limit) ? 0 : (limit - used);
        }
    };
}
