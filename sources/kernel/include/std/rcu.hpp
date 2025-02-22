#pragma once

#include "std/queue.hpp"

#include "std/shared_spinlock.hpp"

#include <atomic>

namespace sm {
    class RcuDomain;
    class RcuGuard;

    /// @brief A domain for RCU operations.
    ///
    /// This domain presents an EBR interface for deferred reclaimation and is implemented
    /// using generational reclaimation.
    ///
    /// @cite RcuDataStructures
    class RcuDomain {
        struct RcuCall {
            void *value;
            void(*call)(void*);

            operator bool() const { return call != nullptr; }
        };
    public:
        friend RcuGuard;

        /// @brief A generation of the RCU domain.
        ///
        /// Each generation tracks the number of reader threads currently accessing it.
        /// As well as all the data that can be reclaimed once the last reader has finished.
        class RcuGeneration {
            friend RcuDomain;
            friend RcuGuard;

            std::atomic<uint32_t> guard;
            moodycamel::ConcurrentQueue<RcuCall> retired;
        };

        UTIL_NOCOPY(RcuDomain);
        UTIL_NOMOVE(RcuDomain);

        constexpr RcuDomain() = default;

        ~RcuDomain();

        /// @brief Wait until all current readers have finished.
        ///
        /// @details This function will block until all readers that are currently
        ///          reading have finished. If new readers arrive after this function is
        ///          called, this function will not wait for them to finish.
        void synchronize();

        /// @brief Mark an object as retired.
        ///
        /// Retiring an object means that it is no longer current and should be
        /// reclaimed when all readers have finished with it.
        ///
        /// @tparam T The type of object to reclaim.
        ///
        /// @param object The object to retire.
        ///
        /// @details This function is internally synchronized.
        template<typename T>
        void retire(T *object) {
            call((void*)object, +[](void *ptr) { delete static_cast<T*>(ptr); });
        }

        /// @brief Defer a call to happen during reclaimation.
        ///
        /// @param data Argument to provide to @p fn
        /// @param fn Function to call during reclaimation
        void call(void *data, void(*fn)(void*));

    private:
        std::atomic<RcuGeneration*> mCurrent = new RcuGeneration();
        stdx::SharedSpinLock mCurrentMutex;

        void destroy(RcuGeneration *generation);
        RcuGeneration *acquire();
        RcuGeneration *exchange(RcuGeneration *generation);
    };

    /// @brief Manages the lifetime of a reader lock on a RCU domain.
    class RcuGuard {
    public:
        RcuGuard(RcuDomain& domain);
        ~RcuGuard();

        RcuGuard(RcuGuard&& other);

        RcuGuard& operator=(RcuGuard&& other) = delete;

        UTIL_NOCOPY(RcuGuard);

    private:
        using Generation = typename RcuDomain::RcuGeneration;
        Generation *mGeneration = nullptr;
    };
}
