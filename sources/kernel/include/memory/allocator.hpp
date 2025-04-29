#pragma once

#include "memory/detail/tlsf.hpp"
#include "memory/layout.hpp"

namespace km {
    class MappingAllocation {
        km::TlsfAllocation mMemoryAllocation;
        km::TlsfAllocation mAddressAllocation;

    public:
        MappingAllocation(km::TlsfAllocation memory, km::TlsfAllocation address) noexcept
            : mMemoryAllocation(memory)
            , mAddressAllocation(address)
        {
            KM_ASSERT(!mMemoryAllocation.isNull());
            KM_ASSERT(!mAddressAllocation.isNull());
            KM_ASSERT(mMemoryAllocation.size() == mAddressAllocation.size());
        }

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
    };
}
