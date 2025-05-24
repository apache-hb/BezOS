#pragma once

#include "common/util/util.hpp"
#include "panic.hpp"

#include <atomic>

#include <bezos/status.h>

namespace sm {
    class RcuDomain;
    class RcuGuard;
    class RcuObject;

    using SimpleRetireCallback = void(*)(void *data);
    using RetireCallback = void(*)(RcuDomain *domain, void *data);

    namespace detail {
        /// @brief A generation of the RCU domain.
        ///
        /// Each generation tracks the number of reader threads currently accessing it.
        /// As well as all the data that can be reclaimed once the last reader has finished.
        class RcuGeneration {
            friend RcuDomain;

            std::atomic<uint32_t> mGuard{0};
            std::atomic<RcuObject*> mHead{nullptr};

        public:
            /// @pre `mGuard.load() > 0`
            /// @pre `object->rcuRetireFn != nullptr`
            /// @post `mGuard.load() > 0`
            void append(RcuObject *object [[gnu::nonnull]]) noexcept [[clang::reentrant, clang::nonblocking]];

            /// @pre `mGuard.load() == 0`
            /// @post `mGuard.load() == 0`
            ///
            /// @return The number of objects that were destroyed.
            size_t destroy(RcuDomain *domain) noexcept [[clang::nonreentrant, clang::blocking]];

            /// @brief Lock the generation for appending an element.
            /// This is not a blocking lock, and will always return immediately.
            /// This prevents the generation from being cleaned up while we are adding elements to it.
            void lock(std::memory_order order = std::memory_order_seq_cst) noexcept [[clang::reentrant]];

            /// @brief Unlock the generation.
            /// This notifies the generation that we are done with it and it can be cleaned up.
            void unlock(std::memory_order order = std::memory_order_seq_cst) noexcept [[clang::reentrant]];

            bool isLocked(std::memory_order order = std::memory_order_seq_cst) const noexcept [[clang::reentrant]];
        };
    }

    /// @brief A domain for RCU operations.
    ///
    /// This domain presents an EBR interface for deferred reclaimation and is implemented
    /// using generational reclaimation.
    ///
    /// @cite RcuDataStructures
    class RcuDomain {
    public:
        friend RcuGuard;

        UTIL_NOCOPY(RcuDomain);
        UTIL_NOMOVE(RcuDomain);

        constexpr RcuDomain() noexcept = default;

        ~RcuDomain() noexcept;

        /// @brief Wait until all current readers have finished.
        ///
        /// @warning This function must not be called from multiple threads at the same time.
        ///
        /// @details This function will block until all readers that are currently
        ///          reading have finished. If new readers arrive after this function is
        ///          called, this function will not wait for them to finish.
        size_t synchronize() noexcept [[clang::allocating, clang::nonreentrant]];

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
        template<std::derived_from<RcuObject> T>
        void retire(T *object [[gnu::nonnull]]) noexcept [[clang::reentrant, clang::nonblocking]] {
            enqueue(object, [](RcuDomain *domain [[maybe_unused]], void *ptr) { delete static_cast<T*>(ptr); });
        }

        /// @brief Enqueue a function to be called during reclaimation.
        ///
        /// @param object The object to retire.
        /// @param fn The function to call during reclaimation.
        ///
        /// @warning @p object is not automatically deleted, its memory must be managed externally.
        void enqueue(RcuObject *object [[gnu::nonnull]], RetireCallback fn [[gnu::nonnull]]) noexcept [[clang::reentrant, clang::nonblocking]];

        /// @brief Defer a call to happen during reclaimation.
        ///
        /// @param data Argument to provide to @p fn
        /// @param fn Function to call during reclaimation
        [[nodiscard]]
        OsStatus call(void *data, RetireCallback fn [[gnu::nonnull]]) [[clang::allocating, clang::nonreentrant]];

