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
