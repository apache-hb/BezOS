#include "kernel.hpp"
#include "memory/vmm_heap.hpp"
#include "drivers/block/ramblk.hpp"
#include "std/shared.hpp"
#include "fs/vfs.hpp"
#include "fs/tarfs.hpp"

#include <ktest/ktest.h>

class SystemTest : public km::testing::Test {

};

KTEST_F(SystemTest, RunProgram) {
    auto launchInfo = km::testing::getLaunchInfo();
    auto *systemMemory = km::GetSystemMemory();
    auto& vmm = systemMemory->pageTables();
    vfs::VfsRoot vfsRoot;
    auto initrd = launchInfo.initrd;

    km::VmemAllocation initrdMemory;
    OsStatus status = vmm.map(km::alignedOut(initrd, x64::kPageSize), km::PageFlags::eRead, km::MemoryType::eWriteBack, &initrdMemory);
    KASSERT_EQ(OsStatusSuccess, status);

    sm::SharedPtr<km::MemoryBlk> block = new km::MemoryBlk{std::bit_cast<std::byte*>(initrdMemory.address()), initrd.size()};

    vfs::IVfsMount *mount = nullptr;
    status = vfsRoot.addMountWithParams(&vfs::TarFs::instance(), vfs::BuildPath("Init"), &mount, block);
    KASSERT_EQ(OsStatusSuccess, status);
}
