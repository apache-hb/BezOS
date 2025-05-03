#include "pvtest/memory.hpp"
#include "pvtest/pvtest.hpp"

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

pv::Memory::Memory(off64_t size) : mMemorySize(size) {
    PV_POSIX_CHECK((mMemoryFd = memfd_create("pvtest", 0)));

    PV_POSIX_CHECK(ftruncate64(mMemoryFd, getMemorySize()));

    PV_POSIX_ASSERT((mHostMemory = mmap(nullptr, getMemorySize(), PROT_READ | PROT_WRITE, MAP_SHARED, mMemoryFd, 0)) != MAP_FAILED);
}

pv::Memory::~Memory() {
    munmap(mHostMemory, getMemorySize());
    close(mMemoryFd);
}
