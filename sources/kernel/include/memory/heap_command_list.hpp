#pragma once

#include "memory/detail/tlsf.hpp"

#include "std/inlined_vector.hpp"

namespace km {
    namespace detail {
        struct TlsfHeapCommand {
            enum Command {
                eSplit,
                eSplitVector,
            };

            Command command;
            TlsfAllocation subject;
            sm::InlinedVector<PhysicalAddress, 4> midpoints;

            union Result {
                Result() noexcept [[clang::nonallocating]] { }
                ~Result() noexcept [[clang::nonallocating]] { }

                std::span<TlsfAllocation> results;
                std::array<TlsfAllocation*, 2> split;
            };

            Result results;

            bool isSplitVector() const noexcept [[clang::nonblocking]] {
                return command == eSplitVector;
            }
        };
    }

    struct TlsfHeapCommandListStats {
        size_t commandCount;
        size_t blockCount;
    };

    class TlsfHeapCommandList {
        TlsfHeap *mHeap;
        detail::TlsfBlockList mStorage;
        sm::InlinedVector<detail::TlsfHeapCommand, 4> mCommands;

        void add(detail::TlsfHeapCommand command) noexcept [[clang::allocating]];

    public:
        UTIL_NOCOPY(TlsfHeapCommandList);

        constexpr TlsfHeapCommandList() noexcept = default;

        constexpr TlsfHeapCommandList(TlsfHeap *heap [[gnu::nonnull]]) noexcept [[clang::nonallocating]]
            : mHeap(heap)
        { }

        ~TlsfHeapCommandList() noexcept [[clang::nonallocating]];

        TlsfHeapCommandList(TlsfHeapCommandList&& other) noexcept [[clang::nonallocating]];
        TlsfHeapCommandList& operator=(TlsfHeapCommandList&& other) noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus split(TlsfAllocation allocation, PhysicalAddress midpoint, TlsfAllocation *lo, TlsfAllocation *hi) noexcept [[clang::allocating]];

        [[nodiscard]]
        OsStatus splitv(TlsfAllocation allocation, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results) noexcept [[clang::allocating]];

        void commit() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus validate() const noexcept [[clang::nonallocating]];

        TlsfHeapCommandListStats stats() const noexcept [[clang::nonallocating]];
    };
}
