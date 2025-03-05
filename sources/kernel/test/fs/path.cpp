#include <gtest/gtest.h>

#include "fs2/path.hpp"

using namespace vfs2;

TEST(VfsPathTest, Empty) {
    VfsPath path;
}

TEST(VfsPathTest, Construct) {
    VfsPath vfs = "System";
}

TEST(VfsPathTest, StringFormat) {
    auto path = detail::BuildPathText("System", "Init");
    ASSERT_EQ(path, VfsString("System\0Init"));
}

TEST(VfsPathTest, Verify) {
    ASSERT_TRUE(VerifyPathText("System"));

    // Empty paths are the root path
    ASSERT_TRUE(VerifyPathText(""));

    // No invalid characters
    ASSERT_FALSE(VerifyPathText("Sys/tem"));
    ASSERT_FALSE(VerifyPathText("Sys\\tem"));

    // No leading or trailing separators
    ASSERT_FALSE(VerifyPathText("\0System"));
    ASSERT_FALSE(VerifyPathText("System\0"));
    ASSERT_FALSE(VerifyPathText("\0System\0"));

    // No empty segments
    ASSERT_FALSE(VerifyPathText(detail::BuildPathText("System", "", "Init")));

    ASSERT_TRUE(VerifyPathText(detail::BuildPathText("System", "Init")));

    ASSERT_FALSE(VerifyPathText(".\0Folder"));
    ASSERT_FALSE(VerifyPathText("Folder\0."));
}

TEST(VfsPathTest, VerifySpecialNames) {
    ASSERT_FALSE(VerifyPathText("Folder\0.\0Subfolder"));
}

TEST(VfsPathTest, SegmentCount) {
    auto path = BuildPath("System", "Init", "Drivers", "DriverConfig.db");
    ASSERT_EQ(path.segmentCount(), 4);
}

TEST(VfsPathTest, SegmentCountOne) {
    auto path = BuildPath("System");
    ASSERT_EQ(path.segmentCount(), 1);
}

TEST(VfsPathTest, Iterate) {
    auto path = BuildPath("System", "Init", "Drivers", "DriverConfig.db");
    auto begin = path.begin();
    auto end = path.end();

    ASSERT_NE(begin, end);

    auto segment = *begin;
    ASSERT_EQ(segment, "System") << std::string_view(segment);
    ++begin;

    segment = *begin;
    ASSERT_EQ(segment, "Init") << std::string_view(segment);
    ++begin;

    segment = *begin;
    ASSERT_EQ(segment, "Drivers") << std::string_view(segment);
    ++begin;

    segment = *begin;
    ASSERT_EQ(segment, "DriverConfig.db") << std::string_view(segment);
    ++begin;

    ASSERT_EQ(begin, end);
}

TEST(VfsPathTest, Parent) {
    auto path = BuildPath("System", "Init", "Drivers", "DriverConfig.db");
    auto parent = path.parent();

    auto expected = BuildPath("System", "Init", "Drivers");

    ASSERT_EQ(parent, expected) << std::string_view(expected.string()) << " != " << std::string_view(parent.string());
}

TEST(VfsPathTest, FileName) {
    auto path = BuildPath("System", "Init", "Drivers", "DriverConfig.db");

    auto name = path.name();
    ASSERT_EQ(name, "DriverConfig.db") << std::string_view(name);
}

TEST(VfsPathTest, FolderName) {
    auto path = BuildPath("System", "Init", "Drivers");
    auto name = path.name();

    ASSERT_EQ(name, "Drivers") << std::string_view(name);
}

TEST(VfsPathTest, SingleName) {
    auto path = BuildPath("Root");
    auto name = path.name();

    ASSERT_EQ(name, "Root") << std::string_view(name);
}

struct StringStream : public km::IOutStream {
    std::string buffer;

    void write(stdx::StringView message) override {
        buffer.append(std::string_view(message));
    }
};

TEST(VfsPathTest, FormatOut) {
    auto path = BuildPath("System", "Init", "Drivers", "DriverConfig.db");
    StringStream result;
    result.format(path);

    ASSERT_EQ(result.buffer, "/System/Init/Drivers/DriverConfig.db");
}

TEST(VfsPathTest, RootPath) {
    auto path = BuildPath("");
    ASSERT_EQ(path.segmentCount(), 0);
}
