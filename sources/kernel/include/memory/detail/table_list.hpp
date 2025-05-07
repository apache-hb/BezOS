#pragma once

#include "common/util/util.hpp"

#include <stddef.h>

#include <utility>

namespace x64 {
    struct page;
}

namespace km::detail {
    class PageTableList {
        x64::page *mTable;

    public:
        UTIL_NOCOPY(PageTableList);

        PageTableList(PageTableList&& other) noexcept [[clang::nonblocking]]
            : mTable(std::exchange(other.mTable, nullptr))
        { }

        PageTableList& operator=(PageTableList&& other) noexcept [[clang::nonblocking]] {
            std::swap(mTable, other.mTable);
            return *this;
        }

        PageTableList() noexcept [[clang::nonblocking]] : mTable(nullptr) {}
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
