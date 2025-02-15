#pragma once

#include "std/forward_list.hpp"

#include <atomic>

namespace sm {
    class RcuObject {

    };

    class RcuDomain {
        AtomicForwardList<RcuObject*> mRetired;
        std::atomic<uint32_t> mReaders;

    public:
        constexpr RcuDomain()
            : mReaders(0)
        { }

        /// @brief Acquire a read lock on this domain.
        void lock();

        /// @brief Acquire a read lock on this domain.
        ///
        /// This function behaves identically to lock(), as rcu can by design
        /// never block a reader. This function is provided to remain consistent
        /// with the C++ standard locking interface.
        ///
        /// @return Always true.
        bool try_lock();

        /// @brief Release a read lock on this domain.
        void unlock();

        /// @brief Wait until all current readers have finished.
        void synchronize();

        /// @brief Mark an object as retired.
        ///
        /// Retiring an object means that it is no longer current and should be
        /// reclaimed when all readers have finished with it.
        ///
        /// @param object The object to retire.
        void retire(RcuObject *object);
    };
}
