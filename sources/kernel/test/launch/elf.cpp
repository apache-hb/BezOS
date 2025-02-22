#include <gtest/gtest.h>

#include <fstream>

#include "elf.hpp"
#include "drivers/block/ramblk.hpp"
#include "std/shared.hpp"
#include "fs2/vfs.hpp"
#include "fs2/tarfs.hpp"

#include "test/test_memory.hpp"

TEST(ElfTest, LoadElf) {
    std::vector<std::byte> data;
    std::ifstream archive(getenv("TAR_TEST_ARCHIVE"), std::ios::binary);
    ASSERT_TRUE(archive.is_open()) << "Failed to open archive: " << getenv("TAR_TEST_ARCHIVE");

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

    SystemMemoryTestBody body;

    body.addSegment(sm::megabytes(16).bytes(), boot::MemoryRegion::eUsable);

    km::SystemMemory memory = body.make(sm::megabytes(4).bytes(), sm::megabytes(4).bytes());

    km::SystemObjects objects;

    km::ProcessLaunch launch{};
    OsStatus status = km::LoadElf(std::move(elf), memory, objects, &launch);
    ASSERT_EQ(OsStatusSuccess, status);
}
