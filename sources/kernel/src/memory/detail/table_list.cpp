#include "memory/detail/table_list.hpp"

#include "arch/paging.hpp"
#include "panic.hpp"

using PageTableList = km::detail::PageTableList;

PageTableList::PageTableList(x64::page *tables, size_t count) noexcept [[clang::nonblocking]] {
    [[assume(count > 0)]];

    for (size_t i = 0; i < count - 1; i++) {
        *(void**)(tables + i) = tables + i + 1;
    }

    *(void**)(tables + count - 1) = nullptr;
    mTable = tables;
}

void PageTableList::push(x64::page *table) noexcept [[clang::nonblocking]] {
    *(void**)(table) = mTable;
    mTable = table;
}

void PageTableList::push(x64::page *pages, size_t count) noexcept [[clang::nonblocking]] {
    [[assume(count > 0)]];

    for (size_t i = 0; i < count; i++) {
        push(pages + i);
    }
}

void PageTableList::append(PageTableList list) noexcept [[clang::nonblocking]] {
    while (x64::page *page = list.drain()) {
        push(page);
    }
}

x64::page *PageTableList::next() noexcept [[clang::nonblocking]] {
    KM_CHECK(mTable != nullptr, "PageTableList exhausted.");

    x64::page *it = mTable;
    mTable = (x64::page*)*(void**)(mTable);
    memset(it, 0, sizeof(x64::page)); // Clear the page, next() is expected to return a zeroed page.
    return it;
}

x64::page *PageTableList::drain() noexcept [[clang::nonblocking]] {
    if (mTable == nullptr) {
        return nullptr;
    }

    x64::page *it = mTable;
    mTable = (x64::page*)*(void**)(mTable);
    return it;
}

size_t PageTableList::count() const noexcept [[clang::nonblocking]] {
    size_t count = 0;
    for (x64::page *it = mTable; it != nullptr; it = (x64::page*)*(void**)(it)) {
        count++;
    }
    return count;
}
