#include <gtest/gtest.h>

#include <fstream>

#include "elf.hpp"
#include "drivers/block/ramblk.hpp"
#include "allocator/tlsf.hpp"
#include "hypervisor.hpp"
#include "std/shared.hpp"
#include "fs2/vfs.hpp"
#include "fs2/tarfs.hpp"

static km::PageMemoryTypeLayout GetDefaultPatLayout(void) {
    enum {
        kEntryWriteBack,
        kEntryWriteThrough,
        kEntryUncachedOverridable,
        kEntryUncached,
        kEntryWriteCombined,
        kEntryWriteProtect,
    };

    return km::PageMemoryTypeLayout {
        .deferred = kEntryUncachedOverridable,
        .uncached = kEntryUncached,
        .writeCombined = kEntryWriteCombined,
        .writeThrough = kEntryWriteThrough,
        .writeProtect = kEntryWriteProtect,
        .writeBack = kEntryWriteBack,
    };
}

TEST(ElfTest, LoadElf) {
    std::vector<std::byte> data;
    std::ifstream archive(getenv("TAR_TEST_ARCHIVE"), std::ios::binary);
    archive.seekg(0, std::ios::end);
    data.resize(archive.tellg());
    archive.seekg(0, std::ios::beg);
    archive.read(reinterpret_cast<char*>(data.data()), data.size());

    sm::SharedPtr<km::MemoryBlk> block = new km::MemoryBlk(data.data(), data.size());

    vfs2::VfsRoot vfs;

    {
        vfs2::IVfsMount *mount = nullptr;
        OsStatus status = vfs.addMountWithParams(&vfs2::TarFs::instance(), vfs2::BuildPath("Init"), &mount, block);
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(mount, nullptr);
    }

    std::unique_ptr<vfs2::IVfsNodeHandle> elf;
    {
        OsStatus status = vfs.open(vfs2::BuildPath("Init", "init.elf"), std::out_ptr(elf));
        ASSERT_EQ(OsStatusSuccess, status);
        ASSERT_NE(elf, nullptr);
    }

    static constexpr size_t kSystemAreaSize = sm::megabytes(2).bytes();
    static constexpr size_t kUserAreaSize = sm::megabytes(2).bytes();
    static constexpr size_t kAllocAreaSize = sm::megabytes(2).bytes();

    std::unique_ptr<std::byte[]> userArea{new std::byte[kUserAreaSize]};
    km::MemoryRange userRange{(uintptr_t)userArea.get(), (uintptr_t)userArea.get() + kUserAreaSize};
    boot::MemoryRegion userRegion{boot::MemoryRegion::eUsable, userRange};
    std::array regions{userRegion};
    boot::MemoryMap memmap{regions};

    std::unique_ptr<std::byte[]> systemArea{new std::byte[kSystemAreaSize]};
    km::VirtualRange systemRange{systemArea.get(), systemArea.get() + kSystemAreaSize};

    auto cpuInfo = km::GetProcessorInfo();
    std::unique_ptr<std::byte[]> allocArea{new std::byte[kAllocAreaSize]};
    mem::TlsfAllocator allocator{allocArea.get(), kAllocAreaSize};

    km::PageBuilder pm { cpuInfo.maxpaddr, cpuInfo.maxvaddr, 0, GetDefaultPatLayout() };

    km::SystemMemory memory { memmap, systemRange, pm, &allocator };
    km::SystemObjects objects;

    km::ProcessLaunch launch{};
    OsStatus status = km::LoadElf(std::move(elf), memory, objects, &launch);
    ASSERT_EQ(OsStatusSuccess, status);
}
