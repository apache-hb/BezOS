#pragma once

#include "memory/memory.hpp"
#include "pvtest/system.hpp"

#include "boot.hpp"
#include "common/util/util.hpp"

#include <unistd.h>

#include <vector>

namespace pv {
    class Memory {
        int mMemoryFd;
        sm::VirtualAddress mHostMemory;
        off64_t mMemorySize;
        std::vector<boot::MemoryRegion> mSections;

        /// @brief Area of virtual memory that is reserved for the guest to use.
        sm::VirtualAddress mGuestMemory;
        off64_t mGuestMemorySize;

    public:
        UTIL_NOCOPY(Memory);
        UTIL_NOMOVE(Memory);

        Memory(off64_t memorySize, off64_t guestMemorySize = sm::gigabytes(64).bytes());
        ~Memory();

        PVTEST_SHARED_OBJECT(Memory);

        /// @brief Add a section and return the host address of the section.
        km::AddressMapping addSection(boot::MemoryRegion section);

        std::span<const boot::MemoryRegion> getSections() const noexcept { return mSections; }

        off64_t getMemorySize() const noexcept { return mMemorySize; }

        sm::VirtualAddress getHostMemory() const noexcept { return mHostMemory; }

        sm::VirtualAddress getHostAddress(km::PhysicalAddressEx address) const noexcept {
            return mHostMemory + address.address;
        }

        sm::PhysicalAddress getMemoryAddress(sm::VirtualAddress address) const noexcept {
            return sm::PhysicalAddress { static_cast<uintptr_t>(address - mHostMemory) };
        }

        sm::VirtualAddress getGuestMemory() const noexcept {
            return mGuestMemory;
        }

        off64_t getGuestMemorySize() const noexcept {
            return mGuestMemorySize;
        }

        km::VirtualRangeEx getGuestRange() const noexcept {
            return km::VirtualRangeEx::of(getGuestMemory(), getGuestMemorySize());
        }

        void *mapGuestPage(sm::VirtualAddress address, km::PageFlags access, sm::PhysicalAddress memory);
    };
}
