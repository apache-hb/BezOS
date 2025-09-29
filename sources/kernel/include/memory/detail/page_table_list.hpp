#pragma once

#include "common/util/util.hpp"

#include "memory/detail/page_table_allocation.hpp"
#include "panic.hpp"

namespace km::detail {
    class PageTableList2 {
        PageTableAllocation mTable;

    public:
        UTIL_NOCOPY(PageTableList2);
        UTIL_DEFAULT_MOVE(PageTableList2);

        PageTableList2() noexcept [[clang::nonblocking]] = default;

        PageTableList2(PageTableAllocation table, size_t count) noexcept [[clang::nonblocking]] {
            [[assume(count > 0)]];

            for (size_t i = 0; i < count - 1; i++) {
                table.setNext(table.offset(1));
                table = table.getNext();
            }

            table.setNext(PageTableAllocation{});
            mTable = table;
        }

        void push(PageTableAllocation page) noexcept [[clang::nonblocking]] {
            page.setNext(mTable);
            mTable = page;
        }

        void push(PageTableAllocation pages, size_t count) noexcept [[clang::nonblocking]] {
            [[assume(count > 0)]];

            for (size_t i = 0; i < count; i++) {
                push(pages.offset(i));
            }
        }

        void append(PageTableList2 list) noexcept [[clang::nonblocking]] {
            while (PageTableAllocation page = list.drain()) {
                push(page);
            }
        }

        PageTableAllocation next() noexcept [[clang::nonblocking]] {
            KM_CHECK(mTable.isPresent(), "PageTableList exhausted.");

            PageTableAllocation it = mTable;
            mTable = mTable.getNext();
            memset(it.getVirtual(), 0, x64::kPageSize); // Clear the page, next() is expected to return a zeroed page.
            return it;
        }

        PageTableAllocation drain() noexcept [[clang::nonblocking]] {
            if (!mTable.isPresent()) {
                return PageTableAllocation{};
            }

            PageTableAllocation it = mTable;
            mTable = mTable.getNext();
            return it;
        }

        size_t count() const noexcept [[clang::nonblocking]] {
            size_t count = 0;
            for (PageTableAllocation it = mTable; it.isPresent(); it = it.getNext()) {
                count++;
            }
            return count;
        }
    };
}
