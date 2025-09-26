#pragma once

#include "memory/detail/tlsf.hpp"
#include "std/std.hpp"

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
        OsStatus split(TlsfAllocation allocation, PhysicalAddress midpoint, TlsfAllocation *lo [[outparam]], TlsfAllocation *hi [[outparam]]) noexcept [[clang::allocating]] {
            return split(allocation, midpoint.address, lo, hi);
        }

        [[nodiscard]]
        OsStatus split(TlsfAllocation allocation, uintptr_t midpoint, TlsfAllocation *lo [[outparam]], TlsfAllocation *hi [[outparam]]) noexcept [[clang::allocating]];

        [[nodiscard]]
        OsStatus splitv(TlsfAllocation allocation, std::span<const PhysicalAddress> points, std::span<TlsfAllocation> results) noexcept [[clang::allocating]];

        void commit() noexcept [[clang::nonallocating]];

        [[nodiscard]]
        OsStatus validate() const noexcept [[clang::nonallocating]];

        TlsfHeapCommandListStats stats() const noexcept [[clang::nonallocating]];
    };

    template<typename T>
    class GenericTlsfHeapCommandList {
        TlsfHeapCommandList mList;

    public:
        using Heap = GenericTlsfHeap<T>;
        using Allocation = GenericTlsfAllocation<T>;
        using Address = T;
        using Range = AnyRange<T>;

        UTIL_NOCOPY(GenericTlsfHeapCommandList);

        constexpr GenericTlsfHeapCommandList() noexcept = default;

        constexpr GenericTlsfHeapCommandList(Heap *heap [[gnu::nonnull]]) noexcept [[clang::nonallocating]]
            : mList(&heap->mHeap)
        { }

        GenericTlsfHeapCommandList(GenericTlsfHeapCommandList&& other) noexcept [[clang::nonallocating]]
            : mList(std::move(other.mList))
        { }

        GenericTlsfHeapCommandList& operator=(GenericTlsfHeapCommandList&& other) noexcept [[clang::nonallocating]] {
            if (this != &other) {
                mList = std::move(other.mList);
            }

            return *this;
        }

        [[nodiscard]]
        OsStatus split(Allocation allocation, Address midpoint, Allocation *lo [[outparam]], Allocation *hi [[outparam]]) noexcept [[clang::allocating]] {
            TlsfAllocation *loAlloc = std::bit_cast<TlsfAllocation*>(lo);
            TlsfAllocation *hiAlloc = std::bit_cast<TlsfAllocation*>(hi);

            return mList.split(TlsfAllocation{allocation.getBlock()}, midpoint.address, loAlloc, hiAlloc);
        }

        [[nodiscard]]
        OsStatus splitv(Allocation allocation, std::span<const Address> points, std::span<Allocation> results) noexcept [[clang::allocating]] {
            const PhysicalAddress *ptrIn = std::bit_cast<const PhysicalAddress*>(points.data());
            std::span<const PhysicalAddress> p { ptrIn, points.size() };

            Allocation *resIn = std::bit_cast<Allocation*>(results.data());
            std::span<Allocation> r { resIn, results.size() };
            return mList.splitv(allocation, p, r);
        }

        void commit() noexcept [[clang::nonallocating]] {
            mList.commit();
        }

        [[nodiscard]]
        OsStatus validate() const noexcept [[clang::nonallocating]] {
            return mList.validate();
        }

        TlsfHeapCommandListStats stats() const noexcept [[clang::nonallocating]] {
            return mList.stats();
        }
    };
}
