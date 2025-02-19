#pragma once

#include <atomic>
#include <emmintrin.h>

namespace stdx {
    class ConditionVariable {

    };

    class Barrier {

    };

    class Latch {
        std::atomic<uint32_t> mCount;

    public:
        constexpr Latch(uint32_t count)
            : mCount(count)
        { }

        void arrive() {
            if (mCount.fetch_sub(1) > 1) {
                while (mCount.load() != 0) {
                    _mm_pause();
                }
            }
        }
    };
}
