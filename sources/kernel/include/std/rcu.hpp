#pragma once

#include "common/util/util.hpp"
#include "panic.hpp"

#include <atomic>

#include <bezos/status.h>

namespace sm {
    class RcuDomain;
    class RcuGuard;
    class RcuObject;

    /// @brief A domain for RCU operations.
    ///
    /// This domain presents an EBR interface for deferred reclaimation and is implemented
    /// using generational reclaimation.
    ///
    /// @cite RcuDataStructures
    class RcuDomain {
    public:
        friend RcuGuard;

        /// @brief A generation of the RCU domain.
        ///
        /// Each generation tracks the number of reader threads currently accessing it.
        /// As well as all the data that can be reclaimed once the last reader has finished.
        class RcuGeneration {
            friend RcuDomain;
            friend RcuGuard;

            std::atomic<uint32_t> mGuard{0};
            std::atomic<RcuObject*> mHead{nullptr};

            /// @pre `mGuard.load() > 0`
            /// @pre `object->rcuRetireFn != nullptr`
            /// @post `mGuard.load() > 0`
            void append(RcuObject *object [[gnu::nonnull]]) noexcept [[clang::reentrant, clang::nonallocating]];

            /// @pre `mGuard.load() == 0`
            /// @post `mGuard.load() == 0`
            void destroy() noexcept [[clang::nonreentrant]];
        };

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
        void synchronize() noexcept [[clang::allocating, clang::nonreentrant]];

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
        void retire(T *object [[gnu::nonnull]]) noexcept [[clang::reentrant]] {
            object->rcuRetireFn = [](void *ptr) { delete static_cast<T*>(ptr); };
            append(object);
        }

        /// @brief Defer a call to happen during reclaimation.
        ///
        /// @param data Argument to provide to @p fn
        /// @param fn Function to call during reclaimation
        [[nodiscard]]
        OsStatus call(void *data, void(*fn [[gnu::nonnull]])(void*)) [[clang::allocating, clang::nonreentrant]];

    private:
        static constexpr uint32_t kCurrentGeneration = (1u << (sizeof(uint32_t) * CHAR_BIT - 1));
        std::atomic<uint32_t> mState{0};

        RcuGeneration mGenerations[2];

        /// @pre `object->rcuRetireFn != nullptr`
        void append(RcuObject *object) noexcept [[clang::reentrant]];

        /// @brief Acquire a reader lock on the current generation.
        ///
        /// @return The current generation.
        [[nodiscard]]
        RcuGeneration *acquire() noexcept [[clang::nonblocking, clang::reentrant]];

        /// @brief Exchange the current generation with a new one.
        ///
        /// @return The old generation.
        RcuGeneration *exchange() noexcept [[clang::nonreentrant]];
    };

    /// @brief Intrusive pointer base type for RCU objects.
    class RcuObject {
        friend class RcuDomain;
        friend class RcuGuard;

        std::atomic<RcuObject*> rcuNextObject = nullptr;
        void(*rcuRetireFn)(void*) = nullptr;
    };

    /// @brief Manages the lifetime of a reader lock on a RCU domain.
    class RcuGuard {
        friend class RcuDomain;
    public:
        RcuGuard(RcuDomain& domain) noexcept [[clang::reentrant]];
        ~RcuGuard() noexcept [[clang::reentrant]];

        RcuGuard(RcuGuard&& other) noexcept [[clang::reentrant]];

        RcuGuard& operator=(RcuGuard&& other) = delete;

        UTIL_NOCOPY(RcuGuard);

        template<std::derived_from<RcuObject> T>
        void retire(T *object [[gnu::nonnull]]) noexcept [[clang::reentrant]] {
            KM_ASSERT(mGeneration != nullptr);
            object->rcuRetireFn = [](void *ptr) { delete static_cast<T*>(ptr); };
            append(object);
        }

    private:
        void append(RcuObject *object) noexcept [[clang::reentrant]];

        using Generation = typename RcuDomain::RcuGeneration;
        Generation *mGeneration = nullptr;
    };
}
