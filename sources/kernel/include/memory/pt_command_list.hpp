#pragma once

#include <bezos/status.h>

#include "memory/detail/table_list.hpp"
#include "memory/memory.hpp"
#include "memory/range.hpp"

#include "std/inlined_vector.hpp"

namespace km {
    class PageTables;

    namespace detail {
        struct PageTableCommand {
            enum Command {
                eMap,
                eUnmap,
            };

            Command command;
            VirtualRange range;
            PhysicalAddress base;
            PageFlags flags;
            MemoryType type;
        };
    }

    struct PageTableCommandListStats {
        size_t commandCount;
        size_t tableCount;
    };

    class PageTableCommandList {
        PageTables *mTables;
        detail::PageTableList mStorage;
        sm::InlinedVector<detail::PageTableCommand, 8> mCommands;

        [[nodiscard]]
        OsStatus add(detail::PageTableCommand command) noexcept [[clang::allocating]];

    public:
        UTIL_NOCOPY(PageTableCommandList);

        constexpr PageTableCommandList() noexcept = default;
        constexpr PageTableCommandList(PageTables *tables [[gnu::nonnull]]) noexcept
            : mTables(tables)
        { }

        ~PageTableCommandList() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus validate() const noexcept [[clang::nonallocating]];

        void commit() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus unmap(VirtualRange range) noexcept [[clang::allocating]];

        [[nodiscard]]
        OsStatus map(AddressMapping mapping, PageFlags flags, MemoryType type) noexcept [[clang::allocating]];

        PageTableCommandListStats stats() const noexcept [[clang::nonallocating]];
    };
}
