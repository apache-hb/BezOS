
#include <gtest/gtest.h>

#include "fs/vfs.hpp"

using namespace vfs;

TEST(RamFsTest, Construct) {
    vfs::VfsRoot vfs;
}

TEST(RamFsTest, DestroyNode) {
    vfs::VfsRoot vfs;

    sm::RcuSharedPtr<INode> node = nullptr;
    {
        OsStatus status = vfs.create(BuildPath("inventory.txt"), &node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    std::unique_ptr<IFileHandle> inventory;
    OsStatus status = vfs.open(BuildPath("inventory.txt"), std::out_ptr(inventory));
    ASSERT_EQ(status, OsStatusSuccess);

    {
        OsStatus status = vfs.remove(node);
        ASSERT_EQ(OsStatusSuccess, status);
    }

    {
        // eventually we want to return not found for this, but for now
        // just check that we dont crash.
        OsFileInfo stat{};
        OsStatus status = inventory->stat(&stat);
        ASSERT_EQ(OsStatusSuccess, status);
    }
}
