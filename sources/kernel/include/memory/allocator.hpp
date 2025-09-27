#pragma once

#include "log.hpp"
#include "logger/categories.hpp"
#include "memory/detail/tlsf.hpp"
#include "memory/layout.hpp"
#include "memory/pmm_heap.hpp"
#include "memory/vmm_heap.hpp"

namespace km {
    class MappingAllocation {
        km::PmmAllocation mMemoryAllocation;
        km::TlsfAllocation mAddressAllocation;

        constexpr MappingAllocation(km::PmmAllocation memory, km::TlsfAllocation address) noexcept
            : mMemoryAllocation(memory)
            , mAddressAllocation(address)
        { }
    public:
        constexpr MappingAllocation() noexcept = default;

        km::PhysicalAddressEx baseMemory() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<km::PhysicalAddressEx>(mMemoryAllocation.address());
        }

        km::MemoryRangeEx memory() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.range().cast<km::PhysicalAddressEx>();
        }

        sm::VirtualAddress baseAddress() const noexcept [[clang::nonblocking]] {
            return sm::VirtualAddress{mAddressAllocation.offset()};
        }

        km::VirtualRangeEx range() const noexcept [[clang::nonblocking]] {
            return mAddressAllocation.range().cast<sm::VirtualAddress>();
        }

        size_t size() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.size();
        }

        km::AddressMapping mapping() const noexcept [[clang::nonblocking]] {
            return km::AddressMapping {
                .vaddr = baseAddress(),
                .paddr = std::bit_cast<km::PhysicalAddress>(baseMemory()),
                .size = size(),
            };
        }

        km::PmmAllocation memoryAllocation() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation;
        }

        km::TlsfAllocation virtualAllocation() const noexcept [[clang::nonblocking]] {
            return mAddressAllocation;
        }

        [[nodiscard]]
        static OsStatus create(PmmAllocation memory, TlsfAllocation address, MappingAllocation *result [[outparam]]) {
            bool valid = memory.isValid() && address.isValid() && (memory.size() == address.size());
            if (!valid) {
                return OsStatusInvalidInput;
            }
            *result = MappingAllocation::unchecked(memory, address);
            return OsStatusSuccess;
        }

        static MappingAllocation unchecked(PmmAllocation memory, TlsfAllocation address) {
            KM_ASSERT(memory.isValid());
            KM_ASSERT(address.isValid());
            if (memory.size() != address.size()) {
                MemLog.fatalf("Memory and address allocations must be the same size. ", memory.size(), " != ", address.size(), "\n");
                KM_PANIC("Memory and address allocations must be the same size.");
            }

            return MappingAllocation { memory, address };
        }
    };
}
