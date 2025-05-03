#pragma once

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

    public:
        UTIL_NOCOPY(Memory);
        UTIL_NOMOVE(Memory);

        Memory(off64_t size);
        ~Memory();

        PVTEST_SHARED_OBJECT(Memory);

        /// @brief Add a section and return the host address of the section.
        void *addSection(boot::MemoryRegion section);

        std::span<const boot::MemoryRegion> getSections() const noexcept { return mSections; }
        off64_t getMemorySize() const noexcept { return mMemorySize; }
        sm::VirtualAddress getHostMemory() const noexcept { return mHostMemory; }
        sm::VirtualAddress getHostAddress(km::PhysicalAddressEx address) const noexcept {
            return mHostMemory + address.address;
        }
    };
}
