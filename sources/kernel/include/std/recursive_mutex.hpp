#pragma once

#include "processor.hpp"

#include <atomic>

namespace stdx {
    class RecursiveSpinLock {
        std::atomic<km::CpuCoreId> mOwner;
        unsigned mCount;

    public:
        constexpr RecursiveSpinLock()
            : mOwner(km::CpuCoreId::eInvalid)
            , mCount(0)
        { }

        void lock() {
            km::CpuCoreId expected = km::CpuCoreId::eInvalid;
            km::CpuCoreId thread = km::GetCurrentCoreId();

            while (!mOwner.compare_exchange_strong(expected, thread, std::memory_order_acquire) && expected != thread) {
                expected = km::CpuCoreId::eInvalid;
                _mm_pause();
            }

            mCount += 1;
        }

        [[nodiscard]]
        bool try_lock() {
            km::CpuCoreId expected = km::CpuCoreId::eInvalid;
            km::CpuCoreId thread = km::GetCurrentCoreId();
            if (mOwner.compare_exchange_strong(expected, thread, std::memory_order_acquire) || expected == thread) {
                mCount += 1;
                return true;
            }

            return false;
        }

        void unlock() {
            if (--mCount == 0) {
                mOwner.store(km::CpuCoreId::eInvalid, std::memory_order_release);
            }
        }
    };
}
