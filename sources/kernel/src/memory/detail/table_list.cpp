#include "memory/detail/table_list.hpp"

#include "arch/paging.hpp"
#include "panic.hpp"

using km::detail::PageTableList;

PageTableList::PageTableList(x64::page *tables, size_t count, uintptr_t slide) noexcept [[clang::nonblocking]]
    : PageTableList(PageTableAllocation{ (void*)tables, slide }, count)
{ }

PageTableList::PageTableList(PageTableAllocation allocation, size_t count) noexcept [[clang::nonblocking]] {
    [[assume(count > 0)]];

    for (size_t i = 0; i < count - 1; i++) {
        allocation.offset(i).setNext(allocation.offset(i + 1));
    }

    allocation.offset(count - 1).setNext(PageTableAllocation{});
    mTable = allocation;
}

void PageTableList::push(PageTableAllocation allocation) noexcept [[clang::nonblocking]] {
    allocation.setNext(mTable);
    mTable = allocation;
}

void PageTableList::push(PageTableAllocation allocation, size_t count) noexcept [[clang::nonblocking]] {
    [[assume(count > 0)]];

    for (size_t i = 0; i < count; i++) {
        push(allocation.offset(i));
    }
}

void PageTableList::append(PageTableList list) noexcept [[clang::nonblocking]] {
    while (PageTableAllocation page = list.drain()) {
        push(page);
    }
}

km::PageTableAllocation PageTableList::next() noexcept [[clang::nonblocking]] {
    KM_CHECK(mTable.getVirtual() != nullptr, "PageTableList exhausted.");

    PageTableAllocation it = mTable;
    mTable = it.getNext();
    memset(it.getVirtual(), 0, sizeof(x64::page)); // Clear the page, next() is expected to return a zeroed page.
    return it;
}

km::PageTableAllocation PageTableList::drain() noexcept [[clang::nonblocking]] {
    if (!mTable.isPresent()) {
        return PageTableAllocation{};
    }

    PageTableAllocation it = mTable;
    mTable = mTable.getNext();
    return it;
}

size_t PageTableList::count() const noexcept [[clang::nonblocking]] {
    size_t count = 0;
    for (PageTableAllocation it = mTable; it.isPresent(); it = it.getNext()) {
        count++;
    }
    return count;
}
