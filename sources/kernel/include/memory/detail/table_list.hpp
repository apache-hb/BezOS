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

        PageTableList(PageTableList&& other) noexcept [[clang::nonallocating]]
            : mTable(std::exchange(other.mTable, nullptr))
        { }

        PageTableList& operator=(PageTableList&& other) noexcept [[clang::nonallocating]] {
            mTable = std::exchange(other.mTable, nullptr);
            return *this;
        }

        PageTableList() noexcept [[clang::nonallocating]] : mTable(nullptr) {}
        PageTableList(x64::page *table [[gnu::nonnull]], size_t count) noexcept [[clang::nonallocating]];

        void push(x64::page *page [[gnu::nonnull]]) noexcept [[clang::nonallocating]];
        void push(x64::page *pages [[gnu::nonnull]], size_t count) noexcept [[clang::nonallocating]];
        void append(PageTableList list) noexcept [[clang::nonallocating]];

        [[gnu::returns_nonnull]]
        x64::page *next() noexcept [[clang::nonallocating]];

        x64::page *drain() noexcept [[clang::nonallocating]];

        size_t count() const noexcept [[clang::nonallocating]];
    };
}
