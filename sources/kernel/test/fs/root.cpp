#include <gtest/gtest.h>

#include "fs2/vfs.hpp"

using namespace vfs2;

TEST(VfsRootTest, Construct) {
    vfs2::VfsRoot vfs;

    {
        IVfsNode *node = nullptr;
        OsStatus status = vfs.create(BuildPath("inventory.txt"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    std::unique_ptr<IVfsNodeHandle> inventory;
    OsStatus status = vfs.open(BuildPath("inventory.txt"), std::out_ptr(inventory));
    ASSERT_EQ(status, OsStatusSuccess);
}

TEST(VfsRootTest, QueryRoot) {
    vfs2::VfsRoot vfs;

    std::unique_ptr<vfs2::IVfsNodeHandle> handle = nullptr;
    OsStatus status = vfs.device(vfs2::VfsPath{}, kOsIdentifyGuid, std::out_ptr(handle));
    ASSERT_EQ(OsStatusSuccess, status);
}
