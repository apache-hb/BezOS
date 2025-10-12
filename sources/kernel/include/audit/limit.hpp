#pragma once

#include <bezos/status.h>

#include "std/std.hpp"
#include <stddef.h>

#include <atomic>
#include <string_view>

namespace km {
    /**
     * @brief Represents a limit on a system resource.
     *
     * This class is thread-safe and is used to track resource usage for an actor
     * against a system resource. E.G. how much physical memory a process can use,
     * how much cpu time a process can use, etc.
     */
    class ResourceLimit {
        /**
         * @brief The name of the resource being limited.
         */
        std::string_view mName;

        /**
         * @brief The hard limit for the resource.
         * This limit cannot be exceeded.
         */
        std::atomic<size_t> mHardLimit;

        /**
         * @brief The soft limit for the resource.
         * This limit can be exceeded temporarily, but the kernel will aim to
         * keep usage below this limit.
         */
        std::atomic<size_t> mSoftLimit;

        /**
         * @brief How much of the resource is currently used.
         */
        std::atomic<size_t> mUsed;

    public:
        /**
         * @brief Create a resource limit.
         *
         * @pre @p hardLimit must be non-zero.
         * @pre @p softLimit must be non-zero and less than or equal to @p hardLimit.
         *
         * @param name The name of the resource being limited.
         * @param hardLimit The hard limit for the resource.
         * @param softLimit The soft limit for the resource.
         */
        ResourceLimit(std::string_view name, size_t hardLimit, size_t softLimit) noexcept [[clang::nonblocking]];

        /**
         * @brief Attempt to claim a portion of the resource.
         *
         * If the claim would exceed the hard limit then this function fails.
         * When the claim is successful the resource must be released later by calling
         * @ref ResourceLimit#release.
         *
         * @param amount The amount of the resource to claim.
         * @param reason A string describing the reason for the claim, used for auditing.
         *
         * @return If the claim was successful.
         */
        [[nodiscard]]
        bool use(size_t amount, std::string_view reason) noexcept [[clang::nonblocking]];

        /**
         * @brief Release a portion of the resource.
         *
         * @param amount The amount of the resource to release.
         */
        void release(size_t amount) noexcept [[clang::nonblocking]];

        /**
         * @brief Get the hard limit for the resource.
         *
         * @return The hard limit.
         */
        [[nodiscard]]
        size_t getHardLimit() const noexcept [[clang::nonblocking]];

        /**
         * @brief Get the soft limit for the resource.
         *
         * @return The soft limit.
         */
        [[nodiscard]]
        size_t getSoftLimit() const noexcept [[clang::nonblocking]];

        /**
         * @brief Get the current usage of the resource.
         *
         * @return The current usage.
         */
        [[nodiscard]]
        size_t getUsed() const noexcept [[clang::nonblocking]];

        /**
         * @brief Get the amount of the resource that is still available.
         *
         * @return The available amount.
         */
        [[nodiscard]]
        size_t getAvailable() const noexcept [[clang::nonblocking]];

        /**
         * @brief Create a resource limit.
         *
         * This is a factory function that does input sanitization, as opposed to the constructor
         * which will abort if given invalid input.
         *
         * @pre @p name must be non-empty.
         * @pre @p hardLimit must be non-zero.
         * @pre @p softLimit must be non-zero and less than or equal to @p hardLimit.
         * @post @p result is left in an undefined state if this function returns an error.
         *
         * @param name The name of the resource being limited.
         * @param hardLimit The hard limit for the resource.
         * @param softLimit The soft limit for the resource.
         * @param[out] result The resource limit to write the result to.
         *
         * @return The status of the operation.
         *
         * @retval OsStatusSuccess The resource limit was created successfully, @p result is initialized.
         * @retval OsStatusInvalidInput The input parameters were invalid, no resource limit has been created.
         */
        [[nodiscard]]
        static OsStatus create(std::string_view name, size_t hardLimit, size_t softLimit, ResourceLimit *result [[outparam]]) noexcept [[clang::nonblocking]];
    };
}
