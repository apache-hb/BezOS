#include <barrier>
#include <gtest/gtest.h>

#include "system/system.hpp"
#include "system/process.hpp"
#include "system/schedule.hpp"

#include "system/thread.hpp"
#include "test/test_memory.hpp"

#include <thread>

struct MemoryState {
    km::PageAllocatorStats pmm;
    km::PteAllocatorStats vmm;
    size_t freeSpace;
};

static inline sys2::ProcessHandle *GetProcessHandle(sm::RcuSharedPtr<sys2::Process> parent, OsProcessHandle handle) {
    sys2::ProcessHandle *hChild = nullptr;
    OsStatus status = parent->findHandle(handle, &hChild);
    if (status != OsStatusSuccess) {
        return nullptr;
    }

    return hChild;
}

static inline sm::RcuSharedPtr<sys2::Process> GetProcess(sm::RcuSharedPtr<sys2::Process> parent, OsProcessHandle handle) {
    sys2::ProcessHandle *hChild = GetProcessHandle(parent, handle);
    if (hChild == nullptr) {
        return nullptr;
    }

    return hChild->getProcess();
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
        state.freeSpace = memory.pageTables().TESTING_getVmemAllocator().freeSpace();
        return state;
    }

    void CheckMemoryState(km::SystemMemory& memory, const MemoryState& before) {
        (void)memory.systemTables().compact();
        auto after = GetMemoryState(memory);

        EXPECT_EQ(after.vmm.freeBlocks, before.vmm.freeBlocks) << "Virtual memory was leaked";
        EXPECT_EQ(after.pmm.freeMemory, before.pmm.freeMemory) << "Physical memory was leaked";
        EXPECT_EQ(after.freeSpace, before.freeSpace) << "Address space memory was leaked";
    }

    void RecordMemoryUsage(km::SystemMemory& memory) {
        before = GetMemoryState(memory);
    }

    void CheckMemoryUsage(km::SystemMemory& memory) {
        (void)memory.systemTables().compact();

        CheckMemoryState(memory, before);
    }

    void DebugMemoryUsage(km::SystemMemory& memory) {
        auto state = GetMemoryState(memory);

        std::cout << "PMM: " << state.pmm.freeMemory << std::endl;
        std::cout << "VMM: " << state.vmm.freeBlocks << std::endl;
        std::cout << "Address Space: " << std::string_view(km::format(sm::bytes(state.freeSpace))) << std::endl;
    }

    SystemMemoryTestBody body { sm::megabytes(4).bytes() };

    MemoryState before;
    MemoryState after;
};
