#include <barrier>
#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

#include "system/thread.hpp"
#include "test_memory.hpp"

#include <thread>

struct MemoryState {
    km::PageAllocatorStats pmm;
    km::PteAllocatorStats vmm;
    size_t freeSpace;
};

static inline sys::ProcessHandle *GetProcessHandle(sm::RcuSharedPtr<sys::Process> parent, OsProcessHandle handle) {
    sys::ProcessHandle *hChild = nullptr;
    OsStatus status = parent->findHandle(handle, &hChild);
    if (status != OsStatusSuccess) {
        return nullptr;
    }

    return hChild;
}

static inline sm::RcuSharedPtr<sys::Process> GetProcess(sm::RcuSharedPtr<sys::Process> parent, OsProcessHandle handle) {
    sys::ProcessHandle *hChild = GetProcessHandle(parent, handle);
    if (hChild == nullptr) {
        return nullptr;
    }

    return hChild->getProcess();
}

static inline sm::RcuSharedPtr<sys::Thread> GetThread(sm::RcuSharedPtr<sys::Process> parent, OsThreadHandle handle) {
    sys::ThreadHandle *hChild = nullptr;
    OsStatus status = parent->findHandle(handle, &hChild);
    if (status != OsStatusSuccess) {
        return nullptr;
    }

    return hChild->getThread();
}

class SystemBaseTest : public testing::Test {
public:
    void SetUp() override {
        km::detail::SetupXSave(km::SaveMode::eNoSave, 0);

        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
        body.addSegment(sm::megabytes(4).bytes(), boot::MemoryRegion::eUsable);
    }

    MemoryState GetMemoryState(km::SystemMemory& memory) {
        MemoryState state;
        state.pmm = memory.pmmAllocator().stats();
        state.vmm = memory.systemTables().TESTING_getPageTableAllocator().stats();
        state.freeSpace = memory.pageTables().stats().heap.freeMemory;
        return state;
    }

    void CheckMemoryState(km::SystemMemory& memory, const MemoryState& before) {
        (void)memory.systemTables().compact();
        auto after = GetMemoryState(memory);

        EXPECT_EQ(after.vmm.freeBlocks, before.vmm.freeBlocks) << "Virtual memory was leaked";
        EXPECT_EQ(after.pmm.heap.freeMemory, before.pmm.heap.freeMemory) << "Physical memory was leaked";
        EXPECT_EQ(after.freeSpace, before.freeSpace) << "Address space memory was leaked";
    }

    void RecordMemoryUsage(km::SystemMemory& memory) {
        before = GetMemoryState(memory);

        ASSERT_NE(before.pmm.heap.freeMemory, 0) << "PMM is empty";
        ASSERT_NE(before.vmm.freeBlocks, 0) << "VMM is empty";
        ASSERT_NE(before.freeSpace, 0) << "Address space is empty";
    }

    void CheckMemoryUsage(km::SystemMemory& memory) {
        (void)memory.systemTables().compact();

        CheckMemoryState(memory, before);
    }

    void DebugMemoryUsage(km::SystemMemory& memory) {
        auto state = GetMemoryState(memory);

        std::cout << "PMM: " << std::string_view(km::format(sm::bytes(state.pmm.heap.freeMemory))) << std::endl;
        std::cout << "VMM: " << std::string_view(km::format(sm::bytes(state.vmm.freeBlocks * x64::kPageSize))) << std::endl;
        std::cout << "Address Space: " << std::string_view(km::format(sm::bytes(state.freeSpace))) << std::endl;
    }

    SystemMemoryTestBody body { sm::megabytes(4).bytes() };

    MemoryState before;
    MemoryState after;
};
