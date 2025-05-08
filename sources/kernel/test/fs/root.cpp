#include <gtest/gtest.h>

#include "fs/vfs.hpp"

using namespace vfs;

TEST(VfsRootTest, Construct) {
    vfs::VfsRoot vfs;

    {
        sm::RcuSharedPtr<INode> node = nullptr;
        OsStatus status = vfs.create(BuildPath("inventory.txt"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    std::unique_ptr<IFileHandle> inventory;
    OsStatus status = vfs.open(BuildPath("inventory.txt"), std::out_ptr(inventory));
    ASSERT_EQ(status, OsStatusSuccess);
}

TEST(VfsRootTest, QueryRoot) {
    vfs::VfsRoot vfs;

    std::unique_ptr<vfs::IHandle> handle = nullptr;
    OsStatus status = vfs.device(vfs::VfsPath{}, kOsIdentifyGuid, nullptr, 0, std::out_ptr(handle));
    ASSERT_EQ(OsStatusSuccess, status);
}
