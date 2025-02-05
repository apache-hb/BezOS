#include <gtest/gtest.h>

#include "fs/vfs.hpp"

void DefaultInternalLog(absl::LogSeverity, const char *, int, std::string_view message) {
    std::cout << "Internal log: " << message << std::endl;
}

constinit absl::base_internal::AtomicHook<absl::raw_log_internal::InternalLogFunction>
        absl::raw_log_internal::internal_log_function(DefaultInternalLog);

using namespace stdx::literals;

TEST(VfsTest, Construct) {
    km::VirtualFileSystem vfs;
}

TEST(VfsText, CreateRoot) {
    km::VirtualFileSystem vfs;
    km::VfsEntry *entry = vfs.mkdir("/"_sv);

    ASSERT_NE(entry, nullptr);
    ASSERT_EQ(entry->type(), km::VfsEntryType::eFolder);
    ASSERT_NE(entry->node(), nullptr);
    ASSERT_NE(entry->mount(), nullptr);
}

TEST(VfsTest, CreateFolder) {
    km::VirtualFileSystem vfs;
    vfs.mkdir("/"_sv);
    km::VfsEntry *entry = vfs.mkdir("/Users"_sv);

    ASSERT_NE(entry, nullptr);
    ASSERT_EQ(entry->type(), km::VfsEntryType::eFolder);
    ASSERT_NE(entry->node(), nullptr);
    ASSERT_NE(entry->mount(), nullptr);
}

TEST(VfsTest, CreateFile) {
    km::VirtualFileSystem vfs;
    vfs.mkdir("/"_sv);
    vfs.mkdir("/Users"_sv);

    vfs.mkdir("/Users/Guest"_sv);
    std::unique_ptr<km::VfsHandle> file{vfs.open("/Users/Guest/motd.txt"_sv)};

    ASSERT_NE(file, nullptr);

    size_t write = 0;
    KmStatus status = file->write("Welcome.\n", 9, &write);
    ASSERT_EQ(status, ERROR_SUCCESS);
    ASSERT_EQ(write, 9);

    std::unique_ptr<km::VfsHandle> read{vfs.open("/Users/Guest/motd.txt"_sv)};

    ASSERT_NE(read, nullptr);

    char buffer[256];
    size_t readSize = 0;
    status = read->read(buffer, 256, &readSize);
    ASSERT_EQ(status, ERROR_SUCCESS);
    ASSERT_EQ(readSize, 9);

    buffer[readSize] = '\0';
    ASSERT_STREQ(buffer, "Welcome.\n");
}
