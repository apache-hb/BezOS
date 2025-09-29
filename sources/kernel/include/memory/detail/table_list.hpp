#pragma once

#include "common/util/util.hpp"
#include "memory/detail/page_table_allocation.hpp"

#include <stddef.h>

namespace x64 {
    struct page;
}

namespace km::detail {
    class PageTableList {
        PageTableAllocation mTable;

    public:
        UTIL_NOCOPY(PageTableList);
        UTIL_DEFAULT_MOVE(PageTableList);

        constexpr PageTableList() noexcept [[clang::nonblocking]] = default;

        PageTableList(x64::page *table [[gnu::nonnull]], size_t count, uintptr_t slide = 0) noexcept [[clang::nonblocking]];
        PageTableList(PageTableAllocation allocation, size_t count) noexcept [[clang::nonblocking]];

        void push(PageTableAllocation allocation) noexcept [[clang::nonblocking]];

        void push(PageTableAllocation allocation, size_t count) noexcept [[clang::nonblocking]];

        void append(PageTableList list) noexcept [[clang::nonblocking]];

        PageTableAllocation next() noexcept [[clang::nonblocking]];

        PageTableAllocation drain() noexcept [[clang::nonblocking]];

        size_t count() const noexcept [[clang::nonblocking]];
    };
}
