#pragma once

#include <stddef.h>

namespace x64 {
    struct page;
}

namespace km::detail {
    class PageTableList {
        x64::page *mTable;

    public:
        PageTableList(x64::page *table, size_t count);

        void push(x64::page *page);

        x64::page *next();

        x64::page *drain();

        x64::page *head();
    };
}
