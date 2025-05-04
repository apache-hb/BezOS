#include "pvtest/memory.hpp"
#include "pvtest/pvtest.hpp"

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

pv::Memory::Memory(off64_t memorySize, off64_t guestMemorySize)
    : mMemorySize(memorySize)
    , mGuestMemorySize(guestMemorySize)
{
    PV_POSIX_CHECK((mMemoryFd = memfd_create("pv_ram", 0)));

    PV_POSIX_CHECK(ftruncate64(mMemoryFd, getMemorySize()));

    PV_POSIX_ASSERT((mHostMemory = mmap(nullptr, getMemorySize(), PROT_READ | PROT_WRITE, MAP_SHARED, mMemoryFd, 0)) != MAP_FAILED);

    PV_POSIX_ASSERT((mGuestMemory = mmap(nullptr, getGuestMemorySize(), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) != MAP_FAILED);
}

pv::Memory::~Memory() {
    munmap(mGuestMemory, getMemorySize());
    munmap(mHostMemory, getMemorySize());
    close(mMemoryFd);
}

km::AddressMapping pv::Memory::addSection(boot::MemoryRegion section) {
    auto front = section.range.front;
    sm::VirtualAddress host = mHostMemory + front.address;
    mSections.push_back(section);
    return km::AddressMapping {
        .vaddr = host,
        .paddr = front,
        .size = section.range.size(),
    };
}

void *pv::Memory::mapGuestPage(sm::VirtualAddress address, km::PageFlags access, sm::PhysicalAddress memory) {
    int prot = [&] {
        int result = PROT_NONE;
        if (bool(access & km::PageFlags::eRead)) {
            result |= PROT_READ;
        }
        if (bool(access & km::PageFlags::eWrite)) {
            result |= PROT_WRITE;
        }
        if (bool(access & km::PageFlags::eExecute)) {
            result |= PROT_EXEC;
        }
        return result;
    }();

    void *result = mmap(address, x64::kPageSize, prot, MAP_SHARED | MAP_FIXED, mMemoryFd, memory.address);
    if (result == MAP_FAILED) {
        PV_POSIX_ERROR(errno, "mmap failed: %s\n", strerror(errno));
        return nullptr;
    }
    return result;
}
