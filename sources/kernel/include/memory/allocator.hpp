#pragma once

#include "memory/detail/tlsf.hpp"
#include "memory/layout.hpp"

namespace km {
    class MappingAllocation {
        km::TlsfAllocation mMemoryAllocation;
        km::TlsfAllocation mAddressAllocation;

        constexpr MappingAllocation(km::TlsfAllocation memory, km::TlsfAllocation address) noexcept
            : mMemoryAllocation(memory)
            , mAddressAllocation(address)
        { }
    public:
        constexpr MappingAllocation() noexcept = default;

        km::PhysicalAddress baseMemory() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.address();
        }

        km::MemoryRange memory() const noexcept [[clang::nonblocking]] {
            return mMemoryAllocation.range();
        }

        sm::VirtualAddress baseAddress() const noexcept [[clang::nonblocking]] {
            return std::bit_cast<sm::VirtualAddress>(mAddressAllocation.address());
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
                .paddr = baseMemory(),
                .size = size(),
            };
        }

        [[nodiscard]]
        static OsStatus create(TlsfAllocation memory, TlsfAllocation address, MappingAllocation *result) {
            bool valid = memory.isValid() && address.isValid() && (memory.size() == address.size());
            if (!valid) {
                return OsStatusInvalidInput;
            }
            *result = MappingAllocation::unchecked(memory, address);
            return OsStatusSuccess;
        }

        static MappingAllocation unchecked(TlsfAllocation memory, TlsfAllocation address) {
            KM_ASSERT(memory.isValid());
            KM_ASSERT(address.isValid());
            KM_ASSERT(memory.size() == address.size());

            return MappingAllocation { memory, address };
        }
    };
}
