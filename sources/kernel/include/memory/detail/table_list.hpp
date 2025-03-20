#pragma once

#include <stddef.h>

namespace x64 {
    struct page;
}

namespace km::detail {
    class PageTableList {
        x64::page *mTable;

    public:
        PageTableList() : mTable(nullptr) {}
        PageTableList(x64::page *table [[gnu::nonnull]], size_t count);

        void push(x64::page *page [[gnu::nonnull]]);
        void push(x64::page *pages [[gnu::nonnull]], size_t count);

        [[gnu::returns_nonnull]]
        x64::page *next();

        x64::page *drain();
    };
}
