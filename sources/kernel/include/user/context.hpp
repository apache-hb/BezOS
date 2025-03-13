#pragma once

#include <stddef.h>

#include <bezos/status.h>

#include "memory/memory.hpp"

namespace km {
    class PageBuilder;
    class PageTables;

    struct VerifyRules {
        /// @brief The minimum alignment of the pointer.
        size_t alignment = 8;

        /// @brief The minimum size of the span.
        size_t minSize = 0;

        /// @brief The maximum size of the span.
        size_t maxSize = SIZE_MAX;

        /// @brief If the span must be a multiple of a size.
        size_t multiple = 1;
    };

    OsStatus VerifyUserPointer(const VerifyRules& rules, uintptr_t address, size_t size, const PageBuilder *pm);
    OsStatus VerifyUserRange(const VerifyRules& rules, uintptr_t begin, uintptr_t end, const PageBuilder *pm);
    OsStatus VerifyRangeMapped(uintptr_t begin, uintptr_t end, PageFlags flags, PageTables *pt);
    OsStatus ReadUserRange(const VerifyRules& rules, uintptr_t begin, uintptr_t end, void *dst, const PageBuilder *pm, PageTables *pt);
}
