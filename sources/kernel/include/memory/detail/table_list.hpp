#pragma once

#include "common/util/util.hpp"

#include <stddef.h>

namespace x64 {
    struct page;
}

namespace km::detail {
    class PageTableList {
        x64::page *mTable{nullptr};

    public:
        UTIL_NOCOPY(PageTableList);
        UTIL_DEFAULT_MOVE(PageTableList);

        constexpr PageTableList() noexcept [[clang::nonblocking]] = default;

        PageTableList(x64::page *table [[gnu::nonnull]], size_t count) noexcept [[clang::nonblocking]];

        void push(x64::page *page [[gnu::nonnull]]) noexcept [[clang::nonblocking]];
        void push(x64::page *pages [[gnu::nonnull]], size_t count) noexcept [[clang::nonblocking]];
        void append(PageTableList list) noexcept [[clang::nonblocking]];

        [[gnu::returns_nonnull]]
        x64::page *next() noexcept [[clang::nonblocking]];

        x64::page *drain() noexcept [[clang::nonblocking]];

        size_t count() const noexcept [[clang::nonblocking]];
    };
}
