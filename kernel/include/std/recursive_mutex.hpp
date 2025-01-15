#pragma once

#include "kernel.hpp"

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
            km::CpuCoreId thread = km::GetCurrentCoreId();
            km::CpuCoreId expected = km::CpuCoreId::eInvalid;
            while (!mOwner.compare_exchange_strong(expected, thread, std::memory_order_acquire)) {
                _mm_pause();
            }

            mCount += 1;
        }

        bool try_lock() {
            km::CpuCoreId thread = km::GetCurrentCoreId();
            km::CpuCoreId expected = km::CpuCoreId::eInvalid;
            if (mOwner.compare_exchange_strong(expected, thread, std::memory_order_acquire)) {
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
