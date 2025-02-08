#include <gtest/gtest.h>

#include "fs2/path.hpp"

using namespace vfs2;

TEST(VfsPathTest, Empty) {
    VfsPath path;
}

TEST(VfsPathTest, Construct) {
    VfsPath vfs = "System";
}

TEST(VfsPathTest, Verify) {
    ASSERT_TRUE(VerifyPathText("System"));

    // No empty paths
    ASSERT_FALSE(VerifyPathText(""));

    // No invalid characters
    ASSERT_FALSE(VerifyPathText("Sys/tem"));
    ASSERT_FALSE(VerifyPathText("Sys\\tem"));

    // No leading or trailing separators
    ASSERT_FALSE(VerifyPathText("\0System"));
    ASSERT_FALSE(VerifyPathText("System\0"));

    // No empty segments
    ASSERT_FALSE(VerifyPathText(detail::BuildPathText("System", "", "Init")));
}
