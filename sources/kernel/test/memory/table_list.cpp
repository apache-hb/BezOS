#include <gtest/gtest.h>

#include "memory/detail/table_list.hpp"
#include "arch/paging.hpp"

using km::PageTableAllocation;

TEST(TableListTest, TableList) {
    std::unique_ptr<x64::page[]> tables(new x64::page[10]);

    km::detail::PageTableList list(tables.get(), 10);

    for (size_t i = 0; i < 10; i++) {
        PageTableAllocation table = list.next();
        ASSERT_EQ(table.getVirtual(), tables.get() + i);
    }
}

TEST(TableListTest, AddList) {
    std::unique_ptr<x64::page[]> tables0(new x64::page[10]);
    std::unique_ptr<x64::page[]> tables1(new x64::page[10]);

    auto isValid = [&](PageTableAllocation table) {
        const x64::page *t0begin = tables0.get();
        const x64::page *t0end = tables0.get() + 10;

        const x64::page *t1begin = tables1.get();
        const x64::page *t1end = tables1.get() + 10;

        return (t0begin <= table.getVirtual() && table.getVirtual() < t0end) || (t1begin <= table.getVirtual() && table.getVirtual() < t1end);
    };

    km::detail::PageTableList list(tables0.get(), 10);
    for (size_t i = 0; i < 10; i++) {
        list.push(km::PageTableAllocation{tables1.get() + i, 0});
    }

    for (size_t i = 0; i < 10; i++) {
        PageTableAllocation table = list.next();
        ASSERT_EQ((uintptr_t)table.getVirtual() % sizeof(x64::page), 0);
        ASSERT_TRUE(isValid(table));
    }

    for (size_t i = 0; i < 10; i++) {
        PageTableAllocation table = list.next();
        ASSERT_EQ((uintptr_t)table.getVirtual() % sizeof(x64::page), 0);
        ASSERT_TRUE(isValid(table));
    }
}
