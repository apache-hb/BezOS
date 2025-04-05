
#include <gtest/gtest.h>

#include "fs2/vfs.hpp"

using namespace vfs2;

TEST(RamFsTest, Construct) {
    vfs2::VfsRoot vfs;
}

TEST(RamFsTest, DestroyNode) {
    vfs2::VfsRoot vfs;

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
        NodeStat stat{};
        OsStatus status = inventory->stat(&stat);
        ASSERT_EQ(OsStatusSuccess, status);
    }
}
