#pragma once

#include "common/util/util.hpp"

#include <stddef.h>

namespace km {
    /// @brief Information about a failed page mapping operation.
    class PageMappingResult {
        size_t mTotalPagesRequired{0};
        size_t mExtraPagesRequired{0};

    public:
        constexpr PageMappingResult() noexcept = default;

        constexpr PageMappingResult(size_t total, size_t pages) noexcept
            : mTotalPagesRequired(total)
            , mExtraPagesRequired(pages)
        { }

        UTIL_NOCOPY(PageMappingResult);
        UTIL_DEFAULT_MOVE(PageMappingResult);

        /// @brief Get the total number of pages required to complete the mapping.
        ///
        /// @return The total number of pages required.
        size_t totalPagesRequired() const noexcept [[clang::nonblocking]] { return mTotalPagesRequired; }

        /// @brief Get the number of extra pages required to complete the mapping.
        ///
        /// @return The number of extra pages required.
        size_t extraPagesRequired() const noexcept [[clang::nonblocking]] { return mExtraPagesRequired; }
    };
}
