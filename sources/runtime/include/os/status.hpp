#pragma once

#include <bezos/status.h>

namespace os {
    class Status {
        OsStatus mStatus;

    public:
        constexpr Status() noexcept
            : Status(OsStatusSuccess)
        { }

        constexpr Status(OsStatus status) noexcept
            : mStatus(status)
        { }

        constexpr bool operator==(const Status& other) const noexcept {
            return mStatus == other.mStatus;
        }

        constexpr bool operator!=(const Status& other) const noexcept {
            return mStatus != other.mStatus;
        }

        /// @brief Overload of bool conversion to allow shorthand error checks.
        ///
        /// Evaluates to true if the status is an error.
        ///
        /// @code{.cpp}
        /// if (os::Status status = OsClockStat(&info)) {
        ///     // Handle error
        /// }
        /// @endcode
        ///
        /// @note This is a shorthand for `status.isError()`.
        ///
        /// @return true if the status is an error, false otherwise.
        constexpr operator bool() const noexcept {
            return isError();
        }

        constexpr OsStatus get() const noexcept {
            return mStatus;
        }

        constexpr bool isSuccess() const noexcept {
            return OS_SUCCESS(mStatus);
        }

        constexpr bool isError() const noexcept {
            return OS_ERROR(mStatus);
        }
    };
}
