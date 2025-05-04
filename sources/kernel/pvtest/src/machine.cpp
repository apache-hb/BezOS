#include "pvtest/machine.hpp"
#include "pvtest/system.hpp"
#include "pvtest/pvtest.hpp"

#include <atomic>
#include <format>
#include <sys/mman.h>
#include <rpmalloc.h>

static int gSharedMemoryFd = -1;
static sm::VirtualAddress gSharedHostMemory = nullptr;
static std::atomic<uintptr_t> gSharedHostBase;
static constexpr size_t kSharedMemorySize = sm::gigabytes(1).bytes();

static void *RpMallocSharedMap(size_t size, size_t align, size_t *offset, size_t *mappedSize) {
    size_t mapSize = size + align;
    sm::VirtualAddress result = nullptr;
    size_t resultSize = 0;
    size_t resultOffset = 0;

    if (mapSize > kSharedMemorySize) {
        fprintf(stderr, "rpmalloc: too large %zu\n", size);
        return nullptr;
    }

    result = gSharedHostBase.fetch_add(mapSize);
    if (result + mapSize > (gSharedHostMemory + kSharedMemorySize)) {
        fprintf(stderr, "rpmalloc: out of memory %p %zu\n", (void*)result, mapSize);
        return nullptr;
    }

    if (align) {
        size_t padding = (result.address & (uintptr_t)(align - 1));
        if (padding)
            padding = align - padding;
        result += padding;
        resultOffset = padding;
    }

    *offset = resultOffset;
    *mappedSize = resultSize;
    return result;
}

static void RpMallocSharedUnmap(void *ptr, size_t offset, size_t size) {
    fprintf(stderr, "rpmalloc: unmap %p %zu %zu\n", ptr, offset, size);
}

static void RpMallocError(const char *msg) {
    fprintf(stderr, "rpmalloc: error %s\n", msg);
}

static void RpMallocCommit(void *ptr, size_t size) {
    fprintf(stderr, "rpmalloc: commit %p %zu\n", ptr, size);
}

static void RpMallocDecommit(void *ptr, size_t size) {
    fprintf(stderr, "rpmalloc: decommit %p %zu\n", ptr, size);
}

static int RpMallocMapFailCallback(size_t size) {
    fprintf(stderr, "rpmalloc: map fail %zu\n", size);
    return false;
}

static rpmalloc_interface_t gMallocInterface {
    .memory_map = RpMallocSharedMap,
    .memory_commit = RpMallocCommit,
    .memory_decommit = RpMallocDecommit,
    .memory_unmap = RpMallocSharedUnmap,
    .map_fail_callback = RpMallocMapFailCallback,
    .error_callback = RpMallocError,
};

pv::Machine::Machine(size_t cores, off64_t memorySize)
    : mCores(cores)
    , mMemory(memorySize)
{ }

km::VirtualRangeEx pv::Machine::getSharedMemory() {
    return km::VirtualRangeEx::of(gSharedHostMemory, kSharedMemorySize);
}

void pv::Machine::init() {
    PV_POSIX_CHECK((gSharedMemoryFd = memfd_create("pv_host", 0)));
    PV_POSIX_CHECK(ftruncate64(gSharedMemoryFd, kSharedMemorySize));
    PV_POSIX_ASSERT((gSharedHostMemory = mmap(nullptr, kSharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, gSharedMemoryFd, 0)) != MAP_FAILED);
    gSharedHostBase = gSharedHostMemory.address;

    if (int err = rpmalloc_initialize(&gMallocInterface)) {
        throw std::runtime_error(std::format("rpmalloc_initialize: {}", err));
    }
}

void pv::Machine::finalize() {
    rpmalloc_dump_statistics(stderr);
    rpmalloc_finalize();
}

void pv::Machine::initChild() {
    rpmalloc_thread_initialize();
}

void pv::Machine::finalizeChild() {
    rpmalloc_thread_finalize();
}

void *pv::SharedObjectMalloc(size_t size) {
    return rpmalloc(size);
}

void *pv::SharedObjectAlignedAlloc(size_t align, size_t size) {
    return rpaligned_alloc(align, size);
}

void pv::SharedObjectFree(void *ptr) {
    return rpfree(ptr);
}
