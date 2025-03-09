#include "memory/detail/table_list.hpp"

#include "arch/paging.hpp"
#include "panic.hpp"

using PageTableList = km::detail::PageTableList;

PageTableList::PageTableList(x64::page *tables, size_t count) {
    for (size_t i = 0; i < count - 1; i++) {
        *(void**)(tables + i) = tables + i + 1;
    }

    *(void**)(tables + count - 1) = nullptr;
    mTable = tables;
}

void PageTableList::push(x64::page *table) {
    *(void**)(table) = mTable;
    mTable = table;
}

x64::page *PageTableList::next() {
    KM_CHECK(mTable != nullptr, "PageTableList exhausted.");

    x64::page *it = mTable;
    mTable = (x64::page*)*(void**)(mTable);
    return it;
}

x64::page *PageTableList::drain() {
    if (mTable == nullptr) {
        return nullptr;
    }

    x64::page *it = mTable;
    mTable = (x64::page*)*(void**)(mTable);
    return it;
}

x64::page *PageTableList::head() {
    return mTable;
}
