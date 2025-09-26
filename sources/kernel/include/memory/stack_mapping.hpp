#pragma once

#include "log.hpp"
#include "logger/categories.hpp"
#include "memory/detail/tlsf.hpp"
#include "memory/pmm_heap.hpp"
#include "memory/vmm_heap.hpp"

namespace km {
    class StackMappingAllocation {
        km::PmmAllocation mMemoryAllocation;
        km::VmemAllocation mAddressAllocation;
        km::VmemAllocation mGuardHead;
        km::VmemAllocation mGuardTail;

        StackMappingAllocation(km::PmmAllocation memory, km::VmemAllocation address, km::VmemAllocation head, km::VmemAllocation tail) noexcept
            : mMemoryAllocation(memory)
            , mAddressAllocation(address)
            , mGuardHead(head)
            , mGuardTail(tail)
        {
            KM_ASSERT(memory.isValid());
            KM_ASSERT(address.isValid());

            if (memory.size() != address.size()) {
                MemLog.fatalf("Memory size: ", memory.size(), " != total size: ", address.size());
                KM_ASSERT(memory.size() == address.size());
            }

            if (head.isValid()) {
                if (head.range().back != address.range().front) {
                    MemLog.fatalf("Head: ", head.range(), " != Address: ", address.range());
                }

                KM_ASSERT(head.range().back == address.range().front);
            }

            if (tail.isValid()) {
                if (tail.range().front != address.range().back) {
                    MemLog.fatalf("Tail: ", tail.range(), " != Address: ", address.range());
                }

                KM_ASSERT(tail.range().front == address.range().back);
            }
        }
    public:
        constexpr StackMappingAllocation() noexcept = default;

        km::StackMapping stackMapping() const noexcept [[clang::nonblocking]] {
            km::AddressMapping mapping {
                .vaddr = std::bit_cast<const void*>(baseAddress()),
                .paddr = std::bit_cast<km::PhysicalAddress>(baseMemory()),
                .size = mMemoryAllocation.size()
            };

            auto range = mAddressAllocation.range().cast<sm::VirtualAddress>();

            if (hasGuardHead()) {
                range.front -= mGuardHead.size();
            }

            if (hasGuardTail()) {
                range.back += mGuardTail.size();
            }

            return km::StackMapping {
                .stack = mapping,
                .total = range.cast<const void*>(),
            };
        }

        km::PhysicalAddressEx baseMemory() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<km::PhysicalAddressEx>(mMemoryAllocation.address());
        }

        km::MemoryRangeEx memory() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.range().cast<km::PhysicalAddressEx>();
        }

        km::VirtualRangeEx stack() const noexcept [[clang::nonblocking]] {
            return mAddressAllocation.range().cast<sm::VirtualAddress>();
        }

        sm::VirtualAddress baseAddress() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<sm::VirtualAddress>(mAddressAllocation.address());
        }

        sm::VirtualAddress stackBaseAddress() const noexcept [[clang::nonblocking]] {
            return baseAddress() + mAddressAllocation.size();
        }

        size_t stackSize() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.size();
        }

        size_t totalSize() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.size() + (hasGuardHead() ? mGuardHead.size() : 0) + (hasGuardTail() ? mGuardTail.size() : 0);
        }

        km::VmemAllocation guardHead() const noexcept [[clang::nonblocking]] {
            return mGuardHead;
        }

        bool hasGuardHead() const noexcept [[clang::nonblocking]] {
            return mGuardHead.isValid();
        }

        km::VmemAllocation guardTail() const noexcept [[clang::nonblocking]] {
            return mGuardTail;
        }

        bool hasGuardTail() const noexcept [[clang::nonblocking]] {
            return mGuardTail.isValid();
        }

        km::PmmAllocation memoryAllocation() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation;
        }

        km::VmemAllocation stackAllocation() const noexcept [[clang::nonblocking]] {
            return mAddressAllocation;
        }

        [[nodiscard]]
        static OsStatus create(PmmAllocation memory, VmemAllocation address, VmemAllocation head, VmemAllocation tail, StackMappingAllocation *result [[outparam]]) {
            if (!memory.isValid() || !address.isValid() || (memory.size() != address.size())) {
                return OsStatusInvalidInput;
            }

            if (head.isValid() && (head.range().back != address.range().front)) {
                return OsStatusInvalidInput;
            }

            if (tail.isValid() && (tail.range().front != address.range().back)) {
                return OsStatusInvalidInput;
            }

            *result = StackMappingAllocation::unchecked(memory, address, head, tail);
            return OsStatusSuccess;
        }

        static StackMappingAllocation unchecked(PmmAllocation memory, VmemAllocation address, VmemAllocation head, VmemAllocation tail) {
            return StackMappingAllocation{memory, address, head, tail};
        }
    };
}