    private:
        static constexpr uint32_t kCurrentGeneration = (1u << (sizeof(uint32_t) * CHAR_BIT - 1));

        /// @brief Guard for the RCU generations.
        ///
        /// @details The top bit encodes which generation is currently active, the remaining bits
        ///          track the number of threads currently accessing the generation array.
        ///          The generation must never be swapped when the reader count is non-zero.
        std::atomic<uint32_t> mState{0};

        /// @brief Double buffered rcu generations.
        ///
        /// Generations are double buffered to allow for reclaimation to occur while new operations
        /// are being added to the current generation.
        detail::RcuGeneration mGenerations[2];

        /// @pre `object->rcuRetireFn != nullptr`
        void append(RcuObject *object) noexcept [[clang::reentrant, clang::nonblocking]];

        /// @brief Acquire a reader lock on the current generation.
        ///
        /// @return The current generation.
        [[nodiscard]]
        detail::RcuGeneration *acquireReadLock() noexcept [[clang::reentrant, clang::nonblocking]];

        void releaseReadLock(detail::RcuGeneration *generation) noexcept [[clang::reentrant]];

        detail::RcuGeneration *selectGeneration(uint32_t state) noexcept [[clang::reentrant]] {
            return &mGenerations[state & kCurrentGeneration ? 1 : 0];
        }

        /// @brief Exchange the current generation with a new one.
        ///
        /// @return The old generation.
        detail::RcuGeneration *exchange() noexcept [[clang::reentrant]];
    };

    /// @brief Intrusive pointer base type for RCU objects.
    class RcuObject {
        friend RcuDomain;
        friend RcuGuard;
        friend detail::RcuGeneration;

        std::atomic<RcuObject*> rcuNextObject = nullptr;
        RetireCallback rcuRetireFn = nullptr;
    };

    /// @brief Manages the lifetime of a reader lock on a RCU domain.
    class RcuGuard {
        friend RcuDomain;
    public:
        RcuGuard& operator=(RcuGuard&& other) = delete;
        UTIL_NOCOPY(RcuGuard);

        RcuGuard(RcuDomain& domain) noexcept [[clang::reentrant, clang::nonblocking]];
        ~RcuGuard() noexcept [[clang::reentrant, clang::nonblocking]];

        RcuGuard(RcuGuard&& other) noexcept [[clang::reentrant]];

        /// @brief Retire an object.
        ///
        /// Defer the reclaimation of an object until all readers in the current generation
        /// have completed.
        ///
        /// @tparam T The type of object to reclaim.
        ///
        /// @param object The object to retire.
        template<std::derived_from<RcuObject> T>
        void retire(T *object [[gnu::nonnull]]) noexcept [[clang::reentrant, clang::nonblocking]] {
            KM_ASSERT(mGeneration != nullptr);
            enqueue(object, [](RcuDomain *domain [[maybe_unused]], void *ptr) { delete static_cast<T*>(ptr); });
        }

        /// @brief Enqueue a function to be called during reclaimation.
        ///
        /// @warning @p object is not automatically deleted, its memory must be managed externally.
        ///
        /// @param object The object to retire.
        /// @param fn The function to call during reclaimation.
        void enqueue(RcuObject *object [[gnu::nonnull]], RetireCallback fn [[gnu::nonnull]]) noexcept [[clang::reentrant, clang::nonblocking]];

        OsStatus call(void *data, RetireCallback fn [[gnu::nonnull]]) noexcept [[clang::allocating]];

        constexpr bool operator==(const RcuGuard& other) const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mGeneration == other.mGeneration;
        }

        constexpr bool operator!=(const RcuGuard& other) const noexcept [[clang::reentrant, clang::nonblocking]] {
            return mGeneration != other.mGeneration;
        }

    private:
        void append(RcuObject *object) noexcept [[clang::reentrant, clang::nonblocking]];

        RcuDomain *mDomain = nullptr;
        detail::RcuGeneration *mGeneration = nullptr;
    };
}
