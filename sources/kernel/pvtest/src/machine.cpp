#include "pvtest/machine.hpp"
#include "pvtest/system.hpp"
#include "pvtest/pvtest.hpp"

#include <format>
#include <sys/mman.h>
#include <rpmalloc.h>

static int gSharedMemoryFd = -1;
static sm::VirtualAddress gSharedHostMemory = nullptr;
static constexpr size_t kSharedMemorySize = sm::gigabytes(1).bytes();
static std::once_flag gSharedMemoryOnceFlag;

static void *RpMallocSharedMap(size_t size, size_t align, size_t *offset, size_t *mappedSize) {
    void *result = nullptr;
    size_t resultSize = 0;

    if (size > kSharedMemorySize) {
        printf("rpmalloc: size %zu\n", size);
        return nullptr;
    }

    if (align != 0) {
        printf("rpmalloc: align %zu\n", align);
        return nullptr;
    }

    std::call_once(gSharedMemoryOnceFlag, [&] {
        result = gSharedHostMemory;
        resultSize = kSharedMemorySize;
    });

    *offset = 0;
    *mappedSize = resultSize;
    return result;
}

static void RpMallocSharedUnmap(void *ptr, size_t offset, size_t size) {
    printf("rpmalloc: unmap %p %zu %zu\n", ptr, offset, size);
}

pv::Machine::Machine(size_t cores, off64_t memorySize)
    : mCores(cores)
    , mMemory(memorySize)
{ }

void pv::Machine::init() {
    PV_POSIX_CHECK((gSharedMemoryFd = memfd_create("pv_host", 0)));
    PV_POSIX_CHECK(ftruncate64(gSharedMemoryFd, kSharedMemorySize));
    PV_POSIX_ASSERT((gSharedHostMemory = mmap(nullptr, kSharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, gSharedMemoryFd, 0)) != MAP_FAILED);

    rpmalloc_interface_t rpmallocInterface {
        .memory_map = RpMallocSharedMap,
        .memory_unmap = RpMallocSharedUnmap,
    };

    if (int err = rpmalloc_initialize(&rpmallocInterface)) {
        throw std::runtime_error(std::format("rpmalloc_initialize: {}", err));
    }

    pv::CpuCore::installSignals();
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
