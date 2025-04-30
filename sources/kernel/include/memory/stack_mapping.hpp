#pragma once

#include "memory/detail/tlsf.hpp"

namespace km {
    class StackMappingAllocation {
        km::TlsfAllocation mMemoryAllocation;
        km::TlsfAllocation mAddressAllocation;
        km::TlsfAllocation mGuardHead;
        km::TlsfAllocation mGuardTail;

        constexpr StackMappingAllocation(km::TlsfAllocation memory, km::TlsfAllocation address, km::TlsfAllocation head, km::TlsfAllocation tail) noexcept
            : mMemoryAllocation(memory)
            , mAddressAllocation(address)
            , mGuardHead(head)
            , mGuardTail(tail)
        { }
    public:
        constexpr StackMappingAllocation() noexcept = default;

        km::PhysicalAddressEx baseMemory() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<km::PhysicalAddressEx>(mMemoryAllocation.address());
        }

        km::MemoryRangeEx memory() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.range().cast<km::PhysicalAddressEx>();
        }

        km::VirtualRangeEx stack() const noexcept [[clang::nonblocking]] {
            return mAddressAllocation.range().cast<sm::VirtualAddress>();
        }

        sm::VirtualAddress stackBaseAddress() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<sm::VirtualAddress>(mAddressAllocation.address()) + mAddressAllocation.size();
        }

        size_t stackSize() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.size();
        }

        size_t totalSize() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.size() + (mGuardHead.isValid() ? mGuardHead.size() : 0) + (mGuardTail.isValid() ? mGuardTail.size() : 0);
        }

        km::TlsfAllocation guardHead() const noexcept [[clang::nonblocking]] {
            return mGuardHead;
        }

        bool hasGuardHead() const noexcept [[clang::nonblocking]] {
            return mGuardHead.isValid();
        }

        km::TlsfAllocation guardTail() const noexcept [[clang::nonblocking]] {
            return mGuardTail;
        }

        bool hasGuardTail() const noexcept [[clang::nonblocking]] {
            return mGuardTail.isValid();
        }

        km::TlsfAllocation memoryAllocation() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation;
        }

        km::TlsfAllocation stackAllocation() const noexcept [[clang::nonblocking]] {
            return mAddressAllocation;
        }

        [[nodiscard]]
        static OsStatus create(TlsfAllocation memory, TlsfAllocation address, TlsfAllocation head, TlsfAllocation tail, StackMappingAllocation *result) {
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

        static StackMappingAllocation unchecked(TlsfAllocation memory, TlsfAllocation address, TlsfAllocation head, TlsfAllocation tail) {
            KM_ASSERT(memory.isValid());
            KM_ASSERT(address.isValid());
            KM_ASSERT(memory.size() == address.size());

            if (head.isValid()) {
                KM_ASSERT(head.range().back == address.range().front);
            }

            if (tail.isValid()) {
                KM_ASSERT(tail.range().front == address.range().back);
            }

            return StackMappingAllocation(memory, address, head, tail);
        }
    };
}
